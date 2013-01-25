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

#include "i2c-master.h"
#include "ports.h"

/**
 * @brief Clock frequency of the I2C bus
 *
 * This is the frequency the I2C bus will be operated at in Hz. The default is
 * 100 kHz, which should be fine in most cases
 *
 * @see i2c_master_init()
 */
#define SCL_CLOCK 100000

/**
 * @brief Loop, which will wait for the transmission to complete
 *
 * This waits [actively][1] for the current transmission to be completed by
 * polling the TWINT bit in the TWCR register, see [2], p. 236f, section
 * 22.9.2 for details.
 *
 * [1]: https://en.wikipedia.org/wiki/Busy_waiting
 * [2]: http://www.atmel.com/images/doc2545.pdf
 */
#define WAIT_UNTIL_TRANSMISSION_COMPLETED while (!(TWCR & _BV(TWINT)));

/**
 * @brief Loop, which will wait for a stop condition to be executed
 *
 * This waits [actively][1] for the stop condition to be executed by
 * polling the TWSTO bit in the TWCR register, see [2], p. 236f, section
 * 22.9.2 for details.
 *
 * [1]: https://en.wikipedia.org/wiki/Busy_waiting
 * [2]: http://www.atmel.com/images/doc2545.pdf
 */
#define WAIT_UNTIL_STOP_CONDITION_EXECUTED while (TWCR & _BV(TWSTO));

#if defined (__AVR_ATmega88P__) || (__AVR_ATmega168P__) || defined (__AVR_ATmega328P__)

    /**
     * @brief Indicates that the I2C bus can be reset manually
     *
     * @see i2c_master_init()
     */
    #define HAS_RESET 1

    /**
     * @brief Port and pin of where the SCL line is attached to
     *
     * @see i2c_master_init()
     */
    #define SCL PORTC, 5

    /**
     * @brief Port and pin of where the SDA line is attached to
     *
     * @see i2c_master_init()
     */
    #define SDA PORTC, 4

    /**
     * @brief Pulls the SCL line low
     */
    #define SCL_LOW DDR(SCL) |= _BV(BIT(SCL))

    /**
     * @brief Pulls the SCL line high
     */
    #define SCL_HIGH DDR(SCL) &= ~_BV(BIT(SCL))

    /**
     * @brief Checks whether the SDA line is high
     */
    #define SDA_IS_HIGH (PIN(SDA) & _BV(BIT(SDA)))

    /**
     * @brief Checks whether the SDA line is low
     */
    #define SDA_IS_LOW (!SDA_IS_HIGH)

    /**
     * @brief Checks whether the SCL line is high
     */
    #define SCL_IS_HIGH (PIN(SCL) & _BV(BIT(SCL)))

    /**
     * @brief Checks whether the SCL line is low
     */
    #define SCL_IS_LOW (!SCL_IS_HIGH)

#else

    /**
     * @brief Indicates that the I2C bus cannot be reset manually
     *
     * @see i2c_master_init()
     */
    #define HAS_RESET 0

#endif

