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

//Variables
unsigned char	DxDriveSelected=0;	//Variable that holds selected drive number
		
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

//Functions
unsigned char	ShiftInP(void);				//Function for shifting in command and address with parity bit
							//from the controller. 0xFF - parity error
unsigned char	ShiftIn(void);				//Function for shifting in data from controller with no parity

void		ShiftOut(unsigned char ShiftByte);	//Function for shifting out data to controller

unsigned char 	RomRead(unsigned char TrkNum,unsigned char SecNum);	//Writes 128-byte sector ro flash memory
unsigned char 	RomWrite(unsigned char TrkNum,unsigned char SecNum);	//Reads 128-byte sector from flash memory

void		ExitState(void);			//Function that exits current state because of reset signal or error

void		UartSend(unsigned char UartData);		//Function for debug data sending to PC


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
//	UBRR0L=15;		//115200 8N1
	UBRR0L=191;		//9600 8N1

	//SPI-mem init

	_delay_ms(1000);		//Startup delay
	DxDonePort&=~(1<<DxDonePin);	//Ready to receive command

	//========== MAIN LOOP ==========

	while(1)
	{
		if ((DxRunIn&(1<<DxRunPin))==0)
		{
			if(DxCommand==NoCommand)
			{
				UartSend('C');				//Debug
				//Controller has initiated command transfer. Read command over SPI-DX.
				DxDonePort|=(1<<DxDonePin);	//Reply with command sequence started
				DxErrPort|=(1<<DxErrPin);	//Clears error signal

				//Read command - shift in from controller
				unsigned char CommandTemp=ShiftInP();
				//TO DO - parity test goes here
				DxDriveSelected=(CommandTemp>>4);	//Saves selected drive
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
					//	UartSend('T');
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
					//	UartSend('T');
						ShiftOut(DxArray[0]);
						DxArrayPointer=1;
						DxIrPort&=~(1<<DxIrPin);
					}
					else
					{
						DxIrPort|=(1<<DxIrPin);
					//	UartSend('T');
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
						SecAddr=SectorTemp;
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
							UartSend(SecAddr);
							UartSend(TrackAddr);

							for(DxArrayPointer=0;DxArrayPointer<128;DxArrayPointer++)
								UartSend(DxArray[DxArrayPointer]);
							_delay_ms(500);

							break;
							}

							case 3:
							{
							UartSend('R');
							UartSend(SecAddr);
							UartSend(TrackAddr);

							for(DxArrayPointer=0;DxArrayPointer<128;DxArrayPointer++)
							{
								while((UCSR0A&0b10000000)==0);
								DxArray[DxArrayPointer]=UDR0;
							}
							_delay_ms(500);

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
	for(unsigned char ShiftCount=0;ShiftCount<7;ShiftCount++)
	{
		ShiftData=(ShiftData<<1);
		if((DxDiIn&(1<<DxDiPin))==0)
			ShiftData+=1;
		DxShftPort&=~(1<<DxShftPin);
		_delay_us(1);
		DxShftPort|=(1<<DxShftPin);
		_delay_us(2);
	}
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


unsigned char RomRead(unsigned char TrkNum,unsigned char SecNum)
{
	unsigned int BlockBase=((TrkNum*32*128)+(SecNum*128));
	BlockBase++;
	for(unsigned int i=0;i<128;i++)
	{
		
	}
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
