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
 * @file pwm.c
 * @brief Implements the header declared in pwm.h
 *
 * Each channel (red, green, blue) is controlled separately and is attached
 * to a different output compare pin of the microcontroller (Red = OC0A,
 * Green = OC0B, Blue = OC2B). This means that actually two timers, namely
 * Timer/Counter0 and Timer/Counter2 are used.
 *
 * In case the software is compiled without support for RGB LEDs
 * (ENABLE_RGB_SUPPORT == 0), Timer/Counter2 will be left untouched.
 *
 * For details about the various counters and registers involved it might be
 * useful to take a look at [1].
 *
 * [1]: http://www.atmel.com/images/doc2545.pdf
 *
 * @see pwm.h
 */

#include <inttypes.h>
#include <avr/io.h>
#include <util/delay.h>
#include <avr/pgmspace.h>
#include <stdbool.h>
#include "wceeprom.h"

#include "uart.h"
#include "base.h"
#include "config.h"
#include "pwm.h"
#include "ports.h"

/**
 * @brief Port and pin definition of the line in control of the red channel
 *
 * @see ports.h
 */
#define PWM_RED PORTD, 6

/**
 * @brief Port and pin definition of the line in control of the green channel
 *
 * @see ports.h
 */
#define PWM_GREEN PORTD, 5

/**
 * @brief Port and pin definition of the line in control of the blue channel
 *
 * @see ports.h
 */
#define PWM_BLUE PORTD, 3

#if (MAX_PWM_STEPS == 32)

    /**
     * @brief Table containing values to generate PWM signal
     *
     * This table is used to generate a PWM signal. It contains 32 values,
     * making it possible to implement 32 different steps of brightness.
     * These values are precalculated to fit the logarithmic nature of the
     * human eye as close as possible.
     *
     * @see pwm_set_brightness()
     * @see pwm_set_color_step()
     */
    const uint8_t pwm_table[MAX_PWM_STEPS] PROGMEM =
        {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 12, 14, 16, 18, 21, 24, 28, 32, 37,
        42, 48, 55, 63, 72, 83, 96, 111, 129, 153, 182, 216, 255};

#elif (MAX_PWM_STEPS == 64)

    /**
     * @brief Table containing 64 values to generate PWM signal
     *
     * This table is used to generate a PWM signal. It contains 64 values,
     * making it possible to implement 64 different steps of brightness.
     * These values are precalculated to fit the logarithmic nature of the
     * human eye as close as possible.
     *
     * @see pwm_set_brightness()
     * @see pwm_set_color_step()
     */
    const uint8_t pwm_table[MAX_PWM_STEPS] PROGMEM =
        {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19,
        21, 22, 23, 25, 26, 27, 29, 30, 32, 33, 35, 36, 38, 40, 42, 44, 46,
        48, 51, 53, 56, 58, 61, 63, 66, 69, 72, 75, 78, 81, 85, 89, 93, 98,
        103, 109, 116, 125, 135, 146, 158, 171, 189, 214, 255};

#else

    /*
     * For now only 32 and 64 bit step sizes are allowed, see pwm_table.
     */
    #error Unknown PWM step size

#endif

/**
 * @brief Indicates whether PWM is currently turned on
 *
 * This can be used to control whether or not the generation of the PWM
 * signal(s) is enabled. It will affect most of the functions defined here,
 * and should only be accessed using the function pwm_on(), pwm_off() and
 * pwm_on_off().
 *
 * @see pwm_on()
 * @see pwm_off()
 * @see pwm_on_off()
 */
static bool pwm_is_on;

/**
 * @brief Current base PWM index
 *
 * Pointing to value within pwm_table and representing the current base to be
 * used for various calculations within this module. It ranges from 0 to
 * MAX_PWM_STEPS - 1 and points to one value within the PWM table, see
 * pwm_table.
 *
 * @see MAX_PWM_STEPS
 * @see pwm_table
 * @see pwm_set_brightness()
 * @see pwm_set_base_brightness_step()
 * @see pwm_step_up_brightness()
 * @see pwm_step_down_brightness()
 * @see pwm_modifyLdrBrightness2pwmStep()
 */
static uint8_t base_pwm_idx;

