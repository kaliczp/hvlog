/*
**********************************************************************
* @file     uart.c
* @version  V0.9
* @date     2018-05-01
* @brief    UART source file
**********************************************************************/
/* Author    Péter Kalicz
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

#include "uart.h"

/**
   - GPIO clock enable
   - Configures UART pin GPIO PB7
*/
void Configure_GPIOB_Test(void)
{
  volatile uint32_t tmpreg;
  /* Enable PB7 input */
  /* (1) Enable GPIOB clock */
  /* (1b) Some delay based on errata sheet */
  /* (2) Set PB7 input mode */
  /* (3) Enable pull-down */

  RCC->IOPENR |= RCC_IOPENR_GPIOBEN; /* (1) */
  tmpreg = RCC->IOPENR; /* (1b) */
  (void)tmpreg; /* (1b) */
  GPIOB->MODER = (GPIOB->MODER & ~(GPIO_MODER_MODE7)); /* (2) */
  GPIOB->PUPDR = (GPIOB->PUPDR & ~(GPIO_PUPDR_PUPD7)) | (GPIO_PUPDR_PUPD7_1); /* (3) */
}

void Deconfigure_GPIOB_Test(void)
{
  /* (1) Disable pull-down resistor */
  /* (1a) Select high-impedance analog mode PB7 */
  /* (2) Disable GPIOB clock */
  GPIOB->PUPDR = (GPIOB->PUPDR & ~(GPIO_PUPDR_PUPD7)); /* (1) */
  GPIOB->MODER |= GPIO_MODER_MODE7; /* (1a) */
  RCC->IOPENR &= ~(RCC_IOPENR_GPIOBEN); /* (2) */
}

void Configure_USART2(void)
{
  volatile uint32_t tmpreg;
  /* GPIO configuration for USART2 signals */
  /* (0) Enable GPIOB clock */
  /* (1) Some delay based on LL */
  /* (2) Select AF mode (10) on PB6 (TX) and PB7 (RX) */
  /* push-pull default */
  /* (3) pull-up */
  /* (4) high speed */
  /* AF0 default value for USART2 signals in PB6 and 7 */
  RCC->IOPENR |= RCC_IOPENR_GPIOBEN; /* (0) */
  tmpreg = RCC->IOPENR; /* (1) */
  (void)tmpreg; /* (1) */

  /* Select LSE as USART2 clock source 11 */
  RCC->CCIPR |= RCC_CCIPR_USART2SEL;

  GPIOB->MODER = (GPIOB->MODER & ~(GPIO_MODER_MODE6 | GPIO_MODER_MODE7)) \
    | (GPIO_MODER_MODE6_1 | GPIO_MODER_MODE7_1); /* (2) */
  GPIOB->PUPDR = (GPIOB->PUPDR & ~(GPIO_PUPDR_PUPD6 | GPIO_PUPDR_PUPD7)) | (GPIO_PUPDR_PUPD6_0 | GPIO_PUPDR_PUPD7_0); /* (3) */

  /* Enable the peripheral clock USART2 */
  RCC->APB1ENR |= RCC_APB1ENR_USART2EN;

  /* Configure USART2 */
  /* 32768 / 1200 = 27 */;
  /* (1b) 1200 baud shifted right RM 0377 24.5.4 */
  /* (2a) oversampling by 16 no OVER8, 8 data bit, 1 start bit, no
     parity, receive and receive interrupt enabled */
  /* (2aa) Set ONEBIT to increase the USART tolerance to timing deviations */
  /* (2ab) 2 stop bits STOP[1:0] = 10 */
  /* (2b) UART enabled with default values above */
  USART2->BRR = 0b11011; /* (1) */
  USART2->CR1 |= USART_CR1_RXNEIE | USART_CR1_RE ; /* (2a) */
  USART2->CR3 |= USART_CR3_ONEBIT; /* (2aa) */
  USART2->CR2 |= USART_CR2_STOP_1; /* (2ab) */
  USART2->CR1 |= USART_CR1_UE ; /* (2b) */
  /* Configure exti and IT */
  /* (7) unmask line 25 for USART2 */
  /* (8) Set priority for USART2_IRQn */
  /* (9) Enable USART2_IRQn */
  /* EXTI->IMR |= EXTI_IMR_IM25; /\* (7) *\/  */
  NVIC_SetPriority(USART2_IRQn, 0); /* (8) */
  NVIC_EnableIRQ(USART2_IRQn); /* (9) */
}

void EnableTransmit_USART2(void)
{
  /* (3) Enable UART transmitter line */
  /* (4) Wait for idle frame transmission maybe write 0 and 1 in TE */
  USART2->CR1 &= ~(USART_CR1_TE); /* (1) */
  USART2->CR1 |= USART_CR1_TE ; /* (3) */
  while((USART2->ISR & USART_ISR_TC) != USART_ISR_TC) /* (4) */
    {
    }
  /* (5) Clear TC flag */
  /* (6) Enable TC interrupt */
  USART2->ICR |= USART_ICR_TCCF; /* (5) */
  USART2->CR1 |= USART_CR1_TCIE; /* (6) */
}

void DisableTransmit_USART2(void)
{
  /* (0) disable transmitter interrupt*/
  /* (1) disable transmitter */
  /* (2) wait until TC=1 avoid corrupt last transmission */
  USART2->CR1 &= ~(USART_CR1_TCIE); /* (0) */
  USART2->CR1 &= ~(USART_CR1_TE); /* (1) */
  USART2->ICR |= USART_ICR_TCCF; /* Clear transfer complete flag */
}

void Deconfigure_USART2(void)
{
  /* (3) UART disabled */
  /* (4) Disable receiver */
  /* (4) Disable USART2 clock */
  /* (5) Disable GPIOB clock */
  NVIC_DisableIRQ(USART2_IRQn); /* Disable USART2_IRQn */
  USART2->CR1 &= ~(USART_CR1_UE); /* (3) */
  USART2->CR1 &= ~(USART_CR1_RE); /* (4) */
  GPIOB->MODER |= (GPIO_MODER_MODE6 | GPIO_MODER_MODE7); /* (4a) */
  RCC->APB1ENR &= ~(RCC_APB1ENR_USART2EN); /* (4b) */
  RCC->IOPENR &= ~(RCC_IOPENR_GPIOBEN); /* (5) */
}

/**
  * Brief   This function handles USART2 interrupt request.
  * Param   None
  * Retval  None
  */
void USART2_IRQHandler(void)
{
  if((USART2->ISR & USART_ISR_RXNE) == USART_ISR_RXNE)
  {
    CharToReceive = (uint8_t)(USART2->RDR); /* Receive data, clear flag */
    MyStateRegister |= CHAR_RECEIVED;
  }
  else if((USART2->ISR & USART_ISR_TC) == USART_ISR_TC)
  {
    if(uartsend == 8)
    {
      uartsend=4;
      USART2->ICR |= USART_ICR_TCCF; /* Clear transfer complete flag */
      /* Activate transmit disable flag */
    }
    else
    {
      /* clear transfer complete flag and fill TDR with a new char */
      USART2->TDR = ToEEPROM[uartsend++];
    }
  }
  else
  {
      NVIC_DisableIRQ(USART2_IRQn); /* Disable USART2_IRQn */
  }
}
