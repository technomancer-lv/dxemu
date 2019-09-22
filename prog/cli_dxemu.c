void    CliInit(void)
{
	UartSendString("\x0D\x0A-----\x0D\x0A");
	UartSendString("CLI started\x0D\x0A>");
}

void    CliRoutine(unsigned char CliData)
{
	switch(CliData)
	{
		case 0x0D:
		{
			if(CliBufferPointer==0)
			{
				UartSendString("\x0D\x0A");
				UartSendString(">");
			}
			else
			{
				switch(CliBuffer[0])
				{

					//Version
					case 'v':
					case 'V':
					{
						if(CliBufferPointer==1)
						{
							//TO DO - store bootloader and software version somewhere it can be read from here
							//TO DO - automatically add build number, date and time
				       			UartSendString("\x0D\x0A");
							UartSendString("PCB v0.1\x0D\x0A");
							UartSendString("BLD v0.1\x0D\x0A");
							UartSendString("SW  v0.1\x0D\x0A");
							UartSendString(">");
						}
						else
						{
							UnknownCommand();
						}
						break;
					}

					//Xmodem
					case 'x':
					case 'X':
					{
						if(CliBufferPointer==3)
						{
							switch(CliBuffer[1])
							{
								case 's':
								case 'S':
								{
									UartSendString("\x0D\x0A");
									UartSendString("Sending RX");
									UartTxAddByte(CliBuffer[2]);
									UartSendString(" image over Xmodem...");




									UartSendString("\x0D\x0A");
									UartSendString(">");
									break;
								}

								case 'r':
								case 'R':
								{
									UartSendString("\x0D\x0A");
									UartSendString(">");
									break;
								}

								default:
								{
								UnknownCommand();
								break;
								}
							}
						}
						else
							UnknownCommand();
						break;
					}

					//Backup
					case 'b':
					case 'B':
					{
                                                if(CliBufferPointer==2)
                                                {
							if((CliBuffer[1]==0x30)|(CliBuffer[1]==0x31))
							{
								UartSendString("\x0D\x0A");
								UartSendString("Backing up RX");
								UartTxAddByte(CliBuffer[1]);
								UartSendString(" image...\x0D\x0A");

								unsigned char BackupDriveNumber=(CliBuffer[1]&1);

								//Erase backup region
								RomEnWrite();

								unsigned long int EraseAddr=BackupDriveNumber;
								EraseAddr=EraseAddr*0x40000;
								EraseAddr+=0x100000;

								for(unsigned char EraseLoop=0;EraseLoop<64;EraseLoop++)
								{
								/*	UartSendString("Erasing 0x");
									UartTxAddByte(((EraseAddr>>20)&0x0F)+0x30);
									UartTxAddByte(((EraseAddr>>16)&0x0F)+0x30);
									UartTxAddByte(((EraseAddr>>12)&0x0F)+0x30);
									UartTxAddByte(((EraseAddr>>8)&0x0F)+0x30);
									UartTxAddByte(((EraseAddr>>4)&0x0F)+0x30);
									UartTxAddByte(((EraseAddr)&0x0F)+0x30);
									UartSendString("\x0D\x0A");*/
									RomEraseBlock(EraseAddr);
									EraseAddr+=0x1000;
								}
								//Copy image, sector by sector

								UartSendString("Done\x0D\x0A");
								UartSendString(">");
							}
							else
								IncorrectDrive();
						}
						else
							UnknownCommand();
						break;
					}

					//Restore
					case 'r':
					case 'R':
					{
                                                if(CliBufferPointer==2)
                                                {
				       			UartSendString("\x0D\x0A");
							UartSendString("RESTORE\x0D\x0A");
							UartSendString(">");
						}
						else
							UnknownCommand();
						break;
					}

					//Debug
					case 'd':
					case 'D':
					{
                                                if(CliBufferPointer==2)
                                                {
				       			UartSendString("\x0D\x0A");
							UartSendString("DEBUG\x0D\x0A");
							UartSendString(">");
						}
						else
							UnknownCommand();
						break;
					}

					default:
					{
		        			UartSendString("\x0D\x0A");
		        			UartSendString("Commands:\x0D\x0A");
		        			UartSendString("? - help\x0D\x0A");
		        			UartSendString("xs0 - send RX0 image over Xmodem\x0D\x0A");
		        			UartSendString("xs1 - send RX1 image over Xmodem\x0D\x0A");
		        			UartSendString("xr0 - receive RX0 image over Xmodem\x0D\x0A");
		        			UartSendString("xr1 - receive RX1 image over Xmodem\x0D\x0A");
		        			UartSendString("b0 - backup RX0 image\x0D\x0A");
		        			UartSendString("b1 - backup RX1 image\x0D\x0A");
		        			UartSendString("r0 - restore RX0 image from backup\x0D\x0A");
		        			UartSendString("r1 - restore RX1 image from backup\x0D\x0A");
		        			UartSendString("d0 - disable debug\x0D\x0A");
		        			UartSendString("d1 - enable debug\x0D\x0A");
		        			UartSendString("v - show version\x0D\x0A\x0D\x0A");
						UartSendString(">");
						break;
					}
				}
				CliBufferPointer=0;
			}
		break;
		}

		//Ctrl-C
		case 0x03:
		{
	        	UartSendString("^C\x0D\x0A");
			UartSendString(">");
			break;
		}

		//Backspace
		case 0x08:
		{
			if(CliBufferPointer>0)
			{
		        	UartSendString("\x1B[D\x1B[K");
				CliBufferPointer--;
			}
			else
				UartTxAddByte(0x07);
			break;
		}



		default:
		{
			if(CliBufferPointer<(CliBufferMax))
			{
				UartTxAddByte(CliData);
				CliBuffer[CliBufferPointer]=CliData;
				CliBufferPointer++;
			}
			else
			{
				UartTxAddByte(0x07);
			}
		break;
		}
	}
}

void	UnknownCommand(void)
{
	UartSendString("\x0D\x0A");
	UartSendString("UNKNOWN COMMAND\x0D\x0A\x0D\x0A");
	UartSendString(">");
}

void	IncorrectDrive(void)
{
	UartSendString("\x0D\x0A");
	UartSendString("INCORRECT DRIVE NUMBER\x0D\x0A\x0D\x0A");
	UartSendString(">");
}
