/*
**********************************************************************
* Author    Péter Kalicz
* Version   V0.9
* Date      2018-05-01
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

#include "lpwr.h"

void SwitchVregulatorRange1(void)
{
  /* (1) Select voltage scale 1 (1.65V - 1.95V) i.e. (01) for VOS bits
     in PWR_CR During voltage scaling configuration, the system clock
     is stopped until the regulator is stabilized (VOSF=0).*/
  /* (2) Check voltage scalin flag */
  PWR->CR = (PWR->CR & ~(PWR_CR_VOS)) | PWR_CR_VOS_0; /* (1) */
  while((PWR->CSR & PWR_CSR_VOSF) == PWR_CSR_VOSF) /* (2) */
    {
    }
}

void Configure_Lpwr(uint8_t LpwrMode)
{
  /* (0) Enable power module */
  RCC->APB1ENR |= RCC_APB1ENR_PWREN; /* (0) */
  if(LpwrMode > 0)
    {
#ifdef DEBUG
      /* Enable the peripheral clock of DBG register */
      RCC->APB2ENR |= RCC_APB2ENR_DBGMCUEN;
      if(LpwrMode == ModeSTOP)
	{
	  /* To be able to debug in stop mode */
	  DBGMCU->CR |= DBGMCU_CR_DBG_STOP;
	}
      else
	{
	  /* To be able to debug in standby mode */
	  DBGMCU->CR |= DBGMCU_CR_DBG_STANDBY;
	}
#endif
      /* (1)  Clear the WUF flag after 2 clock cycles */
      /* (2) Regulator in LowPower mode and disable VREFINT and enable fast wake-up */
      /* (3a) Select STOP mode in the PWR_CR register */
      /* (3b) Select Standby mode in the PWR_CR register */
      /* (4) Enter deep sleep when __WFI() */
      /* (4b) Disable PWR clock */
      /* (5) WFI */
      /* (6) Clear deep sleep after wake up */
      /* (7) Disable regulator low-power mode */
      PWR->CR |= PWR_CR_CWUF; /* (1) */
      if(LpwrMode == ModeSTOP)
	{
	  PWR->CR |= (PWR_CR_LPSDSR | PWR_CR_ULP | PWR_CR_FWU); /* (2) */
	  PWR->CR &= ~(PWR_CR_PDDS); /* (3a) */
	}
      else
	{
	  PWR->CR |= PWR_CR_CSBF; /* (1) */
	  PWR->CR |= (PWR_CR_ULP | PWR_CR_FWU); /* (2) */
	  PWR->CR |= PWR_CR_PDDS;  /* (3b) */
	}
      SCB->SCR |= SCB_SCR_SLEEPDEEP_Msk; /* (4) */
      RCC->APB1ENR &= ~(RCC_APB1ENR_PWREN); /* (4b) */
      __WFI(); /* (5) */
      SCB->SCR &= ~(SCB_SCR_SLEEPDEEP_Msk); /* (6) */
    }
  else
    {
      PWR->CR &= ~(PWR_CR_LPSDSR); /* (7) */
      RCC->APB1ENR &= ~(RCC_APB1ENR_PWREN); /* (4b) */
      __WFE();
    }
}
