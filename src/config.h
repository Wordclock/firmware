/*
 * Copyright (C) 2012, 2013, 2014 Karol Babioch <karol@babioch.de>
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
 * @file config.h
 * @brief The main configuration file of the whole project
 *
 * This file contains global configuration options for the whole project and
 * is used throughout the project.
 */

#ifndef _WC_CONFIG_H_
#define _WC_CONFIG_H_

/**
 * @brief Macro building the actual version number
 *
 * This macro will build actual version number by combining `MAJOR_VERSION`
 * and `MINOR_VERSION` and shifting it appropriately.
 *
 * @see MAJOR_VERSION
 * @see MINOR_VERSION
 */
#define BUILD_VERSION(x, y) ((uint8_t)((x << 4) | (y)))

/**
 * @brief Major version number
 *
 * @note As this is combined with `MINOR_VERSION` into a single byte, so
 * actually only values between 0 - 15 are valid.
 *
 * @see BUILD_VERSION()
 * @see MINOR_VERSION
 * @see SW_VERSION
 */
#define MAJOR_VERSION 0

/**
 * @brief Minor version number
 *
 * @note As this is combined with `MINOR_VERSION` into a single byte, so
 * actually only values between 0 - 15 are valid.
 *
 * @see BUILD_VERSION()
 * @see MINOR_VERSION
 * @see SW_VERSION
 */
#define MINOR_VERSION 13

/**
 * @brief Actual version number
 *
 * The actual version number consists of both the major and the minor version
 * number. It is build by the the macro `BUILD_VERSION()`.
 *
 * @see MINOR_VERSION
 * @see MAJOR_VERSION
 * @see BUILD_VERSION()
 */
#define SW_VERSION BUILD_VERSION(MAJOR_VERSION, MINOR_VERSION)

/**
 * @brief Classic German frontpanel layout
 *
 * If set to 1, the software will be compiled with support for the classic
 * German frontpanel layout. This layout supports up to three different idioms:
 *
 *  - Wessi
 *  - Ossi
 *
 * For further details please refer to [1].
 *
 * [1]: https://www.mikrocontroller.net/articles/Word_Clock#Deutsch_.282-sprachig.29
 *
 * @note Only of the following options should be set to 1: `WC_DISP_GER`,
 * `WC_DISP_GER3`, `WC_DISP_ENG`.
 *
 * @see WC_DISP_GER3
 * @see WC_DISP_ENG
 * @see display.h
 */
#define WC_DISP_GER 0

/**
 * @brief Modern German frontpanel layout
 *
 * If set to 1, the software will be compiled with support for the modern
 * German frontpanel layout. This layout supports up to three different idioms:
 *
 *  - Wessi
 *  - Rhein-Ruhr
 *  - Ossi
 *
 * For further details please refer to [1].
 *
 * [1]: https://www.mikrocontroller.net/articles/Word_Clock#Deutsch_.283-sprachig.29
 *
 * @note Only of the following options should be set to 1: `WC_DISP_GER`,
 * `WC_DISP_GER3`, `WC_DISP_ENG`.
 *
 * @see WC_DISP_GER
 * @see WC_DISP_ENG
 * @see display.h
 */
#define WC_DISP_GER3 1

/**
 * @brief English frontpanel layout
 *
 * If set to 1, the software will be compiled with support for the English
 * frontpanel layout. For further details please refer to [1].
 *
 * [1]: https://www.mikrocontroller.net/articles/Word_Clock#Englisch
 *
 * @note Only of the following options should be set to 1: `WC_DISP_GER`,
 * `WC_DISP_GER3`, `WC_DISP_ENG`.
 *
 * @see WC_DISP_GER
 * @see WC_DISP_GER3
 * @see display.h
 */
#define WC_DISP_ENG 0

