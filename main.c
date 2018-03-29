/*
**********************************************************************
* Author    Péter Kalicz
* Version   V0.5
* Date      2017-10-01
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

volatile uint32_t MyStateRegister;

/* Timestamp values */
volatile uint32_t TimestampTime;
volatile uint32_t TimestampDate;

uint16_t SPIEEPROMaddr;
uint16_t LastReadSPIEEPROMaddr;
uint16_t ReadSPIEEPROMaddr;

uint8_t FromLowPower;
volatile uint8_t uartsend;
uint8_t ToEEPROM[TO_EPR_LENGTH] = {WRITE, 0x0, 0x0, 0x0, 0x17, 0x03, 0x15};

int main(void)
{
  FromLowPower = 0;
  RCC->APB1ENR |= RCC_APB1ENR_PWREN; // Enable PWR
  Enable_RTC_registers();
  /* If RTC is not set configure and initialise */
  if((PWR->CSR & PWR_CSR_SBF) == 1)
    {
      PWR->CR |= PWR_CR_CSBF;
      PWR->CR |= PWR_CR_CWUF;
      FromLowPower = 1;
    }
  else
    {
      Configure_RTC_Clock();
    }
  RCC->APB1ENR &=~ RCC_APB1ENR_PWREN; // Disable PWR
  if(FromLowPower == 0)
    {
      Configure_RTC_Func();
      Init_RTC(CURR_TIM, CURR_DAT);
    }
  /* Important variables. Loaded from RTC domain */
  SPIEEPROMaddr =  RTC->BKP0R & 0xFFFF;
  LastReadSPIEEPROMaddr =  (RTC->BKP0R >> 16) & 0xFFFF;

  while(1)
    {
      if(MyStateRegister > 0)
	{
	  if((MyStateRegister & (TIMESTAMP_CAPTURED)) == (TIMESTAMP_CAPTURED))
	    /* Prevent overwrite TS with some additional flag check */
	    {
	      NVIC_DisableIRQ(RTC_IRQn);
	      MyStateRegister &= ~TIMESTAMP_CAPTURED;
	      /* Test UART */
	      Configure_GPIOB_Test();
	      // Test UART connection
	      if((GPIOB->IDR & (GPIO_IDR_ID7)) == (GPIO_IDR_ID7))
		{
		  MyStateRegister |= INIT_SPIREAD;
		  MyStateRegister |= INIT_UART;
		}
	      else
		{
		  MyStateRegister &= ~ (SPI_READROM);
		}
	      Deconfigure_GPIOB_Test();
	      if(RTC->BKP2R < TimestampDate)
		{
		  MyStateRegister |= STORE_TIMESTAMP_DAT;
		}
	      StoreDateTime();
	      RTC->BKP1R = TimestampTime;
	      RTC->BKP2R = TimestampDate;
	    }
	  else if((MyStateRegister & (SPI_READROM)) == (SPI_READROM))
	    {
	      MyStateRegister &= ~SPI_READROM;
	      if(ReadSPIEEPROMaddr < SPIEEPROMaddr)
		{
		  ToEEPROM[0] = READ;
		  ToEEPROM[1] = (ReadSPIEEPROMaddr >> 8) & 0xFF;
		  ToEEPROM[2] = ReadSPIEEPROMaddr & 0xFF;
		  Write_SPI(ToEEPROM, 7);
		  ReadSPIEEPROMaddr += 4;
		  MyStateRegister |= UART_SEND;
		}
	      else
		{
		  MyStateRegister &= ~UART_SEND;
		  MyStateRegister &= ~UART_PROGRESS;
		  LastReadSPIEEPROMaddr = ReadSPIEEPROMaddr;
		  RTC->BKP0R =  (RTC->BKP0R & 0x0000FFFF) | ((uint32_t)LastReadSPIEEPROMaddr << 16);
		  Deconfigure_USART1();
		  Deconfigure_GPIO_SPI1();
		  Deconfigure_GPIOB_Test();
		}
	    }
	  else if((MyStateRegister & (UART_PROGRESS)) == (UART_PROGRESS))
	    {
	      /* (1) clear TC flag */
	      USART1->ICR |= USART_ICR_TCCF; /* (1) */
	      for(uartsend=3;uartsend < 7; uartsend ++)
		{
		  USART1->TDR = ToEEPROM[uartsend];
		  while ((USART1->ISR & USART_ISR_TXE) == 0)
		    {
		    }
		}
	      MyStateRegister |= SPI_READROM;
	    }
	  else if((MyStateRegister & (INIT_SPIREAD)) == (INIT_SPIREAD))
	    {
	      MyStateRegister &= ~(INIT_SPIREAD);
	      Configure_GPIO_SPI1();
	      ConfigureLPTIM1();
	      do
		{
		  StartLPTIM1(35);
		  Configure_Lpwr(ModeSTOP);
		  // Read Status Reg
		  ToEEPROM[0] = RDSR;
		  Write_SPI(ToEEPROM, 2);
		}
	      while (ToEEPROM[1] > 0);
	      DeconfigureLPTIM1();
	      ReadSPIEEPROMaddr = LastReadSPIEEPROMaddr;
	      MyStateRegister |= SPI_READROM;
	    }
	  else if((MyStateRegister & (UART_SEND)) == (UART_SEND))
	    {
	      MyStateRegister &= ~UART_SEND;
	      if((MyStateRegister & (INIT_UART)) == (INIT_UART))
		{
		  MyStateRegister &= ~INIT_UART;
		  Configure_USART1();
		  MyStateRegister |= UART_PROGRESS;
		}
	    }
	  else if((MyStateRegister & (DAILY_ALARM)) == (DAILY_ALARM))
	    {
	      MyStateRegister &= ~DAILY_ALARM;
	      // Write internal EEPROM with SPI EEPROM pointer and date
	    }
	  else
	    {
	      MyStateRegister = NOTHING_TODO;
	    }
	}
      else
	{
	  RTC_ReEnableTamperIRQ();
	  /* Go to low power */
	  Configure_Lpwr(ModeSTOP);
	}
    }
}
void StoreDateTime()
{
  uint8_t TSToEEPROM[TSTO_EPR_LENGTH] = {WRITE, 0x0, 0x0, 0x0, 0x21, 0x31, 0x0, 0x40, 0x17, 0x10, 0x01,};
  uint8_t spibufflength = 4;

  if((MyStateRegister & (INIT_SPIREAD)) == (INIT_SPIREAD))
    {
      TSToEEPROM[3] = 0x80; // Time flag & during read
    }
  else
    {
      TSToEEPROM[3] = 0x0; // Time flag
    }
  TSToEEPROM[4] = (TimestampTime >> 16) & 0xFF;
  TSToEEPROM[5] = (TimestampTime >> 8) & 0xFF;
  TSToEEPROM[6] = TimestampTime & 0xFF;
  if((MyStateRegister & (STORE_TIMESTAMP_DAT)) == (STORE_TIMESTAMP_DAT))
    {
      MyStateRegister &= ~STORE_TIMESTAMP_DAT;
      spibufflength = 8;
      if((MyStateRegister & (INIT_SPIREAD)) == (INIT_SPIREAD))
	{
	  TSToEEPROM[7] = 0xC0; // Data flag & during read
	}
      else
	{
	  TSToEEPROM[7] = 0x40; // Date flag
	}
      TSToEEPROM[8] = (TimestampDate >> 16) & 0xFF;
      TSToEEPROM[9] = (TimestampDate >> 8) & 0xFF;
      TSToEEPROM[10] = TimestampDate & 0xFF;
    }
  Configure_GPIO_SPI1();
  // Read Status Reg
  TSToEEPROM[0] = RDSR;
  Write_SPI(TSToEEPROM, 2);
  if(TSToEEPROM[1] > 0)
    {
      // Save data to SPIEEPROM
      // Test the page barrier!
      TSToEEPROM[0] = WRITE;
      TSToEEPROM[1] = (SPIEEPROMaddr >> 8) & 0xFF;
      TSToEEPROM[2] = SPIEEPROMaddr & 0xFF;
      Write_SPI(TSToEEPROM, spibufflength + 3);
      /* Checque SPI EEPROM address valid? */
      SPIEEPROMaddr += spibufflength;
      RTC->BKP0R = (RTC->BKP0R & ~0xFFFF) | SPIEEPROMaddr;
    }
  Deconfigure_GPIO_SPI1();
}
