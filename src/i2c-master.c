/*
 * Copyright (C) 2012, 2013 Karol Babioch <karol@babioch.de>
 * Copyright (c) 2010 Frank Meyer - frank(at)fli4l.de
 * Copyright (C) 2005, 2008 Peter Fleury <pfleury@gmx.ch>
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
 * @file i2c-master.c
 * @brief Implementation of the header declared in i2c-master.h
 *
 * This header makes the I2C hardware unit available to other modules of this
 * project, e.g. for the "i2c-rtc" module, see i2c-rtc.h. This module
 * implements the master mode in both directions - transmission and reception.
 *
 * For an overview of the I2C bus take a look at [1]. The specification
 * (version 4.0) can be found at [2].
 *
 * For details about the hardware unit itself, take a look at [3], p. 222f,
 * section 22. [3], p. 236f, section 22.9, might be of special interest as it
 * describes the various registers used here.
 *
 * [1]: https://en.wikipedia.org/wiki/I%C2%B2C
 * [2]: http://www.nxp.com/documents/user_manual/UM10204.pdf
 * [3]: http://www.atmel.com/images/doc2545.pdf
 *
 * @see i2c-master.h
 */

#include <inttypes.h>
#include <util/twi.h>
#include <util/delay.h>
#include <stdbool.h>

#include "i2c-master.h"
#include "ports.h"

#define SCL_CLOCK 100000

#define WAIT_UNTIL_TRANSMISSION_COMPLETED   while (!(TWCR & _BV(TWINT)));
#define WAIT_UNTIL_STOP_CONDITION_EXECUTED  while (TWCR & _BV(TWSTO));

/*---------------------------------------------------------------------------------------------------------------------------------------------------
 * fm: If SDA is low, you can reset the I2C bus by doing following steps:
 * 1. Clock up to 9 cycles.
 * 2. Look for SDA high in each cycle while SCL is high.
 * 3. Create a start condition.
 *---------------------------------------------------------------------------------------------------------------------------------------------------
 */
#if defined (__AVR_ATmega88P__) || (__AVR_ATmega168P__) || defined (__AVR_ATmega328P__)

    #define HAS_RESET 1

    #define SCL PORTC, 5
    #define SDA PORTC, 4

    #define SCL_LOW     DDR(SCL) |= _BV(BIT(SCL))
    #define SCL_HIGH    DDR(SCL) &= ~_BV(BIT(SCL))
    #define SDA_IS_HIGH (PIN(SDA) & _BV(BIT(SDA)))
    #define SDA_IS_LOW  (!SDA_IS_HIGH)
    #define SCL_IS_HIGH (PIN(SCL) & _BV(BIT(SCL)))
    #define SCL_IS_LOW  (!SCL_IS_HIGH)

#else

    #define HAS_RESET 0

#endif

/*---------------------------------------------------------------------------------------------------------------------------------------------------
 *  Reset I2C bus
 *  @details  Resets I2C bus by doing the steps above
 *  @return    0 = successful, else failed
 *---------------------------------------------------------------------------------------------------------------------------------------------------
 */
static uint8_t i2c_reset(void)
{

    uint8_t result = 0;

    #if HAS_RESET == 1

        PORT(SCL) &= ~_BV(BIT(SCL));
        DDR(SCL)  &= ~_BV(BIT(SCL));
        PORT(SDA) &= ~_BV(BIT(SDA));
        DDR(SDA)  &= ~_BV(BIT(SDA));

        _delay_ms(1);

        if (SCL_IS_LOW) {

            result = I2C_ERROR_SCL_LOW;

        } else {

            if (SDA_IS_LOW) {

                SCL_LOW;

                _delay_ms(1);

                for(uint8_t i = 0; i < 9; i++) {

                    SCL_HIGH;

                    _delay_ms(1);

                    if (SDA_IS_HIGH) {

                        break;

                    }

                    SCL_LOW;

                    _delay_ms(1);

                }

                DDR(SCL)  &= ~_BV(BIT(SCL));

            }

            SCL_HIGH;

            _delay_ms(1);

            if (SDA_IS_LOW)  {

                result = I2C_ERROR_SDA_LOW;

            }

        }

        #endif

        return result;

}


/*---------------------------------------------------------------------------------------------------------------------------------------------------
 *  Initializes the I2C hardware
 *  @details  Configures I2C bus in order to operate as I2C master
 *  @return    0 if successful, anything else if not
 *---------------------------------------------------------------------------------------------------------------------------------------------------
 */
uint8_t i2c_master_init(void)
{

    static bool initialized;
    uint8_t result;

    if (!initialized) {

        result = i2c_reset();

        TWSR = 0;
        TWBR = ((F_CPU / SCL_CLOCK) - 16) / 2;

        if (result == 0) {

            initialized = true;

        }

    } else {

        result = 0;

    }

    return result;

}

/*---------------------------------------------------------------------------------------------------------------------------------------------------
 *  Start I2C transfer
 *  @details  Issues a start condition and sends address and transfer direction
 *  @param    I2C address
 *  @param    pointer to byte in order to store I2C status
 *  @return    1 = failed to access device, 0 = device accessible
 *---------------------------------------------------------------------------------------------------------------------------------------------------
 */
