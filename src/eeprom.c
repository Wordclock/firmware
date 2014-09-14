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
 * @file eeprom.c
 * @brief Implementation of module allowing for read and write access to EEPROM
 *
 * This implements the functionality declared in {@link eeprom.h}. For now it
 * is essentially only a wrapper around `<avr/eeprom.h>`.
 *
 * @todo Implement interrupt driven approach using functionality {@link fifo.h}
 *
 * For details about how the EEPROM works in detail and how to access it from
 * within the program, refer to [1] and/or [2].
 *
 * [1]: http://www.atmel.com/images/doc2545.pdf
 * [2]: http://www.nongnu.org/avr-libc/user-manual/group__avr__eeprom.html
 *
 * @see eeprom.h
 */

#include <avr/eeprom.h>

#include "eeprom.h"

/**
 * @brief Reads in a block with given length from EEPROM
 *
 * This reads in a block of data pointed to by src from the EEPROM and puts it
 * into the SRAM location pointed to by dst. The length of the block is
 * described by length.
 *
 * @param dst Destination where to put data to (within SRAM)
 * @param src Source where to read data from (within EEPROM)
 * @param length Length of data
 *
 * @see eeprom_read_block()
 */
void eeprom_get_block(void* dst, const void* src, size_t length)
{

    return eeprom_read_block(dst, src, length);

}

/**
 * @brief Puts a block of data with given length to EEPROM
 *
 * This writes a block of data from the SRAM location pointed to by src and
 * puts it into the EEPROM location pointed to by dst. The length of the block
 * is described by length.
 *
 * @param src Source where to read data from (within SRAM)
 * @param dst Destination where to put data to (within EEPROM)
 * @param length Length of data
 *
 * @see eeprom_update_block()
 */
void eeprom_put_block(const void* src, void* dst, size_t length)
{

    eeprom_update_block(src, dst, length);

}
