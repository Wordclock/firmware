/*
 * Copyright (C) 2012, 2013 Karol Babioch <karol@babioch.de>
 * Copyright (c) 2012 Uwe Höß
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

/*------------------------------------------------------------------------------------------------------------------------------------------------*//**
 * @file display_wc_ger3.c
 * 
 *  This files implements the german language specific.
 *
 * \version $Id: display_wc_ger3.c 425 2013-03-14 19:05:31Z vt $
 * 
 * \author Copyright (c) 2012 Vlad Tepesch    
 * \author Copyright (c) 2012 Uwe Höß
 * 
 * \remarks
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 */
 /*-----------------------------------------------------------------------------------------------------------------------------------------------*/


#include <inttypes.h>
#include <avr/io.h>
#include <util/delay.h>
#include <avr/pgmspace.h>

#include "main.h"
#include "base.h"
#include "display.h"
#include "shift.h"

#include "wceeprom.h"

#include "simple_random.h"



#if (WC_DISP_GER3 == 1)

//   following table will be implemented
//   the bit counts define the position and width in the mode number
//
//        0:  h Uhr
//        5:  5 nach h                  0bit
//        10: 10 nach h
//            20 vor halb h+1           1bit
//        15: viertel nach h
//            viertel h+1
//            viertel vor halb h+1     
//            dreiviertel vor h+1
//            dreiviertel nach halb h   3bit
//        20: 10 vor halb h+1
//            20 nach h                 1bit
//        25: 5 vor halb h+1            0bit
//        30: halb h+1   
//            halb nach h               1bit
//        35: 5 nach halb h+1           0bit
//        40: 10 nach halb h+1
//            20 vor h+1                1bit
//        45: viertel vor h+1
//            dreiviertel h+1
//            dreiviertel nach h
//            viertel nach halb h+1   
//            dreiviertel vor halb h+2  3bit
//        50: 10 vor h+1
//            20 nach halb h+1          1bit
//        55: 5 vor h+1                 0bit
//                                     -----
//                                     11bit
// -->  5 bits left for identify special modes
//      e.g switch mode,  jester mode



#define DISP_SETBIT(x) ( 1 <<( (x) - DWP_MIN_FIRST))

/**
 * array that contains the minute part of all the possible ways to display 
 * a correct (but not allways sensible) time
 *
 * \remark
 *     if you change this section do not forget to change all the other look up tables too
 *
 * \todo make PROGMEM if more ram is needed (+10B Progmem)
 */
static const uint8_t s_minData[] =  {

    (DISP_SETBIT(DWP_fuenfMin)   | DISP_SETBIT(DWP_nach)                           ), // 5
                                                           
    (DISP_SETBIT(DWP_zehnMin)    | DISP_SETBIT(DWP_nach)                           ), // 10
    (DISP_SETBIT(DWP_zwanzigMin) | DISP_SETBIT(DWP_vor )   | DISP_SETBIT(DWP_halb) ),
                                                         
    (DISP_SETBIT(DWP_viertel)    | DISP_SETBIT(DWP_nach)                           ), // 15
    (DISP_SETBIT(DWP_viertel)                                                      ),
    (DISP_SETBIT(DWP_viertel)    | DISP_SETBIT(DWP_vor )   | DISP_SETBIT(DWP_halb) ),
    (DISP_SETBIT(DWP_dreiMin)    | DISP_SETBIT(DWP_viertel)| DISP_SETBIT(DWP_vor)  ),
    (DISP_SETBIT(DWP_dreiMin)    | DISP_SETBIT(DWP_viertel)| DISP_SETBIT(DWP_nach) | DISP_SETBIT(DWP_halb) ),

    (DISP_SETBIT(DWP_zwanzigMin) | DISP_SETBIT(DWP_nach)                           ), // 20
    (DISP_SETBIT(DWP_zehnMin)    | DISP_SETBIT(DWP_vor )   | DISP_SETBIT(DWP_halb) ), 

    (DISP_SETBIT(DWP_fuenfMin)   | DISP_SETBIT(DWP_vor )   | DISP_SETBIT(DWP_halb) ), // 25

    (DISP_SETBIT(DWP_halb)                                                         ), // 30
    (DISP_SETBIT(DWP_halb)       | DISP_SETBIT(DWP_nach)                           ), 

    (DISP_SETBIT(DWP_fuenfMin)   | DISP_SETBIT(DWP_nach)   | DISP_SETBIT(DWP_halb) ), // 35

    (DISP_SETBIT(DWP_zehnMin)    | DISP_SETBIT(DWP_nach)   | DISP_SETBIT(DWP_halb) ), // 40
    (DISP_SETBIT(DWP_zwanzigMin) | DISP_SETBIT(DWP_vor )                           ),

    (DISP_SETBIT(DWP_viertel)    | DISP_SETBIT(DWP_vor )                           ), // 45
    (DISP_SETBIT(DWP_dreiMin)    | DISP_SETBIT(DWP_viertel)                        ),
    (DISP_SETBIT(DWP_dreiMin)    | DISP_SETBIT(DWP_viertel)| DISP_SETBIT(DWP_nach) ),
    (DISP_SETBIT(DWP_viertel)    | DISP_SETBIT(DWP_nach)   | DISP_SETBIT(DWP_halb) ),
    (DISP_SETBIT(DWP_dreiMin)    | DISP_SETBIT(DWP_viertel)| DISP_SETBIT(DWP_vor)  | DISP_SETBIT(DWP_halb) ),

    (DISP_SETBIT(DWP_zehnMin)    | DISP_SETBIT(DWP_vor )                           ), // 50
    (DISP_SETBIT(DWP_zwanzigMin) | DISP_SETBIT(DWP_nach)   | DISP_SETBIT(DWP_halb) ), 

    (DISP_SETBIT(DWP_fuenfMin)   | DISP_SETBIT(DWP_vor )                           ), // 55

};
#undef DISP_SETBIT



