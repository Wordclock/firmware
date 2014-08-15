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
 * @brief Bootloader for the Wordclock project
 *
 * This file contains the officially supported bootloader for the Wordclock
 * project. It is based upon [optiboot][1], but was heavily modified. These
 * modifications not only include massive clean-ups and simplifications, but
 * also add support to indicate the current status on the Wordclock frontpanel.
 *
 * [1]: https://code.google.com/p/optiboot/
 */

#include <avr/boot.h>
#include <avr/eeprom.h>
#include <avr/interrupt.h>
#include <avr/io.h>
#include <avr/pgmspace.h>
#include <avr/wdt.h>

#include <inttypes.h>

#include "stk500.h"

/**
 * @brief Major version of the bootloader
 *
 * This version number is being returned by the bootloader (along with
 * WORDBOOT_MINOR_VERSION) and is evaluated by the programmer, e.g. avrdude.
 *
 * @see WORDBOOT_MINOR_VERSION
 */
#define WORDBOOT_MAJOR_VERSION 6

/**
 * @brief Minor version of the bootloader
 *
 * This version number is being returned by the bootloader (along with
 * WORDBOOT_MAJOR_VERSION) and is evaluated by the programmer, e.g. avrdude.
 *
 * @see WORDBOOT_MAJOR_VERSION
 */
#define WORDBOOT_MINOR_VERSION 0

#ifndef LED_START_FLASHES

    /**
     * @brief Default number of LED flashes during startup
     */
    #define LED_START_FLASHES 3

#endif

#ifndef BAUD_RATE

    /**
     * @brief Default baud rate used by the UART hardware module
     */
    #define BAUD_RATE 9600L

#endif

/**
 * @brief Helper macro to define the baud rate as expected by <util/setbaud.h>
 */
#define BAUD BAUD_RATE
#include <util/setbaud.h>

#ifndef BOOTLOADER_TIMEOUT_MS

    /**
     * @brief Default timeout (in ms) after which the application is started
     *
     * @see get_ch()
     */
    #define BOOTLOADER_TIMEOUT_MS 1000

#endif

/**
 * @brief Compare value for timeout
 *
 * This is the actual value that the timeout counter is being compared against
 * to determine whether the bootloader should be exited.
 *
 * @see BOOTLOADER_TIMEOUT_MS
 * @see get_ch()
 */
#define BOOTLOADER_TIMEOUT_COMPARE_VALUE F_CPU / 256 / 256 * 1000 / BOOTLOADER_TIMEOUT_MS

/**
 * @brief Content of the MCU status register
 *
 * This variable is being used to save the content of `MCUSR` during start up.
 * `MCUSR` is reset immediately, so the source of the next reset can be
 * determined reliably. This variable is passed to the main application using
 * `r2` before jumping to the actual reset vector.
 *
 * @see main()
 * @see start_application()
 */
static uint8_t mcusr;

/**
 * @brief Buffer for memory write operations
 *
 * This buffer is used for write operations to memory. Before a write is
 * initiated, a page worth of content is received and put into the buffer. The
 * buffer is then passed on to {@link #write_memory()}.
 *
 * @see write_memory()
 */
static uint8_t page_buffer[256];


int main(int argc, char* argv[]) __attribute__ ((OS_main)) __attribute__ ((section (".init9")));

void __attribute__ ((noinline)) put_ch(char ch);
uint8_t __attribute__ ((noinline)) get_ch();
void __attribute__ ((noinline)) verify_command_terminator();

static inline void drop_ch(uint8_t count);
static inline void flash_start_leds(uint8_t count);
static inline void toggle_minute_leds();
static inline void write_memory(char memtype, uint8_t* buffer, uint16_t address, uint8_t length);
static inline void read_memory(char memtype, uint16_t address, uint8_t length);

void start_application() __attribute__ ((naked));


/**
 * @brief Main entry point
 *
 * This initializes the needed hardware modules, saves the `MCUSR`, disables
 * the watchdog to prevent infinite boot lopps, and enters the main loop, which
 * handles most of the STK500 protocol by evaluating the received characters.
 *
 * @see mcusr
 * @see flash_start_leds()
 * @see get_ch()
 */
int main(int argc, char* argv[])
{

    // Save MCU status register
    mcusr = MCUSR;
    MCUSR = 0;

    // Disable watchdog to prevent indefinite boot loops
    wdt_disable();

    // Set up Timer0
    // Prescaler: 256
    TCCR0B = _BV(CS02);

    #if (LED_START_FLASHES > 0)

        // Set up Timer1
        // Prescaler: 1024
        TCCR1B = _BV(CS12) | _BV(CS10);

    #endif

    #if USE_2X

        // Double speed mode
        UCSR0A = _BV(U2X0);

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

        #if (ENABLE_RGB_SUPPORT == 1)

            // Set PWM channels as output
            DDRD |= _BV(PD6) | _BV(PD5) | _BV(PD3);

        #else

            // Set PWMR pins as output
            DDRD |= _BV(PD6);

        #endif

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

    // Used in main loop for Cmnd_STK_LOAD_ADDRESS and Cmnd_STK_PROG_PAGE
    uint16_t address = 0;
    uint8_t length;

    // Forever loop: exits by timeout within get_ch()
    while(1) {

        // Get character from UART
        uint8_t ch = get_ch();

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

            // Read a page worth of contents
            bufPtr = page_buffer;
            do {

                *bufPtr++ = get_ch();

            } while (--length);

            // Read command terminator, start reply
            verify_command_terminator();

            // Write to memory
            write_memory(desttype, page_buffer, address, savelength);

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

            // Enable watchdog for faster reset
            wdt_enable(WDTO_15MS);
            verify_command_terminator();

        } else {

            // This covers the response to commands like STK_ENTER_PROGMODE
            verify_command_terminator();

        }

        put_ch(Resp_STK_OK);

    }

}

