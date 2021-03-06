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
 * @file i2c_master.h
 * @brief Header allowing access to the I2C hardware unit in master mode
 *
 * This header makes the I2C hardware unit available to other modules. It
 * implements the I2C master mode in both directions - transmission and
 * reception.
 *
 * This library was originally based upon a library named
 * "I2C Master Interface" from Peter Fleury, see [1].
 *
 * [1]: http://homepage.hispeed.ch/peterfleury/avr-software.html
 *
 * @see i2c_master.c
 */

#ifndef _WC_I2C_MASTER_H_
#define _WC_I2C_MASTER_H_

#include <stdbool.h>

/**
 * @brief Clock frequency of the I2C bus
 *
 * This defines the frequency of the I2C bus will be operated at (in Hz). The
 * default is 100 kHz, which should be fine in most cases.
 *
 * @see i2c_master_init()
 */
#define I2C_MASTER_SCL_CLOCK 100000

/**
 * @brief Defines the type of error codes during the initialization of the I2C
 *
 * @see i2c_master_init()
 */
typedef enum {

    /**
     * @brief Indicates that the SCL line is low
     *
     * During initialization the line is checked for its level. When it is
     * detected that SCL is low, this error code will be returned. It usually
     * is an indicator for something being badly wrong, e.g. bad wiring, as the
     * SCL line should totally be in control by the master.
     *
     * @see i2c_master_init()
     */
    I2C_MASTER_ERROR_SCL_LOW,

    /**
     * @brief Indicates that the SDA line is low
     *
     * During initialization the line is checked for its level. Normally it
     * should be high. However, it might be the case that a device is "stuck",
     * in which case this error code will be returned.
     *
     * This condition itself is described in detail at [1].
     *
     * [1]: http://www.nxp.com/documents/user_manual/UM10204.pdf
     *
     * @see i2c_master_init()
     */
    I2C_MASTER_ERROR_SDA_LOW,

    /**
     * @brief Indicates that the addressed slave could not be found
     *
     * When the addressed slave is not responding to the requests addressed,
     * this error code will be returned.
     */
    I2C_MASTER_ERROR_SLAVE_NOT_FOUND,

} i2c_master_error_t;

extern bool i2c_master_init(i2c_master_error_t* error);

extern bool i2c_master_start(uint8_t address, uint8_t* status);

extern void i2c_master_start_wait(uint8_t address);

extern bool i2c_master_rep_start(uint8_t address, uint8_t* status);

extern void i2c_master_stop();

extern bool i2c_master_write(uint8_t data, uint8_t* status);

extern uint8_t i2c_master_read_ack();

extern uint8_t i2c_master_read_nak();

/**
 * @brief Requests to read at least one byte
 *
 * The I2C specification allows to read multiple bytes by simply acknowledging
 * the transfer with an ACK bit. However, after the last byte has been received
 * a NACK should be sent to the slave informing it about the end of the
 * transmission.
 *
 * This macro exploits this by either expanding to `i2c_master_read_ack()`
 * or `i2c_master_read_nak()`, depending upon the given parameter.
 *
 * @param ack Boolean value, true if another byte should be requested
 *
 * @return Either `i2c_master_read_ack()` or `i2c_master_read_nak()`
 *
 * @see i2c_master_read_ack()
 * @see i2c_master_read_nak()
 */
#define i2c_master_read(ack) (ack) ? i2c_master_read_ack() : i2c_master_read_nak();

#endif /* _WC_I2C_MASTER_H_ */
