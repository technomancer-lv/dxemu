
#define	TxBuffCount 140
#define	RxBuffCount 140

#define	UartRxDataLoss	3
#define	UartRxFifoEmpty	4
#define UartRxFifoFull	5
#define	UartTxFifoEmpty	6
#define	UartTxFifoFull	7




int	UartFlags=0b01010000;
int	UartTxFifoAddPoint=0;
int	UartTxFifoSendPoint=0;
int	UartRxFifoRecPoint=0;
int	UartRxFifoGetPoint=0;
unsigned char	TxBuff[TxBuffCount];
unsigned char	RxBuff[RxBuffCount];

void	UartInit(unsigned long int UartBaudrate);
void	UartSend(unsigned char UartData);
char	UartRxGetByte(void);
char	UartTxAddByte(unsigned char UartData);
void    UartSendString(char UartString[]);
unsigned char	UartIsBufferEmpty(void);

#include	<avr/interrupt.h>
#include	<avr/io.h>
#include	"avr_uart_fifo.c"
