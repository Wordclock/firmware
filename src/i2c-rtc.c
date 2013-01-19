/*
 * Copyright (C) 2012, 2013 Karol Babioch <karol@babioch.de>
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
 * @file i2c-rtc.c
 * @brief Implementation of header declared in i2c-rtc.h
 *
 * The [RTC][1] (Real-time clock) is connected to the microcontroller via the
 * [I2C][2] bus. This module implements functions, which can then be used to
 * access the [DS1307][3].
 *
 * Internally it makes use of i2c-master.h quite heavily.
 *
 * [1]: https://en.wikipedia.org/wiki/Real-time_clock
 * [2]: https://en.wikipedia.org/wiki/I2c
 * [3]: http://datasheets.maximintegrated.com/en/ds/DS1307.pdf
 *
 * @see i2c-rtc.h
 * @see i2c-master.h
 */

#include <avr/io.h>
#include <util/delay.h>

#include "main.h"
#include "base.h"
#include "i2c-master.h"
#include "i2c-rtc.h"

/**
 * @brief Device address of the DS1307
 *
 * This is the "base" address of the DS1307, see [1], p. 12. In order to write
 * to the device I2C_WRITE needs to be added, for reading use I2C_READ instead.
 *
 * [1]: http://datasheets.maximintegrated.com/en/ds/DS1307.pdf
 *
 * @see I2C_READ
 * @see I2C_WRITE
 */
#define DEVRTC 0xD0

/**
 * @brief Indicates whether this module has already been initialized
 *
 * This module needs to be initialized **before** it can actually be used. In
 * order to do so, i2c_rtc_init() has to be called. Various functions check
 * whether the module has already been initialized and return false if it
 * hasn't.
 *
 * @see i2c_rtc_init()
 */
static bool rtc_initialized = false;

/**
 * @brief Holds the status of the I2C bus of the last operation
 *
 * Various functions can fail for multiple reasons. This variable holds the
 * status of some functions. It can be retrieved using i2c_rtc_get_stats().
 *
 * The possible states can be found at [1], p. 224f, table 22-2 and [1],
 * p. 227f, table 22-3. A more compact overview in form of macros can be
 * found at [2]. This file is part of recent versions of the AVR C Runtime
 * Library (avr-libc). The file can be included using <util/twi.h>.
 *
 * [1]: http://www.atmel.com/images/doc2545.pdf
 * [2]: http://www.nongnu.org/avr-libc/user-manual/group__util__twi.html#ga4440385d1818b4fe89b20341ea47b75a
 *
 * @see <util/twi.h>
 * @see i2c_rtc_get_status()
 */
static uint8_t i2c_rtc_status;

/**
 * @brief DS1307 Output Control bit
 *
 * *This bit controls the output level of the SQW/OUT pin when the square-wave
 * output is disabled. If SQWE = 0, the logic level on the SQW/OUT pin is 1 if
 * OUT = 1 and is 0 if OUT = 0. On initial application of power to the device,
 * this bit is typically set to a 0.*, cp. [1].
 *
 * [1]: http://datasheets.maximintegrated.com/en/ds/DS1307.pdf
 *
 * @see CTRL_REG_SQWE
 * @see ctrlreg
 */
#define CTRL_REG_OUT 0x80

/**
 * @brief DS1307 Square-Wave Enable (SQWE) bit
 *
 * *This bit, when set to logic 1, enables the oscillator output. The frequency
 * of the square-wave output depends upon the value of the RS0 and RS1 bits.
 * With the square-wave output set to 1Hz, the clock registers update on the
 * falling edge of the square wave. On initial application of power to the
 * device, this bit is typically set to a 0.*, cp. [1].
 *
 * [1]: http://datasheets.maximintegrated.com/en/ds/DS1307.pdf
 *
 * @see CTRL_REG_RS0
 * @see CTRL_REG_RS1
 * @see ctrlreg
 */
#define CTRL_REG_SQWE 0x10

/**
 * @brief DS1307 Rate Select (RS1) bit
 *
 * This usually has to be combined with CTRL_REG_RS0
 *
 * *These bits control the frequency of the square-wave output when the square-
 * wave output has been enabled. The following table lists the square-wave
 * frequencies that can be selected with the RS bits. On initial application
 * of power to the device, these bits are typically set to a 1.*, cp. [1].
 *
 * The following combinations of RS1 and RS0 are possible:
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
 * @see ctrlreg
 */
