/*
 * Copyright (C) 2012, 2013, 2014 Karol Babioch <karol@babioch.de>
 * Copyright (c) 2010 Frank Meyer - frank(at)fli4l.de
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
 * @file i2c_rtc.c
 * @brief Implementation of header declared in i2c_rtc.h
 *
 * The [RTC][1] (Real-time clock) is connected to the microcontroller via the
 * [I2C][2] bus. This module implements functions, which can then be used to
 * access the DS1307.
 *
 * Internally it makes use of `i2c_master.h` quite heavily.
 *
 * Refer to [3] for any details about the DS1307 itself.
 *
 * [1]: https://en.wikipedia.org/wiki/Real-time_clock
 * [2]: https://en.wikipedia.org/wiki/I2c
 * [3]: http://datasheets.maximintegrated.com/en/ds/DS1307.pdf
 *
 * @see i2c_rtc.h
 * @see i2c_master.h
 */

#include <util/twi.h>

#include "base.h"
#include "i2c_rtc.h"

/**
 * @brief Device address of the DS1307
 *
 * This is the "base" address of the DS1307. In order to write to the device
 * `TW_WRITE` needs to be added, for reading use `TW_READ` instead.
 */
#define I2C_RTC_DEV_ADDR 0xD0

/**
 * @brief Indicates whether this module has already been initialized
 *
 * This module needs to be initialized **before** it can actually be used. The
 * initialization is performed by `i2c_rtc_init()`. Only if properly
 * initialized the other functions will work correctly.
 *
 * @see i2c_rtc_init()
 */
static bool i2c_rtc_initialized = false;

/**
 * @brief Holds the status of the last operation on the I2C bus
 *
 * The functions within this module can fail for multiple reasons. This
 * variable will contain the status for some functions of this module. It can
 * then be retrieved using `i2c_rtc_get_status()`.
 *
 * The possible states can be found at [1], p. 224f, table 22-2 and [1],
 * p. 227f, table 22-3. A more compact overview in form of macros can be
 * found at [2]. This file is part of recent versions of the AVR C Runtime
 * Library (avr-libc).
 *
 * [1]: http://www.atmel.com/images/doc2545.pdf
 * [2]: http://www.nongnu.org/avr-libc/user-manual/group__util__twi.html#ga4440385d1818b4fe89b20341ea47b75a
 *
 * @see i2c_rtc_get_status()
 */
static uint8_t i2c_rtc_status;

/**
 * @brief DS1307 Output control
 *
 * This bit controls the output level of the SQW/OUT pin when the square-wave
 * output is disabled. If SQWE = 0, the logic level on the SQW/OUT pin is 1 if
 * OUT = 1 and is 0 if OUT = 0. On initial application of power to the device,
 * this bit is typically set to 0.
 *
 * @see CTRL_REG_SQWE
 * @see CTRL_REG
 */
#define CTRL_REG_OUT 1

/**
 * @brief DS1307 Square-Wave Enable (SQWE)
 *
 * This bit, when set to logic 1, enables the oscillator output. The frequency
 * of the square-wave output depends upon the value of the RS0 and RS1 bits.
 * With the square-wave output set to 1Hz, the clock registers update on the
 * falling edge of the square wave. On initial application of power to the
 * device, this bit is typically set to 0.
 *
 * @see CTRL_REG_RS0
 * @see CTRL_REG_RS1
 * @see CTRL_REG
 */
#define CTRL_REG_SQWE 1

/**
 * @brief DS1307 Rate Select (RS1)
 *
 * This is used in combination with `CTRL_REG_RS0`.
 *
 * These bits control the frequency of the square-wave output when the square-
 * wave output has been enabled. The following table lists the square-wave
 * frequencies that can be selected with the RS bits. On initial application
 * of power to the device, these bits are typically set to 1.
 *
 * The following table lists all possible combinations along with the
 * appropriate result:
 *
 * \code
 * RS1  RS0  SQW/OUT OUTPUT  SQWE  OUT
 *
 *  0    0    1 Hz            1     X
 *  0    1    4.096 kHz       1     X
 *  1    0    8.192 kHz       1     X
 *  1    1    32.768 kHz      1     X
 *  X    X    0               0     0
 *  X    X    1               0     1
 * \endcode
 *
 * @see CTRL_REG_RS0
 * @see CTRL_REG
 */