/**
 * @brief Controls whether the software should include support for RGB
 *
 * If set to 0, the firmware will be built with full support for RGB LEDs.
 * Otherwise the resulting firmware will not contain any RGB specific code,
 * rendering it into a monochromatic build.
 *
 * @note Depending upon the value of `ENABLE_INDIVIDUAL_CONFIG` the appropriate
 * user commands might be unavailable when disabling this option.
 *
 * @note In a monochromatic build only the red channel will output a signal.
 *
 * @see pwm.h
 */
#define MONO_COLOR_CLOCK 0

/**
 * @brief Defines whether support for DCF77 should be included
 *
 * If set to 1, the firmware will be built with support for decoding the
 * DCF77 time signal in order to be able to get the current date and time
 * automatically.
 *
 * @note Depending upon the value of `ENABLE_INDIVIDUAL_CONFIG` the appropriate
 * user command might be unavailable when disabling this option.
 *
 * @see dcf77.h
 */
#define ENABLE_DCF_SUPPORT 1

/**
 * @brief Defines whether support for Ambilight should be included
 *
 * If set to 1, the firmware will be built with support for the Ambilight,
 * which basically comes down to toggling the pin defined in `USER_AMBILIGHT`.
 *
 * @note Depending upon the value of `ENABLE_INDIVIDUAL_CONFIG` the appropriate
 * user command might be unavailable when disabling this option.
 *
 * @see user.h
 */
#define ENABLE_AMBILIGHT_SUPPORT 1

/**
 * @brief Defines whether support for Bluetooth should be included
 *
 * If set to 1, the firmware will be built with support for a Bluetooth
 * transceiver, which basically comes down to toggling the pin defined in
 * `USER_BLUETOOTH`. This makes it possible to enable and/or disable the
 * Bluetooth transceiver using the IR remote control.
 *
 * @note Depending upon the value of `ENABLE_INDIVIDUAL_CONFIG` the appropriate
 * user command might be unavailable when disabling this option.
 *
 * @see user.h
 */
#define ENABLE_BLUETOOTH_SUPPORT 1

/**
 * @brief Defines whether support for auxiliary should be included
 *
 * If set to 1, the firmware will be built with support for auxiliary, which
 * basically comes down to toggling the pin defined in `USER_AUXPOWER`.
 *
 * @note Depending upon the value of `ENABLE_INDIVIDUAL_CONFIG` the appropriate
 * user command might be unavailable when disabling this option.
 *
 * @see user.h
 */
#define ENABLE_AUXPOWER_SUPPORT 1

/**
 * @brief Defines whether unused user command should be included
 *
 * If set to 1, the firmware will be built without support for user commands
 * of disabled options. This makes the resulting binary a smaller and doesn't
 * require the user to train user commands that wouldn't have any effect
 * anyway.
 *
 * @see ENABLE_DCF_SUPPORT
 * @see MONO_COLOR_CLOCK
 * @see ENABLE_AMBILIGHT_SUPPORT
 * @see ENABLE_BLUETOOTH_SUPPORT
 * @see ENABLE_AUXPOWER_SUPPORT
 * @see user_command_t
 */
#define ENABLE_INDIVIDUAL_CONFIG 1

/**
 * @brief Defines whether changed settings should be saved automatically
 *
 * If set to 1, the firmware will be built in a way that settings changed by
 * the user will be saved automatically after a timeout defined by
 * `USER_DELAY_BEFORE_SAVE_EEPROM_S`.
 *
 * @see USER_DELAY_BEFORE_SAVE_EEPROM_S
 */
#define ENABLE_USER_AUTOSAVE 1

/**
 * @brief Defines how a reset of the microcontroller will be performed
 *
 * If set to 1, the firmware will be built in a way that a reset of the
 * microcontroller is being performed by making use of the watchdog timer.
 * Otherwise the microcontroller will simply jump directly to the the
 * bootloader, which technically is not a reset, but is the way
 * [chip45boot2][1] expects it to be.
 *
 * [1]: http://www.chip45.com/avr_bootloader_atmega_xmega_chip45boot2.php
 *
 * @see _reset()
 */
#define BOOTLOADER_RESET_WDT 1

