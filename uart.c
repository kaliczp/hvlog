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

void Configure_USART(void)
{
  volatile uint32_t tmpreg;
  /* GPIO configuration for USART signals */
  /* (0) Enable GPIOB clock */
  /* (1) Some delay based on LL */
  /* (2) Select AF mode (10) on PB6 (TX) and PB7 (RX) */
  /* push-pull default */
  /* (3) pull-up */
  /* (4) high speed */
  /* AF0 default value for USART signals in PB6 and 7 */
  RCC->IOPENR |= RCC_IOPENR_GPIOBEN; /* (0) */
  tmpreg = RCC->IOPENR; /* (1) */
  (void)tmpreg; /* (1) */

  /* Select LSE as USART clock source 11 */
  /* RCC->CCIPR |= RCC_CCIPR_USART1SEL; */

  GPIOB->MODER = (GPIOB->MODER & ~(GPIO_MODER_MODE6 | GPIO_MODER_MODE7)) \
    | (GPIO_MODER_MODE6_1 | GPIO_MODER_MODE7_1); /* (2) */
  GPIOB->PUPDR = (GPIOB->PUPDR & ~(GPIO_PUPDR_PUPD6 | GPIO_PUPDR_PUPD7)) | (GPIO_PUPDR_PUPD6_0 | GPIO_PUPDR_PUPD7_0); /* (3) */
  /* GPIOB->OSPEEDR = (GPIOB->OSPEEDR & ~(GPIO_OSPEEDER_OSPEED6)) | (GPIO_OSPEEDER_OSPEED6_1); /\* (4) *\/ */

  /* Enable the peripheral clock USART */
  RCC->APB2ENR |= RCC_APB2ENR_USART1EN;

  /* Configure USART */
  /* 2097152 / 1200 = 1748 */
  /* (1b) 1200 baud RM 0377 24.5.4 */
  /* (2a) oversampling by 16 no OVER8, 8 data bit, 1 start bit, no
     parity, receive and receive interrupt enabled and transmit */
  /* (2aa) Set ONEBIT to increase the USART tolerance to timing deviations */
  /* (2ab) 2 stop bits STOP[1:0] = 10 */
  /* (2b) UART enabled with default values above */
  USART1->BRR = 0b11011010100; /* (1) */
  USART1->CR1 |= USART_CR1_RXNEIE | USART_CR1_RE | USART_CR1_TE; /* (2a) */
  USART1->CR3 |= USART_CR3_ONEBIT; /* (2aa) */
  USART1->CR2 |= USART_CR2_STOP_1; /* (2ab) */
  USART1->CR1 |= USART_CR1_UE ; /* (2b) */
  /* Configure exti and IT */
  /* (7) unmask line 25 for USART */
  /* (8) Set priority for USART_IRQn to 1 0b01000000 */
  /* (9) Enable USART_IRQn */
  /* EXTI->IMR |= EXTI_IMR_IM25; /\* (7) *\/  */
  NVIC_SetPriority(USART1_IRQn, 1); /* (8) */
  NVIC_EnableIRQ(USART1_IRQn); /* (9) */
  /* Disable transmit */
  USART1->CR1 &= ~(USART_CR1_TE); /* (10) */
}

void EnableTransmit_USART(void)
{
  /* (3) Enable UART transmitter line */
  /* (5) poll the TEACK bit in the USART_ISR register */
  /* Wait for idle frame transmission maybe write 0 and 1 in TE */
  USART1->CR1 |= USART_CR1_TE ; /* (3) */
  while((USART1->ISR & USART_ISR_TEACK) != USART_ISR_TEACK) /* (5) */
    {
    }
}

void DisableTransmit_USART(void)
{
  /* (0) disable transmitter interrupt*/
  /* (1) disable transmitter */
  /* (2) wait until TC=1 avoid corrupt last transmission */
  USART1->CR1 &= ~(USART_CR1_TCIE); /* (0) */
  USART1->CR1 &= ~(USART_CR1_TE); /* (1) */
  USART1->ICR |= USART_ICR_TCCF; /* Clear transfer complete flag */
}

void Deconfigure_USART(void)
{
  /* (3) UART disabled */
  /* (4) Disable receiver */
  /* (4) Disable USART clock */
  /* (5) Disable GPIOB clock */
  NVIC_DisableIRQ(USART1_IRQn); /* Disable USART1_IRQn */
  /* EXTI->IMR &= ~(EXTI_IMR_IM25); /\* (5) *\/ */
  USART1->CR1 &= ~(USART_CR1_UE); /* (3) */
  USART1->CR1 &= ~(USART_CR1_RE | USART_CR1_RXNEIE); /* (4) */
  GPIOB->MODER |= (GPIO_MODER_MODE6 | GPIO_MODER_MODE7); /* (4a) */
  RCC->APB2ENR &= ~(RCC_APB2ENR_USART1EN); /* (4b) */
  RCC->IOPENR &= ~(RCC_IOPENR_GPIOBEN); /* (5) */
}

/**
  * Brief   This function handles USART interrupt request.
  * Param   None
  * Retval  None
  */
void USART1_IRQHandler(void)
{
  if((USART1->ISR & USART_ISR_RXNE) == USART_ISR_RXNE)
  {
    CharToReceive = (uint8_t)(USART1->RDR); /* Receive data, clear flag */
    MyStateRegister |= CHAR_RECEIVED;
  }
  else if((USART1->ISR & USART_ISR_TC) == USART_ISR_TC)
  {
    if(uartsend > TimeDateRegS.align)
    {
      uartsend = UFIRST_DATA;
      PtrTDTimeR = (uint8_t *)&TimeDateRegS.TimeRegister;
      USART1->ICR |= USART_ICR_TCCF; /* Clear transfer complete flag */
      /* Activate transmit disable flag */
    }
  }
  else if((USART1->ISR & USART_ISR_TXE) == USART_ISR_TXE)
  {
    if(uartsend == (TimeDateRegS.align))
    {
      /* (1) Disable TX register Empty interrupt */
      /* (2) Clear TC flag */
      /* (3) Enable TC interrupt */
      USART1->CR1 &= ~(USART_CR1_TXEIE); /* (1) */
      USART1->ICR |= USART_ICR_TCCF; /* (2) */
      USART1->CR1 |= USART_CR1_TCIE; /* (3) */
    }
    uartsend++;
    /* Fill TDR with a new data and clear transmit register empty flag */
    USART1->TDR = *PtrTDTimeR++;
  }
  else
  {
    /* Clear all possible flags */
    USART1->ICR |= USART_ICR_PECF | USART_ICR_FECF | USART_ICR_NCF | \
      USART_ICR_ORECF | USART_ICR_IDLECF | USART_ICR_TCCF | USART_ICR_CTSCF;
  }
}
