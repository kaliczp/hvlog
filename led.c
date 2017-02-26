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
   - Configures output GPIO PA5
*/
void Configure_GPIO_LED(void)
{
  /* (1) Enable the peripheral clock of GPIOA */
  /* (2) Select output mode (01) on GPIOA pin 5 */
  /* (3) Select output mode (01) on GPIOA pin 6 */
  /* (4) Set GPIOA pin 5 */
  RCC->IOPENR |= RCC_IOPENR_GPIOAEN; /* (1) */  
  GPIOA->MODER = (GPIOA->MODER & ~(GPIO_MODER_MODE5)) 
    | (GPIO_MODER_MODE5_0); /* (2) */
  GPIOA->MODER = (GPIOA->MODER & ~(GPIO_MODER_MODE6)) 
    | (GPIO_MODER_MODE6_0); /* (3) */
  /* lit green LED */
  GPIOA->BSRR = GPIO_BSRR_BS_5; /* (4) */
}
