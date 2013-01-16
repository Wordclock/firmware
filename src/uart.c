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
 * @file uart.c
 * @brief Implementation of the interface for access to the UART hardware
 *
 * This implements the various functions declared in uart.h in order to enable
 * access to the UART hardware. Furthermore it implements a rather basic ISR,
 * which will be executed once data has been received. This is used to start
 * the bootloader, either by using the watchdog timer or by jumping to it
 * directly.
 *
 * @see uart.h
 */

#include <inttypes.h>
#include <avr/io.h>
#include <avr/pgmspace.h>

#include "uart.h"
#include "main.h"

#if (BOOTLOADER_RESET_UART == 1)
#if (BOOTLOADER_RESET_WDT == 1)
#include <avr/wdt.h>
#endif
#include <avr/interrupt.h>
#endif

#define UART_BAUD             9600L

// calculate real baud rate:
#define UBRR_VAL              ((F_CPU+UART_BAUD*8)/(UART_BAUD*16)-1)            // round
#define BAUD_REAL             (F_CPU/(16*(UBRR_VAL+1)))                         // real baudrate
#define BAUD_ERROR            ((BAUD_REAL*1000)/UART_BAUD)                      // error in promille

#if ((BAUD_ERROR < 990) || (BAUD_ERROR > 1010))
#  error Error of baud rate of RS232 UART is more than 1%. That is too high!
#endif

/**
 * @brief Initializes the UART hardware
 *
 * This functions initializes the UART hardware. It needs to be called once
 * **before** other functions of this module can be used.
 *
 * For a detailed description of the various registers used here, take a look
 * at [1], p. 190f.
 *
 * [1]: http://www.atmel.com/images/doc2545.pdf
 */
void
uart_init (void)
{
#if (BOOTLOADER_RESET_UART == 1)
  UCSR0B |= (1<<TXEN0)|(1<<RXEN0)|(1<<RXCIE0);                                  // activate UART0 TX,RX,RXINT
#else
  UCSR0B |= (1<<TXEN0);                                                         // activate UART0 TX
#endif
  UBRR0H = UBRR_VAL >> 8;                                                       // store baudrate (upper byte)
  UBRR0L = UBRR_VAL & 0xFF;                                                     // store baudrate (lower byte)
}

/**
 * @brief Transmits a single character
 *
 * This functions transmits a single character using the UART hardware. It
 * basically just puts the data into the UDR0 register. However it might be
 * possible that the last transmission is not yet completed, so that the
 * UDRE0 bit within the UCSR0A register needs to be polled (busy waiting).
 *
 * @param ch Character to transmit
 */
void
uart_putc (unsigned char ch)
{
  while (!(UCSR0A & (1<<UDRE0)))
  {
    ;
  }

  UDR0 = ch;
}

/**
 * @brief Transmits a string
 *
 * This functions transmits a complete string. The string needs to be null
 * terminated. Internally it makes use of uart_putc(), so each character gets
 * transmitted individually.
 *
 * @param s Pointer to string to transmit
 * @see uart_putc()
 */
void
uart_puts (const char * s)
{
  while (*s)
  {
    uart_putc (*s++);
  }
}

/**
 * @brief Transmits a string stored in program memory
 *
 * This functions transmits a complete string stored in program memory. The
 * string needs to be null terminated. Internally it makes use of
 * pgm_read_byte() to retrieve the data and uart_putc() to transmit it.
 *
 * @param progmem_s Pointer to string stored in program memory to transmit
 * @see uart_putc()
 * @see pgm_read_byte()
 */
void
uart_puts_p (const char * progmem_s)
{
  char ch;

  while ((ch = pgm_read_byte (progmem_s++)) != '\0')
  {
    uart_putc (ch);
  }
}

/**
 * @brief ISR for data received on UART
 *
 * This ISR is only available when BOOTLOADER_RESET_UART is enabled. When a "R"
 * is received, it will reset the microcontroller and start the bootloader.
 *
 * Depending on the setting of BOOTLOADER_RESET_WDT this is either achieved by
 * enabling the watchdog timer and waiting for it to reset the microcontroller,
 * which will then start the bootloader or by directly jumping to it. The
 * latter is needed for [chip45boot2][1].
 *
 * [1]: http://www.chip45.com/avr_bootloader_atmega_xmega_chip45boot2.php
 *
 * @see BOOTLOADER_RESET_UART
 * @see BOOTLOADER_RESET_WDT
 */
#if (BOOTLOADER_RESET_UART == 1)
ISR(USART_RX_vect){
  if(UDR0=='R')                                                                 //Reset-Signal für BT-Bootloader bekommen?
  {
#if (BOOTLOADER_RESET_WDT == 1)                                                 // Reset via Watchdog
    //cli();                                                                      // disable Interrupts - not neccessary because interupt context
    wdt_enable(WDTO_15MS);                                                      // enable Watchdog Timer with lowest Timeout Value
    while(1);                                                                   // will never end until Watchdog Reset
#else
    asm volatile ("jmp 0x3800");                                                // jump directly to chip45boot2 bootloader
#endif
  }
}
#endif
