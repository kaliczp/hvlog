/*
**********************************************************************
* Author    Péter Kalicz
* Version   V0.9
* Date      2018-05-01
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
   - RTC register access enable
*/
void Enable_RTC_registers(void)
{
/* Befor use this function enable power interface in the RCC_APB1ENR register. */
/* (1) Set the DBP bit in the PWR_CR register (see RM Section 6.4.1). */
  PWR->CR |= PWR_CR_DBP; /* (1) */
}

void Configure_RTC_Clock(void)
{
  /* Before use this function enable PWR clock */
  /* (1) Enable the LSE */
  /* (2) Change LSE driving power if neccessary */
  /* (3) Wait while it is not ready */
  /* (4) LSE for RTC clock */
  RCC->CSR |= RCC_CSR_LSEON; /* (1) */
  
  //  RCC->CSR |= RCC_CSR_LSEDRV_0; /* (2) */

  while((RCC->CSR & RCC_CSR_LSERDY)!=RCC_CSR_LSERDY) /* (3) */
    { 
      /* add time out here for a robust application */
    }
  
  RCC->CSR = (RCC->CSR & ~(RCC_CSR_RTCSEL)) | RCC_CSR_RTCEN | RCC_CSR_RTCSEL_0; /* (4) */
}

void Configure_RTC_Func(void)
{
  /* Configure RTC */
  /* (1) Write access for RTC regsiters */
  /* (2) Disable alarm A to modify it */
  /* (3) Wait until it is allow to modify alarm A value */
  /* (4) Modify alarm A mask to have an interrupt each 1/60Hz (1 days) */
  /* (5) Enable alarm A and alarm A interrupt */
  /* (6) Disable write access */

  RTC->WPR = 0xCA; /* (1) */
  RTC->WPR = 0x53; /* (1) */
  RTC->CR &= ~(RTC_CR_ALRAE); /* (2) */
  while((RTC->ISR & RTC_ISR_ALRAWF) != RTC_ISR_ALRAWF) /* (3) */
    { 
      /* add time out here for a robust application */
    }
  RTC->ALRMAR = RTC_ALRMAR_MSK4; /* (4) */
  RTC->CR = RTC_CR_ALRAIE | RTC_CR_ALRAE; /* (5) */
  RTC->WPR = 0xFF; /* (6) */
  /* (7a) Tamper configuration:
     - Tamper activated after 2 samples (precharged 1 RTCCLOCK TAMPPRCH)
       TAMP2TRG = 0, low input triggers
     - RTCCLK/1024 tamper sampling frequency (32 Hz at 32768 Hz RTC clock)
     - Activate time stamp on tamper detection even if TSE=0
     - enable input low level detection on RTC_TAMP2 (PA0)
     - No erase backup registers */
  /* (7b) Tamper interrupt enable */

  RTC->TAMPCR = RTC_TAMPCR_TAMPFLT_0 | RTC_TAMPCR_TAMPFREQ_2 | RTC_TAMPCR_TAMPFREQ_0 | RTC_TAMPCR_TAMPTS | RTC_TAMPCR_TAMP2E | RTC_TAMPCR_TAMP2NOERASE; /* (7a) */
  RTC->TAMPCR |= RTC_TAMPCR_TAMPIE; /* (7b) */
  
  /* Configure exti for RTC IT */
  /* (8) unmask line 17 */
  /* (9) Rising edge for line 17 */
  /* (10) unmask line 19 */
  /* (11) Rising edge for line 19 */
  EXTI->IMR |= EXTI_IMR_IM17; /* (8) */ 
  EXTI->RTSR |= EXTI_RTSR_TR17; /* (9) */
  EXTI->IMR |= EXTI_IMR_IM19; /* (10) */ 
  EXTI->RTSR |= EXTI_RTSR_TR19; /* (11) */ 

  
  /* Configure NVIC */
  /* (12) Set priority */
  /* (13) Enable RTC_IRQn */
  NVIC_SetPriority(RTC_IRQn, 0x00); /* (12) */ 
  NVIC_EnableIRQ(RTC_IRQn); /* (13) */
  RTC->ISR &= ~(RTC_ISR_TAMP2F); /* clear tamper flag */
  RTC->ISR &= ~(RTC_ISR_TSF); /* clear timestamp flag */
  RTC->ISR &= ~(RTC_ISR_TSOVF); /* clear timestamp overflow flag */
}

