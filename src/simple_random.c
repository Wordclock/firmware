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
 * @file simple_random.c
 * @brief Implementation of the header declared in simple_random.h
 *
 * This module will generate **pseudo** random numbers. The principle is based
 * upon a concept known as Linear congruential generator, refer to [1] for
 * details.
 *
 * This is based upon Simon Küppers code posted at [2]. However the posted
 * version has a bug in regards to the multiplier that makes it unusable. This
 * version fixes it by using 17 as a multiplier, see [3]. The periodicity of
 * this implementation of a RNG is 256.
 *
 * [1]: https://en.wikipedia.org/wiki/Linear_congruential_generator
 * [2]: http://www.mikrocontroller.net/topic/112114#998347
 * [3]: https://www.mikrocontroller.net/topic/112114#998667
 *
 * @see simple_random.h
 */

#include "simple_random.h"

#define SR_PRIME_MULT 17
#define SR_PRIME_ADD  37

static uint8_t g_lastVal=1;

void simpleRand_setSeed(uint8_t i_seed)
{
  g_lastVal = i_seed;
}

uint8_t simpleRand_get()
{
  g_lastVal = (g_lastVal*SR_PRIME_MULT) + SR_PRIME_ADD; // modulo 255 is implicit
  return g_lastVal;
}

