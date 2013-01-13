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
 * @brief Provides some common useful and quite basic functions and/or macros
 *
 * The functions and macros declared here might be useful to various parts and
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
 * you would call it like that: BIN8(0, 1, 1, 0, 1, 0, 0, 1).
 *
 * Obviously b7..b0 are expected to be either 0 and/or 1.
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
 * This can be used to build up a byte manually by explicitly listing all the
 * bits.
 *
 * Obviously b15..b0 are expected to be either 0 and/or 1.
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
 * location in memory provided by the second argument.
 *
 * This function makes use of an approximation, which multiplies the given
 * parameter by a constant and shifts it to the right afterwards. This holds
 * true for 0-255 at least.
 *
 * This is only applicable to integers stored in a single byte (uint8_t).
 *
 * @param x Value to divide (dividend)
 * @param remainder Pointer to store the remainder at
 * @return Integer result
 */
extern uint8_t          bcdtoi (uint8_t bcd);

/**
 * converts an integer to bcd pair
 * @param i    integer number (0-255)
 * @return    converted bcd pair
 */
extern uint8_t          itobcd (uint8_t i);


/**
 * Divides a byte by ten. Also returns remainder
 * @param x         the value to divide
 * @param o_remaind pointer to memory there the remainder will be stored
 * @return the quotient
 */
static inline uint8_t div10 (uint8_t x, uint8_t* o_remaind)
{
  // seems to produce bug in byteToStrLessHundred, but why?
    //uint8_t y;
    //register uint8_t hilf;
    //asm(
    //   "ldi %[temp], 205     \n"
    //   "mul %[temp], %[input]   \n"
    //   "lsr R1             \n"
    //   "lsr R1             \n"
    //   "lsr R1             \n"
    //   "mov %[result], R1  \n"
    //    : [result] "=d" (y), [temp]"=d" (hilf)
    //    : [input]"d" (x)
    //    : "r1","r0"
    // );
    //*o_remaind = x-(10*y);
    //return y;
  uint8_t y = (((uint16_t)x)*205)>>11;
  *o_remaind = x-(y*10);
  return y;
}

/**
 *  translates a byte to a string
 *  Stripped verion of byteToStr that only can handle values between 0-99
 * @param val    the value to transform into string [0,99]
 * @param o_buf  Buffer that will receive the transformed string
 *               Has to be at least 3 bytes long because result is
 *               right justified. a \\0 is written to o_buf[2]
 *
 */
void byteToStrLessHundred( uint8_t val, char o_buf[3] );


/**
 *  translates a byte to a string
 * @param val    the value to transform into string
 * @param o_buf  Buffer that will receive the transformed string
 *               Has to be at least 4 bytes long because result is
 *               right justified. a \\0 is written to o_buf[3]
 *
 */
void byteToStr( uint8_t val, char o_buf[4] );

/**
 * creates a hexadecimal character from a nibble
 */
static inline char nibbleToHex(uint8_t nibble)
{
  return nibble + ( (nibble<10)?'0':'A'-10 );
}


/**
 *  converts data into hex-format and prints data into o_text
 *  @param   data    the 16bit word to convert
 *  @param   o_text  The buffer that will receive the converted number.
 *                   The buffer should have at least a length of 5.
 *                   
 */
static inline void uint16ToHexStr(uint16_t data, char* o_text)
{
  o_text[4]  = 0;
  o_text[3]  = getHexDigit(data & 0xF);
  o_text[2]  = getHexDigit((data>> 4) & 0xF);
  o_text[1]  = getHexDigit((data>> 8) & 0xF);
  o_text[0]  = getHexDigit((data>>12) & 0xF);
}

#endif /* _WC_BASE_H_ */
