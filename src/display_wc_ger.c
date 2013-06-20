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

/*------------------------------------------------------------------------------------------------------------------------------------------------*//**
 * @file display_wc_ger.c
 *
 *  This files implements the german language specific.
 *
 * \version $Id: display_wc_ger.c 328 2010-07-15 20:28:16Z vt $
 *
 * \author Copyright (c) 2010 Vlad Tepesch
 *
 * \remarks
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 */
 /*-----------------------------------------------------------------------------------------------------------------------------------------------*/


#include <inttypes.h>
#include <avr/io.h>
#include <util/delay.h>
#include <avr/pgmspace.h>

#include "main.h"
#include "display.h"
#include "shift.h"
#include "wceeprom.h"

#if (WC_DISP_GER == 1)

    /**
     * @brief Macro making it easier to set single bits within a display state
     *
     * This makes it easier to deal with display state, whenever single bits
     * need to be set. It is used quite heavily within minData to define
     * the single values.
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
    #define DISP_SETBIT(x) ((DisplayState)1 << ((x) - DWP_MIN_FIRST))

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

        (DISP_SETBIT(DWP_fuenfMin) | DISP_SETBIT(DWP_nach)),
        (DISP_SETBIT(DWP_zehnMin) | DISP_SETBIT(DWP_nach)),
        (DISP_SETBIT(DWP_viertel)),
        (DISP_SETBIT(DWP_zehnMin) | DISP_SETBIT(DWP_halb) | DISP_SETBIT(DWP_vorMin)),
        (DISP_SETBIT(DWP_fuenfMin) | DISP_SETBIT(DWP_halb) | DISP_SETBIT(DWP_vorMin)),
        (DISP_SETBIT(DWP_halb)),
        (DISP_SETBIT(DWP_fuenfMin) | DISP_SETBIT(DWP_halb) | DISP_SETBIT(DWP_nach)),
        (DISP_SETBIT(DWP_zehnMin) | DISP_SETBIT(DWP_halb) | DISP_SETBIT(DWP_nach)),
        (DISP_SETBIT(DWP_viertel) | DISP_SETBIT(DWP_dreiHour)),
        (DISP_SETBIT(DWP_zehnMin) | DISP_SETBIT(DWP_vorMin)),
        (DISP_SETBIT(DWP_fuenfMin) | DISP_SETBIT(DWP_vorHour)),

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
    static const uint8_t minWessiViertel = (DISP_SETBIT(DWP_viertel) | DISP_SETBIT(DWP_nach));

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
    static const uint8_t minWessidreiViertel = (DISP_SETBIT(DWP_viertel) | DISP_SETBIT(DWP_vorHour));

    #undef DISP_SETBIT

    /**
     * @see display.h
     */
    DisplayState display_getTimeState(const datetime_t* i_newDateTime)
    {

        uint8_t hour = i_newDateTime->hh;
        uint8_t minutes = i_newDateTime->mm;
        uint8_t minuteLeds = minutes % 5;
        minutes = minutes / 5;
        DisplayState leds;
        uint8_t langMode = g_displayParams->mode;

        #if (DISPLAY_DEACTIVATABLE_ITIS == 1)

            leds = 0;
            langMode /= 2;

            if (((g_displayParams->mode & 1) == 0) || (0 == minutes) || (6 == minutes)) {

                leds |= ((DisplayState)1 << DWP_itis);

            }

        #else

            leds = ((DisplayState)1 << DWP_itis);

        #endif

        if (hour > 12) {

            hour -= 12;

        }

        if (hour == 0) {

            hour = 12;

        }

        if (minutes > 0) {

            uint8_t minState;
            minState = (minDataOssi[minutes - 1]);

            if (tm_wessi == langMode) {

                if (minutes == 3) {

                    minState = minWessiViertel;

                } else if (minutes == 9) {

                    minState = minWessidreiViertel;

                }

                if (minutes >= 4) {

                    hour++;

                }

            } else {

                if (minutes >= 3) {

                    hour++;

                }

            }

            leds |= ((DisplayState)minState) << DWP_MIN_FIRST;

        } else {

            leds |= ((DisplayState)1 << DWP_clock);

        }

        for (; minuteLeds; minuteLeds--) {

            leds |= ((DisplayState)1 << (minuteLeds - 1 + DWP_MIN_LEDS_BEGIN));

        }

        if (hour > 12) {

            hour -= 12;

        }

        if (hour == 1 && minutes >= 1) {

            leds |= ((DisplayState)1 << DWP_s);

        }

        leds |= ((DisplayState)1 << (DWP_HOUR_BEGIN - 1 + hour));

        return leds;

}

#endif /* WC_DISP_GER */
