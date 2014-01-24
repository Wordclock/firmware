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
 * This module is based upon a concept known as Linear congruential generator,
 * refer to [1] for details. It is based upon Simon Küppers code posted at [2].
 * However, the posted version contains a bug in regards to the multiplier that
 * renders it quite useless. This implementation fixes it by using a different
 * multiplier, which is justified by [3]. The periodicity of this PRNG is 256,
 * which is exactly the width of the data types used internally.
 *
 * [1]: https://en.wikipedia.org/wiki/Linear_congruential_generator
 * [2]: http://www.mikrocontroller.net/topic/112114#998347
 * [3]: https://www.mikrocontroller.net/topic/112114#998667
 *
 * @see prng.h
 */

#include "prng.h"

/**
 * @brief Multiplier used for calculating the next random number
 *
 * @see PRNG_INCREMENT
 * @see prng_rand()
 */
#define PRNG_MULTIPLIER 17

/**
 * @brief Summand used for calculating the next random number
 *
 * @see PRNG_MULTIPLIER
 * @see prng_rand()
 */
#define PRNG_INCREMENT 37

/**
 * @brief Last generated random number and/or seed value
 *
 * This contains either the last random number generated by `prng_rand()`
 * and/or the seed value set by `prng_set_seed()`.
 *
 * In either case it is used as the basis for the next random number requested
 * by `prng_rand()`.
 *
 * @see prng_set_seed()
 * @see prng_rand()
 */
static uint8_t seed = 1;

/**
 * @brief Sets the seed value
 *
 * This sets the seed value, which the next requested random number will be
 * based upon.
 *
 * @param value The seed value you want to set
 *
 * @see seed
 * @see prng_rand()
 */
void prng_set_seed(uint8_t value)
{

    seed = value;

}

/**
 * @brief Returns a pseudo random number
 *
 * The pseudo random number is generated by a simple calculation based upon
 * the current value of `seed`, `PRNG_MULTIPLIER` and `PRNG_INCREMENT`.
 *
 * @note The modulo 256 operation is done implicitly as the data type is
 * uint8_t.
 *
 * @return The next pseudo random number
 *
 * @see seed
 * @see PRNG_MULTIPLIER
 * @see PRNG_INCREMENT
 */
uint8_t prng_rand()
{

    seed = (seed * PRNG_MULTIPLIER) + PRNG_INCREMENT;

    return seed;

}
