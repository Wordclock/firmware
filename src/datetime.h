/*
 * Copyright (C) 2012, 2013 Karol Babioch <karol@babioch.de>
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
 * @file datetime.h
 * @brief Contains the datetime_t type definition
 *
 * The datetime_t type is used throughout the whole project in various places
 * where access to time is needed. Every module that needs access to this is
 * supposed to include this header.
 */

#ifndef _WC_DATETIME_H_
#define _WC_DATETIME_H_

/**
 * @brief Type definition describing a point in time
 *
 * This type describes an arbitrary point in time, including the date and
 * time by combining these properties in a struct.
 */
typedef struct
{

    unsigned char YY; /**< Year, ranges from 0 to 99 */
    unsigned char MM; /**< Month, ranges from 1 to 12 */
    unsigned char DD; /**< Day, ranges from 0 to 31 */
    unsigned char hh; /**< Hour, ranges from 0 to 23 */
    unsigned char mm; /**< Minute, ranges from 0 to 59 */
    unsigned char ss; /**< Seconds, ranges from 0 to 59 */
    unsigned char wd; /**< Weekday, ranges from 0 to 6, 0 represents Sunday */

} datetime_t;

#endif /* _WC_DATETIME_H_ */
