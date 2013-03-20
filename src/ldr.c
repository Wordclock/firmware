/*
 * Copyright (C) 2012, 2013 Karol Babioch <karol@babioch.de>
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
 * ldr_get_brightness() you need to initialize the module **once** by
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

#include "main.h"
#include "uart.h"
#include "ldr.h"
#include "base.h"

/**
 * @brief Stores the last measurements taken
 *
 * This effectively acts as a low pass filter, as the brightness calculated by
 * ldr_get_brightness() will be the mean value of the values stored in this
 * array. The size of the array is defined in MEASUREMENTS_ARRAY_SIZE.
 *
 * This is actually a [ring buffer][1], which means that
 * MEASUREMENTS_ARRAY_SIZE should be a multiple of two.
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
 * actually returning the "current" brightness with ldr_get_brightness().
 *
 * @warning: Consider that the datatype must be able to store values at least
 * as big as MEASUREMENTS_ARRAY_SIZE * 255.
 *
 * @see measurements
 * @see ldr_get_brightness()
 * @see ldr_ISR()
 */
static volatile uint16_t curr_sum;

/**
 * @brief Initializes this module
 *
 * This function needs to be called **once** before any brightness can be
 * retrieved using ldr_get_brightness(). It primarily initializes the ADC
 * unit, see [1], p. 244f, chapter 24 for details.
 *
 * For an overview of the various registers involved take a look at [1],
 * p. 255f.
 *
 * Besides setting up the hardware this function takes the first measurement
 * and saves it, so that ldr_get_brightness() can return it immediately when
 * called afterwards.
 *
 * If the logging for this module is activated (LOG_LDR == 1), it also will
 * output the value of the first measurement.
 *
 * [1]: http://www.atmel.com/images/doc2545.pdf
 *
 * @see curr_sum
 * @see LOG_LDR
 */
void ldr_init(void)
{

    uint8_t result;

    /*
     * Set up the ADC unit
     *
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
     * Start first conversion
     */
    ADCSRA |= _BV(ADSC);

    /*
     * Wait for conversion to finish
     */
    while (ADCSRA & _BV(ADSC));

    /*
     * Read out the measured value
     */
    result = ADCH;

    /*
     * Initialize measurements by putting the measured value in every field
     */
    for (int i = 0; i < MEASUREMENTS_ARRAY_SIZE; i++) {

        measurements[i] = result;

    }

    /*
     * Initialize curr_sum by multiplying it with MEASUREMENTS_ARRAY_SIZE
     */
    curr_sum = result;
    curr_sum *= MEASUREMENTS_ARRAY_SIZE;

    /*
     * Check whether logging is enabled
     */
    # if (LOG_LDR == 1)

        char buff[5];

        byteToStr(result, buff);
        uart_puts_P("LDR init: ");
        uart_puts(buff);
        uart_putc('\n');

    # endif

    /*
     * Start next measurement, which will then be handled by ldr_ISR()
     */
    ADCSRA |= _BV(ADSC);

}

/**
 * @brief Returns the "current" brightness - calculated from the last taken
 * measurements
 *
 * Before this function can be called, the module needs to be initialized
 * by calling ldr_init().
 *
 * This function returns the "current" brightness. "Current" means that it
 * is actually the mean of the last taken measurements. The number of
 * measurements taken into account is defined in MEASUREMENTS_ARRAY_SIZE.
 *
 * The returned value is actually the mean value of the last measurements.
 * This makes it more robust against sudden changes of the ambient light.
 *
 *
 * @see ldr_init()
 * @see MEASUREMENTS_ARRAY_SIZE
 * @return Eight bit value containing the brightness. 255 represents the
 *     maximum brightness, 0 represents darkness.
 */
uint8_t ldr_get_brightness(void)
{

    return  255 - (curr_sum / MEASUREMENTS_ARRAY_SIZE);

}

/**
 * @brief Takes the measurement and recalculates the new value to return
 *
 * This function does the actual work. It should be called regularly, e.g.
 * once every second. This is achieved by using the various macros defined in
 * timer.c, namely INTERRUPT_1HZ.
 *
 * It will read out the value from the ADC measurement started in the last
 * cycle and put it into measurements. It will then also recalculate the new value
 * for curr_sum, which is needed by ldr_get_brightness().
 *
 * If logging is enabled it will also output the value of the measurement
 * taken.
 *
 * It uses an counter in order to keep track of the current index for measurements.
 * It uses a modulo operation to get the new value of this index, which is
 * reason why MEASUREMENTS_ARRAY_SIZE should be a multiple of 2, so this can be
 * performed by a appropriate bit shift.
 *
 * @see measurements
 * @see curr_sum
 * @see LOG_LDR
 * @see MEASUREMENTS_ARRAY_SIZE
 */
void ldr_ISR(void)
{

    /*
     * This counter keeps track of the index we need to put the measurement in
     */
    static uint8_t curr_index = 0;

    /*
     * Check whether last conversion has been completed
     */
    if ((ADCSRA & _BV(ADSC))) {

        /*
         * Read out value of last conversion
         */
        uint8_t measurement = ADCH;

        /*
         * Check whether logging is enabled
         */
        #if (LOG_LDR == 1)

            char buff[5];

            byteToStr(measurement, buff);
            uart_puts_P("LDR: ");
            uart_puts(buff);
            uart_putc('\n');

        #endif

        /*
         * As we are going to replace the value at curr_index with the value
         * of the last conversion, we need to subtract it from curr_sum
         */
        curr_sum -= measurements[curr_index];

        /*
         * Put value of last conversion into measurements
         */
        measurements[curr_index] = measurement;

        /*
         * Add the value of the last measurement to curr_sum
         */
        curr_sum += measurement;

        /*
         * Increment curr_index for next iteration of this function
         */
        curr_index++;

        /*
         * Use modulo operation to get the next value for curr_index,
         * see [ring buffer][1].
         *
         * [1]: https://en.wikipedia.org/wiki/Circular_buffer
         */
        curr_index %= MEASUREMENTS_ARRAY_SIZE;

        /*
         * Start next conversion, which will be handled in the next
         * iteration of this function
         */
        ADCSRA |= _BV(ADSC);

    }

}
