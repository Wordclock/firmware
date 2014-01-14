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
 * @file user.h
 * @brief Header file of the user module implementing the user interface
 *
 * The user module implements the user interface of the Wordclock. Beside
 * being responsible for the output of the current time this also includes
 * various menus which the user can interact with via remote control to
 * influence the behavior of the Wordclock.
 *
 * @see user.c
 * @see usermodes.c
 */

#ifndef _WC_USER_H_
#define _WC_USER_H_

#include <stdbool.h>

#include "main.h"
#include "user_command.h"

/**
 * @brief Enumeration of all the states the Wordclock can enter
 *
 * This defines all the different states (and/or menus) the Wordclock can enter
 * or reside in. Besides actually showing the current time
 * (menu_state_t::MS_normalMode) there are a couple of different states,
 * which are needed to enable the possibility for interaction with the user.
 *
 * @warning Some "modes" may not be available due to configuration settings.
 *
 * @see MONO_COLOR_CLOCK
 */
typedef enum
{

    /**
     * @brief Represents the "training" mode
     *
     * This mode is used to train the IR codes for the various commands defined
     * in user_command_t.
     *
     * @see TrainIrState_enter()
     * @see user_command_t
     */
    MS_irTrain = 0,

    /**
     * @brief Represents the "normal" mode
     *
     * This represents the "normal" mode used for displaying the current time.
     *
     * @see NormalState_enter()
     */
    MS_normalMode,

    /**
     * @brief Represents the "demo" mode
     *
     * This represents the "demo" mode, which can be used to debug the hardware
     * by enabling all of the LED groups - either by iterating over them or
     * by enabling all at once using multiplexing.
     *
     * @see DemoState_init()
     */
    MS_demoMode,

    #if (MONO_COLOR_CLOCK != 1)

        /**
         * @brief Represents the "hue fading" mode
         *
         * This represents the "hue fading" mode, which change the hue of the
         * color automatically by iterating over it step by step.
         *
         * @see AutoHueState_enter()
         */
        MS_hueMode,

    #endif

    /**
     * @brief Represents the "pulse" mode
     *
     * This represents the "pulse" mode, which will constantly change the
     * brightness in a "pulsing" way.
     *
     * Note that this is implemented different internally than the other modes
     * (menu_state_t::MS_normalMode and menu_state_t::MS_hueMode) as it can
     * be activated on top of these other modes, which makes it possible to
     * pulse within both of these modes.
     *
     * @see PulseState_init()
     */
    MS_pulse,

    /**
     * @brief Represents the "set system time" mode
     *
     * This mode is used to set the system time manually.
     *
     * @see SetSystemTimeState_enter()
     */
    MS_setSystemTime,

    /**
     * @brief Represents the "set on/off time(s)" (autoOff) mode
     *
     * This mode is used to setup the on/off time(s) during which the clock
     * will disable itself. This is also being referred to as autoOff feature.
     * Depending upon the value of UI_MAX_ONOFF_TIMES there might be a couple
     * of on/off pairs.
     *
     * @see SetOnOffTimeState_enter()
     * @see UI_MAX_ONOFF_TIMES
     */
    MS_setOnOffTime,

    /**
     * @brief Represents the "enter time" mode
     *
     * This mode is used to enter times. It can actually be reached from two
     * other modes(menu_state_t::MS_setSystemTime and
     * menu_state_t::MS_setOnOffTime), which both need the possibility to enter
     * times.
     *
     * @see EnterTimeState_enter()
     */
    MS_enterTime,

    /**
     * @brief Represents the "show number" mode
     *
     * This mode is used to display a single number.
     *
     * @see ShowNumberState_enter()
     */
    MS_showNumber,

    /**
     * @brief Simple dummy representing number of items
     *
     * This will represent the amount of items within this enumeration.
     */
    MS_COUNT

} menu_state_t;

#ifndef ENABLE_INDIVIDUAL_CONFIG

    /**
     * @copydoc main.h::ENABLE_INDIVIDUAL_CONFIG
     */
    #define ENABLE_INDIVIDUAL_CONFIG 1

#endif

#ifndef USER_AUTOSAVE

    /**
     * @copydoc main.h::USER_AUTOSAVE
     */
    #define USER_AUTOSAVE 1

#endif

/**
 * @brief Time without receiving an IR command before data is written to EEPROM
 *
 * This defines the time interval in seconds which has to elapse without
 * receiving an IR command before changed data is written back to EEPROM.
 *
 * @see USER_AUTOSAVE
 * @see user_isr1Hz()
 */
#define USER_DELAY_BEFORE_SAVE_EEPROM_S 120

