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
#error F_CPU unkown
#endif


  
/** Macro to build software version byte */
#define BUILD_VERSION( x,y )    ( (uint8_t)((x<<4)|(y)))

#define MAJOR_VERSION 0
#define MINOR_VERSION 12

/** Defines the software version  */
#define SW_VERSION   BUILD_VERSION(  MAJOR_VERSION, MINOR_VERSION  )

#define WC_DISP_GER   0   /**< Word Clock with german layout with 2 regional idoms */
#define WC_DISP_GER3  1   /**< Word Clock with german layout with 3 regional idoms */
#define WC_DISP_ENG   0   /**< Word Clock with english layout                      */

/**
 *  if only one color LEDs are used.
 *  changing this to 1 will save a lot of memory
 *  by disabling all the RGB-Code
 */
#define MONO_COLOR_CLOCK 0

/**
 *  activates code for DCF control 
 *  \details
 *      If active there is also a detection if really a dcf module is connected.\n
 *      So there is no need to deactivate it. 
 *      Never the less deactivating saves ~1k so it may help using a smaller controller.
 */
#define DCF_PRESENT           1
#define AMBILIGHT_PRESENT     1  /**< activates Code for ambilight control GPO1 */
#define BLUETOOTH_PRESENT     1  /**< activates Code for bluetooth control GPO2 */
#define AUXPOWER_PRESENT      1  /**< activates Code for auxiliary control GPO3 */
	
/**
 *  activates code for UART Reset to start bootloader
 *  \ details
 *       If active the UART is enable to jump to the bootloader.\n
 *       Therefore just send an 'R' via UART and the bootloader will be started.\n
 *       It is possible to reset via Watchdog (BOOTLOADER_RESET_WDT = 1)
 *       or to jump direclty to the bootloader (BOOTLOADER_RESET_WDT = 0).
 */
#define BOOTLOADER_RESET_UART 1
#define BOOTLOADER_RESET_WDT  1


/* most important user parameters */

/** if '1' only the commands of activated functions (e.g. ambilight) will be 
 *  saved and trained. Also individual latex manual will be created. 
 *  Else if '0' all functions (present or not) have to be assigned in training sequence
 */
#define INDIVIDUAL_CONFIG 1

/** enables or disables auto save of parameters to eeprom 
 *  after a while without received IR-Data
 *  also see USER_DELAY_BEFORE_SAVE_EEPROM_s
 */
#define USER_AUTOSAVE 1




// Debugging:
#define LOG_MAIN               0  /**< reports clock init on UART                               */
#define LOG_MAIN_BRIGHTNESS    0  /**< reports brightnes updates to UART                        */
#define LOG_LDR                0  /**< reports all ldr measurements to UART                     */
#define LOG_LDR2PWM            0  /**< reports changes on ldr2pwm mapping to UART               */
#define LOG_DISPLAY_STATE      0  /**< reports the display output data to UART                  */
#define LOG_USER_IR_CMD        0  /**< reports the detected IR Command to UART                  */
#define LOG_USER_STATE         0  /**< reports the internal state changes to UART               */
#define LOG_USER_TIME          0  /**< reports internal time updates to UART                    */
#define LOG_USER_IR_TRAIN      0  /**< reports some usefull information on IR-Training to UART  */
#define LOG_EEPROM_INIT        0  /**< reports EEPROM data on init to UART                      */
#define LOG_EEPROM_WRITEBACK   0  /**< reports changed EEPROM data to UART                      */
#define LOG_DCF77              0  /**< reports various states and changes of the DCF module		*/

extern void       main_ISR(void);

#endif /* _WC_MAIN_H_ */
