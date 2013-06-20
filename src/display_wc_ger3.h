/*
 * Copyright (C) 2012, 2013 Karol Babioch <karol@babioch.de>
 * Copyright (c) 2012 Uwe Höß
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
 * @file display_wc_ger3.h
 * @brief Header file containing definitions specific to the German frontpanel
 *
 * This header file contains various definitions specific to the "new" German
 * frontpanel, which supports four different modes.
 *
 * The frontpanel looks basically like this:
 *
 * \verbatim
 *  E S K I S T L F Ü N F   => ES IST FÜNF
 *  Z E H N Z W A N Z I G   => ZEHN ZWANZIG
 *  D R E I V I E R T E L   => DREI|VIERTEL
 *  T G N A C H V O R J M   => NACH VOR
 *  H A L B Q Z W Ö L F P   => HALB ZWÖLF
 *  Z W E I N S I E B E N   => ZW|EI|N|S|IEBEN
 *  K D R E I R H F Ü N F   => DREI FÜNF
 *  E L F N E U N V I E R   => ELF NEUN VIER
 *  W A C H T Z E H N R S   => ACHT ZEHN
 *  B S E C H S F M U H R   => SECHS UHR
 * \endverbatim
 *
 * This basically enables the following modes:
 *
 * - "Wessi":
 *  - ES IST EIN UHR
 *  - ES IST FÜNF NACH EINS
 *  - ES IST ZEHN NACH EINS
 *  - ES IST VIERTEL NACH EINS
 *  - ES IST ZEHN VOR HALB ZWEI
 *  - ES IST FÜNF VOR HALB ZWEI
 *  - ES IST HALB ZWEI
 *  - ES IST FÜNF NACH HALB ZWEI
 *  - ES IST ZEHN NACH HALB ZWEI
 *  - ES IST VIERTEL VOR ZWEI
 *  - ES IST ZEHN VOR ZWEI
 *  - ES IST FÜNF VOR ZWEI
 *
 * - "Rhein-Ruhr":
 *  - ES IST EIN UHR
 *  - ES IST FÜNF NACH EINS
 *  - ES IST ZEHN NACH EINS
 *  - ES IST VIERTEL NACH EINS
 *  - ES IST ZWANZIG NACH EINS
 *  - ES IST FÜNF VOR HALB ZWEI
 *  - ES IST HALB ZWEI
 *  - ES IST FÜNF NACH HALB ZWEI
 *  - ES IST ZWANZIG VOR ZWEI
 *  - ES IST VIERTEL VOR ZWEI
 *  - ES IST ZEHN VOR ZWEI
 *  - ES IST FÜNF VOR ZWEI
 *
 * - "Ossi":
 *  - ES IST EIN UHR
 *  - ES IST FÜNF NACH EINS
 *  - ES IST ZEHN NACH EINS
 *  - ES IST VIERTEL ZWEI
 *  - ES IST ZEHN VOR HALB ZWEI
 *  - ES IST FÜNF VOR HALB ZWEI
 *  - ES IST HALB ZWEI
 *  - ES IST FÜNF NACH HALB ZWEI
 *  - ES IST ZEHN NACH HALB ZWEI
 *  - ES IST DREIVIERTEL ZWEI
 *  - ES IST ZEHN VOR ZWEI
 *  - ES IST FÜNF VOR ZWEI *
 *
 * - "Schwaben":
 *  - ES IST EIN UHR
 *  - ES IST FÜNF NACH EINS
 *  - ES IST ZEHN NACH EINS
 *  - ES IST VIERTEL ZWEI
 *  - ES IST ZWANZIG NACH EINS
 *  - ES IST FÜNF VOR HALB ZWEI
 *  - ES IST HALB ZWEI
 *  - ES IST FÜNF NACH HALB ZWEI
 *  - ES IST ZWANZIG VOR ZWEI
 *  - ES IST DREIVIERTEL ZWEI
 *  - ES IST ZEHN VOR ZWEI
 *  - ES IST FÜNF VOR ZWEI
 *
 * Support to disable the phrase "ES IST" (it is) during runtime can optionally
 * be compiled in, see DISPLAY_DEACTIVATABLE_ITIS.
 *
 * @note Keep in mind that this is the "new" German frontpanel, which
 * supports four different modes, whereas the "old" one (ger) supports only two
 * different modes.
 *
 * @note This file should be left untouched if making general adaptations to
 * the display module. Only things specific to the German language should be
 * put inside this file.
 *
 * @see display_wc_ger3.c
 */

#ifndef _WC_DISPLAY_GER3_H_
#define _WC_DISPLAY_GER3_H_

