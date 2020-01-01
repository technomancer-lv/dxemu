/*	Program for emulating PDP-11 RX01 floppy disk drive
*
*	CPU:	ATMega328PB
*	F_CLK:	14,7456MHz
*
*	Fuses:
*
*	Pin naming is taken from KR1801VP1-033 interface pin naming.
*	Data is shifted MSB first
*	Command and both addresses are 9-bit long with parity bit as LSB
*
*	TODO:	Write marked sectors
*		Add 12bit compatibility?
*		Parity check (in progress)
*		Function time-outs
*		Divide code into separate files
*		Statistics - error count
*
*/

#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include "avr_uart_fifo.h"

//DX drive IO pin init
//"Signal name in russian" (controller pin name) (I/O type)
//"Nach. ustanovka" (SET) (IN)
#define	DxSetPort	PORTD
#define	DxSetIn		PIND
#define	DxSetDdr	DDRD
#define	DxSetPin	3

//"Pusk" (RUN) (IN)
#define	DxRunPort	PORTD
#define	DxRunIn		PIND
#define	DxRunDdr	DDRD
#define	DxRunPin	2

//Dannie (DI) (I)
#define	DxDiPort	PORTB
#define	DxDiIn		PINB
#define	DxDiDdr		DDRB
#define	DxDiPin		0

//Dannie (DO) (O)
#define	DxDoPort	PORTB
#define	DxDoIn		PINB
#define	DxDoDdr		DDRB
#define	DxDoPin		1

//Treb. peredachi (IR) (OUT)
#define	DxIrPort	PORTB
#define	DxIrIn		PINB
#define	DxIrDdr		DDRB
#define	DxIrPin		2

//Zaversheno (DONE) (OUT)
#define	DxDonePort	PORTD
#define	DxDoneIn	PIND
#define	DxDoneDdr	DDRD
#define	DxDonePin	6

//Vivod (OUT) (OUT)
#define	DxOutPort	PORTD
#define	DxOutIn		PIND
#define	DxOutDdr	DDRD
#define	DxOutPin	5

//Oshibka (ERR) (OUT)
#define	DxErrPort	PORTD
#define	DxErrIn		PIND
#define	DxErrDdr	DDRD
#define	DxErrPin	7

//Sdvig (SHFT) (OUT)
#define	DxShftPort	PORTD
#define	DxShftIn	PIND
#define	DxShftDdr	DDRD
#define	DxShftPin	4

//TODO - add 12bit input pin and functionality
//so this would be compatible with ... what?
//12bit (IN)

#define	Dx12bitPort	PORTC
#define	Dx12bitIn	PINC
#define	Dx12bitDdr	DDRC
#define	Dx12bitPin	5

//UART
//Tx
#define	UartTxPort	PORTD
#define	UartTxIn	PIND
#define	UartTxDdr	DDRD
#define	UartTxPin	1

//Rx
#define	UartRxPort	PORTD
#define	UartRxIn	PIND
#define	UartRxDdr	DDRD
#define	UartRxPin	0

//SPI
//CE (software controlled)
#define	SpiCePort	PORTC
#define	SpiCeIn		PINC
#define	SpiCeDdr	DDRC
#define	SpiCePin	1

//MISO
#define	SpiMisoPort	PORTB
#define	SpiMisoIn	PINB
#define	SpiMisoDdr	DDRB
#define	SpiMisoPin	4

//MOSI
#define	SpiMosiPort	PORTB
#define	SpiMosiIn	PINB
#define	SpiMosiDdr	DDRB
#define	SpiMosiPin	3

//SCK
#define	SpiSckPort	PORTB
#define	SpiSckIn	PINB
#define	SpiSckDdr	DDRB
#define	SpiSckPin	5

//Activity LED
#define	ActLedPort	PORTC
#define	ActLedIn	PINC
#define	ActLedDdr	DDRC
#define	ActLed0Pin	2
#define	ActLed1Pin	3

//Memory regions
#define	Dx0MemStart	0x000000
#define	CopyMemStart	0x040000
#define	Dx1MemStart	0x080000
#define	XmodemMemStart	0x0C0000
#define	Dx0BackupStart	0x100000
#define	Dx1BackupStart	0x180000

//TODO - change all mem offsets with these defines

//Welcome text, saved in program memory
unsigned char const StartupText[] PROGMEM="\r\n-----\r\nRX01 drive emulator for PDP-11 compatible computers.\r\nTested with MC1201.01, MC1201.02 and I4 boards.\r\nCreated by Technomancer\r\nphantom.sannata.ru";