/**
 * @brief Contains the current value for generating the PWM signal
 *
 * This contains the value pointed to by base_pwm_idx + offset_pwm_idx from
 * pwm_table. As the table is stored in program space the value will be read
 * from the table and stored within this variable for convenience and/or
 * performance.
 *
 * This value is then used to calculate the value for the various compare
 * registers (OCR0A, OCR0B, OCR2B), which in return is responsible for the
 * shape of the PWM signal itself.
 *
 * @see base_pwm_idx
 * @see pwm_table
 * @see pwm_set_brightness()
 * @see pwm_set_colors()
 * @see pwm_lock_brightness_val()
 */
static uint8_t brightness_pwm_val;

/**
 * @brief Indicates whether brightness can actually be changed
 *
 * This variable is used by pwm_lock_brightness_val() and
 * pwm_release_brightness(). If "locked" the brightness is set to a fixed value
 * and can't be changed by pwm_set_brightness() anymore.
 *
 * @see pwm_lock_brightness_val()
 * @see pwm_release_brightness()
 * @see pwm_on_off()
 */
static bool brightness_lock;

/**
 * @brief Macro for getting the user defined offset for the PWM index
 *
 * This retrieves the user defined offset for the PWM index from the EEPROM.
 * It makes use of wcEeprom_getData(), which is part of the wceeprom.h
 *
 * This offset is added to and/or subtracted from base_pwm_idx.
 *
 * @see wcEeprom_getData()
 * @see wceeprom.h
 * @see base_pwm_idx
 */
#define offset_pwm_idx (wcEeprom_getData()->pwmParams.brightnessOffset)

/**
 * @brief Macro allowing access to occupancy backed by EEPROM
 *
 * @see PwmEepromParams::occupancy
 */
#define g_occupancy (wcEeprom_getData()->pwmParams.occupancy)

/**
 * @brief Macro allowing access to brightness2pwmStep backed by EEPROM
 *
 * @see PwmEepromParams::brightness2pwmStep
 */
#define g_ldrBrightness2pwmStep \
    (wcEeprom_getData()->pwmParams.brightness2pwmStep)

/**
 * @brief Current index within g_ldrBrightness2pwmStep
 *
 * This is used to keep track of the current index within
 * g_ldrBrightness2pwmStep, which in turn is used to adjust the brightness
 * automatically to the value of the LDR.
 *
 * @see pwm_set_base_brightness_step()
 * @see pwm_modifyLdrBrightness2pwmStep()
 */
static uint8_t base_ldr_idx;

#if (ENABLE_RGB_SUPPORT == 1)

    /**
     * @brief Keeps track of the current PWM index for the red channel
     *
     * @see pwm_set_color_step()
     * @see pwm_get_color_step()
     */
    static uint8_t red_pwm_idx;

    /**
     * @brief Keeps track of the current PWM index for the green channel
     *
     * @see pwm_set_color_step()
     * @see pwm_get_color_step()
     */
    static uint8_t green_pwm_idx;

    /**
     * @brief Keeps track of the current PWM index for the blue channel
     *
     * @see pwm_set_color_step()
     * @see pwm_get_color_step()
     */
    static uint8_t blue_pwm_idx;

    /**
     * @brief Keeps track of the current PWM value for the red channel
     *
     * @see pwm_set_colors()
     * @see pwm_get_colors()
     */
    static uint8_t red_pwm;

    /**
     * @brief Keeps track of the current PWM value for the green channel
     *
     * @see pwm_set_colors()
     * @see pwm_get_colors()
     */
    static uint8_t green_pwm;

    /**
     * @brief Keeps track of the current PWM value for the blue channel
     *
     * @see pwm_set_colors()
     * @see pwm_get_colors()
     */
    static uint8_t blue_pwm;

#endif /* (ENABLE_RGB_SUPPORT == 1) */

