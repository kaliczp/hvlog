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

int main(void)
{
  FromLowPower = 0;
  RCC->APB1ENR |= RCC_APB1ENR_PWREN; // Enable PWR
  Enable_RTC_registers();
  /* If RTC is not set configure and initialise */
  if((PWR->CSR & PWR_CSR_WUF) == 1)
    {
      PWR->CR |= PWR_CR_CWUF;
      FromLowPower = 1;
    }
  else
    {
      Configure_RTC();
      Init_RTC(0);
    }
  RCC->APB1ENR &=~ RCC_APB1ENR_PWREN; // Disable PWR
  /* Important variables. Loaded from RTC domain */
  /* Status register to follow state */
  MyStateRegister = RTC->BKP0R;
  /* Older timestamp values */
  OldTimestampTime = RTC->BKP1R;
  OldTimestampDate = RTC->BKP2R;

  while(1)
    {
      if(MyStateRegister == TIMESTAMP_CAPTURED)
	{
	  MyStateRegister = NOTHING_TODO;
	  OldTimestampTime = TimestampTime;
	  OldTimestampDate = TimestampDate;
	  RTC->BKP1R = OldTimestampTime;
	  RTC->BKP2R = OldTimestampDate;
	}
      RTC->BKP0R = MyStateRegister;
      /* Go to sleep */
      if(MyStateRegister == NOTHING_TODO)
	{
	  RCC->APB1ENR |= RCC_APB1ENR_PWREN; // Enable PWR
	  Configure_Lpwr(); // Initialise STOP mode and debug
	  RCC->APB1ENR &=~ RCC_APB1ENR_PWREN; // Disable PWR
	}
      __WFI();
    }
}
