PROGRAM    = dxemu_bootloader
ISPTOOL	   = atmelice_isp

#LDSECTION  = --section-start=.text=0x3800

ISPFUSES    = avrdude -c $(ISPTOOL) -p $(MCU_TARGET) -e -u -U lock:w:0xff:m -U efuse:w:0x$(EFUSE):m -U hfuse:w:0x$(HFUSE):m -U lfuse:w:0x$(LFUSE):m
ISPFLASH    = avrdude -c $(ISPTOOL) -p $(MCU_TARGET) -U flash:w:$(PROGRAM).hex -U lock:w:0x0f:m

OBJ        = $(PROGRAM).o
OPTIMIZE   = -Os

DEFS       =
LIBS       =

CC         = avr-gcc

# Override is only needed by avr-lib build system.

override CFLAGS        = -g -Wall $(OPTIMIZE) -mmcu=$(MCU_TARGET) -DF_CPU=$(AVR_FREQ) $(DEFS)
override LDFLAGS       = -Wl,$(LDSECTION)
#override LDFLAGS       = -Wl,-Map,$(PROGRAM).map,$(LDSECTION)

OBJCOPY        = avr-objcopy
OBJDUMP        = avr-objdump

all:	TARGET = atmega328
all:	MCU_TARGET = atmega328p
all:	CFLAGS += '-DMAX_TIME_COUNT=F_CPU>>4' '-DNUM_LED_FLASHES=1' -DBAUD_RATE=57600
all:	AVR_FREQ = 14745600L
all:	LDSECTION  = --section-start=.text=0x7800
all:	$(PROGRAM).hex

flash:	TARGET = atmega328
flash:	MCU_TARGET = atmega328p
flash:	HFUSE = DA
flash:	LFUSE = FF
flash:	EFUSE = FD
flash:	isp

isp: $(TARGET)
	$(ISPFUSES)
	$(ISPFLASH)

%.elf: $(OBJ)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^ $(LIBS)

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
	
