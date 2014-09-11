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
 * This macro will build the actual version number by combining `MAJOR_VERSION`
 * and `MINOR_VERSION` and shifting it appropriately.
 *
 * @see MAJOR_VERSION
 * @see MINOR_VERSION
 */
#define BUILD_VERSION(x, y) ((uint16_t)((x << 8) | (y)))

/**
 * @brief Major version number
 *
 * @note This is effectively a byte wide and can hold values between 0 and 255.
 *
 * @see BUILD_VERSION()
 * @see MINOR_VERSION
 * @see SW_VERSION
 */
#define MAJOR_VERSION 1

/**
 * @brief Minor version number
 *
 * @note This is effectively a byte wide and can hold values between 0 and 255.
 *
 * @see BUILD_VERSION()
 * @see MINOR_VERSION
 * @see SW_VERSION
 */
#define MINOR_VERSION 0

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
 * If set to 1, the firmware will be built with full support for RGB LEDs.
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
#define ENABLE_RGB_SUPPORT 1

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
 * @see ENABLE_RGB_SUPPORT
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
 * @brief Default state of logging module
 *
 * This controls the default state (enabled, disabled) of the logging module.
 * If enabled (= 1) the logging is enabled globally by default, otherwise it
 * is disabled and has to be enabled during runtime in order to generate any
 * output at all.
 *
 * @see log_enable()
 */
#define LOG_ENABLE_DEFAULT 0

/**
 * @brief Default log level of main module
 *
 * This affects `main.c` and controls whether information and errors during the
 * initialization should be output.
 *
 * @see log_level_t
 * @see main.c
 */
#define LOG_LEVEL_MAIN_DEFAULT LOG_LEVEL_NONE

/**
 * @brief Default log level of the brightness module
 *
 * This affects `brightness.c` and controls whether the current brightness
 * value should be output.
 *
 * @see log_level_t
 * @see brightness.c
 */
#define LOG_LEVEL_BRIGHTNESS_DEFAULT LOG_LEVEL_NONE

/**
 * @brief Default log level of the LDR module
 *
 * This affects whether LDR values as measured by the ADC should be output.
 *
 * @see log_level_t
 * @see ldr.c
 */
#define LOG_LEVEL_LDR_DEFAULT LOG_LEVEL_NONE

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
 * @brief Default log level for IR commands within user module
 *
 * This affects the {@link user.h user} module and controls whether debug
 * information about the received IR frames and the training state should be
 * output.
 *
 * @see log_level_t
 */
#define LOG_LEVEL_USER_IR_DEFAULT LOG_LEVEL_NONE

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
 * @brief Default log level of the UART protocol module
 *
 * This affects `uart_protocol.c` and controls whether information about its
 * current state should be output.
 *
 * @see log_level_t
 * @see uart_protocol.c
 */
#define LOG_LEVEL_UART_PROTOCOL_DEFAULT LOG_LEVEL_NONE

/**
 * @brief Default log level of the datetime module
 *
 * @see log_level_t
 * @see datetime.c
 */
#define LOG_LEVEL_DATETIME_DEFAULT LOG_LEVEL_NONE

#endif /* _WC_CONFIG_H_ */
