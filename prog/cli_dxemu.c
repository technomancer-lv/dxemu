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

								unsigned long int BackupAddr=BackupDriveNumber;
								BackupAddr=BackupAddr*0x80000;
								BackupAddr+=0x100000;
								UartSendString("Erasing...\x0D\x0A");

								for(unsigned char EraseLoop=0;EraseLoop<64;EraseLoop++)
								{
									/*UartSendString("Erasing 0x");
									UartTxAddByte(((BackupAddr>>20)&0x0F)+0x30);
									UartTxAddByte(((BackupAddr>>16)&0x0F)+0x30);
									UartTxAddByte(((BackupAddr>>12)&0x0F)+0x30);
									UartTxAddByte(((BackupAddr>>8)&0x0F)+0x30);
									UartTxAddByte(((BackupAddr>>4)&0x0F)+0x30);
									UartTxAddByte(((BackupAddr)&0x0F)+0x30);
									UartSendString("\x0D\x0A");*/
									RomEraseBlock(BackupAddr);
									BackupAddr+=0x1000;
								}
								UartSendString("Previous backup erased\x0D\x0A");
								UartSendString("Copying...\x0D\x0A");

								//Copy image, sector by sector

								BackupAddr=BackupDriveNumber;
								BackupAddr=BackupAddr*0x80000;

								for(unsigned int BackupCopyLoop=0;BackupCopyLoop<2048;BackupCopyLoop++)
								{
									RomSectorRead(BackupAddr,0);
									RomSectorWrite(BackupAddr+0x100000,0);
									BackupAddr+=0x80;
								}

								UartSendString("Done\x0D\x0A\x0D\x0A");
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
							if((CliBuffer[1]==0x30)|(CliBuffer[1]==0x31))
							{
								UartSendString("\x0D\x0A");
								UartSendString("Restoring RX");
								UartTxAddByte(CliBuffer[1]);
								UartSendString(" image from backup...\x0D\x0A");

								unsigned char BackupDriveNumber=(CliBuffer[1]&1);

								//Erase backup region
								RomEnWrite();

								unsigned long int BackupAddr=BackupDriveNumber;
								BackupAddr=BackupAddr*0x80000;
								//BackupAddr+=0x100000;
								UartSendString("Erasing ...\x0D\x0A");

								for(unsigned char EraseLoop=0;EraseLoop<64;EraseLoop++)
								{
									/*UartSendString("Erasing 0x");
									UartTxAddByte(((BackupAddr>>20)&0x0F)+0x30);
									UartTxAddByte(((BackupAddr>>16)&0x0F)+0x30);
									UartTxAddByte(((BackupAddr>>12)&0x0F)+0x30);
									UartTxAddByte(((BackupAddr>>8)&0x0F)+0x30);
									UartTxAddByte(((BackupAddr>>4)&0x0F)+0x30);
									UartTxAddByte(((BackupAddr)&0x0F)+0x30);
									UartSendString("\x0D\x0A");*/
									RomEraseBlock(BackupAddr);
									BackupAddr+=0x1000;
								}
								UartSendString("Floppy image erased\x0D\x0A");
								UartSendString("Copying...\x0D\x0A");

								//Copy image, sector by sector

								BackupAddr=BackupDriveNumber;
								BackupAddr=BackupAddr*0x80000;

								for(unsigned int BackupCopyLoop=0;BackupCopyLoop<2048;BackupCopyLoop++)
								{
									RomSectorRead(BackupAddr+0x100000,0);
									RomSectorWrite(BackupAddr,0);
									BackupAddr+=0x80;
								}

								UartSendString("Done\x0D\x0A\x0D\x0A");
								UartSendString(">");
							}
							else
								IncorrectDrive();
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
							switch(CliBuffer[1])
							{
								case '0':
								{
									SystemStatus&=~((1<<DebugOn)|(1<<DebugVerbose));
									UartSendString("Debug off\x0D\x0A");
									break;
								}

								case '1':
								{
									SystemStatus&=~(1<<DebugVerbose);
									SystemStatus|=(1<<DebugOn);
									UartSendString("Debug on\x0D\x0A");
									break;
								}

								case '2':
								{
									SystemStatus|=((1<<DebugOn)|(1<<DebugVerbose));
									UartSendString("Verbose debug on\x0D\x0A");
									break;
								}

								default:
								{
									UartSendString("INCORRECT DEBUG SETTING\x0D\x0A");
									break;
								}

							}
							UartSendString("\x0D\x0A>");
						}
						else
							UnknownCommand();
						break;
					}

					//Tests
					case 's':
					{
					/*	UartSendString("\x0D\x0A");
						RomSectorRead(0x1000,0);
						for (unsigned int testvar=0;testvar<128;testvar++)
						{
							HexSend(CopyArray[testvar]);
							UartTxAddByte(' ');
							if((testvar&7)==7)
								UartSendString("\x0D\x0A");
						}
						UartSendString("\x0D\x0A");
						RomSectorRead(0x101000,0);
						for (unsigned int testvar=0;testvar<128;testvar++)
						{
							HexSend(CopyArray[testvar]);
							UartTxAddByte(' ');
							if((testvar&7)==7)
								UartSendString("\x0D\x0A");
						}
						UartSendString("\x0D\x0A>");*/
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
		        			UartSendString("d2 - enable verbose debug\x0D\x0A");
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
				if(CliData>=0x20)
				{
					UartTxAddByte(CliData);
					CliBuffer[CliBufferPointer]=CliData;
					CliBufferPointer++;
				}
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
