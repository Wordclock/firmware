/*
 * Copyright (C) 2013 Karol Babioch <karol@babioch.de>
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
 * @file user_command.h
 * @brief Header containing an enumeration with all of the valid user commands
 *
 * This header files contains all of the valid user commands that can be
 * initiated by the user and defines it as an own type. Each module that
 * interacts with the user should make use of it.
 *
 * @see user_command_t
 */

#ifndef _WC_USER_COMMAND_TIMER_H_
#define _WC_USER_COMMAND_TIMER_H_

#include "main.h"
#include "display.h"

/**
 * @brief Enumeration of valid commands that can be executed
 *
 * This implicitly also defines the order in which the codes are expected to be
 * trained during the "training phase", see menu_state_t::MS_irTrain.
 *
 * If the flag for an individual configuration is set (INDIVIDUAL_CONFIG) then
 * this enumeration is built dynamically and it depends upon the current
 * settings which members will be included and which won't.
 *
 * The commands itself can be executed by passing items from this enumeration
 * to UserState_HandleUserCommand() as argument.
 *
 * @note If changing anything here, make sure to also change the default
 * settings within USER_COMMANDCODES_DEFAULTS.
 *
 * @see INDIVIDUAL_CONFIG
 * @see UserState_HandleUserCommand()
 */