// the following two bitsets define on which entries of s_minData
// an hour increment is neccessary
//
//                                    55 4    4 33 22 1    1  
//                                    50 5    0 50 50 5    0 5 
static const uint32_t s_hourInc1st= 0b111110111110111001110100;    // todo use compiler independent constants
static const uint32_t s_hourInc2nd= 0b000100000000000000000000;



/**
 *  defines the start index for variants for each 5-min-time-slice in s_minData 
 */
static const uint8_t s_minStartInd[] = {
   0, //  5  // his is obviously allwas 0, but special handling in code would eat up more than this byte
   1, // 10
   3, // 15
   8, // 20
  10, // 25
  11, // 30
  13, // 35
  14, // 40
  16, // 45
  21, // 50
  23, // 55
};


/** helper macro for easier definition of bit offset and bit masks */
#define MASK_SHIFT(numBits, bitOffset)\
  ((( ((numBits)==0)?0:((numBits==1)?1:((numBits==2)?0x3:((numBits==3)?0x7:0xF ) ) ) )<<4) | bitOffset)

/**
 * defines the position of subindeces for each 5-min-time-slice
 *
 * lower nibble:  bit offset,\n
 * higher nibble: mask to apply after shifting the index to bit 0
 */
static const uint8_t s_modeShiftMask[] = {
  MASK_SHIFT(0, 0), //  5  no variants
  MASK_SHIFT(1, 0), // 10   2  variants
  MASK_SHIFT(3, 1), // 15   5  variants
  MASK_SHIFT(1, 4), // 20   2  variants
  MASK_SHIFT(0, 0), // 25  no variants
  MASK_SHIFT(1, 5), // 30   2 variants
  MASK_SHIFT(0, 0), // 35  no variants
  MASK_SHIFT(1, 6), // 40   2 variants
  MASK_SHIFT(3, 7), // 45   5 variants
  MASK_SHIFT(1,10), // 50   2 variants
  MASK_SHIFT(0, 0), // 55  no variants
};
#undef MASK_SHIFT

static const uint8_t s_minVariants[] = {
   1,    //  5 
   2,    // 10 
   5,    // 15 
   2,    // 20 
   1,    // 25 
   2,    // 30 
   1,    // 35 
   2,    // 40 
   5,    // 45 
   2,    // 50 
   1,    // 55 
};

#define SELECT_MODE( i5, i10, i15, i20, i25, i30, i35, i40, i45, i50, i55 )\
  ((i10) | ((i15)<<1) | ((i20)<<4) | ((i30)<<5) | ((i40)<<6) | ((i45)<<7) | ((i50)<<10))

#define JESTER_MODE 0xffFF

/**
 *  the modes that can be selected with remote control
 */
static const uint16_t s_modes[] = 
{
//            5 10 15 20 25 30 35 40 45 50 55
  SELECT_MODE(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0), // wessi
  SELECT_MODE(0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0), // rhein ruhr
  SELECT_MODE(0, 0, 1, 0, 0, 0, 0, 0, 1, 0, 0), // ossi
  SELECT_MODE(0, 0, 1, 1, 0, 0, 0, 1, 1, 0, 0), // schwabe
# if (DISPLAY_ADD_JESTER_MODE==1)
  JESTER_MODE
# endif
};
#undef SELECT_MODE



#define DISP_SETBIT(x) ( 1L <<( (x) ))
/**
 *  defines the display states for the hours
 */
const uint16_t s_numbers[12] = {
  ( DISP_SETBIT(DWP_zwoelf)                                              ),
  ( DISP_SETBIT(DWP_ei)       | DISP_SETBIT(DWP_n)  | DISP_SETBIT(DWP_s) ),
  ( DISP_SETBIT(DWP_zw)       | DISP_SETBIT(DWP_ei)                      ),
  ( DISP_SETBIT(DWP_drei)                                                ),
  ( DISP_SETBIT(DWP_vier)                                                ),
  ( DISP_SETBIT(DWP_fuenf)                                               ),
  ( DISP_SETBIT(DWP_sechs)                                               ),
  ( DISP_SETBIT(DWP_s)        | DISP_SETBIT(DWP_ieben)                   ),
  ( DISP_SETBIT(DWP_acht)                                                ),
  ( DISP_SETBIT(DWP_neun)                                                ),
  ( DISP_SETBIT(DWP_zehn)                                                ),
  ( DISP_SETBIT(DWP_elf)                                                 ),
};
#undef DISP_SETBIT


