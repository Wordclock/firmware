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
 * @file i2c_rtc.c
 * @brief Implementation of header declared in i2c_rtc.h
 *
 * The [RTC][1] (Real-time clock) is connected to the microcontroller via the
 * [I2C][2] bus. This module implements functions, which can then be used to
 * access the [DS1307][3].
 *
 * Internally it makes use of i2c_master.h quite heavily.
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
#include "i2c_master.h"
#include "i2c_rtc.h"

/**
 * @brief Device address of the DS1307
 *
 * This is the "base" address of the DS1307, see [1], p. 12. In order to write
 * to the device TW_WRITE needs to be added, for reading use TW_READ instead.
 *
 * [1]: http://datasheets.maximintegrated.com/en/ds/DS1307.pdf
 *
 * @see TW_READ
 * @see TW_WRITE
 */
#define DEVRTC 0xD0

/**
 * @brief Indicates whether this module has already been initialized
 *
 * This module needs to be initialized **before** it can actually be used. This
 * is achieved using i2c_rtc_init(). Various functions rely on a successfully
 * initialized RTC and will simply return false otherwise.
 *
 * @see i2c_rtc_init()
 */
static bool rtc_initialized = false;

/**
 * @brief Holds the status of the last operation on the I2C bus
 *
 * The functions within this module can fail for multiple reasons. This
 * variable will contain the status for some functions of this module. It can
 * then be retrieved using i2c_rtc_get_status().
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
 * this bit is typically set to a 0, cp. [1].
 *
 * [1]: http://datasheets.maximintegrated.com/en/ds/DS1307.pdf
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
 * device, this bit is typically set to a 0, cp. [1].
 *
 * [1]: http://datasheets.maximintegrated.com/en/ds/DS1307.pdf
 *
 * @see CTRL_REG_RS0
 * @see CTRL_REG_RS1
 * @see CTRL_REG
 */
#define CTRL_REG_SQWE 1

/**
 * @brief DS1307 Rate Select (RS1)
 *
 * This is used in combination with CTRL_REG_RS0.
 *
 * These bits control the frequency of the square-wave output when the square-
 * wave output has been enabled. The following table lists the square-wave
 * frequencies that can be selected with the RS bits. On initial application
 * of power to the device, these bits are typically set to a 1, cp. [1].
 *
 * The following table lists all combinations of RS1 and RS0 and the
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
 * [1]: http://datasheets.maximintegrated.com/en/ds/DS1307.pdf
 *
 * @see CTRL_REG_RS0
 * @see CTRL_REG
 */
#define CTRL_REG_RS1 1

/**
 * @brief DS1307 Rate Select (RS0)
 *
 * Refer to CTRL_REG_RS0 for details.
 *
 * @see CTRL_REG_RS0
 * @see CTRL_REG
 *
 */
#define CTRL_REG_RS0 1

/**
 * @brief Holds the actual value for the control register
 *
 * This is a combination of the macros defined above. It will set up the RTC
 * according to this options and will store it in the control register of
 * the RTC itself, so it persists even when it is powered off. This is done
 * during the initialization, see i2c_rtc_init().
 *
 * The internal control register of the DS1307 is located at address 0x07, see
 * [1], p. 8.
 *
 * [1]: http://datasheets.maximintegrated.com/en/ds/DS1307.pdf
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
 * This writes a new date and time to the RTC, which it then will store and
 * work with it from there on. The argument is a pointer to a buffer in form of
 * datetime_t, which holds the datetime to write.
 *
 * @param datetime Pointer to memory holding the datetime to write
 *
 * @return Result of the operation, true if successful, false otherwise
 *
 * @see datetime_t
 */
