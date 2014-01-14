/*
 * Copyright (C) 2013, 2014 Karol Babioch <karol@babioch.de>
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
 * @file fifo.h
 * @brief Header declaring functionality needed for a FIFO
 *
 * This header declares all functions and type definitions needed for a FIFO
 * implementation. This FIFO operates only with single bytes (8 bit).
 *
 * For more information about how a FIFO is supposed to work, refer to [1].
 *
 * [1]: https://en.wikipedia.org/wiki/FIFO
 *
 * @see fifo.c
 */

#ifndef WC_FIFO_H
#define WC_FIFO_H

#include <avr/io.h>
#include <avr/interrupt.h>

#include <stdbool.h>

/**
 * @brief Typedef for variables holding the organizational data of a FIFO
 *
 * To organize the FIFO additional data is needed to keep track of the current
 * state along with the buffer holding the actual data.
 */
typedef struct
{

    /**
     * @brief Number of elements currently stored in the FIFO
     */
    uint8_t volatile count;

    /**
     * @brief Size of the buffer holding the actual data
     */
    uint8_t size;

    /**
     * @brief Pointer to location where to read from
     */
    uint8_t *pread;

    /**
     * @brief Pointer to location where to write to
     */
    uint8_t *pwrite;

    /**
     * @brief Number of elements for read pointer to overflow
     */
    uint8_t read2end;

    /**
     * @brief Number of elements for write pointer to overflow
     */
    uint8_t write2end;

} fifo_t;

extern void fifo_init(fifo_t* fifo, uint8_t* buffer, uint8_t size);

extern bool fifo_put(fifo_t* fifo, uint8_t data);

extern uint8_t fifo_get_wait(fifo_t* fifo);

extern bool fifo_get_nowait(fifo_t* fifo, uint8_t* data);

#endif /* WC_FIFO_H */
