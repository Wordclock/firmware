/*
 * Copyright (C) 2012, 2013 Karol Babioch <karol@babioch.de>
 * Copyright (c) 2010 Torsten Giese
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
 * @file dcf77.c
 * @brief Implementation of the header declared in dcf77.h
 *
 * This file contains the implementation of the header declared in dcf77.h. In
 * order to understand the source code completely, a basic understanding of the
 * DCF77 signal is needed. Take a look at the Wikipedia [article][1] for an
 * overview of the concept and a detailed description of the the time signal.
 *
 * This moudule can detect the availability of the module itself and determine
 * whether it is active high or low. Furthermore it checks whether the internal
 * pull up resistor is needed. Take a look at dcf77_check_module_type() for
 * details.
 *
 * [1]: https://en.wikipedia.org/wiki/DCF77
 *
 * @see dcf77.h
 */

#include <inttypes.h>
#include <avr/io.h>
#include <stdbool.h>

#include "main.h"
#include "dcf77.h"
#include "uart.h"
#include "ports.h"

/*
 * Check whether DCF77 functionality is enabled
 */
#if (DCF_PRESENT == 1)

/*
 * Check whether logging for this module is enabled
 */
#if  (LOG_DCF77 == 1)

	/**
	 * @brief Used to output logging information of this module
	 *
	 * When the logging for this module is enabled (LOG_DCF77 == 1), this macro
	 * is used to output various kinds of information.
	 *
	 * @see LOG_DCF77
	 */
	#define log_dcf77(x) uart_puts_P(x)

#else

	/**
	 * @brief Used to output logging information of this module
	 *
	 * This makes sure that nothing is being output when the logging for this
	 * module is deactivated (LOG_DCF77 == 0).
	 *
	 * @see LOG_DCF77
	 */
	#define log_dcf77(x)

#endif

/**
 * @brief Contains various flags used within this module
 *
 * These flags are mainly used to enable a basic form of communication between
 * the various functions of this module. To save some space they are combined
 * into an enumeration.
 *
 * @see FLAGS_e
 * @see getFlag()
 * @see setFlag()
 * @see clearFlag()
 */
uint8_t DCF_FLAG;

/**
 * @brief Holds various flags defined in FLAGS_e
 *
 * This is the actual variable holding the flags. It gets used by getFlag(),
 * setFlag() and clearFLag().
 *
 * @see getFlag()
 * @see setFlag()
 * @see clearFlag()
 */
typedef enum FLAGS_e {

    CHECK = 0, /**< Indicates whether a full valid time frame has been received */
    DEFINED, /**< Indicates whether the module type has already been determined */
    AVAILABLE, /**< Indicates whether or not there actually is a DCF77 module */
    HIGH_ACTIVE, /**< Indicates whether the DCF77 module is high or low active */

} FLAGS;

/**
 * @brief Retrieves the value of an individual flag
 *
 * @see FLAGS_e
 * @see setFlag()
 * @see clearFlag()
 */
static inline bool getFlag(FLAGS flag)
{

	return DCF_FLAG & _BV(flag);

}

/**
 * @brief Sets the value for an individual flag
 *
 * @see FLAGS_e
 * @see getFlag()
 * @see clearFlag()
 */
static inline void setFlag(FLAGS flag)
{

	DCF_FLAG |= _BV(flag);

}

/**
 * @brief Clears the value for an individual flag
 *
 * @see FLAGS_e
 * @see getFlag()
 * @see clearFlag()
 */
static inline void clearFlag(FLAGS flag)
{

	DCF_FLAG &= ~_BV(flag);

}

/**
 * @brief Port and pin of where the DCF77 receiver is attached to
 *
 * @see ports.h
 */
#define DCF_INPUT PORTB, 7

