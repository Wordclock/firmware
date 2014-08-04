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
 * @file uart_protocol.c
 * @brief Implementation of the header declared in uart_protocol.c
 *
 * This implements the functionality declared in uart_protocol.h. It basically
 * retrieves any characters received by the UART hardware, puts it into a
 * buffer and compares it against a predefined table of commands once the EOL
 * character has been detected. Callback functions process each command
 * individually.
 *
 * @see uart_protocol.h
 * @see uart.h
 */

#include <avr/interrupt.h>

#include <stdarg.h>
#include <string.h>

#include "config.h"
#include "base.h"
#include "datetime.h"
#include "i2c_rtc.h"
#include "ldr.h"
#include "uart.h"
#include "uart_protocol.h"
#include "user.h"
#include "user_command.h"
#include "pwm.h"
#include "wceeprom.h"


/**
 * @brief Outputs a message in the correct format
 *
 * This outputs the given message in the correct format (enclosed by
 * #UART_PROTOCOL_OUTPUT_PREFIX and #UART_PROTOCOL_OUTPUT_EOL).
 *
 * @see UART_PROTOCOL_OUTPUT_PREFIX
 * @see UART_PROTOCOL_OUTPUT_EOL
 */
static void uart_protocol_output(const char* message)
{

    uart_puts_P(UART_PROTOCOL_OUTPUT_PREFIX);
    uart_puts(message);
    uart_puts_P(UART_PROTOCOL_OUTPUT_EOL);

}

/**
 * @brief Outputs a success message
 *
 * @see uart_protocol_output()
 */
static void uart_protocol_ok()
{

    uart_protocol_output("OK");

}

/**
 * @brief Outputs an error message
 *
 * @see uart_protocol_output()
 */
static void uart_protocol_error()
{

    uart_protocol_output("ERROR");

}

/**
 * @brief Puts out the hex representation of the given arguments
 *
 * This puts out the hex representation for each given argument by building up
 * the string manually and finally passing it over to uart_protocol_output().
 *
 * @param args Number of arguments
 * @param ... Actual arguments to convert and output
 *
 * @see uint8ToHexStr()
 * @see uart_protocol_output()
 *
 * @note This is a variadic function based upon `<stdarg.h>`. The number of
 * arguments varies and is described by the first argument.
 */
static void uart_protocol_output_args_hex(uint8_t args, ...)
{

    va_list va;
    va_start(va, args);

    // Three bytes per character (2 byte hex representation + space/terminator)
    char str[args * 3];

    for (uint8_t i = 0; i < args; i++) {

        uint8ToHexStr((uint8_t)va_arg(va, int), &str[i * 3]);

        if (i == args - 1) {

            // String terminator at the very end
            str[i * 3 + 2] = '\0';

        } else {

            // Space between arguments
            str[i * 3 + 2] = ' ';

        }

    }

    uart_protocol_output(str);

    va_end(va);

}

/**
 * @brief Defines the size of the command buffer
 *
 * @note Commands are represented as simple strings, this also needs to include
 * the null terminator, making the possible command length effectively one
 * character smaller than defined here.
 *
 * @see uart_protocol_command_buffer
 */
#define UART_PROTOCOL_COMMAND_BUFFER_SIZE 16

/**
 * Maximum length of a command (without arguments)
 *
 * A command along with its arguments can be as long as defined by
 * #UART_PROTOCOL_COMMAND_BUFFER_SIZE. The command itself without arguments,
 * however, can only be as long as defined here.
 *
 * @note Keep the length of commands short to make more arguments possible.
 *
 * @see UART_PROTOCOL_COMMAND_BUFFER_SIZE
 * @see uart_protocol_command_t::command
 */
#define UART_PROTOCOL_COMMAND_MAX_LENGTH 3

/**
 * @brief Maximum amount of arguments for a single command
 *
 * The command itself can be as long as defined by
 * #UART_PROTOCOL_COMMAND_MAX_LENGTH. Everything following the command is
 * considered to be one or more argument(s). Arguments are separated by a space
 * character between them. This defines the maximum amount of arguments a
 * single command can have.
 *
 * @see UART_PROTOCOL_COMMAND_MAX_LENGTH
 * @see uart_protocol_tokenize_command_buffer()
 * @see uart_protocol_handle()
 */
