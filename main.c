/*	Program for emulating PDP-11 DX and DY
*	floppy disk drives
*
*	CPU:
*	F_CLK:
*
*	Fuses:
* 	
*	Pin naming is taken from KR1801VP1-033 interface pin naming.
*	Data is shifted MSB first
*	Command and both addresses are 9-bit long with parity bit as LSB
*
* 	PUSK - visiem procesiem kopeejs kaa saakuma pulss
* 	Peec PUSK sanjemshanas deaktivizee ZAVERSHENO un
* 	ieshifto iekshaa komandas baitu (8 biti)
* 	Atkariibaa no komandas baita veic kaadu no pieprasiitajiem procesiem.
* 
* 
* 
* 
* 
* 
* 
*/
#include <avr/io.h>
#include <util/delay.h>

//Nach. ustanovka (IN)
#define	DxSetPort	PORTD
#define	DxSetIn		PIND
#define	DxSetDdr	DDRD
#define	DxSetPin	2

//Pusk (IN)
#define	DxRunPort	PORTD
#define	DxRunIn		PIND
#define	DxRunDdr	DDRD
#define	DxRunPin	3

//Dannie (I)
#define	DxDiPort	PORTC
#define	DxDiIn		PINC
#define	DxDiDdr		DDRC
#define	DxDiPin		1

//Dannie (O)
#define	DxDoPort	PORTC
#define	DxDoIn		PINC
#define	DxDoDdr		DDRC
#define	DxDoPin		0

//Treb. peredachi (OUT)
#define	DxIrPort	PORTC
#define	DxIrIn		PINC
#define	DxIrDdr		DDRC
#define	DxIrPin		5

//Zaversheno (OUT)
#define	DxDonePort	PORTC
#define	DxDoneIn	PINC
#define	DxDoneDdr	DDRC
#define	DxDonePin	2

//Vivod (OUT)
#define	DxOutPort	PORTC
#define	DxOutIn		PINC
#define	DxOutDdr	DDRC
#define	DxOutPin	3

//Oshibka (OUT)
#define	DxErrPort	PORTC
#define	DxErrIn		PINC
#define	DxErrDdr	DDRC
#define	DxErrPin	4

//Sdvig (OUT)
#define	DxShftPort	PORTD
#define	DxShftIn	PIND
#define	DxShftDdr	DDRD
#define	DxShftPin	4

//TO DO - add 12bit input pin and functionality

//UART
//Tx
#define	UartTxPort	PORTD
#define	UartTxIn	PIND
#define	UartTxDdr	DDRD
#define	UartTxPin	1

//SPI
//CE
#define	SpiCePort	PORTB
#define	SpiCeIn		PINB
#define	SpiCeDdr	DDRB
#define	SpiCePin	2

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

//Variables
unsigned char	DxDriveSelected=0;	//Variable that holds selected drive number
					//0 - DX0
					//1 - DX1
					//2 - DY0
					//3 - DY1

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


//Functions
unsigned char	ShiftInP(void);				//Function for shifting in command and address with parity bit
							//from the controller. 0xFF - parity error
unsigned char	ShiftIn(void);				//Function for shifting in data from controller with no parity

void		ShiftOut(unsigned char ShiftByte);	//Function for shifting out data to controller

unsigned char	SpiSend(unsigned char SpiData);

/* Functions for write/read to/from flash memory
*  TrkNum - track number  (0-77 for DX drive)
*  SecNum - sector number (0-26 for DX drive)
*  DiskNum - disk number  (0,1 - DX drives, 2,3 - DY drives if DY will be implemented)
*/

void		RomSetStatus(unsigned char RomStatus);
unsigned char	RomGetStatus(void);
void		RomEnWrite(void);
void		RomEraseBlock(unsigned long int EraseBlockAddr);
unsigned char 	RomRead(unsigned char DiskNum, unsigned char TrkNum,unsigned char SecNum);	//Reads 128-byte sector from flash memory
unsigned char 	RomWrite(unsigned char DiskNum, unsigned char TrkNum,unsigned char SecNum);	//Writes 128-byte sector to flash memory

