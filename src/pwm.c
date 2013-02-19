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
 * @file pwm.c
 * @brief Implements the header declared in pwm.h
 *
 * Each channel (red, green, blue) is controlled separately and is attached
 * to a different output compare pin of the microcontroller (Red = OC0A,
 * Green = OC0B, Blue = OC2B). This means that actually two timers, namely
 * Timer/Counter0 and Timer/Counter2 are used.
 *
 * In case the software is compiled with monochromatic LED support only
 * (MONO_COLOR_CLOCK == 1), Timer/Counter2 will be left untouched.
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
#include "main.h"
#include "pwm.h"
#include "ports.h"

/**
 * @brief Port and pin definition of the line in control of the red channel
 *
 * @see ports.h
 */
#define PWM_RED     PORTD, 6

/**
 * @brief Port and pin definition of the line in control of the green channel
 *
 * @see ports.h
 */
#define PWM_GREEN   PORTD, 5

/**
 * @brief Port and pin definition of the line in control of the blue channel
 *
 * @see ports.h
 */
#define PWM_BLUE    PORTD, 3

#if (MAX_PWM_STEPS == 32)

    const uint8_t pwm_table[MAX_PWM_STEPS] PROGMEM =
        {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 12, 14, 16, 18, 21, 24, 28, 32, 37,
        42, 48, 55, 63, 72, 83, 96, 111, 129, 153, 182, 216, 255};

#elif (MAX_PWM_STEPS == 64)

    const uint8_t pwm_table[MAX_PWM_STEPS] PROGMEM =
        {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19,
        21, 22, 23, 25, 26, 27, 29, 30, 32, 33, 35, 36, 38, 40, 42, 44, 46,
        48, 51, 53, 56, 58, 61, 63, 66, 69, 72, 75, 78, 81, 85, 89, 93, 98,
        103, 109, 116, 125, 135, 146, 158, 171, 189, 214, 255};

#else

    #error unknown pwm step size

#endif

static bool pwm_is_on;
static uint8_t base_pwm_idx;
static uint8_t brightness_pwm_val;
static bool brightness_lock;

#define offset_pwm_idx (wcEeprom_getData()->pwmParams.brightnessOffset)
#define g_occupancy (wcEeprom_getData()->pwmParams.occupancy)
#define g_ldrBrightness2pwmStep (wcEeprom_getData()->pwmParams.brightness2pwmStep)

static uint8_t base_ldr_idx;

#if (MONO_COLOR_CLOCK != 1)

    static uint8_t red_pwm_idx;
    static uint8_t green_pwm_idx;
    static uint8_t blue_pwm_idx;
    static uint8_t red_pwm;
    static uint8_t green_pwm;
    static uint8_t blue_pwm;

#endif

/*---------------------------------------------------------------------------------------------------------------------------------------------------
 *  Set brightness by step
 *  @details  sets brightness by global step values base_pwm_idx + offset_pwm_idx
 *---------------------------------------------------------------------------------------------------------------------------------------------------
 */
static void pwm_set_brightness_step(void)
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

    #if (MONO_COLOR_CLOCK == 1)

        OCR0A = 255 - brightness_pwm_val;

    #else

        pwm_set_colors(red_pwm, green_pwm, blue_pwm);

    #endif

}

/*---------------------------------------------------------------------------------------------------------------------------------------------------
 *  Initialize the PWM
 *  @details  Configures 0CR0A, 0CR0B and 0CR2B as PWM channels
 *---------------------------------------------------------------------------------------------------------------------------------------------------
 */
void pwm_init(void)
{

    PORT(PWM_RED) &= ~_BV(BIT(PWM_RED));
    DDR(PWM_RED) |= _BV(BIT(PWM_RED));

    TCCR0A = _BV(WGM01) | _BV(WGM00);
    TCCR0B = _BV(CS01) | _BV(CS00);

    TCCR2A = _BV(WGM21) | _BV(WGM20);
    TCCR2B = _BV(CS22);

    #if (MONO_COLOR_CLOCK != 1)

        PORT(PWM_GREEN) &= ~_BV(BIT(PWM_GREEN));
        DDR(PWM_GREEN) |= _BV(BIT(PWM_GREEN));
        PORT(PWM_BLUE) &= ~_BV(BIT(PWM_BLUE));
        DDR(PWM_BLUE) |= _BV(BIT(PWM_BLUE));

    #endif

}

