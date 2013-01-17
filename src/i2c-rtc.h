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
 * @file i2c-rtc.h
 * @brief Header for handling access to the DS1307 RTC
 *
 * The [RTC][1] (Real-time clock) is connected to the microcontroller via the
 * [I2C][2] bus. This header makes some functions available, which can be used
 * to access the RTC module used in this project, namely the [DS1307][3].
 *
 * Internally it makes use of i2c-master.h quite heavily.
 *
 * [1]: https://en.wikipedia.org/wiki/Real-time_clock
 * [2]: https://en.wikipedia.org/wiki/I2c
 * [3]: http://datasheets.maximintegrated.com/en/ds/DS1307.pdf
 *
 * @see i2c-rtc.c
 * @see i2c-master.c
 */

#ifndef _WC_I2C_RTC_H_
#define _WC_I2C_RTC_H_

#include <stdbool.h>

#include "datetime.h"

extern uint8_t i2c_rtc_get_status(void);

extern bool i2c_rtc_write(const datetime_t* datetime);

extern bool i2c_rtc_read(datetime_t* datetime);

extern bool i2c_rtc_sram_write(uint8_t addr, void* void_valuep, uint8_t length);

extern bool i2c_rtc_sram_read(uint8_t addr, void* void_valuep, uint8_t length);

extern bool i2c_rtc_init(uint8_t* errorcode_p, uint8_t* status_p);

#endif /* _WC_I2C_RTC_H_ */
