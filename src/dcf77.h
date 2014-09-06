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
 * @file dcf77.h
 * @brief Header file of module decoding the DCF77 time signal
 *
 * DCF77 is used to receive the current date and time automatically. This
 * module is responsible for decoding the signal already demodulated by
 * an appropriate DCF77 receiver. At least following receivers should be
 * supported:
 *
 * - [DCF-Empfangsmodul DCF1][1] from pollin.de
 * - [C-Control DCF-Empfängerplatine][2] from conrad.de
 * - [DCF 77 Empfängermodul][3] from reichelt.de
 *
 * [1]: http://www.pollin.de/shop/dt/NTQ5OTgxOTk-/Bausaetze_Module/Module/DCF_Empfangsmodul_DCF1.html
 * [2]: http://www.conrad.de/ce/de/product/641138/C-Control-DCF-Empfaengerplatine
 * [3]: https://secure.reichelt.de/DCF77-MODUL/3/index.html?ACTION=3&GROUPID=3636&ARTICLE=57772
 *
 * @see dcf77.c
 */

#ifndef _WC_DCF77_H_
#define _WC_DCF77_H_

#include <stdbool.h>

#include "datetime.h"
#include "log.h"

#define DCF_LOG_LEVEL               LOG_LEVEL_ALL

#define DCF_TIME_BASE               (10)

// Rename
#define DCF_FLT_LIMIT_HI            (60)

#define DCF_LIM_LO_BIT_LOWER        (60 / DCF_TIME_BASE)
#define DCF_LIM_LO_BIT_UPPER        (140 / DCF_TIME_BASE)

#define DCF_LIM_HI_BIT_LOWER        (150 / DCF_TIME_BASE)
#define DCF_LIM_HI_BIT_UPPER        (300 / DCF_TIME_BASE)

#define DCF_LIM_PAUSE_LOWER         (700 / DCF_TIME_BASE)
#define DCF_LIM_PAUSE_UPPER         (1000 / DCF_TIME_BASE)

#define DCF_LIM_SYNC_LOWER          (1700 / DCF_TIME_BASE)
#define DCF_LIM_SYNC_UPPER          (2000 / DCF_TIME_BASE)

#define DCF_LIM_TIMER_COUNTER       (2200 / DCF_TIME_BASE)

/**
 * @brief Pin the DCF77 receiver is connected to
 *
 * @see ports.h
 */
#define DCF77_INPUT PORTB, 0

/**
 * @brief Pin the DCF77 signal is being output
 *
 * This is primarily meant for a LED, which would provide an indication of the
 * received DCF77 time signal. With some exercise you can distinguish between
 * 100 ms pulses and 200 ms pulses. Furthermore it is possible to get an idea
 * of the quality of the signal. If there is a lot of noise in the proximity of
 * the receiver the LED will blink too fast and/or too often.
 *
 * @see ports.h
 */
#define DCF77_OUTPUT PORTD, 4

// Macro for inverted logic?

bool dcf77_signal_filter_input();

extern void dcf77_init();

extern void dcf77_enable();

extern void dcf77_disable();

extern void dcf77_ISR();

extern bool dcf77_getDateTime(datetime_t* DateTime_p);

#endif /* _WC_DCF77_H_ */
