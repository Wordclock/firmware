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
 * @file eeprom.h
 * @brief Module allowing for read and write access to the EEPROM
 *
 * This module allows to read from and write to the EEPROM coming along with
 * the AVR microcontroller in use.
 *
 * @see eeprom.c
 */

#ifndef _WC_EEPROM_H_
#define _WC_EEPROM_H_

#include <stdlib.h>

void eeprom_get_block(void* dst, const void* src, size_t length);
void eeprom_put_block(const void* src, void* dst, size_t length);

#endif /* _WC_EEPROM_H_ */
