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
 * @file usermodes.c
 *
 *  This file contains implementation of the internal states and modes
 *  \details
 *       This file is designed to be included in user.c.
 *       SO DO NOT ADD IT TO PROJECT OR MAKEFILE!
 *
 * \version $Id: usermodes.c 424 2013-03-14 18:51:07Z vt $
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

typedef struct TrainIrState {

    uint8_t seconds;

    uint8_t curKey;

} TrainIrState;

static TrainIrState mode_trainIrState;

static uint8_t TrainIrState_handleIR(const IRMP_DATA* i_irCode);

typedef struct ShowNumberState {

    uint8_t delay100ms;

} ShowNumberState;

static ShowNumberState mode_showNumberState;

#if (MONO_COLOR_CLOCK != 1)

    typedef struct NormalState {

        uint8_t colorToSet;

        Hue_t curHue;

    } NormalState;

    static NormalState mode_normalState;

#endif

typedef struct PulseState {

    uint8_t curBrightness;

    uint8_t delay100ms;

} PulseState;

static PulseState mode_pulseState;

#if (MONO_COLOR_CLOCK != 1)

    typedef struct AutoHueState {

        Hue_t curHue;

        uint8_t delay100ms;

    } AutoHueState;

    static AutoHueState mode_autoHueState;

#endif

typedef struct DemoState {

    uint8_t demoStep;

    uint8_t delay100ms;

    uint8_t fastMode;

} DemoState;

static DemoState mode_demoState;

typedef struct SetSystemTimeState {

    uint8_t prohibitLeave;

} SetSystemTimeState;

static SetSystemTimeState mode_setSystemTimeState;

typedef struct SetOnOffmTimeState {

    uint8_t prohibitLeave;

    uint8_t currentTimeToSet;

} SetOnOffmTimeState;

static SetOnOffmTimeState mode_setOnOffTimeState;

typedef struct EnterTimeState {

    uint8_t prohibitLeave;

    datetime_t time;

    enum e_EnterTimesubStates {

        ETS_hour,

        ETS_minutes,

    } curSubState;

} EnterTimeState;

static EnterTimeState mode_enterTimeState;

static void UserState_init();

static uint8_t UserState_prohibitLeave(e_MenuStates state);

static uint8_t UserState_prohibitTimeDisplay(e_MenuStates state);

static void UserState_Isr1000Hz(e_MenuStates state);

static void UserState_Isr100Hz(e_MenuStates state);

static void UserState_Isr10Hz(e_MenuStates state);

static void UserState_Isr1Hz(e_MenuStates state);

static void UserState_LeaveState(e_MenuStates state);

static uint8_t UserState_HandleIr(e_MenuStates state, uint8_t cmdCode);

static void UserState_SubstateFinished(e_MenuStates state, e_MenuStates finishedState, const void* result);

static void UserState_enter(e_MenuStates state, const void* param);

static void incDecRange(uint8_t* val, int8_t dir, uint8_t min, uint8_t max)
{

    if (dir < 0) {

        if (*val > min) {

            *val += dir;

        }

    } else {

        if (*val < max) {

            *val +=dir;

        }

    }

}

static void incDecRangeOverflow(uint8_t* val, int8_t dir, uint8_t max)
{

    if (dir < 0) {

        if (*val < -dir) {

            *val = max + 1 + dir;

            return;

        }

    }

    *val += dir;

    if (*val > max) {

        *val = 0;

    }

}

#if (LOG_USER_IR_TRAIN == 1)

    #define log_irTrain(x) uart_puts_P(x)

#else

    #define log_irTrain(x)

#endif

static void TrainIrState_enter(const void* param)
{

    log_state("enter train\n");

    display_setDisplayState(display_getIndicatorMask(), display_getIndicatorMask());

}

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

