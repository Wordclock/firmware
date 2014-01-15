/*
 * Copyright (C) 2012, 2013, 2014 Karol Babioch <karol@babioch.de>
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
 * @file i2c_rtc.h
 * @brief Header for handling access to the DS1307 RTC
 *
 * This header makes functions of the DS1307 RTC available to other modules.
 * Beside providing the functionality to read and/or set the time, the RTC
 * is also equipped with SRAM to write to and/or read from. This SRAM is
 * battery backed, so it can be used to store things basically persistent.
 *
 * @see i2c_rtc.c
 */

#ifndef _WC_I2C_RTC_H_
#define _WC_I2C_RTC_H_

#include <stdbool.h>

#include "datetime.h"
#include "i2c_master.h"

extern uint8_t i2c_rtc_get_status();

extern bool i2c_rtc_write(const datetime_t* datetime);

extern bool i2c_rtc_read(datetime_t* datetime);

extern bool i2c_rtc_sram_write(uint8_t address, void* data, uint8_t length);

extern bool i2c_rtc_sram_read(uint8_t address, void* data, uint8_t length);

extern bool i2c_rtc_init(i2c_master_error_t* error);

#endif /* _WC_I2C_RTC_H_ */
