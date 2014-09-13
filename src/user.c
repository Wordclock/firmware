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
 * @file user.c
 * @brief Contains the implementation of the user interface
 *
 * This - among other things - implements the functions defined in user.h.
 * Within this file itself mainly generic things (like the stack management
 * and/or the different power states) are defined and implemented. The specific
 * modes with their implementation on the other hand are currently implemented
 * within usermodes.c, which is simply being included into this file.
 *
 * @see user.h
 * @see usermodes.c
 */

#include "config.h"
#include "user.h"
#include "IRMP/irmp.h"
#include "pwm.h"
#include "display.h"
#include "dcf77.h"
#include "base.h"
#include "preferences.h"
#include "uart.h"
#include "color.h"
#include "ports.h"

/**
 * @brief Port and pin definition of the line in control of the Ambilight
 *
 * @see ports.h
 */
#define USER_AMBILIGHT PORTB, 1

/**
 * @brief Port and pin definition of the line in control of the Bluetooth
 *
 * @see ports.h
 */
#define USER_BLUETOOTH PORTC, 1

/**
 * @brief Port and pin definition of the line in control of the auxiliary GPO
 *
 * @see ports.h
 */
#define USER_AUXPOWER PORTD, 2

/**
 * @brief Indicates whether the autoOff animation is currently being enabled
 *
 * @see POWER_STATES::UPS_AUTO_OFF
 */
bool useAutoOffAnimation;

/**
 * @brief Depth of the state stack
 *
 * This defines the depth of the stack, which effectively puts a limit on how
 * many states (menu_state_t) may be entered on top of each other. However each
 * state can be entered only once, so it doesn't make sense to make this value
 * too big. The default value is 10.
 *
 * @see g_stateStack
 */
#define USER_MAX_STATE_DEPTH 10

/**
 * @brief Buffer for the stack
 *
 * This represents the buffer that will be used for the stack itself, which
 * contains all modes currently being active. The stack is "addressed" from the
 * bottom up, so indexing starts from 0 and will grow towards
 * USER_MAX_STATE_DEPTH - 1.
 *
 * Each mode of menu_state_t can only be entered once and will therefore be put
 * onto the stack only once.
 *
 * @see USER_MAX_STATE_DEPTH
 * @see g_topOfStack
 * @see g_currentIdxs
 */
static menu_state_t g_stateStack[USER_MAX_STATE_DEPTH];

/**
 * @brief Contains the current index for each currently active mode
 *
 * This contains the current index number of the stack for each state currently
 * being active. As each mode can only be entered once, there is only one index
 * for each mode, which will be updated when the mode is entered. However it
 * won't be cleared when the mode is left at a later stage, so it may contain
 * values, which are not correct, in these cases.
 *
 * The mapping is done corresponding to the indexing of menu_state_t itself,
 * e.g. the current index for MS_normalMode will be stored at
 * g_currentIdxs[MS_normalMode].
 *
 * @see g_stateStack
 * @see menu_state_t
 * @see addState()
 */
static uint8_t g_currentIdxs[MS_COUNT];

/**
 * @brief Points to the current top of the stack
 *
 * This is needed to manage the stack and can be understood as the stack
 * pointer. It will always point towards the next free "element" of the stack
 * and will be increased when adding a new state to the stack, and decreased
 * when leaving it again.
 *
 * @see g_stateStack
 */
static int8_t g_topOfStack;

/**
 * @brief Counter used to implement a recognition delay after pressing a key
 *
 * This is used to implement the delay in recognition between two key presses.
 * This counter will be assigned the value of USER_KEY_PRESS_DELAY_100MS.
 * handle_ir_code() won't process any other key press until this counter
 * reaches zero. The decrementation itself is done in user_isr10Hz().
 *
 * @see USER_KEY_PRESS_DELAY_100MS
 * @see handle_ir_code()
 * @see user_isr10Hz()
 */
static uint8_t g_keyDelay;

