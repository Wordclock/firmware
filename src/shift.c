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
 * @file shift.c
 * 
 *  This file implements the SPI control of the shift register cascade
 *
 * \version $Id: shift.c 285 2010-03-24 21:43:24Z vt $
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

#include "main.h"
#include "shift.h"

#define SHIFT_SR_SPI_DDR  DDRB
#define SHIFT_SR_SPI_PORT PORTB
#define SHIFT_SR_SPI_MOSI PIN3
#define SHIFT_SR_SPI_MISO PIN4 /* not used, but has to be input*/
#define SHIFT_SR_SPI_RCLK PIN2
#define SHIFT_SR_SPI_SCK  PIN5

void shift24_init (void)
{
  SHIFT_SR_SPI_DDR |= (1<< SHIFT_SR_SPI_MOSI) 
                   |  (1 << SHIFT_SR_SPI_RCLK)
                   |  (1 << SHIFT_SR_SPI_SCK);
  SHIFT_SR_SPI_DDR  &= ~(1<< SHIFT_SR_SPI_MISO); /* MISO muss eingang sein */
  SHIFT_SR_SPI_PORT |= (1<< SHIFT_SR_SPI_RCLK)
                    |  (1<< SHIFT_SR_SPI_MISO);

   // SPI als Master 
   // High-Bits zuerst 
   // SCK ist HIGH wenn inaktiv 
   SPCR = (1 << SPE) | (1 << MSTR) | (1 << CPOL);
	
   // maximale Geschwindigkeit: F_CPU / 2 
   SPSR |= (1 << SPI2X);

   shift24_output(0); /* send dummybytes to intialize */
}

void
shift24_output (uint32_t value)
{
  uint8_t u0 = (uint8_t)(value);     /* to if this somehow can be put between SPDR=x and while() */
  uint16_t u16 = (uint16_t)(value  >>  8);
  uint8_t u2 = (uint8_t)(u16 >> 8);

 
  SPDR = u2;                      // SPDR schreiben startet Uebertragung 
  while (!(SPSR & (1 << SPIF)));  // warten auf Ende der Uebertragung für dieses Byte

  uint8_t u1 = (uint8_t)(u16);
  SPDR = u1;
  while (!(SPSR & (1 << SPIF)));

  SPDR = u0;
  while (!(SPSR & (1 << SPIF)));

  /* latch data */
  SHIFT_SR_SPI_PORT &= ~(1<< SHIFT_SR_SPI_RCLK);
  SHIFT_SR_SPI_PORT |=  (1<< SHIFT_SR_SPI_RCLK);
}


