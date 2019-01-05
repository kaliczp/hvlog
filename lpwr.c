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

void Configure_Lpwr(uint8_t LpwrMode)
{
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
      /* (0) Enable power module */
      /* (1)  Clear the WUF flag after 2 clock cycles */
      /* (2) Regulator in LowPower mode and disable VREFINT and enable fast wake-up */
      /* (3a) Select STOP mode in the PWR_CR register */
      /* (3b) Select Standby mode in the PWR_CR register */
      /* (4) Enter deep sleep when __WFI() */
      /* (4b) Disable PWR clock */
      /* (5) WFI */
      /* (6) Clear deep sleep after wake up */
      /* (7) Disable regulator low-power mode */
      RCC->APB1ENR |= RCC_APB1ENR_PWREN; /* (0) */
      PWR->CR |= PWR_CR_CWUF; /* (1) */
      if(LpwrMode == ModeSTOP)
	{
	  PWR->CR |= (PWR_CR_LPSDSR | PWR_CR_ULP | PWR_CR_FWU); /* (2) */
	  PWR->CR &= ~ (PWR_CR_PDDS); /* (3a) */
	}
      else
	{
	  PWR->CR |= PWR_CR_CSBF; /* (1) */
	  PWR->CR |= (PWR_CR_ULP | PWR_CR_FWU); /* (2) */
	  PWR->CR |= PWR_CR_PDDS;  /* (3b) */
	}
      SCB->SCR |= SCB_SCR_SLEEPDEEP_Msk; /* (4) */
      RCC->APB1ENR &= ~RCC_APB1ENR_PWREN; /* (4b) */
      __WFI(); /* (5) */
      SCB->SCR &= ~SCB_SCR_SLEEPDEEP_Msk; /* (6) */
    }
  else
    {
      RCC->APB1ENR |= RCC_APB1ENR_PWREN; /* (0) */
      PWR->CR &= ~PWR_CR_LPSDSR; /* (7) */
      RCC->APB1ENR &= ~RCC_APB1ENR_PWREN; /* (4b) */
      __WFE();
    }
}
