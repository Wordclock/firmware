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
 * @brief Macros making it easier to access pins and their appropriate ports
 *
 * This file contains macros, which make it easier and more intuitive to deal
 * with single pins and their appropriate ports. In its current form it is
 * very specific to AVR microcontrollers, however.
 *
 * Basically instead of defining up multiple, quite redundant, macros for
 * `PINx`, `PORTx`, `DDRx` and pin number itself, a single definition is
 * sufficient.
 *
 * Consider the following example, which is used in this form quite often:
 *
 * \code
 *  #define WHATEVER_PORT    PORTD
 *  #define WHATEVER_DDR     DDRD
 *  #define WHATEVER_PIN     PIND
 *  #define WHATEVER_BIT     4
 * \endcode
 *
 * This is kind of ugly for various reasons one of which is the implied
 * redundancy. Using the macros defined in this file a single macro is enough:
 *
 * \code
 *  #define WHATEVER PORTD, 4
 * \endcode
 *
 * All of the macros defined here expect two arguments, separated by a `,`,
 * just like shown above. The first argument is always expected to describe the
 * name of the port, whereas the second one describes the pin number itself.
 *
 * Internally these macros exploit the fact that addresses of the involved
 * registers can be calculated quite easily. Normally the `PINx` register comes
 * first and is followed by the `DDRx` register, which in return is followed by
 * the `PORTx` register itself. For instance in case of the port C of an
 * ATmega168 it looks like this (see [1], p. 343, chapter 31).
 *
 * \code
 *   [...]
 *  0x08 (0x28) PORTC
 *  0x07 (0x27) DDRC
 *  0x06 (0x26) PINC
 *   [...]
 * \endcode
 *
 * By knowing the address location of `PORTC` both of the addresses for `DDRC`
 * and `PINC` can be calculated.
 *
 * However there is an exception to this simple scheme in case of the ATmega64
 * and ATmega128. These provide an additional port F, which differs a little
 * bit from the other available ports, as the `PINF` register is located at
 * 0x00 instead of 0x60, which would be expected. For details refer to [2], p.
 * 369ff. However, this is also covered by the macros provided by this file.
 *
 * These macros are based on a concept known as [variadic macro's][3].
 *
 * This implementation is a combination of ideas taken from [4] and [5].
 *
 * [1]: http://www.atmel.com/images/doc2545.pdf
 * [2]: http://www.atmel.com/Images/doc2490.pdf
 * [3]: https://en.wikipedia.org/wiki/Variadic_macro
 * [4]: http://www.avrfreaks.net/index.php?name=PNphpBB2&file=viewtopic&t=91304
 * [5]: http://homepage.hispeed.ch/peterfleury/group__pfleury__lcd.html
 *
 * @warning This hasn't been thoroughly tested with every AVR yet.
 */

#ifndef _WC_PORTS_H_
#define _WC_PORTS_H_

/**
 * @brief Returns the PORT register of an appropriate definition
 *
 * @return PORT register for the given definition
 *
 * @see PORT_()
 */
#define PORT(...) PORT_(__VA_ARGS__)

/**
 * @brief Helper macro needed to implement PORT()
 *
 * @param a Name of the port
 * @param b Number of the pin
 *
 * @see PORT()
 */
#define PORT_(a, b) (a)

/**
 * @brief Returns the DDR register of an appropriate definition
 *
 * @return DDR register for the given definition
 *
 * @see DDR_()
 */
#define DDR(...) DDR_(__VA_ARGS__)

/**
 * @brief Helper macro needed to implement DDR()
 *
 * @param a Name of the port
 * @param b Number of the pin
 *
 * @see DDR()
 */
#define DDR_(a, b) (*(&a - 1))

/**
 * @brief Returns the PIN register of an appropriate definition
 *
 * @note As the ATmega64 and ATmega128 differ from other AVR microcontrollers
 * when it comes down to `PORTF`, there are actually two implementations of
 * `PIN_()`.
 *
 * @return PIN register for the given definition
 *
 * @see PIN_()
 */
#define PIN(...) PIN_(__VA_ARGS__)

#if defined(__AVR_ATmega64__) || defined(__AVR_ATmega128__)

    /**
     * @brief Helper macro needed to implement PIN()
     *
     * With these devices the location of the `PINF` register is located at an
     * address contrary to the expected one, making a distinction necessary.
     *
     * @param a Name of the port
     * @param b Number of the pin
     *
     * @see PIN()
     */
    #define PIN_(a, b) ((&PORTF == &(x)) ? _SFR_IO8(0x00) : (*(&a - 2)))

#else

    /**
     * @brief Helper macro needed to implement PIN()
     *
     * @param a Name of the port
     * @param b Number of the pin
     *
     * @see PIN()
     */
    #define PIN_(a, b) (*(&a - 2))

#endif

/**
 * @brief Returns the pin number of an appropriate definition
 *
 * @return Pin number of the given definition
 *
 * @see BIT_()
 */
#define BIT(...) BIT_(__VA_ARGS__)

/**
 * @brief Helper macro needed to implement BIT()
 *
 * @param a Name of the port
 * @param b Number of the pin
 *
 * @see BIT()
 */
#define BIT_(a, b) b

#endif /* _WC_PORTS_H_ */