#define UART_PROTOCOL_COMMAND_BUFFER_MAX_ARGS 5

/**
 * @brief The actual command buffer
 *
 * This holds all of the characters received via UART until the
 * {@link #UART_PROTOCOL_INPUT_EOL EOL character} is received, which will
 * trigger the command parsing and clear the buffer.
 *
 * @see uart_protocol_handle()
 * @see UART_PROTOCOL_COMMAND_BUFFER_SIZE
 */
static char uart_protocol_command_buffer[UART_PROTOCOL_COMMAND_BUFFER_SIZE];

/**
 * @brief Type definition of callback function for processing a command
 *
 * All callback functions registered within #uart_protocol_commands need to
 * fulfill this type. The first argument contains the number of arguments
 * found, whereas the second one is an array of char pointers to each argument.
 * This is somewhat analogous to the `main` function of a typical C program.
 *
 * @see uart_protocol_command_t
 * @see uart_protocol_commands
 */
typedef void (*uart_protocol_command_callback_t)(uint8_t argc, char* argv[]);

/**
 * @brief Executes the given user command
 *
 * This effectively implements the IR emulation mode. For each user command
 * a single character is defined. The received character will be compared
 * against all of the entries of the defined table.
 *
 * If a match is found the appropriate user command will be issued and a
 * success message will be output. Otherwise an error message will be output.
 *
 * @see uart_protocol_command_callback_t
 * @see uart_protocol_ok()
 * @see uart_protocol_error()
 * @see handle_user_command()
 * @see user_command_t
 */
static void _ir_user_command(uint8_t argc, char* argv[])
{

    if (strlen(argv[1]) == 1) {

        /**
         * @brief Defines the type of entries within user_command_assignments
         *
         * @see user_command_assignments
         */
        typedef struct {

            /**
             * @brief Character that a user command should be assigned to
             */
            char command;

            /**
             * @brief The user command
             */
            user_command_t user_command;

        } user_command_assignment_t;

        /**
         * @brief Definition of all available commands
         *
         * This defines all of the user commands that will be detected and the
         * appropriate character that is responsible for triggering these user
         * commands.
         *
         * @note Depending upon the value of ENABLE_INDIVIDUAL_CONFIG not all
         * of the entries might end up in the resulting firmware.
         *
         * @note Keep in mind that this table is accessed iteratively, so it
         * might make sense to order the entries appropriately.
         *
         * @see user_command_assignment_t
         * @see ENABLE_INDIVIDUAL_CONFIG
         */
        const user_command_assignment_t user_command_assignments[] = {

            {'o', UC_ONOFF},
            {'l', UC_BRIGHTNESS_UP},
            {'m', UC_BRIGHTNESS_DOWN},
            {'+', UC_UP},
            {'-', UC_DOWN},
            {'t', UC_SET_TIME},
            {'a', UC_SET_ONOFF_TIMES},

            #if (ENABLE_INDIVIDUAL_CONFIG == 0 || ENABLE_DCF_SUPPORT == 1)

                {'d', UC_DCF_GET_TIME},

            #endif

            {'N', UC_NORMAL_MODE},
            {'P', UC_PULSE_MODE},
            {'D', UC_DEMO_MODE},

            #if (ENABLE_INDIVIDUAL_CONFIG == 0 || ENABLE_RGB_SUPPORT == 1)

                {'H', UC_HUE_MODE},
                {'r', UC_CHANGE_R},
                {'g', UC_CHANGE_G},
                {'b', UC_CHANGE_B},
                {'h', UC_CHANGE_HUE},

            #endif

            {'c', UC_CALIB_BRIGHTNESS},

            #if (ENABLE_INDIVIDUAL_CONFIG == 0 || ENABLE_AMBILIGHT_SUPPORT == 1)

                {'A', UC_AMBILIGHT},

            #endif

            #if (ENABLE_INDIVIDUAL_CONFIG == 0 || ENABLE_BLUETOOTH_SUPPORT == 1)

                {'B', UC_BLUETOOTH},

            #endif

            #if (ENABLE_INDIVIDUAL_CONFIG == 0 || ENABLE_AUXPOWER_SUPPORT == 1)

                {'X', UC_AUXPOWER},

            #endif

            {'s', UC_SELECT_DISP_MODE}

        };

        uint8_t j = sizeof(user_command_assignments) / sizeof(user_command_assignment_t);

        for (uint8_t i = 0; i < j; i++) {

            if (argv[1][0] == user_command_assignments[i].command) {

                handle_user_command(user_command_assignments[i].user_command);
                uart_protocol_ok();

                return;

            }

        }

    }

    uart_protocol_error();

}

