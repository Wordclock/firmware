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

#include <IRMP/irmp.h>

#include "brightness.h"
#include "config.h"
#include "datetime.h"
#include "dcf77.h"
#include "display.h"
#include "ldr.h"
#include "log.h"
#include "pwm.h"
#include "timer.h"
#include "user.h"
#include "uart.h"
#include "base.h"
#include "wceeprom.h"

/**
* @brief Contains the MCU status register
*
* @note To prevent this variable from being cleared by any initialization
* routines, it is put into the `.noinit` section.
*
* @note For now this variable is declared static as it is not used by any other
* module.
*
* @see reset_mcusr()
*/
static uint8_t mcusr __attribute__ ((section(".noinit")));

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

    uart_init();

    #if (LOG_ENABLE_DEFAULT == 1)

        log_enable();

    #endif

    // Set default log level for this module
    log_set_level(LOG_MODULE_MAIN, LOG_LEVEL_MAIN_DEFAULT);
    log_output_P(LOG_MODULE_MAIN, LOG_LEVEL_INFO, "Init started");

    wcEeprom_init();

    #if (ENABLE_DCF_SUPPORT == 1)

        dcf77_init();

    #endif

    display_init();
    datetime_init();
    ldr_init();
    pwm_init();
    brightness_init();
    irmp_init();
    timer_init();
    user_init();

    sei();

    pwm_on();

    log_output_P(LOG_MODULE_MAIN, LOG_LEVEL_INFO, "Init finished");

    while (1) {

        brightness_handle();
        datetime_handle();
        handle_ir_code();
        uart_protocol_handle();

        #if (ENABLE_DCF_SUPPORT == 1)

            datetime_t dt;

            // TODO: Make sure dcf77_getDateTime() validates its result
            if (dcf77_getDateTime(&dt)) {

                datetime_set(&dt);

            }

        #endif

    }

}

void reset_mcusr() __attribute__((naked)) __attribute__((section(".init0")));

/**
* @brief Saves the MCU status register in case wordboot is being used
*
* This saves the `MCUSR` register and puts it into {@link #mcusr}. In case of
* wordboot being used, the register was already saved and its content was put
* into `r2`, so it is read from there. Otherwise `MCUSR` is being read directly
* and reset afterwards.
*
* Furthermore the watchdog is being disabled, to prevent infinite boot loops.
*
* @note This function is not actually called, but put into the `.init0` section
* automatically.
*
* @see mcusr
*/
void reset_mcusr()
{

    // Read r2 and put its content into mcusr
    __asm__ __volatile__ ("mov %0, r2\n" : "=r" (mcusr));

    // Check for normal boot (without bootloader)
    if (!mcusr) {

        mcusr = MCUSR;
        MCUSR = 0;
        wdt_disable();

    }

}
