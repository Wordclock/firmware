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
 * @file display.c
 * @brief Implementation of the display module generic to all types of clock
 *
 * This file contains the language and type independent implementation of the
 * display module. Among other things this includes the appropriate timer ISR
 * responsible for the fading between display states.
 *
 * @note This file should be left untouched if making adaptations to other
 * display types. Display type specific things reside in their own files, e.g.
 * display_[displaytype].h/c.
 *
 * @see display_wc.c
 * @see shift.h
 */

#include <inttypes.h>
#include <avr/io.h>
#include <util/delay.h>
#include <avr/pgmspace.h>
#include <avr/interrupt.h>

#include "main.h"
#include "display.h"
#include "user.h"
#include "uart.h"
#include "pwm.h"

#define DISPLAY_FADE_STEPS 20
#define DISPLAY_FADE_PERIOD \
    ((uint16_t)(((((uint32_t)DISPLAY_TIMER_FREQUENCY) * DISPLAY_FADE_TIME_MS) / 1000) / DISPLAY_FADE_STEPS))
#define DISPLAY_FADE_PERIOD_ANIM \
    ((uint16_t)((( ((uint32_t)DISPLAY_TIMER_FREQUENCY) * DISPLAY_FADE_TIME_ANIM_MS) / 1000) / DISPLAY_FADE_STEPS))

static uint32_t g_oldDispState;
static uint32_t g_curDispState;
static uint32_t g_blinkState;
static uint8_t g_curFadeCounter;
static uint8_t g_curFadeStep;
static uint16_t g_curFadeStepTimer;

void display_setDisplayState(DisplayState i_showStates, DisplayState i_blinkstates)
{

    g_blinkState = i_blinkstates & i_showStates;
    g_oldDispState = g_curDispState;
    g_curDispState = i_showStates;
    g_curFadeStep = 0;

    display_outputData(g_curDispState);

}

void display_fadeDisplayState(DisplayState i_showStates)
{

    g_blinkState = 0;
    g_oldDispState = g_curDispState;
    g_curDispState = i_showStates;
    g_curFadeStep = DISPLAY_FADE_STEPS - 1;

    if (useAutoOffAnimation) {

        g_curFadeStepTimer = (DISPLAY_FADE_PERIOD_ANIM / DISPLAY_FADE_STEPS) - 1;

    } else {

        g_curFadeStepTimer = (DISPLAY_FADE_PERIOD / DISPLAY_FADE_STEPS) - 1;

    }

    g_curFadeCounter = DISPLAY_FADE_STEPS - 1;

}

ISR(DISPLAY_TIMER_OVF_vect)
{

    if (g_curFadeStep > 0) {

        if (g_curFadeCounter >= g_curFadeStep) {

            display_outputData(g_curDispState);

        } else {

            display_outputData(g_oldDispState);

        }

        if (g_curFadeCounter) {

            g_curFadeCounter--;

        } else {

            g_curFadeCounter = DISPLAY_FADE_STEPS - 1;

            if (g_curFadeStepTimer) {

                g_curFadeStepTimer--;

            } else {

                if(useAutoOffAnimation) {

                    g_curFadeStepTimer = (DISPLAY_FADE_PERIOD_ANIM / DISPLAY_FADE_STEPS) - 1;

                } else {

                    g_curFadeStepTimer = (DISPLAY_FADE_PERIOD / DISPLAY_FADE_STEPS) - 1;

                }

                g_curFadeStep--;

            }

        }

    } else {

        display_outputData(g_curDispState);

    }

}

/**
 * @see INTERRUPT_10HZ
 */
void display_blinkStep()
{

    if(g_blinkState && (g_curFadeStep == 0)) {

        static uint8_t s_blinkPrescale = DISPLAY_BLINK_INT_100MS;

        if(!(--s_blinkPrescale)) {

            g_curDispState ^= g_blinkState;

            display_outputData(g_curDispState);

            s_blinkPrescale = DISPLAY_BLINK_INT_100MS;

        }

    }

}
