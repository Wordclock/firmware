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

/*
 * Check whether CPU frequency is set
 */
#ifndef F_CPU

    /*
     * Output error message and thereby stop compilation
     */
    #error F_CPU unknown!

#endif

/**
 * @brief Macro building the actual version number
 *
 * This macro will "return" the MAJOR_VERSION and MINOR_VERSION combined into
 * a single byte. It is used to determine the exact version number of the
 * software being compiled.
 *
 * @see MAJOR_VERSION
 * @see MINOR_VERSION
 * @see SW_VERSION
 */
#define BUILD_VERSION(x, y) ((uint8_t)((x << 4) | (y)))

/**
 * @brief Major version number
 *
 * Along with the major version number there is a minor version number. Small
 * changes will only result in an increase of the minor version number, whereas
 * bigger releases will result in an increase of the major version number.
 *
 * @note As this is combined with MINOR_VERSION into a single byte, actually
 * only 4 bits are available for this number, which means that only 0 - 15 are
 * valid values.
 *
 * @see BUILD_VERSION()
 * @see MINOR_VERSION
 * @see SW_VERSION
 */
#define MAJOR_VERSION 0

/**
 * @brief Minor version number
 *
 * Along with the minor version number there is a major version number. Small
 * changes will only result in an increase of the minor version number, whereas
 * bigger releases will result in an increase of the major version number.
 *
 * @note As this is combined with MAJOR_VERSION into a single byte, actually
 * only 4 bits are available for this number, which means that only 0 - 15 are
 * valid values.
 *
 * @see BUILD_VERSION()
 * @see MAJOR_VERSION
 * @see SW_VERSION
 */
#define MINOR_VERSION 13

/**
 * @brief Actual version number
 *
 * The actual version number consists of both the major and the minor version
 * number. It is "calculated" using the macro BUILD_VERSION(). This byte will
 * then actually be used to identy and/or compare different versions among each
 * other.
 *
 * @see MINOR_VERSION
 * @see MAJOR_VERSION
 * @see BUILD_VERSION()
 */
#define SW_VERSION BUILD_VERSION(MAJOR_VERSION, MINOR_VERSION)

/**
 * @brief Represents the classic German display
 *
 * If set to 1 the software will be compiled with support for the classic
 * German layout. This effectively will make sure that display_wc_ger.h will
 * be included and compiled.
 *
 * @note Only of the following options should be set to 1: WC_DISP_GER,
 * WC_DISP_GER3, WC_DISP_ENG.
 *
 * @see WC_DISP_GER3
 * @see WC_DISP_ENG
 * @see display_wc_ger.h
 */
#define WC_DISP_GER 0

/**
 * @brief Represents the "new" German display
 *
 * If set to 1 the software will be compiled with support for the "new"
 * German layout. This effectively will make sure that display_wc_ger3.h will
 * be included and compiled.
 *
 * The "new" German layout supports three different "modes": Wessi, Rhein-Ruhr,
 * Ossi. For further details refer to [1].
 *
 * @note Only of the following options should be set to 1: WC_DISP_GER,
 * WC_DISP_GER3, WC_DISP_ENG.
 *
 * [1]: https://www.mikrocontroller.net/articles/Word_Clock#Deutsch_.283-sprachig.29
 *
 * @see WC_DISP_GER
 * @see WC_DISP_ENG
 * @see display_wc_ger3.h
 */
#define WC_DISP_GER3 1

/**
 * @brief Represents the English display
 *
 * If set to 1 the software will be compiled with support for the English
 * layout. This effectively will make sure that display_wc_eng.h will be
 * included and compiled.
 *
 * @note Only of the following options should be set to 1: WC_DISP_GER,
 * WC_DISP_GER3, WC_DISP_ENG.
 *
 * @see WC_DISP_GER
 * @see WC_DISP_GER3
 * @see display_wc_eng.h
 */
#define WC_DISP_ENG 0

/**
 * @brief Controls whether the software should be compiled with support for RGB
 *
 * The software itself supports RGB LEDs. However if non-RGB LEDs are used in a
 * particular build, this can be set to 1. It will make sure that any RGB
 * specific code won't be compiled, which will result in a smaller binary.
 *
 * There are quite a bunch of IR commands that are associated with the RGB
 * functionality. These are expected to be trained, unless INDIVIDUAL_CONFIG
 * is set to 1.
 *
 * @note When set to 1 only the red PWM channel will actually work.
 *
 * @see pwm.h
 */
