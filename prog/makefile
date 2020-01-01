
MCU=atmega328p
F_CPU=14745600
CC=avr-gcc
OBJCOPY=avr-objcopy
CFLAGS=-std=c99 -Wall -g -Os -mmcu=${MCU} -DF_CPU=${F_CPU} -I.
#CFLAGS=-std=c99 -Wall -g -O1 -mmcu=${MCU} -DF_CPU=${F_CPU} -I.
TARGET=dxemu_m328p_latest
SRCS=main.c
BUILD_NUMBER_FILE=build_number.txt

all:
	clear
	@if ! test -f $(BUILD_NUMBER_FILE); then echo 0 > $(BUILD_NUMBER_FILE); fi
	@echo $$(($$(cat $(BUILD_NUMBER_FILE)) + 1)) > $(BUILD_NUMBER_FILE)
	@#date "+%d-%m-%Y %H:%M" > build_date.txt
	@date "+%d-%m-%Y" > build_date.txt
	${CC} ${CFLAGS} -o ${TARGET}.bin ${SRCS}
	@${OBJCOPY} -j .text -j .data -O ihex ${TARGET}.bin ${TARGET}.hex
	@${OBJCOPY} -j .text -j .data -O binary ${TARGET}.bin ${TARGET}.bin

	@#adding version and build number to firmware file
	@cat version_number.txt | tr '\n' ' ' > temp.txt;printf "build " >> temp.txt;cat build_number.txt | tr '\n' ' ' >> temp.txt;cat build_date.txt | tr '\n' ' ' >> temp.txt;srec_cat ${TARGET}.hex -intel temp.txt -binary -offset 0x7700 -o ${TARGET}.hex -intel;rm temp.txt
	
	@echo
	@echo "Build number:  "$$(cat $(BUILD_NUMBER_FILE))
	@echo "Bin file size: "$$(ls ${TARGET}.bin -l| awk '{print $$5}')
	@echo
	
flash_isp:
	avrdude -p ${MCU} -c atmelice_isp -U flash:w:${TARGET}.hex:i -U eeprom:w:eeprom.bin:r -F -P usb -B 2

reset_isp:
	avrdude -p ${MCU} -c atmelice_isp -F -P usb -B 2


flash_bld:
	@#avrdude -p ${MCU} -c arduino -P /dev/ttyUSB0 -b 115200 -D -U flash:w:${TARGET}.hex:i -U eeprom:w:eeprom.bin:r
	avrdude -p ${MCU} -c arduino -P /dev/ttyUSB0 -b 115200 -D -U flash:w:${TARGET}.hex:i

reset_bld:
	avrdude -p ${MCU} -c arduino -P /dev/ttyUSB0 -b 115200 -D

clean:
	rm -f *.bin *.hex