/**
 * @brief Sets brightness according to a value from pwm_table
 *
 * The value is actually taken from pwm_table. The index is calculated by
 * adding up base_pwm_idx and offset_pwm_idx. If it is out of bounds either the
 * biggest or smallest value is taken. It will then set brightness_pwm_val
 * accordingly. Furthermore it will accommodate the colors using
 * pwm_set_colors() in case of RGB and by setting OCR0A in case of
 * monochromatic LEDs.
 *
 * @warning This will only work if the brightness is not locked.
 *
 * @see pwm_table
 * @see base_pwm_idx
 * @see offset_pwm_idx
 * @see brightness_pwm_val
 * @see pwm_set_colors()
 * @see brightness_lock()
 */
static void pwm_set_brightness()
{

    if (!brightness_lock) {

        int8_t pwm_idx = ((int8_t)base_pwm_idx) + offset_pwm_idx;

        if (pwm_idx < 0) {

            pwm_idx = 0;

        } else if (pwm_idx >= MAX_PWM_STEPS) {

            pwm_idx = MAX_PWM_STEPS - 1;

        }

        brightness_pwm_val = pgm_read_byte(pwm_table + pwm_idx);

    }

    #if (ENABLE_RGB_SUPPORT == 1)

        pwm_set_colors(red_pwm, green_pwm, blue_pwm);

    #else

        OCR0A = 255 - brightness_pwm_val;

    #endif

}

/**
 * @brief Initializes the PWM module
 *
 * This sets up the timers involved (Timer/Counter0 and Timer/Counter2) and the
 * appropriate ports. It should be called once to setup this module. Afterwards
 * this module can be used. For details take a look at [1], chapter 15 and 18.
 *
 * [1]: http://www.atmel.com/images/doc2545.pdf
 *
 * @see PWM_RED
 * @see PWM_GREEN
 * @see PWM_BLUE
 */
void pwm_init()
{

    PORT(PWM_RED) &= ~_BV(BIT(PWM_RED));
    DDR(PWM_RED) |= _BV(BIT(PWM_RED));

    #if (ENABLE_RGB_SUPPORT == 1)

        PORT(PWM_GREEN) &= ~_BV(BIT(PWM_GREEN));
        DDR(PWM_GREEN) |= _BV(BIT(PWM_GREEN));
        PORT(PWM_BLUE) &= ~_BV(BIT(PWM_BLUE));
        DDR(PWM_BLUE) |= _BV(BIT(PWM_BLUE));

        TCCR2A = _BV(WGM21) | _BV(WGM20);
        TCCR2B = _BV(CS21);

    #endif

    TCCR0A = _BV(WGM01) | _BV(WGM00);
    TCCR0B = _BV(CS01);

}

/**
 * @brief Switches the generation of the PWM signal on
 *
 * This sets up the timers involved (Timer/Counter0 and Timer/Counter2) in a
 * way that the generated PWM signal will be output on the appropriate pins.
 * For details take a look at [1], chapter 15 and 18.
 *
 * [1]: http://www.atmel.com/images/doc2545.pdf
 *
 * @see pwm_is_on
 * @see pwm_set_brightness()
 */
void pwm_on()
{

    TCCR0A |= _BV(COM0A1) | _BV(COM0A0);

    #if (ENABLE_RGB_SUPPORT == 1)

        TCCR0A |= _BV(COM0B1) | _BV(COM0B0);
        TCCR2A |= _BV(COM2B1) | _BV(COM2B0);

    #endif

    pwm_is_on = true;
    pwm_set_brightness();

}

/**
 * @brief Switches the generation of the PWM signal off
 *
 * This sets up the timers involved (Timer/Counter0 and Timer/Counter2) and
 * the appropriate pins in a way that the generation of the PWM signal is
 * disabled. For details take a look at [1], chapter 15 and 18.
 *
 * [1]: http://www.atmel.com/images/doc2545.pdf
 *
 * @see pwm_is_on
 */
void pwm_off()
{

    TCCR0A &= ~_BV(COM0A1) | _BV(COM0A0);
    PORT(PWM_RED) &= ~_BV(BIT(PWM_RED));

    #if (ENABLE_RGB_SUPPORT == 1)

        TCCR0A &= ~(_BV(COM0B1) | _BV(COM0B0));
        PORT(PWM_GREEN) &= ~_BV(BIT(PWM_GREEN));

        TCCR2A &= ~(_BV(COM2B1) | _BV(COM2B0));
        PORT(PWM_BLUE)  &= ~_BV(BIT(PWM_BLUE));

    #endif

    pwm_is_on = false;

}