#define CTRL_REG_RS1 0x02

/**
 * @brief DS1307 Rate Select (RS0) bit
 *
 * This is usually used in combination with CTRL_REG_RS0. Take a look at
 * CTRL_REG_RS0 for details.
 *
 * @see CTRL_REG_RS0
 * @see ctrlreg
 *
 */
#define CTRL_REG_RS0 0x01

/**
 * @brief Holds the value of the control register
 *
 * This is a combination of the macros defined above. It will set up the RTC
 * according to this options and will store it in the control register of
 * the RTC itself, so it persists even when it is powered off. This is done
 * during the initialization, see i2c_rtc_init().
 *
 * The control register of the DS1307 is located at memory address 0x07,
 * see [1], p. 8.
 *
 * [1]: http://datasheets.maximintegrated.com/en/ds/DS1307.pdf
 *
 * @see CTRL_REG_OUT
 * @see CTRL_REG_SQWE
 * @see CTRL_REG_RS0
 * @see CTRL_REG_RS0
 * @see i2c_rtc_init()
 */
#define CTRL_REG CTRL_REG_OUT

/**
 * @brief Returns the status of the I2C bus of the last operation
 *
 * This returns the status of the last operation performed on the I2C bus in
 * the context of this module. See i2c_rtc_stats for details.
 *
 * @see i2c_rtc_status
 */
uint8_t i2c_rtc_get_status(void)
{

    return i2c_rtc_status;

}

/**
 * @brief Writes date & time to the RTC
 *
 * This writes a new date & time to the RTC, which it then will store and work
 * with it from there on. The argument is a pointer to a buffer in form of the
 * struct datetime_t, which holds the date & time to write.
 *
 * @param datetime Pointer to memory location holding the date & time to write
 * @return Result of the operation, true if successful, false else
 * @see datetime_t
 */
