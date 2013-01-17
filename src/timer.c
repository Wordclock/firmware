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
 * @file timer.c
 * @brief Implementation of interface for access to the timer to execute
 * functions on a regular basis
 *
 * This file contains the implementation of the interface for access to the timer
 * to execute functions on a regular basis. Furthermore the appropriate timer
 * ISR is registered and defined here.
 *
 * For a detailed description of this unit see [1], p. 108f.
 *
 * The various functions needed to be called on a regular basis can simply be
 * added to the appropriate macro definitions, that is INTERRUPT_10000HZ up to
 * INTERRUPT_1M. The names should be pretty self explanatory.
 *
 * These macros will then be added to the ISR, which will call them
 * appropriately.
 *
 * The frequency of the ISR itself can be defined with F_INTERRUPT.
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

#include "main.h"
#include "timer.h"
#include "dcf77.h"
#include "Irmp/irmp.h"
#include "ldr.h"
#include "user.h"
#include "display.h"

#include "uart.h"
#include "wceeprom.h"

/**
 * @brief Number of interrupts per second (frequency)
 *
 * This defines how often per second the ISR itself will be executed. Within
 * the ISR various functions defined in the INTERRUPT_* macros will then be
 * called.
 *
 * Obviously enough this should be bigger and/or equal to the biggest
 * INTERRUPT_* macro, namely INTERRUPT_10000HZ
 */
#define F_INTERRUPT 10000

/**
 * @brief List of functions that should be called 10000 times a second
 */
#define INTERRUPT_10000HZ { irmp_ISR(); }

/**
 * @brief List of functions that should be called 1000 times a second
 */
#define INTERRUPT_1000HZ { }

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
#define INTERRUPT_1HZ { main_ISR(); ldr_ISR(); user_isr1Hz(); }

/**
 * @brief List of functions that should be called once a minute
 */
#define INTERRUPT_1M { }

/**
 * @brief Initializes the timers
 *
 * This function has to be called once, which will initialize the timers
 * and - presumed that interrupts are enabled - will execute the appropriate
 * ISRs regularly.
 *
 * You'll find a description of the various registers used in this function at
 * [1], p. 130f, section 16.11.
 *
 * [1]: http://www.atmel.com/images/doc2545.pdf
 *
 * @see ISR(TIMER1_CAPT_vect)
 */
void timer_init(void)
{

	/*
	 * Input capture register is used as compare value. The formula below
	 * ensures that the ISR will be triggered as often as defined in
	 * F_INTERRUPT.
	 */
	ICR1 = (F_CPU / F_INTERRUPT) - 1;

	/*
	 * Set up Timer/Counter1
	 *
	 * See [1], p. 132, Table 16-4 for an overview of the available modes
	 * See [1], p. 133, Table 16-5 for an overview of the available prescalers
	 *
	 * [1]: http://www.atmel.com/images/doc2545.pdf
	 *
	 * Mode: 14 (Fast PWM)
	 * Prescaler: 1
	 */
	TCCR1A = _BV(WGM11);
	TCCR1B = _BV(WGM13) | _BV(WGM12) | _BV(CS10);

	/*
	 * ICIE1: Input capture interrupt enable
	 */
	TIMSK1 = 1 << ICIE1;

}

/**
 * @brief Timer/Counter1 compare handler (ISR)
 *
 * This function is called with the frequency defined in F_INTERRUPT. It
 * has some internal counters to divide this frequency down into the various
 * smaller frequencies (e.g. INTERRUPT_1000HZ to INTERRUPT_1M). It then, if
 * necessary calls the functions defined in these macros.
 *
 * The timer has to be initialized using timer_init() **before** this is
 * actually executed.
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

	/*
	 * Variables needed to divide the ISR frequency down to smaller frequencies
	 */
	static uint8_t thousands_counter;
	static uint8_t hundreds_counter;
	static uint8_t tenths_counter;
	static uint8_t seconds_counter;
	static uint8_t minutes_counter;

	/*
	 * The following part works by incrementing the thousands_counter each
	 * time the ISR is called. It is then compared against 10, which of course
	 * will only be true every tenth time. In case this comparison returns
	 * false, the execution of the ISR is ended with a simple return. Otherwise
	 * the next smaller counter will be incremented and once again compared
	 * against ten. This is done until the desired resolution of one minute
	 * is reached. Between each of these comparisons the list of macros defined
	 * above is added, so that the appropriate functions will get called with
	 * the specified frequency.
	 */

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
