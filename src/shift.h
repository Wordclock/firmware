/*
 * Copyright (C) 2012, 2013 Karol Babioch <karol@babioch.de>
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
 * @file shift.h
 * @brief Interface to the shift register cascade
 *
 * This module is used to output data to the shift registers. It makes use of
 * the SPI interface. It needs to be initialized once using shift24_init().
 * Afterwards data can be output by using shift24_output().
 *
 * @see shift.c
 */

#ifndef _WC_SHIFT_H_
#define _WC_SHIFT_H_

#include "avr/io.h"

#define SHIFT_SR_SPI_DDR  DDRB
#define SHIFT_SR_SPI_PORT PORTB
#define SHIFT_SR_SPI_MOSI PIN3
#define SHIFT_SR_SPI_MISO PIN4 /* not used, but has to be input*/
#define SHIFT_SR_SPI_RCLK PIN2
#define SHIFT_SR_SPI_SCK  PIN5

extern void		shift24_init (void);
extern void		shift24_output (uint32_t value);

#endif /* _WC_SHIFT_H_ */
