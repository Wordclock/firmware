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

/**
 * @brief Indicates that the SCL line is low during initialization
 *
 * During initialization the line is checked for its level, see
 * i2c_master_init(). When it is detected that SCL is low, this error code
 * will be returned. It usually is an indicator for something being badly
 * wrong, e.g. bad wiring, as the SCL line should totally be in control by
 * the master.
 *
 * @see i2c_master_init()
 */
#define I2C_ERROR_SCL_LOW 1

/**
 * @brief Indicates that the SDA line is low during initialization
 *
 * During initialization the line is checked for its level, see
 * i2c_master_init(). When all is fine, it will be high. However it might be
 * the case that a device is "stuck". This is described at [1], p. 20, section
 * 3.1.16. An application Note from Analog Devices (see [2]) describes possible
 * solutions for this, one of which is "clocking through the problem". This is
 * exactly what is being tried here.
 *
 * However it might be the case that the the line remains low even after that,
 * which either means could either mean that the slave is still stuck or that
 * the I2C bus is faulty. In both of these cases this error code will be
 * returned.
 *
 * [1]: http://www.nxp.com/documents/user_manual/UM10204.pdf
 * [2]: http://www.analog.com/static/imported-files/application_notes/54305147357414AN686_0.pdf
 *
 * @see i2c_master_init()
 */
#define I2C_ERROR_SDA_LOW 2

/**
 * @brief Indicates that the given slave could't be found
 *
 * In a case where everything seems fine with the I2C bus itself, but the slave
 * won't answer to our requests, this error code will be returned.
 *
 * @see i2c_master_init()
 */
#define I2C_ERROR_SLAVE_NOT_FOUND 3

extern uint8_t i2c_master_init(void);

extern uint8_t i2c_master_start(uint8_t address, uint8_t* status_p);

extern void i2c_master_start_wait(uint8_t address);

extern uint8_t i2c_master_rep_start(uint8_t address, uint8_t* status_p);

extern void i2c_master_stop(void);

extern uint8_t i2c_master_write(uint8_t data, uint8_t* status_p);

extern uint8_t i2c_master_read_ack(void);

extern uint8_t i2c_master_read_nak(void);

/**
 * @brief Requests to read at least one byte
 *
 * The I2C specification allows to read multiple bytes by simply acknowledging
 * the transfer with an ACK bit. However after the last byte has been received
 * a NACK should be sent to the slave informing it about the end of the
 * transfer.
 *
 * This macro makes use of this by either expanding to i2c_master_read_ack()
 * or i2c_master_read_nak(), depending upon the given parameter.
 *
 * @param ack Boolean value: True if another byte should be requested, false
 *  otherwise
 * @return The requested byte
 * @see i2c_master_read_ack()
 * @see i2c_master_read_nak()
 */
#define i2c_master_read(ack) (ack) ? i2c_master_read_ack() : i2c_master_read_nak();

#endif /* _WC_I2C_MASTER_H_ */
