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
 * @file attributes.h
 * @brief Definition of common attributes used throughout the project
 *
 * This defines attributes with the __attribute__ keyword that are used
 * throughout the project. It makes sure that these attributes are only defined
 * once and are specified in a consistent way.
 */

#ifndef _WC_ATTRIBUTES_H_
#define _WC_ATTRIBUTES_H_

/**
 * @brief Specifier for variables that are currently not being used
 *
 * This specifier is used to suppress warnings of the compiler about unused
 * variables. When working with callback function not all parameters of a
 * function might get used, so unused parameters should be declared unused,
 * to make the compiler happy.
 */
#define __ATTR_UNUSED__ __attribute__ ((unused))

#endif /* _WC_ATTRIBUTES_H_ */
