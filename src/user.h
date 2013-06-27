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

#include "main.h"
#include "display.h"

typedef enum e_MenuStates
{

    MS_irTrain = 0,
    MS_normalMode,
    MS_demoMode,

    #if (MONO_COLOR_CLOCK != 1)

        MS_hueMode,

    #endif

    MS_pulse,

    MS_setSystemTime,
    MS_setOnOffTime,
    MS_enterTime,

    MS_showNumber,

    MS_COUNT

} e_MenuStates;

#ifndef INDIVIDUAL_CONFIG

    #define INDIVIDUAL_CONFIG 1

#endif

#ifndef USER_AUTOSAVE

    #define USER_AUTOSAVE 1

#endif

#define USER_DELAY_BEFORE_SAVE_EEPROM_s 120

#define USER_DELAY_CHECK_IF_AUTO_OFF_REACHED_s 10

typedef enum e_userCommands
{

    UI_ONOFF = 0,
    UI_BRIGHTNESS_UP,
    UI_BRIGHTNESS_DOWN,

    UI_UP,
    UI_DOWN,

    UI_SET_TIME,
    UI_SET_ONOFF_TIMES,

    #if (INDIVIDUAL_CONFIG == 0 || DCF_PRESENT == 1)

        UI_DCF_GET_TIME,

    #endif

    UI_NORMAL_MODE,
    UI_PULSE_MODE,

    UI_DEMO_MODE,

    #if (INDIVIDUAL_CONFIG == 0 || MONO_COLOR_CLOCK != 1)

        UI_HUE_MODE,
        UI_CHANGE_R,
        UI_CHANGE_G,
        UI_CHANGE_B,
        UI_CHANGE_HUE,

    #endif

    UI_CALIB_BRIGHTNESS,

    #if (INDIVIDUAL_CONFIG == 0 || AMBILIGHT_PRESENT == 1)

        UI_AMBIENT_LIGHT,

    #endif

    #if (INDIVIDUAL_CONFIG == 0 || BLUETOOTH_PRESENT == 1)

        UI_BLUETOOTH,

    #endif

    #if (INDIVIDUAL_CONFIG == 0 || AUXPOWER_PRESENT == 1)

        UI_AUXPOWER,

    #endif

    DISPLAY_SPECIAL_USER_COMMANDS

    UI_COMMAND_COUNT

} e_userCommands;

#define USER_ADDRESS_DEFAULT 0x7B80
#define USER_CMD_DEF_ONOFF 0x0013
#define USER_CMD_DEF_BRIGHTNESS_UP 0x0018
#define USER_CMD_DEF_BRIGHTNESS_DOWN 0x0019
#define USER_CMD_DEF_UP 0x000A
#define USER_CMD_DEF_DOWN 0x000B
#define USER_CMD_DEF_SET_TIME 0x0009
#define USER_CMD_DEF_SET_ONOFF_TIMES 0x0015
#define USER_CMD_DEF_DCF_GET_TIME 0x0000
#define USER_CMD_DEF_NORMAL_MODE 0x0001
#define USER_CMD_DEF_PULSE_MODE 0x001e
#define USER_CMD_DEF_DEMO_MODE 0x0003
#define USER_CMD_DEF_HUE_MODE 0x0002
#define USER_CMD_DEF_CHANGE_R 0x0004
#define USER_CMD_DEF_CHANGE_G 0x0005
#define USER_CMD_DEF_CHANGE_B 0x0006
#define USER_CMD_DEF_CHANGE_HUE 0x0007
#define USER_CMD_DEF_CALIB_BRIGHTNESS 0x0045
#define USER_CMD_DEF_AMBILIGHT 0x0052
#define USER_CMD_DEF_BLUETOOTH 0x0013
#define USER_CMD_DEF_AUXPOWER 0x0013

typedef struct UiTime {

    uint8_t h;
    uint8_t m;

} UiTime;

#define UI_MAX_ONOFF_TIMES 1

#define UI_AUTOOFFTIMES_COUNT (UI_MAX_ONOFF_TIMES * 2)

#define USER_DEFAULT_TIME {0, 0}

#define USER_ON_OFF_TIME_DEFAULTS \
{ \
    USER_DEFAULT_TIME, \
    USER_DEFAULT_TIME \
}

#define UI_COLOR_PRESET_COUNT 4

#define USER_COLORPRESETS_DEFAULTS \
{ \
    {MAX_PWM_STEPS - 1, 0, 0}, \
    {0, MAX_PWM_STEPS - 1, 0}, \
    {0, 0, MAX_PWM_STEPS - 1}, \
    {MAX_PWM_STEPS - 1, MAX_PWM_STEPS - 1, MAX_PWM_STEPS - 1 }, \
}

