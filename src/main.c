/*
 * Copyright (C) 2012, 2013, 2014 Karol Babioch <karol@babioch.de>
 * Copyright (c) 2010 Frank Meyer - frank(at)fli4l.de
 *
 * This file is part of Wordclock.
 *
 * Wordclock is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Wordclock is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Wordclock. If not, see <http://www.gnu.org/licenses/>.
 */

/**
 * @file main.c
 * @brief The main program file
 *
 * This file contains the main function, which is responsible for
 * initialization and actually running the hardware by "gluing" together
 * various modules of the project.
 *
 * @see main()
 * @see main.h
 */

#include <inttypes.h>
#include <avr/io.h>
#include <util/delay.h>
#include <avr/pgmspace.h>
#include <avr/interrupt.h>
#include <avr/wdt.h>

#include "main.h"
#include "dcf77.h"
#include "display.h"
#include "i2c_master.h"
#include "i2c_rtc.h"
#include "ldr.h"
#include "pwm.h"
#include "IRMP/irmp.h"
#include "timer.h"
#include "user.h"
#include "uart.h"
#include "base.h"
#include "wceeprom.h"

#if (ENABLE_UART_PROTOCOL == 1)

    #include "uart_protocol.h"

#endif

/**
 * @brief Interval to (re)read the time from the RTC
 *
 * This defines the interval (in seconds) to (re)read the time from the RTC.
 * Between those reads there is a software clock running (see soft_seconds).
 *
 * @see soft_seconds
 * @see handle_datetime()
 */
#define READ_DATETIME_INTERVAL 15

/**
 * @brief Counter for seconds of software clock
 *
 * This variable is actually needed to run a software clock. It is incremented
 * each second by main_ISR() and used within handle_datetime() quite heavily.
 * After READ_DATETIME_INTERVAL seconds have passed, the time is (re)read from
 * the RTC again.
 *
 * @see main_ISR()
 * @see handle_datetime()
 * @see READ_DATETIME_INTERVAL
 */
static volatile uint8_t soft_seconds;

/*
 * Check whether logging for this module is enabled
 */
#if (LOG_MAIN == 1)

    /**
     * @brief Used to output logging information of this module
     *
     * When the logging for this module is enabled (LOG_MAIN = 1), this macro
     * is used to output various kinds of information.
     *
     * @see LOG_MAIN
     */
    #define log_main(x) uart_puts_P(x)

#else

    /**
     * @brief Used to output logging information of this module
     *
     * This makes sure that nothing is being output when the logging for this
     * module is deactivated (LOG_MAIN = 0).
     *
     * @see LOG_MAIN
     */
    #define log_main(x)

#endif

/**
 * @brief Handles the synchronization between the RTC and the software clock
 *
 * The time itself will only be (re)read from the RTC every
 * READ_DATETIME_INTERVAL seconds. Between those intervals the timekeeping
 * is done by a software clock (see soft_seconds).
 *
 * The software clock can "adjust" itself:
 *
 * - In case it runs too slow (for instance
 * due to the RC oscillator) this function will call i2c_rtc_read() every
 * second in the last part of a minute in order to reach the next full minute
 * as close as possible.
 *
 * - On the other hand it will be slowdowned if the software clock runs too
 * fast, so that the software clock is only updated every
 * READ_DATETIME_INTERVAL - softclock_too_fast_seconds seconds.
 *
 * This function enables the DCF77 reception once an hour to signal to the
 * DCF77 module that it should try to retrieve the current time.
 *
 * @see READ_DATETIME_INTERVAL
 * @see soft_seconds
 * @see i2c_rtc_read()
 * @see dcf77_enable()
 */