/**
 * @brief Enumeration of the different "off" states
 *
 * These are the different kinds of "power" states the Wordclock can be
 * assigned to when turning off the display. These are mainly needed to
 * implement the autoOff feature while also allowing the user to turn out the
 * display manually. Although the outcome may seem equal to the outside, these
 * states are handled quite different internally.
 *
 * The current "power" state is hold by g_powerState and at any given point in
 * time will contain one of these values:
 *
 *  - normal_on (POWER_STATES::UPS_NORMAL_ON)
 *  - auto_off (POWER_STATES::UPS_AUTO_OFF)
 *  - user_off (POWER_STATES::UPS_MANUAL_OFF)
 *  - override_on (POWER_STATES::UPS_OVERRIDE_ON)
 *
 * The following "inputs" are possible for each of this states:
 *
 *  - onTime
 *  - offTime
 *  - IROnOff
 *
 * The following table describes which state can be reached from any given
 * state and an arbitrary input:
 *
 * \code
 *    Z           E            Z'
 * normal_on    onTime      normal_on
 * normal_on    offTime     auto_off
 * normal_on    IROnOff     user_off
 * auto_off     onTime      normal_on
 * auto_off     offTime     auto_off
 * auto_off     IROnOff     override_on
 * user_off     onTime      user_off
 * user_off     offTime     user_off
 * user_off     IROnOff     normal_on
 * override_on  onTime      normal_on
 * override_on  offTime     override_on
 * override_on  IROnOff     user_off
 * \endcode
 *
 * @note Be careful with changes and/or adaptations to the ordering of these
 * items, as some functions rely on it.
 *
 * @see g_powerState
 * @see handle_ir_code()
 * @see user_setNewTime()
 * @see user_isr1Hz()
 */
enum POWER_STATES {

    /**
     * @brief Represents the state when the display is turned on normally
     *
     * This is the value for g_powerState when the display is turned on
     * normally. This is mainly needed to differentiate against the various
     * forms of "off" modes.
     */
    UPS_NORMAL_ON = 0,

    /**
     * @brief Represents the state when the display is "forced" to stay on
     *
     * When the display was turned off by the autoOff feature, but was enabled
     * by the user manually afterwards, this is the value that will be assigned
     * to g_powerState.
     */
    UPS_OVERRIDE_ON,

    /**
     * @brief Represents the state when the display was turned off by autoOff
     *
     * This will be assigned to g_powerState whenever the display is turned off
     * automatically by the autoOff feature.
     *
     * It is possible to activate an animation during this "power" state, which
     * will indicate to the user, which mode the Wordclock is currently in.
     * The animation itself consists of blinking minute LEDs, which move over
     * from one corner to the next one. This is controlled by the value
     * of g_animPreview.
     *
     * @see g_animPreview
     */
    UPS_AUTO_OFF,

    /**
     * @brief Represents the state when the display was turned off manually
     *
     * This will be assigned to g_powerState whenever the display was turned
     * off manually by the user.
     */
    UPS_MANUAL_OFF,

};

/**
 * @brief Holds the current "power" state
 *
 * This contains a value of POWER_STATES and determines the current "power"
 * state. Based on this, other states can be reached - depending upon the
 * "input".
 *
 * @see POWER_STATES
 */
static uint8_t g_powerState;

#if (ENABLE_AMBILIGHT_SUPPORT == 1)

    /**
     * @brief Holds the state of the Ambilight before autoOff
     *
     * This holds the state of the Ambilight before the display was turned off
     * by the autoOff feature. It makes it possible to enable the Ambilight
     * after the display gets enabled again - if necessary.
     *
     * @see USER_AMBILIGHT
     */
    static uint8_t g_settingOfAmbilightBeforeAutoOff;

#endif

/**
 * @brief Indicates whether animation preview of autoOff should be activated
 *
 * This indicates whether the animation preview of the autoOff feature should
 * be turned on and/or off. The animation itself is implemented by
 * display_autoOffAnimStep1Hz(), where this variable will simply be passed on
 * as a parameter.
 *
 * @see POWER_STATES::UPS_AUTO_OFF
 * @see display_autoOffAnimStep1Hz()
 */
static bool g_animPreview = false;

/**
 * @brief Counter implementing the delay before saving to EEPROM
 *
 * This counter is used to implement the delay defined in
 * USER_DELAY_BEFORE_SAVE_EEPROM_S. The value itself is reset within
 * handle_user_command() once a new command has been received. It is increased
 * once a second within user_isr1Hz(). Once it reaches its threshold the
 * writeback to the EEPROM is initiated.
 *
 * @see handle_user_command()
 * @see USER_DELAY_BEFORE_SAVE_EEPROM_S
 * @see user_isr1Hz()
 * @see preferences.h
 */
static uint8_t g_eepromSaveDelay;

