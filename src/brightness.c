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
 * @file brightness.c
 * @brief Implementation of functionality declared in brightness.h
 *
 * Basically, this module glues together the LDR and PWM modules. It adjusts
 * the brightness of the Wordclock by adjusting the PWM output depending upon
 * the current LDR value.
 *
 * @see brightness.h
 */

#include <inttypes.h>

#include "brightness.h"
#include "config.h"
#include "ldr.h"
#include "log.h"
#include "pwm.h"

/**
 * @brief Initializes the brightness module
 *
 * This initializes the brightness module and is expected to be called once
 * during startup. For now it only sets the default logging level.
 *
 * @see log_set_level()
 * @see LOG_LEVEL_BRIGHTNESS_DEFAULT
 */
void brightness_init()
{

    // Set default log level
    log_set_level(LOG_MODULE_BRIGHTNESS, LOG_LEVEL_BRIGHTNESS_DEFAULT);

}

/**
 * @brief Accommodates the brightness in response to changes ambient lightning
 *
 * This functions retrieves the brightness from the LDR and sets a new
 * base brightness for the PWM generation whenever there are changes
 * in comparison with the last taken measurement.
 *
 * @note Only the most upper five bits of `ldr_get_brightness()` are taken into
 * account, as only base brightness step values from 0 to 31 are valid.
 *
 * @note While this function is not time critical, it needs to called on a
 * quasi-regular basis, e.g. as part of the main loop.
 *
 * @todo Think about putting this into a 1 Hz ISR, so the timing becomes more
 * deterministic ...
 *
 * @see ldr_get_brightness()
 * @see pwm_set_brightness()
 * @see ldr.h
 */
void brightness_handle()
{

    static uint8_t last_ldr_brightness = 0xff;

    uint8_t ldr_brightness = ldr_get_brightness() >> 3;

    if (last_ldr_brightness != ldr_brightness) {

        log_output_P(LOG_MODULE_BRIGHTNESS, LOG_LEVEL_INFO, "Changed to: %u", ldr_brightness);

        pwm_set_base_brightness(ldr_brightness);

        last_ldr_brightness = ldr_brightness;

    }

}
