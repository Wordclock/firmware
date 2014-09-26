/*
 * Copyright (C) 2014 Karol Babioch <karol@babioch.de>
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
 * @file log.c
 * @brief Implements functionality declared in log.h
 *
 * This implements the functionality declared in log.h. The format string
 * processing is done manually instead of relying upon `printf()` and/or
 * `sprintf()` in order to save serious amounts of program space, as only a
 * small subset of the commands is needed anyway.
 *
 * As format string specifiers require the support of a variable amount of
 * arguments, this module makes use of functionality provided by `stdarg.h`.
 *
 * The output itself is implemented by functions declared in {@link uart.h}.
 *
 * @see log.h
 * @see uart.h
 */

#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "base.h"
#include "config.h"
#include "log.h"
#include "uart.h"

const char const log_module_name_0[] PROGMEM = "LOG";
const char const log_module_name_1[] PROGMEM = "LDR";
const char const log_module_name_2[] PROGMEM = "BRT";
const char const log_module_name_3[] PROGMEM = "MAIN";
const char const log_module_name_4[] PROGMEM = "UARTP";
const char const log_module_name_5[] PROGMEM = "DATE";
const char const log_module_name_6[] PROGMEM = "IR";
const char const log_module_name_7[] PROGMEM = "PREFS";

/**
 * @brief Names of modules able to output logging information
 *
 * These are the corresponding names for items enumerated within
 * {@link #log_module_t}. It is used as a prefix to messages of any given
 * {@link #log_module_t module}.
 *
 * @note Make sure that there is a string for each element
 * {@link #log_module_t}.
 *
 * @note Try to keep this strings short but unique.
 *
 * @see log_module_t
 */
PGM_P const log_module_names[] PROGMEM = {

    log_module_name_0,
    log_module_name_1,
    log_module_name_2,
    log_module_name_3,
    log_module_name_4,
    log_module_name_5,
    log_module_name_6,
    log_module_name_7,

};

const char const log_level_name_0[] PROGMEM = "NONE";
const char const log_level_name_1[] PROGMEM = "ERROR";
const char const log_level_name_2[] PROGMEM = "WARN";
const char const log_level_name_3[] PROGMEM = "INFO";
const char const log_level_name_4[] PROGMEM = "DEBUG";
const char const log_level_name_5[] PROGMEM = "ALL";

/**
 * @brief Names of available log levels
 *
 * These are the corresponding names for items enumerated within
 * {@link #log_level_t}.
 *
 * @note Make sure that there is a string for each element
 * {@link #log_level_t}.
 *
 * @see log_level_t
 */
PGM_P const log_level_names[] PROGMEM = {

    log_level_name_0,
    log_level_name_1,
    log_level_name_2,
    log_level_name_3,
    log_level_name_4,
    log_level_name_5,

};

/**
 * @brief Maximum length of log format strings
 */
#define LOG_FORMAT_MAX_STRING_LENGTH 80

/**
 * @brief Global logging enable flag
 *
 * This flag determines whether logging messages will be output at all. Unless
 * it is set to true, messages passed to this module are dropped silently.
 *
 * @see log_enable()
 * @see log_disable()
 */
static bool log_enabled = false;

/**
 * @brief Contains log level for each available module
 *
 * This array stores the {@link #log_level_t log level} for each
 * {@link #log_module_t available module} individually.
 *
 * @see log_level_t
 * @see log_module_t
 * @see log_set_level()
 */
static log_level_t log_level[LOG_MODULE_COUNT];

/**
 * @brief Puts the character into the UART transmission buffer
 *
 * This puts the given character into the UART transmission buffer, queuing it
 * up for output. In case the buffer is full, _FDEV_ERR is returned. In case
 * the character was queued up, 0 is returned, indicating success. On top of
 * that the output buffer will be {@link uart_flush_output() flushed} whenever
 * the given character is `\n`.
 *
 * @todo Extract newline character into constant, possibly within UART module
 * @todo Consider flushing output within UART module
 *
 * @param c Character to output
 * @param stream For compatibility reasons with FDEV_SETUP_STREAM()
 *
 * @return 0 in case output was written, _FDEV_ERR else
 *
 * @see uart_putc()
 * @see uart_flush_output()
 */
static int log_putc(char c, FILE* stream)
{

    // Flush output buffer in case of a newline
    if (c == '\n') {

        uart_flush_output();

    }

    bool result = uart_putc(c);

    // Output error in case output buffer was full
    if (!result) {

        return _FDEV_ERR;

    }

    return 0;

}

/**
 * @brief Log output stream used throughout this module
 *
 * This registers a new stream, which is used throughout the module for putting
 * out content. It opens the stream for write intents only, and forwards any
 * output to {@link log_putc()}.
 *
 * @see uart_putc()
 */
static FILE logout = FDEV_SETUP_STREAM(log_putc, NULL, _FDEV_SETUP_WRITE);

