/*
 * Copyright (C) 2013 Karol Babioch <karol@babioch.de>
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
 * @file fifo.c
 * @brief Implementation of functions declared in the header fifo.h
 *
 * This implements all the functionality declared in the header file fifo.h.
 *
 * It is based upon [1] with some minor adaptations.
 *
 * [1]: http://www.rn-wissen.de/index.php/FIFO_mit_avr-gcc
 *
 * @see fifo.h
 */

#include "fifo.h"

/**
 * @brief Initializes the FIFO with the given parameters
 *
 * This initializes the FIFO with the given parameters. It sets up the
 * organizational data for the FIFO (fifo_t). It has to be called *once* for
 * every FIFO before the FIFO itself can be used.
 *
 * @param fifo Pointer to the organizational data for the FIFO of type fifo_t
 * @param buffer Pointer to buffer in memory for holding the actual data
 * @param size The number of bytes the FIFO should manage
 *
 * @see fifo_t
 */
void fifo_init(fifo_t* fifo, uint8_t* buffer, uint8_t size)
{

    fifo->count = 0;
    fifo->pread = buffer;
    fifo->pwrite = buffer;
    fifo->read2end = size;
    fifo->write2end = size;
    fifo->size = size;

}

/**
 * @brief Puts a single byte of data into the FIFO
 *
 * This puts a single byte of data (data) into the FIFO defined by fifo.
 *
 * First of all it makes sure that there is actually place left within the
 * buffer. It makes sure that all of the organizational data is updated
 * correctly and disables the global interrupt flag for a brief period of
 * time to prevent any interactions with interrupts.
 *
 * It returns true when the element could successfully be put into the FIFO,
 * otherwise it will return false, which is an indicator for the FIFO being
 * full.
 *
 * @param fifo Pointer to the organizational data for the FIFO of type fifo_t
 * @param data The actual data to be put into the FIFO
 *
 * @return True if data has been put into FIFO, false otherwise
 *
 * @see fifo_t)
 */
bool fifo_put(fifo_t* fifo, uint8_t data)
{

    if (fifo->count >= fifo->size) {

        return false;

    }

    uint8_t* pwrite = fifo->pwrite;

    *(pwrite++) = data;

    uint8_t write2end = fifo->write2end;

    if (--write2end == 0) {

        write2end = fifo->size;
        pwrite -= write2end;

    }

    fifo->write2end = write2end;
    fifo->pwrite = pwrite;

    uint8_t sreg = SREG;
    cli();
    fifo->count++;
    SREG = sreg;

    return true;

}

/**
 * @brief Retrieves next byte from the FIFO
 *
 * This retrieves the next byte from the FIFO and returns it. It simply returns
 * the byte pointed to by fifo_t::pread and makes sure that all of the
 * organizational data is updated correctly. It also disables the global
 * interrupt flag for a brief period of time to prevent any interactions with
 * interrupts.
 * *
 * @param fifo Pointer to the organizational data for the FIFO of type fifo_t
 *
 * @return Actual data retrieved from the FIFO.
 *
 * @see fifo_t
 */
static inline uint8_t _inline_fifo_get(fifo_t* fifo)
{

    uint8_t *pread = fifo->pread;
    uint8_t data = *(pread++);
    uint8_t read2end = fifo->read2end;

    if (--read2end == 0) {

        read2end = fifo->size;
        pread -= read2end;

    }

    fifo->pread = pread;
    fifo->read2end = read2end;

    uint8_t sreg = SREG;
    cli();
    fifo->count--;
    SREG = sreg;

    return data;

}

/**
 * @brief Retrieves next byte from the FIFO - blocks if FIFO is empty
 *
 * This retrieves the next byte from the FIFO and returns it. When the FIFO is
 * currently empty, it busy waits until there is actually something to return.
 *
 * Internally it makes use of _inline_fifo_get().
 *
 * @param fifo Pointer to the organizational data for the FIFO of type fifo_t
 *
 * @return Actual data retrieved from the FIFO.
 *
 * @warning By using this function carelessly you can effectively stop program
 * execution. Consider using fifo_get_nowait().
 *
 * @see fifo_get_nowait()
 * @see fifo_t
 */
uint8_t fifo_get_wait(fifo_t* fifo)
{

    while (!fifo->count);

    return _inline_fifo_get(fifo);

}

/**
 * @brief Retrieves next byte from the FIFO - if available
 *
 * This retrieves the next byte from the FIFO and returns it - if there is
 * actually something left in the FIFO. The status parameter is an indicator
 * for whether or not something senseful has actually been returned.
 *
 * @param fifo Pointer to the organizational data for the FIFO of type fifo_t
 * @param status Pointer to a boolean variable for holding the success value
 *
 * @return Actual data retrieved from the FIFO, only senseful if status is true
 *
 * @warning Make sure to check the value of the status variable. The returned
 * value makes only sense if the status variable is true.
 *
 * @see fifo_get_wait()
 * @see fifo_t
 */
uint8_t fifo_get_nowait(fifo_t* fifo, bool* status)
{

    if (!fifo->count) {

        *status = false;

        return 0;

    }

    *status = true;

    return _inline_fifo_get(fifo);

}