/**
 * @brief Port and pin of where to output the DCF77 signal
 *
 * This is primarily used for a LED, which would provide an indication of the
 * received DCF77 time signal. With a little bit of exercise you can
 * distinguish between 100 ms pulses and 200 ms pulses.
 *
 * Furthermore it is possible to get an idea of the quality of the signal.
 * If there is a lot of noise in the proximity of the receiver the LED will
 * blink abnormally fast and/or often.
 *
 * @see ports.h
 */
#define DCF_OUTPUT PORTD, 4

/**
 * @brief Type definition containing variables needed for decoding
 *
 * These variables are needed in order to successfully decode the DCF77 time
 * signal. They are combined in a struct.
 */
volatile typedef struct {

    uint8_t PauseCounter; /**< Counter for the pause length, +1 for each 10 ms */
    uint8_t BitCounter; /**< Indicates which bit is currently being broadcasted */
    uint8_t Parity; /**< Parity bit counter */
    uint8_t BCDShifter; /**< Shift counter used for converting the received data from BCD to decimal */
    uint8_t NewTime[6]; /**< Stores date & time of the time frame currently being broadcasted */
    uint8_t NewTimeShifter; /**< Identifies which data is currently being broadcasted */
    uint8_t OldTime; /**< Stores the time of the reception of  valid timeframe */

} DCF_Struct;

/**
 * @brief Holding DCF_Struct to be accessible by various function in this file
 *
 * @see DCF_Struct
 */
static DCF_Struct DCF;

/**
 * @brief Indicates whether the DCF77 reception should be enabled or not
 *
 * Setting this to false will deactivate the DCF77 reception temporarily. This,
 * for instance, can be used to deactivate the DCF77 reception once a
 * successful time frame has been successfully received in a row, within in a
 * hour.
 *
 * Keep in mind that this only will deactivate the decoding within this module.
 * The receiver, however, will stay enabled.
 */
bool enable_dcf77_ISR;

/**
 * @brief Counter keeping track of the amount of low pulses received
 *
 * This is used during the initialization phase where the type of the receiver
 * is determined.
 *
 * @see dcf77_check_module_type()
 */
static uint8_t count_low;

/**
 * @brief Counter keeping track of the amount of high pulses received
 *
 * This is used during the initialization phase where the type of the receiver
 * is determined.
 *
 * @see dcf77_check_module_type()
 */
static uint8_t count_high;

/**
 * @brief Used for converting received data from BCD to decimal
 *
 * @see DCF_Struct::BCDShifter
 */
const static uint8_t BCD_Kodierung[] = {1, 2, 4, 8, 10, 20, 40, 80};

/**
 * @brief Resets the state of the DCF77 module
 *
 * This function resets various variables used to keep track of the DCF77 time
 * signal. It gets usually called when an error during the reception gets
 * detected.
 *
 * @see DCF_Struct
 */
static void dcf77_reset(void)
{

	log_dcf77("DCF77 Reset\n");

	DCF.Parity = 0;
	DCF.PauseCounter = 0;
	DCF.BCDShifter = 0;
	DCF.BitCounter = 0;
	DCF.NewTimeShifter = 0;

	setFlag(CHECK);

	for (uint8_t i = 0; i < 6; i++) {

		DCF.NewTime[i] = 0;

	}

}

/**
 * @brief Determines what kind of module is connected to the microcontroller
 *
 * This function detects the kind of module connected to the microcontroller,
 * which includes the detection of whether the module is high or low active
 * and whether the internal pull up resistor is needed or not.
 *
 * This works by trying out all possible combinations and looking whether
 * transitions on the DCF77 pin occur.
 *
 * This function is only used during the initialization phase. Once the type of
 * the receiver was determined, it won't change during runtime anymore.
 *
 * @see FLAGS_e
 */
