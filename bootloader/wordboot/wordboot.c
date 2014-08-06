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

#define MAJOR_VERSION 6
#define MINOR_VERSION 0

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

/* set the UART baud rate defaults */
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

int main(void) __attribute__ ((OS_main)) __attribute__ ((section (".init9")));

void __attribute__((noinline)) putch(char);
uint8_t __attribute__((noinline)) getch(void);
void __attribute__((noinline)) verifySpace();

static inline void getNch(uint8_t);
static inline void flash_led(uint8_t);
static inline void writebuffer(int8_t memtype, uint8_t *mybuff,
			       uint16_t address, uint16_t len);
static inline void read_mem(uint8_t memtype,
			    uint16_t address, uint16_t len);

void appStart(uint8_t rstFlags) __attribute__ ((naked));

/* C zero initialises all global variables. However, that requires */
/* These definitions are NOT zero initialised, but that doesn't matter */
/* This allows us to drop the zero init code, saving us memory */
#define buff    ((uint8_t*)(RAMSTART))


/* main program starts here */
int main(void) {
  uint8_t ch;

  /*
   * Making these local and in registers prevents the need for initializing
   * them, and also saves space because code no longer stores to memory.
   * (initializing address keeps the compiler happy, but isn't really
   *  necessary, and uses 4 bytes of flash.)
   */
  register uint16_t address = 0;
  register uint16_t  length;

  // After the zero init loop, this is the first code to run.
  //
  // This code makes the following assumptions:
  //  No interrupts will execute
  //  SP points to RAMEND
  //  r1 contains zero
  //
  // If not, uncomment the following instructions:
  cli();
  asm volatile ("clr __zero_reg__");

  // Adaboot no-wait mod
  ch = MCUSR;
  MCUSR = 0;
  if (ch & _BV(WDRF)) appStart(ch);

#if LED_START_FLASHES > 0
  // Set up Timer 1 for timeout counter
  TCCR1B = _BV(CS12) | _BV(CS10); // div 1024
#endif

  #if USE_2X
  UCSR0A = _BV(U2X0); //Double speed mode USART0
  #endif
  UCSR0B = _BV(RXEN0) | _BV(TXEN0);
  UCSR0C = _BV(UCSZ00) | _BV(UCSZ01);
  UBRR0 = UBRR_VALUE;

  wdt_enable(WDTO_1S);

#if (LED_START_FLASHES > 0) || defined(LED_DATA_FLASH)

  // Disable PWM output
  TCCR0A = 0;
  TCCR2A = 0;

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

#if LED_START_FLASHES > 0
  /* Flash onboard LED to signal entering of bootloader */
  flash_led(LED_START_FLASHES * 2);
#endif

  /* Forever loop: exits by causing WDT reset */
  for (;;) {
    /* get character from UART */
    ch = getch();

    if(ch == STK_GET_PARAMETER) {
      unsigned char which = getch();
      verifySpace();
      if (which == 0x82) {
	/*
	 * Send optiboot version as "minor SW version"
	 */
	putch(MINOR_VERSION);
      } else if (which == 0x81) {
	  putch(MAJOR_VERSION);
      } else {
	/*
	 * GET PARAMETER returns a generic 0x03 reply for
         * other parameters - enough to keep Avrdude happy
	 */
	putch(0x03);
      }
    }
    else if(ch == STK_SET_DEVICE) {
      // SET DEVICE is ignored
      getNch(20);
    }
    else if(ch == STK_SET_DEVICE_EXT) {
      // SET DEVICE EXT is ignored
      getNch(5);
    }
    else if(ch == STK_LOAD_ADDRESS) {
      // LOAD ADDRESS
      uint16_t newAddress;
      newAddress = getch();
      newAddress = (newAddress & 0xff) | (getch() << 8);
      newAddress += newAddress; // Convert from word address to byte address
      address = newAddress;
      verifySpace();
    }
    else if(ch == STK_UNIVERSAL) {
      // UNIVERSAL command is ignored
      getNch(4);
      putch(0x00);
    }
    /* Write memory, length is big endian and is in bytes */
    else if(ch == STK_PROG_PAGE) {
      // PROGRAM PAGE - we support flash programming only, not EEPROM
      uint8_t desttype;
      uint8_t *bufPtr;
      uint16_t savelength;

      length = getch()<<8;			/* getlen() */
      length |= getch();
      savelength = length;
      desttype = getch();

      // read a page worth of contents
      bufPtr = buff;
      do *bufPtr++ = getch();
      while (--length);

      // Read command terminator, start reply
      verifySpace();

      writebuffer(desttype, buff, address, savelength);


    }
    /* Read memory block mode, length is big endian.  */
    else if(ch == STK_READ_PAGE) {
      uint8_t desttype;
      length = getch()<<8;			/* getlen() */
      length |= getch();
      desttype = getch();

      verifySpace();
	  
      read_mem(desttype, address, length);
    }

    /* Get device signature bytes  */
    else if(ch == STK_READ_SIGN) {
      // READ SIGN - return what Avrdude wants to hear
      verifySpace();
      putch(SIGNATURE_0);
      putch(SIGNATURE_1);
      putch(SIGNATURE_2);
    }
    else if (ch == STK_LEAVE_PROGMODE) { /* 'Q' */
      // Adaboot no-wait mod
      wdt_enable(WDTO_15MS);
      verifySpace();
    }
    else {
      // This covers the response to commands like STK_ENTER_PROGMODE
      verifySpace();
    }
    putch(STK_OK);
  }
}

