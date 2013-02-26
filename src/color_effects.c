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



#include "main.h"
#include "color_effects.h"

#if (MONO_COLOR_CLOCK != 1)


/**
 *  generates the waveform for the green color
 */
static uint8_t hueWaveform(Hue_t x)
{
  if(x < HUE_MAX/6){
    return x*((HUE_STEPS*6)/HUE_MAX);
  }else if(x< (HUE_MAX*3)/6){
    return HUE_STEPS-1;
  }else if(x< (HUE_MAX*4)/6){
    return (((HUE_MAX*4)/6)-1-x)*((HUE_STEPS*6)/HUE_MAX);
  }else{
    return 0;
  }
}


/**
 * generates rgb from hue with saturation 1 and brightness 1
 */
void hue2rgb(
              Hue_t h, /*uint8  s, uint8  v,*/
              uint8_t* r, uint8_t* g, uint8_t* b
              /* ,sint32 relsat= -1 */ )
{
  uint16_t barg = (((uint16_t)h)+2*HUE_MAX/3);
  uint16_t rarg = (((uint16_t)h)+HUE_MAX/3);

  if(barg>=HUE_MAX){
    barg-=HUE_MAX;
  }
  if(rarg>=HUE_MAX){
    rarg-=HUE_MAX;
  }

  *g = hueWaveform(h);
  *b = hueWaveform( (Hue_t)barg );
  *r = hueWaveform( (Hue_t)rarg );
}



#endif  // (MONO_COLOR_CLOCK != 1)

uint8_t pulseWaveForm(uint8_t step)
{
  #define   COLOR_PULSE_SCALE          128
    uint16_t x;
    uint8_t val;
    uint8_t  t = step+COLOR_PULSE_SCALE; //start on bright time
    t = (t>127) ? (255-t) : t;
    x = ( ((uint16_t)t) *(256-COLOR_PULSE_SCALE)/128)+COLOR_PULSE_SCALE;
    val = (((x*x)/256)*x)/256;
    return val;
}