/**
 * @brief Puts out the version number of the firmware
 *
 * This puts out the hex representation of the version number (major & minor)
 * of the firmware.
 *
 * @see MAJOR_VERSION
 * @see MINOR_VERSION
 * @see uart_protocol_command_callback_t
 * @see uart_protocol_output_args_hex()
 */
static void _version(uint8_t argc, char* argv[])
{

    uart_protocol_output_args_hex(2, MAJOR_VERSION, MINOR_VERSION);

}

/**
 * @brief Puts out a keepalive message to the user
 *
 * This can be used to keep the connection alive and/or check whether the
 * connection is actually still alive and is guaranteed to have no side
 * effects.
 *
 * @see uart_protocol_command_callback_t
 * @see uart_protocol_ok()
 */
static void _keepalive(uint8_t argc, char* argv[])
{

    uart_protocol_ok();

}

/**
 * @brief Resets the microcontroller
 *
 * This performs a reset of the microcontroller. Depending upon the setting of
 * #BOOTLOADER_RESET_WDT this is either performed by letting the watchdog
 * timeout or by directly jumping to a specific address location (`0x3800`). To
 * make sure that this whole procedure will not be interrupted, the interrupt
 * are disabled globally.
 *
 * @see uart_protocol_command_callback_t
 * @see uart_protocol_ok()
 * @see BOOTLOADER_RESET_WDT
 */
static void _reset(uint8_t argc, char* argv[])
{

    uart_protocol_ok();
    uart_flush_output();
    cli();

    #if (BOOTLOADER_RESET_WDT == 1)

        #include <avr/wdt.h>

        wdt_enable(WDTO_15MS);

        while (1);

    #else

        asm volatile("jmp 0x3800");

    #endif

}

/**
 * @brief Resets the firmware to its factory state
 *
 * This performs a factory reset by invalidating WcEepromData#swVersion and
 * resetting the microcontroller afterwards. The built-in integrity check
 * of the EEPROM module will make sure that the default values will be used
 * during the next reset.
 *
 * @see uart_protocol_command_callback_t
 * @see WcEepromData::swVersion
 * @see _reset()
 */
static void _factory_reset(uint8_t argc, char* argv[])
{

    wcEeprom_getData()->swVersion = 0;
    wcEeprom_writeback(&wcEeprom_getData()->swVersion,
        sizeof(wcEeprom_getData()->swVersion));

    _reset(0, NULL);

}

/**
 * @brief Puts out the currently measured brightness of the LDR module
 *
 * This puts out the hex representation of the brightness currently being
 * returned by ldr_get_brightness().
 *
 * @see uart_protocol_command_callback_t
 * @see ldr_get_brightness()
 * @see uart_protocol_output_args_hex()
 */
static void _ldr_brightness(uint8_t argc, char* argv[])
{

    uart_protocol_output_args_hex(1, ldr_get_brightness());

}

#if (ENABLE_RGB_SUPPORT == 1)

/**
 * @brief Puts out the RGB values of the color currently being used
 *
 * This puts out the hex representation of the RGB values of the color
 * currently being used. It retrieves the values directly from the PWM module
 * and passes them over to uart_protocol_output_args_hex().
 *
 * @see uart_protocol_command_callback_t
 * @see pwm_get_color()
 * @see uart_protocol_output_args_hex()
 *
 * @todo Not real 8 bits, but 2^8-1?
 */
