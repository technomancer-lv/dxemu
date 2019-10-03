
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
		unsigned char RxGetByteTemp=0;
		//If there are no data in Rx buffer, returns 0
		if(bit_is_set(UartFlags,UartRxFifoEmpty))
			return 0;
		else
		//Else - return oldest received byte
			RxGetByteTemp=RxBuff[UartRxFifoGetPoint];
		//Was it or not, marks FIFO as not full
		UartFlags&=~(1<<UartRxFifoFull);
		//Increments FIFO address to next byte
		if(UartRxFifoGetPoint==(RxBuffCount-1))
			UartRxFifoGetPoint=0;
		else
			UartRxFifoGetPoint++;
		//If fifo is empty, marks it to status byte
		if(UartRxFifoGetPoint==UartRxFifoRecPoint)
			UartFlags|=(1<<UartRxFifoEmpty);
	//Returns read byte
	return (RxGetByteTemp);
	}


char	UartTxAddByte(unsigned char UartData)
	{
	cli();		//Disables interrupts, so nothing can mess
			//byte addresses
	//If Tx buffer is empty, it can write data directly to UDR data port
	//But if there's already something is being sent out, it writes to
	//Tx buffer
	if(bit_is_set(UartFlags,UartTxFifoEmpty))
		{
		if(bit_is_set(UCSR0A,UDRE0))
			{
			UDR0=UartData;
			sei();
			return(0);
			}
		}
	//If Tx buffer is full, if enables interrupts and waits until at least one byte
	//is freed.
	while(bit_is_set(UartFlags,UartTxFifoFull))
		{
		sei();
		}
		cli();
	//Normal FIFO routine - adds one byte to FIFO
	TxBuff[UartTxFifoAddPoint]=UartData;
	//Marks FIFO as not empty
	UartFlags &= ~(_BV(UartTxFifoEmpty));
	//Icrements address
	if(UartTxFifoAddPoint==(TxBuffCount-1))
		UartTxFifoAddPoint=0;
	else
		UartTxFifoAddPoint++;
	//Aaaaand tests if it's not full
	if(UartTxFifoAddPoint==UartTxFifoSendPoint)
		{
		UartFlags |= _BV(UartTxFifoFull);
		}
	sei();
	return(0);
	}

//Simple function for sending a string
void	UartSendString(char UartString[])
{
	unsigned int StringPointer=0;
	while(UartString[StringPointer]!=0)
	{
		UartTxAddByte(UartString[StringPointer]);
		StringPointer++;
	}
}

//Simple function that checks if any data is received
//This should be checked before reading data, because
//UarRxGetByte can't return error codes
unsigned char   UartIsBufferEmpty(void)
{
	return (UartFlags&(1<<UartRxFifoEmpty));
}

//Interrupt routine when receiving a data byte over serial
ISR(USART_RX_vect)
{
	//This might seem unnecessary variable, but
	//UDR0 contents should be read even if there's
	//nowhere to put that data
	unsigned char UartRxIntTemp=UDR0;
	//If Rx buffer is full, it just discards the data and
	//marks data loss. Then returns.
	if(bit_is_set(UartFlags,UartRxFifoFull))
	{
		UartFlags|=(1<<UartRxDataLoss);
		reti();
	}
	//If buffer is not full, it saves received data
	RxBuff[UartRxFifoRecPoint]=UartRxIntTemp;
	//Marks that buffer is not empty
	UartFlags&=~(1<<UartRxFifoEmpty);
	//Increments address
	if(UartRxFifoRecPoint==(RxBuffCount-1))
		UartRxFifoRecPoint=0;
	else
		UartRxFifoRecPoint++;
	//Tests if buffer is not full and marks that
	if(UartRxFifoRecPoint==UartRxFifoGetPoint)
		UartFlags|=(1<<UartRxFifoFull);
}


//Interrupt routine when byte has been transferred over serial line
ISR(USART_TX_vect)
{
	//If Tx buffer is empty, it means that everything has been sent
	//and there's nothing to do here.
	if(bit_is_clear(UartFlags,UartTxFifoEmpty))
	{
		//Sends out next data byte
		UDR0=TxBuff[UartTxFifoSendPoint];
		//Marks buffer as not empty
		UartFlags &= ~(_BV(UartTxFifoFull));
		//Increments address
		if(UartTxFifoSendPoint==(TxBuffCount-1))
			UartTxFifoSendPoint=0;
		else
			UartTxFifoSendPoint++;
		//Tests if not empty and marks if it is
		if(UartTxFifoSendPoint==UartTxFifoAddPoint)
			UartFlags |= _BV(UartTxFifoEmpty);
	}
}
