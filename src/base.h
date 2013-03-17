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
 * @file base.h
 * @brief Provides some common, useful and basic functions and/or macros
 *
 * The functions and macros declared here are useful on various occasions and
 * are therefore put into a separate module.
 *
 * @see base.c
 */

#ifndef _WC_BASE_H_
#define _WC_BASE_H_

/**
 * @brief Build byte by explicitly setting each bit
 *
 * This can be used to build up a byte manually by explicitly listing all the
 * bits. For instance in order to build up the byte `01101001`<sub>2</sub>,
 * you would use it like that: BIN8(0, 1, 1, 0, 1, 0, 0, 1).
 *
 * b7..b0 are expected to be either 0 and/or 1.
 *
 * @return Byte represented by the given bits.
 * @see BIN16()
 */
#define BIN8(b7, b6, b5, b4, b3, b2, b1, b0) \
    ((b7 << 7) | (b6 << 6) | (b5 << 5) | (b4 << 4) \
    | (b3 << 3) | (b2 << 2) | (b1 << 1) | (b0 << 0))

/**
 * @brief Build short by explicitly setting each bit
 *
 * This can be used to build up a short manually by explicitly listing all the
 * bits it contains in sequence.
 *
 * b15..b0 are expected to be either 0 and/or 1.
 *
 * @return Short represented by the given bits.
 * @see BIN8()
 */
#define BIN16(b15, b14, b13, b12, b11, b10, b9, b8, b7, b6, b5, b4, b3, b2, b1, b0) \
    ((((uint16_t)BIN8(b15, b14, b13, b12, b11, b10, b9, b8)) << 8) \
    | BIN8(b7, b6, b5, b4, b3, b2, b1, b0))

/**
 * @brief Divides an integer by ten while also keeping track of the remainder
 *
 * This function divides the given parameter by ten and returns the resulting
 * integer directly. The remainder of the division will be stored at the
 * location provided by the second argument.
 *
 * This function uses an approximation, which multiplies the given parameter
 * by a constant and shifts it to the right afterwards. The results are valid
 * for input values 0-255 at least.
 *
 * This is only applicable to integers stored in a single byte (uint8_t).
 *
 * @param x Value to divide (dividend)
 * @param remainder Pointer to store the remainder at
 * @return Integer result
 */
static inline uint8_t div10(uint8_t x, uint8_t* remainder)
{

    uint8_t y = ((uint16_t)x * 205) >> 11;
    *remainder = x - (y * 10);

    return y;

}

/**
 * @brief Returns the hex digit representation of a nibble
 *
 * This function expects a nibble - handed over in the four least significant
 * bits of a byte. It then returns the hex representation of it. The
 * byte `00001011`<sub>2</sub> for instance would become `B`<sub>16</sub>.
 *
 * This function expects the upper four bits always to be zero.
 *
 * @param nibble Nibble to convert
 * @return char Hex digit representation
 */
static inline char nibbleToHex(uint8_t nibble)
{

    return nibble + ((nibble < 10) ? '0' : 'A' - 10);

}

/**
 * @brief Converts data into its appropriate hexadecimal representation
 *
 * This function will convert the given data into a string containing the
 * hexadecimal representation of that given data. The result will be stored
 * at the passed buffer. The string will be NULL terminated automatically, so
 * the buffer needs to have a length of 5 bytes at least.
 *
 * @param data Data to convert
 * @param str Pointer to a buffer where the resulting string is stored
 * @return void
 * @see nibbleToHex()
 */
static inline void uint16ToHexStr(uint16_t data, char* str)
{

    str[4] = 0;
    str[3] = nibbleToHex(data & 0xF);
    str[2] = nibbleToHex((data >> 4) & 0xF);
    str[1] = nibbleToHex((data >> 8) & 0xF);
    str[0] = nibbleToHex((data >> 12) & 0xF);

}

extern void byteToStrLessOneHundred(uint8_t val, char str[3]);

extern void byteToStr(uint8_t val, char str[4]);

extern uint8_t bcdtoi(uint8_t bcd);

extern uint8_t itobcd(uint8_t i);

#endif /* _WC_BASE_H_ */