typedef struct UserEepromParams {

    uint16_t irAddress;

    uint16_t irCommandCodes[UI_COMMAND_COUNT];

    struct rgb {

        uint8_t r;

        uint8_t g;

        uint8_t b;

    } colorPresets[UI_COLOR_PRESET_COUNT];

    uint8_t curColorProfile;

    UiTime autoOffTimes[UI_AUTOOFFTIMES_COUNT];

    uint8_t useAutoOffAnimation;

    uint8_t pulseUpdateInterval;

    uint8_t hueChangeIntervall;

    uint8_t mode;

} UserEepromParams;

extern uint8_t useAutoOffAnimation;

#define USER_KEY_PRESS_DELAY_100ms 3

#define USER_KEY_PRESS_REALLY_LONG_DURATION_100ms 30

#define USER_HUE_CHANGE_INT_100ms 1

#define USER_HUE_CHANGE_INT_100ms_MIN 1

#define USER_HUE_CHANGE_INT_100ms_MAX 8

#define USER_PULSE_CHANGE_INT_10ms 1

#define USER_PULSE_CHANGE_INT_10ms_MIN 1

#define USER_PULSE_CHANGE_INT_10ms_MAX 5

#define USER_DEMO_CHANGE_INT_100ms 5

#define USER_NORMAL_SHOW_NUMBER_DELAY_100ms 10

#define USER_STARTUP_WAIT_4_IR_TRAIN_s 7

#define USER_ENTERTIME_DAY_NIGHT_CHANGE_HOUR 8

#define USER_ENTERTIME_DAY_BRIGHTNESS 255

#define USER_ENTERTIME_NIGHT_BRIGHTNESS 50

#define USER_ENTER_ONOFF_TIME_STEP 15

extern void handle_ir_code();

extern void user_setNewTime(const datetime_t* i_time);

extern void user_isr1000Hz();

extern void user_isr100Hz();

extern void user_isr10Hz();

extern void user_isr1Hz();

extern void user_init();

#define USER_CREATE_IR_HANDLER(command, code) \
    } else if (command == ir_code) { \
        code \
        user_setNewTime(0);

#if (INDIVIDUAL_CONFIG == 0 || DCF_PRESENT == 1)

    #define USER_CMD_DEFAULT_DCF USER_CMD_DEF_DCF_GET_TIME,

#else

    #define USER_CMD_DEFAULT_DCF

#endif

#if (INDIVIDUAL_CONFIG == 0 || MONO_COLOR_CLOCK != 1)

    #define USER_CMD_DEFAULT_MULTICOLOR \
        USER_CMD_DEF_HUE_MODE, \
        USER_CMD_DEF_CHANGE_R, \
        USER_CMD_DEF_CHANGE_G, \
        USER_CMD_DEF_CHANGE_B, \
        USER_CMD_DEF_CHANGE_HUE,

#else

    #define USER_CMD_DEFAULT_MULTICOLOR

#endif

#if (INDIVIDUAL_CONFIG == 0 || AMBILIGHT_PRESENT == 1)

    #define USER_CMD_DEFAULT_AMBILIGHT USER_CMD_DEF_AMBILIGHT,

#else

    #define USER_CMD_DEFAULT_AMBILIGHT

#endif

#if (INDIVIDUAL_CONFIG == 0 || BLUETOOTH_PRESENT == 1)

    #define USER_CMD_DEFAULT_BLUETOOTH USER_CMD_DEF_BLUETOOTH,

#else

    #define USER_CMD_DEFAULT_BLUETOOTH

#endif

#if (INDIVIDUAL_CONFIG == 0 || AUXPOWER_PRESENT == 1)

    #define USER_CMD_DEFAULT_AUXPOWER USER_CMD_DEF_AUXPOWER,

#else

    #define USER_CMD_DEFAULT_AUXPOWER

#endif

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

#define USEREEPROMPARAMS_DEFAULT { \
\
    USER_ADDRESS_DEFAULT, \
    USER_COMMANDCODES_DEFAULTS, \
    USER_COLORPRESETS_DEFAULTS, \
    0, \
    USER_ON_OFF_TIME_DEFAULTS, \
    1, \
    USER_PULSE_CHANGE_INT_10ms, \
    USER_HUE_CHANGE_INT_100ms, \
    MS_normalMode \
\
}

#endif /* _WC_USER_H_ */
