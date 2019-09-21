void    CliInit(void)
{
	UartSend(0x0A);
	UartSend(0x0D);
	UartSend('-');
	UartSend('-');
	UartSend('-');
	UartSend('-');
	UartSend('-');
	UartSend(0x0A);
	UartSend(0x0D);
	UartSend('C');
	UartSend('L');
	UartSend('I');
	UartSend(' ');
	UartSend('s');
	UartSend('t');
	UartSend('a');
	UartSend('r');
	UartSend('t');
	UartSend('e');
	UartSend('d');
	UartSend(0x0A);
	UartSend(0x0D);
	UartSend('>');
}

void    CliRoutine(unsigned char CliData)
{
	if(CliStatus==0)
	{
		switch(CliData)
		{

			//Help
			case '?':
			case 'h':
			case 'H':
			{
				UartTxAddByte(CliData);
        			UartSendString("\x0D\x0A");
        			UartSendString("Commands:\x0D\x0A");
        			UartSendString("? - help\x0D\x0A");
        			UartSendString("v - show version\x0D\x0A");
        			UartSendString("d1 - enable debug\x0D\x0A");
        			UartSendString("d0 - disable debug\x0D\x0A");
				UartSendString(">");
				break;
			}

			//Version
			case 'v':
			case 'V':
			{
				UartTxAddByte(CliData);
        			UartSendString("\x0D\x0A");
				UartSendString("version\x0D\x0A");
				UartSendString(">");
				break;
			}

			//Enter
			case 0x0D:
			{
        			UartSendString("\x0D\x0A");
				UartSendString(">");
				break;
			}

			//Ctrl-C
			case 0x03:
			{
        			UartSendString("^C\x0D\x0A");
				UartSendString(">");
				break;
			}

			default:
			{

			break;
			}
		}
	}


}