//Variables
unsigned char	DxDriveSelected=0;	//Variable that holds selected drive number
					//0 - DX0
					//1 - DX1

unsigned char	CopyArray[128];		//Tempraroy data buffer array, used for data transfer over Xmodem
					//and data copying between data blocks

unsigned char 	DxArray[128];		//DX data buffer array, 128 bytes
unsigned char	DxArrayPointer=0;	//Pointer for saving and reading data from array
unsigned char	SecAddr=0;		//Sector address received from controller
unsigned char	TrackAddr=0;		//Track address received from controller

unsigned char	DxCommand=0xFF;		//DX command received from controller. Used in state machine
#define	NoCommand	0xFF		//If no command is being executed at the moment, DxCommand=0xFF

unsigned char	DxState=0;			//State machine state variable
#define	IdleState		0		//Idle state - no command are being executed
#define	SecAddrWaitState	1		//Waits for sector address (Commands 2,3,6)
#define	TrackAddrWaitState	2		//Waits for track address (Commands 2,3,6)
#define	FillBufferState		3
#define	ReadBufferState		4

unsigned char	DxStatusReg=0b10000100;
#define	DxParityErrorBit	1
#define	DxMarkedSectorActionBit	6

unsigned char	DxErrorReg=0;

unsigned char	TempBlockNum=0;

unsigned char	SystemStatus=0;
#define	DebugOn		0
#define	DebugVerbose	1

unsigned char 	DxTimeout=0;
unsigned int 	XmodemTimeout=0;

unsigned int	DxSectorsWritten[2]={0,0};
unsigned int	DxSectorsRead[2]={0,0};
unsigned char	SystemUptimeDiv=0;
unsigned int	SystemUptime=0;

unsigned 	DebugDiv=0;
//Functions
unsigned char	ShiftInP(void);				//Function for shifting in command and address with parity bit
							//from the controller. 0xFF - parity error
unsigned char	ShiftIn(void);				//Function for shifting in data from controller with no parity

void		ShiftOut(unsigned char ShiftByte);	//Function for shifting out data to controller

unsigned char	SpiSend(unsigned char SpiData);

/* Functions for write/read to/from flash memory
*  TrkNum - track number  (0-77 for DX drive)
*  SecNum - sector number (0-26 for DX drive)
*  DiskNum - disk number  (0,1 - DX drives)
*/

void		RomSetStatus(unsigned char RomStatus);
unsigned char	RomGetStatus(void);
void		RomEnWrite(void);
void		RomEraseBlock(unsigned long int EraseBlockAddr);
void		RomEraseImage(unsigned long int EraseBlockAddr);
unsigned char 	RomRead(unsigned char DiskNum, unsigned char TrkNum,unsigned char SecNum);	//Reads 128-byte sector from flash memory
unsigned char 	RomWrite(unsigned char DiskNum, unsigned char TrkNum,unsigned char SecNum);	//Writes 128-byte sector to flash memory
void		RomSectorRead(unsigned long int SectorReadAddr,unsigned char SectorReadBuffer);
void		RomSectorWrite(unsigned long int SectorWriteAddr,unsigned char SectorWriteBuffer);
void		RomCopyImage(unsigned long int ImageSourceAddr,unsigned long int ImageDestAddr);

void		ExitState(void);			//Function that exits current state because of reset signal or error

void		HexSend(unsigned char HexChar);
void		DecSend(unsigned int DecData,unsigned char DecPadding);

unsigned char	Xtransmit (unsigned char DriveNum);

#include "cli_dxemu.h"

