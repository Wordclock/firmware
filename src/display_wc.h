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

/*
 * For language/front related constants look at the approriate display_wc_[language].h
 */

#if (WC_DISP_ENG == 1)
#  include "display_wc_eng.h"
#elif (WC_DISP_GER == 1)
#  include "display_wc_ger.h" 
#elif (WC_DISP_GER3 == 1)
#  include "display_wc_ger3.h" 
#else   /* default to german */
#  define WC_DISP_GER 1
#  include "display_wc_ger.h" 
#endif

/* ports, pins and ddrs for minute LEDs*/
#define DISPLAY_MIN1 PORTB, 0
#define DISPLAY_MIN2 PORTD, 7
#define DISPLAY_MIN3 PORTC, 2
#define DISPLAY_MIN4 PORTC, 3

/**
 * This Enum defines how the led words are connected to the Board and the position 
 * in the state data (	that's why the minutes (gpio, not shift register) are also in this enum )
 * @details In different languages at least the constants for the 
 *          hours (DWP_one to DWP_twelve), the it-is (DWP_itis), the four minutes (DWP_min[1234]) and 
 *          clock (DWP_clock) have to exist, so they can be used for interface
 *          the minute words are too diffrent and should not be used outside of display_[language].h/c
 *          Following two preconditions were made:
 *          - the eight Minute words are placed consecutively
 *            (but not neccessaryly ordered)
 *            and DWP_MIN_FIRST defines the first of them
 *          - the twelve hours are orderd consecutively
 *            and DWP_HOUR_BEGIN defines the first of them
 */
typedef enum e_displayWordPos e_displayWordPos;



/* for documentation see prototype in display.h */
static inline DisplayState display_getIndicatorMask(void)
{
  return   ( 1L<< DWP_min1    )
         | ( 1L<< DWP_min2    ) 
         | ( 1L<< DWP_min3    )
         | ( 1L<< DWP_min4    );
}

#endif /* _WC_DISPLAY_WC_H_ */
