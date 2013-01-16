/*
 * Copyright (C) 2012, 2013 Karol Babioch <karol@babioch.de>
 * Copyright (c) 2010 Frank Meyer - frank(at)fli4l.de
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
 * @file uart.h
 * @brief Interface for access to the UART hardware
 *
 * This defines an interface for access to the UART hardware. For now it is
 * only possible to send data out, which is primarily used for debugging
 * purposes. However if BOOTLOADER_RESET_UART is enabled a pretty basic ISR
 * for receiving data is registered to process the reset in order to start
 * the bootloader.
 *
 * @see BOOTLOADER_RESET_UART
 * @see uart.c
 */

#ifndef _WC_UART_H_
#define _WC_UART_H_

#include <avr/pgmspace.h>

extern void uart_init(void);

extern void uart_putc(unsigned char ch);

extern void uart_puts(const char* s);

extern void uart_puts_p(const char*s);

/**
 * @brief Macro used to automatically put a string constant into program memory
 *
 * This macro puts the given string automatically into program memory (using
 * PSTR()) and replaces the call with a call to uart_puts_p().
 *
 * @see PSTR()
 * @see uart_puts_p()
 */
#define uart_puts_P(__s) uart_puts_p(PSTR(__s))

#endif /* _WC_UART_H_ */
