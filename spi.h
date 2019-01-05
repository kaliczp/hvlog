/*
**********************************************************************
* Author    Péter Kalicz
* Version   V0.9
* Date      2018-05-01
* Brief     SPI header

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

#include "stm32l0xx.h"
#include "global.h"
#include "lptim.h"

void Configure_GPIO_SPI1(void);
void Deconfigure_GPIO_SPI1(void);
void Configure_SPI1(void);
void Activate_SPI1(void);
void Deactivate_SPI1(void);
void Deconfigure_SPI1(void);
void Write_SPI(uint8_t toeeprom[TO_EPR_LENGTH], uint8_t length);
