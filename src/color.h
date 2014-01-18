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
 * @file color.h
 * @brief Header containing functions for various color animations
 *
 * These functions are based upon the hue of the color. Hue is a main property
 * of a color. Take a look at [1] for more details about it.
 *
 * [1]: https://en.wikipedia.org/wiki/Hue
 *
 * @see color.c
 */

#ifndef _WC_COLOR_H_
#define _WC_COLOR_H_

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
#define COLOR_HUE_STEPS 256

/**
 * @brief Largest value for hue
 *
 * There are six different hue regions (Red-Yellow, Yellow-Green, Green-Cyan,
 * Cyan-Blue, Blue-Magenta, Magenta-Red, see [1]). For each region there are
 * COLOR_HUE_STEPS different hues, so the largest possible value is a simple
 * multiplication.
 *
 * [1]: https://en.wikipedia.org/wiki/Hue#Computing_hue_from_RGB
 *
 * @see COLOR_HUE_STEPS
 */
#define COLOR_HUE_MAX (COLOR_HUE_STEPS * 6)

/**
 * @brief The actual type when dealing with hues
 *
 * When dealing with hues this is the type which will be used. Note that it
 * must at least hold values from 0 up to COLOR_HUE_MAX.
 *
 * @see COLOR_HUE_MAX
 */
typedef uint16_t Hue_t;

/**
 * @brief Type definition for a RGB color
 *
 * This is a simple container for a RGB color. It simply combines all of the
 * three colors into a single struct and should be used whenever dealing with
 * RGB colors.
 */
typedef struct {

        /**
         * @brief The red portion of the color
         */
        uint8_t red;

        /**
         * @brief The green portion of the color
         */
        uint8_t green;

        /**
         * @brief The blue portion of the color
         */
        uint8_t blue;

} color_rgb_t;

extern void color_hue2rgb(Hue_t h, color_rgb_t* color);

extern uint8_t color_pulse_waveform(uint8_t step);

#endif /* _WC_COLOR_H_ */
