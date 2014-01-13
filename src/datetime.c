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
 * @file datetime.c
 * @brief Implementation of functions declared in the header datetime.h
 *
 * This implements all the functionality declared in the header filedatetime.h,
 * which for now comes down to functionality to validate the correctness of a
 * given datetime.
 *
 * @see datetime.h
 */

#include "datetime.h"


/**
 * @brief Checks whether the given year is a leap year
 *
 * This algorithm is based upon [1]. As the year only ranges from 0 up to 99,
 * it is not possible to check whether the year is evenly divisible by 400.
 *
 * [1]: https://en.wikipedia.org/wiki/Leap_year#Algorithm
 *
 * @param year Year to check, ranges from 0 to 99
 *
 * @return True if given year is a leap year, false otherwise
 *
 * @see datetime_t::YY
 */
static bool is_leap_year(uint8_t year)
{

    if (year == 0) {

        return false;

    } else if ((year % 4) == 0) {

        return true;

    } else {

        return false;

    }

    return true;

}

/**
 * @brief Returns the number of days for the given month and year
 *
 * @param month Month to return the number of days for
 * @param year Year of month to return the number of days for
 *
 * @return Number of days for given month and year
 *
 * @see is_leap_year()
 */
static uint8_t get_number_of_days_in_month(uint8_t month, uint8_t year)
{

    uint8_t days[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};


   if (is_leap_year(year)) {

       days[1] = 29;

   }

   return days[month];

}

/**
 * @brief This function validates the given datetime
 *
 * The implementation is quite straightforward and checks for the very obvious
 * properties of a datetime. The valid ranges for each property are described
 * at datetime_t itself.
 *
 * @param datetime Pointer to datetime structure, which should be validated
 *
 * @return True if datetime is valid, false otherwise
 *
 * @note The weekday is only checked for its range, but not for its actual
 * correctness in regards to the other date properties.
 *
 * @see datetime_t
 */
bool datetime_validate(const datetime_t* datetime) {

    if (datetime->hh > 23) {

        return false;

    }

    if (datetime->mm > 59) {

        return false;

    }

    if (datetime->ss > 59) {

        return false;

    }

    if (datetime->WD > 6) {

        return false;

    }

    if (datetime->YY > 99) {

        return false;

    }

    if (datetime->MM < 1 || datetime->MM > 12) {

        return false;

    }

    if (datetime->DD > get_number_of_days_in_month(datetime->MM, datetime->YY)) {

        return false;

    }

    return true;

}
