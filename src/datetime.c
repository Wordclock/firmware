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
 * This implements the functionality declared in the header file datetime.h.
 * The software based clock is implemented by incrementing a counter every
 * second. {@link #datetime_handle()} needs to be invoked on a quasi
 * regular basis to re-read the time from the {@link i2c_rtc.h RTC} module.
 * This approach guarantees accuracy without stressing the I2C bus too much.
 *
 * An internal copy of a {@link datetime_t} struct is always held available,
 * and can be retrieved with {@link #datetime_get()}.
 *
 * {@link #datetime_set()} can be used to set a new date and time information.
 * This information is validated and will be written to the RTC as well as
 * being applied directly to the display.
 *
 * @see datetime.h
 */

#include "config.h"
#include "datetime.h"
#include "dcf77.h"
#include "i2c_rtc.h"
#include "user.h"

/**
 * @brief Used to keep track of seconds in software
 *
 * This keeps track of the seconds of the software clock internally. It is
 * incremented each second by {@link #datetime_ISR()} and processed within
 * `datetime_handle()`. It is synchronized with the RTC when an interval of
 * at most `READ_DATETIME_INTERVAL` has passed.
 *
 * @see datetime_ISR()
 * @see READ_DATETIME_INTERVAL
 */
static volatile uint8_t soft_seconds;

/**
 * @brief Defines the interval the time should be (re)read from the RTC
 *
 * This defines the interval (in seconds) which can pass at most before the
 * time of the software clock is synchronized with the RTC again.
 *
 * @see datetime_handle()
 */
#define READ_DATETIME_INTERVAL 15

/**
 * @brief Internal copy of date and time information
 *
 * This is the actual date and time information used throughout the project.
 * A reference to this can be obtained by {@link #datetime_get()}. It is
 * managed in software and backed by the RTC, i.e. the RTC will be re-read
 * every once in a while within {@link #datetime_handle()}.
 *
 * @see datetime_get()
 * @see datetime_set()
 */
static datetime_t datetime;

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
 * @param month Month to return the number of days for, ranges from 1 to 12
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

   return days[month - 1];

}

/**
 * @brief Initializes the datetime module
 *
 * This initializes the datetime module by setting up the
 * {@link i2c_rtc.h RTC} module to be able to read the datetime from the RTC
 * later on.
 *
 * @see i2c_rtc_init()
 */
void datetime_init()
{

    i2c_master_error_t i2c_rtc_error;

    if (!i2c_rtc_init(&i2c_rtc_error)) {

        // TODO Log
        //log_main("RTC init failed\n");

    }

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
 * @see datetime
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

    if (datetime->WD < 1 || datetime->WD > 7) {

        return false;

    }

    if (datetime->YY > 99) {

        return false;

    }

    if (datetime->MM < 1 || datetime->MM > 12) {

        return false;

    }

    if (datetime->DD < 1
        || datetime->DD > get_number_of_days_in_month(datetime->MM, datetime->YY)) {

        return false;

    }

    return true;

}

/**
 * @brief Handles the synchronization between the RTC and the software clock
 *
 * The time is being kept track of using a software clock and is **not**
 * constantly synchronized with the RTC itself. In order for this software
 * clock to be more accurate, it will adjust itself in a way described by
 * the following:
 *
 * - In case it runs too slow (for instance due to the RC oscillator) this
 *   function will poll the RTC for the current time every second in the last
 *   part of the minute in order to reach the transition to the next minute
 *   as close as possible.
 *
 * - On the other hand it will slowdown the polling if it is detected that the
 *   software clock runs too fast, so that the software clock is updated less
 *   often.
 *
 * This function also (re)enables the DCF77 decoding once a new hour begins.
 *
 * @see READ_DATETIME_INTERVAL
 * @see soft_seconds
 * @see i2c_rtc_read()
 * @see dcf77_enable()
 */
void datetime_handle()
{

    static uint8_t last_hour = 0xff;
    static uint8_t last_minute = 0xff;
    static uint8_t last_seconds = 0xff;

    static uint8_t next_read_seconds = 0;

    uint8_t softclock_too_fast_seconds = 0;

    uint8_t rtc;

    if (last_seconds != soft_seconds) {

        /*
         * Check if RTC should be (re)read again
         */
        if (soft_seconds >= next_read_seconds) {

            rtc = i2c_rtc_read(&datetime);

        } else {

            datetime.ss = soft_seconds;
            rtc = true;

        }

        if (rtc) {

            /*
             * Check whether new minute has begun
             */
            if (last_minute != datetime.mm) {

                // TODO: Set time directly without using user module
                user_setNewTime(&datetime);
                last_minute = datetime.mm;

                /*
                 * Check whether new hour has begun
                 */
                if (last_hour != datetime.hh) {

                    #if (ENABLE_DCF_SUPPORT == 1)

                        dcf77_enable();

                    #endif

                    last_hour = datetime.hh;

                }

            }

            /*
             * Check whether software clock is running too fast
             */
            if (last_seconds != 0xff && soft_seconds > datetime.ss) {

                softclock_too_fast_seconds = soft_seconds - datetime.ss;

            }

            last_seconds = soft_seconds = datetime.ss;

            /*
             * Set next time the RTC should be (re)read
             */
            if (softclock_too_fast_seconds > 0) {

                next_read_seconds = soft_seconds + READ_DATETIME_INTERVAL
                      - softclock_too_fast_seconds;

            } else {

                next_read_seconds = soft_seconds + READ_DATETIME_INTERVAL;

            }

            if (next_read_seconds >= 60) {

                next_read_seconds = 0;

            }

        } else {

            // TODO Log
            //log_main("RTC error\n");

        }

    }

}


/**
 * @brief Sets a new datetime
 *
 * This checks the given date and time information for its validity. If it is
 * valid it takes the datetime over and writes it to the RTC. Furthermore the
 * new time is being pushed out to the display. If every operation was
 * performed successfully, it will return true, otherwise false is an
 * indication for an failure.
 *
 * @param dt Pointer to datetime structure, which should be set
 *
 * @return True if datetime was set successfully, false otherwise
 *
 * @see datetime
 * @see datetime_validate()
 * @see i2c_rtc_write()
 * @see user_setNewTime()
 */
bool datetime_set(datetime_t* dt)
{

    if (datetime_validate(dt)) {

        bool status = i2c_rtc_write(dt);

        if (status) {

            datetime = *dt;
            soft_seconds = dt->ss;
            user_setNewTime(dt);

            return true;

        } else {

            // TODO Log unable to write to RTC

        }

    } else {

        // TODO Log invalid datetime

    }

    // TODO Log
    return false;

}

/**
 * @brief Returns a reference to the date and time information used internally
 *
 * @return datetime_t Reference to {@link #datetime}
 *
 * @note Make sure to NOT modify the content of {@link #datetime}.
 *
 * @see datetime
 * @see datetime_t
 */
datetime_t* datetime_get()
{

    return &datetime;

}

/**
 * @brief ISR of the datetime module
 *
 * This is expected to be executed once a second and simply increments
 * {@link #soft_seconds} by one. The date and time information itself is
 * then updated by {@link #datetime_handle()} asynchronically in the
 * background.
 *
 * @see datetime_handle()
 */
void datetime_ISR()
{

    soft_seconds++;

}