// ProgramStart
int main()
{
	//INIT
	//port init
	//DX SET (NACH.UST.)  - input, pulled high
	DxSetPort|=(1<<DxSetPin);
	DxSetDdr&=~(1<<DxSetPin);

	//DX RUN (PUSK)  - input, pulled high
	DxRunPort|=(1<<DxRunPin);
	DxRunDdr&=~(1<<DxRunPin);

	//DX data in  - input, pulled high
	DxDiPort|=(1<<DxDiPin);
	DxDiDdr&=~(1<<DxDiPin);

	//DX 12bit - input, pulled high
	Dx12bitPort|=(1<<Dx12bitPin);
	Dx12bitDdr&=~(1<<Dx12bitPin);

	//DX data out pin - output, high (pulled high on bidir line in computer)
	DxDoPort|=(1<<DxDoPin);
	DxDoDdr|=(1<<DxDoPin);

	//DX IR (TREB.PEREDACHI).  - output, high
	DxIrPort|=(1<<DxIrPin);
	DxIrDdr|=(1<<DxIrPin);

	//DX DONE (ZAVERSHENO)  - output, high
	DxDonePort|=(1<<DxDonePin);
	DxDoneDdr|=(1<<DxDonePin);

	//DX OUTPUT (VYVOD)  - output, high
	DxOutPort|=(1<<DxOutPin);
	DxOutDdr|=(1<<DxOutPin);

	//DX ERROR (OSHIBKA)  - output, high
	DxErrPort|=(1<<DxErrPin);
	DxErrDdr|=(1<<DxErrPin);

	//DX SHIFT (SDVIG)  - output, high
	DxShftPort|=(1<<DxShftPin);
	DxShftDdr|=(1<<DxShftPin);


	//UART init
	UartTxDdr|=(1<<UartTxPin);	//UART Tx - OUT

	UartRxPort|=(1<<UartRxPin);	//UART Rx - IN, pulled high
	UartRxDdr&=~(1<<UartRxPin);
	UCSR0A=0b00000010;
	UCSR0B=0b11011000;	//Tx enabled, RX enabled
	UCSR0C=0b00000110;	//
	UBRR0H=0;
	UBRR0L=15;		//115200 8N1

	//SPI-mem init
	SpiCePort|=(1<<SpiCePin);	//SPI CE - OUT, init high
	SpiCeDdr|=(1<<SpiCePin);
	SpiMisoPort|=(1<<SpiMisoPin);
	SpiMisoDdr&=~(1<<SpiMisoPin);
	SpiMosiDdr|=(1<<SpiMosiPin);
	SpiSckDdr|=(1<<SpiSckPin);

	SPCR=0b01010000;	//mode0, fosc/2
	SPSR=0b00000001;	//SPI2X=1

	//TIMER1 init
	TCCR1A=0b00000000;	//Normal port operation, CTC mode
	TCCR1B=0b00001100;	//CTC mode,Fclk/256
	TCCR1C=0b00000000;

	OCR1A=5759;		//Period of 0,1s=14,7456MHz/256/5760

	TIMSK1=0b00000010;

	//INT init
	sei();

	//Activity LED 0 active high, out
	ActLedPort&=~(1<<ActLed0Pin);
	ActLedPort&=~(1<<ActLed1Pin);
	ActLedDdr|=(1<<ActLed0Pin);
	ActLedDdr|=(1<<ActLed1Pin);

	UartInit(115200);

	unsigned int TextPointer=&(StartupText[0]);
	while(1)
	{
		unsigned char TempChar=pgm_read_byte(TextPointer);
		if(TempChar==0)
			break;
		UartTxAddByte(TempChar);
		TextPointer++;
	}

	_delay_ms(100);
	CliInit();

	_delay_ms(1000);		//Startup delay
	DxDonePort&=~(1<<DxDonePin);	//Ready to receive command

	//ROM init - disable write protection
	RomSetStatus(0b00000000);

	//========== MAIN LOOP ==========

	while(1)
	{
		//TODO - make this process edge sensitive (INT)
		if ((DxRunIn&(1<<DxRunPin))==0)
		{
			if(DxCommand==NoCommand)
			{

				//Controller has initiated command transfer. Read command over SPI-DX.
				DxDonePort|=(1<<DxDonePin);	//Reply with command sequence started
				DxErrPort|=(1<<DxErrPin);	//Clears error signal
				DxTimeout=0;

				//Read command - shift in from controller
				unsigned char CommandTemp=ShiftInP();
				//TODO - parity test goes here
				DxDriveSelected=((CommandTemp>>4)&0x01);	//Saves selected drive
				DxCommand=((CommandTemp>>1)&7);			//Saves command
				if(SystemStatus&(1<<DebugOn))
				{
					DebugDiv=0;
					UartTxAddByte('C');
					UartTxAddByte(' ');
					UartTxAddByte(DxCommand+0x30);
					UartTxAddByte(' ');
				}
			}

			switch (DxCommand)
			{
				case 0:	//Fill buffer command
				{
					if(DxState==IdleState)		//Command received, initialise receive
					{
						DxState=FillBufferState;
						DxIrPort&=~(1<<DxIrPin);
						DxArrayPointer=0;
						if(SystemStatus&(1<<DebugVerbose))
						{
							UartSendString("\x0D\x0A");
							DebugDiv=0;
						}
					}
					else				//Receive 128 bytes from controller
					{
						DxArray[DxArrayPointer]=ShiftIn();
						if(SystemStatus&(1<<DebugVerbose))
						{
							HexSend(DxArray[DxArrayPointer]);
							UartTxAddByte(' ');
							DebugDiv++;
							if(DebugDiv>=8)
							{
								DebugDiv=0;
								UartSendString("\x0D\x0A");
							}
						}

						DxArrayPointer++;

						if(DxArrayPointer==128)
							ExitState();
						else
							DxIrPort&=~(1<<DxIrPin);
					}
					break;
				}

				case 1:	//Read buffer command
				{

					if(DxArrayPointer<128)
					{
						if(DxState==IdleState)
						{
						//TODO - merge with else
							DxState=ReadBufferState;
							DxOutPort&=~(1<<DxOutPin);
							_delay_us(10);
							ShiftOut(DxArray[0]);
							DxArrayPointer=1;
							DxIrPort&=~(1<<DxIrPin);
						}
						else
						{
							DxIrPort|=(1<<DxIrPin);
							ShiftOut(DxArray[DxArrayPointer]);
							DxArrayPointer++;
							DxIrPort&=~(1<<DxIrPin);
						}

					}
					else
					{
						DxIrPort|=(1<<DxIrPin);
						ExitState();
						if(SystemStatus&(1<<DebugVerbose))
						{
							for(DxArrayPointer=0;DxArrayPointer<128;DxArrayPointer++)
							{
								HexSend(DxArray[DxArrayPointer]);
								UartTxAddByte(' ');
								DebugDiv++;
								if(DebugDiv>=8)
								{
									DebugDiv=0;
									UartSendString("\x0D\x0A");
								}
							}
							UartSendString("\x0D\x0A");
						}

					}
					break;
				}

				case 2:	//Write sector to disk command
				case 3:	//Read sector from disk command
				case 6:	//Write marked sector to disk command
				{
					if(DxState==IdleState)
					{
						DxState=SecAddrWaitState;	//Receives sector address
						DxIrPort&=~(1<<DxIrPin);
						break;
					}
					if(DxState==SecAddrWaitState)
					{
						unsigned int	SectorTemp=ShiftInP();
						//TODO - parity test
						SecAddr=(SectorTemp-1);
						if (SecAddr>26)
						{
							DxErrPort&=~(1<<DxErrPin);
							ExitState();
							break;
							//TODO - add error variable
						}
						DxState=TrackAddrWaitState;
						DxIrPort&=~(1<<DxIrPin);
						break;
						//TODO - error if parity error

					}
					if(DxState==TrackAddrWaitState)
					{
						unsigned int	TrackTemp=ShiftInP();
						//TODO - parity test and exit
						TrackAddr=TrackTemp;
						if (TrackAddr>76)
						{
							DxErrPort&=~(1<<DxErrPin);
							ExitState();
							break;
							//TODO - add error variable
						}

						if(SystemStatus&(1<<DebugOn))
						{
							UartTxAddByte('D');
							UartTxAddByte(' ');
							DecSend(DxDriveSelected,0);
							UartTxAddByte('T');
							UartTxAddByte(' ');
							DecSend(TrackAddr,0);
							UartTxAddByte(' ');
							UartTxAddByte('S');
							UartTxAddByte(' ');
							DecSend(SecAddr,0);
						}

						switch (DxCommand)
						{
							case 2:
							case 6:
							{
								RomWrite(DxDriveSelected,TrackAddr,SecAddr);
								break;
							}

							case 3:
							{
								RomRead(DxDriveSelected,TrackAddr,SecAddr);
								break;
							}
						}
						ExitState();
					}
					break;
				}

				case 4:	//Unused
				{
					ExitState();
					break;
				}

				case 5:	//Read error and status register command
				{
					DxOutPort&=~(1<<DxOutPin);
					_delay_us(10);
					ShiftOut(DxStatusReg);
					ExitState();
					break;
				}

				case 7:	//Read disk error register command
				{
					DxOutPort&=~(1<<DxOutPin);
					_delay_us(10);
					ShiftOut(DxErrorReg);
					ExitState();
					break;
				}

				default:
				{
					ExitState();
					break;
				}
			}
		}
		//TODO - make reset edge sensitive (INT)
		if((DxSetIn&(1<<DxSetPin))==0)
		{
			//Fill buffer with drive 0 track 1 sector 0
			//TODO: make reset edge sensitive
			RomRead(0,1,0);
			DxDonePort|=(1<<DxDonePin);
			_delay_ms(100);
			DxOutPort&=~(1<<DxOutPin);
			_delay_us(10);
			ShiftOut(0b10000100);
			if(SystemStatus&(1<<DebugOn))
			{
				UartSendString("RST");
			}
			ExitState();
		}

		//CLI process call
		if((UartFlags&(1<<UartRxFifoEmpty))==0)
		{
		unsigned char RxTempLoop=UartRxGetByte();
		CliRoutine(RxTempLoop);
		}
	}
}

