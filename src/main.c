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
 * This file kind of glues together all of the other modules of the project
 * and contains the main entry point.
 *
 *@see config.h
 */

#include <inttypes.h>
#include <avr/io.h>
#include <util/delay.h>
#include <avr/pgmspace.h>
#include <avr/interrupt.h>
#include <avr/wdt.h>

#include "config.h"
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

/*
 * Make sure F_CPU is set
 */
#ifndef F_CPU

    #error F_CPU unknown!

#endif

/**
 * @brief Defines the interval the time should be (re)read from the RTC
 *
 * This defines the interval (in seconds) which can pass at most before the
 * time of the software clock is synchronized with the RTC again.
 *
 * @see handle_datetime()
 */
#define READ_DATETIME_INTERVAL 15

/**
 * @brief Used to keep track of the seconds of the software
 *
 * This keeps track of the seconds of the software clock internally. It is
 * incremented each second by `main_ISR()` and processed within
 * `handle_datetime()`. It is synchronized with the RTC when an interval of at
 * most `READ_DATETIME_INTERVAL` has passed.
 *
 * @see main_ISR()
 * @see READ_DATETIME_INTERVAL
 */
static volatile uint8_t soft_seconds;

#if (LOG_MAIN == 1)

    /**
     * @brief Used to output logging information of this module
     *
     * @see LOG_MAIN
     */
    #define log_main(x) uart_puts_P(x)

#else

    /**
     * @brief Dummy in case LOG_MAIN is disabled
     *
     * @see LOG_MAIN
     */
    #define log_main(x)

#endif

/**
 * @brief Handles the synchronization between the RTC and the software clock
 *
 * The time is being kept track of using a software clock and is **not**
 * constantly synchronized with the RTC itself. In order for this software
 * clock to be more accurate, it will adjust itself in a way described by
 * the following:
 *
 * - In case it runs too slow (for instance due to the RC oscillator) this
 *   function will poll the RTC for the current time every second in the last
 *   part of the minute in order to reach the transition to the next minute
 *   as close as possible.
 *
 * - On the other hand it will slowdown the polling if it is detected that the
 *   software clock runs too fast, so that the software clock is updated less
 *   often.
 *
 * This function also (re)enables the DCF77 decoding once a new hour begins.
 *
 * @see READ_DATETIME_INTERVAL
 * @see soft_seconds
 * @see i2c_rtc_read()
 * @see dcf77_enable()
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

        /*
         * Check if RTC should be (re)read again
         */
        if (soft_seconds >= next_read_seconds) {

            rtc = i2c_rtc_read(datetime);

        } else {

            datetime->ss = soft_seconds;
            rtc = true;

        }

        if (rtc) {

            /*
             * Check whether new minute has begun
             */
            if (last_minute != datetime->mm) {

                user_setNewTime(datetime);
                last_minute = datetime->mm;

                /*
                 * Check whether new hour has begun
                 */
                if (last_hour != datetime->hh) {

                    #if (ENABLE_DCF_SUPPORT == 1)

                        dcf77_enable();

                    #endif

                    last_hour = datetime->hh;

                }

            }

            /*
             * Check whether software clock is running too fast
             */
            if (last_seconds != 0xff && soft_seconds > datetime->ss) {

                softclock_too_fast_seconds = soft_seconds - datetime->ss;

            }

            last_seconds = soft_seconds = datetime->ss;

            /*
             * Set next time the RTC should be (re)read
             */
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

    }

}

/**
 * @brief Handles changes to the brightness in response to ambient lightning
 *
 * This functions retrieves the brightness as measured by the LDR and sets a
 * new base brightness step for the PWM generation whenever there are changes
 * in comparison with the last taken measurement.
 *
 * @note Only the most upper five bits of `ldr_get_brightness()` are taken into
 * account, as only base brightness step values from 0 to 31 are valid.
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

            uint8ToStrLessOneHundred(ldr_brightness, buff);
            uart_puts_P("brightness: ");
            uart_puts(buff);
            uart_putc('\n');

        #endif

        pwm_set_base_brightness(ldr_brightness);

        last_ldr_brightness = ldr_brightness;

    }

}

/**
 * @brief Entry point to start execution at
 *
 * This is the main entry point where execution will start. It initializes the
 * hardware and enters an infinite loop handling any upcoming events not yet
 * covered.
 *
 * @note This function makes use of the attribute "OS_main". For details
 * refer to [1].
 *
 * [1]: http://gcc.gnu.org/onlinedocs/gcc/Function-Attributes.html
 *
 * @return Should never return anything
 */
__attribute__((OS_main)) int main()
{

    static datetime_t datetime;

    uart_init();

    log_main("Init...\n");

    wcEeprom_init();

    #if (ENABLE_DCF_SUPPORT == 1)

        dcf77_init();

    #endif

    display_init();

    {

        i2c_master_error_t i2c_rtc_error;

        if (!i2c_rtc_init(&i2c_rtc_error)) {

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
        uart_protocol_handle();

        #if (ENABLE_DCF_SUPPORT == 1)

            if (dcf77_getDateTime(&datetime) && datetime_validate(&datetime)) {

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
 * This is executed once a second and simply increments `soft_seconds` by one.
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
    * @brief Turns off the watchdog after a reset has occurred
    *
    * This disables the watchdog upon each reset, which prevents the
    * microcontroller from resetting itself indefinitely.
    *
    * [1]: http://www.nongnu.org/avr-libc/user-manual/group__avr__watchdog.html
    *
    * @note This function is not actually called, but included in the `init3`
    * section automatically.
    */
    void wdt_init()
    {

        MCUSR = 0;
        wdt_disable();

    }

#endif
