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
 * @file wceeprom.h
 * @brief Header enabling access to persistent storage of various settings
 *
 * The microcontroller used in this project provides some amount of
 * [EEPROM][1]. This EEPROM is used to store various settings to be available
 * across resets. This among other things includes various parameters from the
 * user module, the display module and PWM module.
 *
 * This module is used to get previously saved settings during boot up and save
 * the settings whenever changes occur.
 *
 * [1]: https://en.wikipedia.org/wiki/EEPROM
 *
 * @see WcEepromData
 * @see wceeprom.c
 */

#ifndef _WC_EEPROM_H_
#define _WC_EEPROM_H_

#include "user.h"
#include "display.h"
#include "pwm.h"

/**
 * @brief Holds all the data that should be stored persistently
 *
 * This struct holds all of the data that should be stored persistently within
 * the EEPROM. There will basically be two "instances" of this struct.
 * One within the EEPROM itself and one within RAM, which of course can be
 * modified. These changes then will be written to the EEPROM to be persistent
 * using wcEeprom_writeback().
 *
 * Each module listed here has its own struct (UserEepromParams,
 * DisplayEepromParams, PwmEepromParams) and is expected to provide default
 * values for these in a macro called upcase(structname)_DEFAULTS, e.g. the
 * macro name for the struct UserEepromParams would be
 * USEREEPROMPARAMS_DEFAULTS.
 *
 * @see wcEeprom_writeback()
 */
typedef struct WcEepromData {

    /**
     * @brief Parameters to be stored persistently from the user module
     *
     * @see user.h
     */
    UserEepromParams userParams;

    /**
     * @brief Parameters to be stored persistently from the display module
     *
     * @see display.h
     */
    DisplayEepromParams displayParams;

    /**
     * @brief Parameters to be stored persistently from the PWM module
     *
     * @see pwm.h
     */
    PwmEepromParams pwmParams;

    /**
     * @brief Version number
     *
     * This is used as a basic integrity check. Whenever there is difference
     * in the version number provided by the software itself and the one stored
     * in EEPROM, the EEPROM data will be ignored and overwritten by the
     * default values. This is especially for cases when a new version of the
     * software has changed the struct WcEeepromData. It guarantees that no
     * incompatible versions of WcEepromData are mixed with each other.
     *
     * @see BUILD_VERSION()
     * @see MAJOR_VERSION()
     * @see MINOR_VERSION()
     */
    uint8_t swVersion;

    /**
     * @brief Size of this struct
     *
     * Once again, like WcEepromData::swVersion, this is used as a reference
     * for integrity. If there is a difference in size between the size
     * claimed by the firmware itself and the one stored in EEPROM the data
     * of the EEPROM will be ignored and overwritten.
     *
     * The size will be calculated automatically during compilation, so you
     * don't have to worry about it too much.
     *
     * @warning This is a 8 bit value, so the size of the struct shouldn't
     *  exceed 256 bytes.
     */
    uint8_t structSize;

} WcEepromData;

extern void wcEeprom_init();

extern WcEepromData* wcEeprom_getData();

extern void wcEeprom_writeback(const void* start_p, uint8_t len);

#endif /* _WC_EEPROM_H_ */
