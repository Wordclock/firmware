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
 * @file wceeprom.c
 * @brief Implementation of the header declared in wceeprom.h
 *
 * This idea behind this module is to keep a copy of the content of the EEPROM
 * (see [1]) within the SRAM, see g_epromWorking. g_epromWorking will be
 * initialized during bootup. It will either contain the content of the EEPROM
 * and/or if there is some sort of an error regarding the integrity (e.g.
 * different size and/or different SW_VERSION) it will contain the default
 * parameters defined in eepromDefaultParams_P.
 *
 * g_epromWorking can basically be changed like any other variable.
 * wcEeprom_writeback() can then be used to write these changes back to EEPROM
 * in order to make them persistent.
 *
 * For further information about the EEPROM memory integrated into the AVR
 * microcontroller see [2], p. 20f, chapter 8.4. Furhtermore it might be useful
 * to take a look at [3] as these functions are used quite heavily within this
 * module.
 *
 * [1]: https://en.wikipedia.org/wiki/EEPROM
 * [2]: http://www.atmel.com/images/doc2545.pdf
 * [3]: http://www.nongnu.org/avr-libc/user-manual/group__avr__eeprom.html
 *
 * @see SW_VERSION
 * @see WcEepromData::structSize
 * @see eepromDefaultParams_P
 * @see wcEeprom_writeback()
 * @see wceeprom.h
 */

#include <avr/eeprom.h>
#include <avr/pgmspace.h>
#include <avr/interrupt.h>
#include <stdbool.h>

#include "wceeprom.h"
#include "uart.h"
#include "base.h"

#if (LOG_EEPROM_WRITEBACK == 1)

    /**
     * @brief Used to output logging information for changes written back to
     *  EEPROM
     *
     * When the logging is enabled (LOG_EEPROM_WRITEBACK == 1), this macro is
     * used to output changes that are written back to EEPROM.
     *
     * @see LOG_EEPROM_WRITEBACK
     */
    #define log_eeprom(x) uart_puts_P(x)

#else

    /**
     * @brief Used to output logging information for changes written back to
     *  EEPROM
     *
     * When the logging is disabled (LOG_EEPROM_WRITEBACK == 0), this macro is
     * used, so that actually there is nothing being output. This allows to
     * use log_eeprom(x) in the code without needing to check whether the
     * logging is enabled over and over again.
     *
     * @see LOG_EEPROM_WRITEBACK
     */
    #define log_eeprom(x)

#endif

/**
 * @brief Represents the settings to be stored persistently within the EEPROM
 *
 * @see wcEeprom_init()
 * @see wcEeprom_writeback()
 * @see g_epromWorking
 */
WcEepromData EEMEM eepromParams;

/**
 * @brief The default settings to be used in case the content from EEPROM is
 *   invalid
 *
 * These settings are used whenever the basic integrity check fails, see
 * wcEeprom_init().
 *
 * @see wcEeprom_init()
 */
const WcEepromData PROGMEM eepromDefaultParams_P = {

    USEREEPROMPARAMS_DEFAULT,
    DISPLAYEEPROMPARAMS_DEFAULT,
    PWMEEPROMPARAMS_DEFAULT,
    SW_VERSION,
    (uint8_t)sizeof(WcEepromData),

};

/**
 * @brief Copy of data hold in SRAM backed by the content of EEPROM
 *
 * This is holding all of the data defined by `WcEepromData` in SRAM. It will
 * be filled with the appropriate content from EEPROM during initialization and
 * can be accessed by other modules using `wcEeprom_getData()`. Once changes
 * to it are made, `wcEeprom_writeback()` needs to be invoked in order for the
 * changes to be written back.
 *
 * @see WcEepromData
 * @see wcEeprom_init()
 * @see wcEeprom_getData()
 * @see wcEeprom_writeback()
 */
WcEepromData g_epromWorking;

