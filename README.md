What's this project for?

This project is planned as a small and simple replacement for dual 8" floppy drive GMD-70 (ГМД-70) used with Soviet made PDP-11 compatible computers DVK (ДВК) and Electronica-60 (Электроника-60). At the process of design, emulator is tested as working with MS1201.02 (МС1201.02) board, with I4 (И4) interface board and will be tested with MS1201.01 later, and maybe even with original PDP-11 RX01 controller card if I'll ever get one.

What's the purpose of this? There are several (planned) pros of this emulator:
- small size lets to install it directly into case of DVK-2M and maybe Electronica-60 (not yet tested if it fits).
- no need for enormous external 8" floppy drive what makes retro computer easier to demonstrate.
- it's possible to load floppy drive image over Arduino compatible TTL serial cable what makes it easier to exchange files with retro computer.
-  can be used to reliably boot computer and to initialise different boot media, for example, MX format 5" disks and MFM hard drives.
-  same connector that's used for floppy connection is also used for parallel printer connection. Centronics compatible connection to Epson dot-matrix printer is prototyped and tested.

cons:
- not as fast as I thought it will be because of flash memory 4K block erase only. It takes time to read-modify-write to change only 128-byte sector.
- computer board needs one small physical modification (shorted resistor) so it gets enough power from CPU board. Also, 5V power should be added to I4 board connector.

What's the status of this project? Basic functions was tested on prototype board. Virst version of dedicated PCB has been made. It emulates two floppy drives DX0: and DX1:, floppy image upload and download works over Xmodem. There's backup and restore functionality ready to be used. Bootloader has some unsorted issues. Printer port was working on prototype, but is not working on dedicated PCB. Emulator has to be tested on another boards - I4 and MS1201.01. Anyhow, RT-11 is booting stable, drives can be initialised, files can be read and written. 

How it's made? MCU program is written on native C language, it's compiled under linux CLI with simple makefile. Should be easily switch it to any IDE or even as Arduino project. Board will support Arduino-like bootloader for easy software upgrade over serial connection. Alos I had idea about firmware upgrade over Xmodem protocol, but I've not yet finished Arduino compatible loader. Schematics and board are (being) designed with P-CAD 2006. Yeah, I know. I'm an oldfart and pirate, but, hey, I'm grown to this CAD and while I can make anything I need with it, I'm still using it.