/**
 * @brief Outputs a single character via UART
 */
void put_ch(char ch)
{

    // Wait for previous transmission to be completed
    while (!(UCSR0A & _BV(UDRE0)));

    // Start new transmission
    UDR0 = ch;

}

/**
 * @brief Returns character received via UART
 *
 * This function busy waits until a new character has been received.
 * Concurrently a counter is incremented and compared against
 * BOOTLOADER_TIMEOUT_COMPARE_VALUE. Once the value is reached, the actual
 * application is started.
 *
 * @return Received character, void if start_application() is executed
 *
 * @see BOOTLOADER_TIMEOUT_COMPARE_VALUE
 * @see start_application()
 */
uint8_t get_ch()
{

    uint8_t ch;

    #ifdef LED_DATA_FLASH

        toggle_minute_leds();

    #endif

    uint8_t counter = 0;

    while(!(UCSR0A & _BV(RXC0))) {

        if (TIFR0 & _BV(TOV0)) {

            counter++;
            TIFR0 = _BV(TOV0);

        }

        if (counter > BOOTLOADER_TIMEOUT_COMPARE_VALUE) {

            start_application();

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

        toggle_minute_leds();

    #endif

    return ch;

}

/**
 * @brief Drops given amount of characters
 *
 * This retrieves count amount of characters from the UART hardware and simply
 * drops them. It is useful whenever bytes are expected as part of a command,
 * but don't actually need to be evaluated.
 *
 * @see get_ch()
 */
void drop_ch(uint8_t count)
{

    do {

        get_ch();

    } while (--count);

}

/**
 * @brief Verifies that a valid command terminator has been received
 *
 * This verifies that Sync_CRC_EOP has been received, which terminates each
 * command and in return responds with Resp_STK_INSYNC. In case no valid
 * command terminator was received the watchdog is enabled to reset the MCU.
 *
 * @see Sync_CRC_EOP
 * @see Resp_STK_INSYNC
 */
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

    /**
     * @brief Flashes the LEDs at startup
     *
     * This flashes the leds count times at the startup to indicate that the
     * bootloader has been accessed. It is polling the overflow flag of Timer1,
     * and toggling the LEDs by writing to the appropriate PIN register.
     *
     * @param count Number of flashes
     */
    void flash_start_leds(uint8_t count)
    {

        do {

            TCNT1 = -(F_CPU / (1024 * 16));
            TIFR1 = _BV(TOV1);

            while(!(TIFR1 & _BV(TOV1)));

            toggle_minute_leds();

        } while (--count);

    }

#endif

#if (LED_DATA_FLASH || LED_START_FLASHES)

    /**
     * @brief Toggles the minute LEDs
     *
     * This toggles the PWM channels by writing to the appropriate `PINx` register.
     * As only the minute LEDs are enabled, this effectively will toggle the
     * minute LEDs.
     */
    void toggle_minute_leds()
    {

        #if (ENABLE_RGB_SUPPORT == 1)

            PIND = _BV(PD6) | _BV(PD5) | _BV(PD3);

        #else

            PIND = _BV(PD6);

        #endif

    }

#endif

/**
 * @brief Resets the used hardware and jumps to the application
 *
 * This resets all of the used hardware by writing zero to the appropriate
 * registers. Before jumping to the actual application the content of mcusr is
 * put into `r2`, so it can be accessed by the application itself.
 *
 * @see mcusr
 */
void start_application()
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

    // Save the reset flags in r2
    __asm__ __volatile__ ("mov r2, %0\n" :: "r" (mcusr));

    // Jump to reset vector
    ((void(*)(void)) 0x0000)();

}

/**
 * @brief Writes data from given buffer to the specified memory location
 *
 * This writes length bytes of data from the given buffer to the specified
 * memory (EEPROM and/or flash) location, starting at address. It uses
 * eeprom_write_byte() for writing to the EEPROM and functions from
 * <avr/boot.h> for the flash memory.
 *
 * @param memtype 'E' for EEPROM, 'F' (or anything else) for Flash
 * @param address Address to start to write to
 * @param length Number of bytes to write
 *
 * @see put_ch()
 * @see eeprom_write_byte()
 * @see <avr/boot.h>
 */
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

/**
 * @brief Read chunk of memory and put it out via UART
 *
 * This reads length locations of the specified memory (EEPROM and/or Flash)
 * starting at address and puts them directly out via UART.
 *
 * @param memtype 'E' for EEPROM, 'F' (or anything else) for Flash
 * @param address Address to start to read from
 * @param length Number of address locations to read
 *
 * @see put_ch()
 * @see eeprom_read_byte()
 * @see pgm_read_byte()
 */
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