void		ExitState(void);			//Function that exits current state because of reset signal or error

void		UartSend(unsigned char UartData);		//Function for debug data sending to PC
void		HexSend(unsigned char HexChar);




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
	UartTxDdr|=(1<<UartTxPin);
	UCSR0A=0b00000010;
	UCSR0B=0b00011000;	//Tx enabled, RX enabled
	UCSR0C=0b00000110;	//
	UBRR0H=0;
	UBRR0L=15;		//115200 8N1
//	UBRR0L=191;		//9600 8N1

	//SPI-mem init
	SpiCePort|=(1<<SpiCePin);
	SpiCeDdr|=(1<<SpiCePin);
	SpiMisoPort|=(1<<SpiMisoPin);
	SpiMisoDdr&=~(1<<SpiMisoPin);
	SpiMosiDdr|=(1<<SpiMosiPin);
	SpiSckDdr|=(1<<SpiSckPin);

	SPCR=0b01010000;	//mode0, fosc/4
	SPSR=0b00000000;

	_delay_ms(1000);		//Startup delay
	DxDonePort&=~(1<<DxDonePin);	//Ready to receive command

	//========== MAIN LOOP ==========

	UartSend('S');
	UartSend(' ');
	RomSetStatus(0b00000000);
	HexSend(RomGetStatus());
	UartSend(0x0A);
	UartSend(0x0D);


/*	for (unsigned char Tloop=0;Tloop<77;Tloop++)
	{
		for (unsigned char Sloop=0;Sloop<26;Sloop++)
		{
			for (unsigned char i=0;i<128;i++)
				DxArray[i]=i;
			DxArray[0]=Tloop;
			DxArray[1]=Sloop;
			UartSend(0x0A);
			UartSend(0x0D);
			RomWrite(0,Tloop,Sloop);
		}
	}
*/


	while(1)
	{
		if ((DxRunIn&(1<<DxRunPin))==0)
		{
			if(DxCommand==NoCommand)
			{
				UartSend(' ');
				UartSend('C');				//Debug
				//Controller has initiated command transfer. Read command over SPI-DX.
				DxDonePort|=(1<<DxDonePin);	//Reply with command sequence started
				DxErrPort|=(1<<DxErrPin);	//Clears error signal

				//Read command - shift in from controller
				unsigned char CommandTemp=ShiftInP();
				//TO DO - parity test goes here
				DxDriveSelected=((CommandTemp>>4)&0x01);	//Saves selected drive
				DxCommand=((CommandTemp>>1)&7);		//Saves command
				UartSend(DxCommand+0x30);		//Debug
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
					}
					else				//Receive 128 bytes from controller
					{
						DxArray[DxArrayPointer]=ShiftIn();
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
						//TO DO - merge with else
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
						//	if(DxArrayPointer==128)
						//		ExitState();
						//	else
								DxIrPort&=~(1<<DxIrPin);
						}
					}
					else
					{
						DxIrPort|=(1<<DxIrPin);
						ExitState();
					}
					break;
				}

				case 2:	//Write sector to disk command
				case 3:	//Read sector from disk command
				case 6:	//Write marked sector to disk command
				{
					if(DxState==IdleState)
					{
						DxState=SecAddrWaitState;
						DxIrPort&=~(1<<DxIrPin);
						break;
					}
					if(DxState==SecAddrWaitState)
					{
						unsigned int	SectorTemp=ShiftInP();
						//TO DO - parity test
						SecAddr=(SectorTemp-1);
						if (SecAddr>26)
						{
							DxErrPort&=~(1<<DxErrPin);
							ExitState();
							break;
							//TO DO - add error variable
						}
						DxState=TrackAddrWaitState;
						DxIrPort&=~(1<<DxIrPin);
						break;
						//TO DO - error if parity error

					}
					if(DxState==TrackAddrWaitState)
					{
						unsigned int	TrackTemp=ShiftInP();
						//TO DO - parity test and exit
						TrackAddr=TrackTemp;
						if (TrackAddr>76)
						{
							DxErrPort&=~(1<<DxErrPin);
							ExitState();
							break;
							//TO DO - add error variable
						}
				//TO DO - write or read sector here

						switch (DxCommand)
						{
							case 2:
							case 6:
							{
								UartSend('W');
								UartSend((TrackAddr&0x0F)+0x30);
								UartSend((SecAddr&0x0F)+0x30);

								RomWrite(DxDriveSelected,TrackAddr,SecAddr);

							//	_delay_ms(50);

								break;
							}

							case 3:
							{
								RomRead(DxDriveSelected,TrackAddr,SecAddr);


								//Old storage emulation over serial
								UartSend('R');
								UartSend((TrackAddr&0x0F)+0x30);
								UartSend((SecAddr&0x0F)+0x30);

							/*

								for(DxArrayPointer=0;DxArrayPointer<128;DxArrayPointer++)
								{
									while((UCSR0A&0b10000000)==0);
									DxArray[DxArrayPointer]=UDR0;
								}
							*/
							//	_delay_ms(50);

								break;
							}

						}

						ExitState();
					}
					break;
				}

				case 4:	//Unused
				{
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
		if((DxSetIn&(1<<DxSetPin))==0)
		{
			//TO DO - fill buffer with sector 0
			DxDonePort|=(1<<DxDonePin);
			_delay_ms(1000);
			DxOutPort&=~(1<<DxOutPin);
			_delay_us(10);
			ShiftOut(0b10000100);
			ExitState();
		}
	}
}

