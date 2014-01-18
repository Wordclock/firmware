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
 * @file usermodes.c
 * @brief Contains the implementation of the user modes itself
 *
 * This file gets simply included within user.c and does not represent its own
 * module. It contains the implementation of the user modes as well as the
 * appropriate IR handlers, which will process the received IR commands for
 * each user mode differently. It is based around the state handling provided
 * by functions defined in user.h and/or user.c.
 *
 * @see user.h
 * @see user.c
 */

/**
 * @brief Data needed for the "training" mode
 *
 * @see mode_trainIrState
 * @see menu_state_t::MS_irTrain
 */
typedef struct TrainIrState {

    /**
     * @brief Number of seconds passed since entering the "training" mode
     *
     * This is used to determine whether enough time has passed for this mode
     * to quit itself again (USER_STARTUP_WAIT_IR_TRAIN_S).
     *
     * @see TrainIrState_1Hz()
     * @see USER_STARTUP_WAIT_IR_TRAIN_S
     */
    uint8_t seconds;

    /**
     * @brief Key currently being trained
     *
     * This is used to keep track of the key and/or command that is currently
     * being expected to be trained within TrainIrState_handleIR().
     *
     * @see user_command_t
     * @see TrainIrState_handleIR()
     */
    uint8_t curKey;

} TrainIrState;

/**
 * @brief Actual variable holding TrainIrState
 *
 * @see TrainIrState
 * @see TrainIrState_1Hz()
 * @see TrainIrState_handleIR()
 */
static TrainIrState mode_trainIrState;

/**
 * @brief Data needed for the "show number" mode
 *
 * @see mode_showNumberState
 * @see menu_state_t::MS_showNumber
 */
typedef struct ShowNumberState {

    /**
     * @brief Counter to keep track of the time the number has been shown
     *
     * This is used as counter to keep track of the duration the current number
     * has already been displayed. Once the number has been shown for the
     * amount of time defined in USER_NORMAL_SHOW_NUMBER_DELAY_100MS the mode
     * will quit itself.
     *
     * @see ShowNumberState_enter()
     * @see ShowNumberState_10Hz()
     * @see USER_NORMAL_SHOW_NUMBER_DELAY_100MS
     */
    uint8_t delay100ms;

} ShowNumberState;

/**
 * @brief Actual variable holding ShowNumberState
 *
 * @see ShowNumberState
 * @see ShowNumberState_enter()
 * @see ShowNumberState_10Hz()
 */
static ShowNumberState mode_showNumberState;

#if (ENABLE_RGB_SUPPORT == 1)

    /**
     * @brief Allowed properties which can be set in "normal" mode
     *
     * This properties can be changed by the user in "normal" mode by sending
     * the appropriate commands.
     *
     * @see NormalState::propertyToSet
     */
    typedef enum
    {

        /**
         * @brief Represents the property "red color"
         */
        NS_propColorR = 0,

        /**
         * @brief Represents the property "green color"
         */
        NS_propColorG,

        /**
         * @brief Represents the property "blue color"
         */
        NS_propColorB,

        /**
         * @brief Represents the property "hue"
         */
        NS_propHue

    } propToSet_t;

    /**
     * @brief Data needed for the "normal" mode
     *
     * @see mode_normalState
     * @see menu_state_t::MS_normalMode
     */
    typedef struct NormalState {

        /**
         * @brief Keeps track which property is currently being set
         *
         * This is used to keep track of the property (red, green, blue, hue)
         * that is currently being set, so that received "up"
         * (user_command_t::UC_UP) and/or "down" (user_command_t::UC_DOWN)
         * commands can be assigned correctly.
         *
         * @see NormalState_handleUserCommand()
         * @see propToSet_t
         */
        propToSet_t propertyToSet;

        /**
         * @brief Current hue value
         *
         * This holds the current hue value, which can only be changed by the
         * user in this mode.
         *
         * @see NormalState_handleUserCommand()
         * @see UC_CHANGE_HUE
         */
        Hue_t curHue;

    } NormalState;

    /**
     * @brief Actual variable holding NormalState
     *
     * @see NormalState
     * @see NormalState_handleUserCommand()
     */
    static NormalState mode_normalState;

#endif

/**
 * @brief Data needed for the "pulse" mode
 *
 * @see mode_pulseState
 * @see menu_state_t::MS_pulseMode
 */
typedef struct PulseState {

    /**
     * @brief Holds the current brightness step
     *
     * This contains the current brightness step, which will be passed on to
     * color_pulse_waveform() when in  pulse mode and is newly calculated every
     * 10 ms by PulseState_100Hz().
     *
     * @see PulseState_100Hz()
     * @see color_pulse_waveform()
     */
    uint8_t curBrightness;

    /**
     * @brief Counter keeping track of the time displaying with same brightness
     *
     * This is used as counter to keep track of the duration the output is
     * already being displayed with the current brightness. Once the brightness
     * hasn't changed for the amount of time defined by the user in
     * UserEepromParams::pulseUpdateInterval it will be updated.
     *
     * @see UserEepromParams::pulseUpdateInterval
     * @see PulseState_100Hz()
     */
    uint8_t delay10ms;

} PulseState;

/**
 * @brief Actual variable holding PulseState
 *
 * @see PulseState
 * @see PulseState_100Hz()
 */
static PulseState mode_pulseState;

#if (ENABLE_RGB_SUPPORT == 1)

    /**
     * @brief Data needed for the "hue fading" mode
     *
     * @see mode_autoHueState
     * @see menu_state_t::MS_hueMode
     */
    typedef struct AutoHueState {

        /**
         * @brief The hue the output is currently being displayed with
         *
         * This is a simple counter representing the hue the output is
         * currently being displayed with. This is simply passed on to
         * color_hue2rgb() and wraps around once it reaches COLOR_HUE_MAX.
         *
         * @see COLOR_HUE_MAX
         * @see color_hue2rgb()
         * @see AutoHueState_10Hz()
         */
        Hue_t curHue;

        /**
         * @brief Counter to keep track of the length of the current hue fading
         *
         * This is used as counter to keep track of the duration the output
         * has been displayed with the current hue (AutoHueState::curHue).
         * Once the user defined time interval
         * (UserEepromParams::hueChangeInterval) has been reached, it will
         * update and apply the new hue.
         *
         * @see AutoHueState_10Hz()
         * @see UserEepromParams::hueChangeInterval
         * @see USER_NORMAL_SHOW_NUMBER_DELAY_100MS
         */
        uint8_t delay100ms;

    } AutoHueState;

    /**
     * @brief Actual variable holding AutoHueState
     *
     * @see AutoHueState
     * @see AutoHueState_10Hz()
     */
    static AutoHueState mode_autoHueState;