unsigned char	ShiftInP(void)		//Function for shifting in commands and addresses from controller
{
	DxIrPort|=(1<<DxIrPin);
	unsigned char ShiftData=0;
	unsigned char ParityBit=0;
	for(unsigned char ShiftCount=0;ShiftCount<8;ShiftCount++)
	{
		ShiftData=(ShiftData<<1);
		if((DxDiIn&(1<<DxDiPin))==0)
			{
			ShiftData++;	//If received 1, set data LSB to one
			ParityBit++;	//Increase parity counter on every received 1
			}
		DxShftPort&=~(1<<DxShftPin);	//Shift pulse
		_delay_us(1);
		DxShftPort|=(1<<DxShftPin);
		_delay_us(2);
	}
	if((DxDiIn&(1<<DxDiPin))==0)
		ParityBit++;		//Increase parity counter also on received parity 1
					//If there are no parity errors, parity LSB
					//should always be 1.
					//Parity is tested here
	if((ParityBit&1)==1)
	{
		return ShiftData;
	}
	else
	{
		UartSendString("PARITY ERROR");
		return 0xFF;		//Received command or track/sector address should never be
					//0xFF, so I can use this value as an error code.
	}
}

unsigned char	ShiftIn(void)		//Function for shifting in data from controller
{
	DxIrPort|=(1<<DxIrPin);		//Data transmission started, IR signal can be disabled
	unsigned char ShiftData=0;
	for(unsigned char ShiftCount=0;ShiftCount<8;ShiftCount++)
	{				//Shifts in 8 data bits from computer
		ShiftData=(ShiftData<<1);
		if((DxDiIn&(1<<DxDiPin))==0)
			ShiftData+=1;
		DxShftPort&=~(1<<DxShftPin);	//Shift pulse
		_delay_us(1);
		DxShftPort|=(1<<DxShftPin);
		_delay_us(2);
	}
	return ShiftData;		//Returns received data
}


