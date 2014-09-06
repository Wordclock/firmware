/*
 * Copyright (C) 2014 Karol Babioch <karol@babioch.de>
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
 * @file dcf77_decoder.h
 *
 * @see dcf77_decoder.c
 */

#ifndef _WC_DCF77_DECODER_H_
#define _WC_DCF77_DECODER_H_

#define DCF_POS_SIGNAL_START        (0)
#define DCF_POS_TIME_START          (20)
#define DCF_POS_MINUTE_START        (20)
#define DCF_POS_HOUR_START          (28)
#define DCF_POS_DAY_START           (35)
#define DCF_POS_DOW_START           (41)
#define DCF_POS_MONTH_START         (44)
#define DCF_POS_YEAR_START          (49)
#define DCF_POS_MINUTE_PARITY       (28)
#define DCF_POS_HOUR_PARITY         (35)
#define DCF_POS_DATE_PARITY         (58)

#include <stdbool.h>

// Append value and decode it, in case of error return false, otherwise true
bool dcf77_decoder_append(bool value);

// Return true if valid timeframe, false otherwise
// Pass reference to timedate struct?
bool dcf77_decoder_decode();

// TODO: Necessary?
void dcf77_decoder_reset();

#endif /* _WC_DCF77_DECODER_H_ */