#endif

/**
 * @brief Data needed for the "demo" mode
 *
 * @see mode_demoState
 * @see menu_state_t::MS_demoMode
 */
typedef struct DemoState {

    /**
     * @brief The current step of the demo animation
     *
     * The demo (both in normal as well as in fast mode) is consisting of
     * different steps. Each step corresponds to a different display state.
     * The actual meaning of a step depends upon the mode ("normal" and/or
     * "fast"). Within "normal" mode each step corresponds to a different
     * LED group being enabled. Within "fast" mode there are actually only
     * eight different steps. Within each step a different output of each LED
     * driver is enabled. This is responsible for the multiplexing, which
     * makes all LEDs appear to be enabled.
     *
     * @see DisplayState
     * @see DemoState_1000Hz()
     * @see DemoState_10Hz()
     */
    uint8_t demoStep;

    /**
     * @brief Counter to keep track of the length of the current step
     *
     * This is used as counter to keep track of the duration of the current
     * step within the demo animation. It is only used in "normal" mode. Once
     * the interval reaches the limit defined by USER_DEMO_CHANGE_INT_100MS
     * the next demo step is performed.
     *
     * @see AutoHueState_10Hz()
     * @see USER_DEMO_CHANGE_INT_100MS
     */
    uint8_t delay100ms;

    /*
     * @brief Indicates whether or not the "fast" mode is enabled
     *
     * As the "demo" mode itself consists of two modes ("normal" and/or
     * "fast"). This variable is set to true once the "fast" mode has been
     * selected, and reset to false when it is left again.
     *
     * @see DemoState_handleUserCommand()
     * @see AutoHueState_1000Hz()
     */
    bool fastMode;

} DemoState;

/**
 * @brief Actual variable holding DemoState
 *
 * @see DemoState
 * @see DemoState_handleUserCommand()
 * @see AutoHueState_10Hz()
 * @see AutoHueState_1000Hz()
 */
static DemoState mode_demoState;

/**
 * @brief Data needed for the "set system time" mode
 *
 * @see mode_setSystemTimeState
 * @see menu_state_t::MS_setSystemTime
 */
typedef struct SetSystemTimeState {

    /**
     * @brief Indicates whether or not the mode can currently be left
     *
     * @see SetSystemTimeState_enter()
     * @see SetSystemTimeState_substateFinished()
     * @see UserState_prohibitLeave()
     */
    bool prohibitLeave;

} SetSystemTimeState;

/**
 * @brief Actual variable holding SetSystemTimeState
 *
 * @see SetSystemTimeState
 * @see SetSystemTimeState_enter()
 * @see SetSystemTimeState_substateFinished()
 * @see UserState_prohibitLeave()
 */
static SetSystemTimeState mode_setSystemTimeState;

/**
 * @brief Data needed for the "set on/off time(s)" mode
 *
 * @see mode_setOnOffTimeState
 * @see menu_state_t::MS_setOnOffTime
 */
typedef struct SetOnOffTimeState {

    /**
     * @brief Indicates whether or not the mode can currently be left
     *
     * @see SetOnOffTimeState_enter()
     * @see SetOnOffTimeState_handleUserCommand()
     * @see UserState_prohibitLeave()
     */
    bool prohibitLeave;

    /*
     * @brief Index of on/off time currently being set
     *
     * There are up to UI_ONOFFTIMES_COUNT different on/off times, which are
     * set consecutively one by one. This defines the index of the on
     * currently being set.
     *
     * @see UI_ONOFFTIMES_COUNT
     * @see SetOnOffTimeState_enter()
     * @see SetOnOffTimeState_substateFinished()
     */
    uint8_t currentTimeToSet;

} SetOnOffTimeState;

/**
 * @brief Actual variable holding SetOnOffTimeState
 *
 * @see SetOnOffTimeState
 * @see SetOnOffTimeState_enter()
 * @see SetOnOffTimeState_substateFinished()
 * @see UserState_prohibitLeave()
 * @see SetOnOffTimeState_handleUserCommand()
 */
static SetOnOffTimeState mode_setOnOffTimeState;

/**
 * @brief Data needed for the "enter time" mode
 *
 * @see mode_enterTimeState
 * @see menu_state_t::MS_enterTime
 */
typedef struct EnterTimeState {

    /**
     * @brief Indicates whether or not the mode can currently be left
     *
     * @see EnterTimeState_enter()
     * @see EnterTimeState_handleUserCommand()
     * @see UserState_prohibitLeave()
     */
    bool prohibitLeave;

    /**
     * @brief Buffer that is manipulated during time input
     *
     * This buffer will be manipulated during time input and is then copied
     * once the time entering is actually finished.
     *
     * @see EnterTimeState_enter()
     * @see EnterTimeState_handleUserCommand()
     */
    datetime_t time;

    /**
     * @brief Current substate of time input
     *
     * The input works by inputting the hour and the minutes separately. This
     * represents both of these modes.
     *
     * @see EnterTimeState_enter()
     * @see EnterTimeState_handleUserCommand()
     */
    enum e_EnterTimeSubstates {

        /**
         * @brief Represents time input of the hour
         */
        ETS_hour,

        /**
         * @brief Represents time input of the minutes
         */
        ETS_minutes,

    } curSubState;

} EnterTimeState;

/**
 * @brief Actual variable holding EnterTimeState
 *
 * @see EnterTimeState
 * @see EnterTimeState_enter()
 * @see EnterTimeState_handleUserCommand()
 * @see UserState_prohibitLeave()
 */
static EnterTimeState mode_enterTimeState;

static void UserState_init();

static bool UserState_prohibitLeave(menu_state_t state);

static bool UserState_prohibitTimeDisplay(menu_state_t state);

static void UserState_Isr1000Hz(menu_state_t state);

static void UserState_Isr100Hz(menu_state_t state);

static void UserState_Isr10Hz(menu_state_t state);

static void UserState_Isr1Hz(menu_state_t state);

static void UserState_LeaveState(menu_state_t state);

static bool UserState_HandleUserCommand(menu_state_t state, user_command_t command);

static void UserState_SubstateFinished(menu_state_t state, menu_state_t finishedState, const void* result);

static void UserState_enter(menu_state_t state, const void* param);

#if (LOG_USER_IR_TRAIN == 1)

    /**
     * @brief Used to output debug information when in IR training mode
     *
     * When the logging for this module is enabled (LOG_USER_IR_TRAIN == 1),
     * this macro can be used to output information for debugging purposes.
     *
     * @see LOG_USER_IR_TRAIN
     */
    #define log_irTrain(x) uart_puts_P(x)