/**
 * @brief Counter implementing the delay before checking whether to autoOff
 *
 * This counter is used to implement the delay defined in
 * USER_DELAY_CHECK_IF_AUTO_OFF_REACHED_S. The value itself is reset within
 * handle_ir_code() once a new command has been received. It is increased
 * once a second within user_isr1Hz(). Once it reaches its threshold a check
 * for the autoOff feature is initiated and processed appropriately.
 *
 * @see handle_ir_code()
 * @see USER_DELAY_CHECK_IF_AUTO_OFF_REACHED_S
 * @see user_isr1Hz()
 * @see user_setNewTime()
 * @see checkActivation()
 */
static uint8_t g_checkIfAutoOffDelay;

/**
 * @brief Allowing access to global instance of userParams backed by EEPROM
 *
 * @see user_prefs_t
 */
#define g_params (&(preferences_get()->userParams))

static void dispInternalTime(const datetime_t* i_time, display_state_t blinkmask);

static bool checkActivation();

static bool curTimeIsBetween(uint8_t h1, uint8_t m1, uint8_t h2, uint8_t m2);

#if (LOG_USER_STATE == 1)

    /**
     * @brief Outputs the current state of the stack
     *
     * This outputs the current state of the stack using UART. It is used
     * within PRINT_STATE_STACK() and will only be compiled when LOG_USER_STATE
     * == 1.
     *
     * @see PRINT_STATE_STACK()
     */
    static void printStateStack()
    {

        uint8_t i=0;
        uart_puts_P("stack: [");

        for (; i < g_topOfStack; ++i) {

            uart_putc(g_stateStack[i] + '0');
            uart_putc(' ');

        }

        for (; i < USER_MAX_STATE_DEPTH; ++i) {

            uart_putc('_');
            uart_putc(' ');

        }

        uart_putc(']');
        uart_puts_P("top: ");
        uart_putc(g_topOfStack + '0');
        uart_putc('\n');

    }

    /**
     * @brief Wrapper macro for printStateStack()
     *
     * This macro basically makes use of printStateStack() whenever
     * LOG_USER_STATE == 1.
     *
     * @see printStateStack()
     */
    #define PRINT_STATE_STACK() do { printStateStack(); } while (0)

#else

    /**
     * @brief Dummy macro in case logging is disabled
     *
     * When LOG_USER_STATE == 0 this makes sure that nothing is actually added
     * to the code. This makes it possible to add PRINT_STATE_STACK() within
     * various functions without worrying whether or not the debugging is
     * actually enabled.
     */
    #define PRINT_STATE_STACK()

#endif

#if (LOG_USER_STATE == 1)

    /**
     * @brief Macro to output constant strings from program memory
     *
     * This is used within various functions of this module to output constant
     * strings from program memory regarding the state of the module itself
     * using UART.
     *
     * @see uart_puts_P()
     */
    #define log_state(x) uart_puts_P(x)

#else

    /**
     * @brief Dummy macro in case logging is disabled
     *
     * When LOG_USER_STATE == 0 this makes sure that nothing is actually added
     * to the code. This makes it possible to add log_state() within various
     * functions without worrying whether or not the debugging is actually
     * enabled.
     */
    #define log_state(x)

#endif

#if (LOG_USER_TIME == 1)

    /**
     * @brief Macro to output constant strings from program memory
     *
     * This is used within various functions of this module to output constant
     * strings from program memory regarding the time of the module itself
     * using UART.
     *
     * @see g_dateTime
     * @see user_setNewTime()
     * @see uart_puts_P()
     */
    #define log_time(x) uart_puts_P(x)

    /**
     * @brief Outputs the given time
     *
     * This can be used to output the hours and minutes of the given time
     * using UART. Other attributes of datetime_t will simply be ignored.
     *
     * @see datetime_t
     * @see uint8ToStrLessOneHundred()
     */
    void putTime(const datetime_t* time)
    {

        char txt[8];

        uint8ToStrLessOneHundred(time->hh, txt);
        uart_puts(txt);
        uart_putc(':');
        uint8ToStrLessOneHundred(time->mm, txt);
        uart_puts(txt);
        uart_putc('\n');

    }

#else

    /**
     * @brief Dummy macro in case logging is disabled
     *
     * When LOG_USER_TIME == 0 this makes sure that nothing is actually added
     * to the code. This makes it possible to add log_state() within various
     * functions without worrying whether or not the debugging is actually
     * enabled.
     */
    #define log_time(x)

    /**
     * @brief Dummy macro in case logging is disabled
     *
     * When LOG_USER_TIME == 0 this makes sure that nothing is actually added
     * to the code. This makes it possible to add log_state() within various
     * functions without worrying whether or not the debugging is actually
     * enabled.
     */
    #define putTime(x)

