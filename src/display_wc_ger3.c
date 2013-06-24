/*
 * Copyright (C) 2012, 2013 Karol Babioch <karol@babioch.de>
 * Copyright (c) 2012 Vlad Tepesch
 * Copyright (c) 2012 Uwe Höß
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
 * @file display_wc_ger3.c
 * @brief Containing the implementation specific to the "new" German frontpanel
 *
 * This header file contains the actual implementation to the "new" German
 * frontpanel. Among other things this includes the function
 * display_getTimeState().
 *
 * @note Keep in mind that this is the "new" German frontpanel, which is
 * designed a little bit different than the "old" one and therefore supports
 * more modes.
 *
 * @note This file should be left untouched if making general adaptations to
 * the display module. Only things specific to the German language should be
 * put inside this file.
 *
 * @see display_wc_ger3.h
 */

#include <inttypes.h>
#include <stdbool.h>

#include "main.h"
#include "base.h"
#include "display.h"
#include "shift.h"
#include "wceeprom.h"
#include "simple_random.h"

#if (WC_DISP_GER3 == 1)

    #define DISP_SETBIT(x) (1 << (x - DWP_MIN_FIRST))

    static const uint8_t s_minData[] = {

        (0),
        (DISP_SETBIT(DWP_nach)),
        (DISP_SETBIT(DWP_fuenfMin) | DISP_SETBIT(DWP_nach)),
        (DISP_SETBIT(DWP_zehnMin) | DISP_SETBIT(DWP_nach)),
        (DISP_SETBIT(DWP_zwanzigMin) | DISP_SETBIT(DWP_vor ) | DISP_SETBIT(DWP_halb)),
        (DISP_SETBIT(DWP_viertel) | DISP_SETBIT(DWP_nach)),
        (DISP_SETBIT(DWP_viertel)),
        (DISP_SETBIT(DWP_viertel) | DISP_SETBIT(DWP_vor) | DISP_SETBIT(DWP_halb)),
        (DISP_SETBIT(DWP_dreiMin) | DISP_SETBIT(DWP_viertel) | DISP_SETBIT(DWP_vor)),
        (DISP_SETBIT(DWP_dreiMin) | DISP_SETBIT(DWP_viertel) | DISP_SETBIT(DWP_nach) | DISP_SETBIT(DWP_halb)),
        (DISP_SETBIT(DWP_zehnMin) | DISP_SETBIT(DWP_vor) | DISP_SETBIT(DWP_halb)),
        (DISP_SETBIT(DWP_zwanzigMin) | DISP_SETBIT(DWP_nach)),
        (DISP_SETBIT(DWP_fuenfMin) | DISP_SETBIT(DWP_vor) | DISP_SETBIT(DWP_halb)),
        (DISP_SETBIT(DWP_vor) | DISP_SETBIT(DWP_halb)),
        (DISP_SETBIT(DWP_halb)),
        (DISP_SETBIT(DWP_halb) | DISP_SETBIT(DWP_nach)),
        (DISP_SETBIT(DWP_fuenfMin) | DISP_SETBIT(DWP_nach) | DISP_SETBIT(DWP_halb)),
        (DISP_SETBIT(DWP_zehnMin) | DISP_SETBIT(DWP_nach) | DISP_SETBIT(DWP_halb)),
        (DISP_SETBIT(DWP_zwanzigMin) | DISP_SETBIT(DWP_vor)),
        (DISP_SETBIT(DWP_viertel) | DISP_SETBIT(DWP_vor)),
        (DISP_SETBIT(DWP_dreiMin) | DISP_SETBIT(DWP_viertel)),
        (DISP_SETBIT(DWP_dreiMin) | DISP_SETBIT(DWP_viertel) | DISP_SETBIT(DWP_nach)),
        (DISP_SETBIT(DWP_viertel) | DISP_SETBIT(DWP_nach) | DISP_SETBIT(DWP_halb)),
        (DISP_SETBIT(DWP_dreiMin) | DISP_SETBIT(DWP_viertel) | DISP_SETBIT(DWP_vor) | DISP_SETBIT(DWP_halb)),
        (DISP_SETBIT(DWP_zehnMin) | DISP_SETBIT(DWP_vor)),
        (DISP_SETBIT(DWP_zwanzigMin) | DISP_SETBIT(DWP_nach) | DISP_SETBIT(DWP_halb)),
        (DISP_SETBIT(DWP_fuenfMin) | DISP_SETBIT(DWP_vor)),
        (DISP_SETBIT(DWP_vor)),

    };

    #undef DISP_SETBIT

    static const DisplayState s_hourInc1st = BIN32(00001111, 11011111, 11110101, 11010000);
    static const DisplayState s_hourInc2nd = BIN32(00000000, 10000000, 00000000, 00000000);

    static const uint8_t s_minStartInd[] = {

        0,
        2,
        3,
        5,
        10,
        12,
        14,
        16,
        17,
        19,
        24,
        26,

    };

    #define MASK_SHIFT(numBits, bitOffset) \
        ((((numBits == 0) ? 0 : ((numBits == 1) ? 1 : ((numBits == 2) ? 0x3 : ((numBits == 3) ? 0x7 : 0xf)))) << 4) | bitOffset)

    static const uint8_t s_modeShiftMask[] = {

        MASK_SHIFT(1, 0),
        MASK_SHIFT(0, 1),
        MASK_SHIFT(1, 1),
        MASK_SHIFT(3, 2),
        MASK_SHIFT(1, 5),
        MASK_SHIFT(1, 6),
        MASK_SHIFT(1, 7),
        MASK_SHIFT(0, 8),
        MASK_SHIFT(1, 8),
        MASK_SHIFT(3, 9),
        MASK_SHIFT(1, 12),
        MASK_SHIFT(1, 13),

    };

    #undef MASK_SHIFT

    static const uint8_t s_minVariants[] = {

        2,
        1,
        2,
        5,
        2,
        2,
        2,
        1,
        2,
        5,
        2,
        2,

    };

    #define SELECT_MODE(i0, i5, i10, i15, i20, i25, i30, i35, i40, i45, i50, i55) \
        (i0 | (i10 << 1) | (i15 << 2) | (i20 << 5) | (i25 << 6)  | (i30 << 7) | (i40 << 8) | (i45 << 9) | (i50 << 12) | (i55 << 13))

    #define JESTER_MODE 0xffff

    static const uint16_t s_modes[] = {

        SELECT_MODE(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0),
        SELECT_MODE(0, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0),
        SELECT_MODE(0, 0, 0, 1, 0, 0, 0, 0, 0, 1, 0, 0),
        SELECT_MODE(0, 0, 0, 1, 1, 0, 0, 0, 1, 1, 0, 0),

        #if (DISPLAY_ADD_JESTER_MODE == 1)

            JESTER_MODE

        #endif

    };

    #undef SELECT_MODE

    #define DISP_SETBIT(x) ((DisplayState)1 << x)

    const uint16_t s_numbers[12] = {

        (DISP_SETBIT(DWP_zwoelf)),
        (DISP_SETBIT(DWP_ei) | DISP_SETBIT(DWP_n) | DISP_SETBIT(DWP_s)),
        (DISP_SETBIT(DWP_zw) | DISP_SETBIT(DWP_ei)),
        (DISP_SETBIT(DWP_drei)),
        (DISP_SETBIT(DWP_vier)),
        (DISP_SETBIT(DWP_fuenf)),
        (DISP_SETBIT(DWP_sechs)),
        (DISP_SETBIT(DWP_s) | DISP_SETBIT(DWP_ieben)),
        (DISP_SETBIT(DWP_acht)),
        (DISP_SETBIT(DWP_neun)),
        (DISP_SETBIT(DWP_zehn)),
        (DISP_SETBIT(DWP_elf)),

    };

    #undef DISP_SETBIT

    static bool isJesterModeActive(const datetime_t* i_dateTime, uint8_t i_langmode)
    {

        #if (DISPLAY_ADD_JESTER_MODE == 1)

            #if ((DCF_PRESENT == 1) && (DISPLAY_USE_JESTER_MODE_ON_1ST_APRIL == 1))

                return (i_langmode == tm_jesterMode) || (i_dateTime->MM == 4  && i_dateTime->DD == 1);

            #else

                return (i_langmode == tm_jesterMode);

            #endif

        #else

            #if ((DCF_PRESENT == 1) && (DISPLAY_USE_JESTER_MODE_ON_1ST_APRIL == 1))

                return (i_dateTime->MM == 4  && i_dateTime->DD == 1);

            #else

                return false;

            #endif

        #endif

    }

    DisplayState display_getTimeState(const datetime_t* i_newDateTime)
    {

        uint8_t hour = i_newDateTime->hh;
        const uint8_t minutes = i_newDateTime->mm / 5;
        const uint8_t minuteLeds = i_newDateTime->mm % 5;
        uint8_t minuteLedSubState = 0;
        uint8_t jesterMode;
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

        jesterMode = isJesterModeActive(i_newDateTime, langMode);

        if (minutes == 0) {

            leds |= ((DisplayState)1 << DWP_clock);

        }

        uint8_t subInd;
        uint8_t ind;
        DisplayState hincTestBit;

        if (jesterMode) {

            subInd = simpleRand_get() % s_minVariants[minutes];

        } else {

            const uint16_t mode = s_modes[langMode];
            const uint8_t shift = s_modeShiftMask[minutes] & 0x0f;
            const uint8_t mask = s_modeShiftMask[minutes] >> 4;

            subInd = (mode >> shift) & mask;

        }

        ind = s_minStartInd[minutes] + subInd;
        hincTestBit = ((DisplayState)1) << ind;

        leds |= ((DisplayState)(s_minData[ind])) << DWP_MIN_FIRST;

        if (hincTestBit & s_hourInc1st) {

            ++hour;

        }

        if (hincTestBit & s_hourInc2nd) {

            ++hour;

        }

        if (jesterMode) {

            uint8_t r = simpleRand_get() % 4;

            if (minuteLeds <= 2) {

                minuteLedSubState |= (1 << r);

                if (minuteLeds == 2) {

                    uint8_t r2 = simpleRand_get() % 3;
                    r2 = r2 < r ? r2 : r2 + 1;

                    minuteLedSubState |= (1 << r2);

                }


            } else {

                minuteLedSubState = 0xf;

                if (minuteLeds == 3) {

                    minuteLedSubState &= ~(1 << r);

                }

            }

        } else {

            if (minuteLeds >= 4) {

                minuteLedSubState |= (1 << (DWP_min4 - DWP_MIN_LEDS_BEGIN));

            }

            if (minuteLeds >= 3) {

                minuteLedSubState |= (1 << (DWP_min3 - DWP_MIN_LEDS_BEGIN));

            }

            if (minuteLeds >= 2) {

                minuteLedSubState |= (1 << (DWP_min2 - DWP_MIN_LEDS_BEGIN));

            }

            if (minuteLeds >= 1) {

                minuteLedSubState |= (1 << (DWP_min1 - DWP_MIN_LEDS_BEGIN));

            }

        }

        leds |= ((DisplayState)minuteLedSubState) << DWP_MIN_LEDS_BEGIN;

        leds |= display_getNumberDisplayState(hour);

        if ((hour == 1 || hour == 13) && (minutes == 0)) {

            leds &= ~((DisplayState)1 << DWP_s);

        }

        return leds;

    }

#endif /* WC_DISP_GER3 */