static void _color_read(uint8_t argc, char* argv[])
{

    const color_rgb_t* color = pwm_get_color();
    uart_protocol_output_args_hex(3, color->red, color->green, color->blue);

}

/**
 * @brief Sets the currently used color
 *
 * This sets the RGB values currently being used by the PWM module to generate
 * its signal. It retrieves the hex representation of the value for each
 * channel (red, green, blue) from the command buffer and applies them
 * appropriately. When something is wrong with the arguments nothing is
 * actually applied and an error message will be output.
 *
 * @see uart_protocol_command_callback_t
 * @see uart_protocol_command_buffer
 * @see uart_protocol_error()
 * @see uart_protocol_ok()
 * @see hexStrToUint8()
 * @see pwm_set_color()
 *
 * @todo Not real 8 bits, but 2^8-1?
 * @todo Let the user set 00 00 00? Disable?
 */
static void _color_write(uint8_t argc, char* argv[])
{

    color_rgb_t color;
    bool status;

    color.red = hexStrToUint8(argv[1], &status);

    if (!status) {

        uart_protocol_error();

        return;

    }

    color.green = hexStrToUint8(argv[2], &status);

    if (!status) {

        uart_protocol_error();

        return;

    }

    color.blue = hexStrToUint8(argv[3], &status);

    if (!status) {

        uart_protocol_error();

        return;

    }

    pwm_set_color(color);
    uart_protocol_ok();

}

/**
 * @brief Puts out number of color presets compiled into the firmware
 *
 * This puts out the hex representation of the number of supported color
 * presets compiled into the firmware.
 *
 * @see uart_protocol_command_callback_t
 * @see UI_COLOR_PRESET_COUNT
 * @see uart_protocol_output_args_hex()
 */
static void _preset_number(uint8_t argc, char* argv[])
{

    uart_protocol_output_args_hex(1, UI_COLOR_PRESET_COUNT);

}

/**
 * @brief Puts out the currently active color preset
 *
 * This puts out the hex representation of the color preset currently being
 * active.
 *
 * @see uart_protocol_command_callback_t
 * @see uart_protocol_output_args_hex()
 * @see UserEepromParams::curColorProfile
 *
 * @todo Return error when currently not in normal mode?
 */
static void _preset_active(uint8_t argc, char* argv[])
{

    uint8_t preset = (&(wcEeprom_getData()->userParams))->curColorProfile;
    uart_protocol_output_args_hex(1, preset);

}

/**
 * @brief Sets the currently active color preset
 *
 * This retrieves the hex representation of the number to set for the currently
 * active color preset from the command buffer and saves it. If the current
 * mode is equal to #MS_normalMode, it will be applied immediately.
 *
 * @see uart_protocol_command_callback_t
 * @see uart_protocol_command_buffer
 * @see hexStrToUint8()
 * @see UserEepromParams::curColorProfile
 * @see pwm_set_color()
 */
static void _preset_set(uint8_t argc, char* argv[])
{

    bool status;
    uint8_t preset = hexStrToUint8(argv[1], &status);

    if (status && preset < UI_COLOR_PRESET_COUNT) {

        (&(wcEeprom_getData()->userParams))->curColorProfile = preset;

        wcEeprom_writeback(&wcEeprom_getData()->userParams.curColorProfile,
            sizeof(wcEeprom_getData()->userParams.curColorProfile));

        if (user_get_current_menu_state() == MS_normalMode) {

            addState(MS_normalMode, &preset);

        }

        uart_protocol_ok();

        return;

    }

    uart_protocol_error();

}

/**
 * @brief Puts out the RGB values for the given color preset
 *
 * This puts out the hex representation of the RGB values of the given color
 * preset.
 *
 * @see uart_protocol_command_callback_t
 * @see uart_protocol_command_buffer
 * @see hexStrToUint8()
 * @see UserEepromParams::colorPresets
 * @see uart_protocol_output_args_hex()
 */