#endif

#include "usermodes.c"

/**
 * @brief Adds a state on top of the stack
 *
 * This adds the given state (mode) to the stack, when the state was not
 * already put there before. It will update all variables dealing with the
 * state of the stack itself.
 *
 * Regardless of whether or not the given state was already put on the stack
 * beforehand, it will execute UserState_enter() with the given parameter.
 *
 * @param mode The state to add
 * @param param Any parameter for the given state, might also be NULL
 *
 * @see g_stateStack
 * @see g_topOfStack
 * @see g_currentIdxs
 * @see UserState_enter()
 */
void addState(menu_state_t mode, const void* param)
{

    log_state("addstate\n");

    if ((g_topOfStack == 0) || (user_get_current_menu_state() != mode)) {

        g_stateStack[g_topOfStack] = mode;
        g_currentIdxs[mode] = g_topOfStack;
        ++g_topOfStack;

    }

    UserState_enter(mode, param);

    PRINT_STATE_STACK();

}

/**
 * @brief Adds a substate to a specific state
 *
 * A substate can be understood as a state, which in a way is related to its
 * "parent", e.g. MS_enterTime can be a substate of MS_setSystemTime.
 *
 * Only a single substate for each parent can be entered at any given point in
 * time. Therefore this function will try to leave any previously entered
 * substates, before actually entering the given substate itself. The added
 * substate will take up place on the actual stack itself - as it is handled
 * quite similar to a normal state.
 *
 * Passing "-1" as value for the first parameter (curState) this function can
 * be used to replace the "base" state itself (the state at the very bottom).
 * Every state on top of it will then be left - if possible.
 *
 * @param curState The parent of the substate that should be added, -1 for base
 * @param mode The substate that should be added
 * @param param Any parameter for the given state, might be NULL
 *
 * @see g_currentIdxs
 * @see leaveSubState()
 * @see addState()
 */
void addSubState(int8_t curState, menu_state_t mode, const void* param)
{

    bool success = true;
    int8_t nextIdx;

    if (curState == -1) {

        nextIdx = 0;
        log_state("replace Base state\n");

    } else {

        nextIdx = g_currentIdxs[curState] + 1;

    }

    log_state("addSubstate\n");

    if (nextIdx != g_topOfStack) {

        success = leaveSubState(nextIdx);

    }

    if (success) {

        addState(mode, param);

    } else {

        log_state("ERROR: leaving substates failed\n");

    }

}

/**
 * @brief Leaves the state at the given stack position
 *
 * First of all this function will check whether the given state and all states
 * on top of it can actually be left or whether leaving one of them is
 * currently prohibited (UserState_prohibitLeave()). If it is possible to leave
 * all of these states, it will invoke UserState_LeaveState() on each of them
 * and the function will return true. If at least one state can't be left,
 * nothing happens at all and the function simply returns false.
 *
 * @param indexOfStateToLeave Stack position of state to leave
 *
 * @return Indicates whether or not the given state was left
 *
 * @see UserState_prohibitLeave()
 * @see UserState_LeaveState()
 */
bool leaveSubState(int8_t indexOfStateToLeave)
{

    int8_t i;
    bool success = true;

    log_state("leaving substates: ");

    for (i = g_topOfStack - 1; i >= indexOfStateToLeave; --i) {

        bool canLeave = !UserState_prohibitLeave(g_stateStack[i]);
        success &= canLeave;

        #if (LOG_USER_STATE == 1)

        {

            char buff[5];

            uint8ToStrLessOneHundred(g_stateStack[i], buff);
            uart_puts(buff);
            uart_putc(':');
            uart_putc(canLeave ? 'y' : 'n');
            uart_puts_P(", ");

        }

        #endif

    }

    if (success) {

        for (i = g_topOfStack - 1; i >= indexOfStateToLeave; --i) {

            UserState_LeaveState(g_stateStack[i]);
            --g_topOfStack;

            PRINT_STATE_STACK();

        }

    }

    return success;

}

