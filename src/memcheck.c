/*
 * Copyright (C) 2014 Karol Babioch <karol@babioch.de>
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
 * @file memcheck.c
 * @brief Implements functionality to get information about memory usage
 *
 * This implements functionality to get information about the memory usage
 * during runtime. It initializes the memory with a
 * {@link #MEMCHECK_MASK specific} bit pattern during
 * {@link memcheck_init() initialization}. By scanning through memory later on
 * and looking for this bit pattern the maximum amount of used memory can be
 * determined. This implementation is based on [1], which outlines the concept
 * for the AVR GCC toolchain.
 *
 * [1]: http://rn-wissen.de/wiki/index.php/Speicherverbrauch_bestimmen_mit_avr-gcc
 *
 * @see memcheck.h
 */

#include <avr/io.h>

#include "memcheck.h"

/**
 * @brief Bit mask used to distinguish used from unused memory regions
 *
 * This bit pattern gets applied to the SRAM during the
 * {@link memcheck_init() initialization} and is then looked for when getting
 * the size of the {@link memcheck_get_unused() unused} memory.
 *
 * @note This concept only works when dynamic memory allocation is *not*
 * being used.
 *
 * @see memcheck_get_unused()
 * @see memcheck_init()
 */
#define MEMCHECK_MASK 0xaa

extern unsigned char __heap_start;

/**
 * @brief Returns number of bytes that are unused at maximum stack depth
 *
 * This scans over the SRAM and looks for a {@link MEMCHECK_MASK specific} bit
 * pattern. It returns the number of bytes that are currently set to this bit
 * pattern, which are all of the bytes that were not yet modified by the stack.
 *
 * @note The number returned here is the amount of memory that was unused so
 * far. It is not necessarily equal to the unused memory, since there might be
 * functions that reach further down within the stack, but they were not
 * invoked yet.
 *
 * @return Number of unused bytes in SRAM
 *
 * @see MEMCHECK_MASK
 */
unsigned short memcheck_get_unused(void)
{

   unsigned short unused = 0;
   unsigned char *p = &__heap_start;

   do {

       if (*p++ != MEMCHECK_MASK) {

           break;

       }

      unused++;

   } while (p <= (unsigned char*)RAMEND);

   return unused;

}

void __attribute__ ((naked, used, section(".init3"))) memcheck_init(void);

/**
 * @brief Initializes the SRAM with specific bit pattern
 *
 * This applies a {@link MEMCHECK_MASK specific} bit pattern to the SRAM -
 * starting from the start of the heap to the end.
 *
 * @note This is implemented as inline assembler to make sure that it will not
 * be optimized out by the compiler. Furthermore this makes sure that no
 * function calls are involved.
 *
 * @warning This writes the bit pattern to the end of the RAM. In case the
 * stack pointer is not at RAMEND it will overwrite the content of the stack,
 * corrupting any data therein (i.e. return addresses). Therefore it is
 * absolutely essential that this function is *not* called from any other
 * function, but executed in the context of the initialization.
 *
 * @see MEMCHECK_MASK
 */
void memcheck_init(void)
{

   __asm volatile (
      "ldi r30, lo8 (__heap_start)"  "\n\t"
      "ldi r31, hi8 (__heap_start)"  "\n\t"
      "ldi r24, %0"                  "\n\t"
      "ldi r25, hi8 (%1)"            "\n"
      "0:"                           "\n\t"
      "st  Z+,  r24"                 "\n\t"
      "cpi r30, lo8 (%1)"            "\n\t"
      "cpc r31, r25"                 "\n\t"
      "brlo 0b"
         :
         : "i" (MEMCHECK_MASK), "i" (RAMEND + 1)
   );

}
