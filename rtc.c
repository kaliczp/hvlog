/*
**********************************************************************
* Author    Péter Kalicz
* Version   V0.1
* Date      2017-02-11
* Brief     RTC config

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

#include "rtc.h"

/**
   - GPIO clock enable
   - Configures the Push Button GPIO PA0
*/
void Configure_RTC(void)
{
  /* (1) Enable PWR clock the peripheral clock RTC */
  /* (2) Enable write in RTC domain control register for LSE change */
  /* (3) Enable the LSE */
  /* (3a) Change LSE driving power if neccessary */
  /* (4) Wait while it is not ready */
  /* (5) LSE for RTC clock */
  /* (6) Disable PWR clock */
  RCC->APB1ENR |= RCC_APB1ENR_PWREN; /* (1) */
  PWR->CR |= PWR_CR_DBP; /* (2) */

  RCC->CSR |= RCC_CSR_LSEON; /* (3) */
  
  //  RCC->CSR |= RCC_CSR_LSEDRV_0; /* (3a) */

  while((RCC->CSR & RCC_CSR_LSERDY)!=RCC_CSR_LSERDY) /* (4) */
  { 
    /* add time out here for a robust application */
  }
  
  RCC->CSR = (RCC->CSR & ~RCC_CSR_RTCSEL) | RCC_CSR_RTCEN | RCC_CSR_RTCSEL_0; /* (5) */
  RCC->APB1ENR &=~ RCC_APB1ENR_PWREN; /* (6) */

  /* Configure RTC */
  /* (7) Write access for RTC regsiters */
  /* (8) Disable alarm A to modify it */
  /* (9) Wait until it is allow to modify alarm A value */
  /* (10) Modify alarm A mask to have an interrupt each 1/60Hz (1 mins) */
  /* (11) Enable alarm A and alarm A interrupt */
  /* (12) Disable write access */
  RTC->WPR = 0xCA; /* (7) */
  RTC->WPR = 0x53; /* (7) */
  RTC->CR &=~ RTC_CR_ALRAE; /* (8) */
  while((RTC->ISR & RTC_ISR_ALRAWF) != RTC_ISR_ALRAWF) /* (9) */
  { 
    /* add time out here for a robust application */
  }
  RTC->ALRMAR = RTC_ALRMAR_MSK4 | RTC_ALRMAR_MSK3 | RTC_ALRMAR_MSK2 ; /* (10) */
  RTC->CR = RTC_CR_ALRAIE | RTC_CR_ALRAE; /* (11) */ 
  RTC->WPR = 0xFE; /* (12) */
  RTC->WPR = 0x64; /* (12) */
  
  /* Configure exti for RTC IT */
  /* (13) unmask line 17 */
  /* (14) Rising edge for line 17 */
  EXTI->IMR |= EXTI_IMR_IM17; /* (13) */ 
  EXTI->RTSR |= EXTI_RTSR_TR17; /* (14) */

  /* Configure NVIC */
  /* (15) Set priority */
  /* (16) Enable RTC_IRQn */
  NVIC_SetPriority(RTC_IRQn, 0x00); /* (15) */ 
  NVIC_EnableIRQ(RTC_IRQn); /* (16) */ 
}

/**
   - Set RTC TIME
*/
void Init_RTC(uint32_t Time)
{
  /* RTC init mode */
  /* Configure RTC */
  /* Ref. man. p. 534 */
  /* (1) Write access for RTC registers */
  /* (2) Enable init phase */
  /* (3) Wait until it is allow to modify RTC register values */
  /* (4) set prescaler, 40kHz/64 => 625 Hz, 625Hz/625 => 1Hz */
  /* (5) New time in TR */
  /* (6) Disable init phase */
  /* (7) Disable write access for RTC registers */
  RTC->WPR = 0xCA; /* (1) */ 
  RTC->WPR = 0x53; /* (1) */
  RTC->ISR |= RTC_ISR_INIT; /* (2) */
  while((RTC->ISR & RTC_ISR_INITF)!=RTC_ISR_INITF) /* (3) */
    { 
    }
  RTC->PRER = 0x003F0270; /* (4) */
  RTC->TR = RTC_TR_PM | Time; /* (5) */
  RTC->ISR &=~ RTC_ISR_INIT; /* (6) */
  RTC->WPR = 0xFE; /* (7) */
  RTC->WPR = 0x64; /* (7) */
}

/**
   - Handling RTC IRQ
*/
void RTC_IRQHandler(void)
{
  /* Check alarm A flag */
  if((RTC->ISR & (RTC_ISR_ALRAF)) == (RTC_ISR_ALRAF))
    {
      RTC->ISR &=~ RTC_ISR_ALRAF; /* clear flag */
      EXTI->PR |= EXTI_PR_PR17; /* clear exti line 17 flag */
      //      GPIOB->ODR ^= (1 << 4) ; /* Toggle Green LED */
      // Alarm = 1;
    }
  else
    {
      // error = ERROR_RTC;
      NVIC_DisableIRQ(RTC_IRQn);/* Disable RTC_IRQn */
    }
}