/**
 * @brief Makes the given state to quit itself
 *
 * This will invoke leaveSubState() on the current index of the given mode
 * (g_currentIdxs). It will then reset the display state, and invoke
 * UserState_SubstateFinished() on the "parent" of the state just left.
 *
 * This is expected to be used on states that have fulfilled their actual task,
 * and want to hand over control to the "parent" again.
 *
 * @param state The state that should leave itself
 * @param result Result to be handed over to the "parent", might be NULL
 *
 * @see leaveSubState()
 * @see g_currentIdxs
 * @see UserState_SubstateFinished()
 */
void quitMyself(menu_state_t state, const void* result)
{

    bool success;
    int8_t currentIdx = g_currentIdxs[state];

    log_state("quit self\n");

    success = leaveSubState(currentIdx);

    if (!success) {

        log_state("ERROR: leaving substates failed\n");

    }

    dispInternalTime(datetime_get(), 0);
    UserState_SubstateFinished(g_stateStack[currentIdx - 1], state, result);

    log_state("quit self finished\n");

}

/**
 * @brief Returns the currently active menu state
 *
 * @return Currently active menu state
 *
 * @see g_stateStack
 * @see g_topOfStack
 * @see menu_state_t
 */
menu_state_t user_get_current_menu_state()
{

    return g_stateStack[g_topOfStack - 1];

}

/**
 * @brief Handles the given user command
 *
 * This handles the given user command (user_command_t) either by processing
 * it directly, or by passing it over to the actual handler using
 * UserState_HandleUserCommand().
 *
 * g_eepromSaveDelay and g_checkIfAutoOffDelay get reset every time this
 * function is called to make sure the appropriate functionality works as
 * intended.
 *
 * @param user_command The user command that should be handled
 *
 * @see UserState_HandleUserCommand()
 * @see g_eepromSaveDelay
 * @see g_checkIfAutoOffDelay
 */
void handle_user_command(user_command_t user_command)
{

    if (UC_ONOFF == user_command) {

        log_state("OF\n");

        if (g_powerState < UPS_AUTO_OFF) {

            g_powerState = UPS_MANUAL_OFF;
            pwm_off();

        } else {

            if (g_powerState == UPS_MANUAL_OFF) {

                g_powerState = UPS_NORMAL_ON;

            } else {

                g_powerState = UPS_OVERRIDE_ON;

            }

            pwm_on();
            user_setNewTime(NULL);

        }

        preferences_save(preferences_get(), sizeof(prefs_t));

    } else {

        int8_t i;
        bool handled = false;

        for (i = g_topOfStack - 1; i >= 0 && !handled; --i) {

            handled |= UserState_HandleUserCommand(g_stateStack[i], user_command);

        }

        if (!handled) {

            if (UC_BRIGHTNESS_UP == user_command) {

                log_state("B+\n");
                pwm_increase_brightness();

            } else if (UC_BRIGHTNESS_DOWN == user_command) {

                log_state("B-\n");
                pwm_decrease_brightness();

            } else if (UC_NORMAL_MODE == user_command) {

                addSubState(-1, MS_normalMode, (void*)1);

            } else if (UC_SET_TIME == user_command) {

                addState(MS_setSystemTime, NULL);

            } else if (UC_SET_ONOFF_TIMES == user_command) {

                addState(MS_setOnOffTime, NULL);

            } else if (UC_DEMO_MODE == user_command) {

                menu_state_t curTop = user_get_current_menu_state();

                log_state("BS\n");

                if (MS_demoMode == curTop) {

                    quitMyself(MS_demoMode, NULL);

                } else {

                    addState(MS_demoMode, NULL);

                }

            } else if (UC_CALIB_BRIGHTNESS == user_command) {

                pwm_modifyLdrBrightness2pwmStep();

                // Indicate the change to user
                if (pwm_is_enabled()) {

                    pwm_off();
                    _delay_ms(USER_VISUAL_INDICATION_TOGGLE_MS);
                    pwm_on();

                }

            } else if (UC_PULSE_MODE == user_command) {

                menu_state_t curTop = user_get_current_menu_state();

                log_state("PLS\n");

                if (MS_pulse == curTop) {

                    leaveSubState(g_topOfStack - 1);

                } else {

                    if ((MS_normalMode == curTop)
                    #if (ENABLE_RGB_SUPPORT == 1)
                        || (MS_hueMode == curTop)
                    #endif
                    ) {

                        addState(MS_pulse, NULL);

                    }

                }

                DISPLAY_SPECIAL_USER_COMMANDS_HANDLER

                #if (ENABLE_RGB_SUPPORT == 1)

                    } else if (UC_HUE_MODE == user_command) {

                        log_state("HM");

                        addSubState(-1, MS_hueMode, NULL);

                #endif

                #if (ENABLE_DCF_SUPPORT == 1)

                    } else if (UC_DCF_GET_TIME == user_command) {

                        log_state("DCF\n");

                        dcf77_enable();

                #endif

                #if (ENABLE_AMBILIGHT_SUPPORT == 1)

                    } else if (UC_AMBILIGHT == user_command) {

                        log_state("AL\n");

                        PIN(USER_AMBILIGHT) |= _BV(BIT(USER_AMBILIGHT));

                #endif

                #if (ENABLE_BLUETOOTH_SUPPORT == 1)

                    } else if (UC_BLUETOOTH == user_command) {

                        log_state("BT\n");

                        PIN(USER_BLUETOOTH) |= _BV(BIT(USER_BLUETOOTH));

                #endif

                #if (ENABLE_AUXPOWER_SUPPORT == 1)

                    } else if (UC_AUXPOWER == user_command) {

                        log_state("AUX\n");

                        PIN(USER_AUXPOWER) |= _BV(BIT(USER_AUXPOWER));

                #endif

                    } else {

                        return;

                    }

        }

    }

    g_params->mode = g_stateStack[0];

    if (MS_pulse == g_stateStack[1]) {

        g_params->mode |= 0x80;

    }

    g_eepromSaveDelay = 0;
    g_checkIfAutoOffDelay = 0;

}