/**
 * @brief Resets the I2C bus
 *
 * This tries to reset the I2C bus and returns 0 if everything went fine.
 * Otherwise either I2C_ERROR_SCL_LOW and/or I2C_ERROR_SDA_LOW will be
 * returned.
 *
 * It checks both the SDA and the SCL line for its level. If the SCL line is
 * low, we can do nothing about it and return I2C_ERROR_SCL_LOW.
 *
 * If the SDA line is low on the other hand, we try to "clock through the
 * problem", see [1]. This includes the following steps:
 *
 * 1) Try to assert logic one on SDA line
 * 2) SDA line level is still logic zero and generates a clock pulse on SCL
 * 3) Examine SDA: If SDA = 0, repeat step 2, otherwise continue with 4
 * 4) Generate a STOP condition
 *
 * If after this procedure SDA is still low, we return I2C_ERROR_SDA_LOW as
 * we can't do anything about it.
 *
 * [1]: http://www.analog.com/static/imported-files/application_notes/54305147357414AN686_0.pdf
 *
 * @return 0 if successful, else either I2C_ERROR_SCL_LOW or I2C_ERROR_SDA_LOW
 *
 * @see I2C_ERROR_SCL_LOW
 * @see I2C_ERROR_SDA_LOW
 * @see i2c_master_init()
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

                for (uint8_t i = 0; i < 9; i++) {

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

            if (SDA_IS_LOW) {

                result = I2C_ERROR_SDA_LOW;

            }

        }

        #endif

        return result;

}

/**
 * @brief Initializes the I2C unit
 *
 * This initializes the I2C unit to operate in master mode by writing to
 * various registers, see [1], p. 236f, section 22.9.
 *
 * [1]: http://www.atmel.com/images/doc2545.pdf
 *
 * @return 0 if successful, else anything returned by i2c_reset()
 *
 * @see I2C_ERROR_SCL_LOW
 * @see I2C_ERROR_SDA_LOW
 * @see i2c_reset()
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

        /*
         * Module has already been initialized successfully
         */
        result = 0;

    }

    return result;

}

/**
 * @brief Starts an I2C transfer by generating a start condition
 *
 * Generates a start condition addressing the given device. The address
 * specifies the direction and can be "calculated" using I2C_READ and
 * I2C_WRITE.
 *
 * The status of this operation will be stored in the buffer pointed at
 * status_p, when something went wrong. Otherwise the buffer will be left
 * untouched.
 *
 * If everything was successful false will be returned, otherwise true.
 *
 * In order to release the I2C bus i2c_master_stop() needs to be used after
 * the transfer is completed. If the bus shouldn't be released,
 * i2c_master_rep_start() can be used instead, which will generate a repeated
 * start condition.
 *
 * @param address The address of the device a start condition should be sent to
 * @param status_p Pointer to a buffer storing the status of the transfer
 *
 * @return False if transfer was successful, otherwise true
 *
 * @see i2c_master_stop()
 * @see i2c_master_rep_start()
 */
bool i2c_master_start(uint8_t address, uint8_t* status_p)
{

    uint8_t twst;

    TWCR = _BV(TWINT) | _BV(TWSTA) | _BV(TWEN);

    WAIT_UNTIL_TRANSMISSION_COMPLETED;

    twst = TW_STATUS;

    if ((twst != TW_START) && (twst != TW_REP_START)) {

        *status_p = twst;

        return true;

    }

    TWDR = address;
    TWCR = _BV(TWINT) | _BV(TWEN);

    WAIT_UNTIL_TRANSMISSION_COMPLETED

    twst = TW_STATUS;

    if ((twst != TW_MT_SLA_ACK) && (twst != TW_MR_SLA_ACK)) {

        *status_p = twst;

        return true;

    }

    return false;

}

/**
 * @brief Starts an I2C transfer and waits until the device is ready
 *
 * Generates a start condition addressing the given device. The address
 * specifies the direction and can be "calculated" using I2C_READ and
 * I2C_WRITE.
 *
 * In order to release the I2C bus i2c_master_stop() needs to be used after
 * the transfer is completed.
 *
 * @param address The address of the device a start condition should be sent to
 *
 * @see i2c_master_stop()
 */