/**
 * @brief Defines whether the phrase "Es ist" (it is) should be deactivatable
 *
 * This controls whether the code to make the phrase "Es ist" (it is)
 * deactivatable during runtime. This makes it possible to offer different
 * modes, see e_WcGerModes, the user can choose from.
 *
 * To actually be deactivatable some extra code is needed, which can be saved
 * when this functionality is not needed. The default value is 1, so it will
 * be compiled into the binary.
 *
 * When this is enabled the amount of modes is actually doubled as each mode
 * is also offered with either the phrase "Es ist" (it is) enabled and/or
 * disabled.
 *
 * @see e_WcGerModes
 */
#define DISPLAY_DEACTIVATABLE_ITIS 1

/**
 * @brief Controls whether the "Jester mode" should be added
 *
 * If set to 1, the "Jester mode" is added to the list of available modes and
 * can be choosen by the user. The mode itself chooses randomly one of any
 * possible ways to display the time. This even includes some strange options
 * like "dreiviertel nach halb neun" ("quarter after half ten") instead of
 * "viertel nach neun" ("quarter past nine")
 *
 * This controls whether the code to make the phrase "Es ist" (it is)
 * deactivatable during runtime. This makes it possible to offer different
 * modes, see e_WcGerModes, the user can choose from.
 *
 * @see DISPLAY_USE_JESTER_MODE_ON_1ST_APRIL
 */
#define DISPLAY_ADD_JESTER_MODE 1

/**
 * @brief Controls whether the "Jester mode" should be activated on April 1st
 *
 * This will enable the "Jester mode" on April 1st each year as kind of an
 * April fools' joke. It will only work when a DCF receiver is present
 * (DCF_PRESENT) to make sure the current date is set and valid.
 *
 * The mode itself is described at DISPLAY_ADD_JESTER_MODE. This option can
 * be turned on and/or off independently of DISPLAY_ADD_JESTER_MODE itself.
 *
 * @see DCF_PRESENT
 * @see DISPLAY_ADD_JESTER_MODE
 */
#define DISPLAY_USE_JESTER_MODE_ON_1ST_APRIL 1

/**
 * @brief Enumeration defining the way in which the LEDs are connected to the
 *   board.
 *
 * To make various operations regarding the display state more efficient,
 * some implicit assumptions about the ordering of the items within this
 * enumerations are made. You should keep that in mind when actually trying to
 * customizing it.  Refer to DWP_MIN_FIRST, DWP_HOUR_BEGIN, DWP_LAST_SR_POS and
 * DWP_MIN_LEDS_BEGIN for some more details.
 *
 * @warning The order of these items may not be mixed arbitrarily.
 *
 * @see DWP_MIN_FIRST
 * @see DWP_HOUR_BEGIN
 * @see DWP_MIN_LEDS_BEGIN
 */
enum e_displayWordPos
{

    DWP_zw  = 0,
    DWP_ei,
    DWP_n,
    DWP_s,
    DWP_ieben,
    DWP_drei,
    DWP_vier,
    DWP_fuenf,
    DWP_sechs,
    DWP_acht,
    DWP_neun,
    DWP_zehn,
    DWP_elf,
    DWP_zwoelf,

    DWP_itis,
    DWP_clock,

    DWP_fuenfMin,
    DWP_zehnMin,
    DWP_zwanzigMin,
    DWP_dreiMin,
    DWP_viertel,
    DWP_nach,
    DWP_vor,
    DWP_halb,

    DWP_min1,
    DWP_min2,
    DWP_min3,
    DWP_min4,

    DWP_WORDSCOUNT

};

/**
 * @brief First item within e_displayWordPos representing a minute "block"
 *
 * This is expected to "point" at the first item within e_displayWordPos which
 * represents a word regarding the minutes (five, ten, ...).
 *
 * @see e_displayWordPos
 */
#define DWP_MIN_FIRST DWP_fuenfMin

/**
 * @brief First item within e_displayWordPos representing a hour
 *
 * This is expected to "point" at the first item within e_displayWordPost which
 * represents a word regarding the hour (1 to 12).
 *
 * @see e_displayWordPos
 */
#define DWP_HOUR_BEGIN DWP_zw

/**
 * @brief First item within e_displayWordPost representing the four minute LEDs
 *
 * This is expected to "point" at the first item within e_displayWordPost which
 * represents a minute LED. There are four of these and they are controlled
 * directly instead of indirectly via the shift registers. The minute LEDs are
 * expected to be ordered ascendingly, so various operations regarding this can
 * be implemented more efficiently.
 *
 * @see e_displayWordPos
 */
