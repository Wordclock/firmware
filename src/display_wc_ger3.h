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

#define DISPLAY_DEACTIVATABLE_ITIS 1

#define DISPLAY_ADD_JESTER_MODE 1

#define DISPLAY_USE_JESTER_MODE_ON_1ST_APRIL 1

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

#define DWP_MIN_FIRST DWP_fuenfMin

#define DWP_HOUR_BEGIN DWP_zw

#define DWP_MIN_LEDS_BEGIN DWP_min1

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

struct DisplayEepromParams {

    e_WcGerModes mode;

};

#define DISPLAYEEPROMPARAMS_DEFAULT { \
\
    (e_WcGerModes)0 \
\
}

#define DISPLAY_SPECIAL_USER_COMMANDS \
    UI_SELECT_DISP_MODE,

#define DISPLAY_SPECIAL_USER_COMMANDS_CODES \
    0x0008,

#define _DISP_TOGGLE_DISPMODE_CODE \
    ++g_displayParams->mode; \
    g_displayParams->mode %= (TM_COUNT * (DISPLAY_DEACTIVATABLE_ITIS + 1)); \
    addState(MS_showNumber, (void*)(g_displayParams->mode + 1)); \
    log_state("WRO\n");

#define DISPLAY_SPECIAL_USER_COMMANDS_HANDLER \
    USER_CREATE_IR_HANDLER(UI_SELECT_DISP_MODE, _DISP_TOGGLE_DISPMODE_CODE)

static inline DisplayState display_getMinuteMask()
{

    return (1L << DWP_fuenfMin)
        | (1L << DWP_zehnMin)
        | (1L << DWP_zwanzigMin)
        | (1L << DWP_dreiMin)
        | (1L << DWP_viertel)
        | (1L << DWP_nach)
        | (1L << DWP_vor)
        | (1L << DWP_halb)
        | (1L << DWP_min1)
        | (1L << DWP_min2)
        | (1L << DWP_min3)
        | (1L << DWP_min4);
}

static inline DisplayState display_getHoursMask()
{

    return (1L << DWP_zw)
        | (1L << DWP_ei)
        | (1L << DWP_n)
        | (1L << DWP_s)
        | (1L << DWP_ieben)
        | (1L << DWP_drei)
        | (1L << DWP_vier)
        | (1L << DWP_fuenf)
        | (1L << DWP_sechs)
        | (1L << DWP_acht)
        | (1L << DWP_neun)
        | (1L << DWP_zehn)
        | (1L << DWP_elf)
        | (1L << DWP_zwoelf);

}

static inline DisplayState display_getTimeSetIndicatorMask()
{

    return (1L << DWP_clock);

}

static inline DisplayState display_getNumberDisplayState(uint8_t number)
{

    extern const uint16_t s_numbers[12];
    number = number % 12;

    return ((DisplayState)(s_numbers[number])) << DWP_HOUR_BEGIN;

}

#endif /* _WC_DISPLAY_GER3_H_ */
