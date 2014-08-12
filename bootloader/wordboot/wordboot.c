/*
 * Copyright (C) 2014 Karol Babioch <karol@babioch.de>
 * Copyright (C) 2010 Peter Knight
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
 * @file wordboot.c
 * @brief Bootloader of the Wordclock project
 *
 * This file contains the officially supported bootloader of the Wordclock
 * project. It is based upon [optiboot][1], but was heavily modified. These
 * modifications not only include massive clean-ups and simplifications, but
 * also support to indicate the current status visually on the Wordclock
 * itself by making use of the minute LEDs.
 *
 * [1]: https://code.google.com/p/optiboot/
 */

#define WORDBOOT_MAJOR_VERSION 6
#define WORDBOOT_MINOR_VERSION 0

#include <inttypes.h>
#include <avr/io.h>
#include <avr/pgmspace.h>
#include <avr/eeprom.h>
#include <avr/boot.h>
#include <avr/wdt.h>
#include <avr/interrupt.h>

#include "stk500.h"

#ifndef LED_START_FLASHES

    #define LED_START_FLASHES 3

#endif

#ifndef BAUD_RATE

    #define BAUD_RATE 115200L

#endif

#define BAUD BAUD_RATE
#include <util/setbaud.h>

/* Function Prototypes
 * The main() function is in init9, which removes the interrupt vector table
 * we don't need. It is also 'OS_main', which means the compiler does not
 * generate any entry or exit code itself (but unlike 'naked', it doesn't
 * supress some compile-time options we want.)
 */

int main(int argc, char* argv[]) __attribute__ ((OS_main)) __attribute__ ((section (".init9")));

void __attribute__((noinline)) put_ch(char ch);
uint8_t __attribute__((noinline)) get_ch();
void __attribute__((noinline)) verify_command_terminator();

static inline void drop_ch(uint8_t count);
static inline void flash_start_leds(uint8_t count);
static inline void write_memory(char memtype, uint8_t* buffer, uint16_t address, uint8_t length);
static inline void read_memory(char memtype, uint16_t address, uint8_t length);

void start_application(uint8_t reset_flags) __attribute__ ((naked));

/* C zero initialises all global variables. However, that requires */
/* These definitions are NOT zero initialised, but that doesn't matter */
/* This allows us to drop the zero init code, saving us memory */
#define buff ((uint8_t*)(RAMSTART))