/*---------------------------------------------------------------------------------------------------------------------------------------------------
 *  Switch PWM on
 *---------------------------------------------------------------------------------------------------------------------------------------------------
 */
void pwm_on(void)
{

    TCCR0A |= _BV(COM0A1) | _BV(COM0A0);

    #if (MONO_COLOR_CLOCK != 1)

        TCCR0A |= _BV(COM0B1) | _BV(COM0B0);
        TCCR2A |= _BV(COM2B1) | _BV(COM2B0);

    #endif

    pwm_is_on = true;
    pwm_set_brightness_step();

}

/*---------------------------------------------------------------------------------------------------------------------------------------------------
 *  Switch PWM off
 *---------------------------------------------------------------------------------------------------------------------------------------------------
 */
void pwm_off(void)
{

    PORT(PWM_RED) &= ~_BV(BIT(PWM_RED));

    TCCR0A &= ~_BV(COM0A1) | _BV(COM0A0);

    #if (MONO_COLOR_CLOCK != 1)

        PORT(PWM_GREEN) &= ~_BV(BIT(PWM_GREEN));
        TCCR0A &= ~(_BV(COM0B1) | _BV(COM0B0));

        PORT(PWM_BLUE)  &= ~_BV(BIT(PWM_BLUE));
        TCCR2A &= ~(_BV(COM2B1) | _BV(COM2B0));

    #endif

    pwm_is_on = false;

}

/*---------------------------------------------------------------------------------------------------------------------------------------------------
 *  Toggle PWM off/on
 *---------------------------------------------------------------------------------------------------------------------------------------------------
 */
void pwm_on_off(void)
{

    if (pwm_is_on) {

        pwm_off();

    } else {

        pwm_on();
        pwm_set_brightness_step();

    }

}

#if (MONO_COLOR_CLOCK != 1)

    /*---------------------------------------------------------------------------------------------------------------------------------------------------
     *  Set RGB colors
     *  @param    red: range 0-255
     *  @param    green: range 0-255
     *  @param    blue: range 0-255
     *---------------------------------------------------------------------------------------------------------------------------------------------------
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

    /*---------------------------------------------------------------------------------------------------------------------------------------------------
     *  Get RGB colors
     *  @param    pointer to value of red pwm: range 0-255
     *  @param    pointer to value of green pwm: range 0-255
     *  @param    pointer to value of blue pwm: range 0-255
     *---------------------------------------------------------------------------------------------------------------------------------------------------
     */
    void pwm_get_colors(uint8_t* redp, uint8_t* greenp, uint8_t* bluep)
    {

        *redp   = red_pwm;
        *greenp = green_pwm;
        *bluep  = blue_pwm;

    }

    /*---------------------------------------------------------------------------------------------------------------------------------------------------
     *  Set RGB colors by step values
     *  @param    red_step: range 0 to MAX_PWM_STEPS
     *  @param    green_step: range 0 to MAX_PWM_STEPS
     *  @param    blue_step: range 0 to MAX_PWM_STEPS
     *---------------------------------------------------------------------------------------------------------------------------------------------------
     */
    void pwm_set_color_step(uint8_t red_step, uint8_t green_step, uint8_t blue_step)
    {

        if (red_step >= MAX_PWM_STEPS) {

            red_step = MAX_PWM_STEPS - 1;

        }

        if (green_step >= MAX_PWM_STEPS) {

            green_step = MAX_PWM_STEPS - 1;

        }

        if (blue_step >= MAX_PWM_STEPS) {

            blue_step = MAX_PWM_STEPS - 1;

        }

        red_pwm_idx   = red_step;
        green_pwm_idx = green_step;
        blue_pwm_idx  = blue_step;

        pwm_set_colors(pgm_read_byte(pwm_table + red_step),
                pgm_read_byte(pwm_table + green_step),
                pgm_read_byte (pwm_table + blue_step));

    }

    /*---------------------------------------------------------------------------------------------------------------------------------------------------
     *  Get RGB color step values
     *  @param    pointer to red_step: range 0 to MAX_PWM_STEPS
     *  @param    pointer to green_step: range 0 to MAX_PWM_STEPS
     *  @param    pointer to blue_step: range 0 to MAX_PWM_STEPS
     *---------------------------------------------------------------------------------------------------------------------------------------------------
     */
    void pwm_get_color_step(uint8_t* red_stepp, uint8_t* green_stepp, uint8_t* blue_stepp)
    {

        *red_stepp   = red_pwm_idx;
        *green_stepp = green_pwm_idx;
        *blue_stepp  = blue_pwm_idx;

    }