/**
 * @brief Time without receiving IR command before checking whether autoOff
 * is active
 *
 * This defines the interval in seconds after the last received IR command
 * before checking whether the display should be disabled due to the on/off
 * time(s) defined by the user. This makes sure that the display isn't simply
 * turned off while the user is still interacting with the clock itself.
 *
 * @see user_isr1Hz()
 */
#define USER_DELAY_CHECK_IF_AUTO_OFF_REACHED_S 10

/**
 * @brief Default address of the IR remote control
 *
 * This is the default address of the IR remote control, which the Wordclock
 * will react to until otherwise "trained" (menu_state_t::MS_irTrain). The
 * defined default value corresponds to the remote control "MAGIC LIGHTNING
 * REMOTE CONTROLLER" purchasable on DX.com. This kind of form factor is
 * quite common for LED stripes and there are labels to stick on top of the
 * remote control itself available, see [1].
 *
 * [1]: https://www.mikrocontroller.net/articles/Word_Clock_Variante_1#IR
 *
 * @see menu_state_t::MS_irTrain
 */
#define USER_ADDRESS_DEFAULT 0xFF00

/**
 * @brief Default command code for the "On" and/or "Off" command
 *
 * This is the default command code of the IR remote control for this command,
 * which the Wordclock will react to until otherwise "trained"
 * (menu_state_t::MS_irTrain).
 *
 * The defined default value corresponds to the "ON" button.
 *
 * @see user_command_t::UC_ONOFF
 */
#define USER_CMD_DEF_ONOFF 0x0007

/**
 * @brief Default command code for the "increase brightness" command
 *
 * This is the default command code of the IR remote control for this command,
 * which the Wordclock will react to until otherwise "trained"
 * (menu_state_t::MS_irTrain).
 *
 * The defined default value corresponds to the first button from the left in
 * the fourth row from the top.
 *
 * @see user_command_t::UC_BRIGHTNESS_UP
 */
#define USER_CMD_DEF_BRIGHTNESS_UP 0x0015

/**
 * @brief Default command code for the "decrease brightness" command
 *
 * This is the default command code of the IR remote control for this command,
 * which the Wordclock will react to until otherwise "trained"
 * (menu_state_t::MS_irTrain).
 *
 * The defined default value corresponds to the first button from the left in
 * the fifth row from the top.
 *
 * @see user_command_t::UC_BRIGHTNESS_DOWN
 */
#define USER_CMD_DEF_BRIGHTNESS_DOWN 0x0019

/**
 * @brief Default command code for the "up" command
 *
 * This is the default command code of the IR remote control for this command,
 * which the Wordclock will react to until otherwise "trained"
 * (menu_state_t::MS_irTrain).
 *
 * The defined default value corresponds to the "STROBE" button.
 *
 * @see user_command_t::UC_UP
 */
#define USER_CMD_DEF_UP 0x0017

/**
 * @brief Default command code for the "down" command
 *
 * This is the default command code of the IR remote control for this command,
 * which the Wordclock will react to until otherwise "trained"
 * (menu_state_t::MS_irTrain).
 *
 * The defined default value corresponds to the "FADE" button.
 *
 * @see user_command_t::UC_DOWN
 */
#define USER_CMD_DEF_DOWN 0x001B

/**
 * @brief Default command code for the "set time" command
 *
 * This is the default command code of the IR remote control for this command,
 * which the Wordclock will react to until otherwise "trained"
 * (menu_state_t::MS_irTrain).
 *
 * The defined default value corresponds to the third button from the left in
 * the fifth row from the top.
 *
 * @see user_command_t::UC_SET_TIME
 */
#define USER_CMD_DEF_SET_TIME 0x001A

/**
 * @brief Default command code for the "setup on/off times" command
 *
 * This is the default command code of the IR remote control for this command,
 * which the Wordclock will react to until otherwise "trained"
 * (menu_state_t::MS_irTrain).
 *
 * The defined default value corresponds to the third button from the left in
 * the fourth row from the top.
 *
 * @see user_command_t::UC_SET_ONOFF_TIMES
 */
#define USER_CMD_DEF_SET_ONOFF_TIMES 0x0016

/**
 * @brief Default command code for the "force DCF77 update" command
 *
 * This is the default command code of the IR remote control for this command,
 * which the Wordclock will react to until otherwise "trained"
 * (menu_state_t::MS_irTrain).
 *
 * The defined default value corresponds to the second button from the left in
 * the fifth row from the top.
 *
 * @see user_command_t::UC_DCF_GET_TIME
 */
#define USER_CMD_DEF_DCF_GET_TIME 0x0018

/**
 * @brief Default command code for the "normal mode" command
 *
 * This is the default command code of the IR remote control for this command,
 * which the Wordclock will react to until otherwise "trained"
 * (menu_state_t::MS_irTrain).
 *
 * The defined default value corresponds to the "R" button.
 *
 * @see user_command_t::UC_NORMAL_MODE
 */