void putch(char ch) {

  while (!(UCSR0A & _BV(UDRE0)));
  UDR0 = ch;

}

uint8_t getch(void) {
  uint8_t ch;

#ifdef LED_DATA_FLASH
  PIND = _BV(PD6) | _BV(PD5) | _BV(PD3);
#endif

  while(!(UCSR0A & _BV(RXC0)))
    ;
  if (!(UCSR0A & _BV(FE0))) {
      /*
       * A Framing Error indicates (probably) that something is talking
       * to us at the wrong bit rate.  Assume that this is because it
       * expects to be talking to the application, and DON'T reset the
       * watchdog.  This should cause the bootloader to abort and run
       * the application "soon", if it keeps happening.  (Note that we
       * don't care that an invalid char is returned...)
       */
    wdt_reset();
  }
  
  ch = UDR0;

#ifdef LED_DATA_FLASH
  PIND = _BV(PD6) | _BV(PD5) | _BV(PD3);
#endif

  return ch;
}

void getNch(uint8_t count) {
  do getch(); while (--count);
  verifySpace();
}

void verifySpace() {
  if (getch() != CRC_EOP) {
    wdt_enable(WDTO_15MS);    // shorten WD timeout
    while (1)			      // and busy-loop so that WD causes
      ;				      //  a reset and app start.
  }
  putch(STK_INSYNC);
}

#if LED_START_FLASHES > 0
void flash_led(uint8_t count) {
  do {
    TCNT1 = -(F_CPU/(1024*16));
    TIFR1 = _BV(TOV1);
    while(!(TIFR1 & _BV(TOV1)));

    PIND = _BV(PD6) | _BV(PD5) | _BV(PD3);

    wdt_reset();
  } while (--count);
}
#endif

void appStart(uint8_t rstFlags) {
  // save the reset flags in the designated register
  //  This can be saved in a main program by putting code in .init0 (which
  //  executes before normal c init code) to save R2 to a global variable.
  __asm__ __volatile__ ("mov r2, %0\n" :: "r" (rstFlags));

  wdt_disable();
  __asm__ __volatile__ (
    // Jump to RST vector
    "jmp 0x0000\n"
  );
}

/*
 * void writebuffer(memtype, buffer, address, length)
 */
static inline void writebuffer(int8_t memtype, uint8_t *mybuff,
			       uint16_t address, uint16_t len)
{
    switch (memtype) {
    case 'E': // EEPROM

        while(len--) {
	    eeprom_write_byte((uint8_t *)(address++), *mybuff++);
        }

	break;
    default:  // FLASH
	/*
	 * Default to writing to Flash program memory.  By making this
	 * the default rather than checking for the correct code, we save
	 * space on chips that don't support any other memory types.
	 */
	{
	    // Copy buffer into programming buffer
	    uint8_t *bufPtr = mybuff;
	    uint16_t addrPtr = (uint16_t)(void*)address;

	    /*
	     * Start the page erase and wait for it to finish.  There
	     * used to be code to do this while receiving the data over
	     * the serial link, but the performance improvement was slight,
	     * and we needed the space back.
	     */
	    boot_page_erase((uint16_t)(void*)address);
	    boot_spm_busy_wait();

	    /*
	     * Copy data from the buffer into the flash write buffer.
	     */
	    do {
		uint16_t a;
		a = *bufPtr++;
		a |= (*bufPtr++) << 8;
		boot_page_fill((uint16_t)(void*)addrPtr,a);
		addrPtr += 2;
	    } while (len -= 2);

	    /*
	     * Actually Write the buffer to flash (and wait for it to finish.)
	     */
	    boot_page_write((uint16_t)(void*)address);
	    boot_spm_busy_wait();

	    // Reenable read access to flash
	    boot_rww_enable();

	} // default block
	break;
    } // switch
}

static inline void read_mem(uint8_t memtype, uint16_t address, uint16_t length)
{
    uint8_t ch;

    switch (memtype) {

    case 'E': // EEPROM
	do {
	    putch(eeprom_read_byte((uint8_t *)(address++)));
	} while (--length);
	break;

    default:
	do {
	    // read a Flash byte and increment the address
	    __asm__ ("lpm %0,Z+\n" : "=r" (ch), "=z" (address): "1" (address));
	    putch(ch);
	} while (--length);
	break;
    } // switch
}