static void dcf77_check_module_type(void)
{

	/*
	 * Keeps track of how often this method was already called without enabling
	 * and/or disabling the pull up resistor
	 */
	static uint8_t count_pass;

	/*
	 * Keeps track of how often the pull up resistor was switched on and/or off
	 */
	static uint8_t count_switch;

	/*
	* Check whether one second has passed
	*/
	if (count_low + count_high >= 100) {

    	#if (LOG_DCF77 == 1)

			char log_text[8];

			byteToStr (count_low, log_text);
			uart_puts(log_text);
			uart_puts(" ");

			byteToStr (count_high, log_text);
			uart_puts(log_text);
			uart_puts(" ");

			byteToStr (count_high+count_low, log_text);
			uart_puts(log_text);
			uart_puts(" ");

			byteToStr (count_pass, log_text);
			uart_puts(log_text);
			uart_puts(" ");

			byteToStr (count_switch, log_text);
			uart_puts(log_text);
			uart_puts("\n");

    	#endif

		/*
		 * Check if at least one of both counters is low
		 */
		if ((count_low == 0) || (count_high == 0)) {

			/**
			 * Check whether this was already tried 20 times
			 */
			if (++count_pass == 20) {

				/*
				 * Reset counter for keeping track of how often this function
				 * was called with the same setting for the pull up resistor
				 */
				count_pass = 0;

				/*
				 * Increase the counter for keeping track of how often the
				 * setting for the pull up resistor has been changed, as we
				 * are going to change it immediately.
				 */
				count_switch++;

				/*
				 * Check whether pull up resistor is activated right now
				 */
				if ((PIN(DCF_INPUT) & _BV(BIT(DCF_INPUT)))) {

					/*
					 * Deactivate pull up resistor
					 */
					PORT(DCF_INPUT) &= ~_BV(BIT(DCF_INPUT));

					/*
					 * Output logging information regarding the change
					 */
					log_dcf77(" Pull-UP deactivated\n");

				} else {

					/*
					 * Activate pull up resistor
					 */
					PORT(DCF_INPUT)           |= _BV(BIT(DCF_INPUT));

					/*
					 * Output logging information regarding the change
					 */
					log_dcf77(" Pull-UP activated\n");

				}

				/*
				 * Check whether setting for the pull up resistor has been
				 * changed 30 times already. If the module hasn't been detected
				 * by now, it probably isn't available.
				 */
				if (count_switch == 30) {

					/*
					 * Activate pull up resistor just in case
					 */
					PORT(DCF_INPUT) |= _BV(BIT(DCF_INPUT));

					/*
					 * Set and/or clear appropriate flags
					 */
					setFlag(DEFINED);
					clearFlag(AVAILABLE);

					/*
					 * Output logging information
					 */
					log_dcf77("\nNo DCF77 Module detected!\n");

				}

			}

		} else {

			/*
			 * Both count_low and count_high have values different from 0,
			 * which means that there probably is a receiver available
			 */

			/*
			 * Check whether there were 30 changes without a change of the
			 * setting for the pull up resistor
			 */
			if (++count_pass == 30) {

				/*
				 * Module successfully detected, set appropriate flags
				 */
				setFlag(DEFINED);
				setFlag(AVAILABLE);

				/*
				 * If logging for this module is enabled the type of the
				 * module detected should be output
				 */
        		#if (LOG_DCF77 == 1)

					if (getFlag(HIGH_ACTIVE)) {

						log_dcf77("\nHigh active DCF77 Module detected!\n");

					} else {

						log_dcf77("\nLow active DCF77 Module detected!\n");

					}

        		#endif

				return;

			}

			/*
			 * Check which of both counters is bigger, which is an indicator
			 * for the type of the receiver, as one of both should be
			 * significantly bigger than the other.
			 *
			 * Remember: The pulse length is 100 ms and/or 200 ms, but the
			 * pause between two pulses is at least 800 ms long.
			 */
			if (count_low > count_high) {

				/*
				 * Check whether the current presumed type of the module is
				 * high active.
				 */
				if (getFlag(HIGH_ACTIVE)) {

					/*
					 * We would expect count_high to be bigger in case of a
					 * high active module, therefore we reset the pass counter
					 * and presume an low active module for the next passes.
					 */
					count_pass = 0;
					clearFlag(HIGH_ACTIVE);

				}

			/*
			 * count_low is smaller (and/or equal) to count_high
			 */
			} else {

				/*
				 * Check whether the current presumed type of the module is
				 * low active
				 */
				if (!(getFlag(HIGH_ACTIVE))) {

					/*
					 * We would expect count_low to be bigger in case of a low
					 * active module, therefore we reset the pass counter and
					 * presume an low active module for the next passes.
					 */
					count_pass = 0;
					setFlag(HIGH_ACTIVE);

				}

			}

		}

		/*
		 * Reset both counters, the high and/or low detection will be reached
		 * directly the next time this function is called
		 */
		count_low = 0;
		count_high = 0;

	}

}
/**
 * @brief Analyzes the received information
 *
 * This functions analyzes the received information. It gets called once a new
 * bit has been received. Any errors will trigger a reset (see dcf77_reset()).
 * This includes invalid pulse lenghts, invalid amount of bits received and/or
 * invalid parity bits. If the received time frame was valid true is returned,
 * otherwise it will return false.
 *
 * @return True if valid time frame was received, false otherwise
 * @see DCF_Struct
 * @see dcf77_reset()
 */