#define USER_CMD_DEF_NORMAL_MODE 0x0009

/**
 * @brief Default command code for the "pulse mode" command
 *
 * This is the default command code of the IR remote control for this command,
 * which the Wordclock will react to until otherwise "trained"
 * (menu_state_t::MS_irTrain).
 *
 * The defined default value corresponds to the "G" button.
 *
 * @see user_command_t::UC_PULSE_MODE
 */
#define USER_CMD_DEF_PULSE_MODE 0x000A

/**
 * @brief Default command code for the "demo mode" command
 *
 * This is the default command code of the IR remote control for this command,
 * which the Wordclock will react to until otherwise "trained"
 * (menu_state_t::MS_irTrain).
 *
 * The defined default value corresponds to the "B" button.
 *
 * @see user_command_t::UC_DEMO_MODE
 */
#define USER_CMD_DEF_DEMO_MODE 0x000B

/**
 * @brief Default command code for the "hue fading mode" command
 *
 * This is the default command code of the IR remote control for this command,
 * which the Wordclock will react to until otherwise "trained"
 * (menu_state_t::MS_irTrain).
 *
 * The defined default value corresponds to the "W" button.
 *
 * @see user_command_t::UC_HUE_MODE
 */
#define USER_CMD_DEF_HUE_MODE 0x0008

/**
 * @brief Default command code for the "change red channel" command
 *
 * This is the default command code of the IR remote control for this command,
 * which the Wordclock will react to until otherwise "trained"
 * (menu_state_t::MS_irTrain).
 *
 * The defined default value corresponds to the first button from the left in
 * the third row from the top.
 *
 * @see user_command_t::UC_CHANGE_R
 */
#define USER_CMD_DEF_CHANGE_R 0x000D

/**
 * @brief Default command code for the "change green channel" command
 *
 * This is the default command code of the IR remote control for this command,
 * which the Wordclock will react to until otherwise "trained"
 * (menu_state_t::MS_irTrain).
 *
 * The defined default value corresponds to the second button from the left in
 * the third row from the top.
 *
 * @see user_command_t::UC_CHANGE_G
 */
#define USER_CMD_DEF_CHANGE_G 0x000C

/**
 * @brief Default command code for the "change blue channel" command
 *
 * This is the default command code of the IR remote control for this command,
 * which the Wordclock will react to until otherwise "trained"
 * (menu_state_t::MS_irTrain).
 *
 * The defined default value corresponds to the third button from the left in
 * the third row from the top.
 *
 * @see user_command_t::UC_CHANGE_B
 */
#define USER_CMD_DEF_CHANGE_B 0x000E

/**
 * @brief Default command code for the "change hue" command
 *
 * This is the default command code of the IR remote control for this command,
 * which the Wordclock will react to until otherwise "trained"
 * (menu_state_t::MS_irTrain).
 *
 * The defined default value corresponds to the "FLASH" button.
 *
 * @see user_command_t::UC_CHANGE_HUE
 */
#define USER_CMD_DEF_CHANGE_HUE 0x000F

/**
 * @brief Default command code for the "brightness calibration" command
 *
 * This is the default command code of the IR remote control for this command,
 * which the Wordclock will react to until otherwise "trained"
 * (menu_state_t::MS_irTrain).
 *
 * The defined default value corresponds to the first button from the left in
 * the sixth row from the top.
 *
 * @see user_command_t::UC_CALIB_BRIGHTNESS
 */
#define USER_CMD_DEF_CALIB_BRIGHTNESS 0x0011

/**
 * @brief Default command code for the "Ambilight on/off" command
 *
 * This is the default command code of the IR remote control for this command,
 * which the Wordclock will react to until otherwise "trained"
 * (menu_state_t::MS_irTrain).
 *
 * The defined default value corresponds to the second button from the left in
 * the sixth row from the top.
 *
 * @see user_command_t::UC_AMBILIGHT
 */
#define USER_CMD_DEF_AMBILIGHT 0x0010

/**
 * @brief Default command code for the "Bluetooth on/off" command
 *
 * This is the default command code of the IR remote control for this command,
 * which the Wordclock will react to until otherwise "trained"
 * (menu_state_t::MS_irTrain).
 *
 * The defined default value corresponds to the third button from the left in
 * the sixth row from the top.
 *
 * @see user_command_t::UC_BLUETOOTH
 */
#define USER_CMD_DEF_BLUETOOTH 0x0012

