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
#define HUE_MAX (HUE_STEPS * 6)
#define HUE_MANUAL_STEPS 10
#define SetPWMs pwm_set_colors

typedef uint16_t Hue_t;

extern void hue2rgb(Hue_t h, uint8_t* r, uint8_t* g, uint8_t* b);

extern uint8_t pulseWaveForm(uint8_t step);

#endif /* _WC_COLOR_EFFECTS_H_ */
