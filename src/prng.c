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
 * @file prng.c
 * @brief Implementation of the header declared in prng.h
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
 * @see prng.h
 */

#include "prng.h"

/**
 * @brief The prime multiplier used for calculating the next random number
 *
 * This is used within prng_rand() to calculate the next random number
 * based upon the last generated number and/or the initial seed value.
 *
 * @see SR_PRIME_ADD
 * @see prng_rand()
 */
#define SR_PRIME_MULT 17

/**
 * @brief The prime summand used for calculating the next random number
 *
 * This is used within prng_rand() to calculate the next random number
 * based upon the last generated number and/or the initial seed value.
 *
 * @see SR_PRIME_MULT
 * @see prng_rand()
 *
 */
#define SR_PRIME_ADD 37

/**
 * @brief The last generated random number and/or the initial seed value
 *
 * This contains either the last generated random number (prng_rand())
 * and/or the initial seed value (prng_set_seed()). In either case it
 * is used as the basis for the next random number requested by
 * prng_rand().
 *
 * @see prng_set_seed()
 * @see prng_rand()
 *
 */
static uint8_t g_lastVal = 1;

/**
 * @brief Sets the initial seed value
 *
 * This sets the initial seed value, which the next requested random number
 * (by prng_rand()) will be based upon.
 *
 * @param value The seed value you want to set
 *
 * @see g_lastVal
 * @see prng_rand()
 *
 */
void prng_set_seed(uint8_t value)
{

    g_lastVal = value;

}

/**
 * @brief Returns a pseudo random number
 *
 * The pseudo random number is based upon the last value and/or seed value
 * (g_lastVal).
 *
 * @note The modulo 256 operation is done implicitly as the used data type of
 * g_lastVal is uint8_t.
 *
 * @return The calculated pseudo random number
 *
 * @see g_lastVal
 *
 */
uint8_t prng_rand()
{

    g_lastVal = (g_lastVal * SR_PRIME_MULT) + SR_PRIME_ADD;

    return g_lastVal;

}
