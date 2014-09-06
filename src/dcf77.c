/*
 * Copyright (C) 2012, 2013, 2014 Karol Babioch <karol@babioch.de>
 * Copyright (c) 2010 Torsten Giese
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
 * @file dcf77.c
 * @brief Implementation of the header declared in dcf77.h
 *
 * This file contains the implementation of the header declared in dcf77.h. In
 * order to understand the source code completely, basic knowledge about the
 * DCF77 signal is needed. Take a look at the Wikipedia [article][1] for an
 * overview of the concept. For a detailed description of the the time signal,
 * refer to [2].
 *
 * [1]: https://en.wikipedia.org/wiki/DCF77
 * [2]: https://en.wikipedia.org/wiki/DCF77#Time_code_interpretation
 *
 * @see dcf77.h
 */

#include <avr/io.h>
#include <stdbool.h>
#include <stdio.h>

#include "dcf77.h"
#include "dcf77_decoder.h"
#include "log.h"
#include "ports.h"

#define LOG_LEVEL_DCF77_DEFAULT LOG_LEVEL_ALL

typedef enum {

    DCF77_FSM_STATE_WAIT_FOR_SYNC_PULSE = 0,
    DCF77_FSM_STATE_CURRENTLY_RECEIVING,
    DCF77_FSM_STATE_RECEPTION_DISABLED,


} dcf77_fsm_state_t;

static dcf77_fsm_state_t dcf77_fsm_state;

static void dcf77_reset()
{

    dcf77_decoder_reset();
    dcf77_fsm_state = DCF77_FSM_STATE_WAIT_FOR_SYNC_PULSE;

    log_output_P(LOG_MODULE_DCF77, LOG_LEVEL_INFO, "Reset, waiting for sync pulse");

}

/**
 * @brief Initializes this module
 *
 * This function initializes internal variables needed for the module to work
 * correctly. Furthermore the I/O pins are setup appropriately.
 *
 * @see DCF77_INPUT
 * @see DCF77_OUTPUT
 */
void dcf77_init()
{

    // Make pin input and disable internal pull-up
    DDR(DCF77_INPUT) &= ~(_BV(BIT(DCF77_INPUT)));
    PORT(DCF77_INPUT) &= ~(_BV(BIT(DCF77_INPUT)));

    // Make pin output and keep output low
    DDR(DCF77_OUTPUT) |= _BV(BIT(DCF77_OUTPUT));
    PORT(DCF77_OUTPUT) &= ~(_BV(BIT(DCF77_OUTPUT)));

    dcf77_fsm_state = DCF77_FSM_STATE_WAIT_FOR_SYNC_PULSE;

    log_output_P(LOG_MODULE_DCF77, LOG_LEVEL_INFO, "Initialized, waiting for sync pulse");

}

void dcf77_enable()
{

    dcf77_fsm_state = DCF77_FSM_STATE_WAIT_FOR_SYNC_PULSE;

    log_output_P(LOG_MODULE_DCF77, LOG_LEVEL_INFO, "Reception enabled, waiting for sync pulse");

}

void dcf77_disable()
{

    dcf77_fsm_state = DCF77_FSM_STATE_RECEPTION_DISABLED;

    log_output_P(LOG_MODULE_DCF77, LOG_LEVEL_INFO, "Reception disabled");

}


/**
 * @brief Filters the DCF77 input signal for spikes and returns the result
 *
 * This will make sure that any spikes shorter than DCF_FLT_LIMIT_HI will be
 * filtered and hence ignored. Only signal transitions holding longer than
 * DCF_FLT_LIMIT_HI are considered valid and therefore returned.
 *
 * The current signal is compared against the last filtered signal and whenever
 * a transition is detected, a counter will be incremented. Once the counter
 * reaches the limit defined by DCF_FLT_LIMIT_HI the filtered signal is
 * swapped and returned.
 *
 * @note Keep in mind that by using this function to detect edges in the input
 * signal, a delay of DCF_FLT_LIMIT_HI is introduced, as the transition is only
 * reported once a spike has been ruled out.
 *
 * @note This function is expected to be invoked regularly with the timebase
 * defined in DCF_TIME_BASE, otherwise assumptions about the timing are
 * incorrect leading to wrong filter results.
 *
 * @see DCF77_INPUT
 * @see DCF_TIME_BASE
 */
