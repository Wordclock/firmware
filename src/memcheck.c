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
size_t memcheck_get_unused(void)
{

   size_t unused = 0;
   unsigned char *p = &__heap_start;

   do {

       if (*p++ != MEMCHECK_MASK) {

           break;

       }

      unused++;

   } while (p <= (unsigned char*)RAMEND);

   return unused;

}

/**
 * @brief Returns amount of currently unused bytes of SRAM
 *
 * This retrieves the currently unused bytes of SRAM by subtracting the
 * stack pointer address from the address of the heap start. Anything in
 * between is unused right now.
 *
 * @return Number of currently unused SRAM bytes
 */
size_t memcheck_get_current(void)
{

    return SP - (size_t)&__heap_start;

}


void __attribute__ ((naked, used, section(".init3"))) memcheck_init(void);

/**
 * @brief Initializes the SRAM with specific bit pattern
 *
 * This applies a {@link MEMCHECK_MASK specific} bit pattern to the SRAM -
 * starting from the start of the heap up to right before the stack pointer.
 *
 * @warning This function is expected to be executed in the context of the
 * initialization (`.init3`) and must not be called during runtime.
 *
 * @warning To prevent stack corruption this function must not manipulate the
 * stack.
 *
 * @see MEMCHECK_MASK
 */
void memcheck_init(void)
{

    for (uint8_t *p = &__heap_start; p < (uint8_t*)SP; p++) {

        *p = MEMCHECK_MASK;

    }

}
