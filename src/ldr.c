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
 * @brief Implements the interface for handling access to the LDR sensor
 *
 * This file contains the implementation for the interface declared in ldr.h.
 * It makes use of the ADC unit, so it might be useful to take a look at [1],
 * p. 244f, chapter 24.
 *
 * Before you can actually retrieve the "current" brightness using
 * ldr_get_brightness() you need to initialize the module **once** by simply
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
 * @brief Contains the last measurements taken
 *
 * This effectively acts as a low pass filter, as the brightness calculated by
 * ldr_get_brightness() will be the mean value of the values stored in this
 * array. The array is as big as defined in LDR_ARRAY_SIZE.
 *
 * This is actually a [ring buffer][1], which means that LDR_ARRAY_SIZE
 * should be a multiple of two.
 *
 * [1]: https://en.wikipedia.org/wiki/Circular_buffer
 *
 * @see LDR_ARRAY_SIZE
 * @see ldr_get_brightness()
 * @see ldr_ISR()
 */
static volatile uint8_t             array[LDR_ARRAY_SIZE];

/**
 * @brief Contains the sum of all elements in array
 *
 * Although this is redundant and could be calculated by simply adding up the
 * values in array itself, it makes things faster when it comes down to
 * actually returning the "current" brightness with ldr_get_brightness().
 *
 * \warning: Consider that the datatype must be able to store values at least
 * as big as LDR_ARRAY_SIZE * 255.
 *
 * @see array
 * @see ldr_get_brightness()
 * @see ldr_ISR()
 */
static volatile uint16_t            curr_sum;



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
void
ldr_init (void)
{
  volatile uint8_t result;
 
  ADMUX = 0                       // ADC0
        | (1<<REFS0)              // use Avcc
        | (1<<ADLAR);             // left justify result in ADCH
 
  ADCSRA = (1<<ADEN) | (1<<ADPS2) | (1<<ADPS0);      // prescaler to 32 / in our case sampling frequence of 250kHz
                                                     // and activate ADC

  ADCSRA |= (1<<ADSC);                      // ADC start 
  while ( ADCSRA & (1<<ADSC) ) {
    ;     
  }

  result = ADCH;  // read out the value and init the summary array
  for (int i = 0; i < LDR_ARRAY_SIZE; array[i++] = result);

  curr_sum = result;              // also init the sum
  curr_sum *= LDR_ARRAY_SIZE;     //

# if (LOG_LDR == 1)
  {
    char buff[5];
    byteToStr(result, buff);
    uart_puts_P("LDR init: ");
    uart_puts(buff);
    uart_putc('\n');
  }
# endif

  ADCSRA |= (1<<ADSC);     // start next measurement (will be read in 1Hz Interupt)

  return;
}

/**
 * @brief Returns the "current" brightness - calculated from the last taken
 * measurements
 *
 * Before this function can be called, the module first needs to be initialized
 * by calling ldr_init().
 *
 * This function returns the "current" brightness. "Current" means that it
 * is actually calculated from the last taken measurements. The number of
 * measurements taken into account is defined in LDR_ARRAY_SIZE.
 *
 * The returned value is actually the mean value of the last measurements.
 * This makes it more robust against sudden changes of the ambient light.
 *
 *
 * @see ldr_init()
 * @see LDR_ARRAY_SIZE
 * @return Eight bit value containing the brightness. 255 represents the
 * 			maximum brightness, 0 represents darkness.
 */
uint8_t
ldr_get_brightness (void)
{
  return ( 255-(curr_sum / LDR_ARRAY_SIZE)); 
}


/**
 * @brief Takes the measurement and recalculates the new value to return
 *
 * This function is the actual workhorse. It should be called regularly, e.g.
 * once every second. This is achieved by using the various macros defined in
 * timer.c, namely INTERRUPT_1HZ.
 *
 * It will read out the value from the ADC measurement started in the last
 * cycle and put it into array. It will then also recalculate the new value
 * for curr_sum, which is needed by ldr_get_brightness().
 *
 * If logging is enabled it will also output the value of the measurement
 * taken.
 *
 * It uses an counter in order to keep track of the current index for array.
 * It uses a modulo operation to get the new value of this index, which is
 * reason why LDR_ARRAY_SIZE should be a multiple of 2, so this can be
 * performed by a appropriate bit shift.
 *
 * @see array
 * @see curr_sum
 * @see LOG_LDR
 * @see LDR_ARRAY_SIZE
 */
void
ldr_ISR (void)
{
  static uint8_t   curr_index = 0;

  if ( (ADCSRA & (1<<ADSC)) == 0) {
    // read out last conversion and recalculating summary
    uint8_t measurement = ADCH;

# if (LOG_LDR == 1)
  {
    char buff[5];
    byteToStr(measurement, buff);
    uart_puts_P("LDR: ");
    uart_puts(buff);
    uart_putc('\n');
  }
# endif

    curr_sum -= array[curr_index];
    array[curr_index] = measurement;
    curr_sum += measurement;
    curr_index++;

    curr_index %= LDR_ARRAY_SIZE;

    // start next ADC converting
    ADCSRA |= (1<<ADSC); 
  } 

  return;
}
