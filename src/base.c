/*
 * Copyright (C) 2012, 2013 Karol Babioch <karol@babioch.de>
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
 * @brief Converts a single byte (uint8_t) into a string
 *
 * This function converts a given byte (uint8_t) to a string. It takes up
 * considerable less program space than the printf() equivalent.
 *
 * The buffer needs to be large enough to hold at least 4 bytes.
 *
 * @note The string will be NULL terminated automatically.
 *
 * @param val The byte (uint8_t) to convert into a string
 * @param str Pointer to a buffer where the resulting string is stored
 *
 * @see uint8ToStrLessOneHundred()
 */
void uint8ToStr(uint8_t val, char str[4])
{

    uint8_t v = val;
    uint8_t r;

    str[3] = '\0';
    str[1] = ' ';
    str[0] = ' ';
    v = div10(v, &r);
    str[2] = r + '0';

    if (v > 0) {

        v = div10(v, &r);
        str[1] = r + '0';

        if (v > 0) {

            str[0] = v + '0';

        }

    }

}

/**
 * @brief Converts hex representation into byte (uint8_t)
 *
 * This converts a string containing the hex representation of a byte into its
 * binary value (uint8_t). The status parameter indicates whether the operation
 * could be performed successfully and/or whether some unexpected character
 * was detected, in which case the status will be false and the return value
 * has no real meaning.
 *
 * @warning Make sure to check the value of the status variable. The returned
 * value makes only sense if the status variable is true.
 *
 * @param str String containing the hex representation of the byte to convert
 * @param status Pointer to boolean variable indicating success
 *
 * @return Converted byte (uint8_t)
 */
uint8_t hexStrToUint8(char str[2], bool* status)
{

    if (strlen(str) != 2) {

        *status = false;

        return 0;

    }

    uint8_t result;

    if (str[0] >= '0' && str[0] <= '9') {

        result = (str[0] - '0') << 4;

    } else if (str[0] >= 'a' && str[0] <= 'f') {

        result = (10 + str[0] - 'a') << 4;

    } else {

        *status = false;

        return 0;

    }

    if (str[1] >= '0' && str[1] <= '9') {

        result |= (str[1] - '0');

    } else if (str[1] >= 'a' && str[1] <= 'f') {

        result |= (10 + str[1] - 'a');

    } else {

        *status = false;

        return 0;

    }

    *status = true;

    return result;

}

/**
 * @brief Converts a byte (uint8_t) smaller than 100 into a string
 *
 * This function converts a given byte (uint8_t) to a string - quite similar to
 * uint8ToStr(). However this function is restricted to input values from 0 to
 * 99. Therefore the size of the buffer only needs to be 3 bytes big.
 *
 * @note The string will be NULL terminated automatically.
 *
 * @param val The byte (uint8_t) to convert into a string
 * @param str Pointer to a buffer where the resulting string is stored
 *
 * @see uint8ToStr()
 */
void uint8ToStrLessOneHundred(uint8_t val, char str[3])
{

    uint8_t v = val;
    uint8_t r;

    str[2] = '\0';
    v = div10(v, &r);
    str[0] = v ? v + '0' : ' ';
    str[1] = r + '0';

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
