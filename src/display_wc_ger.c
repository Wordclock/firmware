/*
 * Copyright (C) 2012, 2013 Karol Babioch <karol@babioch.de>
 * Copyright (c) 2010 Vlad Tepesch
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
 * @file display_wc_ger.c
 * @brief Containing the implementation specific to the German frontpanel
 *
 * This header file contains the actual implementation to the "old" German
 * frontpanel. Among other things this includes the function
 * display_getTimeState().
 *
 * @note Keep in mind that this is the "old" German frontpanel, which only
 * supports two different modes, whereas the new one (ger3) supports three
 * different modes.
 *
 * @note This file should be left untouched if making general adaptations to
 * the display module. Only things specific to the English language should be
 * put inside this file.
 *
 * @see display_wc_ger.h
 */

#include <inttypes.h>
#include <avr/io.h>
#include <util/delay.h>
#include <avr/pgmspace.h>

#include "config.h"
#include "display.h"
#include "shift.h"
#include "wceeprom.h"

#if (WC_DISP_GER == 1)

    /**
     * @brief Macro making it easier to set single bits within a display state
     *
     * This makes it easier to deal with display states, whenever single bits
     * need to be set. It is used quite heavily within minData to define
     * the single states.
     *
     * The position is simply calculated by subtracting DWP_MIN_FIRST from
     * the given parameter, which is expected to be from e_displayWordPos and
     * shifting a one by this amount.
     *
     * @see minData
     * @see DWP_MIN_FIRST
     * @see e_displayWordPos
     * @see DisplayState
     */
    #define _DISP_SETBIT(x) ((DisplayState)1 << ((x) - DWP_MIN_FIRST))

    /**
     * @brief Containing the display states for the minute blocks
     *
     * There are eleven "five-minute" blocks (5, 10, 15, 20, 25, 30, 35, 40,
     * 45, 50, 55), which are defined ascendingly here. To make it easier,
     * _DISP_SETBIT() is used quite heavily.
     *
     * This defines the display states for the "Ossi" mode. As there are
     * only subtle differences between the "Ossi" and the "Wessi" mode, the
     * "Wessi" mode is actually also using these definitions. However in the
     * case of "viertel" (quarter past) and "dreiviertel" (quarter to)
     * minWessiViertel and minWessidreiViertel are used.
     *
     * @see _DISP_SETBIT()
     * @see e_displayWordPos
     * @see DisplayState
     * @see minWessiViertel
     * @see minWessidreiViertel
     */
    static const uint8_t minDataOssi[11] = {

        (_DISP_SETBIT(DWP_fuenfMin) | _DISP_SETBIT(DWP_nach)),
        (_DISP_SETBIT(DWP_zehnMin) | _DISP_SETBIT(DWP_nach)),
        (_DISP_SETBIT(DWP_viertel)),
        (_DISP_SETBIT(DWP_zehnMin) | _DISP_SETBIT(DWP_halb) | _DISP_SETBIT(DWP_vorMin)),
        (_DISP_SETBIT(DWP_fuenfMin) | _DISP_SETBIT(DWP_halb) | _DISP_SETBIT(DWP_vorMin)),
        (_DISP_SETBIT(DWP_halb)),
        (_DISP_SETBIT(DWP_fuenfMin) | _DISP_SETBIT(DWP_halb) | _DISP_SETBIT(DWP_nach)),
        (_DISP_SETBIT(DWP_zehnMin) | _DISP_SETBIT(DWP_halb) | _DISP_SETBIT(DWP_nach)),
        (_DISP_SETBIT(DWP_viertel) | _DISP_SETBIT(DWP_dreiHour)),
        (_DISP_SETBIT(DWP_zehnMin) | _DISP_SETBIT(DWP_vorMin)),
        (_DISP_SETBIT(DWP_fuenfMin) | _DISP_SETBIT(DWP_vorHour)),

    };

    /**
     * @brief Containing the display state for "viertel nach" (quarter past)
     *
     * This defines the display state for the "Wessi" mode in case of
     * "viertel nach" (quarter past).
     *
     * @see _DISP_SETBIT()
     * @see e_displayWordPos
     * @see DisplayState
     * @see minDataOssi
     * @see minWessidreiViertel
     */
    static const uint8_t minWessiViertel = (_DISP_SETBIT(DWP_viertel) | _DISP_SETBIT(DWP_nach));

    /**
     * @brief Containing the display state for "dreiviertel" (quarter to)
     *
     * This defines the display state for the "Wessi" mode in case of
     * "dreiviertel" (quarter to).
     *
     * @see _DISP_SETBIT()
     * @see e_displayWordPos
     * @see DisplayState
     * @see minDataOssi
     * @see minWessiViertel
     */
    static const uint8_t minWessidreiViertel = (_DISP_SETBIT(DWP_viertel) | _DISP_SETBIT(DWP_vorHour));

    /*
     * Undefine helper macro as it is no longer needed
     */
    #undef _DISP_SETBIT

    /**
     * @see display.h
     */
    DisplayState display_getTimeState(const datetime_t* i_newDateTime)
    {

        /*
         * Get a local copy of hour and minutes
         */
        uint8_t hour = i_newDateTime->hh;
        uint8_t minutes = i_newDateTime->mm;

        /*
         * Get the number of minutes past the current "five-minute" block.
         * This will later on be used to setup the minute LEDs
         */
        uint8_t minuteLeds = minutes % 5;

        /*
         * Get the current "five-minute" block
         */
        minutes = minutes / 5;

        /*
         * The variable which will be returned later on
         */
        DisplayState leds;

        /*
         * Get the chosen language mode
         */
        uint8_t langMode = g_displayParams->mode;

        /*
         * Check whether code for deactivating the phrase "Es ist" (it is)
         * should be compiled in.
         */
        #if (DISPLAY_DEACTIVATABLE_ITIS == 1)

            /*
             * Initialize the display state
             */
            leds = 0;

            /*
             * As DISPLAY_DEACTIVATABLE_ITIS == 1 the amount of states is
             * actually doubled, as each mode is offered with and/or without
             * the phrase "Es ist" (it is), so this division deal with it.
             */
            langMode /= 2;

            /*
             * Check whether the phrase "Es ist" (it is) should actually be
             * displayed. This is the case when either the mode is even or
             * when the hour is "full" and/or half full.
             */
            if (((g_displayParams->mode & 1) == 0) || (0 == minutes) || (6 == minutes)) {

                leds |= ((DisplayState)1 << DWP_itis);

            }

        /*
         * The phrase "Es ist" (it is) should always be enabled and is
         * not deactivatable.
         */
        #else

            /*
             * Enable the phrase "Es ist" (it is)
             */
            leds = ((DisplayState)1 << DWP_itis);

        #endif

        /*
         * Check whether it is p.m.: In case the hour is bigger than twelve
         * it needs to be decremented by twelve, as only hours between one and
         * twelve can be displayed.
         */
        if (hour > 12) {

            hour -= 12;

        }

        /*
         * Zero equals twelve
         */
        if (hour == 0) {

            hour = 12;

        }

        /*
         * Check whether the hour is "full" or whether the time has actually
         * progressed so far that a "five-minute" block needs to be displayed.
         */
        if (minutes > 0) {

            /*
             * Holding the "state" for the current "five-minute" block
             */
            uint8_t minState;
            minState = (minDataOssi[minutes - 1]);

            /*
             * Checking whether "Wessi" mode is selected
             */
            if (tm_wessi == langMode) {

                /*
                 * Use state defined in minWessiViertel when the time is
                 * "viertel nach" (quarter past)
                 */
                if (minutes == 3) {

                    minState = minWessiViertel;

                /*
                 * Use state defined in minWessidreiViertel when the time is
                 * "dreiviertel" (quarter to)
                 */
                } else if (minutes == 9) {

                    minState = minWessidreiViertel;

                }

                /*
                 * Increment the hour in case the time is being displayed
                 * relative to the next full hour, e.g. "9:30" would actually
                 * be called "halb zehn" ("half ten") in this mode.
                 */
                if (minutes >= 4) {

                    hour++;

                }

            /*
             * "Ossi" mode is selected
             */
            } else {

                /*
                 * Increment the hour in case the time is being displayed
                 * relative to the next full hour, e.g. "9:15" would actually
                 * be called "viertel zehn" ("quarter ten") in this mode.
                 */
                if (minutes >= 3) {

                    hour++;

                }

            }

            /*
             * Add minState to the result to be returned
             */
            leds |= ((DisplayState)minState) << DWP_MIN_FIRST;

        /*
         * The hour is "full" and no "five-minute" block needs to be
         * displayed, e.g. 9:00 - 9:04
         */
        } else {

            /*
             * Activate the phrase "Uhr" (clock)
             */
            leds |= ((DisplayState)1 << DWP_clock);

        }

        /*
         * Iteratove over minute LEDs and enable them if necessary
         */
        for (; minuteLeds; minuteLeds--) {

            leds |= ((DisplayState)1 << (minuteLeds - 1 + DWP_MIN_LEDS_BEGIN));

        }

        /*
         * Once again check whether the hour is bigger than twelve and
         * decrement it if necessary, as we potentially have increased the hour
         * in the previous passage when displaying the time relative to the
         * next hour, e.g. "halb zehn" ("half ten").
         */
        if (hour > 12) {

            hour -= 12;

        }

        /*
         * Check whether the "s" from "Eins" needs to be enabled to be
         * correct German. This is needed in case the hour is not "full" and
         * a "five-minute" block needs to be displayed.
         */
        if (hour == 1 && minutes >= 1) {

            leds |= ((DisplayState)1 << DWP_s);

        }

        /*
         * Enable the LED group corresponding to the hour
         */
        leds |= ((DisplayState)1 << (DWP_HOUR_BEGIN - 1 + hour));

        return leds;

}

#endif /* WC_DISP_GER */
