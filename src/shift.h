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
 * @file shift.h
 * 
 *  The shift-module provides the interface to the SPI shift register cascade
 *
 * \version $Id: shift.h 285 2010-03-24 21:43:24Z vt $
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


#ifndef _WC_SHIFT_H_
#define _WC_SHIFT_H_

#include "avr/io.h"

#ifdef __cplusplus
extern "C"
{
#endif


#define SHIFT_SR_SPI_DDR  DDRB
#define SHIFT_SR_SPI_PORT PORTB
#define SHIFT_SR_SPI_MOSI PIN3
#define SHIFT_SR_SPI_MISO PIN4 /* not used, but has to be input*/
#define SHIFT_SR_SPI_RCLK PIN2
#define SHIFT_SR_SPI_SCK  PIN5

extern void		shift24_init (void);
extern void		shift24_output (uint32_t value);


#ifdef __cplusplus
}
#endif

#endif /* _WC_SHIFT_H_ */