/**
 * @brief Processes any received IR commands
 *
 * This function handles any IR commands received by IRMP. It will check
 * whether a command was decoded successfully using irmp_get_data() and
 * implements a key press delay before any other key press can be recognized.
 *
 * If currently in training state it will dispatch the handling to
 * TrainIrState_handleIR(). Otherwise it will find the correct user command
 * (user_command_t) for the received code and pass it to handle_user_command().
 *
 * @note To make sure not to loose any events, this function should be called
 * on a quasi-regular basis.
 *
 * @see irmp_get_data()
 * @see g_keyDelay
 * @see TrainIrState_handleIR()
 * @see handle_user_command()
 */
void handle_ir_code()
{

    IRMP_DATA ir_data;

    if (irmp_get_data(&ir_data)) {

        if (g_keyDelay) {

            return;

        }

        #if (LOG_USER_IR_CMD == 1)

        {

            char text[20];

            uart_puts_P("IR-cmd: ");
            uint16ToHexStr(ir_data.protocol, text);
            uart_puts(text);
            uint16ToHexStr(ir_data.address, text);
            uart_puts(text);
            uint16ToHexStr(ir_data.command, text);
            uart_puts(text);
            uart_putc('\n');

        }

        #endif

        g_keyDelay = USER_KEY_PRESS_DELAY_100MS;

        if (user_get_current_menu_state() == MS_irTrain) {

            TrainIrState_handleIR(&ir_data);

        } else {

            uint8_t ir_code = 0;

            if (g_params->irAddress != ir_data.address) {

                return;

            }

            while ((ir_code < UC_COMMAND_COUNT)
                && (g_params->irCommandCodes[ir_code] != ir_data.command)) {

                ++ir_code;

            }

            handle_user_command((user_command_t)ir_code);

        }

    }

}

/**
 * @brief Initializes the user module
 *
 * This initializes the user module: First of all it calls UserState_init(),
 * which will initialize all the user modes implemented within usermodes.c.
 * Afterwards it will restore the mode previously stored. If no mode was
 * stored, it will fallback to the default values. Furthermore this function
 * also sets up the data direction registers of the lines in control of the
 * Ambilight, Bluetooth and/or auxiliary GPO lines.
 *
 * @note This has to be executed after the EEPROM module (preferences_init()) has
 * been initialized, as it accesses data provided by the EEPROM module.
 *
 * @see UserState_init()
 * @see g_params
 * @see menu_state_t::MS_irTrain
 * @see USER_AMBILIGHT
 * @see USER_BLUETOOTH
 * @see ENABLE_AUXPOWER_SUPPORT
 */
void user_init()
{

    UserState_init();
    addState(g_params->mode & 0x7f, 0);

    if (g_params->mode & 0x80) {

        addState(MS_pulse, NULL);

    }

    addState(MS_irTrain, NULL);

    #if (ENABLE_AMBILIGHT_SUPPORT == 1)

        DDR(USER_AMBILIGHT) |= _BV(BIT(USER_AMBILIGHT));

    #endif

    #if (ENABLE_BLUETOOTH_SUPPORT == 1)

        DDR(USER_BLUETOOTH) |= _BV(BIT(USER_BLUETOOTH));

    #endif

    #if (ENABLE_AUXPOWER_SUPPORT == 1)

        DDR(USER_AUXPOWER) |= _BV(BIT(USER_AUXPOWER));

    #endif

}

