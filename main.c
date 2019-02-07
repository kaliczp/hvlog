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

#include "main.h"

volatile uint32_t MyStateRegister;

/* Timestamp values */
volatile uint32_t SubSecondRegister;
volatile uint32_t TimeRegister;
volatile uint32_t DateRegister;
const uint32_t FWTime = CURR_TIM;
const uint32_t FWDate = CURR_DAT;

uint32_t SPIEEPROMaddr;
uint32_t LastReadSPIEEPROMaddr;
uint32_t ReadSPIEEPROMaddr;

uint8_t FromLowPower;
volatile uint8_t uartsend = 3;
volatile uint8_t CharToReceive;
uint8_t ToEEPROM[TO_EPR_LENGTH] = {WRITE, 0x0, 0x0, 0x0, 0x17, 0x03, 0x15};

int main(void)
{
  FromLowPower = 0;
  RCC->APB1ENR |= RCC_APB1ENR_PWREN; // Enable PWR
  Enable_RTC_registers();
  /* If RTC is not set configure and initialise */
  if((PWR->CSR & PWR_CSR_SBF) == (PWR_CSR_SBF))
    {
      PWR->CR |= PWR_CR_CSBF;
      PWR->CR |= PWR_CR_CWUF;
      FromLowPower = 1;
    }
  else
    {
      Configure_RTC_Clock();
    }
  RCC->APB1ENR &= ~(RCC_APB1ENR_PWREN); // Disable PWR
  if(FromLowPower == 0)
    {
      Configure_RTC_Func();
      Init_RTC(FWTime, FWDate);
    }
  /* Important variables. Loaded from RTC domain */
  SPIEEPROMaddr =  RTC->BKP0R;
  LastReadSPIEEPROMaddr =  RTC->BKP3R;

  while(1)
    {
      if(MyStateRegister > 0)
	{
	  if((MyStateRegister & (TIMESTAMP_CAPTURED)) == (TIMESTAMP_CAPTURED))
	    /* Prevent overwrite TS with some additional flag check */
	    {
	      NVIC_DisableIRQ(RTC_IRQn);
	      MyStateRegister &= ~(TIMESTAMP_CAPTURED);
	      /* Test UART */
	      Configure_GPIOB_Test();
	      // Test UART connection
	      if((GPIOB->IDR & (GPIO_IDR_ID7)) == (GPIO_IDR_ID7))
		{
		  MyStateRegister |= INIT_UART;
		  MyStateRegister |= STORE_TIMESTAMP_DAT;
		}
	      Deconfigure_GPIOB_Test();
	      if(RTC->BKP2R < DateRegister)
		{
		  MyStateRegister |= (STORE_TIMESTAMP_DAT | STORE_TIMESTAMP_TIM);
		}
	      /* Prevent debouncing, store same time */
	      else if(RTC->BKP1R != TimeRegister)
		{
		  MyStateRegister |= STORE_TIMESTAMP_TIM;
		}
	      /* Join time with subseconds */
	      TimeRegister = TimeRegister << 8;
	      TimeRegister |= SubSecondRegister;
	      if((MyStateRegister & (STORE_TIMESTAMP_TIM)) == (STORE_TIMESTAMP_TIM))
		{
		  MyStateRegister &= ~(STORE_TIMESTAMP_TIM);
		  StoreDateTime();
		  RTC->BKP1R = TimeRegister;
		  if((MyStateRegister & (STORE_TIMESTAMP_DAT)) == (STORE_TIMESTAMP_DAT))
		    {
		      MyStateRegister &= ~(STORE_TIMESTAMP_DAT);
		      RTC->BKP2R = DateRegister;
		    }
		}
	    }
	  else if((MyStateRegister & (CHAR_RECEIVED)) == (CHAR_RECEIVED))
	    {
	      MyStateRegister &= ~(CHAR_RECEIVED);
	      /* Datum or time set */
	      if((MyStateRegister & (SET_DATE)) == (SET_DATE))
		{
		  MyStateRegister++;
		  ProcessDateTimeSetting();
		}
	      else
		{
		  /* Command character */
		  if(CharToReceive == 100) /* 'd' letter code */
		    {
		      MyStateRegister |= SET_DATE;
		    }
		  else if(CharToReceive == 99) /* 'c' letter code */
		    {
		      /* Keep readout date and time as the first record 
			 of next readout */
		      LastReadSPIEEPROMaddr = ReadSPIEEPROMaddr - 8;
		      RTC->BKP3R =  LastReadSPIEEPROMaddr;
		    }
		  else if(CharToReceive == 101) /* 'e' letter code */
		    {
		      LastReadSPIEEPROMaddr = 0;
                      MyStateRegister |= INIT_SPIREAD;
                    }
		  else if(CharToReceive == 98) /* 'b' letter code */
		    {
		      /* Send firmware date and after the stored timestamps */
		      /* if the send of timestamp finished */
		      ToEEPROM[3] = 0xC0;
		      ToEEPROM[4] = (FWDate >> 16) & 0xFF;
		      ToEEPROM[5] = (FWDate >> 8) & 0xFF;
		      ToEEPROM[6] = FWDate & 0xFF;
		      MyStateRegister |= UART_PROGRESS;
		      EnableTransmit_USART1();
		      /* Start UART transmission */
		      USART1->TDR = ToEEPROM[uartsend++];
		      /* Enable TXE interrupt */
		      USART1->CR1 |= USART_CR1_TXEIE;
		    }
		  else if(CharToReceive == 97) /* 'a' letter code */
		    {
		      /* Read and send current time, without control*/
		      ToEEPROM[6] = RTC->SSR & 0xFF;
		      TimeRegister = RTC->TR;
		      /* Shadow register is frozen until read DR */
		      DateRegister = RTC->DR;
		      ToEEPROM[3] = (TimeRegister >> 16) & 0xFF;
		      ToEEPROM[4] = (TimeRegister >> 8) & 0xFF;
		      ToEEPROM[5] = TimeRegister & 0xFF;
		      MyStateRegister |= UART_PROGRESS;
		      EnableTransmit_USART1();
		      /* Start UART transmission */
		      USART1->TDR = ToEEPROM[uartsend++];
 		      /* Enable TXE interrupt */
		      USART1->CR1 |= USART_CR1_TXEIE;
		    }
		  else if(CharToReceive == 113) /* 'q' letter code */
		    {
                      MyStateRegister &= ~(READY_UART);
		      Deconfigure_USART1();
		    }
		}
	    }
	  else if(((MyStateRegister & (UART_PROGRESS)) == (UART_PROGRESS)) && (uartsend == 3))
	    {
              MyStateRegister &= ~(UART_PROGRESS);
	      DisableTransmit_USART1();
	      if(CharToReceive == 98) /* char b */
		{
		  /* If SPI not initialised fire up */
		  if((GPIOA->MODER & (GPIO_MODER_MODE15_0)) == (GPIO_MODER_MODE15_0))
		    {
		      MyStateRegister |= INIT_SPIREAD;
		    }
		  else
		    {
		      MyStateRegister |= SPI_READROM;
		    }
		}
	    }
	  else if((MyStateRegister & (SPI_READROM)) == (SPI_READROM))
	    {
	      MyStateRegister &= ~(SPI_READROM);
	      if(ReadSPIEEPROMaddr < SPIEEPROMaddr)
		{
		  ToEEPROM[0] = READ;
		  ToEEPROM[1] = (ReadSPIEEPROMaddr >> 8) & 0xFF;
		  ToEEPROM[2] = ReadSPIEEPROMaddr & 0xFF;
		  Write_SPI(ToEEPROM, 7);
		  ReadSPIEEPROMaddr += 4;
		  MyStateRegister |= UART_PROGRESS;
		  EnableTransmit_USART1();
		  /* Start UART transmission */
		  USART1->TDR = ToEEPROM[uartsend++];
 		  /* Enable TXE interrupt */
		  USART1->CR1 |= USART_CR1_TXEIE;
		}
	      else
		{
		  Deconfigure_GPIO_SPI1(SPI_IS_NOT_STANDALONE);
		}
	    }
	  else if((MyStateRegister & (INIT_SPIREAD)) == (INIT_SPIREAD))
	    {
	      MyStateRegister &= ~(INIT_SPIREAD);
	      Configure_GPIO_SPI1();
	      ConfigureLPTIM1();
	      do
		{
		  StartLPTIM1(35);
		  Configure_Lpwr(ModeSleep);
		  // Read Status Reg
		  ToEEPROM[0] = RDSR;
		  Write_SPI(ToEEPROM, 2);
		}
	      while (ToEEPROM[1] > 0);
	      DeconfigureLPTIM1();
	      ReadSPIEEPROMaddr = LastReadSPIEEPROMaddr;
	      MyStateRegister |= SPI_READROM;
	    }
	  else if((MyStateRegister & (INIT_UART)) == (INIT_UART))
	    {
              MyStateRegister &= ~(INIT_UART);
	      Configure_USART1();
	      MyStateRegister |= READY_UART;
	    }
	  else if((MyStateRegister & (READY_UART)) == (READY_UART))
	    {
	      Configure_Lpwr(ModeSleep);
	    }
	  else if((MyStateRegister & (DAILY_ALARM)) == (DAILY_ALARM))
	    {
	      MyStateRegister &= ~(DAILY_ALARM);
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
  uint8_t pagebarrier = 0;

  TSToEEPROM[3] = (TimeRegister >> 24) & 0xFF;
  TSToEEPROM[4] = (TimeRegister >> 16) & 0xFF;
  TSToEEPROM[5] = (TimeRegister >> 8) & 0xFF;
  TSToEEPROM[6] = TimeRegister & 0xFF;
  if((MyStateRegister & (STORE_TIMESTAMP_DAT)) == (STORE_TIMESTAMP_DAT))
    {
      spibufflength = 8;
      if((MyStateRegister & (INIT_UART)) == (INIT_UART))
	{
	  TSToEEPROM[7] = 0xC0; // Data flag & during read
	}
      else
	{
	  TSToEEPROM[7] = 0x40; // Date flag
	}
      TSToEEPROM[8] = (DateRegister >> 16) & 0xFF;
      TSToEEPROM[9] = (DateRegister >> 8) & 0xFF;
      TSToEEPROM[10] = DateRegister & 0xFF;
      /* Test the page barrier! SPI_EPR_PG_SUB1 page size in bytes */
      /* It uses binary modulo */
      if(((SPIEEPROMaddr + 4) & SPI_EPR_PG_SUB1) == 0)
	{
	  pagebarrier = 1;
	}
    }
  Configure_GPIO_SPI1();
  TSToEEPROM[0] = WREN;
  Write_SPI(TSToEEPROM, 1);
  // Read Status Reg
  TSToEEPROM[0] = RDSR;
  Write_SPI(TSToEEPROM, 2);
  if((TSToEEPROM[1] & (WEL)) == (WEL))
    {
      // Save data to SPIEEPROM
      TSToEEPROM[0] = WRITE;
      TSToEEPROM[1] = (SPIEEPROMaddr >> 8) & 0xFF;
      TSToEEPROM[2] = SPIEEPROMaddr & 0xFF;
      if(pagebarrier == 0)
	{
	  Write_SPI(TSToEEPROM, spibufflength + 3);
	}
      /* if at the barrier divide date and time */
      else
	{
	  Write_SPI(TSToEEPROM, 4 + 3);
	  /* Wait till succesful write */
	  ConfigureLPTIM1();
	  do
	    {
	      StartLPTIM1(35);
	      Configure_Lpwr(ModeSleep);
	      // Read Status Reg
	      ToEEPROM[0] = RDSR;
	      Write_SPI(ToEEPROM, 2);
	    }
	  while (ToEEPROM[1] > 0);
	  DeconfigureLPTIM1();
	  TSToEEPROM[0] = WREN;
	  Write_SPI(TSToEEPROM, 1);
	  TSToEEPROM[0] = WRITE;
	  TSToEEPROM[1] = ((SPIEEPROMaddr + 4) >> 8) & 0xFF;
	  TSToEEPROM[2] = (SPIEEPROMaddr + 4) & 0xFF;
	  TSToEEPROM[3] = TSToEEPROM[7];
	  TSToEEPROM[4] = TSToEEPROM[8];
	  TSToEEPROM[5] = TSToEEPROM[9];
	  TSToEEPROM[6] = TSToEEPROM[10];
	  Write_SPI(TSToEEPROM, 4 + 3);
	}
      /* Checque SPI EEPROM address valid? */
      SPIEEPROMaddr += spibufflength;
      RTC->BKP0R = SPIEEPROMaddr;
    }
  Deconfigure_GPIO_SPI1(SPI_IS_STANDALONE);
}

/**
  * Brief   This function sets RTC clock with USART communication.
  * Param   None
  * Retval  None
  */
void ProcessDateTimeSetting(void)
{
  CharToReceive -= 48;
  switch(MyStateRegister & COUNTER_MSK)
  {
  case 1: /* First char */
    {
      TimeRegister = CharToReceive << 20;
    }
    break;
  case 2: /* Second char */
    {
      TimeRegister |= CharToReceive << 16;
    }
    break;
  case 3: /* Third char */
    {
      TimeRegister |= CharToReceive << 12;
    }
    break;
  case 4: /* Fourth char */
    {
      TimeRegister |= CharToReceive << 8;
    }
    break;
  case 5: /* Fifth char */
    {
      TimeRegister |= CharToReceive << 4;
    }
    break;
  case 6: /* Sixth char */
    {
      TimeRegister |= CharToReceive;
      /* After the sixth character set value */
      if((MyStateRegister & (SET_TIME)) == (SET_TIME)) {
	Init_RTC(TimeRegister, DateRegister);
	MyStateRegister &= ~(SET_DATE);
	MyStateRegister &= ~(SET_TIME);
      } else {
	DateRegister = TimeRegister;
	MyStateRegister |= SET_TIME;
      }
      /* Clear Counter */
      MyStateRegister &= ~(COUNTER_MSK);
    }
    break;
  }
}
