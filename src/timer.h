/*
 * Copyright (C) 2012, 2013 Karol Babioch <karol@babioch.de>
 * Copyright (c) 2010 Frank Meyer - frank(at)fli4l.de
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
 * @file timer.h
 * @brief Interface for access to the timers including the appropriate ISRs
 *
 * This module is responsible for setting up the timers of the microcontroller.
 * It's basically enough to initialize the module by calling timer_init().
 *
 * @see timer.c
 */

#ifndef _WC_TIMER_H_
#define _WC_TIMER_H_

extern void                   timer_init (void);

#endif /* _WC_TIMER_H_ */
