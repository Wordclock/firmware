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
 * @file color_effects.c
 * @brief Actual implementation of the header declared in color_effects.h
 *
 * This implements the function declared in color_effects.h. To understand
 * the functions a bit of knowledge about the math concerning hues is needed.
 * Details can be found at [1].
 *
 * [1]: https://en.wikipedia.org/wiki/Hue
 *
 * @see color_effects.h
 */

#include "color_effects.h"
#include "main.h"

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
 * HUE_STEPS and HUE_MAX.
 *
 * This function will return a value from 0 up to 255, where 0 means there is
 * no green portion, and 255 represents completely green.
 *
 * @see Hue_t The given hue
 *
 * @return The green portion of the given hue, ranges from 0 up to 255
 *
 * @see HUE_STEPS
 * @see HUE_MAX
 * @see hue2rgb()
 *
 * [1]: https://en.wikipedia.org/wiki/File:HueScale.svg
 */
static uint8_t hueWaveform(Hue_t x)
{

    if (x < (HUE_MAX / 6)) {

        return x * ((HUE_STEPS * 6) / HUE_MAX);

    } else if (x < ((HUE_MAX * 3) / 6)) {

        return HUE_STEPS - 1;

    } else if (x < ((HUE_MAX * 4) / 6)) {

        return (((HUE_MAX * 4) / 6) - 1 - x) * ((HUE_STEPS * 6) / HUE_MAX);

    } else {

        return 0;

    }

}

/**
 * @brief Generates RGB values for a given hue
 *
 * The hue is interpreted as Hue_t and ranges from 0 up to HUE_MAX. This
 * calculations will always consider the brightness and saturation to be 1.
 * Internally after some basic calculations it makes use of hueWaveform().
 *
 * @param h The hue value to transform, range 0 up to HUE_MAX
 * @param r Pointer to variable to store the calculated red value
 * @param g Pointer to variable to store the calculated green value
 * @param b Pointer to variable to store the calculated blue value
 *
 * @see Hue_t
 * @see HUE_MAX
 * @see hueWaveform()
 */
void hue2rgb(Hue_t h, uint8_t* r, uint8_t* g, uint8_t* b)
{

    uint16_t barg = (((uint16_t)h) + 2 * HUE_MAX / 3);
    uint16_t rarg = (((uint16_t)h) + HUE_MAX / 3);

    if (barg >= HUE_MAX) {

        barg -= HUE_MAX;

    }

    if (rarg >= HUE_MAX) {

        rarg -= HUE_MAX;

    }

    *g = hueWaveform(h);
    *b = hueWaveform((Hue_t)barg);
    *r = hueWaveform((Hue_t)rarg);

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
 *  @return The calculated value for the given step
 */
uint8_t pulseWaveForm(uint8_t step)
{

    #define COLOR_PULSE_SCALE 128

    uint16_t x;
    uint8_t val;
    uint8_t t = step + COLOR_PULSE_SCALE;

    t = (t > 127) ? (255 - t) : t;
    x = (((uint16_t)t) * (256 - COLOR_PULSE_SCALE) / 128) + COLOR_PULSE_SCALE;
    val = x * x * x / 256 / 256;

    return val;

}