static void handle_datetime(datetime_t* datetime)
{

    /*
     * Value of last hour, minute and seconds evaluated during last call
     */
    static uint8_t last_hour = 0xff;
    static uint8_t last_minute = 0xff;
    static uint8_t last_seconds = 0xff;

    /*
     * Time in seconds when to (re)read the RTC
     */
    static uint8_t next_read_seconds = 0;

    /*
     * Difference of seconds when software clock is running too fast
     */
    uint8_t softclock_too_fast_seconds = 0;

    /*
     * Variable for storing status of various RTC operations
     */
    uint8_t rtc;

    /*
     * Check if time has changed
     */
    if (last_seconds != soft_seconds) {

        /*
         * Check if RTC should be (re)read again
         */
        if (soft_seconds >= next_read_seconds) {

            /*
             * (Re)read the RTC
             */
            rtc = i2c_rtc_read(datetime);

        } else {

            /*
             * Time has changed, bu RTC was not (re)read. Set the new time and
             * set status of RTC operation to true.
             */
            datetime->ss = soft_seconds;
            rtc = true;

        }

        /*
         * Check if last RTC operation was successful
         */
        if (rtc) {

            /*
             * Check whether the minute has changed
             */
            if (last_minute != datetime->mm) {

                /*
                 * Set new time and take over the new minute into last_minute
                 */
                user_setNewTime(datetime);
                last_minute = datetime->mm;

                /*
                 * Check if the hour has changed
                 */
                if (last_hour != datetime->hh) {

                    /*
                     * Check if software is compiled with DCF77 functionality
                     */
                    #if (ENABLE_DCF_SUPPORT == 1)

                        /*
                         * Enable DCF77 reception to indicate that the time
                         * should be synchronized using DCF77
                         */
                        dcf77_enable();

                    #endif

                    /*
                     * Take over the new hour into last_hour
                     */
                    last_hour = datetime->hh;

                }

            }

            /*
             * Check if software clock is running too fast
             */
            if (last_seconds != 0xff && soft_seconds > datetime->ss) {

                /*
                 * Calculate difference between soft_seconds actual seconds and
                 * assign it to softclock_too_fast_seconds
                 */
                softclock_too_fast_seconds = soft_seconds - datetime->ss;

            }

            /*
             * Take over the actual value for seconds into last_seconds
             */
            last_seconds = soft_seconds = datetime->ss;

            /*
             * Set next time the RTC should be (re)read by calculating the
             * amount of seconds by some simple math.
             */
            if (softclock_too_fast_seconds > 0) {

                next_read_seconds = soft_seconds + READ_DATETIME_INTERVAL
                      - softclock_too_fast_seconds;

            } else {

                next_read_seconds = soft_seconds + READ_DATETIME_INTERVAL;

            }

            /*
             * Reset next_read_seconds to 0 (meaning the next minute) whenever
             * it is equal to and/or bigger than 60
             */
            if (next_read_seconds >= 60) {

                next_read_seconds = 0;

            }

        } else {

            /*
             * Output error message indicating there was an error during the
             * last operation.
             */
            log_main("RTC error\n");

        }

    }

}

/**
 * @brief Handles changes to the brightness due to changes in the ambient light
 *
 * This functions retrieves the brightness measured by the LDR (only using the
 * most upper five bits) and if there are any changes to the last measurement
 * it sets the base brightness according to the measured value.
 *
 * If logging for this aspect is enabled (LOG_MAIN_BRIGHTNESS = 1) then the
 * measured brightness will also be output.
 *
 * @see ldr_get_brightness()
 * @see LOG_MAIN_BRIGHTNESS
 * @see ldr.h
 */
static void handle_brightness()
{

    /*
     * Value of the last measured LDR brightness
     */
    static uint8_t last_ldr_brightness = 0xff;

    /*
     * Get measured LDR brightness and use only the most upper 5 bits (0 - 31)
     */
    uint8_t ldr_brightness = ldr_get_brightness() >> 3;

    /*
     * Check if there was a difference in brightness
     */
    if (last_ldr_brightness != ldr_brightness) {

        /*
         * Check if logging for this aspect is enabled and output some
         * debugging if necessary.
         */
        #if (LOG_MAIN_BRIGHTNESS == 1)

            char buff[5];

            uint8ToStrLessOneHundred(ldr_brightness, buff);
            uart_puts_P("brightness: ");
            uart_puts(buff);
            uart_putc('\n');

        #endif

        /*
         * Set the base brightness of the PWM generation according to the
         * measured brightness from the LDR.
         */
        pwm_set_base_brightness_step(ldr_brightness);

        /*
         * Assign the just measured brightness from the LDR to
         * "last_ldr_brightness" preparing it for the next time it is called.
         */
        last_ldr_brightness = ldr_brightness;

    }

}