#else

    /**
     * @brief Dummy used to output debug information when in IR training mode
     *
     * This makes sure that nothing is being output when the logging for this
     * module is deactivated (LOG_USER_IR_TRAIN == 0).
     *
     * @see LOG_USER_IR_TRAIN
     */
    #define log_irTrain(x)

#endif

/**
 * @brief Routine executed when entering the "training" mode
 *
 * This routine gets executed whenever the "training"
 * (menu_state_t::MS_irTrain) mode is entered. It retrieves the indicator mask
 * for the display and applies it in a way that it will be blinking.
 *
 * @param param Void parameter for consistency reasons only
 *
 * @see display_getIndicatorMask()
 * @see display_setDisplayState()
 */
static void TrainIrState_enter(const void* param)
{

    DisplayState disp;

    log_state("enter train\n");

    disp = display_getIndicatorMask();
    display_setDisplayState(disp, disp);

}

/**
 * @brief ISR for the "training" mode executed with a frequency of 1 Hz
 *
 * This "ISR" gets executed with a frequency of 1 Hz whenever the "training"
 * (menu_state_t::MS_irTrain) mode is currently active. It checks whether
 * enough time (USER_STARTUP_WAIT_IR_TRAIN_S) has passed to quit itself and
 * does so if necessary.
 *
 * @see TrainIrState::seconds
 * @see USER_STARTUP_WAIT_IR_TRAIN_S
 * @see quitMyself()
 */
static void TrainIrState_1Hz()
{

    if (mode_trainIrState.seconds != UINT8_MAX) {

        ++mode_trainIrState.seconds;

        if (mode_trainIrState.seconds == USER_STARTUP_WAIT_IR_TRAIN_S) {

            log_irTrain("leave IR-wait4train\n");

            quitMyself(MS_irTrain, NULL);

        }

    }

}

/**
 * @brief IR handling routine for the "training" mode
 *
 * This function handles the received IR commands for the "training"
 * (menu_state_t::MS_irTrain) mode. It stores the address of the first valid IR
 * frame and expect all further commands to come from this address. A counter
 * (mode_trainIrState.curKey) is used to keep track of the command currently
 * being trained. Whenever receiving a command from this controller it will
 * assign the received command to the key and increment the counter. Once every
 * key has been assigned, it will initiate a writeback to the EEPROM and exit
 * itself. It will also set the display state to the number of the key
 * currently being assigned and let it blink.
 *
 * @param i_irCode The decoded IRMP data that was received
 *
 * @see mode_trainIrState
 * @see TrainIrState::curKey
 * @see UserEepromParams::irAddress
 * @see UserEepromParams::irCommandCodes
 * @see wcEeprom_writeback()
 * @see quitMyself()
 * @see display_getNumberDisplayState()
 * @see display_setDisplayState()
 */
static void TrainIrState_handleIR(const IRMP_DATA* i_irCode)
{

    DisplayState disp;

    if (mode_trainIrState.curKey > 0) {

        if (g_params->irAddress != i_irCode->address) {

            log_irTrain("invalid ir-address\n");

        } else {

            g_params->irCommandCodes[mode_trainIrState.curKey - 1] = i_irCode->command;

            if (mode_trainIrState.curKey == UC_COMMAND_COUNT) {

                log_irTrain("Ir train finished\n");

                wcEeprom_writeback(g_params, sizeof(*g_params));

                quitMyself(MS_irTrain, NULL);

            }

            ++mode_trainIrState.curKey;

        }

    } else {

        mode_trainIrState.seconds = UINT8_MAX;
        g_params->irAddress = i_irCode->address;
        ++mode_trainIrState.curKey;

    }

    #if (LOG_USER_IR_TRAIN)

    {

        char buff[5];

        uart_puts_P("Ir train. Enter cmd #");
        uint8ToStrLessOneHundred(mode_trainIrState.curKey, buff);
        uart_puts(buff);
        uart_putc('\n');

    }

    #endif

    disp = display_getNumberDisplayState(mode_trainIrState.curKey);
    disp |= display_getIndicatorMask();
    display_setDisplayState(disp, disp);

}

/**
 * @brief ISR for the "show number" mode executed with a frequency of 10 Hz
 *
 * This "ISR" gets executed with a frequency of 10 Hz whenever the
 * "show number" (menu_state_t::MS_showNumber) mode is currently active. It
 * check whether enough time has passed to quit itself and does so if
 * necessary.
 *
 * @see ShowNumberState::delay100ms
 * @see mode_showNumberState
 * @see quitMyself()
 */
static void ShowNumberState_10Hz()
{

    --mode_showNumberState.delay100ms;

    if (mode_showNumberState.delay100ms == 0) {

        quitMyself(MS_showNumber, NULL);

    }

}

/**
 * @brief Routine executed when entering the "show number" mode
 *
 * This routine gets executed whenever the "show number"
 * (menu_state_t::MS_showNumber) mode is entered. It sets up the counter
 * (mode_showNumberState.delay100ms) with the value defined in
 * USER_NORMAL_SHOW_NUMBER_DELAY_100MS, gets the display state for the given
 * number and applies it to the display. The parameter itself is a pointer to
 * a value interpreted as uint8_t.
 *
 * @param param Pointer to number to be shown (uint8_t)
 *
 * @see ShowNumberState::delay100ms
 * @see mode_showNumberState
 * @see display_getNumberDisplayState()
 * @see display_setDisplayState()
 */
static void ShowNumberState_enter(const void* param)
{

    DisplayState disp;

    log_state("enter showNumber\n");

    mode_showNumberState.delay100ms = USER_NORMAL_SHOW_NUMBER_DELAY_100MS;

    /*
     * Double cast to prevent warning
     */
    disp = display_getNumberDisplayState((uint8_t)(uint16_t)param);
    display_setDisplayState(disp, 0);

}

/**
 * @brief Routine executed when entering the "normal" mode
 *
 * This routine gets executed whenever the "normal"
 * (menu_state_t::MS_normalMode) mode is entered. In case the software is built
 * with support for RGB colors (ENABLE_RGB_SUPPORT == 1), it applies the color
 * settings of the current color profile for each channel. When a parameter
 * is given it will display the number of the the appropriate color profile for
 * a short period of time. In case of no RGB support all of the logic mentioned
 * above is skipped and the current time will be displayed directly.
 *
 * @param param Pointer to number of the color profile to work on
 *
 * @see ENABLE_RGB_SUPPORT
 * @see pwm_set_color_step()
 * @see UserEepromParams::curColorProfile
 * @see UserEepromParams::colorPresets
 * @see addSubState()
 * @see menu_state_t::MS_showNumber
 * @see dispInternalTime()
 */
