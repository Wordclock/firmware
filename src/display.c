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

/**
 * @brief Amount of steps the fading should take
 *
 * This can be referred to as the "granularity" of the fading. Obviously
 * increasing the amount of steps will make the fading itself look smoother,
 * however the amount of steps is limited by the timer frequency
 * (DISPLAY_TIMER_FREQUENCY) and the proposed fade time itself
 * (DISPLAY_FADE_TIME_MS). The default value for this option is 20, which
 * seems to be a good tradeoff with the factors mentioned above in mind.
 *
 * @see DISPLAY_FADE_PERIOD
 * @see DISPLAY_FADE_PERIOD_ANIM
 * @see g_curFadeStep
 * @see g_curFadeStepTimer
 * @see g_curFadeStepCounter
 */
#define DISPLAY_FADE_STEPS 20

/**
 * @brief Amount of PWM cycles per step
 *
 * This is the amount of PWM cycles a single step (see DISPLAY_FADE_STEPS) will
 * take up.
 *
 * @see DISPLAY_FADE_STEPS
 * @see g_curFadeStepTimer
 *
 * @note A single step length should be at least one PWM cycle long
 */
#define DISPLAY_FADE_PERIOD \
    ((uint16_t)(((((uint32_t)DISPLAY_TIMER_FREQUENCY) * DISPLAY_FADE_TIME_MS) / 1000) / DISPLAY_FADE_STEPS))

/**
 * @brief Amount of PWM cycles per step in case of an auto off animation
 *
 * This is the amount of PWM cycles a single step (see DISPLAY_FADE_STEPS) will
 * take up.
 *
 * @see DISPLAY_FADE_STEPS
 * @see g_curFadeStepTimer
 *
 * @note A single step length should be at least one PWM cycle long
 */
#define DISPLAY_FADE_PERIOD_ANIM \
    ((uint16_t)((( ((uint32_t)DISPLAY_TIMER_FREQUENCY) * DISPLAY_FADE_TIME_ANIM_MS) / 1000) / DISPLAY_FADE_STEPS))

/**
 * @brief Global variable containing the old display state
 *
 * This variable is mainly used by the appropriate ISR
 * (DISPLAY_TIMER_OVF_vect) and allows the ISR to implement a fading between
 * the old and the new (current) state, e.g. using display_fadeDisplayState().
 *
 * @see ISR(DISPLAY_TIMER_OVF_vect)
 * @see g_curDispState
 */
static DisplayState g_oldDispState;

 /**
  * @brief Global variable containing the current display state
  *
  * This variable is mainly used by the appropriate ISR
  * (DISPLAY_TIMER_OVF_vect) and allows the ISR (in combination with
  * g_blinkState) to implement a blinking effect for certain words.
  *
  * @see ISR(DISPLAY_TIMER_OVF_vect)
  * @see g_blinkState
  * @see g_oldDispState
  */
static DisplayState g_curDispState;

/**
 * @brief Global variable containing words that should be blinking
 *
 * This contains words that should be blinking rather than being displayed
 * statically as words within g_curDispState. It is mainly used by the
 * appropriate ISR (DISPLAY_TIMER_OVF_vect). The blinking itself is achieved
 * using display_blinkStep().
 *
 * @see ISR(DISPLAY_TIMER_OVF_vect)
 * @see display_blinkStep()
 * @see g_curDispState
 */
static DisplayState g_blinkState;

/**
 * @brief Global variable keeping track of the status within a single fade step
 *
 * The fading between two states consists of a predefined amount of steps
 * (DISPLAY_FADE_STEPS). Within each of these steps this variable is
 * compared against the current fade step (g_curFadeStep). When it is bigger
 * than the current fade step g_curFadeStep, the new and/or current display
 * state (g_curDispState) will be output, otherwise the old one will be output
 * (g_oldDispState). This counter will then be decremented by one. When
 * reaching 0, it will be reset to DISPLAY_FADE_STEPS - 1, which marks the
 * current fade step as completed, and enables the next fading step to be
 * processed. This can be referred to as a kind of "microsteps".
 *
 * @see DISPLAY_FADE_STEPS
 * @see g_curFadeStep
 * @see ISR(DISPLAY_TIMER_OVF_vect)
 */
static uint8_t g_curFadeCounter;