void		ShiftOut(unsigned char ShiftByte)	//Function for shifting out data to controller
{
	for(unsigned char ShiftCount=0;ShiftCount<8;ShiftCount++)
	{						//Shifts out eight data bits
		if((ShiftByte&0b10000000)==0)
			DxDoPort|=(1<<DxDoPin);
		else
			DxDoPort&=~(1<<DxDoPin);
		ShiftByte=(ShiftByte<<1);
		_delay_us(1);
		DxShftPort&=~(1<<DxShftPin);
		_delay_us(1);
		DxShftPort|=(1<<DxShftPin);
		_delay_us(2);
	}
	DxDoPort|=(1<<DxDoPin);				//Data line - idle
}


unsigned char	SpiSend(unsigned char SpiData)		//Function for data receive/transmit to ROM over SPI
{
	SPDR=SpiData;					//Outputs data
	while((SPSR&0b10000000)==0);			//Waits for data to be shifted out
	unsigned char SpiTemp=SPDR;			//Reads received data
	return(SpiTemp);				//Returns received data
}


/*
* Data map in ROM:
000000-03FFFF:	DX0 disk
040000-07FFFF:	Temporary data blocks (sectors)
080000-0FFFFF:	DX1 disk
040000-07FFFF:	Temporary data blocks (Xmodem)
100000-13FFFF:	backup for DX0
180000-1BFFFF:	backup for DX1

DX disks populates only about half of 4MB assigned to each disk.
Flash memory can be only erased by 4KB blocks, so there's need
for temporary storage when one of 32 emulated sectors located in
flash memory block that is erased. So second half of DX0 disk is used
as a blocks for temporary storage while changing one sector. So:
040000-07FFFF:  64 temporary blocks

Also, temporary location is used for temporary saving  image
that's being received over serial line using Xmodem. After
successfully receiving full image, it's copied to desired
floppy area.
*/

void		RomSetStatus(unsigned char RomStatus)	//Function that sets flash memory status byte
{
	SpiCePort&=~(1<<SpiCePin);
	_delay_us(2);
	SpiSend(0x50);			//Enable write status command
	SpiCePort|=(1<<SpiCePin);
	_delay_us(2);

	SpiCePort&=~(1<<SpiCePin);
	_delay_us(2);
	SpiSend(0x01);			//Write status command
	SpiSend(RomStatus);		//Set status
	SpiCePort|=(1<<SpiCePin);
	_delay_us(2);
}