void i2c_master_start_wait(uint8_t address)
{

    uint8_t twst;

    while (1) {

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

/**
 * @brief Starts an I2C transfer by generating a repeated start condition
 *
 * Generates a repeated start condition addressing the given device. The
 * address specifies the direction and can be "calculated" using I2C_READ and
 * I2C_WRITE.
 *
 * The status of this operation will be stored in the buffer pointed at
 * status_p, when something went wrong. Otherwise the buffer will be left
 * untouched.
 *
 * If everything was successful false will be returned, otherwise true.
 *
 * In order to release the I2C bus i2c_master_stop() needs to be used after
 * the transfer is completed.
 *
 * @param address The address of the device a start condition should be sent to
 * @param status_p Pointer to a buffer storing the status of the transfer
 *
 * @return False if transfer was successful, otherwise true
 *
 * @see i2c_master_stop()
 * @see i2c_master_start()
 */
bool i2c_master_rep_start(uint8_t address, uint8_t* status_p)
{

    return i2c_master_start(address, status_p);

}

/**
 * @brief Stop the I2C transfer by generating a stop condition
 *
 * This generates a stop condition and thus releases the I2C bus. For this to
 * make actually sense a start condition should be generated prior to that by
 * either i2c_master_start() and/or i2c_master_rep_start().
 *
 * @see i2c_master_start()
 * @see i2c_master_rep_start()
 */
void i2c_master_stop(void)
{

    TWCR = _BV(TWINT) | _BV(TWEN) | _BV(TWSTO);

    WAIT_UNTIL_STOP_CONDITION_EXECUTED;

}

/**
 * @brief Sends a byte to the I2C device previously specified
 *
 * Prior to calling this function a start condition to the desired device has
 * to be generated using either i2c_master_start() and/or
 * i2c_master_rep_start().
 *
 * The status of this operation will be stored in the buffer pointed at
 * status_p, when something went wrong. Otherwise the buffer will be left
 * untouched.
 *
 * If everything was successful false will be returned, otherwise true.
 *
 * @param data The byte to be transfered
 * @param status_p Pointer to a buffer storing the status of the transfer
 *
 * @return False if transfer was successful, otherwise true
 *
 * @see i2c_master_start()
 * @see i2c_master_rep_start()
 */
bool i2c_master_write(uint8_t data, uint8_t* status_p)
{

    uint8_t twst;

    TWDR = data;
    TWCR = _BV(TWINT) | _BV(TWEN);

    WAIT_UNTIL_TRANSMISSION_COMPLETED;

    twst = TW_STATUS;

    if (twst != TW_MT_DATA_ACK) {

        *status_p = twst;

        return true;

    }

    return false;

}

/**
 * @brief Reads a byte and requests more data
 *
 * Prior to calling this function a start condition to the desired device has
 * to be generated using either i2c_master_start() and/or
 * i2c_master_rep_start().
 *
 * This function, in opposite to i2c_master_read_nak(), requests more data
 * after receiving the originally requested data.
 *
 * i2c_master_read() is a macro, which can be used to easily switch between
 * i2c_master_read_ack() and i2c_master_read_nak().
 *
 * @return The byte read from the device
 *
 * @see i2c_master_start()
 * @see i2c_master_rep_start()
 * @see i2c_master_stop()
 * @see i2c_master_read_nak()
 * @see i2c_master_read()
 */
uint8_t i2c_master_read_ack(void)
{

    TWCR = _BV(TWINT) | _BV(TWEN) | _BV(TWEA);

    WAIT_UNTIL_TRANSMISSION_COMPLETED;

    return TWDR;

}

/**
 * @brief Reads a byte and generates a stop condition afterwards
 *
 * Prior to calling this function a start condition to the desired device has
 * to be generated using either i2c_master_start() and/or
 * i2c_master_rep_start().
 *
 * This function, in opposite to i2c_master_read_ack(), doesn't request more
 * data after receiving the originally requested data, but generates a stop
 * condition instead.
 *
 * i2c_master_read() is a macro, which can be used to easily switch between
 * i2c_master_read_ack() and i2c_master_read_nak().
 *
 * @return The byte read from the device
 *
 * @see i2c_master_start()
 * @see i2c_master_rep_start()
 * @see i2c_master_stop()
 * @see i2c_master_read_ack()
 * @see i2c_master_read()
 */
uint8_t i2c_master_read_nak(void)
{

    TWCR = _BV(TWINT) | _BV(TWEN);

    WAIT_UNTIL_TRANSMISSION_COMPLETED;

    return TWDR;

}
