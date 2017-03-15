# STM32L0x1 Makefile for GNU toolchain 
#
# Usage:
#	make template		Makes template
#  	make clean		Clean all files of compilation
#  	make veryclean		Remove all system files and snippets link
#	make main.elf		Compile project
#	make main.bin		Make binary

TARGET	= main

SNIPPETS_DIR	= snippets
SNIPPETS_TMPL	= $(SNIPPETS_DIR)/Drivers/CMSIS/Device/ST/STM32L0xx/Source/Templates

MCU_FAMILY	= stm32l0xx
MCU_LC		= stm32l051xx
MCU_UC		= STM32L051xx

# Toolchain
PREFIX	= arm-none-eabi
CC	= $(PREFIX)-gcc
OBJCOPY	= $(PREFIX)-objcopy

## Define correct mcu for header file selection an inline
DEFS	= -D$(MCU_UC)
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
INCS	 = -I$(SNIPPETS_DIR)/Drivers/CMSIS/Include
INCS	+= -I$(SNIPPETS_DIR)/Drivers/CMSIS/Device/ST/STM32L0xx/Include
INCS	+= -I.

LDFLAGS	 = -Wl,--gc-sections,-Map=$(TARGET).map,-lgcc,-lc,-lnosys
LDFLAGS	+= -ffunction-sections -fdata-sections -T$(MCU_LC).ld -L.

## Sources
SRCS := $(wildcard *.c)
OBJS := ${SRCS:.c = .o}

.PHONY: template clean veryclean

# %.o : %.c
#	$(CC) $(DEFS) $(CFLAGS) $(INCS) -c $< -o $@

$(TARGET).bin: $(TARGET).elf
	$(OBJCOPY) -O binary $(TARGET).elf $(TARGET).bin

$(TARGET).elf: startup_$(MCU_LC).s $(OBJS) 
	$(CC) $(UDEFS) $(DEFS) $(CFLAGS) $(INCS) $(LDFLAGS) $^ -o $@

## Copy system files
template:
	cp -i $(SNIPPETS_TMPL)/system_$(MCU_FAMILY).c .
	cp -i $(SNIPPETS_TMPL)/gcc/startup_$(MCU_LC).s .

## Clean all files of compilation
clean:
	rm -f *.o
	rm -f $(TARGET).bin
	rm -f $(TARGET).elf
	rm -f $(TARGET).map
	rm -f $(TARGET).lst

## Remove all system files and snippets link
veryclean:
	rm -f $(SNIPPETS_DIR)
	rm -f system_$(MCU_FAMILY).c
	rm -f startup_$(MCU_LC).s