static void NormalState_enter(const void* param)
{

    log_state("nm\n");

    #if (ENABLE_RGB_SUPPORT == 1)

        pwm_set_colors(g_params->colorPresets[g_params->curColorProfile].r,
            g_params->colorPresets[g_params->curColorProfile].g,
            g_params->colorPresets[g_params->curColorProfile].b);

        if (((uint16_t)param) != 0) {

            /*
             * Double cast to prevent warning
             */
            void* param = (void*)(((uint16_t)g_params->curColorProfile + 1));
            addSubState(MS_normalMode, MS_showNumber, param);

        }

    #else

        dispInternalTime(&g_dateTime, 0);

    #endif
}

/**
 * @brief User command handling routine for the "normal" mode
 *
 * This function handles the received user commands for the "normal"
 * (menu_state_t::MS_normalMode) mode. It switches the color profile when the
 * "normal mode" user command is received. When receiving the commands to
 * change the color and/or hue, it makes sure that the appropriate property is
 * set and directly applied to the display whenever receiving "up" and/or
 * "down" user commands.
 *
 * @param command The received user command
 *
 * @see mode_normalState
 * @see NormalState::propertyToSet
 * @see user_command_t::UC_NORMAL_MODE
 * @see user_command_t::UC_CHANGE_R
 * @see user_command_t::UC_CHANGE_G
 * @see user_command_t::UC_CHANGE_B
 * @see user_command_t::UC_CHANGE_HUE
 * @see user_command_t::UC_UP
 * @see user_command_t::UC_DOWN
 * @see incDecRange()
 */
static bool NormalState_handleUserCommand(user_command_t command)
{

    #if (ENABLE_RGB_SUPPORT == 1)

        if (UC_NORMAL_MODE == command) {

            ++(g_params->curColorProfile);
            g_params->curColorProfile %=  UI_COLOR_PRESET_COUNT;
            NormalState_enter((void*)1);

        } else if (UC_CHANGE_R == command) {

            log_state("CR\n");

            mode_normalState.propertyToSet = NS_propColorR;

        } else if (UC_CHANGE_G == command) {

            log_state("CG\n");

            mode_normalState.propertyToSet = NS_propColorG;

        } else if (UC_CHANGE_B == command) {

            log_state("CB\n");

            mode_normalState.propertyToSet = NS_propColorB;

        } else if (UC_CHANGE_HUE == command) {

            log_state("CH\n");

            mode_normalState.propertyToSet = NS_propHue;

        } else if (UC_UP == command || UC_DOWN == command) {

            int8_t dir = (UC_UP == command) ? 1 : -1;

            log_state("CC\n");

            if (mode_normalState.propertyToSet == NS_propHue) {

                uint8_t r, g, b;

                if (dir < 0 && mode_normalState.curHue < USER_HUE_CHANGE_MANUAL_STEPS) {

                    mode_normalState.curHue = COLOR_HUE_MAX;

                } else if (dir > 0 && mode_normalState.curHue >= COLOR_HUE_MAX - USER_HUE_CHANGE_MANUAL_STEPS) {

                    mode_normalState.curHue = 0;

                } else {

                    mode_normalState.curHue += dir * USER_HUE_CHANGE_MANUAL_STEPS;

                }

                color_hue2rgb(mode_normalState.curHue, &r, &g, &b);
                pwm_set_colors(r, g, b);

            } else {

                uint8_t* rgb = (uint8_t*)(&g_params->colorPresets[g_params->curColorProfile]);

                incDecRange(&rgb[mode_normalState.propertyToSet], dir, 0, 255);
                pwm_set_colors(rgb[0], rgb[1], rgb[2]);

            }

        } else {

            return false;

        }

        return true;

    #else

        return false;

    #endif

}

#if (ENABLE_RGB_SUPPORT == 1)

    /**
     * @brief ISR for the "hue fading" mode executed with a frequency of 10 Hz
     *
     * This "ISR" gets executed with a frequency of 10 Hz whenever the
     * "hue fading" (menu_state_t::MS_hueMode) mode is currently active. It
     * checks the appropriate time interval set by the user
     * (UserEepromParams::hueChangeInterval) has passed and updates the hue
     * if necessary.
     *
     * @see AutoHueState::delay100ms
     * @see mode_autoHueState
     * @see UserEepromParams::hueChangeInterval
     * @see color_hue2rgb()
     * @see pwm_set_colors()
     */
    static void AutoHueState_10Hz()
    {

        mode_autoHueState.delay100ms++;

        if (mode_autoHueState.delay100ms > (volatile uint8_t)(g_params->hueChangeInterval)) {

            uint8_t r, g, b;

            ++mode_autoHueState.curHue;
            mode_autoHueState.curHue %= (COLOR_HUE_MAX + 1);
            color_hue2rgb(mode_autoHueState.curHue, &r, &g, &b);
            pwm_set_colors(r, g, b);
            mode_autoHueState.delay100ms = 0;

        }

    }

    /**
     * @brief Routine executed when entering the "hue fading" mode
     *
     * This routine gets executed whenever the "hue fading"
     * (menu_state_t::MS_hueMode) mode is entered. It simply resets the
     * appropriate counter responsible for the hue fading interval.
     *
     * @param param Void parameter for consistency reasons only
     *
     * @see AutoHueState::delay100ms
     * @see mode_autoHueState
     */
    static void AutoHueState_enter(const void* param)
    {

        mode_autoHueState.delay100ms = 0;

    }

    /**
     * @brief User command handling routine for the "hue fading" mode
     *
     * This function handles the received user commands for the "hue fading"
     * mode (menu_state_t::MS_hueMode). It checks whether an "up" and/or "down"
     * command was received and increments and/or decrements the interval
     * (UserEepromParams::hueChangeInterval) appropriately.
     *
     * @param command The received user command
     *
     * @see user_command_t::UC_UP
     * @see user_command_t::UC_DOWN
     * @see incDecRange()
     * @see UserEepromParams::hueChangeInterval
     * @see USER_HUE_CHANGE_INT_100MS_MIN
     * @see USER_HUE_CHANGE_INT_100MS_MAX
     */
    static bool AutoHueState_handleUserCommand(user_command_t command)
    {

        if (UC_UP == command || UC_DOWN == command) {

            int8_t dir = (UC_UP == command) ? -1 : 1;

            log_state("CHS \n");

            incDecRange(&g_params->hueChangeInterval, dir,
                USER_HUE_CHANGE_INT_100MS_MIN, USER_HUE_CHANGE_INT_100MS_MAX);

            #if (LOG_USER_STATE == 1)

                uart_putc('0' + g_params->hueChangeInterval);

            #endif

            return true;

        }

        return false;

    }

#endif