/**
 * @brief Initializes this module by copying over the content of the EEPROM
 *
 * This has to be called **before** any other functions of this module can be
 * used.
 *
 * It reads in the contents of EEPROM into `g_epromWorking` and performs some
 * basic integrity checks. These include:
 *
 * - Compare software version stored in EEPROM against SW_VERSION
 * - Compare struct size stored in EEPROM against WcEepromData::structSize
 *
 * When this check fails, the default values (`eepromDefaultParams_P`) will be
 * used. Otherwise the data from EEPROM is considered to be valid and will end
 * up being used.
 *
 * @see WcEepromData::swVersion
 * @see SW_VERSION
 * @see g_epromWorking
 * @see eepromParams
 */
void wcEeprom_init()
{

    eeprom_read_block(&g_epromWorking, &eepromParams, sizeof(eepromParams));

    if ((g_epromWorking.swVersion != SW_VERSION)
        || (g_epromWorking.structSize != sizeof(g_epromWorking))) {

        #if (LOG_EEPROM_INIT == 1)

            uart_puts_P("Using default settings\n");

        #endif

        memcpy_P(&g_epromWorking, &eepromDefaultParams_P, sizeof(WcEepromData));

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
 * @brief Returns a pointer to a copy of WcEepromData
 *
 * This returns a pointer to a variable of type WcEepromData. This variable
 * can then be used to get the persistently stored values. It can also be
 * used to modify the values. In order for the changes to get written back
 * into EEPROM, wcEeprom_writeback() needs to be called.
 *
 * @warning WcEepromData shouldn't become larger than 254 bytes for now, as
 *  the code right now uses a lot of 8 bit counters.
 *
 * @warning WcEepromData definitely shouldn't become bigger than the size of
 *  EEPROM itself, which is 512 bytes for the ATmega168.
 *
 * @see WcEepromData
 * @see wcEeprom_writeback()
 */
WcEepromData* wcEeprom_getData()
{

    return &g_epromWorking;

}

/**
 * @brief Writes byte at the given index to EEPROM if it has been changed
 *
 * In order to speed up the process of writing to EEPROM we only write bytes
 * to EEPROM that actually have been changed. This also increases the lifespan
 * of the EEPROM itself. If logging is enabled (LOG_EEPROM_WRITEBACK == 1) the
 * difference between the byte in SRAM and the one in EEPROM will be output,
 * too.
 *
 * This function will be invoked by wcEeprom_writeback() automatically.
 *
 * @see wcEeprom_writeback()
 * @see LOG_EEPROM_WRITEBACK
 * @see uint8ToHexStr()
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
 *  This initiates the writeback to the EEPROM in order for data to be stored
 *  persistently. This function will iterate over the range described by the
 *  parameters `start` and `length` and invoke `wcEeprom_writeIfChanged()` for
 *  each of them.
 *
 * @warning Because writing to EEPROM takes quite some time it is possible
 * that interrupts will be missed.
 *
 * @param start Pointer to the start of the data that has to be written back
 * @param length The length of the data that has to be written back
 *
 * @see wcEeprom_writeIfChanged()
 * @see g_epromWorking
 */
void wcEeprom_writeback(const void* start, uint8_t length)
{

    uint8_t eepromIndex = (((uint8_t*)start) - ((uint8_t*)&g_epromWorking));
    uint8_t eepromIndexEnd = eepromIndex + length - 1;

    #if (LOG_EEPROM_WRITEBACK == 1)

        char buf[3];

        uart_puts_P("EEPROM write: Index: ");
        uint8ToHexStr(eepromIndex, buf);
        uart_puts(buf);
        uart_puts_P(", Length: ");
        uint8ToHexStr(length, buf);
        uart_puts(buf);
        uart_putc('\n');

    #endif

    while (eepromIndex <= eepromIndexEnd) {

        wcEeprom_writeIfChanged(eepromIndex++);

    }

}