#define CTRL_REG_RS1 1

/**
 * @brief DS1307 Rate Select (RS0)
 *
 * Refer to `CTRL_REG_RS0` for details about the meaning of this bit.
 *
 * @see CTRL_REG_RS0
 * @see CTRL_REG
 *
 */
#define CTRL_REG_RS0 1

/**
 * @brief Holds the actual value for the control register
 *
 * This is a combination of the macros defined beforehand. This is the value
 * that will end up in the RTC itself during the initialization.
 *
 * @see CTRL_REG_OUT
 * @see CTRL_REG_SQWE
 * @see CTRL_REG_RS0
 * @see CTRL_REG_RS1
 * @see i2c_rtc_init()
 */
#define CTRL_REG \
    (CTRL_REG_RS0 | CTRL_REG_RS0 << 1 | CTRL_REG_SQWE << 4 | CTRL_REG_OUT << 7)

/**
 * @brief Returns the status of the last operation on the I2C bus
 *
 * This returns the status of the last operation performed on the I2C bus in
 * the context of this module.
 *
 * @see i2c_rtc_status
 */
uint8_t i2c_rtc_get_status()
{

    return i2c_rtc_status;

}

/**
 * @brief Writes the given datetime to the RTC
 *
 * This writes the given date and time to the RTC. As the RTC works with BCD
 * internally the values need to be converted before they are actually written
 * to the appropriate registers.
 *
 * @param datetime Pointer to memory holding the datetime to be written to RTC
 *
 * @return Result of the operation, true if successful, false otherwise
 *
 * @see i2c_rtc_sram_write()
 * @see itobcd()
 * @see datetime_t
 */
bool i2c_rtc_write(const datetime_t* datetime)
{

    uint8_t rtcbuf[7];
    bool rtc = false;

    if (i2c_rtc_initialized) {

        rtcbuf[0] = itobcd(datetime->ss);
        rtcbuf[1] = itobcd(datetime->mm);
        rtcbuf[2] = itobcd(datetime->hh);
        rtcbuf[3] = itobcd(datetime->WD);
        rtcbuf[4] = itobcd(datetime->DD);
        rtcbuf[5] = itobcd(datetime->MM);
        rtcbuf[6] = itobcd(datetime->YY);

        if (i2c_rtc_sram_write(0x00, rtcbuf, 7)) {

            rtc = true;

        }

    }

    return rtc;

}

/**
 * @brief Reads the datetime from the RTC
 *
 * This reads the current date and time from the RTC and puts it into the given
 * buffer. As the RTC works with BCD internally the values need to converted to
 * its appropriate binary representation before they are written to the buffer.
 *
 * @param datetime Pointer to buffer in memory for storing the read datetime
 *
 * @return Result of the operation, true if successful, false otherwise
 *
 * @see bcdtoi()
 * @see i2c_rtc_sram_read()
 * @see datetime_t
 */
bool i2c_rtc_read(datetime_t* datetime)
{

    uint8_t rtcbuf[7];
    bool rtc = false;

    if (i2c_rtc_initialized) {

        if (i2c_rtc_sram_read(0x00, rtcbuf, 7)) {

            datetime->YY = bcdtoi(rtcbuf[6]);
            datetime->MM = bcdtoi(rtcbuf[5]);
            datetime->DD = bcdtoi(rtcbuf[4]);
            datetime->WD = bcdtoi(rtcbuf[3]);
            datetime->hh = bcdtoi(rtcbuf[2]);
            datetime->mm = bcdtoi(rtcbuf[1]);
            datetime->ss = bcdtoi(rtcbuf[0]);

            rtc = true;

        }

    }

    return rtc;

}

