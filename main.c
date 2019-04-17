/*	Program for emulating PDP-11 DX and DY
*	floppy disk drives
*
*	CPU:
*	F_CLK:
*
*	Fuses:
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
#define	DxResetPort	PORTD
#define	DxResetIn	PIND
#define	DxResetDdr	DDRD
#define	DxResetPin	3

//Pusk (IN)
#define	DxGoPort	PORTD
#define	DxGoIn		PIND
#define	DxGoDdr		DDRD
#define	DxGoPin		2

//Dannie (I)
#define	DxDataInPort	PORTC
#define	DxDataInIn	PINC
#define	DxDataInDdr	DDRC
#define	DxDataInPin	1

//Dannie (O)
#define	DxDataOutPort	PORTC
#define	DxDataOutIn	PINC
#define	DxDataOutDdr	DDRC
#define	DxDataOutPin	0

//Treb. peredachi (OUT)
#define	DxRtsPort	PORTC
#define	DxRtsIn		PINC
#define	DxRtsDdr	DDRC
#define	DxRtsPin	5

//Zaversheno (OUT)
#define	DxFinishedPort	PORTC
#define	DxFinishedIn	PINC
#define	DxFinishedDdr	DDRC
#define	DxFinishedPin	2

//Vivod (OUT)
#define	DxOutputPort	PORTC
#define	DxOutputIn	PINC
#define	DxOutputDdr	DDRC
#define	DxOutputPin	3

//Oshibka (OUT)
#define	DxErrorPort	PORTC
#define	DxErrorIn	PINC
#define	DxErrorDdr	DDRC
#define	DxErrorPin	4

//Sdvig (OUT)
#define	DxShiftPort	PORTD
#define	DxShiftIn	PIND
#define	DxShiftDdr	DDRD
#define	DxShiftPin	4

//Variables
unsigned char	DxStatus=0;
#define	DxDriveSelected	0
		
unsigned char	DxArrayPointer=0;
unsigned char 	DxArray[128];

//Functions
unsigned char RomRead(unsigned char TrkNum,unsigned char SecNum);
unsigned char RomWrite(unsigned char TrkNum,unsigned char SecNum);



// ProgramStart
int main()
{
	//INIT
	//port init
	//DX NACH.UST.  - input, pulled high
	DxResetPort|=(1<<DxResetPin);
	DxResetDdr&=~(1<<DxResetPin);

	//DX PUSK  - input, pulled high
	DxGoPort|=(1<<DxGoPin);
	DxGoDdr&=~(1<<DxGoPin);

	//DX data in  - input, pulled high
	DxDataInPort|=(1<<DxDataInPin);
	DxDataInDdr&=~(1<<DxDataInPin);

	//DX data out pin - output, high
	DxDataOutPort|=(1<<DxDataOutPin);
	DxDataOutDdr|=(1<<DxDataOutPin);

	//DX TREB.PER.  - output, high
	DxRtsPort|=(1<<DxRtsPin);
	DxRtsDdr|=(1<<DxRtsPin);

	//DX ZAVERSHENO  - output, high
	DxFinishedPort|=(1<<DxFinishedPin);
	DxFinishedDdr|=(1<<DxFinishedPin);

	//DX VYVOD  - output, high
	DxOutputPort|=(1<<DxOutputPin);
	DxOutputDdr|=(1<<DxOutputPin);

	//DX OSHIBKA  - output, high
	DxErrorPort|=(1<<DxErrorPin);
	DxErrorDdr|=(1<<DxErrorPin);

	//DX TREB.PER.  - output, high
	DxShiftPort|=(1<<DxShiftPin);
	DxShiftDdr|=(1<<DxShiftPin);


	//UART init
	//SPI-mem init
	//SPI-DX init	

	//========== MAIN LOOP ==========
	while(1)
	{
		DxFinishedPort&=~(1<<DxFinishedPin);

		if ((DxGoIn&(1<<DxGoPin))==0)
		{
			//Controller has initiated command transfer. Read command over SPI-DX.
			DxFinishedPort|=(1<<DxFinishedPin);

			//Read 8 bits
			for (unsigned char i=0;i<8;i++)
			{
				_delay_us(10);
				DxShiftPort&=~(1<<DxShiftPin);
				_delay_us(10);
				DxShiftPort|=(1<<DxShiftPin);
			}
			unsigned char DxCommand=0;
			
			switch (DxCommand)
			{
				case 0:	//Fill buffer command
				{
					
				}
				
				case 1:	//Read buffer command
				{
					
				}
				
				case 2:	//Write sector to disk command
				{
					
				}
				
				case 3:	//Write marked sector to disk command
				{
					
				}
				
				case 4:	//Read sector from disk command
				{
					
				}
				
				case 5:	//Read error and status register command
				{
					
				}
				
				case 6:	//Read disk error register command
				{
					
				}
				
				
				
			}
		}
	}
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
