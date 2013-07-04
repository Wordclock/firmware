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
 * @file display_wc.c
 * @brief Implementation of the display module specific to the Wordclock
 *
 * This file contains implementation of things specific to the display type
 * "Wordclock". However it is language independent and generic to all
 * languages defined for this type of display. Among other things this file
 * contains the initialization routine as well as a routine to output data to
 * the display.
 *
 * @note This file should be left untouched if making adaptations to other
 * languages. Language specific things reside in their own files, e.g.
 * display_wc_[language].h/c.
 *
 * @see display_wc.h
 */

#include <avr/pgmspace.h>

#include "main.h"
#include "display.h"
#include "shift.h"
#include "uart.h"
#include "ports.h"

/**
 * @brief Initializes the display module
 *
 * This will initialize the module by initializing the involved shift module.
 * Furthermore it will set up the appropriate registers for the minute LEDs
 * (which are attached to the microcontroller directly) as well as enable
 * the involved timer interrupt.
 *
 * @see shift24_init()
 * @see DISPLAY_TIMER_ENABLE_INTS()
 */
void display_init()
{

    shift24_init();

    DDR(DISPLAY_MIN1) |= _BV(BIT(DISPLAY_MIN1));
    DDR(DISPLAY_MIN2) |= _BV(BIT(DISPLAY_MIN2));
    DDR(DISPLAY_MIN3) |= _BV(BIT(DISPLAY_MIN3));
    DDR(DISPLAY_MIN4) |= _BV(BIT(DISPLAY_MIN4));

    PORT(DISPLAY_MIN1) &= ~(_BV(BIT(DISPLAY_MIN1)));
    PORT(DISPLAY_MIN2) &= ~(_BV(BIT(DISPLAY_MIN2)));
    PORT(DISPLAY_MIN3) &= ~(_BV(BIT(DISPLAY_MIN3)));
    PORT(DISPLAY_MIN4) &= ~(_BV(BIT(DISPLAY_MIN4)));

    DISPLAY_TIMER_ENABLE_INTS();

}

/**
 * @brief Outputs a display state
 *
 * This will output a given display state, which involves enabling and/or
 * disabling the minute LEDs and shifting out the bit pattern. When logging
 * for this particular module is enabled (LOG_DISPLAY_STATE), some debugging
 * information will be output via UART.
 *
 * @param state The DisplayState that should be output
 *
 * @see shift24_output()
 *
 * @note The display module should be enabled before this function can be
 * used.
 */
void display_outputData(DisplayState state)
{

    shift24_output(state);

    if (state & ((DisplayState)1 << DWP_min1)) {

        PORT(DISPLAY_MIN1) |= _BV(BIT(DISPLAY_MIN1));

    } else {

        PORT(DISPLAY_MIN1) &= ~(_BV(BIT(DISPLAY_MIN1)));

    }

    if (state & ((DisplayState)1 << DWP_min2)) {

        PORT(DISPLAY_MIN2) |= _BV(BIT(DISPLAY_MIN2));

    } else {

        PORT(DISPLAY_MIN2) &= ~(_BV(BIT(DISPLAY_MIN2)));

    }

    if (state & ((DisplayState)1 << DWP_min3)) {

        PORT(DISPLAY_MIN3) |= _BV(BIT(DISPLAY_MIN3));

    } else {

        PORT(DISPLAY_MIN3) &= ~(_BV(BIT(DISPLAY_MIN3)));

    }

    if (state & ((DisplayState)1 << DWP_min4)) {

        PORT(DISPLAY_MIN4) |= _BV(BIT(DISPLAY_MIN4));

    } else {

        PORT(DISPLAY_MIN4) &= ~(1 << BIT(DISPLAY_MIN4));

    }

    #if (LOG_DISPLAY_STATE == 1)

        uint8_t i;

        uart_puts_P("Disp: ");

        for (i = 0; i < 32; i++) {

            uart_putc('0' + (state & 1));

            state >>= 1;

        }

        uart_putc('\n');

    #endif

}

/**
 * @brief Responsible for the auto on off animation
 *
 * This function is expected to be called on a regular basis with a frequency
 * of 1 Hz. This is usually done via user_isr1Hz(). It will let the minute
 * LEDs appear to blink alternately. When the parameter animPreview is set, the
 * word corresponding to "two" (hour) will be enabled on top of that. This is
 * referred to as "preview mode" and can be useful in case when setting up the
 * auto on off animation up to give the user some form of indication, which
 * mode he actually has selected and how it will look like.
 *
 * @param animPreview Defines whether "two" (hour) will be displayed
 *
 * @see display_getNumberDisplayState()
 */
void display_autoOffAnimStep1Hz(uint8_t animPreview)
{

    static uint8_t s_state = 0;

    s_state++;
    s_state %= 8;

    if (s_state & 1) {

        DisplayState state = ((DisplayState)1) << ((s_state >> 1) + DWP_MIN_LEDS_BEGIN);

        if (animPreview) {

            state |= display_getNumberDisplayState(2);

        }

        display_fadeDisplayState(state);

    } else {

        display_fadeDisplayState(0);

    }

}
