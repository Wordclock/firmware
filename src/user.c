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

#include <avr/interrupt.h>

#include "main.h"
#include "user.h"
#include "IRMP/irmp.h"
#include "pwm.h"
#include "i2c_rtc.h"
#include "display.h"
#include "dcf77.h"
#include "base.h"
#include "wceeprom.h"
#include "uart.h"
#include "color.h"
#include "ports.h"

#define USER_AMBILIGHT PORTB, 1

#define USER_BLUETOOTH PORTC, 1

#define USER_AUXPOWER PORTD, 2

bool useAutoOffAnimation;

static bool leaveSubState(int8_t indexOfStateToLeave);

static void addState(e_MenuStates mode, const void* param);

static void addSubState(int8_t curState, e_MenuStates mode, const void* param);

static void quitMyself(e_MenuStates state, const void* result);

#define USER_MAX_STATE_DEPTH 10

static uint8_t g_stateStack[USER_MAX_STATE_DEPTH];

static uint8_t g_currentIdxs[MS_COUNT];

static int8_t g_topOfStack;

static uint8_t g_keyDelay;

enum SWITCHED_OFF {

    USO_NORMAL_ON = 0,

    USO_OVERRIDE_ON,

    USO_AUTO_OFF,

    USO_MANUAL_OFF,

};

static uint8_t g_userSwitchedOff;

#if (AMBILIGHT_PRESENT == 1)

    static uint8_t g_settingOfAmbilightBeforeAutoOff;

#endif

static uint8_t g_animPreview = 0;

static uint8_t g_eepromSaveDelay;

static uint8_t g_checkIfAutoOffDelay;

static e_userCommands g_lastPressedKey;

static uint8_t g_keyPressTime;

static datetime_t g_dateTime;

#define g_params (&(wcEeprom_getData()->userParams))

static void dispInternalTime(const datetime_t* i_time, DisplayState blinkmask);

static bool checkActivation();

static bool curTimeIsBetween(uint8_t h1, uint8_t m1, uint8_t h2, uint8_t m2);

#if (LOG_USER_STATE == 1)

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

    #define PRINT_STATE_STACK() do { printStateStack(); } while (0)

#else

    #define PRINT_STATE_STACK()

#endif

#if (LOG_USER_STATE == 1)

    #define log_state(x) uart_puts_P(x)

#else

    #define log_state(x)

#endif

#if (LOG_USER_TIME == 1)

    #define log_time(x) uart_puts_P(x)

    void putTime(const datetime_t* time)
    {

        char txt[8];

        byteToStrLessOneHundred(time->hh, txt);
        uart_puts(txt);
        uart_putc(':');
        byteToStrLessOneHundred(time->mm, txt);
        uart_puts(txt);
        uart_putc('\n');

    }

#else

    #define log_time(x)

    #define putTime(x)

#endif

#include "usermodes.c"

static void addState(e_MenuStates mode, const void* param)
{

    log_state("addstate\n");

    if ((g_topOfStack == 0) || (g_stateStack[g_topOfStack - 1] != mode)) {

        g_stateStack[g_topOfStack] = mode;
        g_currentIdxs[mode] = g_topOfStack;
        ++g_topOfStack;

    }

    UserState_enter(mode, param);

    PRINT_STATE_STACK();

}

static void addSubState(int8_t curState, e_MenuStates mode, const void* param)
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

