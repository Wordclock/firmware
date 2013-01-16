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
 * @file shift.c
 * @brief Implementation of the interface for the shift register cascade
 *
 * This file contains the actual implementation of the interface defined in
 * shift.h.
 *
 * @see shift.c
 */

#include <inttypes.h>
#include <avr/io.h>
#include <util/delay.h>

#include "main.h"
#include "shift.h"

/**
 * @brief The port the SPI is attached to
 */
#define SHIFT_SR_SPI_PORT PORTB

/**
 * @brief The DDR of the port the SPI is attached to
 */
#define SHIFT_SR_SPI_DDR DDRB

/**
 * @brief The pin the MOSI line of the SPI is attached to
 */
#define SHIFT_SR_SPI_MOSI PIN3

/**
 * @brief The pin the MISO line of the SPI is attached to
 *
 * Although this pin is actually not needed to output data, it has to be set
 * to input in order for the SPI to act as master, see [1], page 163,
 * table 19-1.
 *
 * [1] http://www.atmel.com/images/doc2545.pdf
 */
#define SHIFT_SR_SPI_MISO PIN4

/**
 * @brief The pin the RCLK lines of the shift registers are attached to
 *
 * This is used to transfer the the contents of the shift registers to the
 * storage registers to output them in parallel.
 */
#define SHIFT_SR_SPI_RCLK PIN2

/**
 * @brief The pin the SCK lines of the shift registers are attached to
 *
 * This is the clock line used for the shift registers.
 */
#define SHIFT_SR_SPI_SCK PIN5

/**
 * @brief Initializes this module
 *
 * This function has to be called once before using shift24_output(). It sets
 * up the hardware by writing the right values into various registers in
 * control of the SPI module. Afterwards data can be output using
 * shift24_output().
 *
 * @see shift24_output()
 */
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

/**
 * @brief Outputs the given over the Serial Peripheral Interface (SPI)
 *
 * This function is responsible for the actual output. It takes the given data
 * at blocks of eight bits and puts them into the appropriate register (SPDR).
 * After the transfer has completed it takes the next byte.
 *
 * Before this function can be used, the module needs to be initialized by
 * calling shift24_init().
 *
 * Although the argument accepts 32 bits (uint32_t), only 24 of the least
 * significant bits will be output. The eight most significant bits are
 * completely ignored.
 *
 * @param data Data to be output
 * @see shift24_init()
 */
void
shift24_output (uint32_t data)
{
  uint8_t u0 = (uint8_t)(data);     /* to if this somehow can be put between SPDR=x and while() */
  uint16_t u16 = (uint16_t)(data  >>  8);
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


