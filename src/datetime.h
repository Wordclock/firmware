/*
 * Copyright (C) 2012, 2013, 2014 Karol Babioch <karol@babioch.de>
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
 * @brief Implements functionality dealing with date and time
 *
 * This module implements all of the functionality that is needed to deal with
 * date and time information. It also implements a software based clock, which
 * is synchronized with the {@link i2c_rtc.h RTC} on a regular basis.
 *
 * To deal with date and time information the type {@link #datetime_t} is
 * used through the project and is also defined in this file.
 */

#ifndef _WC_DATETIME_H_
#define _WC_DATETIME_H_

#include <stdbool.h>
#include <stdint.h>

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
     * Ranges from 1 to 31
     */
    uint8_t DD;

    /**
     * @brief Weekday
     *
     * Ranges from 1 to 7, 1 represents Monday
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

extern void datetime_init();

extern bool datetime_validate(const datetime_t* datetime);

extern void datetime_handle();

extern bool datetime_set(datetime_t* dt);
extern const datetime_t* datetime_get();

extern void datetime_ISR();

#endif /* _WC_DATETIME_H_ */