unsigned char	RomGetStatus(void)			//Function that reads flash memory status 
							//Used to determine if write is busy
{
	SpiCePort&=~(1<<SpiCePin);
	_delay_us(2);
	SpiSend(0x05);					//Read status command


	unsigned char StatTemp=SpiSend(0);		//Get status value
	SpiCePort|=(1<<SpiCePin);
	_delay_us(2);
	return StatTemp;				//Returns status value
}

void		RomEnWrite(void)			//Function enables flash memory write
							//Should be executed before erase or write
{
	SpiCePort&=~(1<<SpiCePin);
	_delay_us(2);
	SpiSend(0x06);					//Enable write command
	SpiCePort|=(1<<SpiCePin);
	_delay_us(2);
}
							//Erase block in flash memory.
void		RomEraseBlock(unsigned long int EraseBlockAddr)
{
	RomEnWrite();
	while(RomGetStatus()&0x01);			//Waits another write to complete
	SpiCePort&=~(1<<SpiCePin);
	_delay_us(2);
	SpiSend(0x20);					//Erase block command
	SpiSend(EraseBlockAddr>>16);
	SpiSend(EraseBlockAddr>>8);
	SpiSend(EraseBlockAddr);
	SpiCePort|=(1<<SpiCePin);
	_delay_us(2);
	while(RomGetStatus()&0x01);			//Waits for erase to complete
}

void		RomEraseImage(unsigned long int EraseBlockAddr)
{
	for(unsigned char EraseLoop=0;EraseLoop<64;EraseLoop++)
	{
        	RomEraseBlock(EraseBlockAddr);
		EraseBlockAddr+=0x1000;
	}
}

void RomSectorRead(unsigned long int SectorReadAddr,unsigned char SectorReadBuffer)
{
	//Read sector into buffer
	SpiCePort&=~(1<<SpiCePin);
	_delay_us(2);
	SpiSend(0x03);
	SpiSend(SectorReadAddr>>16);
	SpiSend(SectorReadAddr>>8);
	SpiSend(SectorReadAddr);
							//Reads one sector (128 bytes) info buffer
	for(unsigned char RomReadLoop=0;RomReadLoop<128;RomReadLoop++)
	{
		if(SectorReadBuffer==0)
			CopyArray[RomReadLoop]=SpiSend(0);	//Read from flash byte by byte
		else
			DxArray[RomReadLoop]=SpiSend(0);	//Read from flash byte by byte

	}
	SpiCePort|=(1<<SpiCePin);
	_delay_us(2);
}

void	RomSectorWrite(unsigned long int SectorWriteAddr,unsigned char SectorWriteBuffer)
{
	RomEnWrite();

	SpiCePort&=~(1<<SpiCePin);
	_delay_us(1);
	SpiSend(0xAD);				//AAI write command
	SpiSend(SectorWriteAddr>>16);
	SpiSend(SectorWriteAddr>>8);
	SpiSend(SectorWriteAddr);
	if(SectorWriteBuffer==0)
	{
		SpiSend(CopyArray[0]);
		SpiSend(CopyArray[1]);
	}
	else
	{
		SpiSend(DxArray[0]);
		SpiSend(DxArray[1]);
	}
	SpiCePort|=(1<<SpiCePin);
	_delay_us(1);
	while(RomGetStatus()&0x01);

	for(unsigned char RomProgLoop=1;RomProgLoop<64;RomProgLoop++)
	{
		SpiCePort&=~(1<<SpiCePin);
		_delay_us(1);
		SpiSend(0xAD);
		if(SectorWriteBuffer==0)
		{
			SpiSend(CopyArray[RomProgLoop*2]);
			SpiSend(CopyArray[(RomProgLoop*2)+1]);
		}
		else
		{
			SpiSend(DxArray[RomProgLoop*2]);
			SpiSend(DxArray[(RomProgLoop*2)+1]);
		}
		SpiCePort|=(1<<SpiCePin);
		_delay_us(1);
		while(RomGetStatus()&0x01);
	}
	SpiCePort&=~(1<<SpiCePin);
	_delay_us(1);
	SpiSend(0x04);				//Disable write command
	SpiCePort|=(1<<SpiCePin);
	_delay_us(1);

}

		//Function that copies floppy image between backup, working or temporary locations.
