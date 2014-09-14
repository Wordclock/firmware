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
 * @file memcheck.h
 * @brief Provides means to check the memory usage during runtime
 *
 * This module allows to check the memory usage during runtime. It can be used
 * to measure the maximum stack depth to get data about the claimed resources.
 *
 * @see memcheck.c
 */

#ifndef _WC_MEMCHECK_H_
#define _WC_MEMCHECK_H_

extern unsigned short memcheck_get_unused(void);

#endif  /* _WC_MEMCHECK_H_ */
