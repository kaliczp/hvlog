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
volatile uint8_t uartsend;
volatile uint8_t CharToReceive;

volatile time_date_reg_t TimeDateRegS;
volatile uint8_t *PtrTimDatS = (uint8_t *)&TimeDateRegS;
volatile uint8_t *PtrTDSPICR = (uint8_t *)&TimeDateRegS.SPICommand;
volatile uint8_t *PtrTDTimeR = (uint8_t *)&TimeDateRegS.TimeRegister;
volatile uint8_t *PtrTDDateR = (uint8_t *)&TimeDateRegS.DateRegister;
volatile time_date_reg_t SendTimeDateRegS;
volatile uint8_t *PtrSendTDSPICR = (uint8_t *)&SendTimeDateRegS.SPICommand;
volatile uint8_t *PtrSendTDDateR = (uint8_t *)&SendTimeDateRegS.DateRegister;

int main(void)
{
  FromLowPower = 0;
  RCC->APB1ENR |= RCC_APB1ENR_PWREN; // Enable PWR
  SwitchVregulatorRange1();
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
      Set_RTC_CALR(RTC_CALM);
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
	      if((RTC->BKP2R & 0xFF1F0000) != SendTimeDateRegS.DateRegister)
		{
		  MyStateRegister |= STORE_TIMESTAMP_DAT;
		}
	      RTC->BKP1R = SendTimeDateRegS.TimeRegister;
	      if((MyStateRegister & (STORE_TIMESTAMP_DAT)) == (STORE_TIMESTAMP_DAT))
		{
		  RTC->BKP2R = SendTimeDateRegS.DateRegister;
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
		      LastReadSPIEEPROMaddr = SPIEEPROMaddr - TIME_DATE_LENGTH;
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
		      LastReadSPIEEPROMaddr = SPIEEPROMaddr + TIME_LENGTH;
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
		      TimeDateRegS.length = TIME_DATE_LENGTH;
		      while((RTC->ISR & RTC_ISR_RSF) != RTC_ISR_RSF)
			{
			}
		      /* Read and send current time, with subsecond */
		      TimeDateRegS.TimeRegister = __REV(RTC->SSR);
		      /* Reverse byte order */
		      TimeDateRegS.TimeRegister |= __REV(RTC->TR << 8);
		      /* Reverse byte order, mask out weekday*/
		      TimeDateRegS.DateRegister = __REV(RTC->DR & 0xFF1FFF);
		      MyStateRegister |= UART_PROGRESS;
		      EnableTransmit_USART();
		      /* Start UART transmission */
		      uartsend = 1;
		      USART1->TDR = *PtrTDTimeR++;
 		      /* Enable TXE interrupt */
		      USART1->CR1 |= USART_CR1_TXEIE;
		    }
		  else if(CharToReceive == 113) /* 'q' letter code */
		    {
		      MyStateRegister &= ~(READY_UART);
		      Deconfigure_USART();
		    }
		}
	    }
	  else if(((MyStateRegister & (UART_SEND_HEADER)) == (UART_SEND_HEADER)) && (uartsend == 0))
	    {
	      /* Send header with compilation date-time and chip UID */
	      MyStateRegister++;
	      switch(MyStateRegister & COUNTER_MSK)
		{
		case 1: /* Send firmware time and date */
		  {
		    EnableTransmit_USART();
		    TimeDateRegS.length = TIME_DATE_LENGTH;
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
	      uartsend = 1;
	      USART1->TDR = *PtrTDTimeR++;
	      /* Enable TXE interrupt */
	      USART1->CR1 |= USART_CR1_TXEIE;
	    }
	  else if(((MyStateRegister & (UART_PROGRESS)) == (UART_PROGRESS)) && (uartsend == 0))
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
		  SendTimeDateRegS.length = TIME_LENGTH;
		  SendTimeDateRegS.SPICommand = READ;
		  SendTimeDateRegS.SPIAddress[0] = (ReadSPIEEPROMaddr >> 8) & 0xFF;
		  SendTimeDateRegS.SPIAddress[1] = ReadSPIEEPROMaddr & 0xFF;
		  Write_SPI(TO_EPR_LENGTH);
		  ReadSPIEEPROMaddr = IncreaseSPIEEPROMaddr(ReadSPIEEPROMaddr, TIME_LENGTH);
		  MyStateRegister |= UART_PROGRESS;
		  EnableTransmit_USART();
		  /* Start UART transmission */
		  uartsend = 1;
		  USART1->TDR = *PtrTDTimeR++;
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
		  SendTimeDateRegS.SPICommand = RDSR;
		  Write_SPI(2);
		}
	      while ((TimeDateRegS.SPIAddress[0] & (WIP)) == (WIP));
	      DeconfigureLPTIM1();
	      ReadSPIEEPROMaddr = LastReadSPIEEPROMaddr;
	      /* Set length 4 bytes to UART communication */
	      TimeDateRegS.length = TIME_LENGTH;
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
  uint8_t spibufflength = TIME_LENGTH;
  uint8_t pagebarrier = 0;
  uint8_t cyclecount;

  if((MyStateRegister & (STORE_TIMESTAMP_DAT)) == (STORE_TIMESTAMP_DAT))
    {
      spibufflength = TIME_DATE_LENGTH;
      if((MyStateRegister & (INIT_UART)) == (INIT_UART))
	{
	  *PtrSendTDDateR = 0xC0; // Data flag & during read
	}
      else
	{
	  *PtrSendTDDateR = 0x40; // Date flag
	}
      /* Read date register and store only year */
      SendTimeDateRegS.DateRegister |= __REV(RTC->DR & 0xFF0000);
      /* Test the page barrier! SPI_EPR_PG_SUB1 page size in bytes */
      /* It uses binary modulo */
      if(((SPIEEPROMaddr + 4) & SPI_EPR_PG_SUB1) == 0)
	{
	  pagebarrier = 1;
	}
    }
  Configure_GPIO_SPI1();
  SendTimeDateRegS.SPICommand = WREN;
  Write_SPI(1);
  // Read Status Reg
  SendTimeDateRegS.SPICommand = RDSR;
  ConfigureLPTIM1();
  cyclecount = 0;
  do
    {
      StartLPTIM1(35);
      Configure_Lpwr(ModeSleep);
      Write_SPI(2);
      cyclecount++;
    }
  while(((TimeDateRegS.SPIAddress[0] & (WIP)) == (WIP)) && (cyclecount < 4));
  DeconfigureLPTIM1();
  if((TimeDateRegS.SPIAddress[0] & (WEL)) == (WEL))
    {
      // Save data to SPIEEPROM
      SendTimeDateRegS.SPICommand = WRITE;
      SendTimeDateRegS.SPIAddress[0] = (SPIEEPROMaddr >> 8) & 0xFF;
      SendTimeDateRegS.SPIAddress[1] = SPIEEPROMaddr & 0xFF;
      if(pagebarrier == 0)
	{
	  Write_SPI(TO_EPR_ADDRESSLENGTHwCOMMAND + spibufflength);
	  SPIEEPROMaddr = IncreaseSPIEEPROMaddr(SPIEEPROMaddr, spibufflength);
	}
      /* if at the barrier divide date and time */
      /* Only TO_EPR_LENGHT wiht 4 byte data */
      else
	{
	  spibufflength = TIME_LENGTH;
	  Write_SPI(TO_EPR_LENGTH);
	  SPIEEPROMaddr = IncreaseSPIEEPROMaddr(SPIEEPROMaddr, spibufflength);
	  /* Wait till succesful write */
	  ConfigureLPTIM1();
	  do
	    {
	      StartLPTIM1(35);
	      Configure_Lpwr(ModeSleep);
	      // Read Status Reg
	      SendTimeDateRegS.SPICommand = RDSR;
	      Write_SPI(2);
	    }
	  while ((TimeDateRegS.SPIAddress[0] & (WIP)) == (WIP));
	  DeconfigureLPTIM1();
	  SendTimeDateRegS.SPICommand = WREN;
	  Write_SPI(1);
	  SendTimeDateRegS.SPICommand = WRITE;
	  SendTimeDateRegS.SPIAddress[0] = ((SPIEEPROMaddr + TIME_LENGTH) >> 8) & 0xFF;
	  SendTimeDateRegS.SPIAddress[1] = (SPIEEPROMaddr + TIME_LENGTH) & 0xFF;
	  SendTimeDateRegS.TimeRegister = SendTimeDateRegS.DateRegister;
	  Write_SPI(TO_EPR_LENGTH);
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
