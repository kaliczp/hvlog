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

int main(void)
{
  Configure_GPIO_Sensor();
  Configure_GPIO_LED();
  Configure_RTC();
  Init_RTC(0);
  while(1)
    {
      if(SensedData == 1)
	{
	  SensedData = 0;
	  GPIOA->ODR ^= (1 << 5); //toggle LED
	}
    }
}
