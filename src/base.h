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
 * you would use it like that: BIN8(01101001), which represents
 * `105`<sub>10</sub>.
 *
 * This is based upon an idea from Yalu X. Refer to [1] for details.
 *
 * [1]: https://www.mikrocontroller.net/topic/180830#1744760
 *
 * @param byte The bit pattern of the byte you want to build
 *
 * @return Byte represented by the given bit pattern
 *
 * @see BIN16()
 * @see BIN32()
 */
#define BIN8(byte) ( \
    0##byte /        01 % 010 << 0 | \
    0##byte /       010 % 010 << 1 | \
    0##byte /      0100 % 010 << 2 | \
    0##byte /     01000 % 010 << 3 | \
    0##byte /    010000 % 010 << 4 | \
    0##byte /   0100000 % 010 << 5 | \
    0##byte /  01000000 % 010 << 6 | \
    0##byte / 010000000 % 010 << 7 )

/**
 * @brief Build short by explicitly setting each bit
 *
 * This can be used to build up a short manually by explicitly listing all the
 * bits it contains in sequence as a pattern. Internally it makes use of
 * BIN8().
 *
 * Example usage: BIN16(10010101, 00111100) = 0x953C = 38204
 *
 * @param msb The most significant byte
 * @param lsb The least significant byte
 *
 * @return Short represented by the two given bit patterns
 *
 * @see BIN8()
 */
#define BIN16(msb, lsb) ((BIN8(msb) << 8) | (BIN8(lsb)))

/**
 * @brief Build short by explicitly setting each bit
 *
 * This can be used to build up a 32 bit constant (Integer) manually by
 * explicitly listing all the bits it contains in sequence as a pattern spit
 * into four bytes.. Internally it makes use of BIN8().
 *
 * Example usage: BIN32(10010101, 00111100, 10101010, 11010101) = 0x953CAAD5
 *  = 2503781077
 *
 * @param b3 Fourth byte (most significant)
 * @param b2 Third byte
 * @param b1 Second byte
 * @param b0 First byte (least significant)
 *
 * @return 32 bit constant (Integer) represented by the given bit patterns
 *
 * @see BIN8()
 */
#define BIN32(b3, b2, b1, b0) ( \
    (BIN8(b3) << 24) | (BIN8(b2) << 16) | (BIN8(b1) << 8) | (BIN8(b0)))

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
 *
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
 *
 * @return char Hex digit representation
 */
static inline char nibbleToHex(uint8_t nibble)
{

    return nibble + ((nibble < 10) ? '0' : 'a' - 10);

}

/**
 * @brief Converts uint8_t into hexadecimal representation
 *
 * This function will convert the given data into a string containing the
 * hexadecimal representation of it. The result will be stored at the given
 * buffer. The string will be NULL terminated, so the buffer should at be
 * 3 bytes long.
 *
 * @param data Data to convert
 * @param str Pointer to a buffer where string will be stored stored
 *
 * @see nibbleToHex()
 * @see uint16ToHexStr()
 */
static inline void uint8ToHexStr(uint8_t data, char str[3])
{

    str[2] = 0;
    str[1] = nibbleToHex(data & 0xF);
    str[0] = nibbleToHex((data >> 4) & 0xF);

}

/**
 * @brief Converts uint16_t into hexadecimal representation
 *
 * This function will convert the given data into a string containing the
 * hexadecimal representation of it. The result will be stored at the given
 * buffer. The string will be NULL terminated, so the buffer should at be
 * 5 bytes long.
 *
 * @param data Data to convert
 * @param str Pointer to a buffer where string will be stored stored
 *
 * @see nibbleToHex()
 * @see uint8ToHexStr()
 */
static inline void uint16ToHexStr(uint16_t data, char str[5])
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

extern void incDecRange(uint8_t* val, int8_t dir, uint8_t min, uint8_t max);

extern void incDecRangeOverflow(uint8_t* val, int8_t opr, uint8_t max);

#endif /* _WC_BASE_H_ */
