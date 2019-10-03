//AVR UART software FIFO implementation
//This part of code buffers incoming data into RxBuff and
//lets buffer outgoing data in TxBuff.
//Receive and transmit are automated with interrupts
//and car function on background
//
//This code was written in about 2011, but there it's included
//in dxemu project that's made in 2019, so coding style may
//differ from other parts of code.

#define	TxBuffCount 140		//Transmit buffer length
#define	RxBuffCount 140		//Receive buffer length

//UartFlags bits
#define	UartRxDataLoss	3
#define	UartRxFifoEmpty	4
#define UartRxFifoFull	5
#define	UartTxFifoEmpty	6
#define	UartTxFifoFull	7

//Variables
int	UartFlags=0b01010000;	//FIFO status flag byte
int	UartTxFifoAddPoint=0;
int	UartTxFifoSendPoint=0;
int	UartRxFifoRecPoint=0;
int	UartRxFifoGetPoint=0;
unsigned char	TxBuff[TxBuffCount];	//Transmit buffer with predefined length
unsigned char	RxBuff[RxBuffCount];	//Receive buffer with predefined length

//Functions
void	UartInit(unsigned long int UartBaudrate);	//UART init function - sets baudrate and other parameters
void	UartSend(unsigned char UartData);		//Direct send function. Is only used before fifo init, to print out "UART OK" message.
char	UartRxGetByte(void);				//Function that lets get one byte from receive buffer
char	UartTxAddByte(unsigned char UartData);		//Function that adds one byte to transmit buffer
void    UartSendString(char UartString[]);		//Function htat adds string to transmit buffer
unsigned char	UartIsBufferEmpty(void);		//Function that tests if there's any data in Rx FIFO

#include	<avr/interrupt.h>
#include	<avr/io.h>
#include	"avr_uart_fifo.c"
