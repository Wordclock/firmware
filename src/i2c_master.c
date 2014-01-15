/*
 * Copyright (C) 2012, 2013, 2014 Karol Babioch <karol@babioch.de>
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
 * @file i2c_master.c
 * @brief Implementation of the header declared in i2c_master.h
 *
 * For an overview of the I2C bus take a look at [1]. The specification
 * (version 4.0) can be found at [2].
 *
 * For details about the hardware unit itself, refer to [3].
 *
 * [1]: https://en.wikipedia.org/wiki/I%C2%B2C
 * [2]: http://www.nxp.com/documents/user_manual/UM10204.pdf
 * [3]: http://www.atmel.com/images/doc2545.pdf
 *
 * @see i2c_master.h
 */

#include <inttypes.h>
#include <util/twi.h>
#include <util/delay.h>

#include "i2c_master.h"
#include "ports.h"

/**
 * @brief Loop waiting for the current transmission to be completed
 *
 * This busy waits for the current transmission to be completed by
 * constantly polling the TWINT bit in the TWCR register.
 */
#define WAIT_UNTIL_TRANSMISSION_COMPLETED while (!(TWCR & _BV(TWINT)));

/**
 * @brief Loop waiting for stop condition to be executed
 *
 * This busy waits for the stop condition to be executed by constantly
 * polling the TWSTO bit in the TWCR register.
 */
#define WAIT_UNTIL_STOP_CONDITION_EXECUTED while (TWCR & _BV(TWSTO));

#if defined (__AVR_ATmega88P__) || (__AVR_ATmega168P__) || defined (__AVR_ATmega328P__)

    /**
     * @brief Indicates that the I2C bus can be reset manually
     *
     * @see i2c_reset()
     */
    #define HAS_RESET 1

    /**
     * @brief Port and pin where the SCL line is attached to
     *
     * @see i2c_master_init()
     * @see ports.h
     */
    #define SCL PORTC, 5

    /**
     * @brief Port and pin where the SDA line is attached to
     *
     * @see i2c_master_init()
     * @see ports.h
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
     * @see i2c_reset()
     */
    #define HAS_RESET 0

#endif

/**
 * @brief Resets the I2C bus
 *
 * This tries to reset the I2C bus and returns true if everything went fine.
 * Otherwise false will be returned and the error code will be put at the
 * location pointed to by `error`.
 *
 * It checks both the SDA and the SCL line for its level. If the SCL line is
 * low, we can do nothing about it and the error code will be
 * `I2C_MASTER_ERROR_SCL_LOW`. If the SDA line is low on the other hand, we
 * try to "clock through the problem", which is described in detail at [1].
 *
 * This includes the following steps:
 *
 * 1) Try to assert logic one on SDA line
 * 2) SDA line level is still logic zero and generates a clock pulse on SCL
 * 3) Examine SDA: If SDA = 0, repeat step 2, otherwise continue with 4
 * 4) Generate a STOP condition
 *
 * If this procedure does not release the SDA line, the error code will be
 * `I2C_MASTER_ERROR_SDA_LOW`.
 *
 * [1]: http://www.analog.com/static/imported-files/application_notes/54305147357414AN686_0.pdf
 *
 * @param error Pointer to variable holding the error code
 *
 * @return True if successful, false otherwise
 *
 * @see I2C_ERROR_SCL_LOW
 * @see I2C_ERROR_SDA_LOW
 * @see i2c_master_error_t
 * @see i2c_master_init()
 */
static bool i2c_reset(i2c_master_error_t* error)
{

    #if (HAS_RESET == 1)

        PORT(SCL) &= ~_BV(BIT(SCL));
        DDR(SCL)  &= ~_BV(BIT(SCL));
        PORT(SDA) &= ~_BV(BIT(SDA));
        DDR(SDA)  &= ~_BV(BIT(SDA));

        _delay_ms(1);

        if (SCL_IS_LOW) {

            *error = I2C_MASTER_ERROR_SCL_LOW;

            return false;

        }

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

            *error = I2C_MASTER_ERROR_SDA_LOW;

            return false;

        }

    #endif

    return true;

}

/**
 * @brief Initializes the I2C unit
 *
 * This initializes the I2C unit and sets it up to operate in master mode.
 * This has to be called **once** before any other function of this module can
 * be used.
 *
 * The return value indicates whether the operation could be performed
 * successfully. If there was an error false will be returned and the error
 * code will be put at the location pointed to by `error`.
 *
 * @param error Pointer to variable holding the error code
 *
 * @return True if successful, false otherwise
 *
 * @see I2C_ERROR_SCL_LOW
 * @see I2C_ERROR_SDA_LOW
 * @see i2c_master_error_t
 * @see i2c_reset()
 */
bool i2c_master_init(i2c_master_error_t* error)
{

    static bool initialized;
    bool result;

    if (!initialized) {

        result = i2c_reset(error);

        if (result) {

            initialized = true;

        }

        TWSR = 0;
        TWBR = ((F_CPU / I2C_MASTER_SCL_CLOCK) - 16) / 2;

    } else {

        result = true;

    }

    return result;

}