/**
 * @brief Toggles the generation of the PWM signal on and/or off
 *
 * Depending on the current value of pwm_is_on this either turns the the
 * generation of the PWM signal on (pwm_on()) and/or off (pwm_off()).
 *
 * @see pwm_is_on
 * @see pwm_off()
 * @see pwm_on()
 */
void pwm_on_off()
{

    if (pwm_is_on) {

        pwm_off();

    } else {

        pwm_on();

    }

}

#if (ENABLE_RGB_SUPPORT == 1)

    /**
     * @brief Sets the RGB colors
     *
     * This sets the color by providing a value for each channel
     * (red, green ,blue) separately. Each parameter expects values from 0 up
     * to 255. It then sets the appropriate output compare registers of the
     * involved timers/counters. For details take a look at [1].
     *
     * [1]: http://www.atmel.com/images/doc2545.pdf
     *
     * @param red Value for the red channel (0 - 255).
     * @param green Value for the green channel (0 - 255).
     * @param blue Value for the blue channel (0 - 255).
     *
     * @see red_pwm
     * @see green_pwm
     * @see blue_pwm
     * @see pwm_get_color()
     */
    void pwm_set_colors(uint8_t red, uint8_t green, uint8_t blue)
    {

        uint16_t brightnessFactor = ((uint16_t)brightness_pwm_val) + 1;

        red_pwm   = red;
        green_pwm = green;
        blue_pwm  = blue;

        OCR0A = 255 - ((brightnessFactor * red) / 256);
        OCR0B = 255 - ((brightnessFactor * green) / 256);
        OCR2B = 255 - ((brightnessFactor * blue) / 256);

    }

    /**
     * @brief Gets the RGB colors
     *
     * This gets the color of each channel (red, green ,blue) and puts them
     * into the provided buffers.
     *
     * @param red Pointer to store value of the red channel.
     * @param green Pointer to store value of the green channel.
     * @param blue Pointer to store value of the blue channel.
     *
     * @see red_pwm
     * @see green_pwm
     * @see blue_pwm
     * @see pwm_set_color()
     */
    void pwm_get_colors(uint8_t* red, uint8_t* green, uint8_t* blue)
    {

        *red   = red_pwm;
        *green = green_pwm;
        *blue  = blue_pwm;

    }

    /**
     * @brief Sets the RGB colors by step values
     *
     * This sets the color by providing a step value for each channel
     * (red, green ,blue) separately. A step value is a value between 0
     * and MAX_PWM_STEPS, that is an index of pwm_table. Internally it makes
     * use of pwm_set_color() by reading the appropriate value from pwm_table.
     *
     * @param red Step for the red channel, range 0 - MAX_PWM_STEPS
     * @param green Step for the green channel, range 0 - MAX_PWM_STEPS
     * @param blue Step for the blue channel, range 0 - MAX_PWM_STEPS
     *
     * @see red_pwm_idx
     * @see green_pwm_idx
     * @see blue_pwm_idx
     * @see pwm_table
     * @see pwm_set_color()
     */
    void pwm_set_color_step(uint8_t red, uint8_t green, uint8_t blue)
    {

        if (red >= MAX_PWM_STEPS) {

            red = MAX_PWM_STEPS - 1;

        }

        if (green >= MAX_PWM_STEPS) {

            green = MAX_PWM_STEPS - 1;

        }

        if (blue >= MAX_PWM_STEPS) {

            blue = MAX_PWM_STEPS - 1;

        }

        red_pwm_idx   = red;
        green_pwm_idx = green;
        blue_pwm_idx  = blue;

        pwm_set_colors(pgm_read_byte(pwm_table + red),
                pgm_read_byte(pwm_table + green),
                pgm_read_byte (pwm_table + blue));

    }

    /**
     * @brief Gets the RGB color step values
     *
     * This gets the color step values of each channel (red, green ,blue) and
     * puts them into the provided buffers. A step value is a value between 0
     * and MAX_PWM_STEPS, that is an index of pwm_table.
     *
     * @param red Pointer to store step value of the red channel.
     * @param green Pointer to store step value of the green channel.
     * @param blue Pointer to store step value of the blue channel.
     *
     * @see red_pwm_idx
     * @see green_pwm_idx
     * @see blue_pwm_idx
     * @see pwm_table
     */
    void pwm_get_color_step(uint8_t* red, uint8_t* green, uint8_t* blue)
    {

        *red   = red_pwm_idx;
        *green = green_pwm_idx;
        *blue  = blue_pwm_idx;

    }

