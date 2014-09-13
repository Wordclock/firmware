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
 * @brief Header providing access to persistent storage backed by EEPROM
 *
 * The EEPROM provided by the microcontroller is used to store various settings
 * persistently. This header defines the type of data that is being stored and
 * provides means to access the data and write it back, once it has been
 * changed.
 *
 * @see preferences.c
 */

#ifndef _WC_PREFERENCES_H_
#define _WC_PREFERENCES_H_

#include "user.h"
#include "display.h"
#include "pwm.h"

/**
 * @brief Defines the type of data stored persistently in the EEPROM
 *
 * This struct describes the data that is stored persistently in the EEPROM.
 * Some members are type definitions itself and are defined in the appropriate
 * modules along with their appropriate default values.
 *
 * @warning This shouldn't become larger than 254 bytes for now, as the code
 * right now uses a lot of 8 bit counters.
 *
 * @warning This shouldn't become bigger than the size of the EEPROM itself,
 * which is 512 bytes for the ATmega168/ATmega328.
 *
 * @see eepromParams
 * @see eepromDefaultParams_P
 * @see g_epromWorking
 */
typedef struct {

    /**
     * @brief Data of the user module to be stored persistently in EEPROM
     *
     * @see user_prefs_t
     */
    user_prefs_t userParams;

    /**
     * @brief Data of the display module to be stored persistently in EEPROM
     *
     * @see display_prefs_t
     */
    display_prefs_t displayParams;

    /**
     * @brief Data of the display module to be stored persistently in EEPROM
     *
     * @see PwmEepromParams
     */
    PwmEepromParams pwmParams;

    /**
     * @brief Version number
     *
     * This along with `prefs_t::structSize` is mainly used as a basic
     * integrity check during the initialization of this module by
     * `preferences_init()`.
     *
     * @see BUILD_VERSION()
     */
    uint16_t swVersion;

    /**
     * @brief Size of this struct
     *
     * Describes the size of this struct and is used along with
     * `prefs_t::swVersion` as a basic integrity check during the
     * initialization of this module by `preferences_init()`.
     *
     * @warning This is a 8 bit value, so the size of the struct shouldn't
     * exceed 256 bytes.
     */
    uint8_t structSize;

} prefs_t;

void preferences_init();

prefs_t* preferences_get();
void preferences_save(const void* start, uint8_t length);

#endif /* _WC_PREFERENCES_H_ */
