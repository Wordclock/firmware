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
 * @file preferences.c
 * @brief Implementation of the header declared in preferences.h
 *
 * This implements the functionality declared in {@link preferences.h}. A copy
 * of the {@link prefs_t preferences} is always hold in SRAM. A reference to
 * this copy can be {@link #preferences_get() retrieved} and manipulated. Note,
 * however, that a {@link preferences_save() save} operation needs to be
 * invoked in order for the data to be written to the persistent storage
 * backend. For now the EEPROM coming along with the microcontroller is used to
 * store the data persistently.
 *
 * For details about how the EEPROM works in detail and how to access it from
 * within the program, refer to [1] and/or [2].
 *
 * [1]: http://www.atmel.com/images/doc2545.pdf
 * [2]: http://www.nongnu.org/avr-libc/user-manual/group__avr__eeprom.html
 *
 * @see preferences.h
 */

#include <avr/eeprom.h>
#include <avr/pgmspace.h>

#include "base.h"
#include "preferences.h"
#include "uart.h"
#include "version.h"

/**
 * @brief Represents the preferences stored persistently within EEPROM
 *
 * @see prefs_t
 */
static prefs_t EEMEM eepromParams;

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
static const prefs_t PROGMEM eepromDefaultParams_P = {

    USEREEPROMPARAMS_DEFAULT,
    DISPLAYEEPROMPARAMS_DEFAULT,
    PWMEEPROMPARAMS_DEFAULT,
    VERSION,
    (uint8_t)sizeof(prefs_t),

};

/**
 * @brief Copy of preferences hold in SRAM
 *
 * This is holding all of the {@link #prefs_t preferences} in SRAM. It is
 * backed by the content of the EEPROM and can be accessed globally using
 * {@link #preferences_get()}. Once changes to it are made,
 * {@link preferences_save()} needs to be invoked in order for the changes to
 * be written back to the persistent storage backend.
 *
 * @see prefs_t
 */
static prefs_t g_epromWorking;

/**
 * @brief Initializes this module
 *
 * This has to be called **before** any other functions of this module can be
 * used. It reads in the contents of EEPROM into {@link #g_epromWorking} and
 * performs a basic integrity check consisting of:
 *
 * - Comparison of software version stored in EEPROM against VERSION
 * - Comparison of struct size stored in EEPROM against prefs_t::prefs_size
 *
 * When this check fails, the {@link #eepromDefaultParams_P default values} are
 * used. Otherwise the data from the EEPROM is considered to be valid and will
 * end up being used.
 *
 * @see prefs_t::version
 * @see prefs_t::prefs_size
 */
void preferences_init()
{

    eeprom_read_block(&g_epromWorking, &eepromParams, sizeof(eepromParams));

    if ((g_epromWorking.version != VERSION)
        || (g_epromWorking.prefs_size != sizeof(g_epromWorking))) {

        #if (LOG_EEPROM_INIT == 1)

            uart_puts_P("Using default settings\n");

        #endif

        memcpy_P(&g_epromWorking, &eepromDefaultParams_P, sizeof(prefs_t));

    }

    #if (LOG_EEPROM_INIT == 1)

        uart_puts_P("EEPROM: ");

        uint8_t* ptr = (uint8_t*)(&g_epromWorking);

        for (uint8_t i = 0; i < sizeof(eepromParams); i++) {

            char buf[3];

            uint8ToHexStr(*ptr++, buf);
            uart_puts(buf);

        }

        uart_putc('\n');

     #endif
}

/**
 * @brief Returns pointer to copy of the preferences hold in SRAM
 *
 * This returns a pointer to the {@link #g_epromWorking workign copy} of the
 * {@link #prefs_t preferences} hold in SRAM. It can be used to access and
 * manipulate the preferences. Once data has been changed,
 * {@link preferences_save()} needs to be invoked in order to write the changes
 * to the persistent storage backend.
 *
 * @return Reference to g_epromWorking
 *
 * @see g_epromWorking
 * @see preferences_save()
 */
prefs_t* preferences_get()
{

    return &g_epromWorking;

}

/**
 * @brief Writes byte at given index back to EEPROM - if it has been changed
 *
 * This writes the byte at the given index into EEPROM. Only bytes that have
 * changed compared to their EEPROM counterpart, will actually be written to
 * the EEPROM. This improves the performance as well as the expected lifespan
 * of the EEPROM itself as the amount of erase/write cycles is limited.
 *
 * @param index Index of byte to be written, 0 - sizeof(prefs_t)
 *
 * @return True if byte at given was written to EEPROM, false otherwise
 *
 * @see preferences_save()
 */
static bool wcEeprom_writeIfChanged(uint8_t index)
{

    uint8_t eepromByte;
    uint8_t sramByte;

    uint8_t* eepromAdress = ((uint8_t*)&eepromParams) + index;

    eepromByte = eeprom_read_byte(eepromAdress);
    sramByte = *(((uint8_t*)&g_epromWorking) + index);

    if (eepromByte != sramByte) {

        #if (LOG_EEPROM_WRITEBACK == 1)

            char buf[5];

            uart_puts_P("EEPROM byte ");
            uint16ToHexStr((uint16_t)eepromAdress, buf);
            uart_puts(buf);
            uart_puts_P(", EEPROM: ");
            uint8ToHexStr(eepromByte, buf);
            uart_puts(buf);
            uart_puts_P(", SRAM: ");
            uint8ToHexStr(eepromByte, buf);
            uart_puts(buf);
            uart_putc('\n');

        #endif

        eeprom_write_byte(eepromAdress, sramByte);

        return true;

    }

    return false;

}

/**
 * @brief Saves manipulated data by writing it back to the storage backend
 *
 * This initiates the writeback to the persistend storage backend. It iterates
 * over {@link g_epromWorking all} preferences and invokes
 * {@link wcEeprom_writeIfChanged()} for each byte.
 *
 * @return True if preferences were saved successfully, false otherwise
 *
 * @see wcEeprom_writeIfChanged()
 * @see g_epromWorking
 */
bool preferences_save()
{

    #if (LOG_EEPROM_WRITEBACK == 1)

        uart_puts_P("Initiating writeback\n");

    #endif

    for (uint8_t i = 0; i < sizeof(prefs_t); i++) {

        wcEeprom_writeIfChanged(i);


    }

    // TODO: Check whether data was actually written successfully
    return true;

}