/**
 * @brief ISR for the "demo" mode executed with a frequency of 1 kHz
 *
 * This "ISR" gets executed with a frequency of 1 kHz whenever the "demo"
 * (menu_state_t::MS_demo) mode is currently active. It is only needed
 * when the "fast" mode (DemoState::fastMode) is being used. Otherwise it will
 * return immediately. In "fast" mode it sets the brightness to its maximum,
 * and then multiplexes the output so each word appears to be enabled at once.
 *
 * @see mode_demoState
 * @see DemoState::fastMode
 * @see DemoState::demoStep
 * @see pwm_lock_brightness_val()
 */
static void DemoState_1000Hz()
{

    DisplayState disp;

    if (!mode_demoState.fastMode) {

        return;

    }

    pwm_lock_brightness_val(255);
    disp = (DisplayState)0x01010101 << mode_demoState.demoStep;
    display_setDisplayState(disp, 0);
    ++mode_demoState.demoStep;
    mode_demoState.demoStep %= 8;

}

/**
 * @brief ISR for the "demo" mode executed with a frequency of 10 Hz
 *
 * This "ISR" gets executed with a frequency of 10 Hz whenever the "demo"
 * (menu_state_t::MS_demo) mode is currently active. It is only needed
 * when the "normal" mode (DemoState::fastMode) is being used. Otherwise it
 * will return immediately. In "normal" mode it checks whether the appropriate
 * time interval has passed (USER_DEMO_CHANGE_INT_100MS) and turns on the next
 * LED group if necessary.
 *
 * @see mode_demoState
 * @see DemoState::fastMode
 * @see DemoState::delay100ms
 * @see USER_DEMO_CHANGE_INT_100MS
 * @see DemoState::demoStep
 */
static void DemoState_10Hz()
{

    DisplayState disp;

    if (mode_demoState.fastMode) {

        return;

    }

    ++mode_demoState.delay100ms;

    if (mode_demoState.delay100ms >= USER_DEMO_CHANGE_INT_100MS) {

        disp = (DisplayState)1 << mode_demoState.demoStep;
        display_setDisplayState(disp, 0);
        ++mode_demoState.demoStep;
        mode_demoState.demoStep %= 32;
        mode_demoState.delay100ms = 0;

    }

}

/**
 * @brief User command handling routine for the "demo" mode
 *
 * This function handles the received user commands for the "demo"
 * (menu_state_t::MS_demo) mode. It checks whether an "up" and/or "down"
 * user command was received and toggles between the "fast" and/or "normal"
 * mode.
 *
 * @param command The received user command
 *
 * @see user_command_t::UC_UP
 * @see user_command_t::UC_DOWN
 * @see mode_demoState
 * @see DemoState::fastMode
 */
static bool DemoState_handleUserCommand(user_command_t command)
{

    if (UC_UP == command || UC_DOWN == command) {

        log_state("DMF\n");

        mode_demoState.fastMode = !mode_demoState.fastMode;

    } else {

        return false;

    }

    return true;

}

/**
 * @brief Routine executed when leaving the "demo" mode
 *
 * This routine gets executed whenever the "demo" (menu_state_t::MS_demo) mode
 * is left. It will make sure the brightness lock for the PWM module is
 * released, which was acquired by the appropriate ISR.
 *
 * @see pwm_release_brightness()
 * @see DemoState_1000Hz()
 */
static void DemoState_leave()
{

    pwm_release_brightness();

}

/**
 * @brief Routine executed when entering the "enter time" mode
 *
 * This routine gets executed whenever the "hue fading"
 * (menu_state_t::MS_enterTime) mode is entered. Initially it saves the pointer
 * to the buffer to be manipulated later on in (EnterTimeState::time), sets the
 * current substate to enter the hour and prohibits to leave the mode. On top
 * of that it sets the brightness according to the current time of day and
 * outputs the time provided in the given buffer along with indicators for the
 * hour and the time setting itself.
 *
 * @param param Pointer to buffer for time manipulation
 *
 * @see mode_enterTimeState
 * @see EnterTimeState
 * @see USER_ENTERTIME_DAY_NIGHT_CHANGE_HOUR
 * @see pwm_lock_brightness_val()
 * @see dispInternalTime()
 * @see display_getHoursMask()
 * @see display_getTimeSetIndicatorMask()
 */
static void EnterTimeState_enter(const void* param)
{

    DisplayState disp;

    log_time("TH\n");

    mode_enterTimeState.time = *((datetime_t*)param);
    mode_enterTimeState.curSubState = ETS_hour;
    mode_enterTimeState.prohibitLeave = true;

    if (mode_enterTimeState.time.hh >= USER_ENTERTIME_DAY_NIGHT_CHANGE_HOUR
        && mode_enterTimeState.time.hh < USER_ENTERTIME_DAY_NIGHT_CHANGE_HOUR + 12) {

        pwm_lock_brightness_val(USER_ENTERTIME_DAY_BRIGHTNESS);

    } else {

        pwm_lock_brightness_val(USER_ENTERTIME_NIGHT_BRIGHTNESS);

    }

    disp = display_getHoursMask();
    disp |= display_getTimeSetIndicatorMask();
    dispInternalTime(&mode_enterTimeState.time, disp);

}

/**
 * @brief IR handling routine for the "enter time" mode
 *
 * This function handles the received IR commands for the "enter time"
 * (menu_state_t::MS_enterTime) mode. As this mode can actually be entered from
 * two modes (menu_state_t::MS_setSystemTime and
 * menu_state_t::MS_setOnOffTime), this function checks whether either the
 * user_command_t::UC_SET_TIME and/or user_command_t::UC_SET_ONOFF_TIMES
 * command was received and switches to the "minute entering" substate if
 * necessary. Once the minutes are entered, too, it releases the brightness
 * lock again, allows the user to leave the mode and quits itself with a
 * pointer to the buffer of the set time.
 *
 * It also handles the minute and/or hour entering itself, whenever receiving
 * an "up" and/or "down" command, sets the brightness according to the set
 * day of time and makes sure that only valid times are entered.
 *
 * It outputs the property being set at all stages, to give the user a visual
 * feedback of what he is actually doing.
 *
 * @param cmdCode The received IR command code
 *
 * @see user_command_t::UC_SET_TIME
 * @see user_command_t::UC_SET_ONOFF_TIMES
 * @see mode_enterTimeState
 * @see USER_ENTERTIME_DAY_NIGHT_CHANGE_HOUR
 * @see pwm_lock_brightness_val()
 * @see pwm_release_brightness()
 * @see quitMyself()
 * @see user_command_t::UC_UP
 * @see user_command_t::UC_DOWN
 * @see incDecRangeOverflow()
 * @see dispInternalTime()
 * @see DemoState::fastMode
 */