int main(int argc, char* argv[])
{

    uint8_t ch;

    uint16_t address = 0;
    uint8_t length;

    ch = MCUSR;
    MCUSR = 0;
    wdt_disable();

    TCCR0B = _BV(CS02);

    #if (LED_START_FLASHES > 0)

        // Set up Timer 1 for timeout counter
        TCCR1B = _BV(CS12) | _BV(CS10); // div 1024

    #endif

    #if USE_2X

        UCSR0A = _BV(U2X0); //Double speed mode USART0

    #endif

    // Initialize UART
    UCSR0B = _BV(RXEN0) | _BV(TXEN0);
    UCSR0C = _BV(UCSZ00) | _BV(UCSZ01);
    UBRR0 = UBRR_VALUE;

    #if ((LED_START_FLASHES > 0) || defined(LED_DATA_FLASH))

        // Enable minute LEDs
        DDRB = _BV(PB0);
        PORTB = _BV(PB0);

        DDRC = _BV(PC3) | _BV(PC2);
        PORTC = _BV(PC3) | _BV(PC2);

        DDRD = _BV(PD7);
        PORTD = _BV(PD7);

        // Set PWM pins as output
        DDRD |= _BV(PD6) | _BV(PD5) | _BV(PD3);

        // Clear matrix
        DDRB |= _BV(PB5) | _BV(PB3) | _BV(PB2);
        PORTB |= _BV(PB2);

        SPCR = _BV(SPE) | _BV(MSTR) | _BV(CPOL);

        uint8_t i;
        for (i = 0; i < 3; i++) {

            SPDR = 0;
            while (!(SPSR & (_BV(SPIF))));

        }

        PORTB &= ~_BV(PB2);
        PORTB |= _BV(PB2);

    #endif

    #if (LED_START_FLASHES > 0)

        // Flash LEDs to signal start of bootloader
        flash_start_leds(LED_START_FLASHES * 2);

    #endif

    /* Forever loop: exits by timeout */
    while(1) {

        // Get character from UART
        ch = get_ch();

        if (ch == Cmnd_STK_GET_PARAMETER) {

            uint8_t which = get_ch();
            verify_command_terminator();

            if (which == Parm_STK_SW_MINOR) {

                put_ch(WORDBOOT_MINOR_VERSION);

            } else if (which == Parm_STK_SW_MAJOR) {

                put_ch(WORDBOOT_MAJOR_VERSION);

            } else {

                /*
                 * GET PARAMETER returns a generic 0x03 reply for other
                 * parameters - enough to keep Avrdude happy
                 */
                put_ch(0x03);

            }

        } else if(ch == Cmnd_STK_SET_DEVICE) {

            // SET DEVICE is ignored
            drop_ch(20);
            verify_command_terminator();

        } else if(ch == Cmnd_STK_SET_DEVICE_EXT) {

            // SET DEVICE EXT is ignored
            drop_ch(5);
            verify_command_terminator();

        } else if(ch == Cmnd_STK_LOAD_ADDRESS) {

            // LOAD ADDRESS
            uint16_t newAddress;

            newAddress = get_ch();
            newAddress = (newAddress & 0xff) | (get_ch() << 8);
            // Convert from word address to byte address
            newAddress += newAddress;
            address = newAddress;

            verify_command_terminator();

        } else if(ch == Cmnd_STK_UNIVERSAL) {

            // UNIVERSAL command is ignored
            drop_ch(4);
            verify_command_terminator();
            put_ch(0x00);

        } else if(ch == Cmnd_STK_PROG_PAGE) {

            uint8_t desttype;
            uint8_t *bufPtr;
            uint16_t savelength;
            // Drop high byte of length
            drop_ch(1);
            length = get_ch();
            savelength = length;
            desttype = get_ch();

            // read a page worth of contents
            bufPtr = buff;
            do {

                *bufPtr++ = get_ch();

            } while (--length);

            // Read command terminator, start reply
            verify_command_terminator();

            write_memory(desttype, buff, address, savelength);

        } else if(ch == Cmnd_STK_READ_PAGE) {

            uint8_t desttype;
            // Drop high byte of length
            drop_ch(1);
            length = get_ch();
            desttype = get_ch();

            verify_command_terminator();

            read_memory(desttype, address, length);

        } else if(ch == Cmnd_STK_READ_SIGN) {

            // READ SIGN - return what Avrdude wants to hear
            verify_command_terminator();
            put_ch(SIGNATURE_0);
            put_ch(SIGNATURE_1);
            put_ch(SIGNATURE_2);

        } else if (ch == Cmnd_STK_LEAVE_PROGMODE) {

            // Adaboot no-wait mod
            wdt_enable(WDTO_15MS);

            verify_command_terminator();

        } else {

            // This covers the response to commands like STK_ENTER_PROGMODE
            verify_command_terminator();

        }

        put_ch(Resp_STK_OK);

    }

}

void put_ch(char ch)
{

    while (!(UCSR0A & _BV(UDRE0)));
    UDR0 = ch;

}

#define BOOTLOADER_TIMEOUT_MS 1000
#define BOOTLOADER_TIMEOUT_COMPARE_VALUE F_CPU / 256 / 256 * 1000 / BOOTLOADER_TIMEOUT_MS

