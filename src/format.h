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
 * @file format.h
 * @brief Format string specifier for the whole project
 *
 * This module specifies various format strings (mainly meant to be used by the
 * printf() and scanf() function family, which can be used throughout the whole
 * project. Defining these formats globally makes sure that they are only
 * included once, saving on program space.
 *
 * This file contains only the declarations to make the variables accessible
 * for other modules. The actual definitions can be found within format.c.
 *
 * @see format.c
 */

#ifndef _WC_FORMAT_H_
#define _WC_FORMAT_H_

#include <avr/pgmspace.h>

extern char const fmt_unsigned_decimal[] PROGMEM;
extern char const fmt_hex[] PROGMEM;

#endif /* _WC_FORMAT_H_ */
