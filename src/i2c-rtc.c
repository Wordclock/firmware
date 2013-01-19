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

#define DEVRTC 0xD0

static bool rtc_initialized = false;

static uint8_t i2c_rtc_status;

/*---------------------------------------------------------------------------------------------------------------------------------------------------
 * bits of control register:
 *
 * NAME     PURPOSE             REMARK
 * OUT      output control      if SQWE=0, the logic level on the SQW/OUT pin is 1 if OUT=1 and is 0 if OUT=0
 * SQWE     square wave enable  enable the oscillator output, rate see below
 * RS1/RS0  rate select         rate select
 *
 * RS1  RS0    SQW output frequency
 *  0    0      1 Hz
 *  0    1      4.096 kHz
 *  1    0      8.192 kHz
 *  1    0      32.768 kHz
 *---------------------------------------------------------------------------------------------------------------------------------------------------
 */

#define CTRL_REG_OUT 	0x80
#define CTRL_REG_SQWE	0x10
#define CTRL_REG_RS1    0x02
#define CTRL_REG_RS0    0x01

static volatile uint8_t ctrlreg;

uint8_t i2c_rtc_get_status(void)
{

	return i2c_rtc_status;

}

/*---------------------------------------------------------------------------------------------------------------------------------------------------
 *  Write date & time
 *  @details  Writes date & time into RTC
 *  @param    date & time
 *  @return    TRUE = successful, FALSE = failed
 *---------------------------------------------------------------------------------------------------------------------------------------------------
 */
bool i2c_rtc_write(const datetime_t * datetime)
{

	uint8_t rtcbuf[7];
	bool rtc = false;

	if (rtc_initialized) {

		rtcbuf[0] = itobcd (datetime->ss);
		rtcbuf[1] = itobcd (datetime->mm);
		rtcbuf[2] = itobcd (datetime->hh);
		rtcbuf[3] = itobcd (datetime->wd) + 1;
		rtcbuf[4] = itobcd (datetime->DD);
		rtcbuf[5] = itobcd (datetime->MM);
		rtcbuf[6] = itobcd (datetime->YY);

		if (i2c_rtc_sram_write (0x00, rtcbuf, 7)) {

			rtc = true;

		}

	}

	return rtc;

}

/*---------------------------------------------------------------------------------------------------------------------------------------------------
 *  Read date & time
 *  @details  Reads date & time from rtc
 *  @param    date & time
 *  @return    TRUE = successful, FALSE = failed
 *---------------------------------------------------------------------------------------------------------------------------------------------------
 */
bool i2c_rtc_read(datetime_t * datetime)
{

	uint8_t  rtcbuf[7];
	bool     rtc = false;

	if (rtc_initialized) {

		if (i2c_rtc_sram_read (0x00, rtcbuf, 7)) {

			datetime->YY = bcdtoi (rtcbuf[6]);
			datetime->MM = bcdtoi (rtcbuf[5]);
			datetime->DD = bcdtoi (rtcbuf[4]);
			datetime->wd = bcdtoi (rtcbuf[3]) - 1;
			datetime->hh = bcdtoi (rtcbuf[2]);
			datetime->mm = bcdtoi (rtcbuf[1]);
			datetime->ss = bcdtoi (rtcbuf[0]);

			rtc = true;

		}

	}

	return rtc;

}

/*---------------------------------------------------------------------------------------------------------------------------------------------------
 *  Write data into SRAM
 *  @details  Writes data into SRAM
 *  @param    address
 *  @param    pointer to buffer
 *  @param    length
 *  @return    TRUE = successful, FALSE = failed
 *---------------------------------------------------------------------------------------------------------------------------------------------------
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

/*---------------------------------------------------------------------------------------------------------------------------------------------------
 *  Read data into SRAM
 *  @details  Reads data into SRAM
 *  @param    address
 *  @param    pointer to buffer
 *  @param    length
 *  @return   TRUE = successful, FALSE = failed
 *---------------------------------------------------------------------------------------------------------------------------------------------------
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

/*---------------------------------------------------------------------------------------------------------------------------------------------------
 *  Write control register
 *  @details    Writes control register
 *  @return     TRUE = successful, FALSE = failed
 *---------------------------------------------------------------------------------------------------------------------------------------------------
 */
static bool i2c_write_ctrlreg(void)
{

	uint8_t value[1];
	bool rtc = false;

	value[0] = ctrlreg;

	if (i2c_rtc_sram_write(0x07, value, 1)) {

		rtc = true;

	}

	return rtc;

}

/*---------------------------------------------------------------------------------------------------------------------------------------------------
 *  Initialize RTC
 *  @details  Initializes & configures RTC
 *  @param    pointer to byte in order to store errorcode
 *  @param    pointer to byte in order to store I2C status
 *  @return   TRUE = successful, FALSE = failed
 *---------------------------------------------------------------------------------------------------------------------------------------------------
 */
bool i2c_rtc_init(uint8_t* errorcode_p, uint8_t* status_p)
{

	bool rtc = false;
	uint8_t seconds;

	*status_p = 0xFF;
	*errorcode_p = i2c_master_init();

	if (*errorcode_p == 0) {

		rtc_initialized = true;

		ctrlreg = CTRL_REG_OUT;

		if (i2c_write_ctrlreg()) {

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