/**
   - Set RTC TIME
*/
void Init_RTC(uint32_t Time, uint32_t Date)
{
  /* RTC init mode */
  /* Configure RTC */
  /* Ref. man. p. 534 */
  /* (1) Write access for RTC registers */
  /* (2) Enable init phase, stop counter */
  /* (3) Wait until it is allow to modify RTC register values */
  /* (4) prescaler default 32768Hz/128 => 256Hz, 256Hz/256 => 1Hz */
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
  /* RTC->PRER = 0x007F00FF; */ /* (4) */
  RTC->TR = __REV(Time); /* (5) */
  RTC->DR = __REV(Date); /* (5a) */
  RTC->ISR &= ~(RTC_ISR_INIT); /* (6) */
  RTC->WPR = 0xFF; /* (7) */
}

void RTC_ReEnableTamperIRQ(void)
{
  /* (1) RTC IRQ fired during data processing? */
  if(NVIC_GetPendingIRQ(RTC_IRQn)) /* (1) */
    {
      /* Here the place for some notification */
      EXTI->PR |= EXTI_PR_PR19; /* clear exti line 19 flag */
      NVIC_ClearPendingIRQ(RTC_IRQn);
    }
  RTC->ISR &= ~(RTC_ISR_TSF); /* clear timestamp flag */
  RTC->ISR &= ~(RTC_ISR_TSOVF); /* clear timestamp overflow flag */
  RTC->ISR &= ~(RTC_ISR_TAMP2F); /* clear tamper flag */
  NVIC_EnableIRQ(RTC_IRQn);
}

/**
   - Handling RTC IRQ
*/
void RTC_IRQHandler(void)
{
  /* Check tamper and timestamp flag */
  if(((RTC->ISR & (RTC_ISR_TAMP2F)) == (RTC_ISR_TAMP2F)) && ((RTC->ISR & (RTC_ISR_TSF)) == (RTC_ISR_TSF)))
    {
      RTC->ISR &= ~(RTC_ISR_TAMP2F); /* clear tamper flag */
      EXTI->PR |= EXTI_PR_PR19; /* clear exti line 19 flag */
      TimeDateRegS.TimeRegister = __REV(RTC->TSSSR); /* First subsecond register saved */
      TimeDateRegS.TimeRegister |= __REV((RTC->TSTR & 0x3FFFFF) << 8); /* AM/PM skip */
      TimeDateRegS.DateRegister = __REV(RTC->TSDR & 0x1FFF); /* Mask out weekday */
      RTC->ISR &= ~(RTC_ISR_TSF); /* clear timestamp flag */
      RTC->ISR &= ~(RTC_ISR_TSOVF); /* clear timestamp overflow flag */
      MyStateRegister |= TIMESTAMP_CAPTURED;
    }
  /* Check alarm A flag */
  else if((RTC->ISR & (RTC_ISR_ALRAF)) == (RTC_ISR_ALRAF))
    {
      RTC->ISR &= ~(RTC_ISR_ALRAF); /* clear flag */
      EXTI->PR |= EXTI_PR_PR17; /* clear exti line 17 flag */
      MyStateRegister |= DAILY_ALARM;
    }
  else if(((RTC->ISR & (RTC_ISR_TSOVF)) == (RTC_ISR_TSOVF)))
    {
      RTC->ISR &= ~(RTC_ISR_TSOVF); /* clear timestamp overflow flag */
    }
  else
    {
      // error = ERROR_RTC;
      NVIC_DisableIRQ(RTC_IRQn);/* Disable RTC_IRQn */
    }
}
