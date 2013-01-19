/*
 * Copyright (C) 2012, 2013 Karol Babioch <karol@babioch.de>
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
 * @file ports.h
 * @brief Helper macros to make dealing with ports easier
 *
 * These file contains various macros, which will make dealing with ports
 * easier, which basically means that instead of defining up to four different
 * macros for the pin, port and ddr registers of a pin, a single definition
 * is enough.
 *
 * Consider this classic example:
 *
 * \code
 *  #define WHATEVER_PORT    PORTD
 *  #define WHATEVER_DDR     DDRD
 *  #define WHATEVER_PIN     PIND
 *  #define WHATEVER_BIT     4
 * \endcode
 *
 * This is kind of ugly for various reasons one of which is the implied
 * redundancy. With the macro defined in this file a simple definitions enough:
 *
 * \code
 *  #define WHATEVER PORTD, 4
 * \endcode
 *
 * This works because the addresses of the involved registers can be calculated
 * as there is a system behind them. Normally the PINx register comes first and
 * is followed by the DDRx register, which in return is followed by the PORTx
 * register itself. For instance for the port C of an ATmega168 it looks like
 * this (see [1], p. 343, chapter 31).
 *
 * \code
 *   [...]
 *  0x08 (0x28) PORTC
 *  0x07 (0x27) DDRC
 *  0x06 (0x26) PINC
 *   [...]
 * \endcode
 *
 * So by knowing the address location of PORTC both of the addresses for DDRC
 * as well as PINC can easily be calculated by subtracting one and/or two.
 *
 * However there is an exception to this in case of ATmega64's and ATmega128's,
 * which provide an additional port F. The PINF for whatever reason register is
 * located at 0x00 instead of 0x60, which one would expect following the scheme
 * described above. For details see [2], p. 369ff. However the appropriate
 * PIN() macro accommodates for this with a relative simple distinction of the
 * used microcontroller.
 *
 * The macros are based on a concept known as [variadic macro's][3].
 *
 * This implementation is a combination of ideas taken from [4] and [5].
 *
 * [1]: http://www.atmel.com/images/doc2545.pdf
 * [2]: http://www.atmel.com/Images/doc2490.pdf
 * [3]: https://en.wikipedia.org/wiki/Variadic_macro
 * [4]: http://www.avrfreaks.net/index.php?name=PNphpBB2&file=viewtopic&t=91304
 * [5]: http://homepage.hispeed.ch/peterfleury/group__pfleury__lcd.html
 *
 * \warning This hasn't been thoroughly tested with every AVR yet.
 */

#ifndef _WC_PORTS_H_
#define _WC_PORTS_H_

/**
 * @brief Returns the port given its name followed by the pin number
 *
 * This macro expects two arguments, the first of which is the name of the
 * port. The second one is the number of the pin. A valid input would be:
 * `PORTD, 0`. This input will then be transformed into "PORTD".
 *
 * This might seem trivial, however it makes things simpler and provides a
 * consequent way of addressing pin numbers analogous to DDR() and PIN().
 * Furthermore it enables the use of a single macro definition.
 *
 * @return Port with the given name
 * @see PORT_()
 */
#define PORT(...) PORT_(__VA_ARGS__)

/**
 * @brief Helper macro needed to implement PORT()
 *
 * @param a Name of the port
 * @param b Number of the pin
 * @see PORT()
 */
#define PORT_(a, b) (a)

/**
 * @brief References the DDR of a port given its name followed by the pin
 *        number
 *
 * This macro expects two arguments, the first of which is the name of the
 * port. The second one is the number of the pin. A valid input would be:
 * `PORTD, 0`. This input will then be transformed into "DDRD".
 *
 * This works because the DDRs are actually always next to the port register
 * itself, only separated by one.
 *
 * @return DDR referenced by the port with the provided name
 * @see DDR_()
 */
#define DDR(...) DDR_(__VA_ARGS__)

/**
 * @brief Helper macro needed to implement DDR()
 *
 * @param a Name of the port
 * @param b Number of the pin
 * @see DDR()
 */
#define DDR_(a, b)     (*(&a - 1))

/**
 * @brief References the PIN register of a port given its name followed by the
 *     pin number
 *
 * This macro expects two arguments, the first of which is the name of the
 * port. The second one is the number of the pin. A valid input would be:
 * `PORTD, 0`. This input will then be transformed into "PIND".
 *
 * This works because the PIN registers are actually always next to the port
 * register itself, only separated by two. It is a little more complex in
 * the case of ATmega64's and ATmega128's as their PINF is actually located at
 * somewhere else (0x00) instead of the expected (0x60) location. This macro
 * accommodates for this.
 *
 * @return PIN register referenced by the port with the provided name
 * @see PIN_()
 */
#define PIN(...) PIN_(__VA_ARGS__)

#if defined(__AVR_ATmega64__) || defined(__AVR_ATmega128__)

    /**
    * @brief Helper macro needed to implement PIN() for ATmega64 and ATmega128
    *     devices
    *
    * With these devices the location of the PINF register is located at an
    * address contrary to the one you might expect, namely 0x00.
    *
    * @param a Name of the port
    * @param b Number of the pin
    * @see PIN()
    */
    #define PIN_(a, b) ((&PORTF == &(x)) ? _SFR_IO8(0x00) : (*(&a - 2)))

#else

    /**
    * @brief Helper macro needed to implement PIN()
    *
    * @param a Name of the port
    * @param b Number of the pin
    * @see PIN()
    */
    #define PIN_(a, b) (*(&a - 2))

#endif

/**
 * @brief Returns the bit number given a port name followed by the pin number
 *
 * This macro expects two arguments, the first of which is the name of the
 * port. The second one is the number of the pin. A valid input would be:
 * `PORTD, 0`. This input will then be transformed into "0".
 *
 * This might seem trivial, however it makes things simpler and provides a
 * consequent way of addressing pin numbers analogous to DDR() and PIN().
 * Furthermore it enables the use of a single macro definition.
 *
 * @return BIT number provided with the port name and pin number
 * @see BIT_()
 */
#define BIT(...) BIT_(__VA_ARGS__)

/**
 * @brief Helper macro needed to implement BIT()
 *
 * @param a Name of the port
 * @param b Number of the pin
 * @see BIT()
 */
#define BIT_(a, b) b

#endif /* _WC_PORTS_H_ */
