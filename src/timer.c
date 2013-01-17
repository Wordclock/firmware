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
 * @brief Implementation of the interface for access to the timers including
 * the appropriate ISRs
 *
 * This file contains the implementation of the interface for access to the
 * timers. Furthermore the appropriate timer ISRs are registered and defined
 * here.
 *
 * For now the whole project gets along with a single timer, namely
 * Timer/Counter1. For a detailed description of this unit see [1], p. 108f.
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
#define F_INTERRUPT           10000                                             // frequency of interrupts

/**
 * @brief List of functions that should be called 10000 times a second
 */
#define  INTERRUPT_10000HZ    {  irmp_ISR(); }
/**
 * @brief List of functions that should be called 1000 times a second
 */
#define  INTERRUPT_1000HZ     { }
/**
 * @brief List of functions that should be called 100 times a second
 */
#define  INTERRUPT_100HZ      { dcf77_ISR(); user_isr100Hz(); }
/**
 * @brief List of functions that should be called 10 times a second
 */
#define  INTERRUPT_10HZ       { user_isr10Hz(); display_blinkStep(); }
/**
 * @brief List of functions that should be called once a second
 */
#define  INTERRUPT_1HZ        { main_ISR(); ldr_ISR(); user_isr1Hz(); }
/**
 * @brief List of functions that should be called once a minute
 */
#define  INTERRUPT_1M         { }

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
void
timer_init (void)
{
  ICR1    = (F_CPU / F_INTERRUPT) - 1;                           // compare value: 1/10000 of CPU frequency
  TCCR1A  = (1 << WGM11);
  TCCR1B  = (1 << WGM13)|(1 << WGM12) | (1 << CS10);             // switch CTC PWM Mode on, set prescaler to 1
  TIMSK1  = 1 << ICIE1;                                          // ICIE1: Interrupt if timer reaches the Top (ICR1 register)
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
  static uint8_t  thousands_counter;
  static uint8_t  hundreds_counter;
  static uint8_t  tenths_counter;
  static uint8_t  seconds_counter;
  static uint8_t  minutes_counter;

  INTERRUPT_10000HZ;

  thousands_counter++;

  if (thousands_counter != 10)
  {
    return;
  }

  thousands_counter = 0;

  INTERRUPT_1000HZ;

  hundreds_counter++;

  if (hundreds_counter != 10)
  {
    return;
  }

  hundreds_counter = 0;

  INTERRUPT_100HZ;

  tenths_counter++;

  if (tenths_counter != 10)                                                     // generate 10Hz ....
  {
    return;
  }

  tenths_counter = 0;

  INTERRUPT_10HZ;

  seconds_counter++;

  if (seconds_counter != 10)                                                    // generate 1Hz ....
  {
    return;
  }

  seconds_counter = 0;

  INTERRUPT_1HZ;

  minutes_counter++;

  if (minutes_counter != 60)                                                    // generate 1/60 Hz ....
  {
    return;
  }

  minutes_counter = 0;

  INTERRUPT_1M;
}
