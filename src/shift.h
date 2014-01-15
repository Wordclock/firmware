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
 * @brief Header file for access to the shift register cascade
 *
 * This module is used to output data to the shift registers. It makes use of
 * the SPI hardware interface of the AVR microcontroller itself. The module
 * needs to be initialized `using shift24_init()`. Afterwards data can be
 * output by using `shift24_output()`.
 *
 * @see shift.c
 */

#ifndef _WC_SHIFT_H_
#define _WC_SHIFT_H_

extern void shift24_init();

extern void shift24_output(uint32_t data);

#endif /* _WC_SHIFT_H_ */