unsigned char i2c_master_start(uint8_t address, uint8_t* status_p)
{

    uint8_t twst;

    TWCR = _BV(TWINT) | _BV(TWSTA) | _BV(TWEN);

    WAIT_UNTIL_TRANSMISSION_COMPLETED;

    twst = TW_STATUS;

    if ((twst != TW_START) && (twst != TW_REP_START)) {

        *status_p = twst;

        return 1;

    }

    TWDR = address;
    TWCR = _BV(TWINT) | _BV(TWEN);

    WAIT_UNTIL_TRANSMISSION_COMPLETED

    twst = TW_STATUS;

    if ((twst != TW_MT_SLA_ACK) && (twst != TW_MR_SLA_ACK)) {

        *status_p = twst;

        return 1;

    }

    return 0;

}

/*---------------------------------------------------------------------------------------------------------------------------------------------------
 *  Start I2C transfer and wait until device is ready
 *  @details  Issues a start condition and sends address and transfer direction
 *  @details  If device is busy, use ack polling to wait until device is ready
 *  @param    I2C address
 *---------------------------------------------------------------------------------------------------------------------------------------------------
 */
void i2c_master_start_wait(uint8_t address)
{

    uint8_t twst;

    while(1) {

        TWCR = _BV(TWINT) | _BV(TWSTA) | _BV(TWEN);

        WAIT_UNTIL_TRANSMISSION_COMPLETED;

        twst = TW_STATUS;

        if ((twst != TW_START) && (twst != TW_REP_START)) {

            continue;

        }

        TWDR = address;
        TWCR = _BV(TWINT) | _BV(TWEN);

        WAIT_UNTIL_TRANSMISSION_COMPLETED;

        twst = TW_STATUS;

        if ((twst == TW_MT_SLA_NACK) || (twst == TW_MR_DATA_NACK)) {

            TWCR = _BV(TWINT) | _BV(TWEN) | _BV(TWSTO);

            WAIT_UNTIL_STOP_CONDITION_EXECUTED;

            continue;

        }

        break;

    }

}

/*---------------------------------------------------------------------------------------------------------------------------------------------------
 *  Start I2C transfer (repeated)
 *  @details  Issues a repeated start condition and sends address and transfer direction
 *  @param    I2C address
 *  @param    pointer to byte in order to store I2C status
 *  @return    1 = failed to access device, 0 = device accessible
 *---------------------------------------------------------------------------------------------------------------------------------------------------
 */
uint8_t i2c_master_rep_start(uint8_t address, uint8_t* status_p)
{

    return i2c_master_start(address, status_p);

}

/*---------------------------------------------------------------------------------------------------------------------------------------------------
 *  Stop I2C transfer
 *  @details  Terminates the data transfer and releases the I2C bus
 *---------------------------------------------------------------------------------------------------------------------------------------------------
 */
void i2c_master_stop (void)
{

    TWCR = _BV(TWINT) | _BV(TWEN) | _BV(TWSTO);

    WAIT_UNTIL_STOP_CONDITION_EXECUTED;

}

/*---------------------------------------------------------------------------------------------------------------------------------------------------
 *  Send one byte to I2C device
 *  @details  Sends one byte to I2C device
 *  @param    byte to be transfered
 *  @param    pointer to byte in order to store I2C status
 *  @return    0 write successful, 1 write failed
 *---------------------------------------------------------------------------------------------------------------------------------------------------
 */
uint8_t i2c_master_write(uint8_t data, uint8_t* status_p)
{

    uint8_t   twst;

    TWDR = data;
    TWCR = _BV(TWINT) | _BV(TWEN);

    WAIT_UNTIL_TRANSMISSION_COMPLETED;

    twst = TW_STATUS;

    if (twst != TW_MT_DATA_ACK) {

        *status_p = twst;

        return 1;

    }

    return 0;

}

/*---------------------------------------------------------------------------------------------------------------------------------------------------
 *  Read one byte, request more data
 *  @details  Reads one byte from the I2C device, then requests more data from device
 *  @return    byte read
 *---------------------------------------------------------------------------------------------------------------------------------------------------
 */
uint8_t i2c_master_read_ack(void)
{

    TWCR = _BV(TWINT) | _BV(TWEN) | _BV(TWEA);

    WAIT_UNTIL_TRANSMISSION_COMPLETED;

    return TWDR;

}

/*---------------------------------------------------------------------------------------------------------------------------------------------------
 *  Read one byte, followed by a stop condition
 *  @details  Reads one byte from the I2C device, read is followed by a stop condition
 *  @return    byte read
 *---------------------------------------------------------------------------------------------------------------------------------------------------
 */
uint8_t i2c_master_read_nak(void)
{

    TWCR = _BV(TWINT) | _BV(TWEN);

    WAIT_UNTIL_TRANSMISSION_COMPLETED;

    return TWDR;

}
