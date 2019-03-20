# STM32L0x1 Makefile for GNU toolchain 
#
# Usage:
#	make template		Makes template
#  	make clean		Clean all files of compilation
#  	make veryclean		Remove all system files and snippets link
#	make main.elf		Compile project
#	make main.bin		Make binary
#	make tags		Make etags for emacs

TARGET	= main

CUBE_DIR	= cube
CUBE_CMSIS	= $(CUBE_DIR)/Drivers/CMSIS
CUBE_CORE	= $(CUBE_CMSIS)/Include
CUBE_STM32	= $(CUBE_CMSIS)/Device/ST/STM32L0xx
CUBE_TMPL	= $(CUBE_STM32)/Source/Templates
CUBE_GENER	= ~/workspace/L0newboard

MCU_FAMILY	= stm32l0xx
MCU_LC		= stm32l051xx
MCU_UC		= STM32L051

# Toolchain
PREFIX	= arm-none-eabi
CC	= $(PREFIX)-gcc
OBJCOPY	= $(PREFIX)-objcopy

## Define correct mcu for header file selection an inline
DEFS	= -D$(MCU_UC)xx
## Possible command line user macro definition by UDEFS
## eg. make UDEFS=-DDEBUG

## Options compiler. -g flag for gdb.
## -Os flag for optimalisation for size
## -O0 or -Og for debugging.
CFLAGS	 = -Wall -g -Og
## Options architecture 
CFLAGS	+=  -mthumb -mcpu=cortex-m0plus -march=armv6-m
CFLAGS	+= -mlittle-endian

## Include files
INCS	 = -I$(CUBE_CORE)
INCS	+= -I$(CUBE_STM32)/Include
INCS	+= -I.

LDFLAGS	 = -Wl,--cref,--gc-sections,-Map=$(TARGET).map,-lgcc,-lc,-lnosys
LDFLAGS	+= -ffunction-sections -fdata-sections -T$(MCU_UC)K6Tx_FLASH.ld -L.
LDFLAGS += --specs=nano.specs

## Sources
SRCS := $(wildcard *.c)
OBJS := ${SRCS:.c = .o}

.PHONY: tags template clean veryclean

# %.o : %.c
#	$(CC) $(DEFS) $(CFLAGS) $(INCS) -c $< -o $@

$(TARGET).bin: $(TARGET).elf
	$(OBJCOPY) -O binary $(TARGET).elf $(TARGET).bin

$(TARGET).elf: startup_$(MCU_LC).s $(OBJS) 
	$(CC) $(UDEFS) $(DEFS) $(CFLAGS) $(INCS) $(LDFLAGS) $^ -o $@

## Emacs etags
tags:
	etags *h *c $(CUBE_CORE)/core_cm0plus.h $(CUBE_CORE)/arm_common_tables.h $(CUBE_CORE)/cmsis_gcc.h $(CUBE_STM32)/Include/$(MCU_LC).h system_$(MCU_FAMILY).c startup_$(MCU_LC).s $(MCU_UC)K6Tx_FLASH.ld

## Copy system files
template:
	cp -i $(CUBE_TMPL)/system_$(MCU_FAMILY).c .
	cp -i $(CUBE_TMPL)/gcc/startup_$(MCU_LC).s .
	cp -i $(CUBE_GENER)/$(MCU_UC)K6Tx_FLASH.ld .

## Clean all files of compilation
clean:
	rm -f *.o
	rm -f $(TARGET).bin
	rm -f $(TARGET).elf
	rm -f $(TARGET).map
	rm -f $(TARGET).lst

## Remove all system files and snippets link
veryclean:
	rm -f $(CUBE_DIR)
	rm -f system_$(MCU_FAMILY).c
	rm -f startup_$(MCU_LC).s