static bool EnterTimeState_handleUserCommand(user_command_t command)
{

    uint8_t caller = g_stateStack[g_currentIdxs[MS_enterTime] - 1];

    if (((MS_setSystemTime == caller) && (UC_SET_TIME == command))
        || ((MS_setOnOffTime == caller) && (UC_SET_ONOFF_TIMES == command))) {

        if (ETS_hour == mode_enterTimeState.curSubState) {

            log_time("TM\n");

            mode_enterTimeState.curSubState = ETS_minutes;

        } else if (ETS_minutes == mode_enterTimeState.curSubState) {

            log_time("TS\n");

            mode_enterTimeState.time.ss = 0;
            mode_enterTimeState.prohibitLeave = false;
            pwm_release_brightness();
            quitMyself(MS_enterTime, &(mode_enterTimeState.time));

            return true;

        }

    } else if (UC_UP == command || UC_DOWN == command) {

        int8_t dir = (UC_UP == command) ? 1 : -1;

        log_state("CHS\n");

        if (ETS_hour == mode_enterTimeState.curSubState) {

            incDecRangeOverflow(&(mode_enterTimeState.time.hh), dir, 23);

            if (mode_enterTimeState.time.hh >= USER_ENTERTIME_DAY_NIGHT_CHANGE_HOUR
                && mode_enterTimeState.time.hh < USER_ENTERTIME_DAY_NIGHT_CHANGE_HOUR + 12) {

                pwm_lock_brightness_val(USER_ENTERTIME_DAY_BRIGHTNESS);

            } else {

                pwm_lock_brightness_val(USER_ENTERTIME_NIGHT_BRIGHTNESS);

            }

        } else if (ETS_minutes == mode_enterTimeState.curSubState) {

            if (MS_setOnOffTime == caller) {

                dir *= USER_ENTER_ONOFF_TIME_STEP;

            }

            incDecRangeOverflow(&(mode_enterTimeState.time.mm), dir, 59);

        }

    }

    dispInternalTime(&mode_enterTimeState.time,
        ((ETS_hour == mode_enterTimeState.curSubState)
        ? display_getHoursMask()
        : display_getMinuteMask()) | display_getTimeSetIndicatorMask());

    return true;

}

/**
 * @brief Routine executed when entering the "set system time" mode
 *
 * This routine gets executed whenever the "set system time"
 * (menu_state_t::MS_setSystemTime) mode is entered. It calls the sub state
 * to enter a time (menu_state_t::MS_enterTime) and prohibits the user to
 * leave until finished.
 *
 * @param param Void parameter for consistency reasons only
 *
 * @see mode_setSystemTimeState
 * @see SetSystemTimeState
 * @see addSubState()
 * @see menu_state_t::MS_enterTime
 * @see g_dateTime
 */
static void SetSystemTimeState_enter(const void* param)
{

    log_state("SST\n");

    addSubState(MS_setSystemTime, MS_enterTime, &g_dateTime);
    mode_setSystemTimeState.prohibitLeave = true;

}

/**
 * @brief Routine executed when a substate of "set system time" was finished
 *
 * This routine gets executed whenever a substate of the "set system time"
 * (menu_state_t::MS_setSystemTime) mode has finished its job. For now the
 * only substate is actually the "enter time" (menu_state_t::MS_enterTime)
 * mode. Once the "enter time" mode has finished, this will make sure the set
 * time will be written to the RTC, makes the set time available to other
 * functions, allows the user to leave the mode and quits itself.
 *
 * @param result Pointer to the result of the substate (set time)
 *
 * @see SetSystemTimeState_enter()
 * @see mode_setSystemTimeState
 * @see SetSystemTimeState
 * @see i2c_rtc_write()
 * @see g_dateTime
 * @see quitMyself()
 */
static void SetSystemTimeState_substateFinished(menu_state_t finishedState, const void* result)
{

    if (finishedState == MS_enterTime) {

        const datetime_t* time = result;

        i2c_rtc_write(time);
        g_dateTime = *time;
        mode_setSystemTimeState.prohibitLeave = false;
        quitMyself(MS_setSystemTime, NULL);

    }

}

/**
 * @brief Routine executed when entering the "set on/off time(s)" mode
 *
 * This routine gets executed whenever the "set on/off time(s)"
 * (menu_state_t::MS_setOnOffTime) mode is entered. After some initialization
 * of various variables related to these settings, it calls the substate to
 * enter a time (menu_state_t::MS_enterTime) for the first "off" time and
 * prohibits the user to leave, which will only be allowed again when all times
 * have been entered.
 *
 * @param param Void parameter for consistency reasons only
 *
 * @see mode_setOnOffTimeState
 * @see SetOnOffTimeState
 * @see UserEepromParam::onOffTimes
 * @see menu_state_t::MS_enterTime
 * @see addSubState()
 */
static void SetOnOffTimeState_enter(const void* param)
{

    datetime_t dt = {0, 0, 0, 0, 0, 0, 0};
    dt.hh = g_params->onOffTimes[0].h;
    dt.mm = g_params->onOffTimes[0].m;

    log_state("SOOT\n");

    mode_setOnOffTimeState.currentTimeToSet = 0;

    addSubState(MS_setOnOffTime, MS_enterTime, &dt);

    mode_setOnOffTimeState.prohibitLeave = true;

}

/**
 * @brief Routine executed when a substate of "set on/off time(s)" was finished
 *
 * This routine gets executed whenever a substate of the "set on/off time(s)"
 * (menu_state_t::MS_setOnOffTime) mode has finished its job. For now the
 * only substate is the "enter time" (menu_state_t::MS_enterTime) mode. Once
 * the "enter time" mode has finished, this will make sure the set "on" and/or
 * "off" time is saved and if there are other times to be set, allows the user
 * to do so. The last stage gives the user the option to enable and/or disable
 * the autoOff animation along with a preview on how this actually looks like.
 *
 * @param result Pointer to the result of the substate (set time)
 *
 * @see SetOnOffTimeState_enter()
 * @see mode_setOnOffTimeState
 * @see SetOnOffTimeState
 * @see UserEepromParams::onOffTimes
 * @see UserEepromParams::useAutoOffAnimation
 * @see UI_ONOFFTIMES_COUNT
 * @see addSubState()
 */
