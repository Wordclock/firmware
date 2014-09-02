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
 * For details about the various counters and registers involved it might be
 * useful to take a look at [1].
 *
 * [1]: http://www.atmel.com/images/doc2545.pdf
 *
 * @note In case the software is compiled without support for RGB LEDs
 * (`ENABLE_RGB_SUPPORT` == 0), Timer/Counter2 will be left untouched.
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
     * @brief Table containing predefined values used to generate PWM signal
     *
     * This table is used to generate a PWM signal. It contains 32 values,
     * making it possible to implement 32 different brightness steps. These
     * values are precalculated to fit the logarithmic nature of the human eye
     * as close as possible.
     *
     * @see pwm_accommodate_brightness()
     */
    const uint8_t pwm_table[MAX_PWM_STEPS] PROGMEM =
        {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 12, 14, 16, 18, 21, 24, 28, 32, 37,
        42, 48, 55, 63, 72, 83, 96, 111, 129, 153, 182, 216, 255};

#elif (MAX_PWM_STEPS == 64)

    /**
     * @copydoc pwm_table
     *
     * Contains 64 predefined values, allowing for 64 different brightness
     * steps.
     */
    const uint8_t pwm_table[MAX_PWM_STEPS] PROGMEM =
        {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19,
        21, 22, 23, 25, 26, 27, 29, 30, 32, 33, 35, 36, 38, 40, 42, 44, 46,
        48, 51, 53, 56, 58, 61, 63, 66, 69, 72, 75, 78, 81, 85, 89, 93, 98,
        103, 109, 116, 125, 135, 146, 158, 171, 189, 214, 255};

#else

    #error Unknown PWM step size

#endif

/**
 * @brief Indicates whether the PWM generation is currently turned on
 *
 * This indicates whether the generation of the PWM signal(s) is currently
 * turned on. `pwm_on()` and `pwm_off()` can be used to change the status
 * appropriately.
 *
 * @see pwm_on()
 * @see pwm_off()
 */
bool pwm_is_on;

/**
 * @brief Current base brightness
 *
 * This points to a specific value within `pwm_table` and represents the
 * current base brightness, which is used for various calculations within this
 * module. All values ranging from 0 to `MAX_PWM_STEPS - 1` are valid.
 *
 * @see MAX_PWM_STEPS
 * @see pwm_table
 */
static uint8_t base_pwm_idx;

/**
 * @brief Current brightness value for generating the PWM signal
 *
 * This contains the current brightness value. Usually this would be a value
 * of `pwm_table` pointed to by `base_pwm_idx + offset_pwm_idx`, although it
 * is also possible to lock the brightness to a specific value using
 * `pwm_lock_brightness_val()`.
 *
 * @see pwm_table
 * @see base_pwm_idx
 * @see offset_pwm_idx
 */
static uint8_t brightness_pwm_val;

/**
 * @brief Indicates whether brightness is currently locked
 *
 * The brightness can be locked to a specific value using
 * `pwm_lock_brightness_val()`. Once the brightness was locked, it needs to
 * be released again using `pwm_release_brightness()` before any other changes
 * to the brightness can be made. This variable indicates whether the
 * brightness is currently being locked.
 *
 * @see pwm_lock_brightness_val()
 * @see pwm_release_brightness()
 */
static bool brightness_lock;

/**
 * @brief Macro for easy access to PwmEepromParams::brightnessOffset
 *
 * @see PwmEepromParams::brightnessOffset
 * @see wcEeprom_getData()
 */
#define offset_pwm_idx (wcEeprom_getData()->pwmParams.brightnessOffset)

/**
 * @brief Macro for easy access to PwmEepromParams::occupancy
 *
 * @see PwmEepromParams::occupancy
 * @see wcEeprom_getData()
 */
#define g_occupancy (wcEeprom_getData()->pwmParams.occupancy)

/**
 * @brief Macro for easy access to PwmEepromParams::brightness2pwmStep
 *
 * @see PwmEepromParams::brightness2pwmStep
 * @see wcEeprom_getData()
 */
#define g_ldrBrightness2pwmStep \
    (wcEeprom_getData()->pwmParams.brightness2pwmStep)

/**
 * @brief Current base LDR index
 *
 * This points to a specific value within `g_ldrBrightness2pwmStep` and is used
 * for various calculations within this module. All values ranging from 0 to
 * `LDR2PWM_COUNT - 1` are valid.
 *
 * @see g_ldrBrightness2pwmStep
 */
static uint8_t base_ldr_idx;