static bool dcf77_filter_input()
{

    static uint8_t filter_counter = 0;

    static bool filtered_signal = false;
    bool current_signal = PIN(DCF77_INPUT) & _BV(BIT(DCF77_INPUT));

    // Check for transition
    if (current_signal == filtered_signal) {

        if (filter_counter != 0) {

            log_output_P(LOG_MODULE_DCF77, LOG_LEVEL_DEBUG, "Spike filtered: %U0 ms", filter_counter);

            // No transition detected, reset counter
            filter_counter = 0;

        }

    } else {

        // Transition detected, increment counter
        filter_counter++;

        // Check if signal is stable for a long enough already
        if (filter_counter > DCF_FLT_LIMIT_HI / DCF_TIME_BASE) {

            filter_counter = 0;
            filtered_signal = current_signal;

        }

    }

    return filtered_signal;

}

typedef enum {

    DCF77_PULSE_LOW,
    DCF77_PULSE_HIGH,
    DCF77_PULSE_PAUSE,
    DCF77_PULSE_SYNC,
    DCF77_PULSE_INVALID,

} dcf77_pulse_t;

static dcf77_pulse_t dcf77_get_high_pulse_type(uint8_t pulse_length)
{

    if (DCF_LIM_LO_BIT_LOWER <= pulse_length && pulse_length <= DCF_LIM_LO_BIT_UPPER) {

        return DCF77_PULSE_LOW;

    }

    if (DCF_LIM_HI_BIT_LOWER <= pulse_length && pulse_length <= DCF_LIM_HI_BIT_UPPER) {

        return DCF77_PULSE_HIGH;

    }

    return DCF77_PULSE_INVALID;

}

static dcf77_pulse_t dcf77_get_low_pulse_type(uint8_t pause_length)
{

    if (DCF_LIM_PAUSE_LOWER <= pause_length && pause_length <= DCF_LIM_PAUSE_UPPER) {

       return DCF77_PULSE_PAUSE;

    }

    if (DCF_LIM_SYNC_LOWER <= pause_length && pause_length <= DCF_LIM_SYNC_UPPER) {

       return DCF77_PULSE_SYNC;

    }

    return DCF77_PULSE_INVALID;

}

//void dcf77_sync()
//{
//    // Take over time as valid (No return type?)
//}

// Log

// Return true if synced in last 24 (?!?) hours
//bool dcf77_is_synced()
//{
//
//}

void dcf77_ISR()
{

    if (dcf77_fsm_state == DCF77_FSM_STATE_RECEPTION_DISABLED) {

        return;

    }

    static bool last_signal = false;
    bool current_signal = dcf77_filter_input();

    static uint8_t pulse_counter = 0;

    if (current_signal != last_signal) {

        if (dcf77_fsm_state == DCF77_FSM_STATE_WAIT_FOR_SYNC_PULSE) {

            if (current_signal) {

                if (dcf77_get_low_pulse_type(pulse_counter) == DCF77_PULSE_SYNC) {

                    dcf77_fsm_state = DCF77_FSM_STATE_CURRENTLY_RECEIVING;

                    log_output_P(LOG_MODULE_DCF77, LOG_LEVEL_INFO, "Sync pulse received");

                }

            }

        } else if (dcf77_fsm_state == DCF77_FSM_STATE_CURRENTLY_RECEIVING) {

            if (current_signal) {

                // low -> high transition

                dcf77_pulse_t pulse_type = dcf77_get_low_pulse_type(pulse_counter);

                if (pulse_type == DCF77_PULSE_PAUSE) {
                    // Normal pause
                } else if (pulse_type == DCF77_PULSE_SYNC) {

                    // Sync
                    dcf77_decoder_decode();
                    log_output_P(LOG_MODULE_DCF77, LOG_LEVEL_INFO, "Sync pulse received");

                } else {

                    log_output_P(LOG_MODULE_DCF77, LOG_LEVEL_INFO, "Invalid low pulse: %U0 ms", pulse_counter);
                    dcf77_reset();

                }

            } else {

                // high -> low transition

                dcf77_pulse_t pulse_type = dcf77_get_high_pulse_type(pulse_counter);

                if (pulse_type == DCF77_PULSE_LOW) {
                    // 0
                    dcf77_decoder_append(false);
                } else if (pulse_type == DCF77_PULSE_HIGH) {
                    // 1
                    dcf77_decoder_append(true);
                } else {

                    log_output_P(LOG_MODULE_DCF77, LOG_LEVEL_INFO, "Invalid high pulse: %U0 ms", pulse_counter);
                    dcf77_reset();

                }

            }

        }

        pulse_counter = 0;
        last_signal = current_signal;

    } else {

        if (pulse_counter < DCF_LIM_TIMER_COUNTER) {

            pulse_counter++;

        } else {

            if (dcf77_fsm_state != DCF77_FSM_STATE_WAIT_FOR_SYNC_PULSE) {

                log_output_P(LOG_MODULE_DCF77, LOG_LEVEL_INFO, "No transition for too long");
                dcf77_reset();

            }

        }

    }

}

bool dcf77_getDateTime(datetime_t* DateTime_p)
{

    return false;

}
