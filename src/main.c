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

#define READ_DATETIME_INTERVAL 15

static volatile uint8_t soft_seconds;

#if (LOG_MAIN == 1)

    #define log_main(x) uart_puts_P(x)

#else

    #define log_main(x)

#endif

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

    void wdt_init()
    {

        MCUSR = 0;
        wdt_disable();

    }

#endif

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

void main_ISR()
{

    soft_seconds++;

}
