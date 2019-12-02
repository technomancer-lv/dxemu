### What's this project for?
This project is small and simple replacement for dual 8" floppy drive GMD-70 (ГМД-70) used with Soviet made PDP-11 compatible computers DVK (ДВК) and Electronica-60 (Электроника-60). Emulator is tested as working with MS1201.02 (МС1201.02) board, with I4 (И4) interface board and will be tested with MS1201.01 later, and maybe even with original PDP-11 RX01 controller card if I'll ever get one.

### What's the purpose of this?
There are several (planned) pros of this emulator:
- small size lets to install it directly into case of DVK-2M and maybe Electronica-60 (not yet tested if it fits).
- no need for enormous external 8" floppy drive what makes retro computer easier to demonstrate.
- it's possible to load floppy drive image over Arduino compatible TTL serial cable what makes it easier to exchange files with retro computer.
-  can be used to reliably boot computer and to initialise different boot media, for example, MX format 5" disks and MFM hard drives.
-  same connector that's used for floppy connection is also used for parallel printer connection. Centronics compatible connection to Epson dot-matrix printer is prototyped and tested.
- bottom side of the board is isolated, so there's no risk of damaging it if bottom side is shorted to ground.

cons:
- not as fast as I thought it will be because of flash memory 4K block erase only. It takes time to read-modify-write to change only 128-byte sector.
- computer board needs one small physical modification (shorted resistor) so it gets enough power from CPU board. Also, 5V power should be added to I4 board connector with short wire.

### What's the status of this project?
It emulates two floppy drives DX0: and DX1:. Floppy image upload and download works over Xmodem, it's tested with minicom on Linux and with TeraTerm on Windows. There's backup and restore functionality ready to be used. Printer port is working with Epson dot-matrix printer. Bootloader seems to be working fine. Emulator has to be tested on MS1201.01. Anyhow, RT-11 is booting stable, drives can be initialised, files can be read and written. Small batch is going to be made soon, it will be fully tested and will be put for sale.

### How it's made?
MCU program is written on native C language, it's compiled under linux CLI. Makefile includes several tweaks for automatic including of serial number and build number in final binary file. Should be easily switch it to any IDE or even as Arduino project. Board supports Arduino-like bootloader for easy software upgrade over serial connection. Also I had an idea about firmware upgrade over Xmodem protocol, but I've put lot of time into this, I'm a bit tired of this and at the moment it's working stable so I'm going to do some other projects while trying to sell few assembled emulators. Schematics and board are designed with P-CAD 2006. Yeah, I know. I'm an oldfart and pirate, but, hey, I'm grown to this CAD and while I can make anything I need with it, I'm still using it.

### How to boot it for first time?
1. **+5V power** should be connected to connector where emulator is connected. On MS1201.01 and .02 bord there's resistor R16 that's going from +5V to "ЛОГ1" signal on floppy/printer connector (pin 56). As this signal is not used, resistor can be shorted with short wire, so there's +5V on that pin.
On И4 controller board, +5V should be connected to unused pin 35 with short wire. See pictures for examples (pictures to be added soon).  
2. **Connect emulator** to prepared board. There are markings on board which side connector is for MS1201 and which for И4.
3. **Switch on your computer.** Bootable RT-11 floppy image should be already programmed into emulator.
4. **Give boot command:**
    MS1201.01 - D0
    MS1201.02 - B DX0
    И4 - 177300G
   Activity LED of drive 0 should start blinking and computer should boot RT-11.
5. **That's it!** Enjoy Your emulated RX01 floppy drive and play some original Tetris game. Drive 1 are not initialised by default, but you can do it easily from RT-11 with command **INI DX1:** and You'll have another 256KB to store Your data.

### Printer port.
MS1201 boards have floppy and printer signals connected to same connector. As an additional function, printer connections are connected to pin header that can be used to connect dot-matrix printer with standard Centronics connection. You can use flat cable to DB-25F adaptor that's removed from old PC compatible motherboard. Connect printer to this adaptor cable and You can print text files from RT-11 with command **COP TEXT.TXT LP0:**. This function is tested with two different Epson printers and one Centronics printer.

### Built-in serial port.
Six pin header on board is Arduino pinout compatible TTL serial port. It can be used for firmware upgrade over Arduino compatible bootloader. Also, there is simple command-line interface, that can be used to upload and download floppy images, to locally backup and restore images and also to use debug and statistics functions. Baudrate for both bootloader and CLI is 115200. Emulator can be supplied standalone from serial connection.

### Console commands.
Commands are case insensitive. Available commands are:\
? (or any other unrecognised command) - help\
xs0 - send RX0 image over Xmodem\
xs1 - send RX1 image over Xmodem\
xr0 - receive RX0 image over Xmodem\
xr1 - receive RX1 image over Xmodem\
b0 - backup RX0 image\
b1 - backup RX1 image\
r0 - restore RX0 image from backup\
r1 - restore RX1 image from backup\
d0 - disable debug\
d1 - enable debug\
d2 - enable verbose debug\
v - show version\
s - statistics\

#### Floppy image upload to emulator.
Floppy images can be uploaded to emulator using Xmodem protocol. First, You need to have floppy image file with correct size - 256256 bytes. Connect serial cable, be sure that command line works and execute command **XR0** or **XR1** depending if You want to upload image of drive 0 or 1. After that, start image upload on your terminal emulator using Xmodem protocol and see if it uploads correctly. While uploading, image is copied to temporary area, so if upload fails, floppy image stays unchanged.

#### Floppy image download from emulator.
Floppy image can also be downloaded from emulator. It's started with command **XS0** or **XS1** and after that you should give receive command to your terminal emulator and see if it's downloading correctly. You'll get 256256 bytes large floppy image file that can be stored for future use, shared with other enthusiasts, written to real 8" floppy drive or loaded into PDP-11 emulator.

#### Backup and restore functions.
Sometimes while you're messing around with old hardware, you know that you can mess things up. In this case, you can accidentally erase floppy image, bake floppy unbootable or do some other stupid things. To save you from that, there's image backup and restore commmands. If you have good working floppy image, you can execute command **B0** or **B1** depending on which drive you would like to backup and image of that drive will be copied to backup area of Flash memory. If you mess something up, anytime you can execute **R0** or **R1** commands to restore previous contents of corresponding floppy drive. 0 and 1 drive image backup are stored in different locations, so both drives can be backuped simultaneously.

#### Debug.
To see if your computer is exchanging data with floppy drive, you can use debug functions. Command **D1** turns on simple debug. It shows command numbers issued by computer, together with track and sector numbers with commands that reads data from floppy or writes to it. Command **D2** turns on verbose debug that also prints out data that are stored in emulator buffer or are read from it. Verbose debug mode is not recommended to be used, because of manu data that has to be send over serial line. It slows things down and caused me some other problems. Maybe this debug should be made in different way. Use on your own risk.
Debug function is turned off with command **D0**.

#### Version.
Command **V** prints out serial number, bootloader version and firmware version. Firmware can be upgraded and so will change firmware number. Bootloader version and serial number are both stored in bootloader section, so these are unaffected by firmware changes.

#### Statistics.
Command **S** prints out simple statistics - board uptime, sectors read and written on both disks.