#define MONO_COLOR_CLOCK 0

/**
 * @brief Controls whether the software should be compiled with support for
 * DCF77
 *
 * This controls whether or not the DCF77 functionality should be included.
 * Disabling it will save about 1 kilobyte of program space, so in case you
 * don't attach a DCF77 receiver to your build, you probably want to disable
 * this.
 *
 * @see dcf77.h
 */
#define DCF_PRESENT 1

/**
 * @brief Controls whether the software should be compiled with support for
 *   Ambilight
 *
 * This controls whether or not support for Ambilight should be included.
 * Disabling it won't save too much program space, however when
 * INDIVIDUAL_CONFIG is set to 1, you are not expected to train an IR command
 * for it. Otherwise you'll have to train an IR command (and/or skip it),
 * although you actually can't use it anyway.
 *
 * The support itself basically consists only of a simple toggle between "on"
 * and/or "off". The Ambilight is expected to be connected as defined in
 * USER_AMBILIGHT.
 *
 * @see user.h
 */
#define AMBILIGHT_PRESENT 1

/**
 * @brief Controls whether the software should be compiled with support for
 *   Bluetooth
 *
 * This controls whether or not support for Bluetooth should be included.
 * Disabling it won't save too much program space, however when
 * INDIVIDUAL_CONFIG is set to 1, you are not expected to train an IR command
 * for it. Otherwise you'll have to train an IR command (and/or skip it),
 * although you actually can't use it anyway.
 *
 * The support itself basically consists only of a simple toggle between "on"
 * and/or "off". The Bluetooth module is expected to be connected as defined in
 * USER_BLUETOOTH.
 *
 * @see user.h
 */
#define BLUETOOTH_PRESENT 1

/**
 * @brief Controls whether the software should be compiled with support for
 *   auxiliaries
 *
 * This controls whether or not support for auxiliaries should be included.
 * Disabling it won't save too much program space, however when
 * INDIVIDUAL_CONFIG is set to 1, you are not expected to train an IR command
 * for it. Otherwise you'll have to train an IR command (and/or skip it),
 * although you actually can't use it anyway.
 *
 * The support itself basically consists only of a simple toggle between "on"
 * and/or "off". The auxiliaries are expected to be connected as defined in
 * USER_AUXPOWER.
 *
 * @see user.h
 */
#define AUXPOWER_PRESENT 1

/**
 * @brief Controls whether the software should be compiled with support for
 *   resets via UART
 *
 * When set to 1 the microcontroller can be reset via UART by sending a "R"
 * to it. Depending upon the value of BOOTLOADER_RESET_WDT the reset is either
 * accomplished by directly jumping to address 0x00 or by letting the watchdog
 * timeout triggering a reset.
 *
 * @see BOOTLOADER_RESET_WDT
 */
#define BOOTLOADER_RESET_UART 1

/**
 * @brief Controls how a reset via UART should be accomplished
 *
 * When set to 1 the reset (see BOOTLOADER_RESET_UART) is accomplished by
 * enabling the watchdog timer and letting it timeout, resulting in a reset.
 * Otherwise it will directly jump to address 0, which is is not a classical
 * reset. This is needed when using [chip45boot2][1].
 *
 * [1]: http://www.chip45.com/avr_bootloader_atmega_xmega_chip45boot2.php
 *
 * @see BOOTLOADER_RESET_UART
 * @see ISR(USART_RX_vect)
 */
#define BOOTLOADER_RESET_WDT 1

/**
 * @brief Controls whether the software should be compiled with support for an
 *   individual config
 *
 * When set to 1 the IR commands that are expected to be trained will be built
 * dynamically, so that commands of modules not enabled are not expected to be
 * trained. Otherwise all commands are expected to be trained (and/or skipped),
 * resulting in useless and unintuitive assignments.
 *
 * @see DCF_PRESENT
 * @see MONO_COLOR_CLOCK
 * @see AMBILIGHT_PRESENT
 * @see BLUETOOTH_PRESENT
 * @see AUXPOWER_PRESENT
 * @see e_userCommands
 * @see user.h
 * @see user.c
 */
#define INDIVIDUAL_CONFIG 1

/**
 * @brief Controls whether changes to the setting should be saved automatically
 *
 * When set to 1 any changes to the settings done by the user will be saved
 * automatically after USER_DELAY_BEFORE_SAVE_EEPROM_S seconds. Otherwise the
 * user will have to trigger the saving manually.
 *
 * The saving itself is done within user_isr1Hz().
 *
 * @see USER_DELAY_BEFORE_SAVE_EEPROM_S
 * @see user.h
 * @see user_isr1Hz()
 */
