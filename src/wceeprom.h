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
 * @see uart.c
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
     * software has changed the struct WcEeepromData. This guarantees that no
     * incompatible version of WcEepromData are mixed with each other.
     *
     * @see BUILD_VERSION()
     * @see MAJOR_VERSION()
     * @see MINOR_VERSION()
     *
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
     */
    uint8_t structSize;

} WcEepromData;

/**
 * @brief Initializes this module by reading data from EEPROM and keeping a
 *   copy of it in memory
 *
 * This has to be called **before** any other functions of this module can be
 * used. It copies the data from EEPROM into memory and makes it accessible
 * with wcEeprom_getData(). Changes to the data done in memory can be written
 * back to be stored persistently using wcEeprom_writeback().
 *
 * @see wcEeprom_getData()
 * @see wcEeprom_writeback()
 */
void wcEeprom_init(void);

/**
 * @brief Returns a pointer to a copy of WcEepromData
 *
 * This returns a pointer to a variable of type WcEepromData. This variable
 * can then be used to get the persistently stored values. It can also be
 * used to modify the values. In order for the changes to get written back
 * into EEPROM, wcEeprom_writeback() needs to be called.
 *
 * @warning WcEepromData shouldn't become larger than 254 bytes, as the code
 * right now uses a lot of 8 bit counters.
 *
 * @see WcEepromData
 * @see wcEeprom_writeback()
 */
static inline WcEepromData* wcEeprom_getData(void)
{

	/*
	 * This is actually defined in wceeprom.c
	 */
    extern WcEepromData g_epromWorking;

    return &g_epromWorking;

}

/**
 * @brief Writes changes done to the WcEepromData instance in RAM into EEPROM
 *
 * As stated in the description of WcEepromData there are basically two
 * "instances" of this variable. One in RAM and one in EEPROM, which the one
 * in RAM is based on. If any changes are done to the instance in RAM this
 * function has to be called in order for the changes to be written back to
 * EEPROM so they are stored persistently.
 *
 * Instead of writing the whole struct into EEPROM each and every time again,
 * it is also possible to only write back bytes that actually have changed,
 * which not only saves space, but also helps to increase the expected lifetime
 * of the EEPROM cells involved.
 *
 * @warning Because writing to EEPROM takes quite some time it is possible
 * that interrupts will be missed.
 *  *
 * @param start Pointer to the start of the data that has to be written back
 * @param len The length of the data that has to be written back
 *
 * @see WcEepromData
 */
void wcEeprom_writeback(const void* start, uint8_t len);

#endif /* _WC_EEPROM_H_ */