/**
 * @brief Outputs the given datetime to the display
 *
 * This will output the given time to the display by retrieving the appropriate
 * display state for the given time using display_getTimeState() and outputting
 * it using display_setDisplayState(). It will also pass the given blinkmask
 * to display_setDisplayState(), so it is possible to let some of the words
 * blink.
 *
 * @param i_time The new date and time to set
 * @param blinkmask Defines words that should be blinking
 *
 * @see display_setDisplayState()
 * @see display_getTimeState()
 */
static void dispInternalTime(const datetime_t* i_time, display_state_t blinkmask)
{

    putTime(i_time);
    display_setDisplayState(display_getTimeState(i_time), blinkmask);

}

/**
 * @brief Takes over the given time internally
 *
 * This will take over the given time internally (g_dateTime). It then checks
 * whether enough time (g_checkIfAutoOffDelay) has passed to check whether the
 * autoOff times should be consulted to decide whether the display should be
 * enabled and/or disabled by the autoOff feature. It will then turn the
 * display on and/or off if neccessary - depending upon the result of the
 * check.
 *
 * @param i_time The new date and time to set
 *
 * @see g_dateTime
 * @see g_checkIfAutoOffDelay
 * @see USER_DELAY_CHECK_IF_AUTO_OFF_REACHED_S
 * @see checkActivation()
 */
void user_setNewTime(const datetime_t* i_time)
{

    if (i_time) {

        log_time("saved Time ");

        if ((g_checkIfAutoOffDelay >= USER_DELAY_CHECK_IF_AUTO_OFF_REACHED_S)) {

            if (checkActivation()) {

                if (g_powerState != UPS_MANUAL_OFF) {

                    #if (ENABLE_AMBILIGHT_SUPPORT == 1)

                        if (g_powerState == UPS_AUTO_OFF) {

                            PORT(USER_AMBILIGHT) |= g_settingOfAmbilightBeforeAutoOff;

                        }

                    #endif

                    g_powerState = UPS_NORMAL_ON;
                    pwm_on();

                }

            } else {

                if (g_powerState == UPS_NORMAL_ON) {

                    g_powerState = UPS_AUTO_OFF;

                    #if (ENABLE_AMBILIGHT_SUPPORT == 1)

                        g_settingOfAmbilightBeforeAutoOff = PORT(USER_AMBILIGHT) & _BV(BIT(USER_AMBILIGHT));
                        PORT(USER_AMBILIGHT) &= ~_BV(BIT(USER_AMBILIGHT));

                    #endif

                    if (!g_params->useAutoOffAnimation) {

                        pwm_off();

                    }

                }

            }

        }

    }

    if (!UserState_prohibitTimeDisplay(user_get_current_menu_state())
            && (g_powerState != UPS_AUTO_OFF)) {

        log_time("disp Time ");

        putTime(datetime_get());
        display_fadeNewTime(datetime_get());

        log_time("\n");

    }

}

/**
 * @brief "ISR" executed with a frequency of 1000 Hz
 *
 * This "ISR" will be executed a thousand times each second (INTERRUPT_1000HZ).
 * It is responsible for calling UserState_Isr1000Hz() with the current state
 * as parameter.
 *
 * @see INTERRUPT_1000HZ
 * @see UserState_Isr1000Hz()
 */
void user_isr1000Hz()
{

    UserState_Isr1000Hz(user_get_current_menu_state());

}

/**
 * @brief "ISR" executed with a frequency of 100 Hz
 *
 * This "ISR" will be executed a hundred times each second (INTERRUPT_100HZ).
 * It is responsible for calling UserState_Isr100Hz() with the current state as
 * parameter.
 *
 * @see INTERRUPT_100HZ
 * @see UserState_Isr100Hz()
 */
void user_isr100Hz()
{

    UserState_Isr100Hz(user_get_current_menu_state());

}

