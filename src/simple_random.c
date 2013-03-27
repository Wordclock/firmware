/*
 * Copyright (C) 2012, 2013 Karol Babioch <karol@babioch.de>
 * Copyright (c) 2013 Vlad Tepesch
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

/*------------------------------------------------------------------------------------------------------------------------------------------------*//**
 * @file simple_random.c
 *
 *  This file implements a very simple 8bit random number generator
 * \version $Id: $
 *
 * \author Copyright (c) 2013 Vlad Tepesch
 *
 * \remarks
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 */
 /*-----------------------------------------------------------------------------------------------------------------------------------------------*/

#include "simple_random.h"

#define SR_PRIME_MULT 17
#define SR_PRIME_ADD  37

static uint8_t g_lastVal=1;

void simpleRand_setSeed(uint8_t i_seed)
{
  g_lastVal = i_seed;
}

uint8_t simpleRand_get()
{
  g_lastVal = (g_lastVal*SR_PRIME_MULT) + SR_PRIME_ADD; // modulo 255 is implicit
  return g_lastVal;
}

