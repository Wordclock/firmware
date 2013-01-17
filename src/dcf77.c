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
 * @brief Implementation of the interface declared in dcf77.h
 *
 * This file contains the implementation of the interface as it is declared
 * in dcf77.h. In order to understand the source code completely, a basic
 * understanding of the DCF77 signal is needed. Take a look at the Wikipedia
 * [article](https://en.wikipedia.org/wiki/DCF77) for a detailed description
 * of the concept and the time signal itself.
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

#if (DCF_PRESENT == 1)



#if  (LOG_DCF77 == 1)
#define log_dcf77(x) uart_puts_P(x)
#else
#define log_dcf77(x)
#endif
/* ********************************************************************************* */
uint8_t DCF_FLAG;

/**
 * @brief Contains various flags used within this module
 *
 * This flags are mainly used to enable a basic form of communication between
 * various functions of this module. To save some space they are combined into
 * an enumeration.
 *
 * @see getFlag()
 * @see setFlag()
 * @see clearFlag()
 */
typedef enum FLAGS_e{
   CHECK = 0, /**< Indicates whether a full time frame has been received */
   DEFINED, /**< Indicates whether the module type has already been determined */
   AVAILABLE, /**< Indicates whether or not there actually is a DCF77 module */
   HIGH_ACTIVE, /**< Indicates whether the DCF77 module is high or low active */

   _FLAG_COUNT /**< Flag counter to ensure that enumeration is not too big */
}FLAGS;

#if (_FLAG_COUNT>8)
#  error To much data for Flag register
#endif

static inline bool getFlag(FLAGS flag){ return DCF_FLAG & _BV(flag); }
static inline void setFlag(FLAGS flag)   { DCF_FLAG |= _BV(flag);       }
static inline void clearFlag(FLAGS flag) { DCF_FLAG &= ~_BV(flag);      }

/* ********************************************************************************* */
// Input definition of DCF Modul
#define DCF_INPUT PORTB, 7

// Output definition of DCF Signal (Control - LED)
#define DCF_OUTPUT PORTD, 4

/**
 * @brief Type definition containing variables needed for decoding
 *
 * These variables are needed in order to successfully decode the DCF77 time
 * signal. They are combined in a struct.
 */
volatile typedef struct{
  uint8_t   PauseCounter; /**< Counter for the pause length, +1 for each 10 ms */
  uint8_t   BitCounter; /**< Indicates which bit is currently being broadcasted */
  uint8_t   Parity; /**< Parity bit counter */
  uint8_t   BCDShifter; /**< Shift counter for decoding BCD */
  uint8_t   NewTime[6]; /**< Stores date & time while receiving DCF signal */
  uint8_t   NewTimeShifter; /**< Identifies which data is currently being broadcasted */
  uint8_t   OldTime; /**< Stores the last successful received time */
} DCF_Struct;

static DCF_Struct             DCF;
bool                          enable_dcf77_ISR;                                 // En- / Disable DCF77 examination
static uint8_t                count_low;
static uint8_t                count_high;

const static uint8_t          BCD_Kodierung[] = { 1,  2,  4,  8, 10, 20, 40, 80 };
/**
 * @brief Resets the state of the DCF77 module
 *
 * This function resets various variables used to keep track of the DCF77 time
 * signal. It gets usually called when an error during the reception gets
 * detected.
 *
 * @see DCF_Struct
 */
static void
dcf77_reset(void)
{
  uint8_t i;
  log_dcf77("DCF77 Reset\n");
  DCF.Parity          = 0;                                                      // Count of high pulse must be even
  DCF.PauseCounter    = 0;                                                      // Count length of pause to identify High/Low Signal
  DCF.BCDShifter      = 0;                                                      // transform transfered data from BCD to decimal
  DCF.BitCounter      = 0;                                                      // Count all received pulses per stream
  DCF.NewTimeShifter  = 0;                                                      // used to identify which data is currently transfered
  setFlag(CHECK);                                                               // TRUE if received DCF77 data stream was valid
  for (i=0; i < 6; i++)
  {
    DCF.NewTime[i]    = 0;                                                      // Store temporary new received data
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
 * This function is only used during the initialization phase. Once the module
 * type was determined, it won't change during runtime anymore.
 *
 * @see FLAGS_e
 */
static void
dcf77_check_module_type(void)
{
  static uint8_t count_pass;
  static uint8_t count_switch;

  if (count_low + count_high >= 100)                                            // one second over?
  {                                                                             // YES ->
    #if (LOG_DCF77 == 1)
    {
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
    }
    #endif
    if ((count_low == 0) || (count_high == 0))                                  //  one of the counters are low?
    {                                                                           //  YES ->
      count_pass++;                                                             //    increase count of passes
      if (count_pass == 20)                                                     //    tried 20 times to identify the signal?
      {                                                                         //    YES ->
        count_pass = 0;
        count_switch++;                                                         //        increase count of activates and deactivaes of the Pull-Up Resistor
        if ((PIN(DCF_INPUT) & _BV(BIT(DCF_INPUT))))                             //      PULL-UP activated?
        {                                                                       //      YES ->
          PORT(DCF_INPUT)           &= ~_BV(BIT(DCF_INPUT));                    //        deactivate Pull-Up Resistor
          log_dcf77(" Pull-UP deactivated\n");
         }
        else
        {                                                                       //      NO ->
          PORT(DCF_INPUT)           |= _BV(BIT(DCF_INPUT));                     //        activate Pull-Up Resistor
          log_dcf77(" Pull-UP activated\n");
        }
        if (count_switch == 30)                                                 //      switched 30 times between active and deactice Pull-Up?
        {                                                                       //      YES ->
          PORT(DCF_INPUT)           |= _BV(BIT(DCF_INPUT));                     //        activate Pull-Up Resistor
          setFlag(DEFINED);                                                     //        DCF Module defined
          clearFlag(AVAILABLE);                                                 //        no DCF Module detected
          log_dcf77("\nNo DCF77 Module detected!\n");
        }
      }
    }
    else
    {                                                                           //  NO ->
      count_pass++;                                                             //        increase count of passes
      if (count_pass == 30)                                                     //    30 passes without a change of the Pull-Up?
      {                                                                         //    YES ->
        setFlag(DEFINED);                                                       //      DCF Module defined
        setFlag(AVAILABLE);                                                     //      DCF Module detected
        #if (LOG_DCF77 == 1)
        {
          if (getFlag(HIGH_ACTIVE))
          {
            log_dcf77("\nhigh active DCF77 Module detected!\n");
          }
          else
          {
            log_dcf77("\nlow active DCF77 Module detected!\n");
          }
        }
        #endif
        return;
      }
      if (count_low > count_high)                                               //    low counter less then high counter?
      {                                                                         //    YES ->
        if (getFlag(HIGH_ACTIVE))                                               //      DCF Module low active?
        {                                                                       //      YES ->
          count_pass = 0;                                                       //        reset count of passes
          clearFlag(HIGH_ACTIVE);
        }
      }
      else
      {                                                                         //    NO ->
        if (!(getFlag(HIGH_ACTIVE)))                                            //      DCF Module high active?
        {                                                                       //      YES ->
          count_pass = 0;                                                       //        reset count of passes
          setFlag(HIGH_ACTIVE);
        }
      }
    }
    count_low   = 0;
    count_high  = 0;
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
static bool
dcf77_check(void)
{
  #if (LOG_DCF77 == 1)
  {
    if (DCF.PauseCounter > 0 )
    {
      char log_text[8];
      byteToStr (DCF.PauseCounter, log_text);
      uart_puts(log_text);
      uart_puts(" ");
    }
  }
  #endif
  if ((DCF.PauseCounter <= 6)                                                   // spike or
      || ((DCF.PauseCounter <= 77)                                              // pause too short and
          && (DCF.PauseCounter >= 96)))                                         // pause too long for data
  {
    DCF.PauseCounter = 0;                                                       // clear pause length counter
    return(false);
  }
  if (((DCF.PauseCounter >= 170)                                                // Sync puls
      && (DCF.BitCounter != 58))                                                // but not 58 bits transfered
      || (DCF.BitCounter >= 59))                                                // or more then 58 Bits
  {
    dcf77_reset();                                                              //   then reset
    return(false);
  }
  if (DCF.BitCounter <= 20)                                                     // Bit 0 - 20 = Weather data
  {
    DCF.BitCounter++;                                                           // increase bit counter
    DCF.PauseCounter = 0;                                                       // clear pause length counter
    return(false);
  }
  if ((DCF.PauseCounter >= 78)
      && (DCF.PauseCounter <= 95))                                              // 0 or 1 detect?
  {
    if (DCF.PauseCounter <= 86)                                                 // 1 detect?
    {
      DCF.Parity++;                                                             // increase parity counter
      if (!((DCF.BitCounter == 28)                                              // not Minutes Parity or
          || (DCF.BitCounter == 35)))                                           // not Hours Parity?
      {
        DCF.NewTime[DCF.NewTimeShifter] +=
            BCD_Kodierung[DCF.BCDShifter];                                      // NewTime Value + BCD-Value
      }
    }
    DCF.BCDShifter++;                                                           // increase BCD Shifter for next data value
    if (((DCF.BitCounter == 28)                                                 // Minutes Parity or
      || (DCF.BitCounter == 35))                                                // Hours Parity and
          && (DCF.Parity%2 != 0))                                               // ParityCount not even?
    {
      dcf77_reset();                                                            //   then reset
      return(false);
    }
    if ((DCF.BitCounter == 28)                                                  // next will be Hours
      || (DCF.BitCounter == 35)                                                 // next will be Day of Month
      || (DCF.BitCounter == 41)                                                 // next will be Day of Week
      || (DCF.BitCounter == 44)                                                 // next will be Month
      || (DCF.BitCounter == 49))                                                // next will be Year
    {
      DCF.NewTimeShifter++;                                                     // increase new time shifter
      DCF.BCDShifter = 0;                                                       // reset BCD Shifter for next data stream
    }
    DCF.BitCounter++;                                                           // increase Bit Counter
    DCF.PauseCounter = 0;                                                       // clear pause length counter
    return(false);
  }
  if (DCF.PauseCounter >= 170)                                                  // sync pause (longer then 1700 ms)?
  {
    if (!(DCF.PauseCounter >= 187))                                             // last Bit = 1 ?
    {
      DCF.Parity++;                                                             //   then Parity + 1
    }
    if ((DCF.BitCounter != 58)                                                  // do not receive 58 bits or
      || (DCF.Parity%2 != 0))                                                   // ParityCount not even?
    {
      dcf77_reset();                                                            //   then reset
      return(false);
    }
    uint8_t NewTime;
    NewTime = DCF.NewTime[0] + DCF.NewTime[1]*60;
    if ((DCF.OldTime + 1) == NewTime)
    {
      log_dcf77(" 2nd DCF77 correct\n");
      return(true);                                                               // Everything fine, received Data could be take over
    }
    else
    {
      log_dcf77(" 1st DCF77 correct\n");
      DCF.OldTime = NewTime;
      dcf77_reset();
      return(false);
    }
  }
  return(false);
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
void
dcf77_init(void)
{
  DDR(DCF_INPUT)            &= ~_BV(BIT(DCF_INPUT));                            // set DCF Pin as input
  PORT(DCF_INPUT)           &= ~_BV(BIT(DCF_INPUT));                            // deactivate Pull-Up Resistor

  DDR(DCF_OUTPUT)           |=  _BV(BIT(DCF_OUTPUT));                           // set Control-LED as output
  PORT(DCF_OUTPUT)          &= ~_BV(BIT(DCF_OUTPUT));                           // set Control-LED to low

  clearFlag(DEFINED);                                                           // DCF Module not defined yet
  setFlag(AVAILABLE);                                                           // TRUE to check if DCF77 module is installed

  dcf77_reset();

  DCF.OldTime = 0;
}
/**
 * @brief ISR counting the pause length between two pulses
 *
 * This ISR needs to be called every 10 ms. It will count the pause between
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
void
dcf77_ISR(void)
{
  if (getFlag(AVAILABLE))                                                       // DCF77 Module available?
  {                                                                             // YES ->
  if (!(getFlag(DEFINED)))                                                      //  Module defined yet?
    {                                                                           //  NO ->
      if (!(PIN(DCF_INPUT) & _BV(BIT(DCF_INPUT))))                              //    DCF77 receiver port HIGH? (low active)
      {                                                                         //    YES ->
        count_low++;                                                            //      increase low counter
      }
      else
      {                                                                         //    NO ->
        count_high++;                                                           //      increase high counter
      }
      dcf77_check_module_type();                                                //    check module type
    }
    else
    {                                                                           //  YES ->
      if (enable_dcf77_ISR)                                                     //    DCF77 analysis enabled?
      {                                                                         //    YES ->
        uint8_t dcf_signal;
        if (getFlag(HIGH_ACTIVE))                                               //      High Active Module?
        {                                                                       //      YES ->
          dcf_signal = (PIN(DCF_INPUT) & _BV(BIT(DCF_INPUT)));                  //        read DCF Signal
        }
        else
        {                                                                       //      NO ->
          dcf_signal = !(PIN(DCF_INPUT) & _BV(BIT(DCF_INPUT)));                 //        read and invert DCF Signal
        }

        if (dcf_signal)                                                         //        DCF77 signal HIGH?
        {                                                                       //        YES ->
          DCF.PauseCounter++;                                                   //          increment PauseCounter
          PORT(DCF_OUTPUT) &= ~_BV(BIT(DCF_OUTPUT));                            //          deactivate Control-LED
        }
        else
        {                                                                       //        NO ->
          setFlag(CHECK);                                                       //          Set DCF.Check (Pulse has ended)
          PORT(DCF_OUTPUT) |= _BV(BIT(DCF_OUTPUT));                             //          activate Control-LED
        }
      }
    }
  }
}


/**
 * @brief Puts the received date & time into a buffer
 *
 * When a valid time frame was received, this functions puts the current date
 * & time into a buffer of type datetime_t and returns true. Otherwise it will
 * return false and do nothing to the buffer.
 *
 * @param DateTime_p Pointer to a buffer where the resulting date & time is stored
 * @return True if date & time has been put into buffer, false otherwise
 * @see FLAGS_e
 */
bool
dcf77_getDateTime(datetime_t * DateTime_p)
{
  if (getFlag(AVAILABLE) && getFlag(CHECK))                                     // DCF Module available and full pulse received?
  {                                                                             // YES ->
      if (dcf77_check())                                                        //    received data are correct?
      {                                                                         //    YES ->
        DateTime_p->ss  = 0;                                                    //      then take over the DCF-Time
        DateTime_p->mm  = DCF.NewTime[0];
        DateTime_p->hh  = DCF.NewTime[1];
        DateTime_p->DD  = DCF.NewTime[2];
        DateTime_p->wd  = DCF.NewTime[3];
        DateTime_p->MM  = DCF.NewTime[4];
        DateTime_p->YY  = DCF.NewTime[5];
        dcf77_reset();                                                          //      Reset Variables
        enable_dcf77_ISR = false;                                               //      Clear enable_dcf77_ISR
        PORT(DCF_OUTPUT) &= ~_BV(BIT(DCF_OUTPUT));                              //      deactivate Control-LED
        return (true);
      }                                                                         //    NO ->
      clearFlag(CHECK);                                                         //      do nothing and return false
  }
  return (false);
}
/*---------------------------------------------------------------------------------------------------------------------------------------------------
 * dcf77: END
 *---------------------------------------------------------------------------------------------------------------------------------------------------
 */

#endif  /** (DCF_PRESENT == 1) */


