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

/**
 * @brief Interval (in multiples of 100 ms) for blinking words
 *
 * The default value for this option is 7 (700 ms)
 *
 * @see display_blinkStep()
 */
#define DISPLAY_BLINK_INT_100MS 7

/**
 * @brief Time (in ms) the fading between two times should take
 *
 * The default value for this option is 500 (500 ms).
 *
 * @see DISPLAY_FADE_PERIOD
 */
#define DISPLAY_FADE_TIME_MS 500

/**
 * @brief Time (in ms) the fading of the auto off animation should take
 *
 * The default value for this option is 1000 (1 s).
 *
 * @see DISPLAY_FADE_PERIOD_ANIM
 * @see useAutoOffAnimation
 */
#define DISPLAY_FADE_TIME_ANIM_MS 1000

/**
 * @brief Overflow interrupt vector of the Timer/Counter involved
 *
 * This macro stores the overflow interrupt vector of the Timer/Counter
 * responsible for the display module. Using this abstraction it is possible
 * to change the involved Timer/Counter without fiddling around in the
 * sources itself too much.
 *
 * @see ISR(TIMER2_OVF_vect)
 */
#define DISPLAY_TIMER_OVF_vect TIMER2_OVF_vect

/**
 * @brief Macro used to enable involved interrupts
 *
 * This is used during initialization to enable the interrupt of the involved
 * Timer/Counter responsible for the display module. Using this abstraction it
 * is possible to change the involved Timer/Counter without fiddling around in
 * the sources itself too much.
 *
 * @see display_init()
 */
#define DISPLAY_TIMER_ENABLE_INTS() TIMSK2 |= _BV(TOIE2);

/**
 * @brief Frequency the timer/counter involved is running at (in Hz)
 *
 * The frequency is calculated in the following way:
 *
 * `8000000 (F_CPU) / 8 (Prescaler) / 256 (8 bit counter) = 3906.25`
 *
 * @see DISPLAY_FADE_PERIOD
 * @see DISPLAY_FADE_PERIOD_ANIM
 */
#define DISPLAY_TIMER_FREQUENCY 3906

/**
 * @brief Allowing access to global instance of displayParams backed by EEPROM
 *
 * @see PwmEepromParams::displayParams
 */
#define g_displayParams (&(wcEeprom_getData()->displayParams))

/**
 * @brief Type definition for storing a display state
 *
 * A display state is basically just a simple bitset, where each bit represents
 * a specific word, which can either be enabled and/or disabled. The order of
 * the words itself is defined by the enum e_displayWordPos, which in return
 * is defined within the language specific files for a given display type.
 *
 * @see e_displayWordPos
 */
typedef uint32_t DisplayState;

/**
 * @brief Data of the display module that should be stored persistently in EEPROM
 *
 * The data from this module to be stored persistently in the EEPROM is
 * expected to be in the form of this type. The definition of the struct itself
 * has to be in the language specific header.
 *
 * The default values are defined in DISPLAYEEPROMPARAMS_DEFAULT.
 *
 * @see DISPLAYEEPROMPARAMS_DEFAULT
 * @see WcEepromData::displayParams
 */
typedef struct DisplayEepromParams DisplayEepromParams;

extern void display_init();
extern void display_outputData(DisplayState state);

/**
 * @brief Returns a display state for the given datetime
 *
 * This functions returns a display state for the given datetime. As this is
 * something specific to the frontpanel, it is actually implemented by the
 * language specific file itself.
 *
 * @param i_newDateTime The datetime to get a display state for
 *
 * @return The display state for the given datetime
 *
 * @see datetime_t
 * @see DisplayState
 */
extern DisplayState display_getTimeState(const datetime_t* i_newDateTime);

extern void display_setDisplayState(DisplayState i_showStates, DisplayState i_blinkstates);
extern void display_fadeDisplayState(DisplayState i_showStates);
extern void display_blinkStep();
extern void display_autoOffAnimStep1Hz(uint8_t animPreview);

/**
 * @brief Returns a display state that contains a visual indication
 *
 * The returned indication can be used as feedback to the user when interacting
 * with the clock. In case of the Wordclock it makes sense to use the minute
 * LEDs as an indicator.
 *
 * @return A display state with activated indicators
 */
static inline DisplayState display_getIndicatorMask();

/**
 * @brief Returns a display state indicating the the time is being set
 *
 * The returned indication can be used as feedback to the user when interacting
 * with the clock while setting the time itself. In case of the Wordclock it
 * makes sense to use the phrase "clock" for that purpose.
 *
 * This is effectively implemented in the language specific files, as it
 * is depending upon the way the LEDs are connected to the frontpanel, see
 * e_displayWordPos.
 *
 * @return A display state with activated indicators
 */
static inline DisplayState display_getTimeSetIndicatorMask();

/**
 * @brief Returns a display state with all bits representing minutes enabled
 *
 * This functions returns a display state with all bits, which represent a
 * minute ("five-minute" as well as the four separate minute LEDs) enabled.
 *
 * This is effectively implemented in the language specific files, as it
 * is depending upon the way the LEDs are connected to the frontpanel, see
 * e_displayWordPos.
 *
 * @return Display state with all bits representing minutes enabled
 *
 * @see DisplayState
 * @see e_displayWordPos
 */
static inline DisplayState display_getMinuteMask();

/**
 * @brief Returns a display state with all bits representing a hour enabled
 *
 * This functions returns a display state with all bits, which represent a
 * hour (1-12) enabled.
 *
 * This is effectively implemented in the language specific files, as it
 * is depending upon the way the LEDs are connected to the frontpanel, see
 * e_displayWordPos.
 *
 * @return Display state with all bits representing hours enabled
 *
 * @see DisplayState
 * @see e_displayWordPos
 */
static inline DisplayState display_getHoursMask();

/**
 * @brief Returns a display state with a representation for the given number
 *
 * This functions returns a display state which will represent the given
 * number. This can be used to interact with the user by offering him an
 * visual feedback within certain menus.
 *
 * This is effectively implemented in the language specific files, as it
 * is depending upon the way the LEDs are connected to the frontpanel, see
 * e_displayWordPos.
 *
 * @param number The number you want to show a representation of to the user
 *
 * @return Display state with a representation of the given number
 *
 * @see DisplayState
 */
static inline DisplayState display_getNumberDisplayState(uint8_t number);

/**
 * @brief Sets a new time to be shown on the display
 *
 * This function should be called on each time change, at least one to two
 * times per minute.
 *
 * @param i_newDateTime The new time that should be shown on the display
 *
 * @see datetime_t
 * @see display_setDisplayState()
 * @see display_getTimeState()
 */
static inline void display_setNewTime(const datetime_t* i_newDateTime)
{

    display_setDisplayState(display_getTimeState(i_newDateTime), 0);

}

/**
 * @brief Sets a new time to be shown on the display by fading out the old one
 *
 * This function should be called on each time change, at least one to two
 * times per minute.
 *
 * @param i_newDateTime The new time that should be shown on the display
 *
 * @see datetime_t
 * @see display_fadeDisplayState()
 * @see display_getTimeState()
 */
static inline void display_fadeNewTime(const datetime_t* i_newDateTime)
{

    display_fadeDisplayState(display_getTimeState(i_newDateTime));

}

#include "display_wc.h"

#endif /* _WC_DISPLAY_H_ */