/**
 * @brief Default command code for the "auxiliary GPO on/off" command
 *
 * This is the default command code of the IR remote control for this command,
 * which the Wordclock will react to until otherwise "trained"
 * (menu_state_t::MS_irTrain).
 *
 * The defined default value corresponds to the third button from the left in
 * the sixth row from the top.
 *
 * @see user_command_t::UC_AUXPOWER
 */
#define USER_CMD_DEF_AUXPOWER 0x0013

/**
 * @brief Simplified time structure containing only the hours and minutes
 *
 * This is a more simplistic time structure (compared to datetime_t), which
 * only holds the hours and the minutes. This is useful when setting up times
 * (e.g. autoOff) as this times refer to a daily basis so other entities such
 * as the date is not needed.
 *
 * @see UserEepromParams::onOffTimes
 */
typedef struct UiTime {

    /**
     * @brief Hours
     *
     * Ranges from 0 to 23.
     */
    uint8_t h;

    /**
     * @brief Minutes
     *
     * Ranges from 0 to 59.
     */
    uint8_t m;

} UiTime;

/**
 * @brief Amount of supported on/off (autoOff) time(s)
 *
 * This defines the maximum amount of usable on/off (autoOff) times. The
 * default value is 1, which allows the user to set it up in a way that the
 * display will disable itself for a specific period of time, e.g. at the
 * night. You can increase this number, however this goes along with an
 * increased consumption of EEPROM and will make the interaction with the user
 * more "complex".
 *
 * @note If changing this number you probably want also to change the
 * default values, see USER_ON_OFF_TIME_DEFAULTS.
 *
 * @see UserEepromParams::onOffTimes
 * @see UI_ONOFFTIMES_COUNT
 * @see USER_ON_OFF_TIME_DEFAULTS
 */
#define UI_MAX_ONOFF_TIMES 1

/**
 * @brief Amount of needed elements to store the on/off (autoOff) times
 *
 * This defines the needed amount of elements within
 * UserEepromParams::onOffTimes to store the configured amount of on/off
 * (autoOff) times (UI_MAX_ONOFF_TIMES).
 *
 * Each on/off (autoOff) time consists of two parts: The "on" time as well as
 * the "off" time. Therefore the value for this macro is simply calculated by
 * multiplying UI_MAX_ONOFF_TIMES by two.
 *
 * @see UserEepromParams::onOffTimes
 * @see UI_MAX_ONOFF_TIMES
 */
#define UI_ONOFFTIMES_COUNT (UI_MAX_ONOFF_TIMES * 2)

/**
 * @brief Default value for an entry within USER_ON_OFF_TIME_DEFAULTS
 *
 * This defines the default value for a single entry within
 * USER_ON_OFF_TIME_DEFAULTS. It defaults to "0" for the hours and "0" for the
 * minutes.
 *
 * @see UiTime
 * @see USER_ON_OFF_TIME_DEFAULTS
 */
#define USER_DEFAULT_TIME {0, 0}

/**
 * @brief Default values for the on/off (autoOff) time(s)
 *
 * This defines the default values for the on/off (autoOff) time(s). Each entry
 * equals to USER_DEFAULT_TIME.
 *
 * @note Keep in mind that the amount of items within this macro should be
 * equal to UI_ONOFFTIMES_COUNT whenever changing UI_MAX_ONOFF_TIMES.
 *
 * @see UI_MAX_ONOFF_TIMES
 * @see UI_ONOFFTIMES_COUNT
 * @see USER_DEFAULT_TIME
 */
#define USER_ON_OFF_TIME_DEFAULTS \
{ \
    USER_DEFAULT_TIME, \
    USER_DEFAULT_TIME \
}

/**
 * @brief Defines the amount of supported color presets
 *
 * This defines the maximum amount of supported color presets. The default
 * value is 4, which allows the user to setup and choose from four different
 * presets. The defaults for this presets are defined within
 * USER_COLORPRESETS_DEFAULTS
 *
 * @note If changing this number you probably want also to change the
 * default values, see USER_COLORPRESETS_DEFAULTS.
 *
 * @see USER_COLORPRESETS_DEFAULTS
 */
#define UI_COLOR_PRESET_COUNT 4

/**
 * @brief Default values for color presets
 *
 * This defines the default values for the color presets times. Each entry
 * expects three parameters for the colors "red", "green" and "blue". The
 * values can range from 0 to MAX_PWM_STEPS - 1.
 *
 * The default is to assign a preset for the colors "red" (1), "green" (2) and
 * "blue" (3). On top of that there is a "white" (4) preset.
 *
 * @note Keep in mind that the amount of items within this macro should be
 * equal to UI_COLOR_PRESET_COUNT whenever changing UI_COLOR_PRESET_COUNT.
 *
 * @see UI_COLOR_PRESET_COUNT
 * @see MAX_PWM_STEPS
 */
