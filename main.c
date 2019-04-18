/*	Program for emulating PDP-11 DX and DY
*	floppy disk drives
*
*	CPU:
*	F_CLK:
*
*	Fuses:
* 	
*	Pin naming is taken from KR1801VP1-033 interface pin naming.
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
#define	DxSetPin	3

//Pusk (IN)
#define	DxRunPort	PORTD
#define	DxRunIn		PIND
#define	DxRunDdr	DDRD
#define	DxRunPin	2

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
#define	DxIrDdr	DDRC
#define	DxIrPin	5

//Zaversheno (OUT)
#define	DxDonePort	PORTC
#define	DxDoneIn	PINC
#define	DxDoneDdr	DDRC
#define	DxDonePin	2

//Vivod (OUT)
#define	DxOutPort	PORTC
#define	DxOutIn	PINC
#define	DxOutDdr	DDRC
#define	DxOutPin	3

//Oshibka (OUT)
#define	DxErrPort	PORTC
#define	DxErrIn	PINC
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
unsigned char	DxCommand=0;		//DX command received from controller. Used in state machine
unsigned char	DxState=0;		//State machine state

//Functions
unsigned char 	RomRead(unsigned char TrkNum,unsigned char SecNum);
unsigned char 	RomWrite(unsigned char TrkNum,unsigned char SecNum);



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

	//DX data out pin - output, low
	DxDoPort&=~(1<<DxDoPin);
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
	DxDonePort&=~(1<<DxDonePin);	//

	//========== MAIN LOOP ==========

	while (1)
	{}

	while(1)
	{
		if ((DxRunIn&(1<<DxRunPin))==0)
		{
			//Controller has initiated command transfer. Read command over SPI-DX.
			DxDonePort|=(1<<DxDonePin);

			//Read 8 bits
			for (unsigned char i=0;i<8;i++)
			{
				_delay_us(1);
				DxShftPort&=~(1<<DxShftPin);
				_delay_us(2);
				DxShftPort|=(1<<DxShftPin);
			}
			
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
