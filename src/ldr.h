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
 * @file ldr.h
 * @brief Interface for handling access to the LDR sensor
 *
 * This module handles the access to the brightness measured by the LDR sensor.
 * The returned brightness is the mean value of the last couple of
 * measurements, defined by LDR_ARRAY_SIZE.
 *
 * It is used to enable the reaction to changes of the ambient light, e.g.
 * increasing the brightness of the LEDs involved when the light in the room
 * the Wordclock is in is switched on.
 *
 * @see ldr.c
 */

#ifndef _WC_LDR_H_
#define _WC_LDR_H_

/**
 * @brief Number of measurements to take into account for calculating the brightness
 *
 * This effectively acts as a low pass filter. The brightness calculated by
 * ldr_get_brightness() will be the mean value of the values stored in an array
 * of this size.
 *
 * In order to keep the calculations fast this should be an multiple of two,
 * so calculations of the current index can be done by appropriate bit shifts.
 *
 * @see ldr_get_brightness()
 */
#define LDR_ARRAY_SIZE     16




/**  Initialize the adc for ldr measurements.*/
extern void     ldr_init (void);

/**
 *  returns the smoothed brightness
 *  @details  The brightness is low pass filtered.
 *            The mean value from the last LDR_ARRAY_SIZE measurements will be returned.
 *  @return   A 8bit value containing the brightness.\n
 *            Brightness means that the value is inverse proportional 
 *            to the adc results from reading the ldr. \n
 *            A value of 255 means very bright and 0 means very dark.
 */
extern uint8_t  ldr_get_brightness (void);

/**
 *  Handles the reading of the ldr with the ADC.
 *  @details  Has to be called regularly. (intendet to be called at 1Hz)
 */
extern void     ldr_ISR  (void);

#endif /* _WC_LDR_H_ */
