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

#include <stdint.h>

/**
 * @brief Number of different hue steps
 *
 * A hue is basically only a number. This defines the amount of different hues
 * for a specific region ((Red-Yellow, Yellow-Green, Green-Cyan, Cyan-Blue,
 * Blue-Magenta, Magenta-Red, see [1]).
 *
 * [1]: https://en.wikipedia.org/wiki/Hue#Computing_hue_from_RGB
 *
 */
#define HUE_STEPS 256

/**
 * @brief Largest value for hue
 *
 * There are six different hue regions (Red-Yellow, Yellow-Green, Green-Cyan,
 * Cyan-Blue, Blue-Magenta, Magenta-Red, see [1]). For each region there are
 * HUE_STEPS different hues, so the largest possible value is a simple
 * multiplication.
 *
 * [1]: https://en.wikipedia.org/wiki/Hue#Computing_hue_from_RGB
 *
 * @see HUE_STEPS
 */
#define HUE_MAX (HUE_STEPS * 6)

/**
 * @brief Amount of steps to change when requested manually
 *
 * Whenever a hue change is requested (up and/or down) manually by the user
 * this defines the amount of steps to change the hue by. It basically is a
 * tradeoff between granular control and an actual visible change to the color.
 */
#define HUE_MANUAL_STEPS 10

/**
 * @brief The actual type when dealing with hues
 *
 * When dealing with hues this is the type which will be used. Note that it
 * must at least hold values from 0 up to HUE_MAX.
 *
 * @see HUE_MAX
 */
typedef uint16_t Hue_t;

extern void hue2rgb(Hue_t h, uint8_t* r, uint8_t* g, uint8_t* b);

extern uint8_t pulseWaveForm(uint8_t step);

#endif /* _WC_COLOR_EFFECTS_H_ */
