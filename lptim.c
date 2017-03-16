/*
**********************************************************************
* Author    Péter Kalicz
* Version   V0.1
* Date      2017-02-11
* Brief     LowPower Timer

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

#include "lptim.h"

void ConfigureLPTIM1(void)
{
  /* (1a) Select clocksource for LPTimer1  RM0377 205p*/
  /* (1b) Enable the peripheral clock of LPTimer1 RM0377 198p*/
  /* (1c) Enable the peripheral clock of LPTimer1 in sleep RM0377 203p*/

  RCC->CCIPR |= RCC_CCIPR_LPTIM1SEL; /* (1a) */
  RCC->APB1ENR |= RCC_APB1ENR_LPTIM1EN; /* (1b) */
  RCC->APB1SMENR |= RCC_APB1SMENR_LPTIM1SMEN; /* (1c) */

  /* (2) Enable interrupt on Autoreload match */
  /* (3) Enable LPTimer  */
  /* (4) Set Autoreload to 163 in order to get an interrupt after 164 pulses
         with 32kHz means almost 5ms */
  LPTIM1->IER |= LPTIM_IER_ARRMIE; /* (2) */
  LPTIM1->CR |= LPTIM_CR_ENABLE; /* (3) */
  LPTIM1->ARR = 163; /* (4) */
  LPTIM1->CR |= LPTIM_CR_SNGSTRT; /* start the counter in single */

  /* Configure EXTI and NVIC for LPTIM1 */
  /* (5) Configure extended interrupt for LPTIM1 */
  /* (6) Enable Interrupt on LPTIM1 */
  /* (7) Set priority for LPTIM1 */
  EXTI->IMR |= EXTI_IMR_IM29; /* (5) */
  NVIC_EnableIRQ(LPTIM1_IRQn); /* (6) */
  NVIC_SetPriority(LPTIM1_IRQn,4); /* (7) */
}

void DeconfigureLPTIM1(void)
{
  /* (1) Disable LPTimer  */
  LPTIM1->CR &= ~(LPTIM_CR_ENABLE); /* (1) */
}

void LPTIM1_IRQHandler(void)
{
  if ((LPTIM1->ISR & LPTIM_ISR_ARRM) != 0) /* Check ARR match */
  {
    LPTIM1->ICR |= LPTIM_ICR_ARRMCF; /* Clear ARR match flag */
  }
}