/**
 * @brief Initializes the logging module
 *
 * This will enable the logging globally if the configuration is set up
 * appropriately.
 *
 * @see log_enable()
 * @see LOG_ENABLE_DEFAULT
 */
void log_init()
{

    #if (LOG_ENABLE_DEFAULT == 1)

        log_enable();

    #endif

}

/**
 * @brief Enables the logging functionality globally
 *
 * This sets the {@link #log_enabled} flag to true and thus enables the logging
 * functionality globally.
 *
 * @see log_enabled
 */
void log_enable()
{

    log_enabled = true;

}

/**
 * @brief Disables the logging functionality globally
 *
 * This sets the {@link #log_enabled} flag to false and thus disables the
 * logging functionality globally.
 *
 * @see log_enabled
 */
void log_disable()
{

    log_enabled = false;

}

/**
 * @brief Returns whether logging is currently enabled on a global basis
 *
 * @return True if logging is enabled globally, false otherwise
 *
 * @see log_enabled
 */
bool log_is_enabled()
{

    return log_enabled;

}

/**
 * @brief Sets log level for a particular module
 *
 * @param module Module to set the level for
 * @param level Level to set for the module
 *
 * @see log_level
 * @see log_module_t
 * @see log_level_t
 */
void log_set_level(log_module_t module, log_level_t level)
{

    log_level[module] = level;

}

/**
 * @brief Gets log level for a particular module
 *
 * @param module Module to get the level for
 *
 * @see log_level
 * @see log_module_t
 * @see log_level_t
 */
log_level_t log_get_level(log_module_t module)
{

    return log_level[module];

}

/**
 * @brief Outputs the log prefix for given module and logging level
 *
 * This outputs a prefix with a predefined format for the given module and
 * level. {@link #LOG_OUTPUT_PREFIX} is used to mark the output as logging
 * message, {@link #LOG_OUTPUT_SEPARATOR} is used to separate the components
 * from each other and the message itself.
 *
 * @param module The module to output a message for
 * @param level The logging level to output the message with
 *
 * @see log_module_t
 * @see log_level_t
 * @see LOG_OUTPUT_PREFIX
 * @see LOG_OUTPUT_SEPARATOR
 */
static void log_output_prefix(log_module_t module, log_level_t level) {

    uart_puts_P(LOG_OUTPUT_PREFIX);
    uart_puts_p((PGM_P)pgm_read_word(&(log_module_names[module])));
    uart_puts_P(LOG_OUTPUT_SEPARATOR);
    uart_puts_p((PGM_P)pgm_read_word(&(log_level_names[level])));
    uart_puts_P(LOG_OUTPUT_SEPARATOR);

}

/**
 * @brief Outputs the EOL string as defined by {@link #LOG_OUTPUT_EOL}
 *
 * @see LOG_OUTPUT_EOL
 */
static void log_output_eol()
{

    uart_puts_P(LOG_OUTPUT_EOL);

}

/**
 * @brief Outputs a message as specified by the format string
 *
 * This function iterates over over the format string on a character basis and
 * examines it for specifiers. It replaces those specifiers with the arguments
 * provided by <code>ap</code>.
 *
 * Currently the following specifiers are supported:
 *
 * - %c: Character
 * - %s: String
 * - %u: uint8_t with values below 100 -> {@link uint8ToStrLessOneHundred()}
 * - %U: uint8_t with the full range of values -> {@link uint8ToStr()}
 * - %h: uint8_t in its hex representation -> {@link uint8ToHexStr()}
 * - %H: uint16_t in its hex representation -> {@link uint16ToHexStr()}
 * - %%: Escape sequence for `%`
 *
 * Invalid specifiers are simply ignored.
 *
 * @param fmt Format string describing logging output
 * @param va List with arguments for specifiers within format string
 *
 * @see uart_puts_p()
 * @see uart_puts_P()
 * @see uart_putc()
 * @see uart_puts()
 * @see uint8ToStrLessOneHundred()
 * @see uint8ToStr()
 * @see uint8ToHexStr()
 * @see uint16ToHexStr()
 */
