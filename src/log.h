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
 * @file log.h
 * @brief Provides means for other modules to output logging information
 *
 * This allows modules enumerated within {@link #log_module_t} to output
 * logging information via the UART interface.
 *
 * Messages processed by this module are processed in a similar way to
 * `printf()`, i.e. specifiers in the format string are replaced by additional
 * arguments. This makes it possible for messages to be built dynamically,
 * making it more flexible and powerful than a static approach.
 *
 * Furthermore there is the notion of log levels. You can specify a log level
 * for each module individually. Only messages that pass this criterion are
 * actually being processed, everything else is silently dropped without
 * wasting too much cycles.
 *
 * @see log.c
 */

#ifndef _WC_LOG_H_
#define _WC_LOG_H_

#include <avr/pgmspace.h>

#include <stdbool.h>

/**
 * @brief Enumeration of modules able to output logging information
 *
 * @note Make sure that this definition is in accordance with
 * {@link #log_module_names}.
 *
 * @see log_module_names
 */
typedef enum {

    LOG_MODULE_LOG = 0,
    LOG_MODULE_LDR,
    LOG_MODULE_BRIGHTNESS,

    LOG_MODULE_COUNT

} log_module_t;

/**
 * @brief Enumeration of available log levels
 *
 * The log level can be specified for each {@link log_module_t module}
 * individually using {@link log_set_level()}.
 *
 * @note The order of the elements enumerated here defines their relevance. A
 * higher log level includes all of the log levels below.
 *
 * @see log_set_level()
 */
typedef enum {

    LOG_LEVEL_NONE = 0,
    LOG_LEVEL_ERROR,
    LOG_LEVEL_WARN,
    LOG_LEVEL_INFO,
    LOG_LEVEL_DEBUG,
    LOG_LEVEL_ALL,

    LOG_LEVEL_COUNT

} log_level_t;

/**
 * @brief Prefix for any output generated by this module
 */
#define LOG_OUTPUT_PREFIX "LOG: "

/**
 * @brief Separator between prefix and actual message
 */
#define LOG_OUTPUT_SEPARATOR ": "

/**
 * @brief EOL marker for any log messages generated by this module
 *
 * To make it compatible with most of the platforms out there, this is defined
 * as `\r\n` by default.
 */
#define LOG_OUTPUT_EOL "\r\n"

void log_enable();
void log_disable();
bool log_is_enabled();

void log_set_level(log_module_t module, log_level_t level);

void log_output(log_module_t module, log_level_t level, const char* fmt, ...);
void log_output_p(log_module_t module, log_level_t level, const char* fmt, ...);

/**
 * @brief Helper macro to put format string into program space automatically
 *
 * @note This is based on additional functionality provided by GCC, as
 * documented [here][1].
 *
 * [1]: https://gcc.gnu.org/onlinedocs/cpp/Variadic-Macros.html
 *
 * @see log_output_p
 */
#define log_output_P(module, level, fmt, ...) log_output_p(module, level, PSTR(fmt), ##__VA_ARGS__)

#endif /* _WC_LOG_H_ */
