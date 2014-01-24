/*
 * Copyright (C) 2012, 2013, 2014 Karol Babioch <karol@babioch.de>
 * Copyright (c) 2013 Vlad Tepesch
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
 * @file prng.h
 * @brief Header file of module generating pseudo random number (PRNG)
 *
 * This module provides means to generate 8 bit pseudo random numbers. Though
 * avr-libc implements this on its own, this implementation is significantly
 * smaller, as it doesn't fulfill the requirements of `<stdlib.h>` and is
 * restricted to `uint8_t`.
 *
 * @see prng.c
 */

#ifndef _WC_PRNG_H_
#define _WC_PRNG_H_

#include <stdint.h>

extern void prng_set_seed(uint8_t value);

extern uint8_t prng_rand();

#endif /* _WC_PRNG_H_ */
