/*
**********************************************************************
* Author    Péter Kalicz
* Version   V0.1
* Date      2017-02-11
* Brief     Test the behavior of Makefile and programming environment

hvlog -- a simple logger based on STM32L0x1 MCU and an EEPROM
Copyright (C) 2017 Péter Kalicz

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>
*/

#include "main.h"

volatile uint32_t MyStateRegister;

/* Timestamp values */
volatile uint32_t TimestampTime;
volatile uint32_t TimestampDate;

uint32_t OldTimestampTime;
uint32_t OldTimestampDate;

uint8_t FromLowPower;

uint8_t ToEEPROM[TO_EPR_LENGTH] = {WRITE, 0x0, 0x0, 0x0, 0x17, 0x03, 0x15};

int main(void)
{
  FromLowPower = 0;
  RCC->APB1ENR |= RCC_APB1ENR_PWREN; // Enable PWR
  Enable_RTC_registers();
  /* If RTC is not set configure and initialise */
  if((PWR->CSR & PWR_CSR_SBF) == 1)
    {
      PWR->CR |= PWR_CR_CSBF;
      PWR->CR |= PWR_CR_CWUF;
      FromLowPower = 1;
    }
  else
    {
      Configure_RTC_Clock();
    }
  RCC->APB1ENR &=~ RCC_APB1ENR_PWREN; // Disable PWR
  if(FromLowPower == 0)
    {
      Configure_RTC_Func();
      Init_RTC(CURR_TIM, CURR_DAT);
    }
  /* Important variables. Loaded from RTC domain */
  /* Older timestamp values */
  OldTimestampTime = RTC->BKP1R;
  OldTimestampDate = RTC->BKP2R;

  while(1)
    {
      if(MyStateRegister > 0)
	{
	  if((MyStateRegister & (TIMESTAMP_CAPTURED)) == (TIMESTAMP_CAPTURED))
	    {
	      MyStateRegister &= ~TIMESTAMP_CAPTURED;
	      OldTimestampTime = TimestampTime;
	      OldTimestampDate = TimestampDate;
	      ToEEPROM[4] = (TimestampTime >> 16) & 0xFF;
	      ToEEPROM[5] = (TimestampTime >> 8) & 0xFF;
	      ToEEPROM[6] = TimestampTime & 0xFF;
	      RTC->BKP1R = OldTimestampTime;
	      RTC->BKP2R = OldTimestampDate;
	      MyStateRegister |= SPI_SAVEROM;
	    }
	  else if((MyStateRegister & (DAILY_ALARM)) == (DAILY_ALARM))
	    {
	      MyStateRegister &= ~DAILY_ALARM;
	      // Write internal EEPROM with SPI EEPROM pointer and date
	    }
	  else if((MyStateRegister & (SPI_SAVEROM)) == (SPI_SAVEROM))
	    {
	      MyStateRegister &= ~SPI_SAVEROM;
	      // Save data to SPIEEPROM
	      ToEEPROM[0] = WREN;
	      Write_SPI(ToEEPROM, 1);
	      ToEEPROM[0] = WRITE;
	      Write_SPI(ToEEPROM, 7);
	      ToEEPROM[2]++;
	      // Status Reg
	      ToEEPROM[0] = RDSR;
	      Write_SPI(ToEEPROM, 2);
	      ConfigureLPTIM1(98);
	      __WFI();
	      DeconfigureLPTIM1();
	      // Status Reg
	      ToEEPROM[0] = RDSR;
	      Write_SPI(ToEEPROM, 2);
	    }
	  else
	    {
	      MyStateRegister = NOTHING_TODO;
	    }
	}
      else
	{
	  /* Go to low power */
	  Configure_Lpwr(ModeSTOP);
	}
    }
}
