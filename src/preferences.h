/*
 * Copyright (C) 2012, 2013, 2014 Karol Babioch <karol@babioch.de>
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
 * @file preferences.h
 * @brief Header providing access to preferences that can be accessed globally
 *
 * This module allows for preferences that are customizable by the user to be
 * accessed globally throughout the project. The preferences are backed by a
 * persistent storage and will be restored during initialization. This module
 * was deliberately created to decouple the persistent storage from the access
 * to the actual data, so in other storage backends could be used in different
 * circumstances.
 *
 * @see preferences.c
 */

#ifndef _WC_PREFERENCES_H_
#define _WC_PREFERENCES_H_

#include <stdbool.h>

#include "user.h"
#include "display.h"
#include "pwm.h"
#include "version.h"

/**
 * @brief Preferences that should be stored and can be accessed globally
 *
 * This structure describes the preferences that are stored persistently and
 * can be accessed globally throughout the project. Modules that want to
 * take advantage of this facility should define their own type and include
 * it into this structure. The default values are defined within
 * {@link prefs_default}.
 *
 * @warning This shouldn't become larger than 254 bytes for now, as the code
 * right now uses a lot of 8 bit counters.
 *
 * @warning This shouldn't become bigger than the size of the EEPROM itself,
 * which is 512 bytes for the ATmega168/ATmega328.
 *
 * @see prefs_default
 */
typedef struct {

    /**
     * @see user_prefs_t
     */
    user_prefs_t user_prefs;

    /**
     * @see display_prefs_t
     */
    display_prefs_t display_prefs;

    /**
     * @see pwm_prefs_t
     */
    pwm_prefs_t pwm_prefs;

    /**
     * @brief Version number
     *
     * This is used along with {@link prefs_t::prefs_size} as a basic integrity
     * check during the {@link #preferences_init() initialization} of this
     * module.
     *
     * @see versiont_t
     */
    version_t version;

    /**
     * @brief Byte size of this structure
     *
     * This holds the size of this structure and is used along with
     * {@link prefs_t::version} as a basic integrity check during the
     * {@link #preferences_init() initialization} of this module.
     *
     * @warning This is a 8 bit value, so the size of the struct shouldn't
     * exceed 256 bytes.
     */
    uint8_t prefs_size;

} prefs_t;

void preferences_init();

prefs_t* preferences_get();
bool preferences_save();

#endif /* _WC_PREFERENCES_H_ */
