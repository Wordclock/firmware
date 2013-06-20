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
 * @file display_wc_ger.h
 * @brief Header file containing definitions specific to the German frontpanel
 *
 * This header file contains various definitions specific to the German
 * frontpanel. Keep in mind that this is the "old" German frontpanel, which
 * only supporst two different modes, whereas the new one (ger3) supports
 * three different modes.
 *
 * The frontpanel looks basically like this:
 *
 * \verbatim
 *  E S K I S T A F Ü N F   => ES IST FÜNF
 *  U Z E H N F M V O R G   => ZEHN VOR
 *  D R E I V I E R T E L   => DREI VIERTEL
 *  N A C H V O R H A L B   => NACH VOR HALB
 *  X F Ü N F R S Z W E I   => FÜNF ZWEI
 *  S I E B E N A V I E R   => SIEBEN VIER
 *  Z E H N T G S E C H S   => ZEHN SECHS
 *  L D R E I U A C H T J   => DREI ACHT
 *  E L F N E U N E I N S   => ELF NEUN EIN|S
 *  B Z W Ö L F R H U H R   => ZWÖLF UHR
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
 *  - ES IST FÜNF VOR ZWEI
 *
 * Support to disable the phrase "ES IST" (it is) during runtime can optionally
 * be compiled in, see DISPLAY_DEACTIVATABLE_ITIS.
 *
 * @note This file should be left untouched if making general adaptations to
 * the display module. Only things specific to the German language should be
 * put inside this file.
 *
 * @see display_wc_ger.c
 */

#ifndef _WC_DISPLAY_GER_H_
#define _WC_DISPLAY_GER_H_

#define DISPLAY_DEACTIVATABLE_ITIS 1

enum e_displayWordPos
{

    DWP_itis = 0,
    DWP_fuenfMin,
    DWP_zehnMin,
    DWP_vorMin,
    DWP_dreiHour,
    DWP_viertel,
    DWP_nach,
    DWP_vorHour,
    DWP_halb,
    DWP_s,
    DWP_one,
    DWP_two,
    DWP_three,
    DWP_four,
    DWP_five,
    DWP_six,
    DWP_seven,
    DWP_eight,
    DWP_nine,
    DWP_ten,
    DWP_eleven,
    DWP_twelve,
    DWP_clock,
    DWP_sr_nc,
    DWP_min1,
    DWP_min2,
    DWP_min3,
    DWP_min4,

    DWP_WORDSCOUNT

};

#define DWP_MIN_FIRST DWP_fuenfMin

#define DWP_HOUR_BEGIN DWP_one

#define DWP_LAST_SR_POS DWP_clock

#define DWP_MIN_LEDS_BEGIN DWP_min1

typedef enum e_WcGerModes {

    tm_wessi = 0,
    tm_ossi,

    TM_COUNT

} e_WcGerModes;

struct DisplayEepromParams {

    e_WcGerModes mode;

};

#define DISPLAYEEPROMPARAMS_DEFAULT { \
\
    0 \
\
}

#define DISPLAY_SPECIAL_USER_COMMANDS \
    UI_SELECT_DISP_MODE,

#define DISPLAY_SPECIAL_USER_COMMANDS_CODES \
    0x0008,

#define _DISP_TOGGLE_DISPMODE_CODE \
    ++g_displayParams->mode; \
    g_displayParams->mode %= (TM_COUNT*(DISPLAY_DEACTIVATABLE_ITIS + 1)); \
    addState( MS_showNumber, (void*)(g_displayParams->mode + 1)); \
    log_state("WO\n");

#define DISPLAY_SPECIAL_USER_COMMANDS_HANDLER \
    USER_CREATE_IR_HANDLER(UI_SELECT_DISP_MODE, _DISP_TOGGLE_DISPMODE_CODE)

static inline DisplayState display_getMinuteMask()
{

    return ((DisplayState)1 << DWP_fuenfMin)
        | ((DisplayState)1 << DWP_zehnMin)
        | ((DisplayState)1 << DWP_vorMin)
        | ((DisplayState)1 << DWP_dreiHour)
        | ((DisplayState)1 << DWP_viertel)
        | ((DisplayState)1 << DWP_nach)
        | ((DisplayState)1 << DWP_vorHour)
        | ((DisplayState)1 << DWP_halb)
        | ((DisplayState)1 << DWP_min1)
        | ((DisplayState)1 << DWP_min2)
        | ((DisplayState)1 << DWP_min3)
        | ((DisplayState)1 << DWP_min4);

}

static inline DisplayState display_getHoursMask()
{

    return ((DisplayState)1 << DWP_s)
        | ((DisplayState)1 << DWP_one)
        | ((DisplayState)1 << DWP_two)
        | ((DisplayState)1 << DWP_three)
        | ((DisplayState)1 << DWP_four)
        | ((DisplayState)1 << DWP_five)
        | ((DisplayState)1 << DWP_six)
        | ((DisplayState)1 << DWP_seven)
        | ((DisplayState)1 << DWP_eight)
        | ((DisplayState)1 << DWP_nine)
        | ((DisplayState)1 << DWP_ten)
        | ((DisplayState)1 << DWP_eleven)
        | ((DisplayState)1 << DWP_twelve);

}

static inline DisplayState display_getTimeSetIndicatorMask()
{

    return ((DisplayState)1 << DWP_clock);

}

static inline DisplayState display_getNumberDisplayState(uint8_t number)
{

    DisplayState res;

    number = number % 12;

    if (number == 0) {

        number = 12;

    }

    res = ((DisplayState)1 << (number + DWP_HOUR_BEGIN - 1));

    return (number == 1) ? res | ((DisplayState)1 << DWP_s) : res;

}

#endif /* _WC_DISPLAY_GER_H_ */
