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
 * @file preferences_eeprom.c
 * @brief Implementation of preference module with EEPROM as backend
 *
 * This implements the functionality declared in {@link preferences.h}. A copy
 * of the {@link prefs_t preferences} is always hold in SRAM. A reference to
 * this copy can be {@link #preferences_get() retrieved} and manipulated. Note,
 * however, that a {@link preferences_save() save} operation needs to be
 * invoked in order for the data to be written to the EEPROM coming along with
 * the microcontroller in use.
 *
 * The EEPROM is accessed by the functionality provided by {@link eeprom.h}.
 *
 * @see eeprom.h
 * @see preferences.h
 */

#include <avr/eeprom.h>
#include <avr/pgmspace.h>

#include "base.h"
#include "eeprom.h"
#include "log.h"
#include "preferences.h"
#include "uart.h"
#include "version.h"

/**
 * @brief Represents the preferences within EEPROM
 *
 * @see prefs_t
 */
static prefs_t prefs_eeprom EEMEM;

/**
 * @brief Default settings for preferences
 *
 * These are the default settings for the {@link #prefs_t preferences}. They
 * are stored within program space and will get used whenever the data in the
 * EEPROM is considered to be invalid.
 *
 * @see prefs_t
 * @see preferences_init()
 */
static const prefs_t prefs_default PROGMEM = {

    USEREEPROMPARAMS_DEFAULT,
    DISPLAYEEPROMPARAMS_DEFAULT,
    PWMEEPROMPARAMS_DEFAULT,
    VERSION,
    sizeof(prefs_t),

};

/**
 * @brief Copy of preferences hold in SRAM
 *
 * This is holding all of the {@link #prefs_t preferences} in SRAM. It is
 * backed by the content of the EEPROM and can be accessed globally using
 * {@link #preferences_get()}. Note though that changes made this variable
 * are not automatically written back to the EEPROM, but a
 * {@link #preferences_save() save} operation must be invoked.
 *
 * @see prefs_t
 */
static prefs_t prefs;

/**
 * @brief Outputs the content of the preferences byte by byte
 *
 * This iterates over the the copy of the preferences hold in SRAM and outputs
 * the content on a byte by byte basis. The length of the content is determined
 * by {@link #prefs_t}.
 *
 * @see g_epromWorking
 * @see eepromParams
 * @see log_output_callback_t
 */
static void preferences_output(FILE* logout)
{

    fprintf_P(logout, PSTR("Content: "));

    uint8_t* ptr = (uint8_t*)(&prefs);

    for (uint8_t i = 0; i < sizeof(prefs_t); i++) {

        // TODO: Get rid of flushing implicitly within UART module
        if (i % UART_BUFFER_SIZE_OUT / 2) {

            uart_flush_output();

        }

        fprintf_P(logout, PSTR("%02x"), *ptr++);

    }

}

/**
 * @brief Initializes this module
 *
 * This has to be called **before** any other functions of this module can be
 * used. It reads in the contents from the EEPROM into {@link #prefs SRAM} and
 * performs a basic integrity check consisting of:
 *
 * - Comparison of software version stored in EEPROM against VERSION
 * - Comparison of struct size stored in EEPROM against prefs_t::prefs_size
 *
 * When this check fails, the {@link #prefs_default default values} are used.
 * Otherwise the data from the EEPROM is considered to be valid and will end
 * up being used.
 *
 * @see prefs_t::version
 * @see prefs_t::prefs_size
 */
void preferences_init()
{

    // Set default log level
    log_set_level(LOG_MODULE_PREFERENCES, LOG_LEVEL_PREFERENCES_DEFAULT);

    eeprom_get_block(&prefs, &prefs_eeprom, sizeof(prefs_t));

    if ((prefs.version != VERSION) || (prefs.prefs_size != sizeof(prefs_t))) {

        log_output_P(LOG_MODULE_PREFERENCES, LOG_LEVEL_INFO, "Using default settings");

        memcpy_P(&prefs, &prefs_default, sizeof(prefs_t));

    }

    log_output_callback(LOG_MODULE_PREFERENCES, LOG_LEVEL_INFO, preferences_output);
}

/**
 * @brief Returns pointer to copy of the preferences hold in SRAM
 *
 * This returns a pointer to the {@link #prefs workign copy} of the
 * {@link #prefs_t preferences} hold in SRAM. It can be used to access and
 * manipulate the preferences. Once data has been changed,
 * {@link preferences_save()} needs to be invoked in order to write the changes
 * to EEPROM.
 *
 * @return Reference to prefs
 *
 * @see prefs
 * @see preferences_save()
 */
prefs_t* preferences_get()
{

    return &prefs;

}

/**
 * @brief Saves manipulated preferences by writing it back to the EEPROM
 *
 * This initiates the writeback to the EEPROM. Essentially it is only a wrapper
 * around {@link eeprom_put_block()}.
 *
 * @return True if preferences were saved successfully, false otherwise
 *
 * @see prefs
 */
bool preferences_save()
{

    log_output_P(LOG_MODULE_PREFERENCES, LOG_LEVEL_INFO, "Initiated saving");

    eeprom_put_block(&prefs, &prefs_eeprom, sizeof(prefs_t));

    // TODO: Check whether data was actually written successfully
    return true;

}