typedef enum
{

    /**
     * @brief Controls the "On" and/or "Off" status of the display
     *
     * This represents the command, which can enable and/or disable the display
     * (along with the optional Ambilight). This command actually toggles
     * between "On" and/or "Off".
     */
    UC_ONOFF = 0,

    /**
     * @brief Represents an increase in brightness
     */
    UC_BRIGHTNESS_UP,

    /**
     * @brief Represents a decrease in brightness
     */
    UC_BRIGHTNESS_DOWN,

    /**
     * @brief Represents an "up" and/or "increase"
     *
     * The exact meaning of this command depends upon the current state the
     * clock is in. Generally speaking it is used to change the value within
     * the current menu in an "upward" direction.
     */
    UC_UP,

    /**
     * @brief Represents a "down" and/or "decrease"
     *
     * The exact meaning of this command depends upon the current state the
     * clock is in. Generally speaking it is used to change the value within
     * the current menu in an "downward" direction.
     */
    UC_DOWN,

    /**
     * @brief Initializes the manual setting of the time
     *
     * This is the command for entering the menu, which enables the user to
     * setup the time manually. This is especially useful when no (valid) DCF77
     * signal can be received.
     */
    UC_SET_TIME,

    /**
     * @brief Initializes the configuration of the on/off time(s)
     *
     * This is the command for entering the menu, which enables the user to
     * setup the on/off (autoOff) time(s).
     *
     * @see menu_state_t::MS_setOnOffTime
     */
    UC_SET_ONOFF_TIMES,

    #if (INDIVIDUAL_CONFIG == 0 || ENABLE_DCF_SUPPORT == 1)

        /**
         * @brief Forces to try to get an update with DCF77
         *
         * Usually the DCF77 decoding is only done once within an hour. This
         * command will enforce to try to get another update, even if there
         * already has been received within the current hour.
         *
         * This will effectively just execute dcf77_enable().
         *
         * @see dcf77_enable()
         */
        UC_DCF_GET_TIME,

    #endif

    /**
     * @brief Display the time in the "normal" mode
     *
     * This represents the command, which will switch to the "normal" display
     * mode (menu_state_t::MS_normalMode). The current time will be displayed
     * in a single color. This command can also switch the currently used color
     * profile (assuming MONO_COLOR_CLOCK is not set), which effectively
     * changes the color the current time is displayed in.
     */
    UC_NORMAL_MODE,

    /**
     * @brief Display the time in the "pulse" mode
     *
     * This represents the command, which will switch to the "pulse" display
     * mode. The color will "pulse", meaning the brightness will alternatively
     * increase and/or decrease. This is possible in both, the "normal"
     * (menu_state_t::MS_normalMode) and in the "hue fading"
     * (menu_state_t::MS_hueMode) mode.
     */
    UC_PULSE_MODE,

    /**
     * @brief Enters the "demo" mode
     *
     * The "demo" mode (menu_state_t::MS_demoMode) makes it easier to test the
     * construction as not working LEDs can be spotted quite easily. There are
     * two different "demo" modes:
     *
     *  - The "normal" mode will display all the words in sequence, iterating
     *  over them one by one in sequence.
     *
     *  - The "fast" mode will display all the words simultaneously. This
     *  is actually achieved by multiplexing.
     */
    UC_DEMO_MODE,

    #if (INDIVIDUAL_CONFIG == 0 || MONO_COLOR_CLOCK != 1)

        /**
         * @brief Displays the time in the "hue fading" mode
         *
         * This represents the command, which will switch to the "hue fading"
         * display mode (menu_state_t::MS_hueMode). The color the current time
         * will be displayed in will slowly change.
         */
        UC_HUE_MODE,

        /**
         * @brief Change the red channel of the color with "up" and/or "down"
         *
         * This enables the red channel to be changed by issuing the "up"
         * (user_command_t::UC_UP) and/or "down" (user_command_t::UC_DOWN)
         * command.
         */
        UC_CHANGE_R,

        /**
         * @brief Change the green channel of the color with "up" and/or "down"
         *
         * This enables the green channel to be changed by issuing the "up"
         * (user_command_t::UC_UP) and/or "down" (user_command_t::UC_DOWN)
         * command.
         */
        UC_CHANGE_G,

        /**
         * @brief Change the blue channel of the color  with "up" and/or "down"
         *
         * This enables the blue channel to be changed by issuing the "up"
         * (user_command_t::UC_UP) and/or "down" (user_command_t::UC_DOWN)
         * command.
         */
        UC_CHANGE_B,

        /**
         * @brief Change the current hue of the color with "up" and/or "down"
         *
         * This enables the current hue to be changed by issuing the "up"
         * (user_command_t::UC_UP) and/or "down" (user_command_t::UC_DOWN)
         * commands.
         */
        UC_CHANGE_HUE,

    #endif

    /**
     * @brief Sets the current brightness as base for adjustments of the LDR
     *
     * Changes to the ambient lightning will be relative to the brightness set
     * here. Refer to pwm.h for details.
     *
     * @see pwm_modifyLdrBrightness2pwmStep()
     */
    UC_CALIB_BRIGHTNESS,

    #if (INDIVIDUAL_CONFIG == 0 || AMBILIGHT_PRESENT == 1)

        /**
         * @brief Controls the "On" and/or "Off" status of the Ambilight
         *
         * This represents the command, which can enable and/or disable the
         * Ambilight. This command actually toggles between "On" and/or "Off".
         *
         * @see USER_AMBILIGHT
         */
        UC_AMBILIGHT,

    #endif

    #if (INDIVIDUAL_CONFIG == 0 || BLUETOOTH_PRESENT == 1)

        /**
         * @brief Controls the "On" and/or "Off" status of the Bluetooth GPO
         *
         * This represents the command, which can enable and/or disable the GPO
         * dedicated to a Bluetooth module, effectively toggling the Bluetooth
         * transceiver "On" and/or "Off".
         *
         * @see USER_BLUETOOTH
         */
        UC_BLUETOOTH,

    #endif

    #if (INDIVIDUAL_CONFIG == 0 || AUXPOWER_PRESENT == 1)

        /**
         * @brief Controls the "On" and/or "Off" status of the auxiliary GPO
         *
         * This represents the command, which can enable and/or disable the
         * auxiliary GPO, which can be used to control some individual hardware
         * attached to it.
         *
         * @see USER_AUXPOWER
         */
        UC_AUXPOWER,

    #endif

    /**
     * @brief Command specific to the chosen display type
     *
     * This represents a command, which is defined by the chosen display type
     * and/or language. Normally it is used to toggle between the offered
     * modes. For details refer to the actually used display type.
     */
    DISPLAY_SPECIAL_USER_COMMANDS

    /**
     * @brief Number of commands defined within this enumeration
     */
    UC_COMMAND_COUNT

} user_command_t;

#endif /* _WC_USER_COMMAND_TIMER_H_ */