#define USER_COLORPRESETS_DEFAULTS \
{ \
    {MAX_PWM_STEPS - 1, 0, 0}, \
    {0, MAX_PWM_STEPS - 1, 0}, \
    {0, 0, MAX_PWM_STEPS - 1}, \
    {MAX_PWM_STEPS - 1, MAX_PWM_STEPS - 1, MAX_PWM_STEPS - 1 }, \
}

/**
 * @brief Data of the user module that should be stored persistently in EEPROM
 *
 * The data from this module to be stored persistently in the EEPROM is
 * expected to be in the form of this type.
 *
 * The default values are defined in USEREEPROMPARAMS_DEFAULT.
 *
 * @see USEREEPROMPARAMS_DEFAULT
 * @see WcEepromData::userParams
 */
typedef struct UserEepromParams {

    /**
     * @brief The address of the trained IR remote control
     */
    uint16_t irAddress;

    /**
     * @brief The trained command codes
     *
     * For details about the specific allocation of the indexes refer to
     * user_command_t.
     *
     * @see user_command_t
     */
    uint16_t irCommandCodes[UC_COMMAND_COUNT];

    /**
     * @brief Struct to hold the RGB values for the color presets
     *
     * This holds the value for the "red", "green" and "blue" channels for
     * each color preset.
     *
     * @see UI_COLOR_PRESET_COUNT
     */
    struct rgb {

        /**
         * @brief The value for the "red" channel
         */
        uint8_t r;

        /**
         * @brief The value for the "green" channel
         */
        uint8_t g;

        /**
         * @brief The value for the "blue" channel
         */
        uint8_t b;

    } colorPresets[UI_COLOR_PRESET_COUNT];

    /**
     * @brief Holds the color preset currently selected by the user
     *
     * @see UI_COLOR_PRESET_COUNT
     */
    uint8_t curColorProfile;

    /**
     * @brief Stores the on/off (autoOff) time(s)
     *
     * This stores the on/off (autoOff) time(s). There are up to
     * UI_MAX_ONOFF_TIMES on/off (autoOff) times supported. Each "autoOff" time
     * is actually a pair of two consecutive times ("on" and/or "off"). If the
     * "on" and/or "off" time are equal to each other, this slot is practically
     * disabled. If the times between two different pairs of on/off (autoOff)
     * times overlap the behavior is undefined and will probably lead to
     * misbehavior.
     *
     * @see UI_MAX_ONOFF_TIMES
     * @see UI_ONOFFTIMES_COUNT
     */
    UiTime onOffTimes[UI_ONOFFTIMES_COUNT];

    /**
     * @brief Stores whether an animation should be displayed during autoOff
     *
     * When the display is deactivated due to the autoOff feature an animation
     * can be displayed to give a visual indication about the state to the
     * user.
     *
     * @see display_autoOffAnimStep1Hz()
     */
    bool useAutoOffAnimation;

    /**
     * @brief Interval between two animation steps in pulse mode
     *
     * The speed of the pulsing in the pulse mode (menu_state_t::MS_pulse) can
     * be changed by the user (within the boundaries defined by
     * USER_PULSE_CHANGE_INT_10MS_MIN and USER_PULSE_CHANGE_INT_10MS_MAX).
     *
     * This represents this setting within the EEPROM.
     *
     * @see menu_state_t::MS_pulse
     * @see PulseState_handleUserCommand()
     * @see PulseState_100Hz()
     * @see USER_PULSE_CHANGE_INT_10MS_MIN
     * @see USER_PULSE_CHANGE_INT_10MS_MAX
     */
    uint8_t pulseUpdateInterval;

    /**
     * @brief Interval between two animation steps in hue fading mode
     *
     * The speed of the fading in the hue fading mode
     * (menu_state_t::MS_hueMode) can be changed by the user (within the
     * boundaries defined by USER_HUE_CHANGE_INT_100MS_MIN and
     * USER_HUE_CHANGE_INT_100MS_MAX).
     *
     * This represents this setting within the EEPROM.
     *
     * @see menu_state_t::MS_hueMode
     * @see AutoHueState_handleUserCommand()
     * @see AutoHueState_10Hz()
     * @see USER_HUE_CHANGE_INT_100MS_MIN
     * @see USER_HUE_CHANGE_INT_100MS_MAX
     */
    uint8_t hueChangeInterval;

    /**
     * @brief Holds the mode currently selected by the user
     *
     * This is the mode the user has selected and should be entered when
     * the Wordclock is started.
     *
     * @see menu_state_t
     */
    uint8_t mode;

} UserEepromParams;

extern bool useAutoOffAnimation;

/**
 * @brief Delay before another key press will be recognized
 *
 * The defines the delay (in multiples of 100 ms) after a key press before
 * another key press will be recognized
 *
 * The default values are defined in USEREEPROMPARAMS_DEFAULT.
 *
 * @see USEREEPROMPARAMS_DEFAULT
 */
