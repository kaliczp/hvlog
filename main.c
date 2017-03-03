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

int main(void)
{
  /* Important variables. Later save to RTC domain and load here */
  /* Status register to follow state */
  MyStateRegister = 0;
  /* Older timestamp values */
  OldTimestampTime = 0x00;
  OldTimestampDate = 0x00;

  Configure_GPIO_LED();
  Configure_RTC();
  Init_RTC(0);
  while(1)
    {
      if(MyStateRegister == TIMESTAMP_CAPTURED)
	{
	  MyStateRegister = NOTHING_TODO;
	  OldTimestampTime = TimestampTime;
	  OldTimestampDate = TimestampDate;
	  GPIOA->ODR ^= (1 << 6); //toggle LED
	}
      /* Go to sleep */
      __WFI();
    }
}
