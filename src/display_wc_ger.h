/*
 * Copyright (C) 2012, 2013, 2014 Karol Babioch <karol@babioch.de>
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
 * This header file contains various definitions specific to the "old" German
 * frontpanel.
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
 * @note Keep in mind that this is the "old" German frontpanel, which only
 * supports two different modes, whereas the new one (ger3) supports three
 * different modes.
 *
 * @note This file should be left untouched if making general adaptations to
 * the display module. Only things specific to the German language should be
 * put inside this file.
 *
 * @see display_wc_ger.c
 */

#ifndef _WC_DISPLAY_GER_H_
#define _WC_DISPLAY_GER_H_

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
 * ("Ossi" & "Wessi) is also offered with either the phrase "Es ist" (it is)
 * enabled and/or disabled.
 *
 * @see e_WcGerModes
 */
#define DISPLAY_DEACTIVATABLE_ITIS 1

/**
 * @brief Enumeration defining the way in which the LEDs are connected to the
 *   board.
 *
 * To make various operations regarding the display state more efficient,
 * some implicit assumptions about the ordering of the items within this
 * enumerations are made:
 *
 * - The "five minute" blocks (5, 10, ...) are placed one after the other
 *   and are ordered consecutively.
 *
 * - The hours are placed one after the other and are ordered consecutively.
 *
 * You should keep that in mind when actually trying to customizing it.  Refer
 * to DWP_MIN_FIRST, DWP_HOUR_BEGIN, DWP_LAST_SR_POS and DWP_MIN_LEDS_BEGIN for
 * some more details.
 *
 * @warning The order of these items cannot be mixed arbitrarily.
 *
 * @see DWP_MIN_FIRST
 * @see DWP_HOUR_BEGIN
 * @see DWP_LAST_SR_POS
 * @see DWP_MIN_LEDS_BEGIN
 */
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

/**
 * @brief First item within e_displayWordPos representing a minute "block"
 *
 * This is expected to "point" at the first item within e_displayWordPos which
 * represents a "five-minute" block (5, 10, 15, 20, 30). The items within this
 * group of blocks itself are expected to be ordered ascendingly, so various
 * operations regarding this can be implemented more efficiently.
 *
 * @see e_displayWordPos
 */
#define DWP_MIN_FIRST DWP_fuenfMin

/**
 * @brief First item within e_displayWordPos representing an hour
 *
 * This is expected to "point" at the first item within e_displayWordPost which
 * represents an hour (1 to 12). The items within this group itself are
 * expected to be ordered ascendingly, so various operations regarding this can
 * be implemented more efficiently.
 *
 * @see e_displayWordPos
 */
#define DWP_HOUR_BEGIN DWP_one

/**
 * @brief Last item within e_displayWordPost connected to the shift registers
 *
 * This is expected to "point" at the last item within e_displayWordPost which
 * is actually controlled by the shift register cascade. Items following this
 * one are then in return controlled directly by I/O operations on the
 * appropriate pins.
 *
 * @see e_displayWordPos
 */
#define DWP_LAST_SR_POS DWP_clock

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
* There are two modes of operation referred to as "Wessi" and "Ossi". See the
* header of this file for details. This enumerations contains constants to
* work with these modes internally. The chosen setting can be stored
* persistently within the EEPROM, see DisplayEepromParams.
*
* @see DisplayEepromParams
*/
typedef enum e_WcGerModes {

    tm_wessi = 0,
    tm_ossi,

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

    /**
     * @brief Chosen mode
     *
     * This contains the chosen mode, which is a value from e_WcGerModes.
     *
     * @see e_WcGerModes
     */
    e_WcGerModes mode;

};

/**
 * @brief Default values of this module that should be stored persistently
 *
 * This defines the default values for this module. Refer to
 * DisplayEepromParams for a detailed description of each member.
 *
 * @note This will also be the values used after flashing the firmware to the
 * microcontroller, so make sure that the defaults are actually useful.
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
 * @brief Defines the item within user_command_t for changing the mode
 *
 * This represents the command, which enables the mode to be changed. The
 * implemented modes can be found within e_WcGerModes. The default value is
 * defined in DISPLAY_SPECIAL_USER_COMMANDS_CODES. The actual code, which will
 * be executed by pressing this key, is defined in _DISP_TOGGLE_DISPMODE_CODE.
 *
 * @see e_WcGerModes
 * @see DISPLAY_SPECIAL_USER_COMMANDS_CODES
 * @see _DISP_TOGGLE_DISPMODE_CODE
*/
#define DISPLAY_SPECIAL_USER_COMMANDS UC_SELECT_DISP_MODE,

/**
 * @brief Defines the default key code for changing the mode
 *
 * This defines the default key code for changing the mode. The implemented
 * modes can be found within e_WcGerModes. The actual code, which will be
 * executed by pressing this key, is defined in _DISP_TOGGLE_DISPMODE_CODE.
 *
 * @see e_WcGerModes
 * @see DISPLAY_SPECIAL_USER_COMMANDS
 * @see _DISP_TOGGLE_DISPMODE_CODE
*/
#define DISPLAY_SPECIAL_USER_COMMANDS_CODES 0x0014,

/**
 * @brief Actual code to be executed when pressing the mode changing key
 *
 * This defines the actual code, which will be executed by pressing the key
 * responsible for changing the mode, see DISPLAY_SPECIAL_USER_COMMANDS. The
 * implemented modes itself are listed within e_WcGerModes.
 *
 * @see e_WcGerModes
 * @see DISPLAY_SPECIAL_USER_COMMANDS
 * @see DISPLAY_SPECIAL_USER_COMMANDS_CODES
*/
#define _DISP_TOGGLE_DISPMODE_CODE \
    ++g_displayParams->mode; \
    g_displayParams->mode %= (TM_COUNT*(DISPLAY_DEACTIVATABLE_ITIS + 1)); \
    addState( MS_showNumber, (void*)(g_displayParams->mode + 1)); \
    log_state("WO\n");

/**
 * @brief IR handler responsible for executing the code when pressing the key
 *
 * This is the actual IR handler, which will generate the appropriate else
 * branch for handling the key press defined in UC_SELECT_DISP_MODE. It
 * will make it possible to execute the given code in
 * _DISP_TOGGLE_DISPMODE_CODE when the appropriate key press is detected.
 *
 * @see DISPLAY_SPECIAL_USER_COMMANDS
 * @see DISPLAY_SPECIAL_USER_COMMANDS_CODES
*/
#define DISPLAY_SPECIAL_USER_COMMANDS_HANDLER \
    USER_CREATE_IR_HANDLER(UC_SELECT_DISP_MODE, _DISP_TOGGLE_DISPMODE_CODE)

/**
 * @see display.h
 */
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

/**
 * @see display.h
 */
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

    DisplayState res;

    number = number % 12;

    if (number == 0) {

        number = 12;

    }

    res = ((DisplayState)1 << (number + DWP_HOUR_BEGIN - 1));

    return (number == 1) ? res | ((DisplayState)1 << DWP_s) : res;

}

#endif /* _WC_DISPLAY_GER_H_ */