/**
 * @brief Defines whether support for the UART protocol should be included
 *
 * This defines whether the support for the UART protocol should be built into
 * the resulting binary. This can be used to control the Wordclock using UART.
 *
 * The protocol itself is documented in the `doc/UART_PROTOCOL.md`.
 *
 * @todo Implement this in a way that is actually saves some amount of program
 * space rather than just disabling the functionality but still compiling it
 * in.
 *
 * @see uart_protocol.h
 */
#define ENABLE_UART_PROTOCOL 1

/**
 * @brief Defines whether the main module should output logging information
 *
 * This affects `main.c` and controls whether functions within this file
 * should output debugging information. This includes information about the
 * initialization process as well as some rudimentary information during
 * runtime.
 *
 * @see main.c
 */
#define LOG_MAIN 0

/**
 * @brief Defines whether changes in the brightness should be logged
 *
 * This affects `main.c` and controls whether debug information about changes
 * in the brightness should be logged.
 *
 * @see main.c
 */
#define LOG_MAIN_BRIGHTNESS 0

/**
 * @brief Defines whether the measured LDR values should be logged
 *
 * This affects `ldr.c` and controls whether debug information about measured
 * values from the LDR should be output.
 *
 * @see ldr.c
 */
#define LOG_LDR 0

/**
 * @brief Defines whether changes of the PWM signal should be logged
 *
 * This affects `pwm.c` and controls whether debug information about changes to
 * the generated PWM signal due to changes in brightness of the ambient light
 * should be output.
 *
 * @see pwm.c
 */
#define LOG_LDR2PWM 0

/**
 * @brief Defines whether changes of the display state should be logged
 *
 * This affects `display_wc.c` and controls whether debug information about any
 * changes to the display state should be output.
 *
 * @see display_wc.c
 */
#define LOG_DISPLAY_STATE 0

/**
 * @brief Defines whether received IR codes should be logged
 *
 * This affects `user.c` and controls whether debug information about any
 * received IR codes should be output. This includes the protocol, address and
 * the actual command of the received IR frame.
 *
 * @see user.c
 */
#define LOG_USER_IR_CMD 0

/**
 * @brief Defines whether information about the user state should be logged
 *
 * This affects `user.c` and controls whether debug information about the user
 * state should be output.
 *
 * @see user.c
 */
#define LOG_USER_STATE 0

/**
 * @brief Defines whether changes in time within user module should be logged
 *
 * This affects `user.c` and controls whether debug information about changes
 * to the time should be output.
 *
 * @see user.c
 */
#define LOG_USER_TIME 0

/**
 * @brief Defines whether information during IR training should be logged
 *
 * This affects `usermodes.c` and controls whether debug information about the
 * IR training process should be output.
 *
 * @see usermodes.c
 */
#define LOG_USER_IR_TRAIN 0

/**
 *
 * @brief Defines whether information about EEPROM init should be logged
 *
 * This affects `wceeprom.c` and controls whether debug information about the
 * initialization process of the EEPROM module should be output.
 *
 * @see wceeprom.c
 */
#define LOG_EEPROM_INIT 0

/**
 * @brief Defines whether information about EEPROM writeback should be logged
 *
 * This affects `wceeprom.c` and controls whether debug information about
 * writebacks to the EEPROM should be output.
 *
 * @see wceeprom.c
 */
#define LOG_EEPROM_WRITEBACK 0

/**
 * @brief Defines whether information about the DCF77 decoding should be logged
 *
 * This affects `dcf77.c` and controls whether various debug information about
 * the current state of the DCF77 module should be output.
 *
 * @see dcf77.c
 */
#define LOG_DCF77 0

/**
 * @brief Defines whether information about the UART protocol should be logged
 *
 * This affects `uart_protocol.c` and controls whether various debug
 * information about the current state should be output.
 *
 * @see uart_protocol.c
 */
#define LOG_UART_PROTOCOL 0

#endif /* _WC_CONFIG_H_ */
