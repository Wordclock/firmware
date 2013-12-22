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
 * @brief Implementation of the header declared in uart.h.
 *
 * This implements the various functions declared in uart.h in order to utilize
 * the UART hardware. Furthermore it implements a rather basic ISR,
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

    #include <avr/interrupt.h>

    #if (BOOTLOADER_RESET_WDT == 1)

        #include <avr/wdt.h>

    #endif

#endif

/**
 * @brief The baud rate used for the serial communication
 *
 * This gets redefined here because the header that is included below expects
 * the macro to be called "BAUD". It will be undefined once it is no longer
 * actually needed.
 *
 * @see UART_BAUD
 * @see uart_init()
 */
#define BAUD UART_BAUD

#include <util/setbaud.h>

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
void uart_init()
{

    /*
     * Check whether bootloader support is enabled
     */
    #if (BOOTLOADER_RESET_UART == 1)

        /*
         * Bootloader support is activated, both receiver and transmitter are
         * needed
         *
         * TXEN0: Enable transmitter
         * RXEN0: Enable receiver
         * RXCIE0: Enable receiver
         */
        UCSR0B |= _BV(TXEN0) | _BV(RXEN0) | _BV(RXCIE0);

    #else

        /*
         * Bootloader support is disabled, only transmitter is needed
         *
         * TXEN0: Enable transmitter
         */
        UCSR0B |= _BV(TXEN0);

    #endif

    /*
     * Set baud rate according to calculated value
     */
    UBRR0 = UBRR_VALUE;

    /*
     * Check whether speed should be doubled
     */
    #if (USE_2X)

        /*
         * U2X: Double the UART transmission speed
         */
        UCSR0A = _BV(U2X);

    #endif

}

/**
 * @brief Transmits a single character
 *
 * This functions transmits a single character using the UART hardware. It
 * basically just puts the data into the UDR0 register. However it might be
 * possible that the last transmission is not yet completed, so that the
 * UDRE0 bit within the UCSR0A register needs to be polled (busy waiting).
 *
 * @param c Character to transmit
 */
void uart_putc(char c)
{

    while (!(UCSR0A & _BV(UDRE0)));

    UDR0 = c;

}

/**
 * @brief Transmits a complete string
 *
 * This functions transmits a complete string. The string needs to be null
 * terminated. Internally it makes use of uart_putc(), so each character gets
 * transmitted individually.
 *
 * @param s Pointer to string to transmit
 *
 * @see uart_putc()
 */
void uart_puts(const char* s)
{

    while (*s) {

        uart_putc(*s++);

    }

}

/**
 * @brief Transmits a complete string stored in program memory
 *
 * This functions transmits a complete string stored in program memory. The
 * string needs to be null terminated. Internally it makes use of
 * pgm_read_byte() to retrieve the data and uart_putc() to transmit it.
 *
 * @param s Pointer to string stored in program memory to transmit
 *
 * @see uart_putc()
 * @see pgm_read_byte()
 */
void uart_puts_p(const char* s)
{

    char ch;

    while ((ch = pgm_read_byte(s++)) != '\0') {

        uart_putc(ch);

    }

}

/**
 * @brief ISR for data received over UART
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

    ISR(USART_RX_vect)
    {

        /*
         * Check whether received data was R
         */
        if (UDR0 == 'R') {

            /*
             * Check whether reset should be performed using the watchdog timer
             */
            #if (BOOTLOADER_RESET_WDT == 1)

                /*
                 * Enable watchdog timer with shortest possible value
                 */
                wdt_enable(WDTO_15MS);

                /*
                 * Do nothing and wait for the reset performed by the watchdog
                 * timer
                 */
                while (1);

            #else

                /*
                 * No actual reset needed, jump to chip45boot2 directly
                 */
                asm volatile("jmp 0x3800");

            #endif

        }

    }

#endif

/*
 * Undefine BAUD macro to prevent name collisions as it is no longer needed
 */
#undef BAUD