#endif /* (ENABLE_RGB_SUPPORT == 1) */

/**
 * @brief Sets the base brightness by step value
 *
 * This sets the base brightness by a step value. A step value is a value
 * between 0 and LDR2PWM_COUNT, that is an index of
 * g_ldrBrightness2pwmStep (usually 0 - 31). Internally it makes use of
 * pwm_set_brightness().
 *
 * @param pwm_idx Step value, range 0 - LDR2PWM_COUNT
 *
 * @see g_ldrBrightness2pwmStep
 * @see base_ldr_idx
 * @see base_pwm_idx
 * @see pwm_set_brightness()
 */
void pwm_set_base_brightness_step(uint8_t pwm_idx)
{

    if (pwm_idx >= LDR2PWM_COUNT) {

        pwm_idx = LDR2PWM_COUNT - 1;

    }

    base_ldr_idx = pwm_idx;
    base_pwm_idx = g_ldrBrightness2pwmStep[pwm_idx];
    pwm_set_brightness();

}

/**
 * @brief Increases the overall brightness
 *
 * This increases the brightness by a single step. Internally it increments
 * offset_pwm_idx and calls pwm_set_brightness() afterwards. This will
 * only work when the generation of the PWM signal is actually turned on
 * (pwm_is_on) and the increase will not lead to an out of bound condition,
 * meaning that the actual index would be greater than MAX_PWM_STEPS. This is
 * the analogy to pwm_step_down_brightness().
 *
 * @see offset_pwm_idx
 * @see pwm_set_brightness()
 * @see pwm_is_on
 * @see MAX_PWM_STEPS
 * @see pwm_step_down_brightness()
 */
void pwm_step_up_brightness()
{

    if (pwm_is_on && (base_pwm_idx + offset_pwm_idx + 1 < MAX_PWM_STEPS)) {

        offset_pwm_idx++;
        pwm_set_brightness();

    }

}

/**
 * @brief Decrease the overall brightness
 *
 * This decreases the brightness by a single step. Internally it decrements
 * offset_pwm_idx and calls pwm_set_brightness() afterwards. This will
 * only work when the generation of the PWM signal is actually turned on
 * (pwm_is_on) and the increase will not lead to an out of bound condition,
 * meaning that the actual index would be smaller than zero. This is the
 * analogy to pwm_step_up_brightness().
 *
 * @see offset_pwm_idx
 * @see pwm_set_brightness()
 * @see pwm_is_on
 * @see pwm_step_up_brightness()
 */
void pwm_step_down_brightness()
{

    if (pwm_is_on && (base_pwm_idx + offset_pwm_idx > 0)) {

        offset_pwm_idx--;
        pwm_set_brightness();

    }

}

/**
 * @brief Locks the brightness to a specific value
 *
 * This locks the brightness to a specific value, so it can't be changed by
 * pwm_set_brightness() anymore, unless the lock is released again by
 * pwm_release_brightness().
 *
 * @param val The value you want to lock the brightness to, range: 0 to 255
 *
 * @see pwm_set_brightness()
 * @see pwm_release_brightness()
 */
void pwm_lock_brightness_val(uint8_t val)
{

    brightness_lock = true;
    brightness_pwm_val = val;
    pwm_set_brightness();

}

/**
 * @brief Unlocks the previously locked brightness
 *
 * This unlocks the brightness. It should be called after the brightness
 * has actually been locked to a specific value using
 * pwm_lock_brightness_val() and it should be changed automatically once again.
 *
 * @see pwm_set_brightness()
 * @see pwm_lock_brightness_val()
 */
void pwm_release_brightness()
{

    brightness_lock = false;
    pwm_set_brightness();

}