/**
 * @brief Starts an I2C transfer by generating a start condition
 *
 * Generates a start condition and sends the given address to the bus.
 *
 * If the operation was successful, true will be returned. Otherwise the return
 * value is false and the status of the operation is being put into the buffer
 * pointed to by `status_p`.
 *
 * @param address Address of device a start condition should be generated for
 * @param status Pointer to buffer storing the status of the transfer
 *
 * @return True if transfer was successful, otherwise false
 *
 * @note Don't forget to release the bus and/or issue another start condition
 * once the transmission is completed.
 *
 * @see i2c_master_stop()
 * @see i2c_master_rep_start()
 */
bool i2c_master_start(uint8_t address, uint8_t* status)
{

    uint8_t twst;

    TWCR = _BV(TWINT) | _BV(TWSTA) | _BV(TWEN);

    WAIT_UNTIL_TRANSMISSION_COMPLETED;

    twst = TW_STATUS;

    if ((twst != TW_START) && (twst != TW_REP_START)) {

        *status = twst;

        return false;

    }

    TWDR = address;
    TWCR = _BV(TWINT) | _BV(TWEN);

    WAIT_UNTIL_TRANSMISSION_COMPLETED

    twst = TW_STATUS;

    if ((twst != TW_MT_SLA_ACK) && (twst != TW_MR_SLA_ACK)) {

        *status = twst;

        return false;

    }

    return true;

}

/**
 * @brief Starts an I2C transfer and waits until the device is ready
 *
 * Generates a start condition and sends the given address to the bus. In
 * opposition to `i2c_master_start()` it busy waits for the device to return
 * an acknowledge.
 *
 * @param address Address of device a start condition should be generated for
 *
 * @note Don't forget to release the bus once the transmission is completed.
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
 * Generates a repeated start condition and sends the given address to the bus.
 *
 * If the operation was successful, true will be returned. Otherwise the return
 * value is false and the status of the operation is being put into the buffer
 * pointed to by `status_p`.
 *
 * @param address Address of device a start condition should be generated for
 * @param status Pointer to buffer storing the status
 *
 * @return True if transfer was successful, otherwise false
 *
 * @note Don't forget to release the bus and/or issue another start condition
 * once the transmission is completed.
 *
 * @see i2c_master_stop()
 * @see i2c_master_start()
 */
bool i2c_master_rep_start(uint8_t address, uint8_t* status)
{

    return i2c_master_start(address, status);

}

/**
 * @brief Stop the I2C transfer by generating a stop condition
 *
 * This generates a stop condition and thus releases the I2C bus. For this to
 * make actually sense a start condition should be generated prior to that.
 *
 * @see i2c_master_start()
 * @see i2c_master_rep_start()
 */
void i2c_master_stop()
{

    TWCR = _BV(TWINT) | _BV(TWEN) | _BV(TWSTO);

    WAIT_UNTIL_STOP_CONDITION_EXECUTED;

}

/**
 * @brief Writes a byte to the I2C device previously addressed
 *
 * If the operation was successful, true will be returned. Otherwise the return
 * value is false and the status of the operation will be put into the buffer
 * pointed to by `status_p`.
 *
 * @note Before this function can be used successfully a start condition has
 * to be generated, addressing the device a byte should be written to.
 *
 * @param data Byte to be transfered
 * @param status Pointer to buffer storing the status of the transfer
 *
 * @return True if transfer was successful, otherwise false
 *
 * @see i2c_master_start()
 * @see i2c_master_rep_start()
 */
bool i2c_master_write(uint8_t data, uint8_t* status)
{

    uint8_t twst;

    TWDR = data;
    TWCR = _BV(TWINT) | _BV(TWEN);

    WAIT_UNTIL_TRANSMISSION_COMPLETED;

    twst = TW_STATUS;

    if (twst != TW_MT_DATA_ACK) {

        *status = twst;

        return false;

    }

    return true;

}

/**
 * @brief Reads a byte and requests more data
 *
 * This reads a single byte and returns it directly. By acknowledging the
 * received byte, the next byte is requested automatically. Use
 * `i2c_master_read_nak()` in cases where this is not desired.
 *
 * @note Before this function can be used successfully a start condition has
 * to be generated, addressing the device a byte should be read from.
 *
 * @return The byte read from the device
 *
 * @see i2c_master_read_nak()
 * @see i2c_master_read()
 */
uint8_t i2c_master_read_ack()
{

    TWCR = _BV(TWINT) | _BV(TWEN) | _BV(TWEA);

    WAIT_UNTIL_TRANSMISSION_COMPLETED;

    return TWDR;

}

/**
 * @brief Reads a byte and generates a stop condition afterwards
 *
 * This reads a single byte and returns it directly. By not acknowledging the
 * received byte no other byte is requested and a stop condition is generated
 * automatically. Use `i2c_master_read_ack()` in cases where this is not
 * desired.
 *
 * @note Before this function can be used successfully a start condition has
 * to be generated, addressing the device a byte should be read from.
 *
 * @return The byte read from the device
 *
 * @see i2c_master_read_ack()
 * @see i2c_master_read()
 */
uint8_t i2c_master_read_nak()
{

    TWCR = _BV(TWINT) | _BV(TWEN);

    WAIT_UNTIL_TRANSMISSION_COMPLETED;

    return TWDR;

}
