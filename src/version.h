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
 * @file version.h
 * @brief Header file dealing with the software version of this project
 *
 * This file contains the version number along with some helper macros to deal
 * with it throughout the whole project.
 */

#ifndef _WC_VERSION_H_
#define _WC_VERSION_H_

/**
 * @brief Macro building the actual version number
 *
 * This macro will build the actual version number by combining
 * {@link #VERSION_MAJOR} and {@link #VERSION_MINOR} and shifting it
 * appropriately.
 *
 * @see VERSION_MAJOR
 * @see VERSION_MINOR
 */
#define VERSION_BUILD(x, y) ((uint16_t)((x << 8) | (y)))

/**
 * @brief Major version number
 *
 * @note This is effectively a byte and can hold values between 0 and 255.
 *
 * @see VERSION_BUILD()
 * @see VERSION_MINOR
 * @see VERSION
 */
#define VERSION_MAJOR 1

/**
 * @brief Minor version number
 *
 * @note This is effectively a byte and can hold values between 0 and 255.
 *
 * @see VERSION_BUILD()
 * @see VERSION_MINOR
 * @see VERSION
 */
#define VERSION_MINOR 0

/**
 * @brief Actual version number
 *
 * The actual version number consists of both the {@link #VERSION_MAJOR major}
 * and the {@link #VERSION_MINOR minor} version number. It is built using the
 * macro {@link #VERSION_BUILD()}.
 *
 * @see VERSION_MINOR
 * @see VERSION_MAJOR
 * @see VERSION_BUILD()
 */
#define VERSION VERSION_BUILD(VERSION_MAJOR, VERSION_MINOR)

#endif /* _WC_VERSION_H_ */