/**
 * @brief Writes data to the SRAM of the RTC
 *
 * This writes the data pointed to by `data` into the SRAM of the RTC. The
 * length of the data is specified by `length`. `address` specifies the
 * starting point within the SRAM of the RTC.
 *
 * The return value indicates whether the operation was performed successfully.
 *
 * @note Only addresses ranging from 0x8 to 0x3f are meant to be used for
 * general purposes, as the lower 7 bytes contain the date and time itself.
 *
 * @param address Starting location in SRAM of the RTC
 * @param data Pointer to buffer in memory containing the actual data
 * @param length Length of the data to be written
 *
 * @return Result of the operation, true if successful, false otherwise
 *
 * @see i2c_master_write()
 */
bool i2c_rtc_sram_write(uint8_t address, void* data, uint8_t length)
{

    uint8_t* value = data;
    bool rtc = false;

    if (i2c_rtc_initialized) {

        if (length && (address + length <= 64)) {

            i2c_master_start_wait(I2C_RTC_DEV_ADDR + TW_WRITE);

            if (i2c_master_write(address, &i2c_rtc_status)) {

                rtc = true;

                while (length--) {

                    if (!i2c_master_write(*value++, &i2c_rtc_status)) {

                        rtc = false;

                        break;

                    }

                }

            }

            i2c_master_stop();

        }

    }

    return rtc;

}

/**
 * @brief Reads data from the SRAM of the RTC
 *
 * This reads data from the SRAM location of the RTC specified by `addr` and
 * puts it into the buffer pointed to by `data`. The length of the data to be
 * read is by the argument `length`.
 *
 * The return value indicates whether the operation was performed successfully.
 *
 * @note The buffer the data will be put in should obviously be big enough to
 * hold all of the requested data.
 *
 * @note Only addresses ranging from 0x8 to 0x3f are meant to be used for
 * general purposes, as the lower 7 bytes contain the date and time itself.
 *
 * @param address Starting location in SRAM of the RTC
 * @param data Pointer to buffer in memory for holding the data
 * @param length Length of the data to be read
 *
 * @return Result of the operation, true if successful, false otherwise
 *
 * @see i2c_master_read_ack()
 * @see i2c_master_read_nak()
 */
bool i2c_rtc_sram_read(uint8_t address, void* data, uint8_t length)
{

    uint8_t* value = data;
    bool rtc = false;

    if (i2c_rtc_initialized) {

        if (length && (address + length <= 64)) {

            i2c_master_start_wait(I2C_RTC_DEV_ADDR + TW_WRITE);

            if (i2c_master_write(address, &i2c_rtc_status)) {

                if (i2c_master_rep_start(I2C_RTC_DEV_ADDR + TW_READ, &i2c_rtc_status)) {

                    rtc = true;

                    while (--length) {

                        *value++ = i2c_master_read_ack();

                    }

                    *value++ = i2c_master_read_nak();

                }

            }

            i2c_master_stop();

        }

    }

    return rtc;

}

/**
 * @brief Initializes this module along with the RTC itself
 *
 * This initializes the module by writing the defined options into the control
 * register of the RTC. It makes also sure that the CH bit (bit 7 of register
 * 0) is set to 0, so the **clock** of the RTC is **not** being halted.
 *
 * If the initialization could be performed successfully, this function will
 * return true. Otherwise false will be returned and the appropriate error
 * code is written to the location pointed to by `error`.
 *
 * @param error Pointer to memory holding possible error code
 *
 * @return Result of the operation, true if successful, false otherwise
 *
 * @see CTRL_REG
 * @see i2c_rtc_initialized
 * @see i2c_rtc_sram_write()
 * @see i2c_master_error_t
 * @see i2c_rtc_status
 */
bool i2c_rtc_init(i2c_master_error_t* error)
{

    i2c_rtc_status = 0;

    if (i2c_master_init(error)) {

        i2c_rtc_initialized = true;
        uint8_t ctrlreg = CTRL_REG;

        if (i2c_rtc_sram_write(0x07, &ctrlreg, 1)) {

            uint8_t seconds;

            if (i2c_rtc_sram_read(0x00, &seconds, 1)) {

                if (seconds & _BV(7)) {

                    seconds &= ~_BV(7);
                    i2c_rtc_sram_write(0x00, &seconds, 1);

                }

            }

            return true;


        } else {

            *error = I2C_MASTER_ERROR_SLAVE_NOT_FOUND;

        }

    }

    return false;

}
