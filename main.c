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

uint32_t SPIEEPROMaddr;
uint32_t LastReadSPIEEPROMaddr;
uint32_t ReadSPIEEPROMaddr;
uint32_t OldTimestampTime;
uint32_t OldTimestampDate;

uint8_t FromLowPower;
volatile uint8_t uartsend;
uint8_t ToEEPROM[TO_EPR_LENGTH] = {WRITE, 0x0, 0x0, 0x0, 0x0, 0x17, 0x03, 0x15};

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
  SPIEEPROMaddr =  RTC->BKP0R;
  LastReadSPIEEPROMaddr =  RTC->BKP3R;
  /* Older timestamp values */
  OldTimestampTime = RTC->BKP1R;
  OldTimestampDate = RTC->BKP2R;

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
	      if(OldTimestampDate < TimestampDate)
		{
		  MyStateRegister |= STORE_TIMESTAMP_DAT;
		}
	      StoreDateTime();
	      OldTimestampTime = TimestampTime;
	      OldTimestampDate = TimestampDate;
	      RTC->BKP1R = OldTimestampTime;
	      RTC->BKP2R = OldTimestampDate;
	    }
	  else if((MyStateRegister & (SPI_READROM)) == (SPI_READROM))
	    {
	      MyStateRegister &= ~SPI_READROM;
	      if(ReadSPIEEPROMaddr < SPIEEPROMaddr)
		{
		  ToEEPROM[0] = READ;
		  ToEEPROM[1] = (ReadSPIEEPROMaddr >> 16) & 0xFF;
		  ToEEPROM[2] = (ReadSPIEEPROMaddr >> 8) & 0xFF;
		  ToEEPROM[3] = ReadSPIEEPROMaddr & 0xFF;
		  Write_SPI(ToEEPROM, TO_EPR_LENGTH);
		  ReadSPIEEPROMaddr += 4;
		  MyStateRegister |= UART_SEND;
		}
	      else
		{
		  MyStateRegister &= ~UART_SEND;
		  MyStateRegister &= ~UART_PROGRESS;
		  LastReadSPIEEPROMaddr = ReadSPIEEPROMaddr;
		  RTC->BKP3R =  LastReadSPIEEPROMaddr;
		  Deconfigure_USART2();
		  Deconfigure_GPIO_SPI1();
		  Deconfigure_GPIOB_Test();
		}
	    }
	  else if((MyStateRegister & (UART_PROGRESS)) == (UART_PROGRESS))
	    {
	      /* (1) clear TC flag */
	      USART2->ICR |= USART_ICR_TCCF; /* (1) */
	      for(uartsend=4;uartsend < TO_EPR_LENGTH; uartsend ++)
		{
		  USART2->TDR = ToEEPROM[uartsend];
		  while ((USART2->ISR & USART_ISR_TXE) == 0)
		    {
		    }
		}
	      MyStateRegister |= SPI_READROM;
	    }
	  else if((MyStateRegister & (INIT_SPIREAD)) == (INIT_SPIREAD))
	    {
	      MyStateRegister &= ~(INIT_SPIREAD);
	      Configure_GPIO_SPI1();
	      // Read Status Reg
	      ToEEPROM[0] = RDSR;
	      Write_SPI(ToEEPROM, 2);
	      if(ToEEPROM[1] > 0)
		{
		  ConfigureLPTIM1(90);
		  /* __WFI(); */
		  DeconfigureLPTIM1();
		}
	      ReadSPIEEPROMaddr = LastReadSPIEEPROMaddr;
	      MyStateRegister |= SPI_READROM;
	    }
	  else if((MyStateRegister & (UART_SEND)) == (UART_SEND))
	    {
	      MyStateRegister &= ~UART_SEND;
	      if((MyStateRegister & (INIT_UART)) == (INIT_UART))
		{
		  MyStateRegister &= ~INIT_UART;
		  Configure_USART2();
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
  uint8_t TSToEEPROM[TSTO_EPR_LENGTH] = {WRITE, 0x0, 0x0, 0x0, 0x0, 0x21, 0x31, 0x0, 0x40, 0x17, 0x10, 0x01};
  uint8_t spibufflength = 4;

  if((MyStateRegister & (INIT_SPIREAD)) == (INIT_SPIREAD))
    {
      TSToEEPROM[4] = 0x80; // Time flag & during read
    }
  else
    {
      TSToEEPROM[4] = 0x0; // Time flag
    }
  TSToEEPROM[5] = (TimestampTime >> 16) & 0xFF;
  TSToEEPROM[6] = (TimestampTime >> 8) & 0xFF;
  TSToEEPROM[7] = TimestampTime & 0xFF;
  if((MyStateRegister & (STORE_TIMESTAMP_DAT)) == (STORE_TIMESTAMP_DAT))
    {
      MyStateRegister &= ~STORE_TIMESTAMP_DAT;
      spibufflength = 8;
      if((MyStateRegister & (INIT_SPIREAD)) == (INIT_SPIREAD))
	{
	  TSToEEPROM[8] = 0xC0; // Data flag & during read
	}
      else
	{
	  TSToEEPROM[8] = 0x40; // Date flag
	}
      TSToEEPROM[9] = (TimestampDate >> 16) & 0xFF;
      TSToEEPROM[10] = (TimestampDate >> 8) & 0xFF;
      TSToEEPROM[11] = TimestampDate & 0xFF;
    }
  Configure_GPIO_SPI1();
  // Read Status Reg
  TSToEEPROM[0] = RDSR;
  Write_SPI(TSToEEPROM, 2);
  if(TSToEEPROM[1] > 0)
    {
      ConfigureLPTIM1(99);
      /* __WFI(); */
      DeconfigureLPTIM1();
      TSToEEPROM[0] = RDSR;
      Write_SPI(TSToEEPROM, 2);
      if(TSToEEPROM[1] > 0)
	{
	  ConfigureLPTIM1(10);
	  /* __WFI(); */
	  DeconfigureLPTIM1();
	  TSToEEPROM[0] = RDSR;
	  Write_SPI(TSToEEPROM, 2);
	}
    }
  // Save data to SPIEEPROM
  // Test the page barrier!
  TSToEEPROM[0] = WREN;
  Write_SPI(TSToEEPROM, 1);
  TSToEEPROM[0] = WRITE;
  TSToEEPROM[1] = (SPIEEPROMaddr >> 16) & 0xFF;
  TSToEEPROM[2] = (SPIEEPROMaddr >> 8) & 0xFF;
  TSToEEPROM[3] = SPIEEPROMaddr & 0xFF;
  Write_SPI(TSToEEPROM, spibufflength + 4);
  Deconfigure_GPIO_SPI1();
  /* Checque SPI EEPROM address valid? */
  SPIEEPROMaddr += spibufflength;
  RTC->BKP0R = SPIEEPROMaddr;
}