#endif

/*---------------------------------------------------------------------------------------------------------------------------------------------------
 *  Set base brightness by step 0-31
 *  @param    pwm step 0-31
 *---------------------------------------------------------------------------------------------------------------------------------------------------
 */
void pwm_set_base_brightness_step(uint8_t pwm_idx)
{

    if (pwm_idx >= LDR2PWM_COUNT) {

        pwm_idx = LDR2PWM_COUNT - 1;

    }

    base_ldr_idx = pwm_idx;
    base_pwm_idx = g_ldrBrightness2pwmStep[pwm_idx];
    pwm_set_brightness_step();

}

/*---------------------------------------------------------------------------------------------------------------------------------------------------
 *  Step up brightness
 *---------------------------------------------------------------------------------------------------------------------------------------------------
 */
void pwm_step_up_brightness(void)
{

    if (pwm_is_on && (base_pwm_idx + offset_pwm_idx + 1 < MAX_PWM_STEPS)) {

        offset_pwm_idx++;
        pwm_set_brightness_step();

    }

}

/*---------------------------------------------------------------------------------------------------------------------------------------------------
 *  Step down brightness
 *---------------------------------------------------------------------------------------------------------------------------------------------------
 */
void pwm_step_down_brightness(void)
{

    if (pwm_is_on && (base_pwm_idx + offset_pwm_idx > 0)) {

        offset_pwm_idx--;
        pwm_set_brightness_step();

    }

}

void pwm_lock_brightness_val(uint8_t val)
{

    brightness_lock = true;
    brightness_pwm_val = val;
    pwm_set_brightness_step();

}

void pwm_release_brightness(void)
{

    brightness_lock = false;
    pwm_set_brightness_step();

}

#if (LOG_LDR2PWM == 1)

    void outputVals()
    {
        char buf[4];

        for (uint8_t i = 0; i < LDR2PWM_COUNT; i++) {

            byteToStr(g_ldrBrightness2pwmStep[i], buf);
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

static void interpolate(uint8_t left, uint8_t right)
{

    enum {

        SHIFT = 8,

    };

    uint8_t d = right - left;

    if (d > 1) {

        int8_t leftVal  = (int8_t)(g_ldrBrightness2pwmStep[left]);
        int8_t rightVal = (int8_t)(g_ldrBrightness2pwmStep[right]);

        int16_t l = (((int16_t)leftVal) << SHIFT) + (((uint16_t)1) << (SHIFT - 1));

        int16_t slope = ((int16_t)(rightVal - leftVal )) << SHIFT;
        slope /= d;

        for (uint8_t i = 1; i < d; i++) {

            l += slope;
            g_ldrBrightness2pwmStep[left + i] = (uint8_t)(l >> SHIFT);

        }

    }

}


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

void pwm_modifyLdrBrightness2pwmStep(void)
{

    if (offset_pwm_idx) {

        uint8_t val = base_pwm_idx + offset_pwm_idx;

        if (val >= MAX_PWM_STEPS) {

            val = MAX_PWM_STEPS;

        }

        modifyLdrBrightness2pwmStep(base_ldr_idx, val);
        base_pwm_idx = val;
        offset_pwm_idx = 0;
        pwm_set_brightness_step();

        pwm_on_off();
        _delay_ms(500);
        pwm_on_off();

    }

}