#if (LOG_LDR2PWM == 1)

    /**
     * @brief Outputs values of g_ldrBrightness2pwmStep & g_occupancy via UART
     *
     * This outputs the values stored in the array
     * PwmEepromParams::brightness2pwmStep and the bitfield
     * PwmEepromParams::occupancy via UART. For g_occupancy an "x" is output
     * when the appropriate bit is set, otherwise " " is output.
     *
     * @see g_ldrBrightness2pwmStep
     * @see PwmEepromParams::occupancy
     * @see uart.h
     */
    void outputVals()
    {

        char buf[4];

        for (uint8_t i = 0; i < LDR2PWM_COUNT; i++) {

            uint8ToStr(g_ldrBrightness2pwmStep[i], buf);
            uart_puts(buf);

        }

        uart_putc('\n');

        for (uint8_t i = 0; i < LDR2PWM_COUNT; i++) {

            uart_putc(' ');

            if (g_occupancy & (((LDR2PWM_OCC_TYPE)1) << i)) {

                uart_putc('x');

            } else {

                uart_putc(' ');

            }

            uart_putc(' ');

        }

        uart_putc('\n');

    }

    /**
     * @brief Outputs a representation of values from 0 up to LDR2PWM_COUNT
     *
     * This function iterates from 0 to LDR2PWM_COUNT. It compares the current
     * counter value against "ind", "l" and "r". Whenever it is equal to
     * one of those parameters it will output an charachter (either "<", "^" or
     * ">"). Otherwise it will just output " ".
     *
     * This function can be used for debugging purposes within
     * modifyLdrBrightness2pwmStep()
     *
     * @param ind Index
     * @param l Left boundary
     * @param r Right boundary
     *
     * @see LDR2PWM_COUNT
     * @see uart.h
     * @see modifyLdrBrightness2pwmStep()
     */
    void outputPointer(uint8_t ind, uint8_t l, uint8_t r)
    {

        for (uint8_t i = 0; i < LDR2PWM_COUNT; i++) {

            if (l == i) {

                uart_putc('<');

            } else {

                uart_putc(' ');

            }

            if (ind == i) {

                uart_putc('^');

            } else {

                uart_putc(' ');

            }

            if (r == i) {

                uart_putc('>');

            } else {

                uart_putc(' ');

            }

        }

        uart_putc('\n');

    }

#endif

/**
 * @brief Gets the left and right boundary for the given index and value
 *
 * This iterates over g_occupancy (multiple times) and looks for both the left
 * and the right boundary.
 *
 * @param ind Index
 * @param val Value
 * @param left Pointer to variable that will hold the left boundary
 * @param right Pointer to variable that will hold the right boundary
 */
static void getBounds(uint8_t ind, uint8_t val, uint8_t* left, uint8_t* right)
{

    while (1) {

        if (ind > 0) {

            LDR2PWM_OCC_TYPE mask = ((LDR2PWM_OCC_TYPE)1) << (ind - 1);

            *left = ind - 1;

            while (!(mask & g_occupancy)) {

                mask >>= 1;
                --(*left);

            }

        } else {

            *left = 0;

        }

        if (g_ldrBrightness2pwmStep[*left] > val) {

            g_ldrBrightness2pwmStep[*left] = val;

            if (*left > 0) {

                g_occupancy &= ~(((LDR2PWM_OCC_TYPE)1) << *left);

            }

        } else {

            break;

        }

    }

    while (1) {

        if (ind < LDR2PWM_COUNT - 1) {

            LDR2PWM_OCC_TYPE mask = ((LDR2PWM_OCC_TYPE)1) << (ind + 1);

            *right = ind + 1;

            while (!(mask & g_occupancy)) {

                mask <<= 1;
                ++(*right);

            }

        } else {

            *right = LDR2PWM_COUNT - 1;

        }

        if (g_ldrBrightness2pwmStep[*right] < val) {

            g_ldrBrightness2pwmStep[*right] = val;

            if (*right < (LDR2PWM_COUNT - 1)) {

                g_occupancy &= ~(((LDR2PWM_OCC_TYPE)1) << *right);

            }

        } else {

            break;

        }

    }

}