unsigned char	ShiftInP(void)		//Function for shifting in data from controller
{
	DxIrPort|=(1<<DxIrPin);
	unsigned char ShiftData=0;
	for(unsigned char ShiftCount=0;ShiftCount<8;ShiftCount++)
	{
		ShiftData=(ShiftData<<1);
		if((DxDiIn&(1<<DxDiPin))==0)
			ShiftData+=1;
		DxShftPort&=~(1<<DxShftPin);
		_delay_us(1);
		DxShftPort|=(1<<DxShftPin);
		_delay_us(2);
	}
	unsigned char ShiftParity=0;
	if((DxDiIn&(1<<DxDiPin))==0)
		ShiftParity+=1;
	//TO DO - test parity here

	return ShiftData;
}

unsigned char	ShiftIn(void)		//Function for shifting in data from controller
{
	DxIrPort|=(1<<DxIrPin);
	unsigned char ShiftData=0;
	for(unsigned char ShiftCount=0;ShiftCount<8;ShiftCount++)
	{
		ShiftData=(ShiftData<<1);
		if((DxDiIn&(1<<DxDiPin))==0)
			ShiftData+=1;
		DxShftPort&=~(1<<DxShftPin);
		_delay_us(1);
		DxShftPort|=(1<<DxShftPin);
		_delay_us(2);
	}
//	while((SPSR&0b10000000)==0);
	return ShiftData;
}