void		RomCopyImage(unsigned long int ImageSourceAddr,unsigned long int ImageDestAddr)
{
	RomEraseImage(ImageDestAddr);
	for(unsigned int CopyLoop=0;CopyLoop<2048;CopyLoop++)
	{
		RomSectorRead(ImageSourceAddr,0);
		RomSectorWrite(ImageDestAddr,0);
		ImageSourceAddr+=0x80;
		ImageDestAddr+=0x80;
	}
}


unsigned char 	RomRead(unsigned char DiskNum,unsigned char TrkNum,unsigned char SecNum)	//Reads 128-byte sector from flash memory
{
	if(DiskNum==0)
		ActLedPort|=(1<<ActLed0Pin);
	else
		ActLedPort|=(1<<ActLed1Pin);
	//Calculates start address of desired sector
        unsigned long int SecAddr=((TrkNum*26));
        SecAddr+=SecNum;
        SecAddr*=128;
	SecAddr+=(DiskNum*0x80000);

	RomSectorRead(SecAddr,1);

	ActLedPort&=~((1<<ActLed0Pin)|(1<<ActLed1Pin));
	DxSectorsRead[DiskNum]++;
	return 0;
}


void HexSend(unsigned char HexChar)			//Function for transmitting debug data in human readable form
{
	unsigned char HexTemp=(HexChar>>4);
	if(HexTemp>9)
		UartTxAddByte(HexTemp+55);
	else
		UartTxAddByte(HexTemp+0x30);

	HexTemp=(HexChar&0x0F);
	if(HexTemp>9)
		UartTxAddByte(HexTemp+55);
	else
		UartTxAddByte(HexTemp+0x30);
}

void	DecSend(unsigned int DecData,unsigned char DecPadding)
{
	unsigned char DecLeadingZero=0;
	unsigned int DecDivider=10000;
	for(unsigned char DecLoop=0;DecLoop<4;DecLoop++)
	{
	unsigned char DecSendTemp=0;
		while(DecData>=DecDivider)
		{
			DecSendTemp++;
			DecData-=DecDivider;
			DecLeadingZero=1;
		}
		DecDivider=(DecDivider/10);
		if(DecLeadingZero>0)
			UartTxAddByte(DecSendTemp+0x30);
		else
			if(DecPadding>0)
				UartTxAddByte(' ');
	}
	UartTxAddByte(DecData+0x30);
}


	//Function for sector write to ROM.
	//Writes 128-byte sector to flash memory
	//First of all, it determines which data block contains
	//sector that has to be written. After that, it copies that
	//4KB data block to unused temporary block outside of disk data
	//boundaries. Then, function erases data block that has to be
	//changed and copies it back with one changed 128-byte sector.
	//To prevent temporary data block fast wear-out, there are 64
	//temporary blocks that are used one after another what gives
	//us about 6400000 write cycles.

