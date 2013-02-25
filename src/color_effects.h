/*
 * Copyright (C) 2012, 2013 Karol Babioch <karol@babioch.de>
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
 * @file color_effects.h
 * @brief Header containing functions for various color animations
 *
 * These functions are based upon the hue of the color. Hue is a main property
 * of a color. Take a look at [1] for more details about it.
 *
 * [1]: https://en.wikipedia.org/wiki/Hue
 *
 * @see color_effects.c
 */

#ifndef _WC_COLOR_EFFECTS_H_
#define _WC_COLOR_EFFECTS_H_

#define HUE_STEPS 256
#define HUE_MAX (HUE_STEPS * 6)      // 192 @ 32 PWM Steps /// 384 @ 64 PWM Steps
#define HUE_MANUAL_STEPS 10
#define SetPWMs pwm_set_colors
typedef uint16_t Hue_t;

/**
 * generates rgb from hue with saturation 1 and brightness 1
 * @param  h   the hue value to transform [0..HUE_MAX)
 * @param  r   retreives the calculated red value
 * @param  g   retreives the calculated green value
 * @param  b   retreives the calculated blue value
 */
extern void hue2rgb(
              Hue_t h, /*uint8  s, uint8  v,*/
              uint8_t* r, uint8_t* g, uint8_t* b
              /* ,sint32 relsat= -1 */ );

/** 
 *  generates a spiky symmetric cubic waveform 
 *  @param step   current step of animation [0..255]
 */
extern uint8_t pulseWaveForm(uint8_t step);

#endif /* _WC_COLOR_EFFECTS_H_ */