static void SetOnOffTimeState_substateFinished(menu_state_t finishedState, const void* result)
{

    if (finishedState == MS_enterTime) {

        const datetime_t* time = result;
        datetime_t dt = *time;
        g_params->onOffTimes[mode_setOnOffTimeState.currentTimeToSet].h = dt.hh;
        g_params->onOffTimes[mode_setOnOffTimeState.currentTimeToSet].m = dt.mm;

        ++mode_setOnOffTimeState.currentTimeToSet;

        if (UI_ONOFFTIMES_COUNT == mode_setOnOffTimeState.currentTimeToSet) {

            DisplayState disp;
            uint8_t autoOnOff = (uint8_t)g_params->useAutoOffAnimation + 1;
            disp = display_getNumberDisplayState(autoOnOff);
            display_setDisplayState(disp, disp);
            g_animPreview = g_params->useAutoOffAnimation;

        } else {

            dt.hh = g_params->onOffTimes[mode_setOnOffTimeState.currentTimeToSet].h;
            dt.mm = g_params->onOffTimes[mode_setOnOffTimeState.currentTimeToSet].m;
            addSubState(MS_setOnOffTime, MS_enterTime, &dt);

        }

    }

}

/**
 * @brief User command handling routine for the "set on/off time(s)" mode
 *
 * This function handles the received user commands for the "set on/off
 * time(s)" mode (menu_state_t::MS_setOnOffTime). It allows the user to enable
 * and/or disable the autoOff animation with the "up" and/or "down" user
 * commands. Depending upon the status of this setting (on and/or off) a
 * preview of the animation is shown. When the actual "set on/off time(s)"
 * command itself was received, the mode will quit itself, as this is an
 * indication that the user has finished going through the process of this
 * mode.
 *
 * @param command The received user command
 *
 * @see mode_setOnOffTimeState
 * @see SetOnOffTimeState
 * @see UserEepromParams::useAutoOffAnimation
 * @see user_command_t::UC_DOWN
 * @see user_command_t::UC_UP
 * @see user_command_t::UC_SET_ONOFF_TIMES
 * @see quitMyself()
 */
static bool SetOnOffTimeState_handleUserCommand(user_command_t command)
{

    if (UI_ONOFFTIMES_COUNT == mode_setOnOffTimeState.currentTimeToSet) {

        if ((command == UC_DOWN) || (command == UC_UP)) {

            DisplayState disp;
            g_params->useAutoOffAnimation = !g_params->useAutoOffAnimation;
            uint8_t autoOnOff = g_params->useAutoOffAnimation ? 2 : 1;
            disp = display_getNumberDisplayState(autoOnOff);
            display_setDisplayState(disp, disp);
            g_animPreview = g_params->useAutoOffAnimation;

        }

        if (command == UC_SET_ONOFF_TIMES) {

            mode_setOnOffTimeState.prohibitLeave = false;
            g_animPreview = false;
            quitMyself(MS_setOnOffTime, NULL);

        }

    }

    return true;

}

/**
 * @brief User command handling routine for the "pulse" mode
 *
 * This function handles the received user commands for the "pulse"
 * (menu_state_t::MS_pulse) mode. It checks whether an "up" and/or "down" user
 * command was received and updates the user defined update interval
 * appropriately.
 *
 * @param command The received user command
 *
 * @see user_command_t::UC_UP
 * @see user_command_t::UC_DOWN
 * @see UserEepromParams::pulseUpdateInterval
 * @see incDecRange()
 * @see USER_PULSE_CHANGE_INT_10MS_MIN
 * @see USER_PULSE_CHANGE_INT_10MS_MAX
 */
static bool PulseState_handleUserCommand(user_command_t command)
{

    if (UC_UP == command || UC_DOWN == command) {

        int8_t dir = (UC_UP == command) ? -1 : 1;

        log_state("CPS \n");

        incDecRange(&g_params->pulseUpdateInterval, dir,
            USER_PULSE_CHANGE_INT_10MS_MIN, USER_PULSE_CHANGE_INT_10MS_MAX);

        #if (LOG_USER_STATE == 1)

            uart_putc('0' + g_params->pulseUpdateInterval);

        #endif

    } else {

        return false;

    }

    return true;

}

/**
 * @brief ISR for the "pulse" mode executed with a frequency of 100 Hz
 *
 * This "ISR" gets executed with a frequency of 100 Hz whenever the "pulse"
 * (menu_state_t::MS_pulse) mode is currently active. It checks whether enough
 * time has already passed (UserEepromParams::pulseUpdateInterval) for
 * a new brightness to be calculated and applied to the display and does so if
 * necessary.
 *
 * @see mode_pulseState
 * @see PulseState::delay10ms
 * @see UserEepromParams::pulseUpdateInterval
 * @see pwm_lock_brightness()
 * @see color_pulse_wafeform
 * @see mode_pulseState.curBrightness
 */
static void PulseState_100Hz()
{

    ++mode_pulseState.delay10ms;

    if (mode_pulseState.delay10ms >= (volatile uint8_t)(g_params->pulseUpdateInterval)) {

        pwm_lock_brightness_val(color_pulse_waveform(mode_pulseState.curBrightness));
        ++mode_pulseState.curBrightness;
        mode_pulseState.delay10ms = 0;

    }

}

/**
 * @brief ISR for the "pulse" mode executed with a frequency of 10 Hz
 *
 * This "ISR" gets executed with a frequency of 10 Hz whenever the "pulse"
 * (menu_state_t::MS_pulse) mode is currently active. It makes sure that the
 * mode underneath the "pulse" mode will get updated, too.
 *
 * @see UserState_Isr10Hz
 * @see g_stateStack
 * @see g_currentIdxs
 */
static void PulseState_10Hz()
{

    UserState_Isr10Hz(g_stateStack[g_currentIdxs[MS_pulse] - 1]);

}

/**
 * @brief Routine executed when leaving the "pulse" mode
 *
 * This routine gets executed whenever the "pulse" (menu_state_t::MS_pulse)
 * mode is left. It will make sure the brightness lock for the PWM module is
 * released, which was acquired by the appropriate ISR (PulseState_100Hz()).
 *
 * @see pwm_release_brightness()
 * @see PulseState_100Hz()
 */
static void PulseState_leave()
{

    pwm_release_brightness();

}

/**
 * @brief Initializes all available user states
 *
 * This makes sure that all appropriate *_init() functions are called, which
 * gives each mode a chance to initialize itself. For now no user mode actually
 * requires a separate initialization, so this can be considered a dummy.
 */
static void UserState_init()
{

}

/**
 * @brief Dispatcher routine for entering a state
 *
 * This function makes sure that the correct *_enter() function will be called
 * for the given state along with the provided parameter.
 *
 * @param state The state to be entered
 * @param param Parameter that should be passed along to the given state
 */