#define USER_KEY_PRESS_DELAY_100MS 3

/**
 * @brief Default value for interval between two animation steps in hue fading
 * mode
 *
 * This defines the default value for the interval (in multiples of 100 ms)
 * between two animations steps within the hue fading mode. It then can be
 * changed by the user (within the boundaries defined in
 * USER_HUE_CHANGE_INT_100MS_MIN and USER_HUE_CHANGE_INT_100MS_MAX).
 *
 * @see menu_state_t::MS_hueMode
 * @see UserEepromParams::hueChangeInterval
 * @see USER_HUE_CHANGE_INT_100MS_MIN
 * @see USER_HUE_CHANGE_INT_100MS_MAX
 */
#define USER_HUE_CHANGE_INT_100MS 1

/**
 * @brief Minimum value for interval between two animation steps in hue fading
 * mode
 *
 * This defines the minimum value for the interval (in multiples of 100 ms)
 * between two animations steps within the hue fading mode. The user can change
 * the default value defined in USER_HUE_CHANGE_INT_100MS, however not below
 * the value defined here.
 *
 * @see menu_state_t::MS_hueMode
 * @see UserEepromParams::hueChangeInterval
 * @see USER_HUE_CHANGE_INT_100MS
 * @see USER_HUE_CHANGE_INT_100MS_MAX
 */
#define USER_HUE_CHANGE_INT_100MS_MIN 1

/**
 * @brief Maximum value for interval between two animation steps in hue fading
 * mode
 *
 * This defines the maximum value for the interval (in multiples of 100 ms)
 * between two animations steps within the hue fading mode. The user can change
 * the default value defined in USER_HUE_CHANGE_INT_100MS, however not above
 * the value defined here.
 *
 * @see menu_state_t::MS_hueMode
 * @see UserEepromParams::hueChangeInterval
 * @see USER_HUE_CHANGE_INT_100MS
 * @see USER_HUE_CHANGE_INT_100MS_MIN
 */
#define USER_HUE_CHANGE_INT_100MS_MAX 8

/**
 * @brief Amount of steps to change when requested manually
 *
 * Whenever a hue change is requested ("up" and/or "down") manually by the user
 * this defines the amount of steps to change the hue by. It basically is a
 * tradeoff between granular control and an actual visible change to the color.
 * Hues are numerical values ranging from 0 up to COLOR_HUE_MAX.
 *
 * @see COLOR_HUE_MAX
 * @see NormalState_handleUserCommand()
 */
#define USER_HUE_CHANGE_MANUAL_STEPS 10

/**
 * @brief Default value for interval between two animation steps in pulse mode
 *
 * This defines the default value for the interval (in multiples of 10 ms)
 * between two animations steps within the pulse mode. It then can be changed
 * by the user (within the boundaries defined in
 * USER_PULSE_CHANGE_INT_10MS_MIN and USER_PULSE_CHANGE_INT_10MS_MAX).
 *
 * @see menu_state_t::MS_pulse
 * @see UserEepromParams::pulseUpdateInterval
 * @see USER_PULSE_CHANGE_INT_10MS_MIN
 * @see USER_PULSE_CHANGE_INT_10MS_MAX
 */
#define USER_PULSE_CHANGE_INT_10MS 1

/**
 * @brief Minimum value for interval between two animation steps in pulse mode
 *
 * This defines the minimum value for the interval (in multiples of 10 ms)
 * between two animations steps within the pulse mode. The user can change
 * the default value defined in USER_PULSE_CHANGE_INT_10MS, however not below
 * the value defined here.
 *
 * @see menu_state_t::MS_pulse
 * @see UserEepromParams::hueChangeInterval
 * @see USER_PULSE_CHANGE_INT_10MS
 * @see USER_PULSE_CHANGE_INT_10MS_MAX
 */
#define USER_PULSE_CHANGE_INT_10MS_MIN 1

/**
 * @brief Maximum value for interval between two animation steps in pulse mode
 *
 * This defines the maximum value for the interval (in multiples of 10 ms)
 * between two animations steps within the pulse mode. The user can change
 * the default value defined in USER_PULSE_CHANGE_INT_10MS, however not below
 * the value defined here.
 *
 * @see menu_state_t::MS_pulse
 * @see UserEepromParams::hueChangeInterval
 * @see USER_PULSE_CHANGE_INT_10MS
 * @see USER_PULSE_CHANGE_INT_10MS_MIN
 */
#define USER_PULSE_CHANGE_INT_10MS_MAX 5