unsigned char 	RomWrite(unsigned char DiskNum, unsigned char TrkNum,unsigned char SecNum)
{
	if(DiskNum==0)
		ActLedPort|=(1<<ActLed0Pin);			//Switches on activity LED
	else
		ActLedPort|=(1<<ActLed1Pin);

	unsigned long int SecAddr=((TrkNum*26));	//Calculate start address of desired sector
	SecAddr+=SecNum;
	SecAddr*=128;
	SecAddr+=(DiskNum*0x80000);
							//Calculates start address of block that contains desired sector
	unsigned long int BlockAddr=(SecAddr&0xFFFFF000);

	//Erase TempBlock
							//Calculates start address of temporary block
	unsigned long int TempBlockAddr=TempBlockNum;
	TempBlockAddr*=0x1000;
	TempBlockAddr+=0x40000;

	RomEnWrite();
	RomEraseBlock(TempBlockAddr);			//Erases temporary block to copy data to 

	//Copy block that contains sector to be written to TempBlock
	unsigned long int CopyAddr=0;
							//4KB block contains 32 sectors, so it copies 
							//it 128 bytes at a time in 32 cycles
							//128*32=4096 (4KB)
	for(unsigned int CopyLoop=0;CopyLoop<32;CopyLoop++)
	{
		CopyAddr=BlockAddr+(128*CopyLoop);

		RomSectorRead(CopyAddr,0);		//Read 128 bytes into buffer

		CopyAddr=TempBlockAddr+(128*CopyLoop);
							//Writes one sector to temporary block
		RomEnWrite();

		SpiCePort&=~(1<<SpiCePin);
		_delay_us(1);
		SpiSend(0xAD);				//AAI write command
		SpiSend(CopyAddr>>16);
		SpiSend(CopyAddr>>8);
		SpiSend(CopyAddr);
		SpiSend(CopyArray[0]);
		SpiSend(CopyArray[1]);
		SpiCePort|=(1<<SpiCePin);
		_delay_us(1);
		while(RomGetStatus()&0x01);

		//CopyAddr+=2;

		for(unsigned char RomProgLoop=1;RomProgLoop<64;RomProgLoop++)
		{
			SpiCePort&=~(1<<SpiCePin);
			_delay_us(1);
			SpiSend(0xAD);
			SpiSend(CopyArray[RomProgLoop*2]);
			SpiSend(CopyArray[(RomProgLoop*2)+1]);
			SpiCePort|=(1<<SpiCePin);
			_delay_us(1);
			while(RomGetStatus()&0x01);
		//	CopyAddr+=2;
		}
		SpiCePort&=~(1<<SpiCePin);
		_delay_us(1);
		SpiSend(0x04);				//Disable write command
		SpiCePort|=(1<<SpiCePin);
		_delay_us(1);
	}

	//Erase block that contains sector to be written
	RomEnWrite();
	RomEraseBlock(BlockAddr);

	//Copy back TempBlock to current block with changed sector

	for(unsigned int CopyLoop=0;CopyLoop<32;CopyLoop++)
	{
		CopyAddr=TempBlockAddr+(128*CopyLoop);
		//Read sector into buffer
		SpiCePort&=~(1<<SpiCePin);
		_delay_us(2);
		SpiSend(0x03);
		SpiSend(CopyAddr>>16);
		SpiSend(CopyAddr>>8);
		SpiSend(CopyAddr);

		for(unsigned char RomReadLoop=0;RomReadLoop<128;RomReadLoop++)
		{
			CopyArray[RomReadLoop]=SpiSend(0);	//Read from flash byte by byte
		}
		SpiCePort|=(1<<SpiCePin);
		_delay_us(2);

		CopyAddr=BlockAddr+(128*CopyLoop);

		RomEnWrite();

		SpiCePort&=~(1<<SpiCePin);
		_delay_us(1);
		SpiSend(0xAD);					//AAI write command
		SpiSend(CopyAddr>>16);
		SpiSend(CopyAddr>>8);
		SpiSend(CopyAddr);
		if(CopyAddr==SecAddr)
		{
			SpiSend(DxArray[0]);
			SpiSend(DxArray[1]);
		}
		else
		{
			SpiSend(CopyArray[0]);
			SpiSend(CopyArray[1]);
		}

		SpiCePort|=(1<<SpiCePin);
		_delay_us(1);
		while(RomGetStatus()&0x01);


		for(unsigned char RomProgLoop=1;RomProgLoop<64;RomProgLoop++)
		{
			SpiCePort&=~(1<<SpiCePin);
			_delay_us(1);
			SpiSend(0xAD);
			if(CopyAddr==SecAddr)
			{
				SpiSend(DxArray[RomProgLoop*2]);
				SpiSend(DxArray[(RomProgLoop*2)+1]);
			}
			else
			{
				SpiSend(CopyArray[RomProgLoop*2]);
				SpiSend(CopyArray[(RomProgLoop*2)+1]);
			}
			SpiCePort|=(1<<SpiCePin);
			while(RomGetStatus()&0x01);
			}
		SpiCePort&=~(1<<SpiCePin);
		_delay_us(1);
		SpiSend(0x04);
		SpiCePort|=(1<<SpiCePin);
		_delay_us(1);

	}

	//Increment TempBlock to prevent wear-out
	TempBlockNum++;
	if(TempBlockNum==64)
		TempBlockNum=0;

	DxSectorsWritten[DiskNum]++;

	ActLedPort&=~((1<<ActLed0Pin)|(1<<ActLed1Pin));	//Turn off activity LED
	return 0;
}

void	ExitState(void)			//Function for resetting DX interface to idle state
{
	DxDonePort&=~(1<<DxDonePin);	//Ready to receive command
	DxOutPort|=(1<<DxOutPin);	//Data - high
	DxCommand=NoCommand;		//Reset state machine
	DxState=IdleState;
	DxArrayPointer=0;
	if(SystemStatus&(1<<DebugOn))
	{
		UartTxAddByte(0x0D);
		UartTxAddByte(0x0A);
	}
}

//Timer INT, executes 10x in one second
ISR(TIMER1_COMPA_vect)
{
	DxTimeout++;
	XmodemTimeout++;
	SystemUptimeDiv++;
	if(SystemUptimeDiv==10)
	{
		SystemUptimeDiv=0;
		SystemUptime++;
	}
}