static bool dcf77_check(void)
{

	/*
	 * Check whether logging for this module is enabled
	 */
	#if (LOG_DCF77 == 1)

		if (DCF.PauseCounter > 0) {

			char log_text[8];

			/*
			 * Output the length of the last pause
			 */
			byteToStr(DCF.PauseCounter, log_text);
			uart_puts(log_text);
			uart_puts(" ");

		}

	#endif

	if ((DCF.PauseCounter <= 6)
      || ((DCF.PauseCounter <= 77) && (DCF.PauseCounter >= 96))) {

		/*
		 * Clear pause length counter
		 */
		DCF.PauseCounter = 0;

		return false;

	}

	if (((DCF.PauseCounter >= 170) && (DCF.BitCounter != 58))
		  || (DCF.BitCounter >= 59)) {

		/*
		 * Reset the internal state of this module
		 */
		dcf77_reset();

		return false;

	}

	/*
	 * The bits 0 - 20 are not important for us and won't be examined at all.
	 * They basically contain only contain weather data and information about
	 * leap seconds and time changes at the end of the hour.
	 */
	if (DCF.BitCounter <= 20) {

		/*
		 * Increase bit counter
		 */
		DCF.BitCounter++;

		/*
		 * Reset pause counter
		 */
		DCF.PauseCounter = 0;

		return false;

	}

	/*
	 * Check whether either 0 or 1 have been received.
	 */
	if ((DCF.PauseCounter >= 78) && (DCF.PauseCounter <= 95)) {

		/*
		 * Check whether 1 has been received
		 */
		if (DCF.PauseCounter <= 86) {

			/*
			 * Increase the parity counter
			 */
			DCF.Parity++;

			/*
			 * Check whether received bit was neither the parity data for the
			 * minutes nor the parity data for the hour
			 */
			if (!((DCF.BitCounter == 28) || (DCF.BitCounter == 35))) {

				/*
				 * Calculate the new time. This is achieved by using the
				 * BCD table represented by BCD_Kodierung.
				 */
				DCF.NewTime[DCF.NewTimeShifter]
					+= BCD_Kodierung[DCF.BCDShifter];

			}

		}

		/*
		 * Increment BCD shift counter preparing it for the next iteration
		 */
		DCF.BCDShifter++;

		/*
		 * Check whether parity data is valid in case we just received bit 28
		 * and/or 35
		 */
		if (((DCF.BitCounter == 28) || (DCF.BitCounter == 35))
				&& (DCF.Parity % 2 != 0)) {

			dcf77_reset();
			return false;

		}

		/*
		 * Check whether the next bit will contain information about another
		 * "property". The order is as following:
		 *
		 * Minute -> Hour -> Day of the month -> Day of the week -> Month
		 * 	-> Year
		 *
		 * 	The new "property" will be stored within its own field in
		 * 	DCF.NewTime. NewTimeShifter is used to keep track of this.
		 *
		 * 	BCDShifter on the other hand is used to keep track of the value
		 * 	the currently received bit represents as it is BCD encoded.
		 */
		if ((DCF.BitCounter == 28) || (DCF.BitCounter == 35)
				|| (DCF.BitCounter == 41) || (DCF.BitCounter == 44)
				|| (DCF.BitCounter == 49)) {

			DCF.NewTimeShifter++;
			DCF.BCDShifter = 0;

		}

		/*
		 * Increase bit counter
		 */
		DCF.BitCounter++;

		/*
		 * Reset pause counter
		 */
		DCF.PauseCounter = 0;

		return false;

	}

	/**
	 * Check whether received bit was the start of a new minute.
	 */
	if (DCF.PauseCounter >= 170) {

		/*
		 * Check whether the last bit broadcasted was 1
		 */
		if (!(DCF.PauseCounter >= 187)) {

			/*
			 * Increase parity counter
			 */
			DCF.Parity++;

		}

		/*
		 * Check whether 58 bits have been received and whether the parity data
		 * for the last one is valid, which means even.
		 */
		if ((DCF.BitCounter != 58) || (DCF.Parity % 2 != 0)) {

			/*
			 * Reset the internal state of this module
			 */
			dcf77_reset();

			return false;

		}

		uint8_t NewTime;

		NewTime = DCF.NewTime[0] + DCF.NewTime[1] * 60;

		if ((DCF.OldTime + 1) == NewTime) {

			/*
			 * Output some log information
			 */
			log_dcf77(" 2nd DCF77 correct\n");

			return true;

		} else {

			/*
			 * Output some log information
			 */
			log_dcf77(" 1st DCF77 correct\n");

			/*
			 * Take over the NewTime as OldTime for the next iteration
			 */
			DCF.OldTime = NewTime;

			/*
			 * Reset the internal state of this module
			 */
			dcf77_reset();

			return false;

		}

	}

	return false;

}

