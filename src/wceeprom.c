/*
 * Copyright (C) 2012, 2013 Karol Babioch <karol@babioch.de>
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
 * @file wceeprom.c
 * 
 *  This file implements the handling of the persistent data of the wordclock modules
 *
 * \version $Id: wceeprom.c 285 2010-03-24 21:43:24Z vt $
 * 
 * \author Copyright (c) 2010 Vlad Tepesch    
 * 
 * \remarks
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 */
 /*-----------------------------------------------------------------------------------------------------------------------------------------------*/

#include <avr/eeprom.h>
#include <avr/pgmspace.h>
#include <avr/interrupt.h>

#include "wceeprom.h"
#include "uart.h"
#include "base.h"


#if (LOG_EEPROM_WRITEBACK == 1)
#  define   log_eeprom(x)  uart_puts_P(x)
#else
#  define   log_eeprom(x)  
#endif

WcEepromData EEMEM eepromParams;

const WcEepromData PROGMEM pm_eepromDefaultParams = {
  /* .userParams    = */ USEREEPROMPARAMS_DEFAULT,
  /* .displayParams = */ DISPLAYEEPROMPARAMS_DEFAULT,
  /* .pwmParams     = */ PWMEEPROMPARAMS_DEFAULT,
  /* .swVersion     = */ SW_VERSION,
  /* .structSize    = */ ((uint8_t)sizeof(WcEepromData)),
};

WcEepromData g_epromWorking; 


# if (LOG_EEPROM_INIT == 1) || (LOG_EEPROM_WRITEBACK == 1)
static void uart_putHexByte(uint8_t byte)
{
  uart_putc( nibbleToHex(byte >> 4 ) );
  uart_putc( nibbleToHex(byte & 0xf ) );
}
#endif

/**
 * @brief Initializes this module by reading data from EEPROM and keeping a
 *   copy of it in memory
 *
 * This has to be called **before** any other functions of this module can be
 * used. It copies the data from EEPROM into memory and makes it accessible
 * with wcEeprom_getData(). Changes to the data done in memory can be written
 * back to be stored persistently using wcEeprom_writeback().
 *
 * @see wcEeprom_getData()
 * @see wcEeprom_writeback()
 */
void wcEeprom_init(void)
{
  eeprom_read_block(&g_epromWorking, &eepromParams, sizeof(eepromParams));
  if(    (g_epromWorking.swVersion  != SW_VERSION)
      || (g_epromWorking.structSize != sizeof(g_epromWorking)))
  {
#   if (LOG_EEPROM_INIT == 1)
    uart_puts_P("Using defaults instead eeprom\n");
#   endif
    memcpy_P(&g_epromWorking, &pm_eepromDefaultParams, sizeof(WcEepromData));
  }
# if (LOG_EEPROM_INIT == 1)
  {
    uint8_t i = 0;
    uint8_t* ptr = (uint8_t*)(&g_epromWorking);
    uart_puts_P("eeprom: ");
    for(; i<sizeof(eepromParams); ++i){
      uart_putHexByte(*ptr);
      ++ptr;
    }
    uart_putc('\n');
  }
# endif
}


static uint8_t wcEeprom_writeIfChanged(uint8_t index)
{
  uint8_t eepromByte;
  uint8_t sdramByte;
  uint8_t* eepromAdress = ( (uint8_t*)&eepromParams) + index ;
  
  eepromByte = eeprom_read_byte( eepromAdress );
  sdramByte  =  *(((uint8_t*)&g_epromWorking)+index);
  if( eepromByte != sdramByte )
  {
#   if (LOG_EEPROM_WRITEBACK == 1)
    {
      char buf[5];
      uart_puts_P("EEPROM Byte ");
      uint16ToHexStr((uint16_t)eepromAdress, buf);
      uart_puts(buf);
      uart_puts_P(" differs ");
      uart_putHexByte(eepromByte);
      uart_putc('\t');
      uart_putHexByte(sdramByte);
      uart_putc('\n');
    }
#   endif

    eeprom_write_byte(eepromAdress, sdramByte);
    return 1;
  }
  return 0;
}

/**
 * @brief Writes changes done to the WcEepromData instance in RAM into EEPROM
 *
 * As stated in the description of WcEepromData there are basically two
 * "instances" of this variable. One in RAM and one in EEPROM, which the one
 * in RAM is based on. If any changes are done to the instance in RAM this
 * function has to be called in order for the changes to be written back to
 * EEPROM so they are stored persistently.
 *
 * Instead of writing the whole struct into EEPROM each and every time again,
 * it is also possible to only write back bytes that actually have changed,
 * which not only saves space, but also helps to increase the expected lifetime
 * of the EEPROM cells involved.
 *
 * @warning Because writing to EEPROM takes quite some time it is possible
 * that interrupts will be missed.
 *  *
 * @param start Pointer to the start of the data that has to be written back
 * @param len The length of the data that has to be written back
 *
 * @see WcEepromData
 */
void wcEeprom_writeback(const void* start, uint8_t len)
{
  uint8_t eepromIndex = ( ((uint8_t*)start) - ( (uint8_t*)&g_epromWorking)); 
  uint8_t eepromIndexEnd = eepromIndex + len-1;
# if (LOG_EEPROM_WRITEBACK == 1)
  {
    uart_puts_P("EEpromWrite idx: ");
    uart_putHexByte(eepromIndex);
    uart_puts_P(" len: ");
    uart_putHexByte(len);
    uart_putc('\n');
  }
# endif

  /** @TODO  rewrite this to use interupts because of waiting for eeprom-write finish */
  for(; eepromIndex<=eepromIndexEnd; ++eepromIndex){
    wcEeprom_writeIfChanged( eepromIndex );
  }

}
