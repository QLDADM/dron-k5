# DRON-K5: dedykowany firmware skanera łączy dronów dla Quansheng UV-K5.
# Sterowniki sprzętu z egzumera (Apache 2.0), aplikacja własna. Radio nie nadaje.
TARGET = scanner
VERSION_STRING ?= DRONK5
AUTHOR_STRING ?= DRONK5

OBJS = start.o init.o external/printf/printf.o \
	driver/adc.o driver/bk4819.o driver/crc.o driver/eeprom.o driver/gpio.o \
	driver/i2c.o driver/keyboard.o driver/spi.o driver/st7565.o driver/system.o driver/systick.o \
	driver/uart.o driver/backlight.o board.o font.o ui/helper.o \
	app/scan.o app/store.o app/link.o app/sound.o app/screens.o main.o

TOP := $(shell pwd)
CC = arm-none-eabi-gcc
AS = arm-none-eabi-gcc
LD = arm-none-eabi-gcc
OBJCOPY = arm-none-eabi-objcopy
SIZE = arm-none-eabi-size

ASFLAGS = -c -mcpu=cortex-m0
CFLAGS = -Os -Wall -Werror -Wextra -mcpu=cortex-m0 -fno-builtin -fshort-enums -fno-delete-null-pointer-checks \
	-std=c2x -MMD -flto=auto -DPRINTF_INCLUDE_CONFIG_H -DENABLE_UART \
	-DVERSION_STRING=\"$(VERSION_STRING)\" -DAUTHOR_STRING=\"$(AUTHOR_STRING)\"
LDFLAGS = -z noexecstack -mcpu=cortex-m0 -nostartfiles -Wl,-T,firmware.ld -Wl,--gc-sections --specs=nano.specs
INC = -I $(TOP) -I $(TOP)/external/CMSIS_5/CMSIS/Core/Include/ -I $(TOP)/external/CMSIS_5/Device/ARM/ARMCM0/Include

all: $(TARGET)
	$(OBJCOPY) -O binary $< $<.bin
	-python fw-pack.py $<.bin $(AUTHOR_STRING) $(VERSION_STRING) $<.packed.bin
	$(SIZE) $<

$(TARGET): $(OBJS)
	$(LD) $(LDFLAGS) $^ -o $@

%.o: %.c
	$(CC) $(CFLAGS) $(INC) -c $< -o $@

%.o: %.S
	$(AS) $(ASFLAGS) $< -o $@

-include $(OBJS:.o=.d)

clean:
	rm -f $(TARGET) $(TARGET).bin $(TARGET).packed.bin $(OBJS) $(OBJS:.o=.d)
