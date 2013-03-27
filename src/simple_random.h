/*
 * Copyright (C) 2012, 2013 Karol Babioch <karol@babioch.de>
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
 * @file simple_random.h
 * @brief Header file of a simple 8 bit random number generator (RNG)
 *
 * This module can be used to generate 8 bit **pseudo** random numbers.
 *
 * @see simple_random.c
 */

#ifndef _WC_SIMPLE_RNG_H_
#define _WC_SIMPLE_RNG_H_

#include <stdint.h>

extern void simpleRand_setSeed(uint8_t i_seed);

extern uint8_t simpleRand_get();

#endif /* _WC_SIMPLE_RNG_H_ */