uint8_t get_ch()
{

    uint8_t ch;

    #ifdef LED_DATA_FLASH

        PIND = _BV(PD6) | _BV(PD5) | _BV(PD3);

    #endif

    uint8_t counter = 0;

    while(!(UCSR0A & _BV(RXC0))) {

        if (TIFR0 & _BV(TOV0)) {

            counter++;
            TIFR0 = _BV(TOV0);

        }

        if (counter > BOOTLOADER_TIMEOUT_COMPARE_VALUE) {

            start_application(0);

        }

    }

    /*
     * A Framing Error indicates (probably) that something is talking
     * to us at the wrong bit rate, so the microcontroller will be reset.
     */
    if (UCSR0A & _BV(FE0)) {

        wdt_enable(WDTO_15MS);

    }

    ch = UDR0;

    #ifdef LED_DATA_FLASH

        PIND = _BV(PD6) | _BV(PD5) | _BV(PD3);

    #endif

    return ch;

}

void drop_ch(uint8_t count)
{

    do {

        get_ch();

    } while (--count);

}

void verify_command_terminator()
{

    if (get_ch() == Sync_CRC_EOP){

        put_ch(Resp_STK_INSYNC);

    } else {

        wdt_enable(WDTO_15MS);
        while (1);

    }

}

#if (LED_START_FLASHES > 0)

    void flash_start_leds(uint8_t count)
    {

        do {

            TCNT1 = -(F_CPU / (1024 * 16));
            TIFR1 = _BV(TOV1);

            while(!(TIFR1 & _BV(TOV1)));

            PIND = _BV(PD6) | _BV(PD5) | _BV(PD3);

        } while (--count);

    }

#endif

void start_application(uint8_t reset_flags)
{

    // Reset I/O registers
    DDRB = 0;
    PORTB = 0;

    DDRC = 0;
    PORTC = 0;

    DDRD = 0;
    PORTD = 0;
    PIND = 0;

    // Reset UART module
    UCSR0A = 0;
    UCSR0B = 0;
    UCSR0C = 0;
    UBRR0 = 0;

    // Reset Timer0
    TCCR0B = 0;
    TCNT0 = 0;

    // Reset Timer1
    TCCR1B = 0;
    TCNT1 = 0;

    // Reset SPI module
    SPCR = 0;

    // Save the reset flags in the designated register
    // This can be accessed in a main program by putting code in .init0 (which
    // executes before normal C init code) to save R2 to a global variable.
    __asm__ __volatile__ ("mov r2, %0\n" :: "r" (reset_flags));

    // Jump to reset vector
    ((void(*)(void)) 0x0000)();

}

static inline void write_memory(char memtype, uint8_t* buffer, uint16_t address, uint8_t length)
{

    switch (memtype) {

        // EEPROM
        case 'E':

            while(length--) {

                eeprom_write_byte((uint8_t *)(address++), *buffer++);

            }

            break;

        // FLASH
        default:
        {

            // Copy buffer into programming buffer
            uint8_t *bufPtr = buffer;
            uint16_t addrPtr = (uint16_t)(void*)address;

            // Start the page erase and wait for it to finish
            boot_page_erase((uint16_t)(void*)address);
            boot_spm_busy_wait();

            // Copy data from the buffer into the flash write buffer
            do {

                uint16_t a;

                a = *bufPtr++;
                a |= (*bufPtr++) << 8;
                boot_page_fill((uint16_t)(void*)addrPtr,a);
                addrPtr += 2;

            } while (length -= 2);

            // Actually write the buffer to flash and wait for it to finish
            boot_page_write((uint16_t)(void*)address);
            boot_spm_busy_wait();

            // Reenable read access to flash
            boot_rww_enable();

        }

        break;

    }

}

static inline void read_memory(char memtype, uint16_t address, uint8_t length)
{

    switch (memtype) {

        // EEPROM
        case 'E':

            do {

                put_ch(eeprom_read_byte((uint8_t *)(address++)));

            } while (--length);

            break;

        // FLASH
        default:

            do {

                put_ch(pgm_read_byte(address++));

            } while (--length);

            break;

    }

}
