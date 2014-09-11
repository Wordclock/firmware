/*
 * Copyright (C) 2014 Karol Babioch <karol@babioch.de>
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
 * @file brightness.h
 * @brief Functionality dealing with brightness control
 *
 * This module is responsible for the brightness control of the Wordclock. For
 * now it only contains a single function, which handles the automatic
 * brightness adjustment in response to changes of the ambient lightning.
 *
 * @see brightness.c
 *
 * @todo Move brightness control functionality from PWM module to this one
 */

#ifndef _WC_BRIGHTNESS_H_
#define _WC_BRIGHTNESS_H_

extern void brightness_init();
extern void brightness_handle();

#endif /* _WC_BRIGHTNESS_H_ */