/**
 * @brief Interpolates linearly between two values from g_ldrBrightness2pwmStep
 *
 * Both parameters are actually indexes for values from
 * g_ldrBrightness2pwmStep. The corresponding value for both of these indexes
 * is taken and a linear interpolation between these two points and their
 * values is performed, see [1] for details.
 *
 * [1]: https://en.wikipedia.org/wiki/Linear_interpolation
 *
 * @param left The left point
 * @param right The right point
 *
 * @see g_ldrBrightness2pwmStep
 */
static void interpolate(uint8_t left, uint8_t right)
{

    enum {

        SHIFT = 8,

    };

    uint8_t d = right - left;

    if (d > 1) {

        int8_t leftVal  = (int8_t)(g_ldrBrightness2pwmStep[left]);
        int8_t rightVal = (int8_t)(g_ldrBrightness2pwmStep[right]);

        int16_t l =
                (((int16_t)leftVal) << SHIFT) + (((uint16_t)1) << (SHIFT - 1));

        int16_t slope = ((int16_t)(rightVal - leftVal )) << SHIFT;
        slope /= d;

        for (uint8_t i = 1; i < d; i++) {

            l += slope;
            g_ldrBrightness2pwmStep[left + i] = (uint8_t)(l >> SHIFT);

        }

    }

}

/**
 * @brief Glues together various function defined in this module to be
 *   easily accessible by pwm_modifyLdrBrightness2pwmStep()
 *
 * This function puts the given value into g_ldrBrightness2pwmStep at the given
 * index and calculates the associated boundaries using getBounds(). It then
 * interpolates the values for both the left and the right boundary using
 * interpolate(). Furthermore it shifts g_occupancy by the amount defined by
 * "ind" to the left.
 *
 * When debugging is enabled (LOG_LDR2PWM) it will output the changes made
 * using outputVals() and outputPointer().
 *
 * @see pwm_modifyLdrBrightness2pwmStep()
 * @see g_ldrBrightness2pwmStep
 * @see getBounds()
 * @see interpolate()
 * @see g_occupancy
 * @see LOG_LDR2PWM
 * @see outputVals()
 * @see outputPointer()
 */
static void modifyLdrBrightness2pwmStep(uint8_t ind, uint8_t val)
{

    uint8_t left;
    uint8_t right;

    #if (LOG_LDR2PWM == 1)

        uart_puts_P("before\n");
        outputVals();

    #endif

    g_ldrBrightness2pwmStep[ind] = val;
    getBounds(ind, val, &left, &right);

    #if (LOG_LDR2PWM == 1)

        uart_puts_P("bounds\n");
        outputVals();
        outputPointer(ind, left, right);

    #endif

    interpolate(left, ind);
    interpolate(ind, right);

    g_occupancy |= (((LDR2PWM_OCC_TYPE)1) << ind);

    #if (LOG_LDR2PWM == 1)

        uart_puts_P("after\n");
        outputVals();

    #endif

}

/**
 * @brief Takes over current PWM settings in order to be considered by changes
 *   due to LDR measurements
 *
 * This makes it possible for the LDR measurements to be relative to previously
 * made settings. So rather than the LDR measurements changing the brightness
 * in absolute terms, they will be applied "on top" of the settings previously
 * made.
 *
 * Internally it checks whether the current values for base_pwm_idx and
 * offset_pwm_idx are within boundaries (0 - MAX_PWM_STEPS - 1), and sets it
 * to the maximum if necessary. It then makes use of
 * modifyLdrBrightness2pwmStep() with the calculated values.
 *
 * @see base_pwm_idx
 * @see offset_pwm_idx
 * @see modifyLdrBrightness2pwmStep()
 */
void pwm_modifyLdrBrightness2pwmStep()
{

    if (offset_pwm_idx) {

        uint8_t val = base_pwm_idx + offset_pwm_idx;

        if (val >= MAX_PWM_STEPS) {

            val = MAX_PWM_STEPS;

        }

        modifyLdrBrightness2pwmStep(base_ldr_idx, val);
        base_pwm_idx = val;
        offset_pwm_idx = 0;
        pwm_set_brightness();

        /*
         * Indicate that the settings have been applied successfully
         */
        pwm_on_off();
        _delay_ms(500);
        pwm_on_off();

    }

}
