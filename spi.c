/*
**********************************************************************
* Author    Péter Kalicz
* Version   V0.9
* Date      2018-05-01
* Brief     SPI config

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

#include "spi.h"

/**
  * Brief   This function :
             - Enables GPIO clock
             - Configures the SPI1 pins on GPIO PA15 PB3 PB4 PB5
  * Param   None
  * Retval  None
  */
void Configure_GPIO_SPI1(void)
{
  volatile uint32_t tmpreg;
  /* Enable the peripheral clock of GPIOA and GPIOB */
  /* (2) Some delay based on LL */
  RCC->IOPENR |= RCC_IOPENR_GPIOAEN;
  RCC->IOPENR |= RCC_IOPENR_GPIOBEN;
  tmpreg = RCC->IOPENR; /* (2) */
  (void)tmpreg; /* (2) */

  /* (1) Select AF mode (10) on PA15 (NSS) */
  /* (2) AF0 for SPI1 signals */
  /* (3) Select AF mode (10) on PB3 (SCK), PB4 (MISO), PB5 (MOSI) */
  /* (4) AF0 for SPI1 signals */
  /* (5) Select medium output speed (01) for SPI1 pins */
  GPIOA->MODER = (GPIOA->MODER & ~(GPIO_MODER_MODE15)) | GPIO_MODER_MODE15_1; /* (1) */
  GPIOA->AFR[1] = (GPIOA->AFR[1] & ~((0xF<<(4*7)))); /* (2) */
  GPIOB->MODER = (GPIOB->MODER
                  & ~(GPIO_MODER_MODE3 | \
                      GPIO_MODER_MODE4 | GPIO_MODER_MODE5))\
                  | (GPIO_MODER_MODE3_1 | \
                     GPIO_MODER_MODE4_1 | GPIO_MODER_MODE5_1); /* (3) */
  GPIOB->AFR[0] = (GPIOB->AFR[0] & \
                   ~((0xF<<(4*3)) | \
                     (0xF<<(4*4)) | ((uint32_t)0xF<<(4*5)))); /* (4) */
  GPIOB->OSPEEDR &= ~(GPIO_OSPEEDER_OSPEED3 | GPIO_OSPEEDER_OSPEED4 | GPIO_OSPEEDER_OSPEED5)| (GPIO_OSPEEDER_OSPEED3_0 | \
                     GPIO_OSPEEDER_OSPEED4_0 | GPIO_OSPEEDER_OSPEED5_0); /* (5) */
}

void Deconfigure_GPIO_SPI1(uint8_t SPI_Standalone)
{
  /* Select high-impedance analog in PA15 (1) */
  /* Select high-impedance analog in PB3 PB4 PB5 (2) */
  GPIOA->MODER |= GPIO_MODER_MODE15; /* (1) */
  GPIOB->MODER |= GPIO_MODER_MODE3 | GPIO_MODER_MODE4 | GPIO_MODER_MODE5; /* (2) */
  /* Disable the peripheral clock of GPIOA and GPIOB */
  RCC->IOPENR &= ~(RCC_IOPENR_GPIOAEN);

  if(SPI_Standalone)
    {
      RCC->IOPENR &= ~(RCC_IOPENR_GPIOBEN);
    }
}

/**
  * Brief   This function configures SPI1.
  * Param   None
  * Retval  None
  */
void Configure_SPI1(void)
{
  /* Enable the peripheral clock SPI1 */
  RCC->APB2ENR |= RCC_APB2ENR_SPI1EN;

  /* Configure SPI1 in master */
  /* (1) Master selection, BR: Fpclk/4
         CPOL and CPHA at zero (rising first edge), 8-bit data frame, NSS output enable (SSM=0,SSOE = 1). */
  /* (2) Slave select output enabled, RXNE IT */
  SPI1->CR1 = SPI_CR1_MSTR | SPI_CR1_BR_0; /* (1) */
  /* SPI1->CR2 = SPI_CR2_SSOE | SPI_CR2_RXNEIE; /\* (2) *\/ */
  SPI1->CR2 = SPI_CR2_SSOE; /* (2) */
}

void Activate_SPI1(void)
{
  /* Enable SPI1 */
  SPI1->CR1 |= SPI_CR1_SPE;
 
  /* Configure IT */
  /* (4) Set priority for SPI1_IRQn */
  /* (5) Enable SPI1_IRQn */
  /* NVIC_SetPriority(SPI1_IRQn, 0); /\* (4) *\/ */
  /* NVIC_EnableIRQ(SPI1_IRQn); /\* (5) *\/  */
}


void Deactivate_SPI1(void)
{
  /* wait until BSY=0 */
  while((SPI1->SR & SPI_SR_BSY) == SPI_SR_BSY)
    {
    }
  /* Disable SPI1 */
  SPI1->CR1 &= ~(SPI_CR1_SPE);
}

void Deconfigure_SPI1(void)
{
  /* Disable SPI1 periperal clock */
  RCC->APB2ENR &= ~(RCC_APB2ENR_SPI1EN);
}

void Write_SPI(uint8_t *buff, uint8_t length)
{
  Configure_SPI1();
  Activate_SPI1();
  /* Test SPI */
  /* Test Tx empty */
  for(uint8_t i=0; i < length; i++)
    {
      while((SPI1->SR & SPI_SR_TXE) != SPI_SR_TXE)
	{
	}
      *(uint8_t *)&(SPI1->DR) = buff[i];
      while((SPI1->SR & SPI_SR_RXNE) != SPI_SR_RXNE)
	{
	}
      buff[i] = SPI1->DR;
    }
  while((SPI1->SR & SPI_SR_TXE) != SPI_SR_TXE)
    {
    }
  Deactivate_SPI1();
  Deconfigure_SPI1();
}