/**
 * @brief "ISR" executed with a frequency of 10 Hz
 *
 * This "ISR" will be executed ten times each second (INTERRUPT_10HZ). It is
 * responsible for calling UserState_Isr10Hz() with the current state as
 * parameter and decreases the delay counter for key press recognition
 * (g_keyDelay).
 *
 * @see INTERRUPT_10HZ
 * @see UserState_Isr10Hz()
 * @see g_keyDelay
 */
void user_isr10Hz()
{

    UserState_Isr10Hz(user_get_current_menu_state());

    if (g_keyDelay != 0) {

        --g_keyDelay;

    }

}

/**
 * @brief "ISR" executed with a frequency of 1 Hz
 *
 * This "ISR" will be executed once a second (INTERRUPT_1HZ) and contains some
 * task, which are executed comparatively slow. Among other things it increases
 * various delay counters (g_eepromSaveDelay, g_checkIfAutoOffDelay) and
 * will initiate the writeback to the EEPROM once the appropriate delay
 * (g_eepromSaveDelay) has reached its threshold
 * (USER_DELAY_BEFORE_SAVE_EEPROM_S). It will also make sure that either
 * UserState_Isr1Hz() with the current state as parameter and/or
 * display_autoOffAnimStep1Hz() with the current setting of g_animPreview are
 * executed - depending on the current power "state" (g_powerState).
 *
 * @see INTERRUPT_1HZ
 * @see g_eepromSaveDelay
 * @see g_checkIfAutoOffDelay
 * @see g_eepromSaveDelay
 * @see USER_DELAY_BEFORE_SAVE_EEPROM_S
 * @see UserState_Isr1Hz()
 * @see display_autoOffAnimStep1Hz()
 * @see g_animPreview
 * @see g_powerState
 */
void user_isr1Hz()
{

    useAutoOffAnimation = false;

    if (g_eepromSaveDelay <= USER_DELAY_BEFORE_SAVE_EEPROM_S) {

        ++g_eepromSaveDelay;

    }

    if (g_checkIfAutoOffDelay <= USER_DELAY_CHECK_IF_AUTO_OFF_REACHED_S) {

        ++g_checkIfAutoOffDelay;

    }

    #if (ENABLE_USER_AUTOSAVE == 1)

        if (g_eepromSaveDelay == USER_DELAY_BEFORE_SAVE_EEPROM_S) {

            preferences_save(preferences_get(), sizeof(prefs_t));

        }

    #endif

    if ((g_powerState != UPS_AUTO_OFF) && (!g_animPreview)) {

        UserState_Isr1Hz(user_get_current_menu_state());

    } else {

        if (g_params->useAutoOffAnimation) {

            display_autoOffAnimStep1Hz(g_animPreview);
            useAutoOffAnimation = true;

        }

    }

}

/**
 * @brief Checks whether the current time lies within the given range
 *
 * The range is defined by the four parameters, whereas the first two parameter
 * define the hours and/or minutes of the first boundary, and the last two
 * parameters the hours and/or minutes of the second boundary.
 *
 * @param h1 The hours of the first boundary
 * @param m1 The minutes of the first boundary
 * @param h2 The hours of the second boundary
 * @param m2 The minutes of the second boundary
 *
 * @return True if current time lies in between the given range, else false
 *
 * @see datetime_get()
 */
static bool curTimeIsBetween(uint8_t h1, uint8_t m1, uint8_t h2, uint8_t m2)
{

    datetime_t* datetime = datetime_get();

    uint8_t h = datetime->hh;
    uint8_t m = datetime->mm;

    uint8_t largert1 = (h > h1) || ((h == h1) && (m >= m1));
    uint8_t lesst2 = (h < h2) || ((h == h2) && (m < m2));

    if (h2 < h1) {

        return  largert1 || lesst2;

    } else {

        return  largert1 && lesst2;

    }

}

/**
 * @brief Checks whether display should be activated for autoOff feature
 *
 * This function checks whether the current time lies outside of any defined
 * on/off (autoOff) time range and returns true if it does.
 *
 * @return True if current time lies outside any on/off time ranges, else false
 *
 * @see user_prefs_t::onOffTimes
 * @see curTimeIsBetween()
 * @see user_setNewTime()
 */
static bool checkActivation()
{

    uint8_t i;

    for (i = 0; i < UI_ONOFFTIMES_COUNT; i += 2) {

        if (curTimeIsBetween(g_params->onOffTimes[i].h, g_params->onOffTimes[i].m,
            g_params->onOffTimes[i + 1].h, g_params->onOffTimes[i + 1].m)) {

            return false;

        }

    }

    return true;

}