bool i2c_rtc_write(const datetime_t* datetime)
{

    uint8_t rtcbuf[7];
    bool rtc = false;

    if (rtc_initialized) {

        /*
         * Write converted fields of the provided buffer in form of datetime_t
         * into the allocated buffer. Special care is needed in case
         * of the "wd" field, which contains the day of the week.
         *
         * The definitions of datetime_t::wd and the one from the RTC itself
         * differ, as datetime_t starts counting at 0, whereas the RTC starts
         * at 1.
         */
        rtcbuf[0] = itobcd(datetime->ss);
        rtcbuf[1] = itobcd(datetime->mm);
        rtcbuf[2] = itobcd(datetime->hh);
        rtcbuf[3] = itobcd(datetime->wd) + 1;
        rtcbuf[4] = itobcd(datetime->DD);
        rtcbuf[5] = itobcd(datetime->MM);
        rtcbuf[6] = itobcd(datetime->YY);

        /*
         * The datetime information is located at the first 7 addresses of
         * the RTC SRAM. Write the previously calculated values to the RTC. If
         * nothing unexpected happens, the return value will be set to true.
         */
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
 * buffer in memory in form of datetime_t.
 *
 * @param datetime Pointer to buffer in memory for storing the read datetime
 *
 * @return Result of the operation, true if successful, false otherwise
 *
 * @see datetime_t
 */
bool i2c_rtc_read(datetime_t* datetime)
{

    uint8_t rtcbuf[7];
    bool rtc = false;

    if (rtc_initialized) {

        /*
         * Read first seven bytes from RTC, which contain the time and date
         * information we are interested in. Only proceed if we get a valid
         * response.
         */
        if (i2c_rtc_sram_read(0x00, rtcbuf, 7)) {

            /*
             * Put the response into the buffer after converting the BCD
             * encoded information. Special care is needed in case of the "wd"
             * field, which contains the day of the week.
             *
             * The definitions of datetime_t::wd and the one from the RTC
             * itself differ, as datetime_t starts counting at 0, whereas the
             * RTC starts at 1.
             */

            datetime->YY = bcdtoi(rtcbuf[6]);
            datetime->MM = bcdtoi(rtcbuf[5]);
            datetime->DD = bcdtoi(rtcbuf[4]);
            datetime->wd = bcdtoi(rtcbuf[3]) - 1;
            datetime->hh = bcdtoi(rtcbuf[2]);
            datetime->mm = bcdtoi(rtcbuf[1]);
            datetime->ss = bcdtoi(rtcbuf[0]);

            /*
             * Indicate success, which then will be returned
             */
            rtc = true;

        }

    }

    return rtc;

}

/**
 * @brief Writes the given data into the SRAM of the RTC
 *
 * This writes data pointed at by "void_valuep" and of the length specified
 * by the parameter "length" into the SRAM of the RTC. The SRAM address can
 * be specified using the "addr" argument.
 *
 * You should only use addresses ranging from 0x8 to 0x3f, which are meant for
 * general purpose. Addresses ranging from 0x0 to 0x7 contain the various
 * date and time registers of the RTC itself, see [1], p. 8.
 *
 * This function internally makes use of various functions declared in
 * i2c_master.h and will return false if there is some kind of an error in
 * regards to the I2C bus.
 *
 * [1]: http://datasheets.maximintegrated.com/en/ds/DS1307.pdf
 *
 * @warning There are only 56 bytes to write to for general purpose.
 *
 * @param addr The address in SRAM of the RTC you want start to write to
 * @param void_valuep Pointer to buffer in memory containing the data to store
 * @param length The length the data you want to store
 *
 * @return Result of the operation, true if successful, false otherwise
 *
 * @see i2c_rtc_sram_read()
 */
bool i2c_rtc_sram_write(uint8_t addr, void* void_valuep, uint8_t length)
{

    unsigned char* valuep = void_valuep;
    bool rtc = false;

    if (rtc_initialized) {

        /*
         * Basic check of the arguments provided. length obviously must be not
         * equal to 0. There are only 64 bytes to write to. This boundary
         * shouldn't be exceeded.
         */
        if (length && (addr + length <= 64)) {

            /*
             * Start I2C transfer. The write address of the RTC can be
             * calculated quite easily using macros defined earlier on.
             */
            i2c_master_start_wait(DEVRTC + TW_WRITE);

            /*
             * Write address requested to write to to the I2C bus. Only proceed
             * if the write operation exits successfully.
             */
            if (i2c_master_write(addr, &i2c_rtc_status)) {

                /*
                 * Indicate true. If nothing unexpected happens, this will stay
                 * true until the end.
                 */
                rtc = true;

                /*
                 * Decrement length until it reaches zero
                 */
                while (length--) {

                    /*
                     * Write next byte to the I2C bus and post increment the
                     * valuep pointer, where the data is actually read from.
                     * If there is some sort of an error, the return value will
                     * be set to false and we break out of this loop.
                     */
                    if (!i2c_master_write(*valuep++, &i2c_rtc_status)) {

                        rtc = false;

                        break;

                    }

                }

            }

            /*
             * Release the I2C bus.
             */
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
 * general purpose. Addresses ranging from 0x0 to 0x7 contain the various
 * registers of the RTC itself, see [1], p. 8.
 *
 * This function internally makes use of various functions declared in
 * i2c_master.h and will return false if there is some kind of an error in
 * regards to the I2C bus.
 *
 * [1]: http://datasheets.maximintegrated.com/en/ds/DS1307.pdf
 *
 * @warning There are only 56 bytes to read from for general purpose.
 *
 * @param addr The address in SRAM of the RTC you want start to read from
 * @param void_valuep Pointer to buffer in memory for holding the data
 * @param length The length the data you want to read
 *
 * @return Result of the operation, true if successful, false otherwise
 *
 * @see i2c_rtc_sram_write()
 * @see i2c_master_write()
 */
bool i2c_rtc_sram_read(uint8_t addr, void* void_valuep, uint8_t length)
{

    unsigned char* valuep = void_valuep;
    bool rtc = false;

    if (rtc_initialized) {

        /*
         * Basic check of the provided arguments. length obviously must be not
         * equal to 0. There are only 64 bytes to write to. This boundary
         * shouldn't be exceeded.
         */
        if (length && (addr + length <= 64)) {

            /*
             * Start I2C transfer. First of all we need to tell the RTC the
             * address to read from, so we actually need to use the write
             * address.
             */
            i2c_master_start_wait(DEVRTC + TW_WRITE);

            /*
             * Write the address to the RTC, only proceed if it returns no
             * error condition.
             */
            if (i2c_master_write(addr, &i2c_rtc_status)) {

                /*
                 * Request the data using i2c_master_rep_start(), which won't
                 * release the bus. Only proceed if there is no error
                 * condition. Use the read address this time.
                 */
                if (i2c_master_rep_start(DEVRTC + TW_READ, &i2c_rtc_status)) {

                    rtc = true;

                    /**
                     * Decrement length until it reaches zero
                     */
                    while (--length) {

                        /*
                         *  Put data into buffer and post increment the pointer
                         */
                        *valuep++ = i2c_master_read_ack();

                    }

                    /*
                     * After last byte has been received, a NACK condition has
                     * to be send.
                     */
                    *valuep++ = i2c_master_read_nak();

                }

            }

            /*
             * Release the I2C bus.
             */
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
 * It should be noted that there is a CH bit (bit 7 of register 0), see [1],
 * p. 8, which will halt the clock of the RTC. This function makes sure that
 * this bit is set to 0, so the clock is **not** being halted.
 *
 * If there is some sort of an error with the I2C bus this function will return
 * false.
 *
 * [1]: http://datasheets.maximintegrated.com/en/ds/DS1307.pdf
 *
 * @return Result of the operation, true if successful, false otherwise
 *
 * @param errorcode_p Pointer to memory for holding possible error codes
 * @param status_p Pointer to memory for holding status codes
 *
 * @see CTRL_REG
 * @see rtc_initialized
 * @see i2c_rtc_sram_write()
 */
bool i2c_rtc_init(uint8_t* errorcode_p, uint8_t* status_p)
{

    bool rtc = false;
    uint8_t seconds;

    /*
     * Initialize status and errorcode
     */
    *status_p = 0xff;
    *errorcode_p = i2c_master_init();

    /*
     * Check whether i2c_master module could be initialized successfully.
     */
    if (*errorcode_p == 0) {

        rtc_initialized = true;
        uint8_t ctrlreg = CTRL_REG;

        /*
         * Write control register to the RTC
         */
        if (i2c_rtc_sram_write(0x07, &ctrlreg, 1)) {

            rtc = true;

            /*
             * Read the first register of RTC
             */
            if (i2c_rtc_sram_read(0x00, &seconds, 1)) {

                /*
                 * Check whether CH bit is set and disable it if necessary.
                 */
                if (seconds & _BV(7)) {

                    seconds &= ~_BV(7);
                    i2c_rtc_sram_write(0x00, &seconds, 1);

                }

            }

        } else {

            /*
             * Something went wrong while trying to write to the RTC. Set
             * errorcode and status variables accordingly.
             */
            *errorcode_p = I2C_ERROR_SLAVE_NOT_FOUND;
            *status_p = i2c_rtc_status;

        }

    }

    return rtc;

}
