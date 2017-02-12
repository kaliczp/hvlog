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

#include "sensor.h"

/* Sensor indicator */
volatile uint32_t SensedData = 0;

/**
   - GPIO clock enable
   - Configures the Push Button GPIO PA0
*/
void Configure_GPIO_Sensor(void)
{
  /* Enable PA0 input */
  /* (1) Enable GPIOA clock */
  /* (2) Set PA0 input mode */

  RCC->IOPENR |= RCC_IOPENR_GPIOAEN; /* (1) */
  GPIOA->MODER = (GPIOA->MODER & ~(GPIO_MODER_MODE0)); /* (2) */

  /* Configure Syscfg, exti for PA0 */
  /* (3) PA0 as source input (reset value) */
  /* (4) Unmask port0 */
  /* (5) Set rising edge */
  SYSCFG->EXTICR[0] = (SYSCFG->EXTICR[0] & ~SYSCFG_EXTICR1_EXTI0) | SYSCFG_EXTICR1_EXTI0_PA; /* (3) */ 
  EXTI->IMR |= EXTI_IMR_IM0; /* (4) */ 
  EXTI->RTSR |= EXTI_RTSR_TR0; /* (5) */

  /* Configure NVIC */
  /* (6) Set priority */
  /* (7) Enable EXTI0_1_IRQn */
  NVIC_SetPriority(EXTI0_1_IRQn, 0x0); /* (6) */ 
  NVIC_EnableIRQ(EXTI0_1_IRQn); /* (7) */ 
}

/**
   - Handling EXTI IRQ
*/
void EXTI0_1_IRQHandler(void)
{
  if((EXTI->PR & EXTI_PR_PR0) == EXTI_PR_PR0)
    {
      /* Clear EXTI0 flag */
      EXTI->PR = EXTI_PR_PR0;	
	
      /* Sensed flag set */
      SensedData = 1;
    }
}
