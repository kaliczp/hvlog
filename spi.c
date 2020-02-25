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
  GPIOB->OSPEEDR = (GPIOB->OSPEEDR & ~(GPIO_OSPEEDER_OSPEED3 | GPIO_OSPEEDER_OSPEED4 | GPIO_OSPEEDER_OSPEED5)) | (GPIO_OSPEEDER_OSPEED3_0 | \
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
void Configure_SPI1(uint8_t length)
{
  /* Enable the peripheral clock SPI1 */
  RCC->APB2ENR |= RCC_APB2ENR_SPI1EN;

  /* Enable the peripheral clock DMA1 */
  RCC->AHBENR |= RCC_AHBENR_DMA1EN;

  /* Configure SPI1 in master */
  /* (1) Master selection, BR: Fpclk/8
         CPOL and CPHA at zero (rising first edge), 8-bit data frame, NSS output enable (SSM=0,SSOE = 1). */
  /* (2) Slave select output enabled, RXNE IT */
  SPI1->CR1 = SPI_CR1_MSTR | SPI_CR1_BR_1; /* (1) */
  /* SPI1->CR2 = SPI_CR2_SSOE | SPI_CR2_RXNEIE; /\* (2) *\/ */
  SPI1->CR2 = SPI_CR2_SSOE | SPI_CR2_RXDMAEN; /* (2) */

  /* DMA1 Channel2 SPI1_RX config */
  /* (3)  Map SPI1_RX DMA channel */
  /* (4) Peripheral address */
  /* (5) Memory address */
  /* (6) Data size */
  /* (7a) Memory increment */
  /*     Peripheral to memory */
  /*     8-bit transfer */
  /*     Priority very high */
  /*     DMA interrupt enabled */
  /* (7b) DMA enabled */
  DMA1_CSELR->CSELR = (DMA1_CSELR->CSELR & ~DMA_CSELR_C2S) | (1 << (1 * 4)); /* (3) */
  DMA1_Channel2->CPAR = (uint32_t)&(SPI1->DR); /* (4) */
    DMA1_Channel2->CMAR = (uint32_t)PtrTDSPICR; /* (5) */
  DMA1_Channel2->CNDTR = length; /* (6) */
  DMA1_Channel2->CCR |= DMA_CCR_MINC | DMA_CCR_PL_1 | DMA_CCR_PL_0 | DMA_CCR_TCIE ; /* (7a) */
  DMA1_Channel2->CCR |= DMA_CCR_EN; /* (7b) */

  /* DMA1 Channel3 SPI1_TX config */
  /* (8)  Map SPI1_TX DMA channel */
  /* (9) Peripheral address */
  /* (10 Memory address */
  /* (11) Memory increment */
  /*     Memory to peripheral*/
  /*     8-bit transfer */
  DMA1_CSELR->CSELR = (DMA1_CSELR->CSELR & ~DMA_CSELR_C3S) | (1 << (2 * 4)); /* (8) */
  DMA1_Channel3->CPAR = (uint32_t)&(SPI1->DR); /* (9) */
    DMA1_Channel3->CMAR = (uint32_t)PtrSendTDSPICR; /* (10) */
  DMA1_Channel3->CCR |= DMA_CCR_MINC | DMA_CCR_DIR; /* (11) */

  /* Configure IT */
  /* (11a) Set priority for DMA1_Channel2_3_IRQn */
  /* (11b) Enable DMA1_Channel2_3_IRQn */
  NVIC_SetPriority(DMA1_Channel2_3_IRQn, 0); /* (11a) */
  NVIC_EnableIRQ(DMA1_Channel2_3_IRQn); /* (11b) */

  /* (12) Enable TX in SPI */
  SPI1->CR2 |= SPI_CR2_TXDMAEN; /* (12) */
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
  /* wait until TXE=1 and BSY=0 */
  while((SPI1->SR & (SPI_SR_TXE | SPI_SR_BSY)) != SPI_SR_TXE)
    {
    }
  /* Disable DMA1 SPI IRQ */
  NVIC_DisableIRQ(DMA1_Channel2_3_IRQn);
  /* Disable DMA TX */
  DMA1_Channel3->CCR &= ~(DMA_CCR_EN);
  /* Disable DMA RX */
  DMA1_Channel2->CCR &= ~(DMA_CCR_EN);
  /* Disable SPI1 */
  SPI1->CR1 &= ~(SPI_CR1_SPE);
  /* Disable DMA Tx and Rx buffers by clearing CR2 bits */
  SPI1->CR2 &= ~(SPI_CR2_TXDMAEN | SPI_CR2_RXDMAEN);
}

void Deconfigure_SPI1(void)
{
  /* Disable the peripheral clock DMA11 */
  RCC->AHBENR &= ~(RCC_AHBENR_DMA1EN);

  /* Disable SPI1 periperal clock */
  RCC->APB2ENR &= ~(RCC_APB2ENR_SPI1EN);
}

void Write_SPI(uint8_t length)
{
  Configure_SPI1(length);
  Activate_SPI1();
  /* Start transfer */
  DMA1_Channel3->CCR &= ~(DMA_CCR_EN);
  DMA1_Channel3->CNDTR = length; /* Data size */
  DMA1_Channel3->CCR |= DMA_CCR_EN;

  /* Wait until transfer complete */
  Configure_Lpwr(ModeSleep);
  Deactivate_SPI1();
  Deconfigure_SPI1();
}

void DMA1_Channel2_3_IRQHandler(void)
{
  if((DMA1->ISR & DMA_ISR_TCIF2) == DMA_ISR_TCIF2)
    {
      DMA1->IFCR |= DMA_IFCR_CTCIF2; /* Clear transfer complete interrrupt flag */
    }
  else
    {
      DMA1->IFCR |= (DMA_IFCR_CGIF2 | DMA_IFCR_CGIF3); /* Clear global interrrupt flags */
    }
}
