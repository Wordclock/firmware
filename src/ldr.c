/*
 * Copyright (C) 2012, 2013, 2014 Karol Babioch <karol@babioch.de>
 * Copyright (c) 2010 René Harsch ( rene@harsch.net )
 * Copyright (c) 2010 Vlad Tepesch
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
 * @file ldr.c
 * @brief Implements the header defined in ldr.h
 *
 * This file contains the implementation of the header declared in ldr.h. It
 * uses the ADC unit. For further details refer to [1], p. 244f, chapter 24.
 *
 * Before you can actually retrieve the "current" brightness using
 * `ldr_get_brightness()` you need to initialize the module **once** by
 * calling ldr_init().
 *
 * [1]: http://www.atmel.com/images/doc2545.pdf
 *
 * @see ldr_init()
 * @see ldr.h
 */

#include <inttypes.h>
#include <avr/io.h>
#include <util/delay.h>
#include <avr/pgmspace.h>
#include <avr/interrupt.h>
#include <stdio.h>

#include "config.h"
#include "format.h"
#include "uart.h"
#include "ldr.h"
#include "base.h"

/**
 * @brief Stores the last measurements taken
 *
 * This effectively acts as a low pass filter, as the brightness calculated by
 * `ldr_get_brightness()` will be the mean value of the values stored in this
 * array.
 *
 * This array is actually organized as a [ring buffer][1].
 *
 * [1]: https://en.wikipedia.org/wiki/Circular_buffer
 *
 * @see MEASUREMENTS_ARRAY_SIZE
 * @see ldr_get_brightness()
 * @see ldr_ISR()
 */
static volatile uint8_t measurements[MEASUREMENTS_ARRAY_SIZE];

/**
 * @brief Contains the sum of all elements in the measurements array
 *
 * Although this is redundant and could be calculated by simply adding up the
 * values in measurements itself, it speeds things up when it comes down to
 * actually returning the "current" brightness with `ldr_get_brightness()`.
 *
 * @note: Consider that the datatype must be able to store values at least as
 * big as MEASUREMENTS_ARRAY_SIZE * 255.
 *
 * @see measurements
 * @see ldr_get_brightness()
 * @see ldr_ISR()
 */
static volatile uint16_t curr_sum;

/**
 * @brief Initializes this module
 *
 * This function initializes the ADC unit and has to be called **once** in
 * order for this module to work correctly. Besides setting up the hardware
 * itself this function takes also the first measurement and saves it, so that
 * `ldr_get_brightness()` could return it immediately. When done initializing
 * the internal state, it activates the ADC interrupt, which will handle the
 * measurements from then on.
 *
 * If the logging for this module is activated (`LOG_LDR`), it will also
 * output the value of the first measurement.
 *
 * @see curr_sum
 * @see LOG_LDR
 * @see ISR(ADC_vect)
 */
void ldr_init()
{

    uint8_t result;

    /*
     * Reference: AVCC with external capacitor at AREF pin
     * Alignment: Left adjusted
     * Channel: 0
     */
    ADMUX = _BV(REFS0) | _BV(ADLAR);

    /*
     * ADEN: Enable ADC
     * ADC prescaler: 32 -> 8 MHz / 32 = 250 kHz
     */
    ADCSRA = _BV(ADEN) | _BV(ADPS2) | _BV(ADPS0);

    /*
     * ADSC: ADC start conversion
     */
    ADCSRA |= _BV(ADSC);

    while (ADCSRA & _BV(ADSC));

    result = ADCH;

    for (uint8_t i = 0; i < MEASUREMENTS_ARRAY_SIZE; i++) {

        measurements[i] = result;

    }

    curr_sum = result;
    curr_sum *= MEASUREMENTS_ARRAY_SIZE;

    #if (LOG_LDR == 1)

        char buff[5];

        sprintf_P(buff, fmt_unsigned_decimal, result);
        uart_puts_P("LDR init: ");
        uart_puts(buff);
        uart_putc('\n');

    #endif

    /*
     * ADIE: ADC interrupt enable
     */
    ADCSRA |= _BV(ADIE);

}

/**
 * @brief Returns the "current" brightness
 *
 * This function returns the "current" brightness. "Current" means that it
 * is actually the mean of all of the last taken measurements.
 *
 * @return Current brightness, 0 = dark, 255 = bright
 *
 * @see ldr_init()
 * @see MEASUREMENTS_ARRAY_SIZE
 */
uint8_t ldr_get_brightness()
{

    return  255 - (curr_sum / MEASUREMENTS_ARRAY_SIZE);

}

/**
 * @brief Processes the last taken measurement
 *
 * This ISR will be executed once a new conversion has been completed. The
 * conversion itself is started using `ldr_ADC()`.
 *
 * It will read out the value from the ADC measurement started in the last
 * cycle and put it into `measurements`. It will then also recalculate the new
 * value for `curr_sum`.
 *
 * If logging is enabled (`LOG_LDR`) it will also output the value of the ADC
 * measurement.
 *
 * @see ldr_ADC()
 * @see measurements
 * @see curr_sum
 * @see LOG_LDR
 * @see MEASUREMENTS_ARRAY_SIZE
 */
ISR(ADC_vect)
{

    static uint8_t curr_index = 0;

    uint8_t measurement = ADCH;

    #if (LOG_LDR == 1)

        char buff[5];

        sprintf_P(buff, fmt_unsigned_decimal, measurement);
        uart_puts_P("LDR: ");
        uart_puts(buff);
        uart_putc('\n');

    #endif

    curr_sum -= measurements[curr_index];
    measurements[curr_index] = measurement;
    curr_sum += measurement;

    curr_index++;

    curr_index %= MEASUREMENTS_ARRAY_SIZE;

}
