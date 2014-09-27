/*
 * Copyright (C) 2012, 2013, 2014 Karol Babioch <karol@babioch.de>
 * Copyright (c) 2010 Frank Meyer - frank(at)fli4l.de
 * Copyright (c) 2010 Vlad Tepesch
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
 * @file pwm.h
 * @brief Header for manipulating the PWM signal in control of the colors (RGB)
 *
 * This module provides means to change the color and/or brightness of the
 * LEDs by manipulating the PWM signal responsible for the appropriate channel.
 *
 * @note In case `ENABLE_RGB_SUPPORT` is disabled the functionality for
 * manipulating the color will not be built into the firmware.
 *
 * @see ENABLE_RGB_SUPPORT
 * @see pwm.c
 */

#ifndef _WC_PWM_H_
#define _WC_PWM_H_

#include <stdbool.h>

#include "color.h"

/**
 * @brief Number of PWM steps
 *
 * This defines the number of predefined values within `pwm_table`. Each value
 * is referred to as a step, so there are 0 to `MAX_PWM_STEPS` steps.
 *
 * @note Currently only two tables with either 32 or 64 values are predefined.
 *
 * @see pwm_table
 */
#define MAX_PWM_STEPS 32

/**
 * @brief Size of lookup table between the brightness and the PWM signal
 *
 * This defines the size of the table mapping the various brightness values to
 * an index in control of the PWM signal.
 *
 * By increasing the size of this table the brightness can be controlled with
 * more granularity in respect to the measured LDR values (which represent the
 * ambient light conditions). However this table is stored in program space,
 * which is limited.
 */
#define LDR2PWM_COUNT 32

/**
 * @brief Data type for various user defined values
 *
 * This defines the data type for various variables containing user defined
 * values throughout this module regarding the mapping between the measured
 * LDR values and the PWM signal.
 */
#define LDR2PWM_OCC_TYPE uint32_t

/**
 * @brief Data of the PWM module that is stored persistently in EEPROM
 *
 * The default values are defined in PWMEEPROMPARAMS_DEFAULT.
 *
 * @see PWMEEPROMPARAMS_DEFAULT
 * @see prefs_t::pwm_prefs
 */
typedef struct {

    /**
     * @brief User defined offset to the brightness control
     *
     * @see offset_pwm_idx
     */
    int8_t brightnessOffset;

    /**
     * @brief Mapping of measured brightness to PWM steps
     */
    uint8_t brightness2pwmStep[LDR2PWM_COUNT];

    /**
     * @brief Bitfield containing array members defined by the user
     */
    LDR2PWM_OCC_TYPE occupancy;

} pwm_prefs_t;

/**
 * @brief Default values of this module that should be stored persistently
 *
 * This defines the default values for this module. Refer to pwm_prefs_t
 * for a detailed description of each member.
 *
 * @note This will also be the values used after flashing the firmware to the
 * microcontroller, so make sure that the defaults are actually useful.
 *
 * @see pwm_prefs_t
 * @see preferences.h
 */
#define PWMEEPROMPARAMS_DEFAULT { \
    0, \
    {5, 6, 7, 8, 8, 9, 10, 11, 12, 13, 13, 14, 15, 16, 17, 18, \
    18, 19, 20, 21, 22, 23, 23, 24, 25, 26, 27, 28, 28, 29, 30, 31}, \
    ((LDR2PWM_OCC_TYPE)1) | (((LDR2PWM_OCC_TYPE)1) << (LDR2PWM_COUNT - 1)) \
}

extern void pwm_init();

extern void pwm_on();

extern void pwm_off();

extern bool pwm_is_enabled();

#if (ENABLE_RGB_SUPPORT == 1)

    extern void pwm_set_color(color_rgb_t color);

    extern const color_rgb_t* pwm_get_color();

#endif

extern void pwm_set_base_brightness(uint8_t base_brightness);

extern void pwm_increase_brightness();

extern void pwm_decrease_brightness();

extern void pwm_lock_brightness_val(uint8_t val);

extern void pwm_release_brightness();

extern void pwm_modifyLdrBrightness2pwmStep();

#endif /* _WC_PWM_H_ */