static void UserState_enter(menu_state_t state, const void* param)
{

    if (MS_enterTime == state) {

        EnterTimeState_enter(param);

    } else if (MS_normalMode == state) {

        NormalState_enter(param);

    } else if (MS_setOnOffTime == state) {

        SetOnOffTimeState_enter(param);

    } else if (MS_setSystemTime == state) {

        SetSystemTimeState_enter(param);

    } else if (MS_showNumber == state) {

        ShowNumberState_enter(param);

    } else if (MS_irTrain == state) {

        TrainIrState_enter(param);

    #if (ENABLE_RGB_SUPPORT == 1)

        } else if (MS_hueMode == state) {

            AutoHueState_enter(param);

    #endif

    }

}

/**
 * @brief Dispatcher routine to handle the result of finished substates
 *
 * A couple of states can execute substates, which will return a result once
 * finished. This makes sure that the result is given back to the state that
 * initially called the substate and provides it with the result.
 *
 * @param state The state that initiated the entering of the substate
 * @param finishedState The substate that has finished its job
 * @param result The result from the job performed by the substate
 */
static void UserState_SubstateFinished(menu_state_t state, menu_state_t finishedState, const void* result)
{

    if (MS_setOnOffTime == state) {

        SetOnOffTimeState_substateFinished(finishedState, result);

    } else if (MS_setSystemTime == state) {

        SetSystemTimeState_substateFinished(finishedState, result);

    }

}

/**
 * @brief Dispatcher routine for user command handling within a given state
 *
 * Each user mode may define its own user command handler along with its own
 * logic on how to handle specific user commands. This will dispatch a received
 * user command to the correct user command handler and return a boolean value
 * which indicates whether or not the appropriate handler has actually
 * processed the command.
 *
 * @param state The state that should handle the command code
 * @param command The command code that should be handled
 *
 * @return True if event was processed by user command handler, false otherwise
 *
 * @see EnterTimeState_handleUserCommand()
 * @see NormalState_handleUserCommand()
 * @see DemoState_handleUserCommand()
 * @see PulseState_handleUserCommand()
 * @see SetOnOffTimeState_handleUserCommand()
 * @see AutoHueState_handleUserCommand()
 */
static bool UserState_HandleUserCommand(menu_state_t state, user_command_t command)
{

    bool handled = false;

    if (MS_enterTime == state) {

        handled = EnterTimeState_handleUserCommand(command);

    } else if (MS_normalMode == state) {

        handled = NormalState_handleUserCommand(command);

    } else if (MS_demoMode == state) {

        handled = DemoState_handleUserCommand(command);

    } else if (MS_pulse == state) {

        handled = PulseState_handleUserCommand(command);

    } else if (MS_setOnOffTime == state) {

        handled = SetOnOffTimeState_handleUserCommand(command);

    #if (ENABLE_RGB_SUPPORT == 1)

        } else if (MS_hueMode == state) {

            handled = AutoHueState_handleUserCommand(command);

    #endif

    }

    return handled;

}

/**
 * @brief Dispatcher routine for leaving a state
 *
 * This will call the appropriate *_leave() function for the given state.
 *
 * @param state The state supposed to be left
 *
 * @see PulseState_leave()
 * @see DemoState_leave()
 */
static void UserState_LeaveState(menu_state_t state)
{

    if (MS_pulse == state) {

        PulseState_leave();

    } else if (MS_demoMode == state) {

        DemoState_leave();

    }

}

/**
 * @brief Dispatcher routine for 1 Hz events
 *
 * This function is called with a frequency of 1 Hz and will call the correct
 * ISR of the given state.
 *
 * @param state The state the correct ISR should be called for
 *
 * @see TrainIrState_1Hz()
 */
static void UserState_Isr1Hz(menu_state_t state)
{
    if (MS_irTrain == state) {

        TrainIrState_1Hz();

    }

}

/**
 * @brief Dispatcher routine for 10 Hz events
 *
 * This function is called with a frequency of 10 Hz and will call the correct
 * ISR of the given state.
 *
 * @param state The state the correct ISR should be called for
 *
 * @see ShowNumberState_10Hz()
 * @see DemoState_10Hz()
 * @see PulseState_10Hz()
 * @see AutoHueState_10Hz()
 */
static void UserState_Isr10Hz(menu_state_t state)
{

    if (MS_showNumber == state) {

        ShowNumberState_10Hz();

    } else if (MS_demoMode == state) {

        DemoState_10Hz();

    } else if (MS_pulse == state) {

        PulseState_10Hz();

    #if (ENABLE_RGB_SUPPORT == 1)

        } else if (MS_hueMode == state) {

            AutoHueState_10Hz();

    #endif

    }

}

/**
 * @brief Dispatcher routine for 100 Hz events
 *
 * This function is called with a frequency of 100 Hz and will call the correct
 * ISR of the given state.
 *
 * @param state The state the correct ISR should be called for
 *
 * @see PulseState_100Hz()
 */
static void UserState_Isr100Hz(menu_state_t state)
{

    if (MS_pulse == state) {

        PulseState_100Hz();

    }

}

/**
 * @brief Dispatcher routine for 1 kHz events
 *
 * This function is called with a frequency of 1 kHz and will call the correct
 * ISR of the given state.
 *
 * @param state The state the correct ISR should be called for
 *
 * @see DemoState_1000Hz()
 */
static void UserState_Isr1000Hz(menu_state_t state)
{

    if (MS_demoMode == state) {

        DemoState_1000Hz();

    }

}

/**
 * @brief Checks whether the current time can be shown
 *
 * When a mode requires the display itself to give visual feedback to the user,
 * the current time cannot be shown. This returns a boolean value, which
 * indicates whether or not the time can actually be shown for the given state.
 *
 * @param state The state the check should be performed for
 *
 * @return True if the given mode prohibits the time display, false otherwise
 */
static bool UserState_prohibitTimeDisplay(menu_state_t state)
{

    return (MS_irTrain == state) || (MS_showNumber == state)
        || (MS_demoMode == state) || (MS_setSystemTime == state)
        || (MS_setOnOffTime  == state) || (MS_enterTime == state);

}

/**
 * @brief Checks whether the given state can currently be left
 *
 * Some modes can only be left when actually finished correctly. In the mean
 * time the user can't leave the mode. This will return a boolean value
 * indicating whether or not the given state can currently be left.
 *
 * @param state The state the check should be performed for
 *
 * @return True if the current mode prohibits to be left, false otherwise
 *
 * @see EnterTimeState
 * @see SetOnOffTimeState
 * @see SetSystemTimeState
 */
static bool UserState_prohibitLeave(menu_state_t state)
{

    bool prohibit = false;

    if (MS_normalMode == state) {

        prohibit = mode_enterTimeState.prohibitLeave;

    }

    if (MS_setOnOffTime == state) {

        prohibit = mode_setOnOffTimeState.prohibitLeave;

    }

    if (MS_setSystemTime == state) {

        prohibit = mode_setSystemTimeState.prohibitLeave;

    }

    return prohibit;

}
