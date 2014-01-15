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
 * @brief Implementation of the header declared in shift.h
 *
 * This file contains the actual implementation of the functionality declared
 * in `shift.h`. For details about the protocol and/or the hardware itself,
 * refer to [1], p. 163, table 19-1.
 *
 * [1]: http://www.atmel.com/images/doc2545.pdf
 *
 * @see shift.c
 */

#include <avr/io.h>

#include "shift.h"

/**
 * @brief The port used for SPI
 */
#define SHIFT_SR_SPI_PORT PORTB

/**
 * @brief The DDR of the port the SPI is attached to
 */
#define SHIFT_SR_SPI_DDR DDRB

/**
 * @brief The pin the MOSI line of the SPI is attached to
 */
#define SHIFT_SR_SPI_MOSI 3

/**
 * @brief The pin the MISO line of the SPI is attached to
 *
 * Although this pin is actually not needed to output data, it has to be set
 * to input in order for the SPI to act as master.
 */
#define SHIFT_SR_SPI_MISO 4

/**
 * @brief The pin the RCLK lines of the shift registers are attached to
 *
 * This is used to transfer the the contents of the shift registers to the
 * storage registers to output them in parallel.
 */
#define SHIFT_SR_SPI_RCLK 2

/**
 * @brief The pin the SCK lines of the shift registers are attached to
 *
 * This is the clock line used for the shift registers.
 */
#define SHIFT_SR_SPI_SCK 5

/**
 * @brief Initializes this module
 *
 * This function has to be called **once** before data can actually be output.
 * It sets up the hardware by writing the proper values into all relevant SPI
 * control registers. Afterwards data can be output using `shift24_output()`.
 *
 * @see shift24_output()
 */
void shift24_init() {

    /*
     * Set the data direction registers to enable SPI master mode
     */
    SHIFT_SR_SPI_DDR |= _BV(SHIFT_SR_SPI_MOSI) | _BV(SHIFT_SR_SPI_RCLK)
            | _BV(SHIFT_SR_SPI_SCK);
    SHIFT_SR_SPI_DDR  &= ~(_BV(SHIFT_SR_SPI_MISO));

    /*
     * Set RCLK line to high. When data is output it will be set low and high
     * again, as the shift registers only latch the data at a low-to-high
     * transition.
     *
     * Furthermore enable the pull-up at the MOSI pin, just in case.
     */
    SHIFT_SR_SPI_PORT |= _BV(SHIFT_SR_SPI_RCLK) | _BV(SHIFT_SR_SPI_MISO);

    /*
     * SPE: Enable SPI hardware
     * MSTR: Select SPI master mode
     * CPOL: SCK is high when idle
     */
    SPCR = _BV(SPE) | _BV(MSTR) | _BV(CPOL);

    /*
     * SPI2X: Double SPI speed
     */
    SPSR |= _BV(SPI2X);

    /*
     * Send out 0 to get well defined states
     */
    shift24_output(0);

}

/**
 * @brief Outputs the given data over the Serial Peripheral Interface (SPI)
 *
 * This function outputs the given data by splitting it into blocks of eight
 * bits and writing it into the appropriate register of the SPI module. After
 * all of the bits have been output the RCLK line is toggled, so that the
 * shift registers will apply the new values.
 *
 * @note Although the argument accepts 32 bits (uint32_t), only the 24 least
 * significant bits will actually be output.
 *
 * @param data Data to be output
 *
 * @see shift24_init()
 */
void shift24_output(uint32_t data) {

    uint8_t u0 = (uint8_t)(data);
    uint8_t u1 = (uint8_t)(data >> 8);
    uint8_t u2 = (uint8_t)(data >> 16);

    /*
     * Writing to SPDR starts the transmission. Afterwards wait until the SPIF
     * bit in SPSR is cleared, which indicates that the transmission was
     * completed, so that the next transmission can be started.
     */

    SPDR = u2;
    while (!(SPSR & (_BV(SPIF))));

    SPDR = u1;
    while (!(SPSR & (_BV(SPIF))));

    SPDR = u0;
    while (!(SPSR & (_BV(SPIF))));

    /*
     * Transfer contents of shift register stages to storage registers and
     * output them in parallel. See [1], p. 5, Table 3, for details.
     *
     * [1]: http://www.nxp.com/documents/data_sheet/74HC_HCT595.pdf
     */
    SHIFT_SR_SPI_PORT &= ~(_BV(SHIFT_SR_SPI_RCLK));
    SHIFT_SR_SPI_PORT |=  (_BV(SHIFT_SR_SPI_RCLK));

}