static void _preset_read(uint8_t argc, char* argv[])
{

    bool status;
    uint8_t preset = hexStrToUint8(argv[1], &status);

    if (status && preset < UI_COLOR_PRESET_COUNT) {

        color_rgb_t color = (&(wcEeprom_getData()->userParams))->colorPresets[preset];

        uart_protocol_output_args_hex(3, color.red, color.green, color.blue);

        return;

    }

    uart_protocol_error();

}

/**
 * @brief Sets the color for the given color preset
 *
 * This retrieves the hex presentation of the RGB values for the given color
 * preset and saves them. If the current mode is equal to #MS_normalMode and
 * the preset written was equal to the one currently being used, it will be
 * applied immediately.
 *
 * @see uart_protocol_command_callback_t
 * @see uart_protocol_command_buffer
 * @see hexStrToUint8()
 * @see UserEepromParams::colorPresets
 * @see uart_protocol_ok()
 */
static void _preset_write(uint8_t argc, char* argv[])
{

    uint8_t preset;
    color_rgb_t color;
    bool status;

    preset = hexStrToUint8(argv[1], &status);

    if (!status || preset >= UI_COLOR_PRESET_COUNT) {

        uart_protocol_error();

        return;

    }

    color.red = hexStrToUint8(argv[2], &status);

    if (!status) {

        uart_protocol_error();

        return;

    }

    color.green = hexStrToUint8(argv[3], &status);

    if (!status) {

        uart_protocol_error();

        return;

    }

    color.blue = hexStrToUint8(argv[4], &status);

    if (!status) {

        uart_protocol_error();

        return;

    }

    (&(wcEeprom_getData()->userParams))->colorPresets[preset] = color;

    wcEeprom_writeback(&wcEeprom_getData()->userParams.colorPresets[preset],
        sizeof(wcEeprom_getData()->userParams.colorPresets[preset]));

    if (preset == (&(wcEeprom_getData()->userParams))->curColorProfile) {

        if (user_get_current_menu_state() == MS_normalMode) {

            addState(MS_normalMode, &preset);

        }

    }

    uart_protocol_ok();

}

#endif /* (ENABLE_RGB_SUPPORT == 1) */

/**
 * @brief Puts out the currently used time
 *
 * This puts out the hex presentation of the currently used time (hour,
 * minutes, seconds). The time itself is retrieved directly from the RTC.
 * If there is an error while trying to communicate with the RTC, an error
 * message will be output.
 *
 * @see uart_protocol_command_callback_t
 * @see datetime_t
 * @see i2c_rtc_read()
 * @see uart_protocol_output_args_hex()
 * @see uart_protocol_error()
 */
static void _time_get(uint8_t argc, char* argv[])
{

    datetime_t datetime;

    if (i2c_rtc_read(&datetime)) {

        uart_protocol_output_args_hex(3, datetime.hh, datetime.mm, datetime.ss);

        return;

    }

    uart_protocol_error();

}

/**
 * @brief Sets the currently used time
 *
 * This sets the currently used time (hour, minutes, seconds). It writes the
 * given arguments directly to the RTC. In order for the date to stay the same,
 * it reads the date first and overwrites only the actual time values. Any
 * error while interacting with the RTC will result in an error message,
 * otherwise a success message will be output. Once the time was successfully
 * written to the RTC, the time is also passed over to the user module making
 * sure it will be applied immediately to the display itself.
 *
 * @see uart_protocol_command_callback_t
 * @see datetime_t
 * @see i2c_rtc_read()
 * @see i2c_rtc_write()
 * @see user_setNewTime()
 * @see uart_protocol_ok()
 * @see uart_protocol_error()
 */
static void _time_set(uint8_t argc, char* argv[])
{

    uint8_t time[3];
    uint8_t i;

    for (i = 1; i < 4; i++) {

        bool status;

        time[i - 1] = hexStrToUint8(argv[i], &status);

        if (!status) {

           break;

        }

    }

    if (i != 4) {

        uart_protocol_error();

        return;

    }

    datetime_t datetime;

    if (!i2c_rtc_read(&datetime)) {

        uart_protocol_error();

        return;

    }

    datetime.hh = time[0];
    datetime.mm = time[1];
    datetime.ss = time[2];

    if (!i2c_rtc_write(&datetime)) {

        uart_protocol_error();

        return;

    }

    user_setNewTime(&datetime);
    uart_protocol_ok();

}