static uint8_t isJesterModeActive(const datetime_t* i_dateTime, uint8_t i_langmode)
{
# if (DISPLAY_ADD_JESTER_MODE==1)
#   if( (DCF_PRESENT==1) && (DISPLAY_USE_JESTER_MODE_ON_1ST_APRIL==1) )
      return    (i_langmode == tm_jesterMode)
             || (i_dateTime->MM == 4  && i_dateTime->DD == 1);
#   else
      return (i_langmode == tm_jesterMode);
#   endif
# else
#   if( (DCF_PRESENT==1) && (DISPLAY_USE_JESTER_MODE_ON_1ST_APRIL==1) )
      return  (i_dateTime->MM == 4  && i_dateTime->DD == 1);
#   else
      return 0;
#   endif
# endif
}



DisplayState display_getTimeState (const datetime_t* i_newDateTime)
{
  uint8_t hour       = i_newDateTime->hh;
  const uint8_t minutes    = i_newDateTime->mm/5;
  const uint8_t minuteLeds = i_newDateTime->mm%5;
  uint8_t minuteLedSubState = 0;



#if DISPLAY_DEACTIVATABLE_ITIS == 1
  uint32_t leds     = 0;
  const uint8_t  langMode   = g_displayParams->mode/2;
  if(   ((g_displayParams->mode & 1) == 0 ) // "Es ist" zur halb/vollen Stunde oder bei gerader Modusnummer
      || (0 == minutes)
      || (6 == minutes) )
  {    
    leds |= (1L << DWP_itis);
  }
#else
  uint32_t leds     = (1L << DWP_itis);
  const uint8_t  langMode = g_displayParams->mode;
#endif
  const uint8_t  jesterMode = isJesterModeActive(i_newDateTime, langMode);

  if(minutes == 0){
    leds |= (1L << DWP_clock);
  }else{
    const uint8_t  minIndM1    = minutes -1;
    uint8_t  subInd;
    uint8_t  ind;        
    uint32_t hincTestBit;

    if( jesterMode ){
      subInd = simpleRand_get()%s_minVariants[minIndM1];
    }else{
      const uint16_t mode        = s_modes[langMode];
      const uint8_t  shift       = s_modeShiftMask[minIndM1] & 0x0f;
      const uint8_t  mask        = s_modeShiftMask[minIndM1] >> 4;
      subInd      = (mode>>shift)&mask;
    }
    ind         = s_minStartInd[minIndM1] + subInd;
    hincTestBit = ((uint32_t)1) << ind;


    leds |= ((DisplayState)(s_minData[ind])) << DWP_MIN_FIRST;
    if(hincTestBit & s_hourInc1st){
      ++hour;
    }
    if(hincTestBit & s_hourInc2nd){
      ++hour;
    }
  }

  if(jesterMode) 
  {
    uint8_t r = simpleRand_get()%4;
    if(minuteLeds <= 2){ 
      minuteLedSubState |= (1 << (r));        // randomly switch on one minute point
      if(minuteLeds == 2){
        uint8_t r2 = simpleRand_get()%3;
        r2 = r2<r?r2:r2+1;                    
        minuteLedSubState |= (1 << (r2));     // switch on one og the remaining three
      }
    }else{                                    // inverse logic for min>=3
      minuteLedSubState = 0xf;                // all on
      if(minuteLeds == 3){
        minuteLedSubState &= ~(1 << (r));     // randomly switch off one minute point
      }  
    }

  }else{
    if(minuteLeds >= 4){
        minuteLedSubState |= (1 << (DWP_min4-DWP_MIN_LEDS_BEGIN));
    }
    if(minuteLeds >= 3){
        minuteLedSubState |= (1 << (DWP_min3-DWP_MIN_LEDS_BEGIN));
    }
    if(minuteLeds >= 2){
        minuteLedSubState |= (1 << (DWP_min2-DWP_MIN_LEDS_BEGIN));
    }
    if(minuteLeds >= 1){
        minuteLedSubState |= (1 << (DWP_min1-DWP_MIN_LEDS_BEGIN));
    }
  }

  leds |= ((DisplayState) minuteLedSubState) << DWP_MIN_LEDS_BEGIN;
 
  leds |= display_getNumberDisplayState(hour);

  if(   (hour==1 || hour==13 )   // if "Es ist ein Uhr" <- remove 's' from "eins"
     && (minutes==0))
  { 
    leds &= ~(1L << DWP_s); 
  }

  return leds;

}

#endif /* WC_DISP_GER3 */