static void log_outputf(const char* fmt, va_list ap)
{

    // Wait for UART output buffer to be empty
    uart_flush_output();

    // Keeps track of length of format string
    uint8_t length = 0;

    // Iterate over format string
    while (*fmt && length++ < LOG_FORMAT_MAX_STRING_LENGTH) {

        // Check for format specifier
        if (*fmt == '%') {

            // Analyze specifier
            switch(*++fmt) {

                // Character
                case 'c':

                    {

                        // Retrieve character and output it
                        char c = (char)va_arg(ap, int);
                        uart_putc(c);

                    }

                    break;

                // String
                case 's':

                    {

                        // Retrieve string and output it
                        const char* s = va_arg(ap, const char*);
                        uart_puts(s);

                    }

                    break;

                // uint8_t smaller than one hundred (takes up 2 digits)
                case 'u':

                    {

                        // Convert into string and output it
                        char buffer[3];
                        uint8_t u = (uint8_t)va_arg(ap, int);
                        uint8ToStrLessOneHundred(u, buffer);
                        uart_puts(buffer);

                    }

                    break;

                // uint8_t bigger than one hundred (takes up 3 digits)
                case 'U':

                    {

                        // Convert into string and output it
                        char buffer[4];
                        uint8_t u = (uint8_t)va_arg(ap, int);
                        uint8ToStr(u, buffer);
                        uart_puts(buffer);

                    }

                    break;

                // uint8_t as hex
                case 'h':

                    {

                        // Convert into hex string and output it
                        char buffer[3];
                        uint8_t u = (uint8_t)va_arg(ap, int);
                        uint8ToHexStr(u, buffer);
                        uart_puts(buffer);

                    }

                    break;

                // uint16_t as hex
                case 'H':

                    {

                        // Convert into hex string and output it
                        char buffer[5];
                        uint16_t u = va_arg(ap, uint16_t);
                        uint16ToHexStr(u, buffer);
                        uart_puts(buffer);

                    }

                    break;

                // Escape sequence for %
                case '%':

                    uart_putc('%');
                    break;

                // Invalid specifiers
                default:

                    // This makes sure that the invalid specifier is output
                    continue;

            }

        // No format specifier
        } else {

            // Simply output character
            uart_putc(*fmt);

        }

        // Handle next character
        fmt++;

    }

}

/**
 * @brief Outputs a log message
 *
 * This is essentially a wrapper around {@link #log_outputf()}. It uses
 * functionality from `<stdarg.h>` to retrieve a `va_list` and passes
 * everything over to generate the actual output.
 *
 * It makes also sure that the output is generated in the correct format,
 * {@link #log_output_prefix()} it and putting {@link #log_output_eol()} at
 * the end.
 *
 * @param module Module to generate logging output for
 * @param level Log level to generate output with
 * @param fmt Format string describing logging output
 * @param ... Arguments for specifiers within format string
 *
 * @see log_outputf()
 */
void log_output(log_module_t module, log_level_t level, const char* fmt, ...)
{

    // Check whether output is enabled (globally and for specific module)
    if (!log_enabled || level > log_level[module]) {

        return;

    }

    // Output prefix, including module name and separator
    log_output_prefix(module, level);

    va_list va;
    va_start(va, fmt);
    log_outputf(fmt, va);
    va_end(va);

    // Output EOL
    log_output_eol();

}

/**
 * @brief Outputs a log message stored in program space
 *
 * This is essentially a wrapper around {@link #log_outputf()} for format
 * strings stored in program space. After copying over the log format string
 * from program space into a buffer, is uses functionality from `<stdarg.h>` to
 * retrieve a `va_list` and passes everything over to generate the actual
 * output.
 *
 * It makes also sure that the output is generated in the correct format,
 * {@link #log_output_prefix()} it and putting {@link #log_output_eol()} at
 * the end.
 *
 * @param module Module to generate logging output for
 * @param level Log level to generate output with
 * @param fmt Format string (from program space) describing logging output
 * @param ... Arguments for specifiers within format string
 *
 * @see log_outputf()
 */
void log_output_p(log_module_t module, log_level_t level, PGM_P fmt, ...)
{

    // Check whether output is enabled (globally and for specific module)
    if (!log_enabled || level > log_level[module]) {

        return;

    }

    // Output prefix, including module name and separator
    log_output_prefix(module, level);

    char buffer[LOG_FORMAT_MAX_STRING_LENGTH];
    strncpy_P(buffer, fmt, LOG_FORMAT_MAX_STRING_LENGTH);

    va_list va;
    va_start(va, fmt);
    log_outputf(buffer, va);
    va_end(va);

    // Output EOL
    log_output_eol();

}

/**
 * @brief Outputs a log message by invoking a callback function
 *
 * This can be used in cases when a formatted string is not powerful enough and
 * a completely custom message and/or action needs to be generated/taken. The
 * {@link #log_output_prefix() prefix} and {@link #log_output_eol() EOL}
 * strings are output automatically, but everything in between is up to the
 * callback function itself.
 *
 * @note Callback functions are expected to use the log_output_put* function
 * family to output content.
 *
 * @param module Module to generate logging output for
 * @param level Log level to generate output with
 * @param callback Callback function responsible for generating output
 *
 * @see log_output_callback_t
 */
void log_output_callback(log_module_t module, log_level_t level, log_output_callback_t callback)
{

    // Check whether output is enabled (globally and for specific module)
    if (!log_enabled || level > log_level[module]) {

        return;

    }

    // Wait for UART output buffer to be empty
    uart_flush_output();

    log_output_prefix(module, level);
    callback(&logout);
    log_output_eol();

}