/**
 * @brief Puts out the currently used date
 *
 * This puts out the hex presentation of the currently used date (day, month
 * year, weekday). The date itself is retrieved directly from the RTC. If there
 * is an error while trying to communicate with the RTC, an error message will
 * be output.
 *
 * @see uart_protocol_command_callback_t
 * @see datetime_t
 * @see i2c_rtc_read()
 * @see uart_protocol_output_args_hex()
 * @see uart_protocol_error()
 */
static void _date_get(uint8_t argc, char* argv[])
{

    datetime_t datetime;

    if (i2c_rtc_read(&datetime)) {

        uart_protocol_output_args_hex(4, datetime.DD, datetime.MM, datetime.YY,
            datetime.WD);

        return;

    }

    uart_protocol_error();

}

/**
 * @brief Sets the currently used date
 *
 * This sets the currently used date (day, month, year, weekday). It writes the
 * given arguments directly to the RTC. In order for the time to stay the same,
 * it reads the time first and overwrites only the actual date values. Any
 * error while interacting with the RTC will result in an error message,
 * otherwise a success message will be output.
 *
 * @see uart_protocol_command_callback_t
 * @see datetime_t
 * @see i2c_rtc_read()
 * @see i2c_rtc_write()
 * @see uart_protocol_ok()
 * @see uart_protocol_error()
 *
 * @todo Check whether user_setNewTime() is necessary or would even make sense
 */
static void _date_set(uint8_t argc, char* argv[])
{

    uint8_t date[4];
    uint8_t i = 0;

    for (i = 1; i < 5; i++) {

        bool status;

        date[i - 1] = hexStrToUint8(argv[i], &status);

        if (!status) {

           break;

        }

    }

    if (i != 5) {

        uart_protocol_error();

        return;

    }

    datetime_t datetime;

    if (!i2c_rtc_read(&datetime)) {

        uart_protocol_error();

        return;

    }

    datetime.DD = date[0];
    datetime.MM = date[1];
    datetime.YY = date[2];
    datetime.WD = date[3];

    if (!i2c_rtc_write(&datetime)) {

        uart_protocol_error();

        return;

    }

    uart_protocol_ok();

}

/**
 * @brief Defines the type of each entry within #uart_protocol_commands
 *
 * @see uart_protocol_commands
 */
typedef struct
{

    /**
     * @brief Char array holding the command
     *
     * @see UART_PROTOCOL_COMMAND_MAX_LENGTH
     */
    char command[UART_PROTOCOL_COMMAND_MAX_LENGTH];

    /**
     * @brief Number of arguments
     *
     * Defines the number of arguments this command is expected to have. The
     * maximum amount of arguments is defined by
     * #UART_PROTOCOL_COMMAND_BUFFER_MAX_ARGS.
     *
     * @warning If the received command does not provide exactly the amount of
     * arguments as specified here, the command won't be recognized.
     *
     * @see UART_PROTOCOL_COMMAND_BUFFER_MAX_ARGS
     */
    uint8_t arguments;

    /**
     * @brief Function to call for this command
     *
     * @see uart_protocol_command_callback_t
     */
    uart_protocol_command_callback_t callback;

} uart_protocol_command_t;

/**
 * @brief Definition of all available commands
 *
 * This defines all of the commands that can be detected along with the
 * appropriate callback function.
 *
 * @note Keep in mind that this table is accessed iteratively, so it might make
 * sense to order the entries appropriately.
 *
 * @see uart_protocol_command_t
 * @see uart_protocol_handle()
 */
static uart_protocol_command_t uart_protocol_commands[] = {

    {"i", 1, _ir_user_command},

    {"v", 0, _version},

    {"k", 0, _keepalive},

    {"r", 0, _reset},
    {"f", 0, _factory_reset},

    {"lb", 0, _ldr_brightness},

    #if (ENABLE_RGB_SUPPORT == 1)

        {"cr", 0, _color_read},
        {"cw", 3, _color_write},

        {"pn", 0, _preset_number},
        {"pa", 0, _preset_active},
        {"ps", 1, _preset_set},
        {"pr", 1, _preset_read},
        {"pw", 4, _preset_write},

    #endif /* (ENABLE_RGB_SUPPORT == 1) */

    {"tg", 0, _time_get},
    {"ts", 3, _time_set},
    {"dg", 0, _date_get},
    {"ds", 4, _date_set},

};

