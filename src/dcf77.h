/*
 * Copyright (C) 2012, 2013 Karol Babioch <karol@babioch.de>
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
 * @brief Header file for access to the DCF77 time signal
 *
 * DCF77 is used to receive the current date and time automatically. This contains
 * the header declaration that provide access to the date and time information.
 *
 * In order for this module to work a DCF77 receiver is needed, which looks
 * something like [this][1].
 *
 * This module is reported to work with the following receivers:
 *
 * * [DCF-Empfangsmodul DCF1][2] from pollin.de
 * * [C-Control DCF-Empfängerplatine][3] from conrad.de
 * * [DCF 77 Empfängermodul][4] from reichelt.de
 *
 * All these receivers output the demodulated signal digitally. It then needs
 * to be decoded, which is done within this module.
 *
 * [1]: https://en.wikipedia.org/wiki/File:Low_cost_DCF77_receiver.jpg
 * [2]: http://www.pollin.de/shop/dt/NTQ5OTgxOTk-/Bausaetze_Module/Module/DCF_Empfangsmodul_DCF1.html
 * [3]: http://www.conrad.de/ce/de/product/641138/C-Control-DCF-Empfaengerplatine
 * [4]: https://secure.reichelt.de/DCF77-MODUL/3/index.html?ACTION=3&GROUPID=3636&ARTICLE=57772
 *
 * @see dcf77.c
 */

#ifndef _WC_DCF77_H_
#define _WC_DCF77_H_

#include <stdbool.h>

#include "datetime.h"

#if (DCF_PRESENT == 1)

extern void dcf77_init(void);

extern void dcf77_enable(void);

extern void dcf77_disable(void);

extern void dcf77_ISR(void);

extern bool dcf77_getDateTime(datetime_t* DateTime_p);

#else

/**
 * @brief Empty macro in case DCF77 functionality is disabled
 *
 * When the DCF77 functionality is disabled during the compilation this empty
 * macro makes sure that the ISR won't be included.
 *
 * @see DCF_PRESENT
 * @see INTERRUPT_100HZ
 */
#define dcf77_ISR()

#endif /* (DCF_PRESENT == 1) */

#endif /* _WC_DCF77_H_ */