static uint8_t TrainIrState_handleIR(const IRMP_DATA* i_irCode)
{

    DisplayState disp;

    if (mode_trainIrState.curKey > 0) {

        if (g_params->irAddress != i_irCode->address) {

            log_irTrain("invalid ir-address\n");

        } else {

            g_params->irCommandCodes[mode_trainIrState.curKey - 1] = i_irCode->command;

            if (mode_trainIrState.curKey == UI_COMMAND_COUNT) {

                log_irTrain("Ir train finished\n");

                wcEeprom_writeback(g_params, sizeof(*g_params));

                quitMyself(MS_irTrain, NULL);

                return true;

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
        byteToStrLessOneHundred(mode_trainIrState.curKey, buff);
        uart_puts(buff);
        uart_putc('\n');

    }

    #endif

    disp = display_getNumberDisplayState(mode_trainIrState.curKey) | display_getIndicatorMask();
    display_setDisplayState(disp, disp);

    return true;

}

static void TrainIrState_init()
{

}

static void ShowNumberState_10Hz()
{

    --mode_showNumberState.delay100ms;

    if (mode_showNumberState.delay100ms == 0) {

        quitMyself(MS_showNumber, NULL);

    }

}

static void ShowNumberState_enter(const void* param)
{

    DisplayState dispData;
    uint8_t paramSN = (uint8_t)((uint16_t)param);

    log_state("enter showNumber\n");

    mode_showNumberState.delay100ms = USER_NORMAL_SHOW_NUMBER_DELAY_100MS;

    dispData = display_getNumberDisplayState(paramSN);
    display_setDisplayState(dispData, 0);

}

static void ShowNumberState_init()
{

}

static void NormalState_enter(const void* param)
{

    log_state("nm\n");

    #if (MONO_COLOR_CLOCK != 1)

        pwm_set_color_step(g_params->colorPresets[g_params->curColorProfile].r,
            g_params->colorPresets[g_params->curColorProfile].g,
            g_params->colorPresets[g_params->curColorProfile].b);

        if (((uint16_t)param) != 0) {

            addSubState(MS_normalMode, MS_showNumber, (void*)(((uint16_t)g_params->curColorProfile + 1)));

        }

    #else

        dispInternalTime(&g_dateTime, 0);

    #endif
}

static uint8_t NormalState_handleIR(uint8_t cmdCode)
{

    #if (MONO_COLOR_CLOCK != 1)

        if (UI_NORMAL_MODE == cmdCode) {

            ++(g_params->curColorProfile);
            g_params->curColorProfile %=  UI_COLOR_PRESET_COUNT;
            NormalState_enter((void*)1);

        } else if (UI_CHANGE_R == cmdCode) {

            log_state("CR\n");

            mode_normalState.colorToSet = 0;

        } else if (UI_CHANGE_G == cmdCode) {

            log_state("CG\n");

            mode_normalState.colorToSet = 1;

        } else if (UI_CHANGE_B == cmdCode) {

            log_state("CB\n");

            mode_normalState.colorToSet = 2;

        } else if (UI_CHANGE_HUE == cmdCode) {

            log_state("CH\n");

            mode_normalState.colorToSet = 4;

        } else if (UI_UP == cmdCode || UI_DOWN == cmdCode) {

            int8_t dir = (UI_UP == cmdCode) ? 1 : -1;

            log_state("CC\n");

            if (mode_normalState.colorToSet < 4){

                uint8_t* rgb = (uint8_t*)(&g_params->colorPresets[g_params->curColorProfile]);

                incDecRange(&rgb[mode_normalState.colorToSet], dir, 0, MAX_PWM_STEPS - 1);
                pwm_set_color_step(rgb[0], rgb[1], rgb[2]);

            } else {

                uint8_t r, g, b;

                if (dir < 0 && mode_normalState.curHue < COLOR_HUE_MANUAL_STEPS) {

                    mode_normalState.curHue = COLOR_HUE_MAX;

                } else if (dir > 0 && mode_normalState.curHue >= COLOR_HUE_MAX - COLOR_HUE_MANUAL_STEPS) {

                    mode_normalState.curHue = 0;

                } else {

                    mode_normalState.curHue += dir * COLOR_HUE_MANUAL_STEPS;

                }

                color_hue2rgb(mode_normalState.curHue, &r, &g, &b);
                pwm_set_colors(r, g, b);

            }

        } else {

            return false;

        }

        return true;

    #else

        return false;

    #endif

}

static void NormalState_init()
{

}

#if (MONO_COLOR_CLOCK != 1)

    static void AutoHueState_10Hz()
    {

        --mode_autoHueState.delay100ms;

        if (mode_autoHueState.delay100ms >= (volatile uint8_t)(g_params->hueChangeIntervall)) {

            uint8_t r, g, b;

            ++mode_autoHueState.curHue;
            mode_autoHueState.curHue %= (COLOR_HUE_MAX + 1);
            color_hue2rgb(mode_autoHueState.curHue, &r, &g, &b);
            pwm_set_colors(r, g, b);
            mode_autoHueState.delay100ms = g_params->hueChangeIntervall;

        }

    }

    static void AutoHueStat_enter(const void* param)
    {

        AutoHueState_10Hz();

    }

    static uint8_t AutoHueState_handleIR(uint8_t cmdCode)
    {

        if (UI_UP == cmdCode || UI_DOWN == cmdCode) {

            int8_t dir = (UI_UP == cmdCode) ? -1 : 1;

            log_state("CHS \n");

            incDecRange(&g_params->hueChangeIntervall, dir,
                USER_HUE_CHANGE_INT_100MS_MIN, USER_HUE_CHANGE_INT_100MS_MAX);

            #if (LOG_USER_STATE == 1)

                uart_putc('0' + g_params->hueChangeIntervall);

            #endif

        } else {

            return false;

        }

        return true;

    }

    static void AutoHueState_init()
    {

    }

#endif

static void DemoState_1000Hz()
{

    if (!mode_demoState.fastMode) {

        return;

    }

    pwm_lock_brightness_val(255);
    display_setDisplayState((0x01010101L << (mode_demoState.demoStep)), 0);
    ++mode_demoState.demoStep;
    mode_demoState.demoStep %= 8;

}

static void DemoState_10Hz()
{

    if (mode_demoState.fastMode) {

        return;

    }

    ++mode_demoState.delay100ms;

    if (mode_demoState.delay100ms >= USER_DEMO_CHANGE_INT_100MS) {

        display_setDisplayState(((DisplayState)1) << (mode_demoState.demoStep), 0);
        ++mode_demoState.demoStep;
        mode_demoState.demoStep %= 32;
        mode_demoState.delay100ms = 0;

    }

}

static uint8_t DemoState_handleIR(uint8_t cmdCode)
{

    if (UI_UP == cmdCode || UI_DOWN == cmdCode) {

        log_state("DMF\n");

        mode_demoState.fastMode = !mode_demoState.fastMode;

    } else {

        return false;

    }

    return true;

}

static void DemoState_init()
{

}

static void DemoState_leave()
{

    pwm_release_brightness();

}

static void EnterTimeState_enter(const void* param)
{

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

    dispInternalTime(&mode_enterTimeState.time, display_getHoursMask() | display_getTimeSetIndicatorMask());

}

static uint8_t EnterTimeState_handleIr(uint8_t cmdCode)
{

    uint8_t caller = g_stateStack[g_currentIdxs[MS_enterTime] - 1];

    if (((MS_setSystemTime == caller) && (UI_SET_TIME == cmdCode))
        || ((MS_setOnOffTime == caller) && (UI_SET_ONOFF_TIMES == cmdCode))) {

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

    } else if (UI_UP == cmdCode || UI_DOWN == cmdCode) {

        int8_t dir = (UI_UP == cmdCode) ? 1 : -1;

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

    } else {

    }

    dispInternalTime(&mode_enterTimeState.time,
        ((ETS_hour == mode_enterTimeState.curSubState)
        ? display_getHoursMask()
        : display_getMinuteMask()) | display_getTimeSetIndicatorMask());

    return true;

}

static void EnterTimeState_init()
{

}

static void SetSystemTimeState_enter(const void* param)
{

    log_state("SST\n");

    addSubState(MS_setSystemTime, MS_enterTime, &g_dateTime);
    mode_setSystemTimeState.prohibitLeave = true;

}

static void SetSystemTimeState_substateFinished(e_MenuStates finishedState, const void* result)
{

    if (finishedState == MS_enterTime) {

        const datetime_t* time = result;

        i2c_rtc_write(time);
        g_dateTime = *time;
        mode_setSystemTimeState.prohibitLeave = false;
        quitMyself(MS_setSystemTime, NULL);

    }

}

static void SetSystemTimeState_init()
{

}

static void SetOnOffTimeState_enter(const void* param)
{

    datetime_t dt = {0, 0, 0, 0, 0, 0, 0};
    dt.hh = g_params->autoOffTimes[0].h;
    dt.mm = g_params->autoOffTimes[0].m;

    log_state("SOOT\n");

    mode_setOnOffTimeState.currentTimeToSet = 0;

    addSubState(MS_setOnOffTime, MS_enterTime, &dt);

    mode_setOnOffTimeState.prohibitLeave = true;

}

static void SetOnOffTimeState_substateFinished(e_MenuStates finishedState, const void* result)
{

    if (finishedState == MS_enterTime) {

        const datetime_t* time = result;
        datetime_t dt = *time;
        g_params->autoOffTimes[mode_setOnOffTimeState.currentTimeToSet].h = dt.hh;
        g_params->autoOffTimes[mode_setOnOffTimeState.currentTimeToSet].m = dt.mm;

        ++mode_setOnOffTimeState.currentTimeToSet;

        if (UI_AUTOOFFTIMES_COUNT == mode_setOnOffTimeState.currentTimeToSet) {

            DisplayState dispData;
            dispData = display_getNumberDisplayState((uint8_t)g_params->useAutoOffAnimation + 1);
            display_setDisplayState(dispData, dispData);

            if (g_params->useAutoOffAnimation) {

                g_animPreview = true;

            } else {

                g_animPreview = false;

            }

        } else {

            dt.hh = g_params->autoOffTimes[mode_setOnOffTimeState.currentTimeToSet].h;
            dt.mm = g_params->autoOffTimes[mode_setOnOffTimeState.currentTimeToSet].m;
            addSubState(MS_setOnOffTime, MS_enterTime, &dt);

        }

    }

}

static void SetOnOffTimeState_init()
{

}

static uint8_t SetOnOffTimeState_handleIr(uint8_t cmdCode)
{

    if (UI_AUTOOFFTIMES_COUNT == mode_setOnOffTimeState.currentTimeToSet) {

        if ((cmdCode == UI_DOWN) || (cmdCode == UI_UP)) {

            DisplayState dispData;
            g_params->useAutoOffAnimation = !g_params->useAutoOffAnimation;
            dispData = display_getNumberDisplayState((uint8_t)g_params->useAutoOffAnimation + 1);
            display_setDisplayState(dispData, dispData);

            if (g_params->useAutoOffAnimation) {

                g_animPreview = true;

            } else {

                g_animPreview = false;

            }

        }

        if (cmdCode == UI_SET_ONOFF_TIMES) {

            mode_setOnOffTimeState.prohibitLeave = false;
            g_animPreview = false;
            quitMyself(MS_setOnOffTime, NULL);

        }

    }

    return true;

}

static uint8_t PulseState_handleIR(uint8_t cmdCode)
{

    if (UI_UP == cmdCode || UI_DOWN == cmdCode) {

        int8_t dir = (UI_UP == cmdCode) ? -1 : 1;

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

static void PulseState_100Hz()
{

    ++mode_pulseState.delay100ms;

    if (mode_pulseState.delay100ms >= (volatile uint8_t)(g_params->pulseUpdateInterval)) {

        pwm_lock_brightness_val(color_pulse_waveform(mode_pulseState.curBrightness));
        ++mode_pulseState.curBrightness;
        mode_pulseState.delay100ms = 0;

    }

}

static void PulseState_10Hz()
{

    UserState_Isr10Hz(g_stateStack[g_currentIdxs[MS_pulse] - 1]);

}

static void PulseState_leave()
{

    pwm_release_brightness();

}

static void PulseState_init()
{

}

static void UserState_init()
{

    TrainIrState_init();
    ShowNumberState_init();
    NormalState_init();
    DemoState_init();
    SetSystemTimeState_init();
    SetOnOffTimeState_init();
    EnterTimeState_init();
    PulseState_init();

    #if (MONO_COLOR_CLOCK != 1)

        AutoHueState_init();

    #endif

}

static void UserState_enter(e_MenuStates state, const void* param)
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

    #if (MONO_COLOR_CLOCK != 1)

        } else if (MS_hueMode == state) {

            AutoHueStat_enter(param);

    #endif

    }

}

static void UserState_SubstateFinished(e_MenuStates state, e_MenuStates finishedState, const void* result)
{

    if (MS_setOnOffTime == state) {

        SetOnOffTimeState_substateFinished(finishedState, result);

    } else if (MS_setSystemTime == state) {

        SetSystemTimeState_substateFinished(finishedState, result);

    }

}

static uint8_t UserState_HandleIr(e_MenuStates state, uint8_t cmdCode)
{

    uint8_t handled = false;

    if (MS_enterTime == state) {

        handled = EnterTimeState_handleIr(cmdCode);

    } else if (MS_normalMode == state) {

        handled = NormalState_handleIR(cmdCode);

    } else if (MS_demoMode == state) {

        handled = DemoState_handleIR(cmdCode);

    } else if (MS_pulse == state) {

        handled = PulseState_handleIR(cmdCode);

    } else if (MS_setOnOffTime == state) {

        handled = SetOnOffTimeState_handleIr(cmdCode);

    #if (MONO_COLOR_CLOCK != 1)

        } else if (MS_hueMode == state) {

            handled = AutoHueState_handleIR(cmdCode);

    #endif

    }

    return handled;

}

static void UserState_LeaveState(e_MenuStates state)
{

    if (MS_pulse == state) {

        PulseState_leave();

    } else if (MS_demoMode == state) {

        DemoState_leave();

    }

}

static void UserState_Isr1Hz(e_MenuStates state)
{
    if (MS_irTrain == state) {

        TrainIrState_1Hz();

    }

}

static void UserState_Isr10Hz(e_MenuStates state)
{

    if (MS_showNumber == state) {

        ShowNumberState_10Hz();

    } else if (MS_demoMode == state) {

        DemoState_10Hz();

    } else if (MS_pulse == state) {

        PulseState_10Hz();

    #if (MONO_COLOR_CLOCK != 1)

        } else if (MS_hueMode == state) {

            AutoHueState_10Hz();

    #endif

    }

}

static void UserState_Isr100Hz(e_MenuStates state)
{

    if (MS_pulse == state) {

        PulseState_100Hz();

    }

}

static void UserState_Isr1000Hz(e_MenuStates state)
{

    if (MS_demoMode == state) {

        DemoState_1000Hz();

    }

}

static uint8_t UserState_prohibitTimeDisplay(e_MenuStates state)
{

    return (MS_irTrain == state) || (MS_showNumber == state)
        || (MS_demoMode == state) || (MS_setSystemTime == state)
        || (MS_setOnOffTime  == state) || (MS_enterTime == state);

}

static uint8_t UserState_prohibitLeave(e_MenuStates state)
{

    uint8_t prohibit = false;

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