#define DWP_MIN_LEDS_BEGIN DWP_min1

 /**
 * @brief Containing different modes the user might choose from
 *
 * There are various modes of operation, which are explained in more detail
 * in the header of this file. This enumerations contains constants to
 * work with these modes internally. The chosen setting can be stored
 * persistently within the EEPROM, see DisplayEepromParams.
 *
 * If enabled (DISPLAY_ADD_JESTER_MODE) the "Jester mode" will be added along
 * with the others.
 *
 * @see DISPLAY_ADD_JESTER_MODE
 * @see DisplayEepromParams
 */
typedef enum e_WcGerModes {

    tm_wessi = 0,
    tm_rheinRuhr,
    tm_ossi,
    tm_swabian,

    #if (DISPLAY_ADD_JESTER_MODE == 1)

        tm_jesterMode,

    #endif

    TM_COUNT

} e_WcGerModes;

/**
 * @brief Containing the parameters of this module to be stored persistently
 *
 * @see DISPLAYEEPROMPARAMS_DEFAULT
 * @see wceeprom.h
 * @see e_WcGerModes
 */
struct DisplayEepromParams {

    e_WcGerModes mode;

};

/**
 * @brief Default settings of this module to be stored persistently
 *
 * @see DisplayEepromParams
 * @see wceeprom.h
*/
#define DISPLAYEEPROMPARAMS_DEFAULT { \
\
    (e_WcGerModes)0 \
\
}

/**
 * @see user.h
 */
#define DISPLAY_SPECIAL_USER_COMMANDS \
    UI_SELECT_DISP_MODE,

/**
 * @see user.h
 */
#define DISPLAY_SPECIAL_USER_COMMANDS_CODES \
    0x0008,

/**
 * @see DISPLAY_SPECIAL_USER_COMMANDS_HANDLER
 */
#define _DISP_TOGGLE_DISPMODE_CODE \
    ++g_displayParams->mode; \
    g_displayParams->mode %= (TM_COUNT * (DISPLAY_DEACTIVATABLE_ITIS + 1)); \
    addState(MS_showNumber, (void*)(g_displayParams->mode + 1)); \
    log_state("WRO\n");

/**
 * @see user.c
 */
#define DISPLAY_SPECIAL_USER_COMMANDS_HANDLER \
    USER_CREATE_IR_HANDLER(UI_SELECT_DISP_MODE, _DISP_TOGGLE_DISPMODE_CODE)

/**
 * @see display.h
 */
static inline DisplayState display_getMinuteMask()
{

    return ((DisplayState)1 << DWP_fuenfMin)
        | ((DisplayState)1 << DWP_zehnMin)
        | ((DisplayState)1 << DWP_zwanzigMin)
        | ((DisplayState)1 << DWP_dreiMin)
        | ((DisplayState)1 << DWP_viertel)
        | ((DisplayState)1 << DWP_nach)
        | ((DisplayState)1 << DWP_vor)
        | ((DisplayState)1 << DWP_halb)
        | ((DisplayState)1 << DWP_min1)
        | ((DisplayState)1 << DWP_min2)
        | ((DisplayState)1 << DWP_min3)
        | ((DisplayState)1 << DWP_min4);
}

/**
 * @see display.h
 */
static inline DisplayState display_getHoursMask()
{

    return ((DisplayState)1 << DWP_zw)
        | ((DisplayState)1 << DWP_ei)
        | ((DisplayState)1 << DWP_n)
        | ((DisplayState)1 << DWP_s)
        | ((DisplayState)1 << DWP_ieben)
        | ((DisplayState)1 << DWP_drei)
        | ((DisplayState)1 << DWP_vier)
        | ((DisplayState)1 << DWP_fuenf)
        | ((DisplayState)1 << DWP_sechs)
        | ((DisplayState)1 << DWP_acht)
        | ((DisplayState)1 << DWP_neun)
        | ((DisplayState)1 << DWP_zehn)
        | ((DisplayState)1 << DWP_elf)
        | ((DisplayState)1 << DWP_zwoelf);

}

/**
 * @see display.h
 */
static inline DisplayState display_getTimeSetIndicatorMask()
{

    return ((DisplayState)1 << DWP_clock);

}

/**
 * @see display.h
 */
static inline DisplayState display_getNumberDisplayState(uint8_t number)
{

    extern const uint16_t s_numbers[12];
    number = number % 12;

    return ((DisplayState)(s_numbers[number])) << DWP_HOUR_BEGIN;

}

#endif /* _WC_DISPLAY_GER3_H_ */
