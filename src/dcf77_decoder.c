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
 * @file dcf77_decoder.c
 *
 * @see dcf77_decoder.h
 */

#include <inttypes.h>

#include "dcf77_decoder.h"
#include "base.h"
#include "log.h"

static uint8_t bit_position;

bool dcf77_decoder_append(bool value)
{

    log_output_P(LOG_MODULE_DCF77_DECODER, LOG_LEVEL_INFO, "Bit: %u, value: %u", bit_position, value);


//    if (bit_position == 58) {
//
//        // decode?
//
//    }

    bit_position++;

    return true;

}

bool dcf77_decoder_decode()
{

    bool result;

    if (bit_position != 59) {

        log_output_P(LOG_MODULE_DCF77_DECODER, LOG_LEVEL_INFO, "Invalid number of bits: %u", bit_position);

        result = false;

    } else {

        log_output_P(LOG_MODULE_DCF77_DECODER, LOG_LEVEL_INFO, "Decoded successfully");

        // TODO: Decode

        result = true;

    }

    dcf77_decoder_reset();

    return result;

}

void dcf77_decoder_reset()
{

    log_output_P(LOG_MODULE_DCF77_DECODER, LOG_LEVEL_INFO, "Reset");

    bit_position = 0;

}
