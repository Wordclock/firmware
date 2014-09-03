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
 * @file timer.c
 * @brief Implementation of the header declared in timer.h
 *
 * This file contains the implementation of the functionality declared in
 * `timer.h`. Furthermore the appropriate timer ISR is defined here.
 *
 * For a detailed description of the hardware unit involved see [1], p. 108f.
 *
 * Functions, which need to be called on a regular basis can simply be added
 * to the appropriate macro, that is `INTERRUPT_10000HZ` up to `INTERRUPT_1M`.
 *
 * [1]: http://www.atmel.com/images/doc2545.pdf
 *
 * @see timer_init()
 * @see timer.h
 */

#include <inttypes.h>
#include <avr/io.h>
#include <util/delay.h>
#include <avr/pgmspace.h>
#include <avr/interrupt.h>

#include "config.h"
#include "main.h"
#include "timer.h"
#include "dcf77.h"
#include "IRMP/irmp.h"
#include "ldr.h"
#include "user.h"
#include "display.h"
#include "uart.h"
#include "wceeprom.h"

/**
 * @brief Defines how often the timer ISR itself is executed
 *
 * @see ISR(TIMER1_CAPT_vect)
 */
#define F_INTERRUPT 10000

/**
 * @brief List of functions that should be called 10000 times a second
 */
#define INTERRUPT_10000HZ { irmp_ISR(); }

/**
 * @brief List of functions that should be called 1000 times a second
 */
#define INTERRUPT_1000HZ { user_isr1000Hz(); }

/**
 * @brief List of functions that should be called 100 times a second
 */
#define INTERRUPT_100HZ { dcf77_ISR(); user_isr100Hz(); }

/**
 * @brief List of functions that should be called 10 times a second
 */
#define INTERRUPT_10HZ { user_isr10Hz(); display_blinkStep(); }

/**
 * @brief List of functions that should be called once a second
 */
#define INTERRUPT_1HZ { datetime_ISR(); ldr_ADC(); user_isr1Hz(); }

/**
 * @brief List of functions that should be called once a minute
 */
#define INTERRUPT_1M { }

/**
 * @brief Initializes the timer
 *
 * This function initializes the timer and sets it up in a way so that the
 * appropriate ISR will be called regularly
 */
void timer_init()
{

    ICR1 = (F_CPU / F_INTERRUPT) - 1;

    /*
     * Mode: 14 (Fast PWM)
     * Prescaler: 1
     */
    TCCR1A = _BV(WGM11);
    TCCR1B = _BV(WGM13) | _BV(WGM12) | _BV(CS10);

    /*
     * ICIE1: Input capture interrupt enable
     */
    TIMSK1 = _BV(ICIE1);

}

/**
 * @brief Timer/Counter1 compare handler (ISR)
 *
 * This function is called with the frequency defined by F_INTERRUPT. It
 * divides this frequency down into various smaller frequencies and executes
 * the appropriate functions sequentially.
 *
 * @note The timer needs to be initialized before this ISR will be executed.
 *
 * @warning You should make sure that the defined functions are short enough
 * to guarantee correct timings. Otherwise some interrupts could be missed,
 * which obviously would mess things up.
 *
 * @see timer_init()
 * @see F_INTERRUPT
 * @see INTERRUPT_10000HZ
 * @see INTERRUPT_1000HZ
 * @see INTERRUPT_100HZ
 * @see INTERRUPT_10HZ
 * @see INTERRUPT_1HZ
 * @see INTERRUPT_1M
 */
ISR(TIMER1_CAPT_vect)
{

    static uint8_t thousands_counter;
    static uint8_t hundreds_counter;
    static uint8_t tenths_counter;
    static uint8_t seconds_counter;
    static uint8_t minutes_counter;

    INTERRUPT_10000HZ;

    if (++thousands_counter != 10) {

        return;

    }

    thousands_counter = 0;

    INTERRUPT_1000HZ;

    if (++hundreds_counter != 10) {

        return;

    }

    hundreds_counter = 0;

    INTERRUPT_100HZ;

    if (++tenths_counter != 10) {

        return;

    }

    tenths_counter = 0;

    INTERRUPT_10HZ;

    if (++seconds_counter != 10) {

        return;

    }

    seconds_counter = 0;

    INTERRUPT_1HZ;

    minutes_counter++;

    if (minutes_counter != 60) {

        return;

    }

    minutes_counter = 0;

    INTERRUPT_1M;

}