/**
 * @brief Delay between two steps within the demo mode
 *
 * This defines the delay (in multiples of 100 ms)  between two steps within
 * the demo mode.
 *
 * @see menu_state_t::MS_demoMode
 */
#define USER_DEMO_CHANGE_INT_100MS 5

/**
 * @brief Interval a number should be displayed
 *
 * This defines the interval (in multiples of 100 ms) how long the Wordclock
 * should reside the "showNumber" mode, effectively controlling how long to
 * display a number. This for instance affects the displaying of the currently
 * chosen color preset.
 *
 * @see menu_state_t::MS_showNumber
 * @see display_getNumberDisplayState()
 */
#define USER_NORMAL_SHOW_NUMBER_DELAY_100MS 10

/**
 * @brief Interval to wait for an IR command to enter the training mode
 *
 * This defines the interval (in seconds) how long during the startup the
 * Wordclock should accept arbitrary IR commands to enter the training mode.
 *
 * @see menu_state_t::MS_irTrain
 */
#define USER_STARTUP_WAIT_IR_TRAIN_S 7

/**
 * @brief Hour at which the brightness should change when inputting times
 *
 * The display itself can only output times between one and twelve o'clock
 * and has no way of indicating whether it is actually referring to "AM"
 * and/or "PM". Therefore the brightness is used as an indicator. This
 * defines the hour at which to switch between the brightness defined for
 * the day (USER_ENTERTIME_DAY_BRIGHTNESS) and the one for the night
 * (USER_ENTERTIME_NIGHT_BRIGHTNESS).
 *
 * Valid values for this range from 0 to 11.
 *
 * @see USER_ENTERTIME_DAY_BRIGHTNESS
 * @see USER_ENTERTIME_NIGHT_BRIGHTNESS
 */
#define USER_ENTERTIME_DAY_NIGHT_CHANGE_HOUR 8

/**
 * @brief Brightness if inputting time and referring to hours during the day
 *
 * The defines the brightness, which should be used when entering times and
 * referring to hours during the day. The distinction between "day" and "night"
 * can be controlled by USER_ENTERTIME_DAY_NIGHT_CHANGE_HOUR. Refer to
 * USER_ENTERTIME_DAY_NIGHT_CHANGE_HOUR for details.
 *
 * @see USER_ENTERTIME_DAY_NIGHT_CHANGE_HOUR
 * @see USER_ENTERTIME_NIGHT_BRIGHTNESS
 */
#define USER_ENTERTIME_DAY_BRIGHTNESS 255

/**
 * @brief Brightness if inputting time and referring to hours during the night
 *
 * The defines the brightness, which should be used when entering times and
 * referring to hours during the night. The distinction between "day" and
 * "night" can be controlled by USER_ENTERTIME_DAY_NIGHT_CHANGE_HOUR. Refer to
 * USER_ENTERTIME_DAY_NIGHT_CHANGE_HOUR for details.
 *
 * @see USER_ENTERTIME_DAY_NIGHT_CHANGE_HOUR
 * @see USER_ENTERTIME_DAY_BRIGHTNESS
 */
#define USER_ENTERTIME_NIGHT_BRIGHTNESS 50

/**
 * @brief Step width when entering autoOff times
 *
 * The defines the step width which will be used when entering autoOff times.
 * It is sufficient to deal with quarters of an hour (rather than the
 * specific minutes) when referring to the autoOff times, so this will save
 * the user some serious amount of key presses.
 */
#define USER_ENTER_ONOFF_TIME_STEP 15

extern void handle_user_command(user_command_t user_command);

extern void handle_ir_code();

extern void user_setNewTime(const datetime_t* i_time);

extern void user_isr1000Hz();

extern void user_isr100Hz();

extern void user_isr10Hz();

extern void user_isr1Hz();

extern void user_init();

/**
 * @brief Creates a handler for the given command with the given code
 *
 * The helper macro will generate a simple handler for a received and/or
 * decoded user command (user_command_t). It is used within the language
 * specific display files to implement commands specific to the language.
 *
 * @see DISPLAY_SPECIAL_USER_COMMANDS_HANDLER
 */