/**
 * @brief Entry point to start execution at
 *
 * This is the entry point where execution will start. It will initialize the
 * hardware and enter an infinite loop, which will handle any upcoming events
 * not yet covered and/or initiated by various interrupts.
 *
 * As this function is the main entry point, it won't actually be called by
 * any other function. Therefore this function makes use of the attribute
 * "OS_main", which will save a couple of bytes as no prologue and/or epilogue
 * is needed to save the content of various registers. The description of
 * this attribute can be found at [1].
 *
 * [1]: http://gcc.gnu.org/onlinedocs/gcc/Function-Attributes.html
 *
 * @return This function should actually never reach its end
 */
__attribute__((OS_main)) int main()
{

    /*
     * Holds the current date & time
     */
    static datetime_t datetime;

    /*
     * Initialize UART module (mainly used for debugging purposes)
     */
    uart_init();

    /*
     * Output debugging information in case it is enabled
     */
    log_main("Init...\n");

    /*
     * Initialize the EEPROM module
     */
    wcEeprom_init();

    /*
     * Check whether DCF77 is enabled
     */
    #if (ENABLE_DCF_SUPPORT == 1)

        /*
         * Initialize DCF77 module
         */
        dcf77_init();

    #endif

    /*
     * Initialize display module
     */
    display_init();

    /*
     * The following segment is declared as "local" as it saves some space
     * on the stack as the variables used therein are only needed for a short
     * period of time and should not occupy place on the stack all the time.
     */
    {

        /*
         * Variables used to keep track of the status of the RTC module
         */
        uint8_t i2c_rtc_errorcode;
        uint8_t i2c_rtc_status;

        /*
         * Initialize RTC module and check if there were any errors
         */
        if (!i2c_rtc_init(&i2c_rtc_errorcode, &i2c_rtc_status)) {

            /*
             * Output error message indicating there was an error
             */
            log_main("RTC init failed\n");

        }

    }

    /*
     * Initialize various other modules
     */
    ldr_init();
    pwm_init();
    irmp_init();
    timer_init();
    user_init();

    /*
     * Enable interrupts globally
     */
    sei();

    /*
     * Enable generation of PWM signals
     */
    pwm_on();

    /*
     * Output debugging information in case it is enabled
     */
    log_main("Init finished\n");

    /*
     * Main loop, which will be where the program should be most of the time
     */
    while (1) {

        /*
         * Check for changes in the brightness and response to them
         */
        handle_brightness();

        /*
         * Check for changes in time and response to them
         */
        handle_datetime(&datetime);

        /*
         * Check for received IR codes and response to them
         */
        handle_ir_code();

        uart_protocol_handle();

        /*
         * Check if DCF77 functionality is enabled
         */
        #if (ENABLE_DCF_SUPPORT == 1)

            /*
             * Check if a valid DCF77 time frame has been received
             */
            if (dcf77_getDateTime(&datetime) && datetime_validate(&datetime)) {

                /*
                 * Set the time according to the just received DCF77 time frame
                 */
                i2c_rtc_write(&datetime);
                soft_seconds = datetime.ss;
                user_setNewTime(&datetime);

            }

        #endif

    }

}

/**
 * @brief ISR of the main module
 *
 * This basically only increases the "soft_seconds" by one each time it is
 * executed. Therefore it should be called every second by adding it
 * to INTERRUPT_1HZ within timer.c.
 *
 * @see INTERRUPT_1HZ
 * @see timer.c
 */
void main_ISR()
{

    soft_seconds++;

}

#if (ENABLE_UART_PROTOCOL == 1)

    void wdt_init() __attribute__((naked)) __attribute__((section(".init3")));

    /**
    * @brief Turns of the watchdog after a watchdog reset has occurred
    *
    * In the case that a watchdog reset occurred this makes sure that the
    * watchdog is turned off to prevent the microcontroller from resetting
    * itself in an infinite loop.
    *
    * [1]: http://www.nongnu.org/avr-libc/user-manual/group__avr__watchdog.html
    *
    * @note This function is not actually called, but included in the "init3"
    * section automatically.
    */
    void wdt_init()
    {

        MCUSR = 0;
        wdt_disable();

    }

#endif