static bool leaveSubState(int8_t indexOfStateToLeave)
{

    int8_t i;
    bool success = true;

    log_state("leaving substates: ");

    for (i = g_topOfStack - 1; i >= indexOfStateToLeave; --i) {

        bool canLeave = !UserState_prohibitLeave(g_stateStack[i]);
        success = success && canLeave;

        #if (LOG_USER_STATE == 1)

        {

            char buff[5];

            byteToStrLessOneHundred(g_stateStack[i], buff);
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

static void quitMyself(e_MenuStates state, const void* result)
{

    bool success;
    int8_t currentIdx = g_currentIdxs[state];

    log_state("quit self\n");

    success = leaveSubState(currentIdx);

    if (!success) {

        log_state("ERROR: leaving substates failed\n");

    }

    dispInternalTime(&g_dateTime, 0);
    UserState_SubstateFinished(g_stateStack[currentIdx - 1], state, result);

    log_state("quit self finished\n");

}

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

        if (g_stateStack[g_topOfStack - 1] == MS_irTrain) {

            TrainIrState_handleIR(&ir_data);

        } else {

            uint8_t ir_code = 0;

            if (g_params->irAddress != ir_data.address) {

                return;

            }

            while ((ir_code < UI_COMMAND_COUNT)
                    && (g_params->irCommandCodes[ir_code] != ir_data.command)) {

                ++ir_code;

            }

            if (UI_ONOFF == ir_code) {

                log_state("OF\n");

                if (g_userSwitchedOff < USO_AUTO_OFF) {

                    g_userSwitchedOff = USO_MANUAL_OFF;
                    pwm_off();

                } else {

                    if (g_userSwitchedOff == USO_MANUAL_OFF) {

                        g_userSwitchedOff = USO_NORMAL_ON;

                    } else {

                        g_userSwitchedOff = USO_OVERRIDE_ON;

                    }

                    pwm_on();
                    user_setNewTime(NULL);

                }

                wcEeprom_writeback(wcEeprom_getData(), sizeof(WcEepromData));

            } else {

                int8_t i;
                uint8_t handled = false;

                for (i = g_topOfStack - 1; i >= 0 && !handled; --i) {

                    handled = handled || UserState_HanbdleIr(g_stateStack[i], ir_code);

                }

                if (!handled) {

                    if (UI_BRIGHTNESS_UP == ir_code) {

                        log_state("B+\n");
                        pwm_step_up_brightness();

                    } else if (UI_BRIGHTNESS_DOWN == ir_code) {

                        log_state("B-\n");
                        pwm_step_down_brightness();

                    } else if (UI_NORMAL_MODE == ir_code) {

                        addSubState(-1, MS_normalMode, (void*)1);

                    } else if (UI_SET_TIME == ir_code) {

                        addState(MS_setSystemTime, NULL);

                    } else if (UI_SET_ONOFF_TIMES == ir_code) {

                        addState(MS_setOnOffTime, NULL);

                    } else if (UI_DEMO_MODE == ir_code) {

                        e_MenuStates curTop = g_stateStack[g_topOfStack - 1];

                        log_state("BS\n");

                        if (MS_demoMode == curTop) {

                            quitMyself(MS_demoMode, NULL);

                        } else {

                            addState(MS_demoMode, NULL);

                        }

                    } else if (UI_CALIB_BRIGHTNESS == ir_code) {

                        pwm_modifyLdrBrightness2pwmStep();

                    } else if (UI_PULSE_MODE == ir_code) {

                        e_MenuStates curTop = g_stateStack[g_topOfStack - 1];

                        log_state("PLS\n");

                        if (MS_pulse == curTop) {

                            leaveSubState(g_topOfStack - 1);

                        } else {

                            if ((MS_normalMode == curTop)
                            #if (MONO_COLOR_CLOCK != 1)
                                || (MS_hueMode == curTop)
                            #endif
                            ) {

                                addState(MS_pulse, NULL);

                            }

                        }

                        DISPLAY_SPECIAL_USER_COMMANDS_HANDLER

                        #if (MONO_COLOR_CLOCK != 1)

                            } else if (UI_HUE_MODE == ir_code) {

                                log_state("HM");

                                addSubState(-1, MS_hueMode, NULL);

                        #endif

                        #if (DCF_PRESENT == 1)

                            } else if (UI_DCF_GET_TIME == ir_code) {

                                log_state("DCF\n");

                                dcf77_enable();

                        #endif

                        #if (AMBILIGHT_PRESENT == 1)

                            } else if (UI_AMBIENT_LIGHT == ir_code) {

                                log_state("AL\n");

                                PIN(USER_AMBILIGHT) |= _BV(BIT(USER_AMBILIGHT));

                        #endif

                        #if (BLUETOOTH_PRESENT == 1)

                            } else if (UI_BLUETOOTH == ir_code) {

                                log_state("BT\n");

                                PIN(USER_BLUETOOTH) |= _BV(BIT(USER_BLUETOOTH));

                        #endif

                        #if (AUXPOWER_PRESENT == 1)

                            } else if (UI_AUXPOWER == ir_code) {

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

    } else {

        g_lastPressedKey = UI_COMMAND_COUNT;

    }

}

void user_init()
{

    UserState_init();
    addState(g_params->mode & 0x7f, 0);

    if (g_params->mode & 0x80) {

        addState(MS_pulse, NULL);

    }

    addState(MS_irTrain, NULL);

    #if (AMBILIGHT_PRESENT == 1)

        DDR(USER_AMBILIGHT) |= _BV(BIT(USER_AMBILIGHT));

    #endif

    #if (BLUETOOTH_PRESENT == 1)

        DDR(USER_BLUETOOTH) |= _BV(BIT(USER_BLUETOOTH));

    #endif

    #if (AUXPOWER_PRESENT == 1)

        DDR(USER_AUXPOWER) |= _BV(BIT(USER_AUXPOWER));

    #endif

}

static void dispInternalTime(const datetime_t* i_time, DisplayState blinkmask)
{

    putTime(i_time);
    display_setDisplayState(display_getTimeState(i_time), blinkmask);

}

extern void user_setNewTime(const datetime_t* i_time)
{

    if (i_time) {

        log_time("saved Time ");

        g_dateTime = *i_time;

        if ((g_checkIfAutoOffDelay >= USER_DELAY_CHECK_IF_AUTO_OFF_REACHED_S)) {

            if (checkActivation()) {

                if (g_userSwitchedOff != USO_MANUAL_OFF) {

                    #if (AMBILIGHT_PRESENT == 1)

                        if (g_userSwitchedOff == USO_AUTO_OFF) {

                            PORT(USER_AMBILIGHT) |= g_settingOfAmbilightBeforeAutoOff;

                        }

                    #endif

                    g_userSwitchedOff = USO_NORMAL_ON;
                    pwm_on();

                }

            } else {

                if (g_userSwitchedOff == USO_NORMAL_ON) {

                    g_userSwitchedOff = USO_AUTO_OFF;

                    #if (AMBILIGHT_PRESENT == 1)

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

    if (!UserState_prohibitTimeDisplay(g_stateStack[g_topOfStack - 1])
            && (g_userSwitchedOff != USO_AUTO_OFF)) {

        log_time("disp Time ");

        putTime(&g_dateTime);
        display_fadeNewTime(&g_dateTime);

        log_time("\n");

    }

}

/**
 * @see INTERRUPT_1000HZ
 */
void user_isr1000Hz()
{

    UserState_Isr1000Hz(g_stateStack[g_topOfStack - 1]);

}

/**
 * @see INTERRUPT_100HZ
 */
void user_isr100Hz()
{

    UserState_Isr100Hz(g_stateStack[g_topOfStack - 1]);

}

/**
 * @see INTERRUPT_10HZ
 */
void user_isr10Hz()
{

    UserState_Isr10Hz(g_stateStack[g_topOfStack - 1]);

    if (g_keyDelay != 0) {

        --g_keyDelay;

    }

    ++g_keyPressTime;

}

/**
 * @see INTERRUPT_1HZ
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

    #if (USER_AUTOSAVE == 1)

        if (g_eepromSaveDelay == USER_DELAY_BEFORE_SAVE_EEPROM_S) {

            wcEeprom_writeback(wcEeprom_getData(), sizeof(WcEepromData));

        }

    #endif

    if ((g_userSwitchedOff != USO_AUTO_OFF) && (!g_animPreview)) {

        UserState_Isr1Hz(g_stateStack[g_topOfStack - 1]);

    } else {

        if (g_params->useAutoOffAnimation) {

            display_autoOffAnimStep1Hz(g_animPreview);
            useAutoOffAnimation = true;

        }

    }

}

static bool curTimeIsBetween(uint8_t h1, uint8_t m1, uint8_t h2, uint8_t m2)
{

    uint8_t h = g_dateTime.hh;
    uint8_t m = g_dateTime.mm;

    uint8_t largert1 = (h > h1) || ((h == h1) && (m >= m1));
    uint8_t lesst2 = (h < h2) || ((h == h2) && (m < m2));

    if (h2 < h1) {

        return  largert1 || lesst2;

    } else {

        return  largert1 && lesst2;

    }

}

static bool checkActivation()
{

    bool result = true;
    uint8_t i;

    for (i = 0; i < UI_AUTOOFFTIMES_COUNT; i += 2) {

        if (curTimeIsBetween(g_params->autoOffTimes[i].h, g_params->autoOffTimes[i].m,
                g_params->autoOffTimes[i + 1].h, g_params->autoOffTimes[i + 1].m)) {

            result = false;

        }

    }

    return result;

}