#define USER_AUTOSAVE 1

/**
 * @brief Controls whether logging for the main "module" should be enabled
 *
 * This affects main.c and controls whether various functions within this file
 * should output debug information. This includes information about the
 * Initialization process as well as some basic information during actual
 * runtime.
 *
 * @see log_main()
 * @see handle_datetime()
 * @see main()
 * @see main.c
 */
#define LOG_MAIN 0

/**
 * @brief Controls whether logging for changes in brightness should be enabled
 *
 * This affects main.c and controls whether debug information about changes
 * in the brightness should be output.
 *
 * @see handle_brightness()
 * @see main.c
 */
#define LOG_MAIN_BRIGHTNESS 0

/**
 * @brief Controls whether logging for LDR measurements should be enabled
 *
 * This affects ldr.c and controls whether debug information about measured
 * values from the LDR should be output.
 *
 * @see ldr_init()
 * @see ldr_ISR()
 * @see ldr.c
 */
#define LOG_LDR 0

/**
 * @brief Controls whether logging of changes to the PWM due to changes in
 *   brightness should be enabled
 *
 * This affects pwm.c and controls whether debug information about changes to
 * the generated PWM signal due to changes in brightness of the ambient light
 * should be output.
 *
 * @see outputVals()
 * @see modifyLdrBrightness2pwmStep()
 * @see pwm.c
 */
#define LOG_LDR2PWM 0

/**
 * @brief Controls whether logging of changes to the display state should be
 *   enabled
 *
 * This affects display_wc.c and controls whether debug information about any
 * changes to the display state should be output.
 *
 * @see display_outputData()
 * @see display_wc.c
 */
#define LOG_DISPLAY_STATE 0

/**
 * @brief Controls whether logging of received IR codes should be enabled
 *
 * This affects user.c and controls whether debug information about any
 * received IR codes should be output. This includes the protocol, address and
 * the command of the received IR frame.
 *
 * @see handle_ir_code()
 * @see user.c
 */
#define LOG_USER_IR_CMD 0

/**
 * @brief Controls whether logging of the user state should be enabled
 *
 * This affects user.c and controls whether debug information about the user
 * state should be output. The user state is organized as a stack and this
 * enables that the content of the stack is output.
 *
 * @see printStateStack()
 * @see log_state()
 * @see addState()
 * @see user.c
 */
#define LOG_USER_STATE 0

/**
 * @brief Controls whether logging of changes to the time should be enabled
 *
 * This affects user.c and controls whether debug information about changes
 * to the time should be output.
 *
 * @see log_time()
 * @see user_setNewTime()
 * @see user.c
 */
#define LOG_USER_TIME 0

/**
 * @brief Controls whether logging of the IR training process should be enabled
 *
 * This affects usermodes.c and controls whether debug information about the IR
 * training process should be output.
 *
 * @see log_irTrain()
 * @see TrainIrState_1Hz()
 * @see TrainIrState_handleIR()
 * @see usermodes.c
 */
#define LOG_USER_IR_TRAIN 0

/**
 * @brief Controls whether logging of the initialization of the EEPROM module
 *   should be enabled
 *
 * This affects wceeprom.c and controls whether debug information about the
 * initialization process of the EEPROM module should be output.
 *
 * @see uart_putHexByte()
 * @see wcEeprom_init()
 * @see wceeprom.c
 */
#define LOG_EEPROM_INIT 0

/**
 * @brief Controls whether logging of writebacks to the EEPROM should be enabled
 *
 * This affects wceeprom.c and controls whether debug information about
 * writebacks to the EEPROM should be output.
 *
 * @see log_eeprom()
 * @see uart_putHexByte()
 * @see wcEeprom_writeIfChanged()
 * @see wcEeprom_writeback()
 * @see wceeprom.c
 */
#define LOG_EEPROM_WRITEBACK 0

/**
 * @brief Controls whether logging of the DCF77 modules should be enabled
 *
 * This affects dcf77.c and controls whether various debug information about
 * actions within the DCF77 modules should be output.
 *
 * @see log_dcf77()
 * @see dcf77_reset()
 * @see dcf77_check_receiver_type()
 * @see dcf77_check()
 * @see dcf77.c
 */
#define LOG_DCF77 0

extern void main_ISR();

#endif /* _WC_MAIN_H_ */
