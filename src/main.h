/*
 * Copyright (C) 2012, 2013 Karol Babioch <karol@babioch.de>
 * Copyright (c) 2010 Frank Meyer - frank(at)fli4l.de
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
 * @file main.h
 * @brief The main configuration file
 *
 * This file contains the most important configuration options of the project.
 * It gets included by various other modules to read back the options and
 * depending upon their value actually change the compilation.
 *
 * @see main.c
 */

#ifndef _WC_MAIN_H_
#define _WC_MAIN_H_

#ifndef F_CPU

    #error F_CPU unknown!

#endif

#define BUILD_VERSION(x, y) ((uint8_t)((x << 4) | (y)))

#define MAJOR_VERSION 0
#define MINOR_VERSION 12

#define SW_VERSION BUILD_VERSION(MAJOR_VERSION, MINOR_VERSION)

#define WC_DISP_GER 0
#define WC_DISP_GER3 1
#define WC_DISP_ENG 0

#define MONO_COLOR_CLOCK 0

#define DCF_PRESENT 1
#define AMBILIGHT_PRESENT 1
#define BLUETOOTH_PRESENT 1
#define AUXPOWER_PRESENT 1

#define BOOTLOADER_RESET_UART 1
#define BOOTLOADER_RESET_WDT 1

#define INDIVIDUAL_CONFIG 1

#define USER_AUTOSAVE 1

#define LOG_MAIN 0
#define LOG_MAIN_BRIGHTNESS 0
#define LOG_LDR 0
#define LOG_LDR2PWM 0
#define LOG_DISPLAY_STATE 0
#define LOG_USER_IR_CMD 0
#define LOG_USER_STATE 0
#define LOG_USER_TIME 0
#define LOG_USER_IR_TRAIN 0
#define LOG_EEPROM_INIT 0
#define LOG_EEPROM_WRITEBACK 0
#define LOG_DCF77 0

extern void main_ISR(void);

#endif /* _WC_MAIN_H_ */
