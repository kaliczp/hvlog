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
  /* (8) Disable alarm A&B to modify it */
  /* (9) Wait until it is allow to modify alarm A value */
  /* (9a) Wait until it is allow to modify alarm B value */
  /* (10) Modify alarm A mask to have an interrupt each 1/60Hz (1 mins) */
  /* (10a) Modify alarm B mask to have an interrupt each 1/60Hz (1 mins)  at 01 sec */
  /* (11) Enable alarm A&B and alarm A&B interrupt */
  /* (12) Disable write access */

  RTC->WPR = 0xCA; /* (7) */
  RTC->WPR = 0x53; /* (7) */
  RTC->CR &= ~(RTC_CR_ALRAE | RTC_CR_ALRBE); /* (8) */
  while((RTC->ISR & RTC_ISR_ALRAWF) != RTC_ISR_ALRAWF) /* (9) */
    { 
      /* add time out here for a robust application */
    }
  while((RTC->ISR & RTC_ISR_ALRBWF) != RTC_ISR_ALRBWF) /* (9a) */
    { 
      /* add time out here for a robust application */
    }
  RTC->ALRMAR = RTC_ALRMAR_MSK4 | RTC_ALRMAR_MSK3 | RTC_ALRMAR_MSK2 ; /* (10) */
  RTC->ALRMBR = RTC_ALRMBR_MSK4 | RTC_ALRMBR_MSK3 | RTC_ALRMBR_MSK2 | RTC_ALRMBR_SU_0 ; /* (10a) */
  RTC->CR = RTC_CR_ALRAIE | RTC_CR_ALRBIE | RTC_CR_ALRAE | RTC_CR_ALRBE; /* (11) */
  RTC->WPR = 0xFE; /* (12) */
  RTC->WPR = 0x64; /* (12) */
  /* (13) Tamper configuration:
     - Disable precharge (PU)
     - RTCCLK/256 tamper sampling frequency
     - Activate time stamp on tamper detection even if TSE=0
     - input rising edge trigger detection on RTC_TAMP2 (PA0)
     - Tamper interrupt enable
     - No erase backup registers */
  RTC->TAMPCR = RTC_TAMPCR_TAMPPUDIS | RTC_TAMPCR_TAMPFREQ | RTC_TAMPCR_TAMPTS | RTC_TAMPCR_TAMP2E | RTC_TAMPCR_TAMPIE | RTC_TAMPCR_TAMP2NOERASE; /* (13) */
  
  /* Configure exti for RTC IT */
  /* (14) unmask line 17 */
  /* (15) Rising edge for line 17 */
  /* (16) unmask line 19 */
  /* (17) Rising edge for line 19 */
  EXTI->IMR |= EXTI_IMR_IM17; /* (14) */ 
  EXTI->RTSR |= EXTI_RTSR_TR17; /* (15) */
  EXTI->IMR |= EXTI_IMR_IM19; /* (16) */ 
  EXTI->RTSR |= EXTI_RTSR_TR19; /* (17) */ 

  
  /* Configure NVIC */
  /* (18) Set priority */
  /* (19) Enable RTC_IRQn */
  NVIC_SetPriority(RTC_IRQn, 0x00); /* (18) */ 
  NVIC_EnableIRQ(RTC_IRQn); /* (19) */ 
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
  /* (2) Enable init phase, stop counter */
  /* (3) Wait until it is allow to modify RTC register values */
  /* (4) set prescaler, 40kHz/64 => 625 Hz, 625Hz/625 => 1Hz */
  /* (5) New time in TR */
  /* (5a) New date in DR */
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
  RTC->DR = 0x00176301; /* (5a) */
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
      RTC->ISR &= ~RTC_ISR_ALRAF; /* clear flag */
      EXTI->PR |= EXTI_PR_PR17; /* clear exti line 17 flag */
      GPIOA->BSRR = GPIO_BSRR_BS_5; /* lit green LED */
      // Alarm = 1;
    }
  else if((RTC->ISR & (RTC_ISR_ALRBF)) == (RTC_ISR_ALRBF))
    {
      RTC->ISR &= ~RTC_ISR_ALRBF; /* clear flag */
      EXTI->PR |= EXTI_PR_PR17; /* clear exti line 17 flag */
      GPIOA->BSRR = GPIO_BSRR_BR_5; /* switch off green LED */
     }

  /* Check tamper and timestamp flag */
  else if(((RTC->ISR & (RTC_ISR_TAMP2F)) == (RTC_ISR_TAMP2F)) && ((RTC->ISR & (RTC_ISR_TSF)) == (RTC_ISR_TSF))) 
    {
      RTC->ISR &= ~(RTC_ISR_TAMP2F); /* clear tamper flag */
      TimestampTime = RTC->TSTR;
      TimestampDate = RTC->TSDR;
      RTC->ISR &= ~(RTC_ISR_TSF); /* clear timestamp flag */
      RTC->ISR &= ~(RTC_ISR_TSOVF); /* clear timestamp overflow flag */
      EXTI->PR |= EXTI_PR_PR19; /* clear exti line 19 flag */
      MyStateRegister = TIMESTAMP_CAPTURED;
    }
  else
    {
      // error = ERROR_RTC;
      NVIC_DisableIRQ(RTC_IRQn);/* Disable RTC_IRQn */
    }
}
