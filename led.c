/*
**********************************************************************
* Author    Péter Kalicz
* Version   V0.1
* Date      2017-02-11
* Brief     Sensor with interrupt

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

#include "led.h"


/**
   - GPIO clock enable
   - Configures output GPIO PA3
*/
void Configure_GPIO_LED(void)
{
  /* (1) Enable the peripheral clock of GPIOA */
  /* (2) Select output mode (01) on GPIOA pin 3 */
  RCC->IOPENR |= RCC_IOPENR_GPIOAEN; /* (1) */  
  GPIOA->MODER = (GPIOA->MODER & ~(GPIO_MODER_MODE3)) 
    | (GPIO_MODER_MODE3_0); /* (2) */  
}
