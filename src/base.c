/*
 * Copyright (C) 2012, 2013, 2014 Karol Babioch <karol@babioch.de>
 * Copyright (c) 2010 Frank Meyer - frank(at)fli4l.de
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
 * @file base.c
 * @brief Contains the implementation of functions declared in base.h
 *
 * This file contains the actual implementation of various useful and/or
 * basic functions as declared in base.h.
 *
 * @see base.h
 */

#include <inttypes.h>
#include <string.h>

#include "base.h"

/**
 * @brief Converts a pair of BCD values into an integer representation
 *
 * This function expects a pair of BCD values in one single byte.
 * The first BCD value should take up the first four bits (0-3),
 * the second BCD value the next four bits (4-7).
 *
 * The first BCD value (bits 0-3) is considered to represent the least
 * significant digit. The second BCD value (bits 4-7) on the other hand
 * represents the most significant digit.
 *
 * To clarify this consider the example `00110101`<sub>2</sub>: The first
 * nibble (bits 4-7) represents the digit `3`<sub>10</sub>. This actually
 * stands for the group of tens. The second nibble (bits 0-3) represents the
 * digit `5`<sub>10</sub>, which stands just for itself in this context (least
 * significant digit). The result then would be `35`<sub>10</sub>.
 *
 * This function obviously returns only values between 0 and 99.
 *
 * @param bcd Pair of BCD values
 *
 * @return Converted integer
 *
 * @see itobcd()
 */
uint8_t bcdtoi(uint8_t bcd)
{

    uint8_t i = 10 * (bcd >> 4) + (bcd & 0x0F);

    return i;

}

/**
 * @brief Converts an integer into a pair of BCD values
 *
 * This function expects an integer, which it will then convert into a pair of
 * BCD values stored in one single byte. The first BCD value will take up the
 * first four bits (0-3), the second BCD value will take up the remaining bits (4-7).
 *
 * The first BCD value (bits 0-3) will represent the least significant digit.
 * The second BCD value (bits 4-7) on the other hand will represent the most
 * significant digit.
 *
 * To clarify this consider the example `95`<sub>10</sub>: The most significant
 * digit is `9`<sub>10</sub>, which in binary is `1001`<sub>2</sub>.
 * `5`<sub>10</sub> is the least significant digit here, and in binary is
 * represented by `0101`<sub>2</sub>. These two nibbles will be then returned
 * in the form of a single byte, namely `10010101`<sub>2</sub>.
 *
 * This function obviously can only work with values from 0 up to 99.
 *
 * @param i Integer to convert
 *
 * @return Pair of BCD values
 *
 * @see bcdtoi()
 */
uint8_t itobcd(uint8_t i)
{

    uint8_t bcd;
    uint8_t r;

    bcd = div10(i, &r) << 4;
    bcd |= r;

    return bcd;

}

/**
 * @brief Increments and/or decrements a value by one within a given range
 *
 * This is meant to increment and/or decrement the value pointed to by "val" by
 * one. The given boundaries ("min", "max") are checked and the value won't be
 * incremented and/or decremented beyond that. The "dir" parameter expects
 * either "-1" for decrementing the given value and/or "+1" for incrementing
 * it.
 *
 * @warning This function does not check the actual value of "dir" and hence
 * other values than "-1" and/or "+1" are possible, but might not be senseful.
 *
 * @param val Pointer to value to be increased and/or decreased by one
 * @param dir Direction: -1 down, +1 up
 * @param min Allowed minimum value
 * @param max Allowed maximum value
 */
void incDecRange(uint8_t* val, int8_t dir, uint8_t min, uint8_t max)
{

    if (dir < 0) {

        if (*val > min) {

            *val += dir;

        }

    } else {

        if (*val < max) {

            *val +=dir;

        }

    }

}

/**
 * @brief Increments and/or decrements given value and overflows if necessary
 *
 * This is meant to increment and/or decrement the value pointed to by "val" by
 * the amount specified in "opr". "val" is expected to range from 0 to "max".
 * When incrementing "val" would result in an overflow, the value of "val" will
 * be set to 0. When decrementing "val" by an operand greater than itself, the
 * "value" of "val" will overflow by this amount.
 *
 * @warning This function does not check whether "opr" actually lies in between
 * 0 and "max", which might result with unexpected values.
 *
 * @param val Pointer to value to be increased and/or decreased by "opr"
 * @param opr Operand by which to increment and/or decrement, range: -max - max
 * @param max Allowed maximum value
 */
void incDecRangeOverflow(uint8_t* val, int8_t opr, uint8_t max)
{

    if (opr < 0) {

        if (*val < -opr) {

            *val = max + 1 + opr;

            return;

        }

    }

    *val += opr;

    if (*val > max) {

        *val = 0;

    }

}
