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
const uint32_t FWTime = CURR_TIM;
const uint32_t FWDate = CURR_DAT;

uint32_t SPIEEPROMaddr;
uint32_t LastReadSPIEEPROMaddr;
uint32_t ReadSPIEEPROMaddr;

uint8_t FromLowPower;
volatile uint8_t uartsend = FIRST_DATA;
volatile uint8_t CharToReceive;

volatile time_date_reg_t TimeDateRegS;
volatile uint8_t *PtrTimDatS = (uint8_t *)&TimeDateRegS;
volatile uint8_t *PtrTDSPICR = (uint8_t *)&TimeDateRegS.SPICommand;
volatile uint8_t *PtrTDTimeR = (uint8_t *)&TimeDateRegS.TimeRegister;
volatile uint8_t *PtrTDDateR = (uint8_t *)&TimeDateRegS.DateRegister;

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
      Init_RTC(__REV(FWTime), __REV(FWDate));
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
	      /* Backup time year masked */
	      if((RTC->BKP2R & 0xFF1F0000) != TimeDateRegS.DateRegister)
		{
		  MyStateRegister |= STORE_TIMESTAMP_DAT;
		}
	      RTC->BKP1R = TimeDateRegS.TimeRegister;
	      if((MyStateRegister & (STORE_TIMESTAMP_DAT)) == (STORE_TIMESTAMP_DAT))
		{
		  RTC->BKP2R = TimeDateRegS.DateRegister;
		}
	      StoreDateTime();
	      if((MyStateRegister & (STORE_TIMESTAMP_DAT)) == (STORE_TIMESTAMP_DAT))
		{
		  MyStateRegister &= ~(STORE_TIMESTAMP_DAT);
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
		      LastReadSPIEEPROMaddr = SPIEEPROMaddr - 8;
		      RTC->BKP3R =  LastReadSPIEEPROMaddr;
		      ReadSPIEEPROMaddr = LastReadSPIEEPROMaddr;
		    }
		  else if(CharToReceive == 101) /* 'e' letter code */
		    {
		      LastReadSPIEEPROMaddr = 0;
                      MyStateRegister |= UART_SEND_HEADER;
                    }
		  else if(CharToReceive == 102) /* 'f' letter code */
		    {
		      LastReadSPIEEPROMaddr = SPIEEPROMaddr + 4;
                      MyStateRegister |= UART_SEND_HEADER;
                    }
		  else if(CharToReceive == 98) /* 'b' letter code */
		    {
                      MyStateRegister |= UART_SEND_HEADER;
		    }
		  else if(CharToReceive == 97) /* 'a' letter code */
		    {
		      /* Wait until shadow register refresh */
		      RTC->ISR &= ~(RTC_ISR_RSF);
		      TimeDateRegS.align = TSTO_EPR_LENGTH;
		      while((RTC->ISR & RTC_ISR_RSF) != RTC_ISR_RSF)
			{
			}
		      /* Read and send current time, with subsecond */
		      TimeDateRegS.TimeRegister = __REV(RTC->SSR);
		      /* Reverse byte order */
		      TimeDateRegS.TimeRegister |= __REV(RTC->TR << 8);
		      /* Shadow register is frozen until read DR */
		      TimeDateRegS.DateRegister = __REV(RTC->DR);
		      MyStateRegister |= UART_PROGRESS;
		      EnableTransmit_USART();
		      /* Start UART transmission */
		      uartsend = UFIRST_DATA + 1;
		      USART2->TDR = *PtrTDTimeR++;
 		      /* Enable TXE interrupt */
		      USART2->CR1 |= USART_CR1_TXEIE;
		    }
		  else if(CharToReceive == 113) /* 'q' letter code */
		    {
		      MyStateRegister &= ~(READY_UART);
		      Deconfigure_USART();
		    }
		}
	    }
	  else if(((MyStateRegister & (UART_SEND_HEADER)) == (UART_SEND_HEADER)) && (uartsend == UFIRST_DATA))
	    {
	      /* Send header with compilation date-time and chip UID */
	      MyStateRegister++;
	      switch(MyStateRegister & COUNTER_MSK)
		{
		case 1: /* Send firmware time and date */
		  {
		    EnableTransmit_USART();
		    TimeDateRegS.align = TSTO_EPR_LENGTH;
		    TimeDateRegS.TimeRegister = __REV(FWTime);
		    TimeDateRegS.DateRegister = __REV(FWDate);
		    *PtrTDDateR = 0xC0;
		  }
		  break;
		case 2: /* Send UID first and second word */
		  {
		    /* LOT number 95:72 */
		    TimeDateRegS.TimeRegister = (*((volatile uint32_t *)(UID_BASE + 0x14)));
		    /* Wafer number  RM0377 28.2*/
		    *PtrTDTimeR = (*((volatile uint8_t *)(UID_BASE + 0x03)));
		    /* LOT number 71:40 */
		    TimeDateRegS.DateRegister = (*((volatile uint32_t *)(UID_BASE + 0x04)));
		    *PtrTDDateR = (*((volatile uint8_t *)(UID_BASE + 0x14)));
		  }
		  break;
		case 3: /* Send UID third word */
		  {
		    /* Clear Counter */
		    MyStateRegister &= ~(COUNTER_MSK);
		    MyStateRegister &= ~(UART_SEND_HEADER);
		    /* LOT number 39:00 */
		    TimeDateRegS.TimeRegister = (*((volatile uint32_t *)(UID_BASE + 0x00)));
		    *PtrTDTimeR = (*((volatile uint8_t *)(UID_BASE + 0x04)));
		    /* Header separator */
		    TimeDateRegS.DateRegister = 0xFFFFFFFF;
		    /* Continue with data */
		    MyStateRegister |= UART_PROGRESS;
		  }
		  break;
		}
	      /* Start UART transmission */
	      uartsend = UFIRST_DATA + 1;
	      USART2->TDR = *PtrTDTimeR++;
	      /* Enable TXE interrupt */
	      USART2->CR1 |= USART_CR1_TXEIE;
	    }
	  else if(((MyStateRegister & (UART_PROGRESS)) == (UART_PROGRESS)) && (uartsend == UFIRST_DATA))
	    {
	      MyStateRegister &= ~(UART_PROGRESS);
	      DisableTransmit_USART();
	      if(CharToReceive == 98 || CharToReceive == 101 || CharToReceive == 102) /* char b or e or f*/
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
	      if(ReadSPIEEPROMaddr != SPIEEPROMaddr)
		{
		  TimeDateRegS.align = TO_EPR_LENGTH;
		  TimeDateRegS.SPICommand = READ;
		  TimeDateRegS.SPIAddress[0] = (ReadSPIEEPROMaddr >> 16) & 0xFF;
		  TimeDateRegS.SPIAddress[1] = (ReadSPIEEPROMaddr >> 8) & 0xFF;
		  TimeDateRegS.SPIAddress[2] = ReadSPIEEPROMaddr & 0xFF;
		  Write_SPI(PtrTDSPICR, TO_EPR_LENGTH);
		  ReadSPIEEPROMaddr = IncreaseSPIEEPROMaddr(ReadSPIEEPROMaddr, 4);
		  MyStateRegister |= UART_PROGRESS;
		  EnableTransmit_USART();
		  /* Start UART transmission */
		  uartsend = UFIRST_DATA + 1;
		  USART2->TDR = *PtrTDTimeR++;
 		  /* Enable TXE interrupt */
		  USART2->CR1 |= USART_CR1_TXEIE;
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
		  TimeDateRegS.SPICommand = RDSR;
		  Write_SPI(PtrTDSPICR, 2);
		}
	      while (TimeDateRegS.SPIAddress[0] > 0);
	      DeconfigureLPTIM1();
	      ReadSPIEEPROMaddr = LastReadSPIEEPROMaddr;
	      MyStateRegister |= SPI_READROM;
	    }
	  else if((MyStateRegister & (INIT_UART)) == (INIT_UART))
	    {
	      MyStateRegister &= ~(INIT_UART);
	      Configure_USART();
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
  uint8_t spibufflength = 4;
  uint8_t pagebarrier = 0;

  if((MyStateRegister & (STORE_TIMESTAMP_DAT)) == (STORE_TIMESTAMP_DAT))
    {
      spibufflength = 8;
      if((MyStateRegister & (INIT_UART)) == (INIT_UART))
	{
	  *PtrTDDateR = 0xC0; // Data flag & during read
	}
      else
	{
	  *PtrTDDateR = 0x40; // Date flag
	}
      /* Read date register and store only year */
      TimeDateRegS.DateRegister |= __REV(RTC->DR & 0xFF0000);
      /* Test the page barrier! SPI_EPR_PG_SUB1 page size in bytes */
      /* It uses binary modulo */
      if(((SPIEEPROMaddr + 4) & SPI_EPR_PG_SUB1) == 0)
	{
	  pagebarrier = 1;
	}
    }
  Configure_GPIO_SPI1();
  TimeDateRegS.SPICommand = WREN;
  Write_SPI(PtrTDSPICR, 1);
  // Read Status Reg
  TimeDateRegS.SPICommand = RDSR;
  Write_SPI(PtrTDSPICR, 2);
  if((TimeDateRegS.SPIAddress[0] & (WEL)) == (WEL))
    {
      // Save data to SPIEEPROM
      TimeDateRegS.SPICommand = WRITE;
      TimeDateRegS.SPIAddress[0] = (SPIEEPROMaddr >> 16) & 0xFF;
      TimeDateRegS.SPIAddress[1] = (SPIEEPROMaddr >> 8) & 0xFF;
      TimeDateRegS.SPIAddress[2] = SPIEEPROMaddr & 0xFF;
      if(pagebarrier == 0)
	{
	  Write_SPI(PtrTDSPICR, TSTO_EPR_LENGTH);
	  SPIEEPROMaddr = IncreaseSPIEEPROMaddr(SPIEEPROMaddr, spibufflength);
	}
      /* if at the barrier divide date and time */
      /* Only TO_EPR_LENGHT wiht 4 byte data */
      else
	{
	  spibufflength = 4;
	  Write_SPI(PtrTDSPICR, TO_EPR_LENGTH);
	  SPIEEPROMaddr = IncreaseSPIEEPROMaddr(SPIEEPROMaddr, spibufflength);
	  /* Wait till succesful write */
	  ConfigureLPTIM1();
	  do
	    {
	      StartLPTIM1(35);
	      Configure_Lpwr(ModeSleep);
	      // Read Status Reg
	      TimeDateRegS.SPICommand = RDSR;
	      Write_SPI(PtrTDSPICR, 2);
	    }
	  while (TimeDateRegS.SPIAddress[0] > 0);
	  DeconfigureLPTIM1();
	  TimeDateRegS.SPICommand = WREN;
	  Write_SPI(PtrTDSPICR, 1);
	  TimeDateRegS.SPICommand = WRITE;
	  TimeDateRegS.SPIAddress[0] = ((SPIEEPROMaddr + 4) >> 16) & 0xFF;
	  TimeDateRegS.SPIAddress[1] = ((SPIEEPROMaddr + 4) >> 8) & 0xFF;
	  TimeDateRegS.SPIAddress[2] = (SPIEEPROMaddr + 4) & 0xFF;
	  TimeDateRegS.TimeRegister = TimeDateRegS.DateRegister;
	  Write_SPI(PtrTDSPICR, TO_EPR_LENGTH);
	  SPIEEPROMaddr = IncreaseSPIEEPROMaddr(SPIEEPROMaddr, spibufflength);
	}
      RTC->BKP0R = SPIEEPROMaddr;
    }
  Deconfigure_GPIO_SPI1(SPI_IS_STANDALONE);
}

