
MCU=atmega328p
F_CPU=14745600
CC=avr-gcc
OBJCOPY=avr-objcopy
CFLAGS=-std=c99 -Wall -g -Os -mmcu=${MCU} -DF_CPU=${F_CPU} -I.
TARGET=dxemu_m328p_latest
SRCS=main.c

all:
	clear
	${CC} ${CFLAGS} -o ${TARGET}.bin ${SRCS}
	${OBJCOPY} -j .text -j .data -O ihex ${TARGET}.bin ${TARGET}.hex

flash_isp:
	avrdude -p ${MCU} -c atmelice_isp -U flash:w:${TARGET}.hex:i -F -P usb -B 2

flash_bld:
	avrdude -p ${MCU} -c arduino -P /dev/ttyUSB0 -b 57600 -D -U flash:w:${TARGET}.hex:i

clean:
	rm -f *.bin *.hex
