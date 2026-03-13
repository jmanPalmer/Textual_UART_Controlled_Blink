#varriables
MCU = atmega2560
F_CPU = 16000000UL
CC = avr-g++
OBJCOPY = avr-objcopy
CFLAGS = -mmcu=$(MCU) -DF_CPU=$(F_CPU) -Os -I./nanopb
TARGET = main

SRC = main.cpp data.pb.c nanopb/pb_common.c nanopb/pb_encode.c nanopb/pb_decode.c

all: $(TARGET).hex

#want to generate the .elf file from .c
$(TARGET).elf: $(SRC)
	$(CC) $(CFLAGS) -o $@ $^
#use the .elf to generate the .hex file as this is what the mega reads
%.hex: %.elf
	$(OBJCOPY) -O ihex $< $@

clean:
	rm -f *.elf *.hex