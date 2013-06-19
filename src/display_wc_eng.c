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
 * @file display_wc_eng.c
 * @brief Containing the implementation specific to the English frontpanel
 *
 * This header file contains the actual implementation to the English
 * frontpanel. Among other things this includes the function
 * display_getTimeState().
 *
 * @note This file should be left untouched if making general adaptations to
 * the display module. Only things specific to the English language should be
 * put inside this file.
 *
 * @see display_wc_eng.h
 */

#include <inttypes.h>
#include <avr/io.h>
#include <util/delay.h>
#include <avr/pgmspace.h>

#include "main.h"
#include "display.h"
#include "shift.h"

#include "wceeprom.h"

#if (WC_DISP_ENG == 1)

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
    #define _DISP_SETBIT(x) ((DisplayState)1 << ((x) - DWP_MIN_FIRST))

    /**
     * @brief Containing the display states for the minute blocks
     *
     * There are eleven "five-minute" blocks (5, 10, 15, 20, 25, 30, 35, 40,
     * 45, 50, 55), which are defined ascendingly here. To make it easier,
     * _DISP_SETBIT() is used quite heavily.
     *
     * @see _DISP_SETBIT()
     * @see e_displayWordPos
     * @see DisplayState
     */
    static const uint8_t minData[11] = {

        (_DISP_SETBIT(DWP_fiveMin) | _DISP_SETBIT(DWP_past)),
        (_DISP_SETBIT(DWP_tenMin) | _DISP_SETBIT(DWP_past)),
        (_DISP_SETBIT(DWP_quarter) | _DISP_SETBIT(DWP_past)),
        (_DISP_SETBIT(DWP_twenty) | _DISP_SETBIT(DWP_past)),
        (_DISP_SETBIT(DWP_twenty) | _DISP_SETBIT(DWP_fiveMin) | _DISP_SETBIT(DWP_past)),
        (_DISP_SETBIT(DWP_half) | _DISP_SETBIT(DWP_past)),
        (_DISP_SETBIT(DWP_twenty) | _DISP_SETBIT(DWP_fiveMin) | _DISP_SETBIT(DWP_to)),
        (_DISP_SETBIT(DWP_twenty) | _DISP_SETBIT(DWP_to)),
        (_DISP_SETBIT(DWP_quarter) | _DISP_SETBIT(DWP_to)),
        (_DISP_SETBIT(DWP_tenMin) | _DISP_SETBIT(DWP_to)),
        (_DISP_SETBIT(DWP_fiveMin) | _DISP_SETBIT(DWP_to))

    };

    /*
     * Undefine helper macro as it is no longer neede
     */
    #undef _DISP_SETBIT

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

        #if (DISPLAY_DEACTIVATABLE_ITIS == 1)

            leds = 0;

            if (((g_displayParams->mode & 1) == 0) || (minutes == 0)) {

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
            minState = minData[minutes - 1];

            if (minutes > 6) {

                ++hour;

            }

            leds |= ((DisplayState)minState) << DWP_MIN_FIRST;

        } else {

            leds |= ((DisplayState)1 << DWP_clock);

        }

        for (; minuteLeds; --minuteLeds) {

            leds |= ((DisplayState)1 << (minuteLeds - 1 + DWP_MIN_LEDS_BEGIN));

        }

        if (hour > 12) {

            hour -= 12;

        }

        leds |= ((DisplayState)1 << (DWP_HOUR_BEGIN - 1 + hour));

        return leds;

}

#endif /* WC_DISP_ENG */
