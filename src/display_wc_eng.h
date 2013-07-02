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
 * @file display_wc_eng.h
 * @brief Header file containing definitions specific to the English frontpanel
 *
 * This header file contains various definitions specific to the English
 * frontpanel. Among other things this includes the positioning of the words,
 * see e_displayWordPos.
 *
 * @note This file should be left untouched if making general adaptations to
 * the display module. Only things specific to the English language should be
 * put inside this file.
 *
 * @see display_wc_eng.c
 */

#ifndef _WC_DISPLAY_ENG_H_
#define _WC_DISPLAY_ENG_H_

/**
 * @brief Defines whether the phrase "It is" should be deactivatable
 *
 * The user might want to activate and/or deactivate the "It is" mode. This
 * is usually done by offering different modes (see e_WcEngModes), which
 * the user can choose from.
 *
 * To actually be deactivatable some extra code is needed, which can be saved
 * when this functionality is not needed. The default value is 1, so it will
 * be compiled into the binary.
 *
 * @see e_WcEngModes
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
#define DWP_MIN_FIRST DWP_fiveMin

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

/*
 * Check whether this code actually has to be compiled
 */
#if (DISPLAY_DEACTIVATABLE_ITIS == 1)

    /**
    * @brief Representing different modes the user might choose from
    *
    * If the LED group representing "It is" can be deactivated
    * (DISPLAY_DEACTIVATABLE_ITIS) there are two modes: The first one will
    * have this phrase enabled, the second one disabled. The user might
    * choose on of these modes. The setting can be stored persistently
    * within the EEPROM, see DisplayEepromParams.
    *
    * @see DISPLAY_DEACTIVATABLE_ITIS
    * @see DisplayEepromParams
    */
    typedef enum e_WcEngModes {

        tm_itIsOn = 0,
        tm_itIsOff,

        TM_COUNT

    } e_WcEngModes;

    /**
    * @brief Containing the parameters of this module to be stored persistently
    *
    * @see DISPLAYEEPROMPARAMS_DEFAULT
    * @see wceeprom.h
    * @see e_WcEngModes
    */
    struct DisplayEepromParams {

        /**
         * @brief Chosen mode
         *
         * This contains the chosen mode, which is a value from e_WcEngModes.
         *
         * @see e_WcEngModes
         */
        e_WcEngModes mode;

    };

    /**
     * @brief Default values of this module that should be stored persistently
     *
     * This defines the default values for this module. Refer to
     * DisplayEepromParams for a detailed description of each member.
     *
     * @note This will also be the values used after flashing the firmware to
     * the microcontroller, so make sure that the defaults are actually useful.
     *
     * @see DisplayEepromParams
     * @see wceeprom.h
     */
    #define DISPLAYEEPROMPARAMS_DEFAULT { \
    \
        (e_WcEngModes)0 \
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
        g_displayParams->mode %= TM_COUNT; \
        addState(MS_showNumber, (void*)(g_displayParams->mode + 1)); \
        log_state("DM\n");

    /**
     * @see user.c
     */
    #define DISPLAY_SPECIAL_USER_COMMANDS_HANDLER \
        USER_CREATE_IR_HANDLER(UI_SELECT_DISP_MODE, _DISP_TOGGLE_DISPMODE_CODE)

#else

    /**
    * @brief Containing a dummy parameter to be stored persistently
    *
    * This is needed to have this struct available even when the appropriate
    * functionality should not be compiled (DISPLAY_DEACTIVATABLE_ITIS).
    *
    * @see DISPLAY_DEACTIVATABLE_ITIS
    */
    struct DisplayEepromParams {

        uint8_t dummy;

    };

    /**
    * @brief Default settings of this module for the dummy parameter
    *
    * @see DisplayEepromParams
    * @see wceeprom.h
    */
    #define DISPLAYEEPROMPARAMS_DEFAULT { \
    \
        0 \
    \
    }

    /**
     * @brief Defines the item within user.h::e_userCommands for changing the mode
     *
     * This represents the command, which enables the mode to be changed. The
     * implemented modes can be found within e_WcGerModes. The default value is
     * defined in DISPLAY_SPECIAL_USER_COMMANDS_CODES.
     *
     * In case of this display frontpanel it is left empty.
     *
     * @see e_WcEngModes
     * @see DISPLAY_SPECIAL_USER_COMMANDS_CODES
    */
    #define DISPLAY_SPECIAL_USER_COMMANDS

    /**
     * @brief Defines the default key code for changing the mode
     *
     * This defines the default key code for changing the mode. The implemented
     * modes can be found within e_WcGerModes.
     *
     * In case of this display frontpanel it is left empty.
     *
     * @see e_WcEngModes
     * @see DISPLAY_SPECIAL_USER_COMMANDS
    */
    #define DISPLAY_SPECIAL_USER_COMMANDS_CODES

    /**
     * @brief IR handler responsible for executing the code when pressing the key
     *
     * This is the actual IR handler, which in case of this display frontpanel
     * is simply left empty as this functionality is not being used.
     *
     * @see DISPLAY_SPECIAL_USER_COMMANDS
     * @see DISPLAY_SPECIAL_USER_COMMANDS_CODES
    */
    #define DISPLAY_SPECIAL_USER_COMMANDS_HANDLER

#endif

/**
 * @see display.h
 */
static inline DisplayState display_getMinuteMask()
{

    return ((DisplayState)1 << DWP_fiveMin)
        | ((DisplayState)1 << DWP_tenMin)
        | ((DisplayState)1 << DWP_quarter)
        | ((DisplayState)1 << DWP_twenty)
        | ((DisplayState)1 << DWP_half)
        | ((DisplayState)1 << DWP_to)
        | ((DisplayState)1 << DWP_past)
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

    return ((DisplayState)1 << DWP_one)
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

    number = number % 12;

    if (number == 0) {

        number = 12;

    }

    return ((DisplayState)1 << (number + DWP_HOUR_BEGIN - 1));

}

#endif /* _WC_DISPLAY_ENG_H_ */
