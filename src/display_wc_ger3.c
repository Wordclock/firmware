/*
 * Copyright (C) 2012, 2013, 2014 Karol Babioch <karol@babioch.de>
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

#include "config.h"
#include "base.h"
#include "display.h"
#include "shift.h"
#include "preferences.h"
#include "prng.h"

#if (WC_DISP_GER3 == 1)

    /**
     * @brief Makes it possible to set "minute groups" within a display state
     *
     * This makes it easier to deal with display states concerning the
     * "minute groups", whenever single groups need to be enabled. This is used
     * quite heavily within s_minData.
     *
     * A "minute group" is something like "fünf" (five), "zehn", (ten) and so
     * forth. By logically "or"-ing these groups it is possible to come up with
     * a display state where more than just a single "group" is enabled.
     *
     * The position is calculated by shifting a one to the the given position
     * subtracted by the value of the position of the first minute LED, see
     * DWP_MIN_FIRST.
     *
     * @see s_minData
     * @see DWP_MIN_FIRST
     */
    #define _DISP_SETBIT_MIN(x) (1 << (x - DWP_MIN_FIRST))

    /**
     * @brief Contains the minute definitions for displaying a time
     *
     * This contains the minute part of a display state with all the possible
     * ways to display a correct time. Note that not all combinations make
     * necessarily sense. To make it easier to define single bits,
     * _DISP_SETBIT_MIN() is used quite heavily here.
     *
     * There are only eight different LED "minute groups", refer to
     * e_displayWordPos for details. Therefore uint8_t is enough for now.
     *
     * The groups itself are:
     *
     * - DWP_fuenfMin
     * - DWP_zehnMin
     * - DWP_zwanzigMin
     * - DWP_dreiMin
     * - DWP_viertel
     * - DWP_nach
     * - DWP_vor
     * - DWP_halb
     *
     * By combining these in various ways all different times throughout the
     * hour can be displayed in all the different modes.
     *
     * This in part defines multiple ways of displaying the same time, e.g.
     * "viertel vor" (quarter to) as well as "dreiviertel" (three-quarter).
     * s_minStartInd defines where definitions for each five minute "block"
     * can be found within this table, as there might be gaps between
     * two five minute "blocks" due to different modes. s_minVariants on the
     * other hand defines how many different variants there are for each five
     * minute "block".
     *
     * @note If making adaptations to this, you probably need to change the
     * other lookup tables (s_minStartInd, s_hourInc2nd, s_hourInc1st,
     * s_minVariants), too.
     *
     * @see s_minStartInd
     * @see s_hourInc2nd
     * @see s_hourInc1st
     * @see s_minVariants
     * @see display_getTimeState()
     */
    static const uint8_t s_minData[] = {

        (0),
        (_DISP_SETBIT_MIN(DWP_nach)),
        (_DISP_SETBIT_MIN(DWP_fuenfMin) | _DISP_SETBIT_MIN(DWP_nach)),
        (_DISP_SETBIT_MIN(DWP_zehnMin) | _DISP_SETBIT_MIN(DWP_nach)),
        (_DISP_SETBIT_MIN(DWP_zwanzigMin) | _DISP_SETBIT_MIN(DWP_vor ) | _DISP_SETBIT_MIN(DWP_halb)),
        (_DISP_SETBIT_MIN(DWP_viertel) | _DISP_SETBIT_MIN(DWP_nach)),
        (_DISP_SETBIT_MIN(DWP_viertel)),
        (_DISP_SETBIT_MIN(DWP_viertel) | _DISP_SETBIT_MIN(DWP_vor) | _DISP_SETBIT_MIN(DWP_halb)),
        (_DISP_SETBIT_MIN(DWP_dreiMin) | _DISP_SETBIT_MIN(DWP_viertel) | _DISP_SETBIT_MIN(DWP_vor)),
        (_DISP_SETBIT_MIN(DWP_dreiMin) | _DISP_SETBIT_MIN(DWP_viertel) | _DISP_SETBIT_MIN(DWP_nach) | _DISP_SETBIT_MIN(DWP_halb)),
        (_DISP_SETBIT_MIN(DWP_zehnMin) | _DISP_SETBIT_MIN(DWP_vor) | _DISP_SETBIT_MIN(DWP_halb)),
        (_DISP_SETBIT_MIN(DWP_zwanzigMin) | _DISP_SETBIT_MIN(DWP_nach)),
        (_DISP_SETBIT_MIN(DWP_fuenfMin) | _DISP_SETBIT_MIN(DWP_vor) | _DISP_SETBIT_MIN(DWP_halb)),
        (_DISP_SETBIT_MIN(DWP_vor) | _DISP_SETBIT_MIN(DWP_halb)),
        (_DISP_SETBIT_MIN(DWP_halb)),
        (_DISP_SETBIT_MIN(DWP_halb) | _DISP_SETBIT_MIN(DWP_nach)),
        (_DISP_SETBIT_MIN(DWP_fuenfMin) | _DISP_SETBIT_MIN(DWP_nach) | _DISP_SETBIT_MIN(DWP_halb)),
        (_DISP_SETBIT_MIN(DWP_zehnMin) | _DISP_SETBIT_MIN(DWP_nach) | _DISP_SETBIT_MIN(DWP_halb)),
        (_DISP_SETBIT_MIN(DWP_zwanzigMin) | _DISP_SETBIT_MIN(DWP_vor)),
        (_DISP_SETBIT_MIN(DWP_viertel) | _DISP_SETBIT_MIN(DWP_vor)),
        (_DISP_SETBIT_MIN(DWP_dreiMin) | _DISP_SETBIT_MIN(DWP_viertel)),
        (_DISP_SETBIT_MIN(DWP_dreiMin) | _DISP_SETBIT_MIN(DWP_viertel) | _DISP_SETBIT_MIN(DWP_nach)),
        (_DISP_SETBIT_MIN(DWP_viertel) | _DISP_SETBIT_MIN(DWP_nach) | _DISP_SETBIT_MIN(DWP_halb)),
        (_DISP_SETBIT_MIN(DWP_dreiMin) | _DISP_SETBIT_MIN(DWP_viertel) | _DISP_SETBIT_MIN(DWP_vor) | _DISP_SETBIT_MIN(DWP_halb)),
        (_DISP_SETBIT_MIN(DWP_zehnMin) | _DISP_SETBIT_MIN(DWP_vor)),
        (_DISP_SETBIT_MIN(DWP_zwanzigMin) | _DISP_SETBIT_MIN(DWP_nach) | _DISP_SETBIT_MIN(DWP_halb)),
        (_DISP_SETBIT_MIN(DWP_fuenfMin) | _DISP_SETBIT_MIN(DWP_vor)),
        (_DISP_SETBIT_MIN(DWP_vor)),

    };

    /*
     * Undefine helper macro as it is no longer needed
     */
    #undef _DISP_SETBIT_MIN

    /**
     * @brief Defining when an increment by one to the current hour is needed
     *
     * Some entries of s_minData require the hour to be incremented by one for
     * the displayed time to be correct. E.g. "8:45" can be displayed as
     * "viertel vor neun" (quarter to nine).
     *
     * The least significant byte represents the first eight entries within
     * s_minData. As there currently are only 28 entries within s_minData, the
     * most significant byte represents the last four entries, which makes
     * the four most significant bits basically useless.
     *
     * @see s_minData
     * @see BIN32()
     * @see s_hourInc2nd
     * @see display_getTimeState()
     */
    static const display_state_t s_hourInc1st = BIN32(00001111, 11011111, 11110101, 11010000);

    /**
     * @brief Defining when an increment by two to the current hour is needed
     *
     * Analogous to s_hourInc1st this defines which entries of s_minData
     * require the current hour to be incremented by two. Currently there
     * is only a single entry where this is the case, namely "dreiviertel vor
     * halb" ("three-quarters to half"). To make this more clear consider
     * "8:45" as an example. This can be displayed as "dreiviertel vor halb
     * zehn" ("three quarters to half ten").
     *
     * The meaning of the bits itself are completely analogous to s_hourInc1st.
     *
     * @see s_minData
     * @see BIN32()
     * @see s_hourInc1st
     * @see display_getTimeState()
     */
    static const display_state_t s_hourInc2nd = BIN32(00000000, 10000000, 00000000, 00000000);

    /**
     * @brief Defines the start indexes for each "block" within s_minData
     *
     * This defines the index within s_minData, where a new five minute time
     * "block" starts. There are exactly twelve entries expected, one for each
     * block (0, 5, 10, 15, 20, 25, 30, 35, 40, 45, 55). As there might be
     * different ways to display the same block, the numbers itself are not
     * completely "gapless". The "gaps", e.g. between 5 and 10, basically mean
     * that there are five possible ways to display "viertel nach" (quarter
     * past).
     *
     * @see s_minData
     * @see display_getTimeState()
     */
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

    /**
     * @brief Makes it possible mask and shift bytes more easily
     *
     * This makes it easier to deal with byte shifting and masking within
     * s_modeShiftMask.
     *
     * The resulting byte is basically actually "halved". The first half
     * (lower nibble) represents a bit offset, whereas the second half
     * (higher nibble) represents the number of bits given.
     *
     * @param numBits Number of bits (0 - 4), higher nibble of resulting byte
     * @param bitOffset Bit offset, lower nibble of the resulting byte
     *
     * @return Byte after applying the operations with the given parameters
     *
     * @see s_modeShiftMask
     */
    #define _MASK_SHIFT(numBits, bitOffset) \
        ((((numBits == 0) ? 0 : ((numBits == 1) ? 1 : ((numBits == 2) ? 0x3 : ((numBits == 3) ? 0x7 : 0xf)))) << 4) | bitOffset)

    /**
     * @brief Defines the position of sub indexes for each 5 minute time "block"
     *
     * There are exactly twelve entries expected, one for each "block"
     * (0, 5, 10, 15, 20, 25, 30, 35, 40, 45, 55).
     *
     * The _MASK_SHIFT() macro is used to define the entries itself:
     *
     * - The first parameter is the number of bits needed to encode the given
     * amount of different variants, e.g. when there are two different variants
     * at least one bit is needed to distinguish between both of them. If there
     * are five different variants then at least 3 bits (2 ^ 3 = 8) are needed.
     * This is effectively the bit mask that has to be applied after the bitset
     * has been shifted.
     *
     * - The second parameter is amount of bits that the bitset needs to be
     * shifted to the right.
     *
     * The number of bits needed to shift an entry should be as big as the
     * number of bits needed by all previous definitions combined, e.g. when
     * the first entry needs one bit, the second entry can only start at bit
     * two and therefore needs to be shifted by one.
     *
     * In combination with s_modes this makes it possible to get the correct
     * index within s_minData for the chosen language mode and the given
     * time.
     *
     * @see _MASK_SHIFT()
     * @see display_getTimeState()
     * @see s_modes
     */
    static const uint8_t s_modeShiftMask[] = {

        _MASK_SHIFT(1, 0),
        _MASK_SHIFT(0, 1),
        _MASK_SHIFT(1, 1),
        _MASK_SHIFT(3, 2),
        _MASK_SHIFT(1, 5),
        _MASK_SHIFT(1, 6),
        _MASK_SHIFT(1, 7),
        _MASK_SHIFT(0, 8),
        _MASK_SHIFT(1, 8),
        _MASK_SHIFT(3, 9),
        _MASK_SHIFT(1, 12),
        _MASK_SHIFT(1, 13),

    };

    /*
     * Undefine helper macro as it is no longer needed
     */
    #undef _MASK_SHIFT

    /**
     * @brief Number of variants for each five minute "block" within s_minData
     *
     * This defines the number of different variants for each five minute
     * "block" within s_minData, e.g. for the "fünf nach" (five past) time
     * "block" is only one entry within s_minData, whereas for the "viertel
     * nach" (quarter past) time "block" there are five different variants
     * within s_minData.
     *
     * This, in a way, is also expressed by the first parameter of _MASK_SHIFT()
     * within s_modeShiftMask.
     *
     * @see s_minData
     * @see s_modeShiftMask
     * @see display_getTimeState()
     */
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

    /**
     * @brief Used to define bitsets representing the different modes
     *
     * This makes it easier to deal with bitsets representing the different
     * modes, which are implemented by this module, namely: "Wessi",
     * "Rhein-Ruhr", "Ossi", "Schwabe".
     *
     * It expects 12 parameters: One for each five minute block (0, 5, 10, 15,
     * 20, 25, 30, 35, 40, 45, 50, 55) starting with the one for 0. Each
     * parameter defines which sub index (variant) should actually be chosen
     * for this specific five minute "block" from within s_minData.
     *
     * The range for each parameter depends upon the number of bits that are
     * used to encode this five minute "block". Refer to s_modeShiftMask for
     * details. Right now this means that the following five minute "blocks"
     * are exactly one bit wide: 0, 10, 20, 25, 30, 40, 50, 55. The five minute
     * "blocks" for 15 and 45 on the other hand are each three bits wide. The
     * five minute blocks for 5 and 35 are not encoded here, as there is only
     * one variant for each of this block.
     *
     * One bit width blocks can either be 0 and/or 1, three bit width blocks
     * can take up values ranging from 0 to 7. Generally speaking the formula
     * is: `0 to (2 ^ bit_width) - 1`
     *
     * @param i0 The subindex (variant) for the first five minute "block"
     * @param i5 The subindex (variant) for the second five minute "block"
     * @param i10 The subindex (variant) for the third five minute "block"
     * @param i15 The subindex (variant) for the fourth five minute "block"
     * @param i20 The subindex (variant) for the fifth five minute "block"
     * @param i25 The subindex (variant) for the sixth five minute "block"
     * @param i30 The subindex (variant) for the seventh five minute "block"
     * @param i35 The subindex (variant) for the eighth five minute "block"
     * @param i40 The subindex (variant) for the ninth five minute "block"
     * @param i45 The subindex (variant) for the tenth five minute "block"
     * @param i50 The subindex (variant) for the eleventh five minute "block"
     * @param i55 The subindex (variant) for the twelfth five minute "block"
     *
     * @return Bitset representing the given mode
     *
     * @see s_minData
     * @see s_modes
     */
    #define _SELECT_MODE(i0, i5, i10, i15, i20, i25, i30, i35, i40, i45, i50, i55) \
        (i0 | (i10 << 1) | (i15 << 2) | (i20 << 5) | (i25 << 6)  | (i30 << 7) | (i40 << 8) | (i45 << 9) | (i50 << 12) | (i55 << 13))

    /**
     * @brief Defines the mode for the "jester mode"
     *
     * This defines the "jester mode", which is may be added to "s_modes" when
     * activated (see DISPLAY_ADD_JESTER_MODE and
     * DISPLAY_USE_JESTER_MODE_ON_1ST_APRIL). The value of this doesn't
     * actually influence anything, because the mode will be selected by
     * chance each time anyway.
     *
     * @see s_minData
     * @see s_modes
     * @see DISPLAY_ADD_JESTER_MODE
     * @see DISPLAY_USE_JESTER_MODE_ON_1ST_APRIL
     */
    #define JESTER_MODE 0xffff

    /**
     * @brief Different modes that are implemented by this module
     *
     * This defines the different modes that this module implements, see
     * e_WcGerModes. The idea is that the user can effectively change the
     * modes, e.g. using the remote control.
     *
     * To make the definition easier, the macro _SELECT_MODE() is used here
     * quite heavily.
     *
     * The bit pattern for each of these modes looks like this:
     *
     * \verbatim
     * Bit number | Number of bits | Minutes | Variants
     *
     * 1    1   0       x UHR (x O'CLOCK)
     *                  NACH x UHR (PAST x O'CLOCK)
     *
     * 2    1   10      ZEHN NACH x (TEN PAST x)
     *                  ZWANZIG VOR HALB x+1 (TWENTY TO HALF x+1)
     *
     * 3    3   15      VIERTEL NACH x (QUARTER PAST x)
     *                  VIERTEL x+1 (QUARTER x+1)
     *                  VIERTEL VOR HALB x+1 (QUARTER TO HALF x+1)
     *                  DREIVIERTEL VOR x+1 (THREE QUARTERS TO x+1)
     *                  DREIVIERTEL NACH HALB x (THREE QUARTERS PAST HALF x)
     *
     * 6    1   20      ZEHN VOR HALB x+1 (TEN TO HALF x+1)
     *                  ZWANZIG NACH x (TWENTY PAST x)
     *
     * 7    1   25      FÜNF VOR HALB x+1 (FIVE TO HALF x+1)
     *                  VOR HALB x+1 (TO HALF x+1)
     *
     * 8    1   30      HALB x+1 (HALF x+1)
     *                  NACH HALB x+1 (PAST HALF x+1)
     *
     * 9    1   40      ZEHN NACH HALB x+1 (TEN PAST HALF x+1)
     *                  ZWANZIG VOR x+1 (TWENTY TO x+1)
     *
     * 10   3   45      VIERTEL VOR x+1 (QUARTER TO x+1)
     *                  DREIVIERTEL x+1 (THREE QUARTER x+1)
     *                  DREIVIERTEL NACH x (THREE QUARTERS PAST x)
     *                  VIERTEL NACH HALB x+1 (QUARTER PAST HALF x+1)
     *                  DREIVIERTEL VOR HALB x+2 (THREE QUARTERS TO HALF x+2)
     *
     * 13   1   50      ZEHN VOR x+1 (TEN TO x+1)
     *                  ZWANZIG NACH HALB x+1 (TWENTY PAST HALF x+1)
     *
     * 14   1   55      FÜNF VOR x+1 (FIVE TO x+1)
     *                  VOR x+1 (TO x+1)
     * \endverbatim
     *
     * Notice that "FÜNF NACH" (FIVE PAST) as well as "FÜNF NACH HALB" (FIVE
     * PAST HALF) are not encoded here, as there are no options for these two
     * five minute "blocks".
     *
     * There are basically four different modes, which will be added:
     *
     * - "Wessi"
     * - "Rhein-Ruhr"
     * - "Ossi"
     * - "Schwabe"
     *
     * For details about the modes itself, take a look at display_wc_ger3.h.
     *
     * If DISPLAY_ADD_JESTER_MODE is set to 1, the "jester mode" will be added,
     * too.
     *
     * @see e_WcGerModes
     * @see _SELECT_MODE
     * @see DISPLAY_ADD_JESTER_MODE
     * @see JESTER_MODE
     */
    static const uint16_t s_modes[] = {

        _SELECT_MODE(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0),
        _SELECT_MODE(0, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0),
        _SELECT_MODE(0, 0, 0, 1, 0, 0, 0, 0, 0, 1, 0, 0),
        _SELECT_MODE(0, 0, 0, 1, 1, 0, 0, 0, 1, 1, 0, 0),

        #if (DISPLAY_ADD_JESTER_MODE == 1)

            JESTER_MODE

        #endif

    };

    /*
     * Undefine helper macro as it is no longer needed
     */
    #undef _SELECT_MODE

    /**
     * @brief Makes it possible to set single bits within a display state
     *
     * This makes it easier to deal with display states, whenever single bits
     * need to be set. This is useful within s_numbers.
     *
     * The position is simply calculated by shifting a one to the the given
     * position. Multiple bits can be set by combining different states
     * (effectively "or"-ing them together).
     *
     * @see s_numbers
     * @see e_displayWordPos
     * @see display_state_t
     */
    #define _DISP_SETBIT(x) ((display_state_t)1 << x)

    /**
     * @brief Definition of a display state for each number from one to twelve
     *
     * This defines a display state for each number reaching from one to
     * twelve, which can be be used for one of two things:
     *
     * - Display the current hour as part of the current time, see
     * display_getTimeState().
     *
     * - Show only the number to give the user some feedback, see
     * display_getNumberDisplayState().
     *
     * The definitions itself are pretty straight forward. The only thing to
     * notice is that uint8_t would not be enough here, as there are more
     * than eight different words to control. Therefore uint16_t is used.
     *
     * @note The number one is defined as "eins". Depending upon the context it
     * is used in, it might be the case that "ein" is actually needed, e.g.
     * "ES IST EIN UHR" (It IS ONE O'CLOCK). In these cases the "s" has to be
     * disabled manually by a simple bit operation.
     *
     * @see _DISP_SETBIT()
     * @see display_getNumberDisplayState()
     */
    const uint16_t s_numbers[12] = {

        (_DISP_SETBIT(DWP_zwoelf)),
        (_DISP_SETBIT(DWP_ei) | _DISP_SETBIT(DWP_n) | _DISP_SETBIT(DWP_s)),
        (_DISP_SETBIT(DWP_zw) | _DISP_SETBIT(DWP_ei)),
        (_DISP_SETBIT(DWP_drei)),
        (_DISP_SETBIT(DWP_vier)),
        (_DISP_SETBIT(DWP_fuenf)),
        (_DISP_SETBIT(DWP_sechs)),
        (_DISP_SETBIT(DWP_s) | _DISP_SETBIT(DWP_ieben)),
        (_DISP_SETBIT(DWP_acht)),
        (_DISP_SETBIT(DWP_neun)),
        (_DISP_SETBIT(DWP_zehn)),
        (_DISP_SETBIT(DWP_elf)),

    };

    /*
     * Undefine helper macro as it is no longer needed
     */
    #undef _DISP_SETBIT

    /**
     * @brief Returns whether the "jester mode" should be activated or not
     *
     * This returns a value indicating whether or not the "jester mode" should
     * be activated based upon two factors:
     *
     * - The "jester mode" was added (see DISPLAY_ADD_JESTER_MODE) and is the
     *  currently chosen mode (see e_WcGerModes).
     *
     * - The software was compiled with support for DCF77 (ENABLE_DCF_SUPPORT)
     *  and the current date is April 1st while the software was compiled with
     * DISPLAY_USE_JESTER_MODE_ON_1ST_APRIL set to 1.
     *
     * One of these conditions must be met for this function to return true.
     * Otherwise it will return false, which means that the "jester mode"
     * should not be enabled.
     *
     * @param i_dateTime The current datetime
     * @param i_langmode The currently chosen langmode (see e_WcGerModes)
     *
     * @return True if the "jester mode" should be activated, otherwise false
     *
     * @see DISPLAY_ADD_JESTER_MODE
     * @see DISPLAY_USE_JESTER_MODE_ON_1ST_APRIL
     * @see ENABLE_DCF_SUPPORT
     * @see e_WcGerModes
     */
    static bool isJesterModeActive(const datetime_t* i_dateTime, e_WcGerModes i_langmode)
    {

        #if (DISPLAY_ADD_JESTER_MODE == 1)

            #if ((ENABLE_DCF_SUPPORT == 1) && (DISPLAY_USE_JESTER_MODE_ON_1ST_APRIL == 1))

                return (i_langmode == tm_jesterMode) || (i_dateTime->MM == 4  && i_dateTime->DD == 1);

            #else

                return (i_langmode == tm_jesterMode);

            #endif

        #else

            #if ((ENABLE_DCF_SUPPORT == 1) && (DISPLAY_USE_JESTER_MODE_ON_1ST_APRIL == 1))

                return (i_dateTime->MM == 4  && i_dateTime->DD == 1);

            #else

                return false;

            #endif

        #endif

    }

    /**
     * @see display.h
     */
    display_state_t display_getTimeState(const datetime_t* i_newDateTime)
    {

        uint8_t hour = i_newDateTime->hh;
        const uint8_t minutes = i_newDateTime->mm / 5;
        const uint8_t minuteLeds = i_newDateTime->mm % 5;
        uint8_t minuteLedSubState = 0;
        bool jesterMode;
        display_state_t leds;
        uint8_t langMode = g_displayParams->mode;

        #if (DISPLAY_DEACTIVATABLE_ITIS == 1)

            leds = 0;
            langMode /= 2;

            if (((g_displayParams->mode & 1) == 0) || (0 == minutes) || (6 == minutes)) {

                leds |= ((display_state_t)1 << DWP_itis);

            }

        #else

            leds = ((display_state_t)1 << DWP_itis);

        #endif

        jesterMode = isJesterModeActive(i_newDateTime, langMode);

        if (minutes == 0) {

            leds |= ((display_state_t)1 << DWP_clock);

        }

        uint8_t subInd;
        uint8_t ind;
        display_state_t hincTestBit;

        if (jesterMode) {

            subInd = prng_rand() % s_minVariants[minutes];

        } else {

            const uint16_t mode = s_modes[langMode];
            const uint8_t shift = s_modeShiftMask[minutes] & 0x0f;
            const uint8_t mask = s_modeShiftMask[minutes] >> 4;

            subInd = (mode >> shift) & mask;

        }

        ind = s_minStartInd[minutes] + subInd;

        leds |= ((display_state_t)(s_minData[ind])) << DWP_MIN_FIRST;

        hincTestBit = ((display_state_t)1) << ind;

        if (hincTestBit & s_hourInc1st) {

            ++hour;

        }

        if (hincTestBit & s_hourInc2nd) {

            ++hour;

        }

        if (jesterMode) {

            uint8_t r = prng_rand() % 4;

            if (minuteLeds <= 2) {

                minuteLedSubState |= (1 << r);

                if (minuteLeds == 2) {

                    uint8_t r2 = prng_rand() % 3;
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

        leds |= ((display_state_t)minuteLedSubState) << DWP_MIN_LEDS_BEGIN;

        leds |= display_getNumberDisplayState(hour);

        if ((hour == 1 || hour == 13) && (minutes == 0)) {

            leds &= ~((display_state_t)1 << DWP_s);

        }

        return leds;

    }

#endif /* WC_DISP_GER3 */
