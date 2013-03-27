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

/*------------------------------------------------------------------------------------------------------------------------------------------------*//**
 * @file simple_random.h
 *
 *  This file implements a very simple 8bit random number generator.
 *
 *  \details
 *     Main Prinziple: http://en.wikipedia.org/wiki/Linear_congruential_generator\n
 *     This is based on Simon Küppers code posted here: \n
 *         http://www.mikrocontroller.net/topic/112114#998347 \n
 *     The postet version has a bug in the multiplikator that makes it unusable.
 *     This is fixed by using 17 as multiplikator.
 *     With the values 17/37 it has a peridicity of 256.
 *
 *
 * \version $Id: $
 *
 * \author Copyright (c) 2013 Vlad Tepesch
 *
 * \remarks
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 */
 /*-----------------------------------------------------------------------------------------------------------------------------------------------*/

#ifndef _WC_SIMPLE_RNG_H_
#define _WC_SIMPLE_RNG_H_

#include <stdint.h>

/** initializes the RNG with the given value*/
void simpleRand_setSeed(uint8_t i_seed);

/** returns a pseudo random byte */
uint8_t simpleRand_get();

#endif //_WC_SIMPLE_RNG_H_
