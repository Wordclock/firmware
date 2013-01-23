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
 * These settings are used whenever the basic integrety check fails, see
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
 * @brief Working copy hold in SRAM backed by the content from EEPROM
 *
 * This variable is stored in SRAM and can be used just like every other
 * variable, too. It will filled with the contents from EEPROM during
 * initialization, see wcEeprom_init(). In order for changes to this variable
 * to be stored persistently in EEPROM, wcEeprom_writeback() has to be used.
 *
 * @see wcEeprom_init()
 * @see wcEeprom_writeback()
 * @see eepromParams
 */
WcEepromData g_epromWorking;

#if (LOG_EEPROM_INIT == 1) || (LOG_EEPROM_WRITEBACK == 1)

    /**
     * @brief Outputs a byte to UART in hexadecimal notation
     *
     * This can be used for debugging purposes. It will convert the byte into
     * its hexadecimal notation and will output it using the UART module.
     *
     * @see uart_putc()
     * @see nibbleToHex()
     */
    static void uart_putHexByte(uint8_t byte)
    {

        uart_putc(nibbleToHex(byte >> 4));
        uart_putc(nibbleToHex(byte & 0xf));

    }

#endif

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
void wcEeprom_init(void)
{

    /*
     * Copy content from EEPROM into SRAM
     */
    eeprom_read_block(&g_epromWorking, &eepromParams, sizeof(eepromParams));

    /*
     * Basic integrity check: Check whether there are differences between
     * either the SW_VERSION or the size of g_epromWorking between the
     * version actually running and the one stored in EEPROM.
     */
    if ((g_epromWorking.swVersion != SW_VERSION)
        || (g_epromWorking.structSize != sizeof(g_epromWorking))) {

        #if (LOG_EEPROM_INIT == 1)

            uart_puts_P("Using default settings\n");

        #endif

        /*
         * Copy default settings into g_epromWorking
         */
        memcpy_P(&g_epromWorking, &eepromDefaultParams_P, sizeof(WcEepromData));

    }

    #if (LOG_EEPROM_INIT == 1)

        uart_puts_P("EEPROM: ");

        /*
         * Iterate over EEPROM content and output to UART
         */

        uint8_t* ptr = (uint8_t*)(&g_epromWorking);

        for(uint8_t i = 0; i < sizeof(eepromParams); i++){

            uart_putHexByte(*ptr++);

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
WcEepromData* wcEeprom_getData(void)
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
 * @see uart_putHexByte()
 */
static bool wcEeprom_writeIfChanged(uint8_t index)
{

    uint8_t eepromByte;
    uint8_t sramByte;

    /*
     * Get the content of the byte at the given index from both, the SRAM
     * and EEPROM
     */

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
            uart_putHexByte(eepromByte);
            uart_puts_P(", SRAM: ");
            uart_putHexByte(sramByte);
            uart_putc('\n');

        #endif

        /*
         * Content of EEPROM and SRAM are different, therefore we need to
         * write the new value into EEPROM.
         */
        eeprom_write_byte(eepromAdress, sramByte);

        return true;

    }

    return false;

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
 * @param start_p Pointer to the start of the data that has to be written back
 * @param len The length of the data that has to be written back
 *
 * @see wcEeprom_writeIfChanged()
 * @see WcEepromData
 */
void wcEeprom_writeback(const void* start_p, uint8_t len)
{

    /*
     * Calculate the index represented by the start pointer.
     */
    uint8_t eepromIndex = (((uint8_t*)start_p) - ((uint8_t*)&g_epromWorking));
    uint8_t eepromIndexEnd = eepromIndex + len - 1;

    #if (LOG_EEPROM_WRITEBACK == 1)

        uart_puts_P("EEPROM write: Index: ");
        uart_putHexByte(eepromIndex);
        uart_puts_P(", Length: ");
        uart_putHexByte(len);
        uart_putc('\n');

    #endif

    /*
     * Iterate over each index and write changes back
     */
    while(eepromIndex <= eepromIndexEnd) {

        wcEeprom_writeIfChanged(eepromIndex++);

    }

}