/**
 * @brief Tokenizes the command buffer
 *
 * This tokenizes #uart_protocol_command_buffer using the space character as
 * delimiter and returns an array of char pointers to the beginning of each
 * token. The first pointer within this array points to the command itself.
 *
 * @param argv Pointer to char array for storing the found char pointers
 *
 * @return The number of tokens found (including the command itself)
 *
 * @see uart_protocol_command_buffer
 * @see UART_PROTOCOL_COMMAND_BUFFER_MAX_ARGS
 */
static uint8_t uart_protocol_tokenize_command_buffer(char* argv[])
{

    int arg = 0;
    char *token = strtok(uart_protocol_command_buffer, " ");

    while (token != NULL && arg < UART_PROTOCOL_COMMAND_BUFFER_MAX_ARGS) {

        argv[arg++] = token;

        token = strtok(NULL, " ");

    }

    return arg;

}

/**
 * @brief Processes the UART protocol
 *
 * This retrieves the input received by the UART hardware and puts it into the
 * {@link #uart_protocol_command_buffer command buffer}. Once a
 * {@link UART_PROTOCOL_INPUT_EOL EOL character} has been detected, the
 * command is tokenized and the appropriate callback function will be executed.
 *
 * If the command could not be detected successfully an error message will be
 * output.
 *
 * If logging is enabled (#LOG_UART_PROTOCOL) some debugging information will
 * be output, too.
 *
 * @warning This function should be called on a regular basis. It is not time
 * critical at all, but once the incoming buffer is full, data might be lost.
 *
 * @see UART_PROTOCOL_INPUT_EOL
 * @see uart_protocol_tokenize_command_buffer()
 * @see uart_protocol_commands
 * @see uart_protocol_error()
 * @see LOG_UART_PROTOCOL
 */
void uart_protocol_handle()
{

    static uint8_t buffer_index = 0;

    char c;

    while (uart_getc_nowait(&c)) {

        // Check whether EOL was received, triggering the command detection
        if (c == UART_PROTOCOL_INPUT_EOL) {

            // Preparing buffer for processing and next iteration
            uart_protocol_command_buffer[buffer_index] = '\0';
            buffer_index = 0;

            #if (LOG_UART_PROTOCOL == 1)

                uart_puts_P("Command: ");
                uart_puts(uart_protocol_command_buffer);
                uart_putc('\n');

            #endif

            char* argv[UART_PROTOCOL_COMMAND_BUFFER_MAX_ARGS];
            uint8_t argc;

            argc = uart_protocol_tokenize_command_buffer(argv);

            // Check whether at least a single command was detected
            if (argc != 0) {

                uint8_t j= sizeof(uart_protocol_commands) / sizeof(uart_protocol_command_t);

                for (uint8_t i = 0; i < j; i++) {

                    bool compare = strncmp(uart_protocol_commands[i].command,
                        argv[0], strlen(uart_protocol_commands[i].command));

                    if (compare == 0
                        && (argc - 1) == uart_protocol_commands[i].arguments) {

                        uart_protocol_commands[i].callback(argc, argv);

                        return;

                    }

                }

            }

            uart_protocol_error();

            return;

        }

        // Check whether there is enough space in buffer to put character in
        if (buffer_index < UART_PROTOCOL_COMMAND_BUFFER_SIZE - 1) {

            #if (LOG_UART_PROTOCOL == 1)

                char str[3];

                uint8ToStrLessOneHundred(buffer_index, str);

                uart_puts_P("Buffer index: ");
                uart_puts(str);
                uart_putc('\n');

                uart_puts_P("Character: ");
                uart_putc(c);
                uart_putc('\n');

            #endif

            // Put character into buffer
            uart_protocol_command_buffer[buffer_index++] = c;

        }

    }

}