#if (ENABLE_RGB_SUPPORT == 1)

    /**
     * @brief Keeps track of the currently used color
     *
     * This is used within the module to keep track of the currently used
     * color. It can be set by `pwm_set_color()` and retrieved by
     * `pwm_get_color()`.
     *
     * @see color_rgb_t
     * @see pwm_set_color()
     * @see pwm_get_color()
     */
    static color_rgb_t pwm_color;

#endif /* (ENABLE_RGB_SUPPORT == 1) */

/**
 * @brief Sets brightness to a value value within pwm_table
 *
 * The brightness will be set to a value of `pwm_table` pointed to by
 * `base_pwm_idx + offset_pwm_idx`. After the value for the brightness has been
 * retrieved, it is stored within `brightness_pwm_val` and applied - either
 * by directly writing to the appropriate OCR register in case of
 * `ENABLE_RGB_SUPPORT` == 0, or by invoking `pwm_set_color()`.
 *
 * @note This will only work if the brightness is currently not being locked.
 *
 * @see base_pwm_idx
 * @see offset_pwm_idx
 * @see pwm_table
 * @see brightness_pwm_val
 * @see pwm_set_color()
 */
static void pwm_accommodate_brightness()
{

    if (!brightness_lock) {

        int8_t pwm_idx = ((int8_t)base_pwm_idx) + offset_pwm_idx;

        if (pwm_idx < 0) {

            pwm_idx = 0;

        } else if (pwm_idx >= MAX_PWM_STEPS) {

            pwm_idx = MAX_PWM_STEPS - 1;

        }

        brightness_pwm_val = pgm_read_byte(&pwm_table[pwm_idx]);

    }

    #if (ENABLE_RGB_SUPPORT == 1)

        pwm_set_color(pwm_color);

    #else

        OCR0A = 255 - brightness_pwm_val;

    #endif

}

/**
 * @brief Initializes the PWM module
 *
 * This sets up the involved timers in a way that the PWM signal(s) are
 * generated. It needs to be called once before the module and its
 * functionality can be used.
 *
 * @note No signal(s) are actually being output until `pwm_on()` is invoked.
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

        /*
         * Waveform generation mode: Fast PWM
         * Top: 0xFF
         * Update of OCRx: Bottom
         * Prescaler: 8
         */
        TCCR2A = _BV(WGM21) | _BV(WGM20);
        TCCR2B = _BV(CS21);

    #endif

    /*
     * Waveform generation mode: Fast PWM
     * Top: 0xFF
     * Update of OCRx: Bottom
     * Prescaler: 8
     */
    TCCR0A = _BV(WGM01) | _BV(WGM00);
    TCCR0B = _BV(CS01);

}

/**
 * @brief Turns the output of the PWM signal on
 *
 * This sets up the timers involved in a way that the generated PWM signal
 * is actually being output on the appropriate pins. Furthermore it invokes
 * `pwm_accommodate_brightness()` to make sure the brightness is set correctly.
 *
 * @see pwm_is_on
 * @see pwm_accommodate_brightness()
 */
void pwm_on()
{

    /*
     * Set OC0A on compare match, clear OC0A at BOTTOM, (inverting mode)
     */
    TCCR0A |= _BV(COM0A1) | _BV(COM0A0);

    #if (ENABLE_RGB_SUPPORT == 1)

        /*
         * Set OC0B on compare match, clear OC0B at BOTTOM, (inverting mode)
         * Set OC2A on compare match, clear OC2A at BOTTOM, (inverting mode)
         */
        TCCR0A |= _BV(COM0B1) | _BV(COM0B0);
        TCCR2A |= _BV(COM2B1) | _BV(COM2B0);

    #endif

    pwm_is_on = true;
    pwm_accommodate_brightness();

}

/**
 * @brief Turns the output of the PWM signal off
 *
 * This sets up the involved timers in a way that no PWM signal is being output
 * on the appropriate pins.
 *
 * @see pwm_is_on
 */
void pwm_off()
{

    /*
     * Normal port operation, OC0A disconnected
     */
    TCCR0A &= ~_BV(COM0A1) | _BV(COM0A0);
    PORT(PWM_RED) &= ~_BV(BIT(PWM_RED));

    #if (ENABLE_RGB_SUPPORT == 1)

        /*
         * Normal port operation, OC0B disconnected
         */
        TCCR0A &= ~(_BV(COM0B1) | _BV(COM0B0));
        PORT(PWM_GREEN) &= ~_BV(BIT(PWM_GREEN));

        /*
         * Normal port operation, OC2A disconnected
         */
        TCCR2A &= ~(_BV(COM2B1) | _BV(COM2B0));
        PORT(PWM_BLUE)  &= ~_BV(BIT(PWM_BLUE));

    #endif

    pwm_is_on = false;

}