void		ShiftOut(unsigned char ShiftByte)	//Function for shifting out data to controller
{
	for(unsigned char ShiftCount=0;ShiftCount<8;ShiftCount++)
	{
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
	DxDoPort|=(1<<DxDoPin);
}


unsigned char	SpiSend(unsigned char SpiData)
{
	SPDR=SpiData;
	while((SPSR&0b10000000)==0);
	unsigned char SpiTemp=SPDR;
	return(SpiTemp);
}


/*
* Data map:
000000-07FFFF:	DX0 disk
080000-0FFFFF:	DX1 disk
100000-17FFFF:	DY1 disk
180000-1FFFFF:	DY2 disk

DX disks populates only about half of 4MB assigned to each disk.
Flash memory can be only erased by 4KB blocks, so there's need
for temporary storage when one of 32 emulated sectors located in 
flash memory block that is erased. So second half of DX0 disk is used
as a blocks for temporary storage while changing one sector. So:
040000-07FFFF:  64 temporary blocks
*/

void		RomSetStatus(unsigned char RomStatus)
{
	SpiCePort&=~(1<<SpiCePin);
	_delay_us(2);
	SpiSend(0x50);
	SpiCePort|=(1<<SpiCePin);
	_delay_us(2);

	SpiCePort&=~(1<<SpiCePin);
	_delay_us(2);
	SpiSend(0x01);
	SpiSend(RomStatus);
	SpiCePort|=(1<<SpiCePin);
	_delay_us(2);
}

unsigned char	RomGetStatus(void)
{
	SpiCePort&=~(1<<SpiCePin);
	_delay_us(2);
	SpiSend(0x05);


	unsigned char StatTemp=SpiSend(0);	//Get status value
	SpiCePort|=(1<<SpiCePin);
	_delay_us(2);
	return StatTemp;
}

void		RomEnWrite(void)
{
	SpiCePort&=~(1<<SpiCePin);
	_delay_us(2);
	SpiSend(0x06);
	SpiCePort|=(1<<SpiCePin);
	_delay_us(2);
}

void		RomEraseBlock(unsigned long int EraseBlockAddr)
{
	while(RomGetStatus()&0x01);
	SpiCePort&=~(1<<SpiCePin);
	_delay_us(2);
	SpiSend(0x20);
	SpiSend(EraseBlockAddr>>16);
	SpiSend(EraseBlockAddr>>8);
	SpiSend(EraseBlockAddr);
	SpiCePort|=(1<<SpiCePin);
	_delay_us(2);
	while(RomGetStatus()&0x01);
}

unsigned char 	RomRead(unsigned char DiskNum,unsigned char TrkNum,unsigned char SecNum)	//Writes 128-byte sector ro flash memory
{
//	unsigned long int SecAddr=(DiskNum*0x80000)+(TrkNum*26*128)+(SecNum*128);



        unsigned long int SecAddr=((TrkNum*26));
        SecAddr+=SecNum;
        SecAddr*=128;
	SecAddr+=(DiskNum*0x80000);


	SpiCePort&=~(1<<SpiCePin);
	_delay_us(2);
	SpiSend(0x03);
	SpiSend(SecAddr>>16);
	SpiSend(SecAddr>>8);
	SpiSend(SecAddr);


	for(unsigned char RomReadLoop=0;RomReadLoop<128;RomReadLoop++)
	{
		DxArray[RomReadLoop]=SpiSend(0);	//Read from flash byte by byte
	}
	SpiCePort|=(1<<SpiCePin);
	_delay_us(2);

//	for(unsigned char i=0;i<128;i++)
//		UartSend(DxArray[i]);

	return 0;
}

void HexSend(unsigned char HexChar)
{
unsigned char HexTemp=(HexChar>>4);
if(HexTemp>9)
	UartSend(HexTemp+55);
else
	UartSend(HexTemp+0x30);

HexTemp=(HexChar&0x0F);
if(HexTemp>9)
	UartSend(HexTemp+55);
else
	UartSend(HexTemp+0x30);
}

unsigned char 	RomWrite(unsigned char DiskNum, unsigned char TrkNum,unsigned char SecNum)	//Reads 128-byte sector from flash memory
{
//	unsigned long int SecAddr=(DiskNum*0x80000)+(TrkNum*26*128)+(SecNum*128);

	UartSend('W');
	HexSend(TrkNum);
	UartSend(' ');
	HexSend(SecNum);
	UartSend(' ');

	unsigned long int SecAddr=((TrkNum*26));
	SecAddr+=SecNum;
	SecAddr*=128;
	SecAddr+=(DiskNum*0x80000);

	HexSend(SecAddr>>16);
	HexSend(SecAddr>>8);
	HexSend(SecAddr);
	UartSend(' ');


	unsigned long int BlockAddr=(SecAddr&0xFFFFF000);

	HexSend(BlockAddr>>16);
	HexSend(BlockAddr>>8);
	HexSend(BlockAddr);
	UartSend(' ');


	//Erase TempBlock
	unsigned long int TempBlockAddr=TempBlockNum;
	TempBlockAddr*=0x1000;
	TempBlockAddr+=0x40000;

	HexSend(TempBlockAddr>>16);
	HexSend(TempBlockAddr>>8);
	HexSend(TempBlockAddr);
	UartSend(' ');

	RomEnWrite();
	RomEraseBlock(TempBlockAddr);

	UartSend('E');
	UartSend('T');
	UartSend(' ');

//	for(unsigned char RomReadLoop=0;RomReadLoop<128;RomReadLoop++)
//		HexSend(DxArray[RomReadLoop]);

	//Copy block that contains sector to be written to TempBlock
	unsigned char CopyArray[128];
	unsigned long int CopyAddr=0;
	for(unsigned int CopyLoop=0;CopyLoop<32;CopyLoop++)
	{
		CopyAddr=BlockAddr+(128*CopyLoop);
		//Read sector to buffer
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

		CopyAddr=TempBlockAddr+(128*CopyLoop);

		for(unsigned char RomProgLoop=0;RomProgLoop<128;RomProgLoop++)
		{
			RomEnWrite();

			SpiCePort&=~(1<<SpiCePin);
			_delay_us(2);
			SpiSend(0x02);
			SpiSend(CopyAddr>>16);
			SpiSend(CopyAddr>>8);
			SpiSend(CopyAddr);
			SpiSend(CopyArray[RomProgLoop]);
			SpiCePort|=(1<<SpiCePin);
			_delay_us(20);
			CopyAddr++;
		}
	}

	//Erase block that contains sector to be written
	RomEnWrite();
	RomEraseBlock(BlockAddr);

	//Copy back TempBlock to current block with changed sector

	for(unsigned int CopyLoop=0;CopyLoop<32;CopyLoop++)
	{
		CopyAddr=TempBlockAddr+(128*CopyLoop);
		//Read sector to buffer
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
		if(CopyAddr==SecAddr)
		{
		UartSend('=');
			for(unsigned char RomProgLoop=0;RomProgLoop<128;RomProgLoop++)
			{
				RomEnWrite();

				SpiCePort&=~(1<<SpiCePin);
				_delay_us(2);
				SpiSend(0x02);
				SpiSend(CopyAddr>>16);
				SpiSend(CopyAddr>>8);
				SpiSend(CopyAddr);
				SpiSend(DxArray[RomProgLoop]);
				SpiCePort|=(1<<SpiCePin);
				_delay_us(20);
				CopyAddr++;
			}

		}
		else
		{
		UartSend('_');
			for(unsigned char RomProgLoop=0;RomProgLoop<128;RomProgLoop++)
			{
				RomEnWrite();

				SpiCePort&=~(1<<SpiCePin);
				_delay_us(2);
				SpiSend(0x02);
				SpiSend(CopyAddr>>16);
				SpiSend(CopyAddr>>8);
				SpiSend(CopyAddr);
				SpiSend(CopyArray[RomProgLoop]);
				SpiCePort|=(1<<SpiCePin);
				_delay_us(20);
				CopyAddr++;
			}
		}
	}


	//Increment TempBlock
	TempBlockNum++;
	if(TempBlockNum==64)
		TempBlockNum=0;

	return 0;
}

void	ExitState(void)
{
	DxDonePort&=~(1<<DxDonePin);	//Ready to receive command
	DxOutPort|=(1<<DxOutPin);
	DxCommand=NoCommand;
	DxState=IdleState;
	DxArrayPointer=0;
}

void	UartSend(unsigned char UartData)		//Function for debug data sending to PC
{
	while((UCSR0A&0b00100000)==0);
		UDR0=UartData;
}