/**
 * @brief Initializes the DCF77 module
 *
 * This function initializes the DCF77 port. Among other things it sets the
 * data direction registers for the input and output. Furthermore some flags
 * are set and/or cleared to trigger further initilization tasks, such as the
 * detection of the module type.
 *
 * @see dcf77_check_module_type()
 * @see FLAGS_e
 */
void dcf77_init(void)
{

	/*
	 * Set up DCF input pin
	 */
	DDR(DCF_INPUT) &= ~_BV(BIT(DCF_INPUT));
	PORT(DCF_INPUT) &= ~_BV(BIT(DCF_INPUT));

	/*
	 * Set up DCF output in
	 */
	DDR(DCF_OUTPUT) |=  _BV(BIT(DCF_OUTPUT));
	PORT(DCF_OUTPUT) &= ~_BV(BIT(DCF_OUTPUT));

	/*
	 * Set and/or clear various flags
	 */
	clearFlag(DEFINED);
	setFlag(AVAILABLE);

	/*
	 * Reset this module using dcf77_reset
	 */
	dcf77_reset();

	/*
	 * Reset the time of last successfully received time frame to 0
	 */
	DCF.OldTime = 0;

}

/**
 * @brief ISR counting the pause length between two pulses
 *
 * This ISR needs to be called every 10 ms. This is achieved by putting it
 * into the macro INTERRUPT_100HZ. It will then count the pause between
 * two pulses, which in return makes it possible to determine the length of the
 * pulse itself and therefore decode the signal afterwards.
 *
 * If a transition from high to low is registered, the CHECK flag is set, so
 * that dcf77_check() will be called to decode the received information.
 *
 * Furthermore it sets the output pin according to the input pin.
 *
 * @see dcf77_check()
 * @see FLAGS_e
 * @see INTERRUPT_100HZ
 */
