/*
**********************************************************************
* @file     uart.c
* @version  V0.1
* @date     2017-03-19
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
  /* Enable PB7 input */
  /* (1) Enable GPIOB clock */
  /* (2) Set PB7 input mode */
  /* (3) Enable pull-down */

  RCC->IOPENR |= RCC_IOPENR_GPIOBEN; /* (1) */
  GPIOB->MODER = (GPIOB->MODER & ~(GPIO_MODER_MODE7)); /* (2) */
  GPIOB->PUPDR = (GPIOB->PUPDR & ~(GPIO_PUPDR_PUPD7)) | (GPIO_PUPDR_PUPD7_1); /* (3) */
}

void Deconfigure_GPIOB_Test(void)
{
  /* (1) Disable pull-down resistor */
  /* (2) Disable GPIOB clock */
  GPIOB->PUPDR = (GPIOB->PUPDR & ~(GPIO_PUPDR_PUPD7)); /* (1) */
  RCC->IOPENR &= ~ (RCC_IOPENR_GPIOBEN); /* (2) */
}

void Configure_USART1(void)
{
  /* GPIOB already running */
  /* GPIO configuration for USART1 signals */
  /* (0) Enable GPIOB clock */
  /* (1) Select AF mode (10) on PB6 and PB7 */
  /* AF0 default value for USART1 signals in PB6 and 7 */
  RCC->IOPENR |= RCC_IOPENR_GPIOBEN; /* (0) */
  GPIOB->MODER = (GPIOB->MODER & ~(GPIO_MODER_MODE6 | GPIO_MODER_MODE7))\
    | (GPIO_MODER_MODE6_1 | GPIO_MODER_MODE7_1); /* (1) */

  /* Select LSE as USART1 clock source 11 */
  RCC->CCIPR |= RCC_CCIPR_USART1SEL;
  /* Enable the peripheral clock USART1 */
  RCC->APB2ENR |= RCC_APB2ENR_USART1EN;

  /* Configure USART1 */
  /* (1) oversampling by 8, 1200 baud because OVER8 */
  /* (2) 8 data bit, 1 start bit, 1 stop bit, no parity*/
  USART1->BRR = 0b11011; /* 32768 / 1200 = 27 */; /* (1) */
  USART1->CR1 = USART_CR1_TE | USART_CR1_UE ; /* (2) */

  /* polling idle frame Transmission */
  while((USART1->ISR & USART_ISR_TC) != USART_ISR_TC)
    {
      /* add time out here for a robust application */
    }
  USART1->ICR |= USART_ICR_TCCF; /* clear TC flag */
}

void Deconfigure_USART1(void)
{
  /* Disable USART1 clock */
  RCC->APB2ENR &= ~ (RCC_APB2ENR_USART1EN);
}
