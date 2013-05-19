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

/*------------------------------------------------------------------------------------------------------------------------------------------------*//**
 * @file display.c
 * 
 *  Implementation of the language-independent display stuff
 *
 * \version $Id: display.c 423 2012-03-20 18:43:53Z pn $
 * 
 * \author Copyright (c) 2010 Vlad Tepesch    
 * 
 * \remarks
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 */
 /*-----------------------------------------------------------------------------------------------------------------------------------------------*/


#include <inttypes.h>
#include <avr/io.h>
#include <util/delay.h>
#include <avr/pgmspace.h>
#include <avr/interrupt.h>

#include "main.h"
#include "display.h"
#include "user.h"
#include "uart.h"

#include "pwm.h"


/** the amount of pwm cycles after that the dutycycle has to be adapted */
/** Stuff for fading display */
#define DISPLAY_FADE_STEPS  20                 // PN: Anzahl der Schritte beim Überblenden
#define DISPLAY_FADE_PERIOD      ((uint16_t)((( ((uint32_t)DISPLAY_TIMER_FREQUENCY)*DISPLAY_FADE_TIME_MS )/1000)/DISPLAY_FADE_STEPS))
#define DISPLAY_FADE_PERIOD_ANIM ((uint16_t)((( ((uint32_t)DISPLAY_TIMER_FREQUENCY)*DISPLAY_FADE_TIME_ANIM_MS )/1000)/DISPLAY_FADE_STEPS))
                                               // PN: Anzahl der PWM-Zyklen pro Schritt
											   // PN: !!! VORSICHT !!! Als Schrittdauer muss mindestens
											   // PN: die Dauer eine PWM-Periode zur Verfügung stehen  !!!



static uint32_t g_oldDispState;
static uint32_t g_curDispState;
static uint32_t g_blinkState;
static uint8_t g_curFadeCounter;             // PN
static uint8_t g_curFadeStep;                // PN
static uint16_t g_curFadeStepTimer;          // PN


void display_setDisplayState( DisplayState i_showStates, uint32_t i_blinkstates)
{
  g_blinkState   = i_blinkstates & i_showStates;
  g_oldDispState = g_curDispState;
  g_curDispState = i_showStates;
  g_curFadeStep  = 0;
  display_outputData(g_curDispState); 
}


void display_fadeDisplayState( DisplayState i_showStates)
{
  g_blinkState   = 0;
  g_oldDispState = g_curDispState;
  g_curDispState = i_showStates;
  g_curFadeStep = DISPLAY_FADE_STEPS - 1;                                   // PN
  if(useAutoOffAnimation)
    {
	g_curFadeStepTimer = (DISPLAY_FADE_PERIOD_ANIM/DISPLAY_FADE_STEPS) - 1; // PN
	}
  else
    {
	g_curFadeStepTimer = (DISPLAY_FADE_PERIOD/DISPLAY_FADE_STEPS) - 1;      // PN
	}
  g_curFadeCounter = DISPLAY_FADE_STEPS - 1;                                // PN 
}



//void fadeTimerOCR(void)
ISR( DISPLAY_TIMER_OCR_vect )
{
                                                   // PN: Überblendung nur im OVF
}


//void displayFadeTimerOvf (void)
ISR( DISPLAY_TIMER_OVF_vect )
{
  // PN: Neue Überblendung
  if (g_curFadeStep > 0)
    {
    if (g_curFadeCounter >= g_curFadeStep)
      {
	  display_outputData( g_curDispState );
	  }
    else
      {
	  display_outputData( g_oldDispState );
	  }

    if (g_curFadeCounter)
      {
	  g_curFadeCounter--;
	  }
    else
      {
	  g_curFadeCounter = DISPLAY_FADE_STEPS - 1;

	  if (g_curFadeStepTimer)
	    {
	    g_curFadeStepTimer--;
		}
      else 
	    {
		if(useAutoOffAnimation)
          {
	      g_curFadeStepTimer = (DISPLAY_FADE_PERIOD_ANIM/DISPLAY_FADE_STEPS) - 1;
	      }
        else
          {
	      g_curFadeStepTimer = (DISPLAY_FADE_PERIOD/DISPLAY_FADE_STEPS) - 1;
		  }
		g_curFadeStep--;
	    }
	  }

	}
  else
    {
	display_outputData( g_curDispState );
	}
  // PN: Ende neue Überblendung
}



/**
 * @see INTERRUPT_10HZ
 */
void display_blinkStep (void)
{
  if(    g_blinkState 
     && (g_curFadeStep == 0))
  {
    static uint8_t s_blinkPrescale = DISPLAY_BLINK_INT_100MS;
    if( ! (--s_blinkPrescale) )
    {
      g_curDispState ^= g_blinkState;
      display_outputData(g_curDispState);
      s_blinkPrescale = DISPLAY_BLINK_INT_100MS;
    }
  }
}