/**
  * Brief   This function increases SPIEEPROM address with datalength and watch end
  * Param   uint32_t currentaddr
  * Param   uint32_t datalength
  * Retval  uint32_t SPIaddress
  */
uint32_t IncreaseSPIEEPROMaddr(uint32_t currentaddr, uint32_t datalength)
{
  currentaddr += datalength;
  /* Checque SPI EEPROM address valid? */
  if(currentaddr >= SPIEEPROM_LENGTH)
    {
      currentaddr = 0;
    }
  return(currentaddr);
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
      *(PtrTDTimeR + 1) = CharToReceive << 4;
    }
    break;
  case 2: /* Second char */
    {
      *(PtrTDTimeR + 1) |= CharToReceive;
    }
    break;
  case 3: /* Third char */
    {
      *(PtrTDTimeR + 2) = CharToReceive << 4;
    }
    break;
  case 4: /* Fourth char */
    {
      *(PtrTDTimeR + 2) |= CharToReceive;
    }
    break;
  case 5: /* Fifth char */
    {
      *(PtrTDTimeR + 3) = CharToReceive << 4;
    }
    break;
  case 6: /* Sixth char */
    {
      *(PtrTDTimeR + 3) |= CharToReceive;
      /* After the sixth character set value */
      if((MyStateRegister & (SET_TIME)) == (SET_TIME)) {
	Init_RTC(TimeDateRegS.TimeRegister, TimeDateRegS.DateRegister);
	/* Save date-and-time of setting */
	TimeDateRegS.TimeRegister = TimeDateRegS.TimeRegister << 8;
	RTC->BKP1R = TimeDateRegS.TimeRegister;
	RTC->BKP2R = TimeDateRegS.DateRegister;
	MyStateRegister &= ~(SET_DATE);
	MyStateRegister &= ~(SET_TIME);
      } else {
	TimeDateRegS.DateRegister = TimeDateRegS.TimeRegister;
	MyStateRegister |= SET_TIME;
      }
      /* Clear Counter */
      MyStateRegister &= ~(COUNTER_MSK);
    }
    break;
  }
}
