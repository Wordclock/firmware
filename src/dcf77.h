/*
 * Copyright (C) 2012, 2013 Karol Babioch <karol@babioch.de>
 * Copyright (c) 2010 Torsten Giese
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
 * @file dcf77.h
 * @brief Interface for access to the DCF77 time signal
 *
 * DCF77 is used to get the current date and time automatically. This file
 * defines the interface to get access to the date and time information. There
 * is an initialization phase, which will try to detect the availability of the
 * module and determine whether or not the internal pull up resistor is needed.
 *
 * @see dcf77.c
 */

#ifndef _WC_DCF77_H_
#define _WC_DCF77_H_

#if (DCF_PRESENT == 1)

extern uint8_t   enable_dcf77_ISR;     // En- / Disable DCF77 examination
/**
 *  Initializes DCF77
 *  @details  Configures DCF77 IO hardware and clear variables
 *
 */
extern void      dcf77_init          (void);
/**
 *  receive and decode DCF77 signal
 *  @details  Decode DCF77 signal - measure pulse time to
 *            decode the received signal
 *            must be called each 10ms (1000 HZ)
 *
 */
extern void      dcf77_ISR           (void);
/**
 *  update RTC with DCF77
 *  @details  update RTC time with dcf77 time
 *            called 1 time per day
 *  @param    DateTime_p as the actual date and time
 *  @return   True if valid dcf77 signal was received and datetime_p updated
 *
 */
extern uint8_t   dcf77_getDateTime   (DATETIME * DateTime_p);
#else

#define  dcf77_ISR()

#endif  /** (DCF_PRESENT == 1) */

#endif /* _WC_DCF77_H_ */
