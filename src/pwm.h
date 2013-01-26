/*
 * Copyright (C) 2012, 2013 Karol Babioch <karol@babioch.de>
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
 * The color and brightness of the LEDs on the front panel (and/or the optional
 * Ambilight) is controlled using a [PWM][1] signal for each channel (red,
 * green, blue) separately.
 *
 * This header can be used to manipulate the color itself and/or the
 * brightness.
 *
 * [1]: https://en.wikipedia.org/wiki/Pulse-width_modulation
 *
 * @see pwm.c
 */

#ifndef _WC_PWM_H_
#define _WC_PWM_H_

#define MAX_PWM_STEPS 32

#define LDR2PWM_COUNT 32

#define LDR2PWM_OCC_TYPE uint32_t

typedef struct PwmEepromParams {

    int8_t brightnessOffset;

    uint8_t brightness2pwmStep[LDR2PWM_COUNT];

    LDR2PWM_OCC_TYPE occupancy;

} PwmEepromParams;

#define PWMEEPROMPARAMS_DEFAULT { \
    0, \
    {5, 6, 7, 8, 8, 9, 10, 11, 12, 13, 13, 14, 15, 16, 17, 18, \
    18, 19, 20, 21, 22, 23, 23, 24, 25, 26, 27, 28, 28, 29, 30, 31}, \
    ((LDR2PWM_OCC_TYPE)1) | (((LDR2PWM_OCC_TYPE)1) << (LDR2PWM_COUNT - 1)) \
}

extern void pwm_init(void);

extern void pwm_on(void);

extern void pwm_off(void);

extern void pwm_on_off(void);

#if (MONO_COLOR_CLOCK != 1)

    extern void pwm_set_colors(uint8_t red, uint8_t green, uint8_t blue);

    extern void pwm_get_colors(uint8_t* redp, uint8_t* greenp, uint8_t* bluep);

    extern void pwm_set_color_step(uint8_t red_step, uint8_t green_step,
            uint8_t blue_step);

    extern void pwm_get_color_step(uint8_t* red_stepp, uint8_t* green_stepp,
            uint8_t* blue_stepp);

#endif

extern void pwm_set_base_brightness_step(uint8_t pwm_idx);

extern void pwm_step_up_brightness(void);

extern void pwm_step_down_brightness(void);

extern void pwm_lock_brightness_val(uint8_t val);

extern void pwm_release_brightness(void);

extern void pwm_modifyLdrBrightness2pwmStep(void);

#endif /* _WC_PWM_H_ */