bool i2c_rtc_write(const datetime_t * datetime)
{

    uint8_t rtcbuf[7];
    bool rtc = false;

    if (rtc_initialized) {

        rtcbuf[0] = itobcd(datetime->ss);
        rtcbuf[1] = itobcd(datetime->mm);
        rtcbuf[2] = itobcd(datetime->hh);
        rtcbuf[3] = itobcd(datetime->wd) + 1;
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
 * @brief Reads date & time from the RTC
 *
 * This reads the date & time from the RTC and puts it into the given buffer in
 * memory in form of the struct datetime_t.
 *
 * @param datetime Pointer to buffer in memory for storing the read date & time
 * @return Result of the operation, true if successful, false else
 * @see datetime_t
 */
bool i2c_rtc_read(datetime_t * datetime)
{

    uint8_t rtcbuf[7];
    bool rtc = false;

    if (rtc_initialized) {

        if (i2c_rtc_sram_read(0x00, rtcbuf, 7)) {

            datetime->YY = bcdtoi(rtcbuf[6]);
            datetime->MM = bcdtoi(rtcbuf[5]);
            datetime->DD = bcdtoi(rtcbuf[4]);
            datetime->wd = bcdtoi(rtcbuf[3]) - 1;
            datetime->hh = bcdtoi(rtcbuf[2]);
            datetime->mm = bcdtoi(rtcbuf[1]);
            datetime->ss = bcdtoi(rtcbuf[0]);

            rtc = true;

        }

    }

    return rtc;

}

/**
 * @brief Writes given data into the SRAM of the RTC
 *
 * This writes data pointed at by "void_valuep" and of the length specified
 * by the parameter "length" into the SRAM of the RTC. The SRAM address can
 * be specified using the "addr" argument.
 *
 * You should only use addresses ranging from 0x8 to 0x3f, which are meant for
 * general purpose. Addresses ranging from 0x0 to ox7 contain the various
 * registers of the RTC itself, see [1], p. 8.
 *
 * This function internally makes use of various function declared in
 * i2c-master.h and will return false if there is some kind of an error with
 * the I2C bus.
 *
 * [1]: http://datasheets.maximintegrated.com/en/ds/DS1307.pdf
 *
 * @warning There are only 56 bytes to write to for general purpose.
 *
 * @param addr The address in SRAM of the RTC you want start to write to
 * @param void_valuep Pointer to buffer in memory containing the data to store
 * @param length The length the data you want to store
 * @return Result of the operation, true if successful, false else
 * @see i2c_rtc_sram_read()
 */
bool i2c_rtc_sram_write(uint8_t addr, void* void_valuep, uint8_t length)
{

    unsigned char* valuep = void_valuep;
    bool rtc = false;

    if (rtc_initialized) {

        if (length && addr + length <= 64) {

            i2c_master_start_wait(DEVRTC + I2C_WRITE);

            if (i2c_master_write(addr, &i2c_rtc_status) == 0) {

                rtc = true;

                while (length--) {

                    if (i2c_master_write(*valuep++, &i2c_rtc_status) != 0) {

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
 * @brief Reads data from the SRAM of the RTC and puts it into a buffer
 *
 * This reads data from the SRAM of the RTC addressed by "addr" and puts
 * it into the buffer in memory pointed at by "void_valuep". The length
 * of the data to be read can be specified by the argument called "length".
 *
 * The buffer the data is put in, should obviously be big enough to hold the
 * requested data.
 *
 * You should only use addresses ranging from 0x8 to 0x3f, which are meant for
 * general purpose. Addresses ranging from 0x0 to ox7 contain the various
 * registers of the RTC itself, see [1], p. 8.
 *
 * This function internally makes use of various function declared in
 * i2c-master.h and will return false if there is some kind of an error with
 * the I2C bus.
 *
 * [1]: http://datasheets.maximintegrated.com/en/ds/DS1307.pdf
 *
 * @warning There are only 56 bytes to read from for general purpose.
 *
 * @param addr The address in SRAM of the RTC you want start to read from
 * @param void_valuep Pointer to buffer in memory for holding the data
 * @param length The length the data you want to read
 * @return Result of the operation, true if successful, false else
 * @see i2c_rtc_sram_write()
 * @see i2c_master_write()
 */
bool i2c_rtc_sram_read(uint8_t addr, void* void_valuep, uint8_t length)
{

    unsigned char* valuep = void_valuep;
    bool rtc = false;

    if (rtc_initialized) {

        if (length && (addr + length <= 64)) {

            i2c_master_start_wait(DEVRTC + I2C_WRITE);

            if (i2c_master_write(addr, &i2c_rtc_status) == 0) {

                if (i2c_master_rep_start(DEVRTC + I2C_READ, &i2c_rtc_status) == 0) {

                    rtc = true;

                    while (--length) {

                        *valuep++ = i2c_master_read_ack();

                    }

                    *valuep++ = i2c_master_read_nak();

                }

            }

            i2c_master_stop();

        }

    }

    return rtc;

}

/**
 * @brief Initializes this module as well as the RTC itself
 *
 * This initializes the module by writing the defined options into the control
 * register of the RTC. Furthermore it sets "rtc_initialized" to true, so
 * other functions of this module can be used.
 *
 * If there is some sort of error with the I2C bus this function will return
 * false.
 *
 * @return Result of the operation, true if successful, false else
 * @see ctrlreg
 * @see rtc_initialized
 * @see i2c_rtc_sram_write()
 */
bool i2c_rtc_init(uint8_t* errorcode_p, uint8_t* status_p)
{

    bool rtc = false;
    uint8_t seconds;

    *status_p = 0xff;
    *errorcode_p = i2c_master_init();

    if (*errorcode_p == 0) {

        rtc_initialized = true;

        uint8_t ctrlreg = CTRL_REG;

        if (i2c_rtc_sram_write(0x07, &ctrlreg, 1)) {

            rtc = true;

            if (i2c_rtc_sram_read(0x00, &seconds, 1)) {

                if (seconds & 0x80) {

                    seconds &= ~0x80;
                    (void)i2c_rtc_sram_write(0x00, &seconds, 1);

                }

            }

        } else {

            *errorcode_p = I2C_ERROR_SLAVE_NOT_FOUND;
            *status_p = i2c_rtc_status;

        }

    }

    return rtc;

}