void dcf77_ISR(void)
{

	/*
	 * Check whether receiver is actually available
	 */
	if (getFlag(AVAILABLE)) {

		/*
		 * Check whether receiver type has already been determined
		 */
		if (!(getFlag(DEFINED))) {

			/*
			 * Check whether high or low level is is on the DCF77 input pin and
			 * increase the appropriate counter
			 */
			if (!(PIN(DCF_INPUT) & _BV(BIT(DCF_INPUT)))) {

				count_low++;

			} else {

				count_high++;

			}

			/*
			 * Determine the receiver type using dcf77_check_module_type()
			 */
			dcf77_check_module_type();

		} else {

			/*
			 * Receiver type has already been determined
			 */

			/*
			 * Check whether the receiver is enabled after all
			 */
			if (enable_dcf77_ISR) {

				uint8_t dcf_signal;

				/*
				 * Check which type of receiver is used and negate the input
				 * pin if necessary. Apply this value to dcf_signal.
				 */
				if (getFlag(HIGH_ACTIVE)) {

					dcf_signal = (PIN(DCF_INPUT) & _BV(BIT(DCF_INPUT)));

				} else {

					dcf_signal = !(PIN(DCF_INPUT) & _BV(BIT(DCF_INPUT)));

				}

				/*
				 * Check if signal is high
				 */
				if (dcf_signal) {

					/*
					 * Increase pause counter
					 */
					DCF.PauseCounter++;

					/*
					 * Disable DCF77 output
					 */
					PORT(DCF_OUTPUT) &= ~_BV(BIT(DCF_OUTPUT));

				} else {

					/*
					 * DCF signal is low
					 */

					/*
					 * Set check flag to analyze signal
					 */
					setFlag(CHECK);

					/*
					 * Enable DCF output
					 */
					PORT(DCF_OUTPUT) |= _BV(BIT(DCF_OUTPUT));

				}

			}

		}

	}

}

/**
 * @brief Puts the received date & time into a buffer
 *
 * When a valid pulse has been received, this functions puts the current date
 * & time into a buffer of type datetime_t and returns true. Otherwise it will
 * return false and do nothing to the buffer at all.
 *
 * @param DateTime_p Pointer to buffer where the resulting date & time should
 * 		  be stored
 * @return True if date & time has been put into buffer, false otherwise
 * @see FLAGS_e
 */
bool dcf77_getDateTime(datetime_t * DateTime_p)
{

	/*
	 * Check whether module is available and whether a full pulse has just
	 * been received
	 */
	if (getFlag(AVAILABLE) && getFlag(CHECK)) {

		/*
		 * Check whether a full valid time frame has been received
		 */
		if (dcf77_check()) {

			/*
			 * Take over the time just broadcasted over DCF77
			 */
			DateTime_p->ss = 0;
			DateTime_p->mm = DCF.NewTime[0];
			DateTime_p->hh = DCF.NewTime[1];
			DateTime_p->DD = DCF.NewTime[2];
			DateTime_p->wd = DCF.NewTime[3];
			DateTime_p->MM = DCF.NewTime[4];
			DateTime_p->YY = DCF.NewTime[5];

			/*
			 * Reset this module
			 */
			dcf77_reset();

			/*
			 * Disable DCF77. Since the time now has been synchronized the
			 * DCF77 module gets deactivated. It will be reenabled by the
			 * main program once every hour.
			 */
			enable_dcf77_ISR = false;

			/*
			 * Deactivate the output since the ISR is deactivated, too
			 */
			PORT(DCF_OUTPUT) &= ~_BV(BIT(DCF_OUTPUT));

			return true;

		}

		/*
		 * Clear the CHECK flag to indicate that the module should continue
		 * to analyze the time signal.
		 */
		clearFlag(CHECK);

	}

	return false;

}

#endif /** (DCF_PRESENT == 1) */
