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
 * @file format.c
 * @brief Definitions of formats declared in format.h
 *
 * This contains the actual definitions of formats specified within format.h.
 *
 * @see format.h
 */

#include "format.h"

/**
 * @brief Format string describing an unsigned decimal of variable length
 */
char const fmt_unsigned_decimal[] PROGMEM = "%u";

/**
 * @brief Format string describing a (unsigned) hexadecimal of variable length
 */
char const fmt_hex[] PROGMEM = "%x";
