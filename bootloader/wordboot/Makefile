PROGRAM     = wordboot
GCCROOT     =
OBJ         = $(PROGRAM).o
OPTIMIZE    = -Os -fno-split-wide-types -mrelax
LIBS        = -lc
CC          = $(GCCROOT)avr-gcc
CFLAGS      = -g -Wall $(OPTIMIZE) -mmcu=$(MCU_TARGET) -DF_CPU=$(AVR_FREQ)
LDFLAGS     = $(LDSECTIONS) -Wl,--relax -nostartfiles -nostdlib
OBJCOPY     = $(GCCROOT)avr-objcopy
OBJDUMP     = $(GCCROOT)avr-objdump
SIZE        = $(GCCROOT)avr-size

wordclock: TARGET = $@
wordclock: MCU_TARGET = atmega328p
wordclock: AVR_FREQ = 8000000L
wordclock: CFLAGS += -DBAUD_RATE=9600
wordclock: CFLAGS += -DLED_START_FLASHES=3
wordclock: CFLAGS += -DLED_DATA_FLASH=1
wordclock: $(PROGRAM).hex
wordclock: $(PROGRAM).lst

%.elf: $(OBJ)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $< $(LIBS)
	$(SIZE) $@

clean:
	rm -rf *.o *.elf *.lst *.map *.sym *.lss *.eep *.srec *.bin *.hex

%.lst: %.elf
	$(OBJDUMP) -h -S $< > $@

%.hex: %.elf
	$(OBJCOPY) -j .text -j .data -O ihex $< $@

%.srec: %.elf
	$(OBJCOPY) -j .text -j .data -O srec $< $@

%.bin: %.elf
	$(OBJCOPY) -j .text -j .data -O binary $< $@

