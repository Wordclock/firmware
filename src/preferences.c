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
 * During initialization this modules copies over the content of the EEPROM
 * into the SRAM and makes it available to other modules. Once the data has
 * been changed, a writeback needs to be initiated in order for the data to
 * be written back to the EEPROM.
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

#include "preferences.h"
#include "uart.h"
#include "base.h"
#include "version.h"

/**
 * @brief Represents the data stored persistently within EEPROM
 *
 * @see prefs_t
 */
static prefs_t EEMEM eepromParams;

/**
 * @brief Default settings for prefs_t stored within program space
 *
 * These are the default settings for `prefs_t`, which are stored within
 * program space and will get used whenever the data in the EEPROM is
 * considered to be invalid.
 *
 * @see prefs_t
 * @see preferences_init()
 * @see USEREEPROMPARAMS_DEFAULT
 * @see DISPLAYEEPROMPARAMS_DEFAULT
 * @see PWMEEPROMPARAMS_DEFAULT
 * @see VERSION
 */
static const prefs_t PROGMEM eepromDefaultParams_P = {

    USEREEPROMPARAMS_DEFAULT,
    DISPLAYEEPROMPARAMS_DEFAULT,
    PWMEEPROMPARAMS_DEFAULT,
    VERSION,
    (uint8_t)sizeof(prefs_t),

};

/**
 * @brief Copy of data hold in SRAM backed by the content of EEPROM
 *
 * This is holding all of the data defined by `prefs_t` in SRAM. It will
 * be filled with the appropriate content from EEPROM during initialization and
 * can be accessed by other modules using `preferences_get()`. Once changes
 * to it are made, `preferences_save()` needs to be invoked in order for the
 * changes to be written back.
 *
 * @see prefs_t
 * @see preferences_init()
 * @see preferences_get()
 * @see preferences_save()
 */
static prefs_t g_epromWorking;

/**
 * @brief Initializes this module by copying over the content of the EEPROM
 *
 * This has to be called **before** any other functions of this module can be
 * used.
 *
 * It reads in the contents of EEPROM into `g_epromWorking` and performs some
 * basic integrity checks. These include:
 *
 * - Compare software version stored in EEPROM against VERSION
 * - Compare struct size stored in EEPROM against prefs_t::prefs_size
 *
 * When this check fails, the default values (`eepromDefaultParams_P`) will be
 * used. Otherwise the data from EEPROM is considered to be valid and will end
 * up being used.
 *
 * @see prefs_t::version
 * @see VERSION
 * @see g_epromWorking
 * @see eepromParams
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
 * @brief Returns pointer to working copy prefs_t
 *
 * This returns a pointer to `g_epromWorking` and can be used to access the
 * data for reading and/or writing. Once data has been changed,
 * `preferences_save()` needs to be invoked in order to write it back to the
 * EEPROM.
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
 * EEPROM. This improves the performance as well as the expected lifespan of
 * the EEPROM itself as the amount of erase/write cycles is limited.
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
 * @brief Initiates the writeback to EEPROM
 *
 * This initiates the writeback to the EEPROM in order for data to be stored
 * persistently. This function will iterates over {@link g_epromWorking all}
 * preferences and writes changes back to the EEPROM.
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