#if (ENABLE_RGB_SUPPORT == 1)

    /**
     * @brief Sets the RGB color and applies it to the output
     *
     * This applies the given color to the output after performing some basic
     * calculations to accommodate for the currently set brightness.
     *
     * @param color The color to be set
     *
     * @see color_rgb_t
     * @see pwm_color
     */
    void pwm_set_color(color_rgb_t color)
    {

        uint16_t brightnessFactor = ((uint16_t)brightness_pwm_val) + 1;

        pwm_color = color;

        OCR0A = 255 - ((brightnessFactor * color.red) / 256);
        OCR0B = 255 - ((brightnessFactor * color.green) / 256);
        OCR2B = 255 - ((brightnessFactor * color.blue) / 256);

    }

    /**
     * @brief Returns the RGB color currently being used
     *
     * This returns a pointer to `pwm_color`, which holds the color currently
     * being used.
     *
     * @return color Pointer to pwm_color
     *
     * @see color_rgb_t
     */
    const color_rgb_t* pwm_get_color()
    {

        return &pwm_color;

    }

#endif /* (ENABLE_RGB_SUPPORT == 1) */

/**
 * @brief Sets the given base brightness and applies it
 *
 * This sets the base brightness (`base_pwm_idx`) to the appropriate value from
 * `g_ldrBrightness2pwmStep`. Valid values range from 0 to `LDR2PWM_COUNT - 1`.
 * After the base brightness has been set, `pwm_accommodate_brightness()` is
 * invoked to accommodate for the change.
 *
 * @param base_brightness Base brightness to set,range 0 - LDR2PWM_COUNT - 1
 *
 * @see base_ldr_idx
 * @see base_pwm_idx
 * @see g_ldrBrightness2pwmStep
 * @see pwm_accommodate_brightness()
 */
void pwm_set_base_brightness(uint8_t base_brightness)
{

    if (base_brightness >= LDR2PWM_COUNT) {

        base_brightness = LDR2PWM_COUNT - 1;

    }

    base_ldr_idx = base_brightness;
    base_pwm_idx = g_ldrBrightness2pwmStep[base_brightness];
    pwm_accommodate_brightness();

}

/**
 * @brief Increases the brightness
 *
 * This increases the brightness by a single step. It increments
 * `offset_pwm_idx` by one and invokes `pwm_accommodate_brightness()`
 * afterwards.
 *
 * @note This only works if the maximum brightness hasn't been reached yet.
 *
 * @see offset_pwm_idx
 * @see pwm_accommodate_brightness()
 * @see pwm_is_on
 * @see MAX_PWM_STEPS
 */
void pwm_increase_brightness()
{

    if (pwm_is_on && (base_pwm_idx + offset_pwm_idx + 1 < MAX_PWM_STEPS)) {

        offset_pwm_idx++;
        pwm_accommodate_brightness();

    }

}

/**
 * @brief Decreases the brightness
 *
 * This decreases the brightness by a single step. It decrements
 * `offset_pwm_idx` by one and invokes `pwm_accommodate_brightness()`
 * afterwards.
 *
 * @note This only works if the minimum brightness hasn't been reached yet.
 *
 * @see offset_pwm_idx
 * @see pwm_accommodate_brightness()
 * @see pwm_is_on
 */
void pwm_decrease_brightness()
{

    if (pwm_is_on && (base_pwm_idx + offset_pwm_idx > 0)) {

        offset_pwm_idx--;
        pwm_accommodate_brightness();

    }

}

/**
 * @brief Locks the brightness to the given value
 *
 * This locks the brightness to the given value, making sure it can't be
 * changed anymore until the lock is released using `pwm_release_brightness()`.
 *
 * @param val The value you want to lock the brightness to, range 0 to 255
 *
 * @see pwm_accommodate_brightness()
 * @see pwm_release_brightness()
 */
void pwm_lock_brightness_val(uint8_t val)
{

    brightness_lock = true;
    brightness_pwm_val = val;
    pwm_accommodate_brightness();

}

/**
 * @brief Releases brightness lock
 *
 * This releases a brightness lock previously acquired by
 * `pwm_lock_brightness_val` and invokes `pwm_accommodate_brightness()`
 * afterwards to make sure the correct brightness will be set.
 *
 * @see pwm_accommodate_brightness()
 * @see pwm_lock_brightness_val()
 */
void pwm_release_brightness()
{

    brightness_lock = false;
    pwm_accommodate_brightness();

}

