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
 * @file display.h
 * @brief Header file for the display module generic to all types of clocks
 *
 * The display module is the main interface for the display hardware of the
 * clock. It converts a given time into binary data, which then for instance
 * can be shifted out to the registers in control of the LEDs.
 *
 * Display type related constants are defined within their appropriate
 * display_wc_[type].h file.
 *
 * @note This file should be left untouched if making adaptations to other
 * display types. Display type specific things reside in their own files, e.g.
 * display_[displaytype].h/c.
 *
 * @see display_wc.c
 * @see shift.h
 */

#ifndef _WC_DISPLAY_H_
#define _WC_DISPLAY_H_

#include "datetime.h"

#define DISPLAY_BLINK_INT_100MS 7
#define DISPLAY_FADE_TIME_MS 500
#define DISPLAY_FADE_TIME_ANIM_MS 1000
#define DISPLAY_TIMER_OVF_vect TIMER2_OVF_vect
#define DISPLAY_TIMER_ENABLE_INTS() TIMSK2 |= _BV(TOIE2);
#define DISPLAY_TIMER_FREQUENCY 3906
#define g_displayParams (&(wcEeprom_getData()->displayParams))

typedef uint32_t DisplayState;
typedef struct DisplayEepromParams DisplayEepromParams;

extern void display_init();
extern void display_outputData(DisplayState state);
extern DisplayState display_getTimeState(const datetime_t* i_newDateTime);
extern void display_setDisplayState(DisplayState i_showStates, DisplayState i_blinkstates);
extern void display_fadeDisplayState(DisplayState i_showStates);
extern void display_blinkStep();
extern void display_autoOffAnimStep1Hz(uint8_t animPreview);

static inline DisplayState display_getIndicatorMask();
static inline DisplayState display_getTimeSetIndicatorMask();
static inline DisplayState display_getMinuteMask();
static inline DisplayState display_getHoursMask();
static inline DisplayState display_getNumberDisplayState(uint8_t number);

static inline void display_setNewTime(const datetime_t* i_newDateTime)
{

    display_setDisplayState(display_getTimeState(i_newDateTime), 0);

}

static inline void display_fadeNewTime(const datetime_t* i_newDateTime)
{

    display_fadeDisplayState(display_getTimeState(i_newDateTime));

}

#include "display_wc.h"

#endif /* _WC_DISPLAY_H_ */
