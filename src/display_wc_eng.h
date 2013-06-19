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
 * @file display_wc_eng.h
 *
 *  This files contain the english language specific definitions that are required for display.
 *
 * \version $Id: display_wc_eng.h 411 2012-01-09 20:46:26Z vt $
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

#ifndef _WC_DISPLAY_ENG_H_
#define _WC_DISPLAY_ENG_H_

#define DISPLAY_DEACTIVATABLE_ITIS 1

enum e_displayWordPos
{

    DWP_itis = 0,
    DWP_fiveMin,
    DWP_tenMin,
    DWP_quarter,
    DWP_twenty,
    DWP_half,
    DWP_to,
    DWP_past,
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
    DWP_sr_nc1,
    DWP_sr_nc2,
    DWP_sr_nc3,
    DWP_min1,
    DWP_min2,
    DWP_min3,
    DWP_min4,

    DWP_WORDSCOUNT

};

#define DWP_MIN_FIRST DWP_fiveMin
#define DWP_HOUR_BEGIN DWP_one
#define DWP_LAST_SR_POS DWP_clock
#define DWP_MIN_LEDS_BEGIN DWP_min1

#if (DISPLAY_DEACTIVATABLE_ITIS == 1)

    typedef enum e_WcEngModes {

        tm_itIsOn = 0,
        tm_itIsOff,

        TM_COUNT

    } e_WcEngModes;

    struct DisplayEepromParams {

      e_WcEngModes mode;

    };

    #define DISPLAYEEPROMPARAMS_DEFAULT { \
        0 \
    }

    #define DISPLAY_SPECIAL_USER_COMMANDS \
        UI_SELECT_DISP_MODE,

    #define DISPLAY_SPECIAL_USER_COMMANDS_CODES \
        0x0008,

    #define _DISP_TOGGLE_DISPMODE_CODE \
        ++g_displayParams->mode; \
        g_displayParams->mode %= TM_COUNT; \
        addState(MS_showNumber, (void*)(g_displayParams->mode + 1)); \
        log_state("DM\n");

    #define DISPLAY_SPECIAL_USER_COMMANDS_HANDLER \
        USER_CREATE_IR_HANDLER(UI_SELECT_DISP_MODE, _DISP_TOGGLE_DISPMODE_CODE)

#else

    struct DisplayEepromParams {

        uint8_t dummy;

    };

    #define DISPLAYEEPROMPARAMS_DEFAULT { \

        0 \

    }

    #define DISPLAY_SPECIAL_USER_COMMANDS
    #define DISPLAY_SPECIAL_USER_COMMANDS_CODES
    #define DISPLAY_SPECIAL_USER_COMMANDS_HANDLER

#endif

static inline DisplayState display_getMinuteMask()
{

    return (1L << DWP_fiveMin)
        | (1L << DWP_tenMin)
        | (1L << DWP_quarter)
        | (1L << DWP_twenty)
        | (1L << DWP_half)
        | (1L << DWP_to)
        | (1L << DWP_past)
        | (1L << DWP_min1)
        | (1L << DWP_min2)
        | (1L << DWP_min3)
        | (1L << DWP_min4);

}

static inline DisplayState display_getHoursMask()
{

    return (1L << DWP_one)
        | (1L << DWP_two)
        | (1L << DWP_three)
        | (1L << DWP_four)
        | (1L << DWP_five)
        | (1L << DWP_six)
        | (1L << DWP_seven)
        | (1L << DWP_eight)
        | (1L << DWP_nine)
        | (1L << DWP_ten)
        | (1L << DWP_eleven)
        | (1L << DWP_twelve);

}

static inline DisplayState display_getTimeSetIndicatorMask()
{

    return (1L << DWP_clock);

}

static inline DisplayState display_getNumberDisplayState(uint8_t number)
{

    number = number % 12;

    if (number == 0) {

        number = 12;

    }

    return (1L << (number + DWP_HOUR_BEGIN - 1));

}

#endif /* _WC_DISPLAY_ENG_H_ */
