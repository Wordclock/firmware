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
 * @file ldr.h
 * @brief Header file allowing to access brightness measurements from the LDR
 *
 * This module handles the access to the brightness measured by the LDR sensor.
 * The returned brightness is the mean value of the last n measurements,
 * where n = `MEASUREMENTS_ARRAY_SIZE`.
 *
 * It is used to provide ambient light influenced behavior, e.g. increasing the
 * brightness of the LEDs involved when the light in the room the Wordclock is
 * in is switched on and/or changed.
 *
 * @see ldr.c
 */

#ifndef _WC_LDR_H_
#define _WC_LDR_H_

#include <avr/io.h>

/**
 * @brief Number of measurements taken account for calculating the brightness
 *
 * The brightness calculated by `ldr_get_brightness()` will be the mean value
 * of the values stored in an array of this size, which effectively acts as a
 * low pass filter.
 *
 * @note For performance reasons this should be a multiple of two.
 *
 * @see ldr_get_brightness()
 */
#define MEASUREMENTS_ARRAY_SIZE 16

extern void ldr_init();

extern uint8_t ldr_get_brightness();

/**
 * @brief Starts a new ADC conversion
 *
 * This is executed within `INTERRUPT_1HZ` to start a new ADC measurement each
 * second. However, before this can be used the module has to initialized first
 * using `ldr_init()`.
 *
 * The measurement itself is being processed by the ISR `ISR(ADC_vect)`.
 *
 * @see ldr_init()
 * @see INTERRUPT_1HZ
 * @see ISR(ADC_vect)
 */
inline void ldr_ADC() {

    /*
     * ADSC: ADC start conversion
     */
    ADCSRA |= _BV(ADSC);

}

#endif /* _WC_LDR_H_ */