#define USER_CREATE_IR_HANDLER(rcvd_user_command, code) \
    } else if (rcvd_user_command == user_command) { \
        code \
        user_setNewTime(0);

#if (ENABLE_INDIVIDUAL_CONFIG == 0 || ENABLE_DCF_SUPPORT == 1)

    /**
     * @see USER_CMD_DEF_DCF_GET_TIME
     */
    #define USER_CMD_DEFAULT_DCF USER_CMD_DEF_DCF_GET_TIME,

#else

    /**
     * @brief Dummy in case of an individual config and ENABLE_DCF_SUPPORT == 0
     */
    #define USER_CMD_DEFAULT_DCF

#endif

#if (ENABLE_INDIVIDUAL_CONFIG == 0 || MONO_COLOR_CLOCK != 1)

    /**
     * @see USER_CMD_DEF_HUE_MODE
     * @see USER_CMD_DEF_CHANGE_R
     * @see USER_CMD_DEF_CHANGE_G
     * @see USER_CMD_DEF_CHANGE_B
     * @see USER_CMD_DEF_CHANGE_HUE
     */
    #define USER_CMD_DEFAULT_MULTICOLOR \
        USER_CMD_DEF_HUE_MODE, \
        USER_CMD_DEF_CHANGE_R, \
        USER_CMD_DEF_CHANGE_G, \
        USER_CMD_DEF_CHANGE_B, \
        USER_CMD_DEF_CHANGE_HUE,

#else

    /**
     * @brief Dummy in case of an individual config and MONO_COLOR_CLOCK == 1
     */
    #define USER_CMD_DEFAULT_MULTICOLOR

#endif

#if (ENABLE_INDIVIDUAL_CONFIG == 0 || ENABLE_AMBILIGHT_SUPPORT == 1)

    /**
     * @see USER_CMD_DEF_AMBILIGHT
     */
    #define USER_CMD_DEFAULT_AMBILIGHT USER_CMD_DEF_AMBILIGHT,

#else

    /**
     * @brief Dummy in case of an individual config and ENABLE_AMBILIGHT_SUPPORT == 0
     */
    #define USER_CMD_DEFAULT_AMBILIGHT

#endif

#if (ENABLE_INDIVIDUAL_CONFIG == 0 || ENABLE_BLUETOOTH_SUPPORT == 1)

    /**
     * @see USER_CMD_DEF_BLUETOOTH
     */
    #define USER_CMD_DEFAULT_BLUETOOTH USER_CMD_DEF_BLUETOOTH,

#else

    /**
     * @brief Dummy in case of an individual config and ENABLE_BLUETOOTH_SUPPORT == 0
     */
    #define USER_CMD_DEFAULT_BLUETOOTH

#endif

#if (ENABLE_INDIVIDUAL_CONFIG == 0 || ENABLE_AUXPOWER_SUPPORT == 1)

    /**
     * @see USER_CMD_DEF_BLUETOOTH
     */
    #define USER_CMD_DEFAULT_AUXPOWER USER_CMD_DEF_AUXPOWER,

#else

    /**
     * @brief Dummy in case of an individual config and ENABLE_AUXPOWER_SUPPORT == 0
     */
    #define USER_CMD_DEFAULT_AUXPOWER

#endif

/**
 * @brief Default values for the command codes
 *
 * This defines the default values for the command codes defined in
 * user_command_t.
 *
 * @see user_command_t
 */
#define USER_COMMANDCODES_DEFAULTS { \
\
    USER_CMD_DEF_ONOFF, \
    USER_CMD_DEF_BRIGHTNESS_UP, \
    USER_CMD_DEF_BRIGHTNESS_DOWN, \
\
    USER_CMD_DEF_UP, \
    USER_CMD_DEF_DOWN, \
\
    USER_CMD_DEF_SET_TIME, \
    USER_CMD_DEF_SET_ONOFF_TIMES, \
\
    USER_CMD_DEFAULT_DCF \
\
    USER_CMD_DEF_NORMAL_MODE, \
    USER_CMD_DEF_PULSE_MODE, \
    USER_CMD_DEF_DEMO_MODE, \
\
    USER_CMD_DEFAULT_MULTICOLOR \
\
    USER_CMD_DEF_CALIB_BRIGHTNESS, \
\
    USER_CMD_DEFAULT_AMBILIGHT \
    USER_CMD_DEFAULT_BLUETOOTH \
    USER_CMD_DEFAULT_AUXPOWER \
\
    DISPLAY_SPECIAL_USER_COMMANDS_CODES \
\
}

/**
 * @brief Default values of this module that should be stored persistently
 *
 * This defines the default values for this module. Refer to UserEepromParams
 * for a detailed description of each member.
 *
 * @note This will also be the values used after flashing the firmware to the
 * microcontroller, so make sure that the defaults are actually useful.
 *
 * @see UserEepromParams
 * @see wceeprom.h
 */
#define USEREEPROMPARAMS_DEFAULT { \
\
    USER_ADDRESS_DEFAULT, \
    USER_COMMANDCODES_DEFAULTS, \
    USER_COLORPRESETS_DEFAULTS, \
    0, \
    USER_ON_OFF_TIME_DEFAULTS, \
    1, \
    USER_PULSE_CHANGE_INT_10MS, \
    USER_HUE_CHANGE_INT_100MS, \
    MS_normalMode \
\
}

#endif /* _WC_USER_H_ */
