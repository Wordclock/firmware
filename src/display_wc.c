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
 * @file display_wc.c
 *
 *  Implementation of the language-independent word clock display stuff
 *
 * \version $Id: display_wc.c 405 2011-11-24 20:39:00Z sm $
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

#include <inttypes.h>
#include <avr/io.h>
#include <util/delay.h>
#include <avr/pgmspace.h>

#include "main.h"
#include "display.h"
#include "shift.h"
#include "uart.h"
#include "ports.h"

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

void display_outputData(DisplayState state)
{

    shift24_output(state);

    if (state & (1L << DWP_min1)) {

        PORT(DISPLAY_MIN1) |= _BV(BIT(DISPLAY_MIN1));

    } else {

        PORT(DISPLAY_MIN1) &= ~(_BV(BIT(DISPLAY_MIN1)));

    }

    if (state & (1L << DWP_min2)) {

        PORT(DISPLAY_MIN2) |= _BV(BIT(DISPLAY_MIN2));

    } else {

        PORT(DISPLAY_MIN2) &= ~(_BV(BIT(DISPLAY_MIN2)));

    }

    if (state & (1L << DWP_min3)) {

        PORT(DISPLAY_MIN3) |= _BV(BIT(DISPLAY_MIN3));

    } else {

        PORT(DISPLAY_MIN3) &= ~(_BV(BIT(DISPLAY_MIN3)));

    }

    if(state & (1L << DWP_min4)) {

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
