This bootloader is based on Arduino compatible bootloader taken from here:
https://github.com/arduino/ArduinoCore-avr/tree/master/bootloaders/atmega
So all credits for bootloader source and makefile is going to authors mentioned in corresponding page. I made only small modifications, like, different F_CLK and LED on another pin.
Also, using srecord utilite, bootloader build number, date, time and device serial number are added at the end of bootloader section, so it can be read inside firmware for informative purposes.
0x7F00 - 0x7F3F - bootloader build information
0x7F40 - 0x7F7F - serial number information
