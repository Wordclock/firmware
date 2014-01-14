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
 * The datetime_t type is used throughout the whole project whenever a date
 * and/or time has to be represented. Every module that needs access to this
 * is supposed to include this header.
 */

#ifndef _WC_DATETIME_H_
#define _WC_DATETIME_H_

/**
 * @brief Type definition describing a point in time
 *
 * This type describes an arbitrary point in time, including the date and
 * time combined within one struct.
 */
typedef struct
{

    /**
     * @brief Year
     *
     * Ranges from 0 to 99
     */
    uint8_t YY;

    /**
     * @brief Month
     *
     * Ranges from 1 to 12
     */
    uint8_t MM;

    /**
     * @brief Day
     *
     * Ranges from 0 to 31
     */
    uint8_t DD;

    /**
     * @brief Weekday
     *
     * Ranges from 0 to 6, 0 represents Sunday
     */
    uint8_t WD;

    /**
     * @brief Hour
     *
     * Ranges from 0 to 23
     */
    uint8_t hh;

    /**
     * @brief Minute
     *
     * Ranges from 0 to 59
     */
    uint8_t mm;

    /**
     * @brief Seconds
     *
     * Ranges from 0 to 59
     */
    uint8_t ss;

} datetime_t;

#endif /* _WC_DATETIME_H_ */
