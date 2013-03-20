/*
 * Copyright (C) 2012, 2013 Karol Babioch <karol@babioch.de>
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
#include "Irmp/irmp.h"
#include "timer.h"
#include "user.h"
#include "uart.h"
#include "base.h"
#include "wceeprom.h"

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
 * This function sets the enable_dcf77_ISR flag once a hour to signal to the
 * DCF77 module that it should try to retrieve the current time.
 *
 * @see READ_DATETIME_INTERVAL
 * @see soft_seconds
 * @see i2c_rtc_read()
 * @see enable_dcf77_ISR
 */
static void handle_datetime(datetime_t* datetime)
{

    static uint8_t last_hour = 0xff;
    static uint8_t last_minute = 0xff;
    static uint8_t last_seconds = 0xff;
    static uint8_t next_read_seconds = 0;
    uint8_t softclock_too_fast_seconds = 0;
    uint8_t rtc;

    if (last_seconds != soft_seconds) {

        if (soft_seconds >= next_read_seconds) {

            rtc = i2c_rtc_read(datetime);

        } else {

            datetime->ss = soft_seconds;
            rtc = true;

        }

        if (rtc) {

            if (last_minute != datetime->mm) {

                user_setNewTime(datetime);
                last_minute = datetime->mm;

                if (last_hour != datetime->hh) {

                    #if (DCF_PRESENT == 1)

                        enable_dcf77_ISR = true;

                    #endif

                    last_hour = datetime->hh;

                }

            }

            if (last_seconds != 0xff && soft_seconds > datetime->ss) {

                softclock_too_fast_seconds = soft_seconds - datetime->ss;

            }

            last_seconds = soft_seconds = datetime->ss;

            if (softclock_too_fast_seconds > 0) {

                next_read_seconds = soft_seconds + READ_DATETIME_INTERVAL
                      - softclock_too_fast_seconds;

            } else {

                next_read_seconds = soft_seconds + READ_DATETIME_INTERVAL;

            }

            if (next_read_seconds >= 60) {

                next_read_seconds = 0;

            }

        } else {

            log_main("RTC error\n");

        }

    } else {

        rtc = true;

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

    static uint8_t last_ldr_brightness = 0xff;

    uint8_t ldr_brightness = ldr_get_brightness() >> 3;

    if (last_ldr_brightness != ldr_brightness) {

        #if (LOG_MAIN_BRIGHTNESS == 1)

            char buff[5];

            byteToStrLessOneHundred(ldr_brightness, buff);
            uart_puts_P("brightness: ");
            uart_puts(buff);
            uart_putc('\n');

        #endif

        pwm_set_base_brightness_step(ldr_brightness);
        last_ldr_brightness = ldr_brightness;

    }

}

#if (BOOTLOADER_RESET_WDT == 1)

    void wdt_init() __attribute__((naked)) __attribute__((section(".init3")));

    /**
     * @brief Makes sure that the watchdog is turned of after a watchdog reset
     *
     * In the case that a watchdog reset occured this makes actually sure that
     * the watchdog is turned off to prevent the microcontroller from resetting
     * itself all the time.
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

/**
 * @brief Entry point to start execution at
 *
 * This is the entry point where execution will start. It will initialize the
 * hardware and enter an infinite loop, which will handle any upcoming events
 * not yet covered and/or initiated by various interrupts.
 *
 * @return This function should actually never reach its end
 */
int main()
{

    static datetime_t datetime;

    uart_init();

    log_main("Init...\n");

    wcEeprom_init();

    #if (DCF_PRESENT == 1)

        dcf77_init();

    #endif

    display_init();

    {

        uint8_t i2c_errorcode;
        uint8_t i2c_status;

        if (!i2c_rtc_init(&i2c_errorcode, &i2c_status)) {

            // TODO: error handling
            log_main("RTC init failed\n");

        }

    }

    ldr_init();
    pwm_init();
    irmp_init();
    timer_init();
    user_init();

    sei();

    pwm_on();

    log_main("Init finished\n");

    while (1) {

        handle_brightness();
        handle_datetime(&datetime);
        handle_ir_code();

        #if (DCF_PRESENT == 1)

            if (dcf77_getDateTime(&datetime)) {

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
