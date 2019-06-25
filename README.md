What's this project for?

This project is planned as a small and simple replacement for dual 8" floppy drive GMD-70 (ГМД-70) used with Soviet made PDP-11 compatible computers DVK (ДВК) and Electronica-60 (Электроника-60). At the process of design, emulator is tested as working with MS1201.02 (МС1201.02) board, but will be tested with MS1201.01 later, and also with I4 (И4) interface board, and maybe even with original PDP-11 RX01 controller card if I'll ever get one.

What's the purpose of this? There are several (planned) pros of this emulator:
	+ small size lets to install it directly into case of DVK-2M and maybe Electronica-60 (not tested yet).
	+ no need for enormous external 8" floppy drive what makes retro computer easier to demonstrate.
	+ it's possible to load floppy drive image over Arduino compatible TTL serial cable what makes it easier to exchange files with retro computer.
	+ can be used to reliably boot computer and to make another boot disks, for example, MX format 5" disks.
	+ same connector that's used for floppy connection is also used for parallel printer connection. I'm planning to add Centronics compatible printer port connector on finished board.

cons:
    - not as fast as I thought it will be because of flash memory 4K block erase only. It takes time to read-modify-write to change only 128-byte sector.
    - computer board needs one small physical modification (shorted resistor) so it gets enough power from CPU board.
    - directly compatible with MC1201.01 and .02 boards (DVK computer), but it needs adapter cable to connect it to I4 floppy interface board used in Electronica-60 computer as it uses different pinout than DVK. If I'll manage to stuff all on one board, there might be second connector on my emulator board for compatibility with I4.

What's the status of this project? At the moment I have made prototype on prototyping board. It's working with basic features - it emulates two floppy drives DX0: and DX1:. RT-11 is booting stable, drives can be initialised, files can be read and written. Additional features will be added soon, like, simple command line interface on serial line, floppy image download and upload over xmodem protocol, Arduino compatible loader for firmware upgrades, printer port connector I mentioned before.

How it's made? MCU program is written on native C language, it's compiled under linux CLI with simple makefile. Should be easily switch it to any IDE or even as Arduino project. Program will support emulation of basic RX01 functions, there will be Xmodem support to up/download floppy imges over serial connection. Also, I have idea about backup function where You can save drive 0 and 1 contents to other flash memory region to restore disk contents in case You mess up. Board will support Arduino-like bootloader for easy software upgrade over serial connection. Schematics and board are (being) designed with P-CAD 2006. Yeah, I know. I'm an oldfart and pirate, but, hey, I'm grown to this CAD and while I can make anything I need with it, I'm still using it. At the moment I'm busy with mesed up printer interface and PCB design, so for some time there will be no updates on firmware.
