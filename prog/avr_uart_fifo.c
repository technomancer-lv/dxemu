	/*Atmel Atmega8 UART FIFO izpildoshais fails
	Programmas:
	RxGetByte - registraa RxData ievieto peedeejo pienjemto baitu
	TxAddByte - registraa Txdata pievieno baitu un uzsaak paarraidi
	RxINT - pilda uztverosaa FIFO funkcijas
	TxINT - pilda raidoshaa FIFO funkcijas*/

void    UartInit(unsigned long int UartBaudrate)
{
	//TODO: Baudrate and INT init here

	UartSend(0x0D);
	UartSend(0x0A);
	UartSend(0x0D);
	UartSend(0x0A);
	UartSend('-');
	UartSend('-');
	UartSend('-');
	UartSend('-');
	UartSend('-');
	UartSend(0x0D);
	UartSend(0x0A);
	UartSend('U');
	UartSend('A');
	UartSend('R');
	UartSend('T');
	UartSend(' ');
	UartSend('O');
	UartSend('K');
	UartSend(' ');
	UartSend('1');
	UartSend('1');
	UartSend('5');
	UartSend('2');
	UartSend('0');
	UartSend('0');

	//TODO: FIFO init and message here
	UartSendString("\x0D\x0AUART FIFO OK");


}

void    UartSend(unsigned char UartData)
{
	while((UCSR0A&0b00100000)==0);  //Waits until Tx buffer empty
	UDR0=UartData;
}

char	UartRxGetByte(void)
	{
	unsigned char temp5;
	if(bit_is_set(UartFlags,UartRxFifoEmpty))
		{
		temp5=0;
		}
	else
		temp5=RxBuff[UartRxFifoGetPoint];
		UartFlags&=~(1<<UartRxFifoFull);
		if(UartRxFifoGetPoint==(RxBuffCount-1))
			{
			UartRxFifoGetPoint=0;
			}
		else
			{
			UartRxFifoGetPoint++;
			}
		if(UartRxFifoGetPoint==UartRxFifoRecPoint)
			{
			UartFlags|=(1<<UartRxFifoEmpty);
			}
		//sei();
	return (temp5);
	}


char	UartTxAddByte(unsigned char UartData)
	{
	cli();
	if(bit_is_set(UartFlags,UartTxFifoEmpty))
		{
		if(bit_is_set(UCSR0A,UDRE0))
			{
			UDR0=UartData;
			sei();
			return(0);
			}
		}
	while(bit_is_set(UartFlags,UartTxFifoFull))
		{
		sei();
		}
		cli();
	//if(bit_is_set(UartFlags,UartTxFifoFull))
	//	{
	//	sei();
	//	return(1);
	//	}
	TxBuff[UartTxFifoAddPoint]=UartData;
	UartFlags &= ~(_BV(UartTxFifoEmpty));
	if(UartTxFifoAddPoint==(TxBuffCount-1))
		UartTxFifoAddPoint=0;
	else
		UartTxFifoAddPoint++;
	if(UartTxFifoAddPoint==UartTxFifoSendPoint)
		{
		UartFlags |= _BV(UartTxFifoFull);
		}
	sei();
	return(0);
	}

void	UartSendString(char UartString[])
{
	unsigned int StringPointer=0;
	while(UartString[StringPointer]!=0)
	{
		UartTxAddByte(UartString[StringPointer]);
		StringPointer++;
	}
}

unsigned char   UartIsBufferEmpty(void)
{
	return (UartFlags&(1<<UartRxFifoEmpty));
}

ISR(USART_RX_vect)
	{
	int	temp;
	temp=UDR0;
	if(bit_is_set(UartFlags,UartRxFifoFull))
		{
		UartFlags|=(1<<UartRxDataLoss);
		reti();
		}
	RxBuff[UartRxFifoRecPoint]=temp;
	UartFlags&=~(1<<UartRxFifoEmpty);
	if(UartRxFifoRecPoint==(RxBuffCount-1))
		{
		UartRxFifoRecPoint=0;
		}
	else
		{
		UartRxFifoRecPoint++;
		}
	if(UartRxFifoRecPoint==UartRxFifoGetPoint)
		{
		UartFlags|=(1<<UartRxFifoFull);
		}
	}



ISR(USART_TX_vect)
	{
	if(bit_is_clear(UartFlags,UartTxFifoEmpty))
		{
		UDR0=TxBuff[UartTxFifoSendPoint];
		UartFlags &= ~(_BV(UartTxFifoFull));
		if(UartTxFifoSendPoint==(TxBuffCount-1))
			{
			UartTxFifoSendPoint=0;
			}
		else
			{
			UartTxFifoSendPoint++;
			}
		if(UartTxFifoSendPoint==UartTxFifoAddPoint)
			{
			UartFlags |= _BV(UartTxFifoEmpty);
			}
		}
	}
