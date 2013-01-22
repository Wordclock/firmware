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
 * @file i2c-master.h
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
 * @see i2c-master.c
 */

#ifndef _WC_I2C_MASTER_H_
#define _WC_I2C_MASTER_H_

#include <stdbool.h>

#define I2C_READ                    1
#define I2C_WRITE                   0

#define I2C_ERROR_SCL_LOW           1
#define I2C_ERROR_SDA_LOW           2
#define I2C_ERROR_SLAVE_NOT_FOUND   3

/**
 *  Initializes the I2C hardware
 *  @details  Configures I2C bus in order to operate as I2C master
 *  @return    TRUE if successful, FALSE, if not
 */
extern bool                   i2c_master_init (void);

/**
 *  Start I2C transfer
 *  @details  Issues a start condition and sends address and transfer direction
 *  @param    address   I2C address
 *  @param    status_p  pointer to byte in order to store I2C status
 *  @return    1 = failed to access device, 0 = device accessible
 */
extern uint8_t                i2c_master_start(uint8_t address, uint8_t * status_p);

/**
 *  Start I2C transfer and wait until device is ready
 *  @details  Issues a start condition and sends address and transfer direction
 *            If device is busy, use ack polling to wait until device is ready
 *  @param    address I2C address
 */
extern void                   i2c_master_start_wait (uint8_t address);

/**
 *  Start I2C transfer (repeated)
 *  @details  Issues a repeated start condition and sends address and transfer direction
 *  @param    address   I2C address
 *  @param    status_p  pointer to byte in order to store I2C status
 *  @return    1 = failed to access device, 0 = device accessible
 */
extern uint8_t                i2c_master_rep_start (uint8_t address, uint8_t * status_p);

/**
 *  Stop I2C transfer
 *  @details  Terminates the data transfer and releases the I2C bus
 */
extern void                   i2c_master_stop (void);

/**
 *  Send one byte to I2C device
 *  @details  Sends one byte to I2C device
 *  @param    data     byte to be transfered
 *  @param    status_p pointer to byte in order to store I2C status
 *  @return    0 write successful, 1 write failed
 */
extern uint8_t                i2c_master_write (uint8_t data, uint8_t * status_p);

/**
 *  Read one byte, request more data
 *  @details  Reads one byte from the I2C device, then requests more data from device
 *  @return    byte read
 */
extern uint8_t                i2c_master_read_ack (void);

/**
 *  Read one byte, followed by a stop condition
 *  @details  Reads one byte from the I2C device, read is followed by a stop condition
 *  @return    byte read
 */
extern uint8_t                i2c_master_read_nak (void);

/**
 *  Read one byte
 *  @details  Read one byte, requests more data if ack == TRUE, issues a stop condition if ack == FALSE
 *  @param    ack: TRUE or FALSE
 *  @return    byte read
 */
#define i2c_master_read(ack)  (ack) ? i2c_master_read_ack() : i2c_master_read_nak();

#endif /* _WC_I2C_MASTER_H_ */
