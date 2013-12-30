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
 * @file color.c
 * @brief Actual implementation of the header declared in color.h
 *
 * This implements the function declared in color.h. To get a better
 * understanding about the details of this functions knowledge about the math
 * concerning hues is helpful. Details can be found at [1].
 *
 * [1]: https://en.wikipedia.org/wiki/Hue
 *
 * @see color.h
 */

#include "color.h"
#include "main.h"

/**
 * @brief Scale factor used for calculation of the signal within
 * color_pulse_waveform().
 *
 * @see color_pulse_waveform()
 */
#define COLOR_PULSE_WAVEFORM_SCALE 128

#if (MONO_COLOR_CLOCK != 1)

/**
 * @brief Returns the green portion of a given hue
 *
 * The color space can be understood as a spectrum from 0 to 360 degrees, see
 * [1]. Colors from 0 to 240 degrees contain green portions (green hue) with
 * varying values of saturation and brightness.
 *
 * From 0 up to 60 degrees the hue increases linearly, from 60 up to 180
 * degrees the hue is at a maximum. From 180 up to 240 degrees the hue
 * decreases linearly again. From 240 degrees up to 360 degrees there is no
 * green portion (hue) left. The various degrees are calculated using
 * COLOR_HUE_STEPS and COLOR_HUE_MAX.
 *
 * This function will return a value from 0 up to 255, where 0 means there is
 * no green portion, and 255 represents completely green.
 *
 * @see Hue_t The given hue
 *
 * @return The green portion of the given hue, ranges from 0 up to 255
 *
 * @see COLOR_HUE_STEPS
 * @see COLOR_HUE_MAX
 * @see color_hue2rgb()
 *
 * [1]: https://en.wikipedia.org/wiki/File:HueScale.svg
 */
static uint8_t color_hue_waveform(Hue_t x)
{

    if (x < (COLOR_HUE_MAX / 6)) {

        return x * ((COLOR_HUE_STEPS * 6) / COLOR_HUE_MAX);

    } else if (x < ((COLOR_HUE_MAX * 3) / 6)) {

        return COLOR_HUE_STEPS - 1;

    } else if (x < ((COLOR_HUE_MAX * 4) / 6)) {

        return (((COLOR_HUE_MAX * 4) / 6) - 1 - x)
                    * ((COLOR_HUE_STEPS * 6) / COLOR_HUE_MAX);

    } else {

        return 0;

    }

}

/**
 * @brief Converts a hue into its RGB values
 *
 * The hue is interpreted as Hue_t and ranges from 0 up to COLOR_HUE_MAX. This
 * calculations will always consider the brightness and saturation to be 1.
 * Internally it makes heavily use of color_hue_waveform().
 *
 * @param h The hue value to convert, ranges from 0 up to COLOR_HUE_MAX
 * @param r Pointer to variable to store the calculated red value
 * @param g Pointer to variable to store the calculated green value
 * @param b Pointer to variable to store the calculated blue value
 *
 * @see Hue_t
 * @see COLOR_HUE_MAX
 * @see color_hue_waveform()
 */
void color_hue2rgb(Hue_t h, uint8_t* r, uint8_t* g, uint8_t* b)
{

    uint16_t barg = (((uint16_t)h) + 2 * COLOR_HUE_MAX / 3);
    uint16_t rarg = (((uint16_t)h) + COLOR_HUE_MAX / 3);

    if (barg >= COLOR_HUE_MAX) {

        barg -= COLOR_HUE_MAX;

    }

    if (rarg >= COLOR_HUE_MAX) {

        rarg -= COLOR_HUE_MAX;

    }

    *g = color_hue_waveform(h);
    *b = color_hue_waveform((Hue_t)barg);
    *r = color_hue_waveform((Hue_t)rarg);

}

#endif /* (MONO_COLOR_CLOCK != 1) */

/**
 * @brief Generates a cyclic spiky signal
 *
 * The function expects a single parameter representing the current step. The
 * signal is generated based upon this step.
 *
 * @param step Current step of animation, ranges from 0 up to 255
 *
 * @return The calculated value for the given step
 */
uint8_t color_pulse_waveform(uint8_t step)
{

    uint16_t x;
    uint8_t val;
    uint8_t t = step + COLOR_PULSE_WAVEFORM_SCALE;

    t = (t > 127) ? (255 - t) : t;
    x = (((uint16_t)t) * (256 - COLOR_PULSE_WAVEFORM_SCALE) / 128)
               + COLOR_PULSE_WAVEFORM_SCALE;
    val = (((x * x) / 256) * x) / 256;

    return val;

}