/**
 * @brief Global variable keeping track of the current step of the fading
 *
 * The fading between two states consists of a predefined amount of steps
 * (DISPLAY_FADE_STEPS). This variable is used mainly within the appropriate
 * ISR (DISPLAY_TIMER_OVF_vect). It should be noted that this variable
 * actually counts backwards, e.g. it usually gets initialized with
 * DISPLAY_FADE_STEPS - 1 and will be decremented by 1 with each step until it
 * reaches 0.
 *
 * @see DISPLAY_FADE_STEPS
 * @see ISR(DISPLAY_TIMER_OVF_vect)
 */
static uint8_t g_curFadeStep;

/**
 * @brief Global variable needed to determine the end of the current fade step
 *
 * This is used within the appropriate ISR to determine when the current fade
 * step is actually over, and therefore g_curFadeStep needs to be decremented
 * by one. It will then reset itself to either
 * (DISPLAY_FADE_PERIOD_ANIM / DISPLAY_FADE_STEPS) - 1 or
 * (DISPLAY_FADE_PERIOD / DISPLAY_FADE_STEPS) - 1 depending upon the value of
 * useAutoOffAnimation.
 *
 * @see ISR(DISPLAY_TIMER_OVF_vect)
 */
static uint16_t g_curFadeStepTimer;

/**
 * @brief Applies the given state the to display and shows it immediately
 *
 * This function applies the given state to the display. The new state will be
 * shown on the display immediately. This can be used to output a special
 * state on the display, which not dependent upon the time itself. That might
 * be useful to provide some feedback to the user when interacting with the
 * clock, e.g. when setting the time or working with and/or on the color
 * profiles.
 *
 * The first parameter (i_showStates) defines which words should be shown at
 * all. The second parameter (i_blinkstates) defines, which of the shown
 * words should blink. The blink interval is defined in DISPLAY_BLINK_INT_MS.
 * Only words that are set to be shown, can actually blink.
 *
 * The state itself is output using display_outputData(). All the rest is
 * handles by the appropriate ISR (DISPLAY_TIMER_OVF_vect) itself.
 *
 * @param i_showStates Defines which words should be shown
 * @param i_blinkstates Defines which of the shown words should blink
 *
 * @see display_outputData()
 * @see ISR(DISPLAY_TIMER_OVF_vect)
 */
void display_setDisplayState(DisplayState i_showStates, DisplayState i_blinkstates)
{

    g_blinkState = i_blinkstates & i_showStates;
    g_oldDispState = g_curDispState;
    g_curDispState = i_showStates;
    g_curFadeStep = 0;

}

/**
 * @brief Applies a new state to the display and fades over from the old one
 *
 * This function applies the given state to the display. The display will then
 * fade over from the old state to the new one to make it look smoother.
 *
 * Internally this function basically just sets up some variables, which will
 * be read back by the appropriate ISR (DISPLAY_TIMER_OVF_vect) during the
 * next iteration.
 *
 * @param i_showStates The new state that should be shown on the display
 *
 * @see ISR(DISPLAY_TIMER_OVF_vect)
 */
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

/**
 * @brief Applies a new state to the display and fades over from the old one
 *
 * This ISR is doing most of the work in regards to the fading between two
 * states. Is is executed at a regular basis, the frequency is defined in
 * DISPLAY_TIMER_FREQUENCY. It will then check whether there are still fade
 * steps left to be processed and will output the appropriate data. Once
 * the fading is completed, it will output the current display state every
 * time it is called.
 *
 * @see g_curFadeStep
 * @see g_curFadeCounter
 * @see g_curFadeStepTimer
 */
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
 * Updates the display if it is currently in a blinking state
 *
 * This implements the blinking effect for chosen words (g_blinkState) of the
 * display. This function is called by a timer ISR on a regular basis
 * (INTERRUPT_10HZ) with 10 Hz. The blinking frequency of the words itself
 * however will only be half as big, as with each execution of this function
 * the bit pattern will be flipped and it takes two flips to get back to the
 * original state again.
 *
 * The first thing this function does is to check whether there actually are
 * any words that should be blinking (g_blinkState != 0), so when no words are
 * set up to be blinking, this function will be returned from pretty quickly.
 *
 * @see INTERRUPT_10HZ
 * @see g_blinkState
 * @see DISPLAY_BLINK_INT_100MS
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