#if (LOG_LDR2PWM == 1)

    /**
     * @brief Puts out g_ldrBrightness2pwmStep and g_occupancy via UART
     *
     * This puts out the variables used for accommodations in response to LDR
     * measurements (`g_ldrBrightness2pwmStep` and `g_occupancy`) via UART.
     * As `g_occupancy` is a bitfield an appropriate representation of it, will
     * be output rather than the actual value itself.
     *
     * @see g_ldrBrightness2pwmStep
     * @see g_occupancy
     * @see uart.h
     */
    static void outputVals()
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
     * @brief Puts out a graphical bar for the given values
     *
     * This puts out a graphical bar with a length of `LDR2PWM_COUNT`
     * characters. `ind` is represented by `^`, `l` by `<` and `r` by `>`.
     * Other positions are left empty. The result might end up looking
     * something like this:
     *
     * \code
     *  <           ^          >
     * \endcode
     *
     * @note This is used for debugging purposes only.
     *
     * @param ind Index, position where `^` will be put
     * @param l Left boundary, position where `<` will be put
     * @param r Right boundary, position where `>` will be put
     *
     * @see LDR2PWM_COUNT
     * @see uart_putc()
     */
    static void outputPointer(uint8_t ind, uint8_t l, uint8_t r)
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
 * This iterates over g_occupancy (multiple times) and determines the leftmost
 * and rightmost bits and writes their position to `left` and/or `right`
 * appropriately.
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
 * @brief Interpolates linearly between two values of g_ldrBrightness2pwmStep
 *
 * Both parameters are actually indexes for values within
 * `g_ldrBrightness2pwmStep`. The corresponding values for both of these
 * indexes are retrieved and a linear interpolation between these two points
 * is performed, see [1] for details. Fixed point arithmetic [2] is used within
 * this function for better accuracy.
 *
 * [1]: https://en.wikipedia.org/wiki/Linear_interpolation
 * [2]: https://en.wikipedia.org/wiki/Fixed-point_arithmetic
 *
 * @param left The left point, index within g_ldrBrightness2pwmStep
 * @param right The right point, index within g_ldrBrightness2pwmStep
 *
 * @see g_ldrBrightness2pwmStep
 */
static void interpolate(uint8_t left, uint8_t right)
{

    enum {

        /**
         * @brief Scaling factor for operands
         *
         * This is the amount of bits operands within this function will be
         * shifted.
         *
         * @note For reasons of performance this was chosen to be eight as it
         * can be better optimized for.
         */
        SHIFT = 8,

    };

    uint8_t d = right - left;

    if (d > 1) {

        int8_t leftVal  = (int8_t)(g_ldrBrightness2pwmStep[left]);
        int8_t rightVal = (int8_t)(g_ldrBrightness2pwmStep[right]);

        int16_t l =
            (((int16_t)leftVal) << SHIFT) + (((uint16_t)1) << (SHIFT - 1));

        int16_t slope = ((int16_t)(rightVal - leftVal)) << SHIFT;
        slope /= d;

        for (uint8_t i = 1; i < d; i++) {

            l += slope;
            g_ldrBrightness2pwmStep[left + i] = (uint8_t)(l >> SHIFT);

        }

    }

}

/**
 * @brief Modifies the internal data responsible for the LDR PWM mapping
 *
 * This modifies `g_ldrBrightness2pwmStep` by getting the boundaries
 * (`getBounds()`) and interpolating linearly between the left one and the
 * index as well as between the index and the right one using `interpolate()`.
 *
 * @see g_ldrBrightness2pwmStep
 * @see g_occupancy
 * @see getBounds()
 * @see interpolate()
 * @see outputVals()
 * @see outputPointer()
 */
static void modifyLdrBrightness2pwmStep(uint8_t ind, uint8_t val)
{

    #if (LOG_LDR2PWM == 1)

        uart_puts_P("before\n");
        outputVals();

    #endif

    uint8_t left;
    uint8_t right;

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
 * @brief Modifies the mapping between the brightness and the PWM step
 *
 * This takes over the offset defined by the user (`offset_pwm_idx`) and
 * incorporates it into `base_pwm_idx`. `modifyLdrBrightness2pwmStep()` is
 * invoked to make sure that the internal data is updated appropriately.
 * After the data has been altered, `pwm_accommodate_brightness()` is invoked to
 * actually change the brightness in accordance to the newly calculated values.
 *
 * @see base_pwm_idx
 * @see offset_pwm_idx
 * @see modifyLdrBrightness2pwmStep()
 * @see pwm_accommodate_brightness()
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
        pwm_accommodate_brightness();

    }

}
