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

//Variables
unsigned char	DxStatus=0;
#define	DxDriveSelected	0
		
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

//Functions
unsigned char	ShiftInP(void);				//Function for shifting in command and address with parity bit
							//from the controller. 0xFF - parity error
unsigned int	ShiftIn(void);				//Function for shifting in data from controller with no parity

void		ShiftOut(unsigned char ShiftByte);	//Function for shifting out data to controller

unsigned char 	RomRead(unsigned char TrkNum,unsigned char SecNum);	//Writes 128-byte sector ro flash memory
unsigned char 	RomWrite(unsigned char TrkNum,unsigned char SecNum);	//Reads 128-byte sector from flash memory




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
	//SPI-mem init
	//SPI-DX init

	_delay_ms(1000);		//Startup delay
	DxDonePort&=~(1<<DxDonePin);	//Ready to receive command

	//========== MAIN LOOP ==========

	while(1)
	{
		if ((DxRunIn&(1<<DxRunPin))==0)
		{
			if(DxCommand==NoCommand)
			{
				//Controller has initiated command transfer. Read command over SPI-DX.
				DxDonePort|=(1<<DxDonePin);	//Reply with command sequence started

				//Read 8 bits

				unsigned int CommandTemp=ShiftInP();

				//Parity test goes here

				DxCommand=(CommandTemp&7);
			}

			switch (DxCommand)
			{
				case 0:	//Fill buffer command
				{
					
					break;
				}

				case 1:	//Read buffer command
				{
					
					break;
				}

				case 2:	//Write sector to disk command
				case 3:	//Read sector from disk command
				case 6:	//Write marked sector to disk command
				{
					if(DxState==IdleState)
					{
						DxState=SecAddrWaitState;
						break;
					}
					if(DxState==SecAddrWaitState)
					{
						unsigned int	SectorTemp=ShiftIn(8);
						//Parity test goes here
						SecAddr=
					}
					break;
				}

				case 4:	//Unused
				{
					
					break;
				}

				case 5:	//Read error and status register command
				{
					
					break;
				}

				case 7:	//Read disk error register command
				{
					
					break;
				}

				default:
				{
					DxCommand=NoCommand;
					DxState=IdleState;
					break;
				}
			}
		}
	}
}

unsigned int	ShiftInP(void)		//Function for shifting in data from controller
{
	unsigned int ShiftData=0;
	if((DxDiIn&(1<<DxDiPin))==0)	//Read data is inverted
		ShiftData=1;

	for(;ShiftCount>0;ShiftCount--)
	{
		DxShftPort&=~(1<<DxShftPin);
		_delay_us(1);
		DxShftPort|=(1<<DxShftPin);
		_delay_us(2);
		if((DxDiIn&(1<<DxDiPin))==0)
			ShiftData+=1;
	}
	return ShiftData>>1;
}


void		ShiftOut(unsigned char ShiftByte)	//Function for shifting out data to controller
{

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
