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
 * @file display_wc.h
 * @brief Header file of the display module for the display hardware
 *
 * The display module converts a given time into binary data, which then can be
 * shifted out to the registers in control of the LEDs using functions declared
 * in shift.h.
 *
 * Language and/or front panel related constants are defined within their
 * appropriate display_wc_[language].h file.
 *
 * @note This file should be left untouched if making adaptations to other
 * languages. Language specific things reside in their own files, e.g.
 * display_wc_[language].h/c.
 *
 * @see display_wc.c
 * @see shift.h
 */

#ifndef _WC_DISPLAY_WC_H_
#define _WC_DISPLAY_WC_H_

#if (WC_DISP_ENG == 1)

    #include "display_wc_eng.h"

#elif (WC_DISP_GER == 1)

    #include "display_wc_ger.h"

#elif (WC_DISP_GER3 == 1)

    #include "display_wc_ger3.h"

#else

    #error "A language for the front panel must be selected"

#endif

#define DISPLAY_MIN1 PORTB, 0
#define DISPLAY_MIN2 PORTD, 7
#define DISPLAY_MIN3 PORTC, 2
#define DISPLAY_MIN4 PORTC, 3

typedef enum e_displayWordPos e_displayWordPos;

static inline DisplayState display_getIndicatorMask(void)
{

    return (1L << DWP_min1) | (1L << DWP_min2) | (1L << DWP_min3) | (1L << DWP_min4);
}

#endif /* _WC_DISPLAY_WC_H_ */
