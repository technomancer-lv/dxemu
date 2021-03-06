//TODO - change all \x0D\x0A to \r\n
//TODO - integer without a caast warning
unsigned char const CatText[] PROGMEM="                _\x0D\x0A                  \\ \\\x0D\x0A                   \\ \\\x0D\x0A                    | |\x0D\x0A     /\\---/\\ _.---._| |\x0D\x0A    / ^  ^  \\         \\\x0D\x0A    ( 0  0  )          ;\x0D\x0A     '=Y=_.'          ;\x0D\x0A      /     __;--._   \\\x0D\x0A     / _) ='     / /  /\x0D\x0A    / ;/ ;'     / / ;'\x0D\x0A   ( /( ;'    (_(__/\x0D\x0A\x0D\x0A>";
unsigned char const HelpText[] PROGMEM="\r\nCommands:\r\n? - help\r\nxs0 - send RX0 image over Xmodem\r\nxs1 - send RX1 image over Xmodem\r\nxr0 - receive RX0 image over Xmodem\r\nxr1 - receive RX1 image over Xmodem\r\nb0 - backup RX0 image\r\nb1 - backup RX1 image\r\nr0 - restore RX0 image from backup\r\nr1 - restore RX1 image from backup\r\nd0 - disable debug\r\nd1 - enable debug\r\nd2 - enable verbose debug\r\nv - show version\r\ns - statistics\r\n\r\n>";

//Command Line Interface functions

//CLI Init - nothing really to init, simply prints
//out that CLI is active and shows prompt
void    CliInit(void)
{
	UartSendString("\x0D\x0A-----\x0D\x0A");
	UartSendString("CLI started\x0D\x0A>");
}

//CLI routine - should be executed periodically.
//It reads characters received over serial line
//and executes commands
void    CliRoutine(unsigned char CliData)
{
	switch(CliData)
	{
		//If 0x0D received, it means that Enter button is presed
		//and command should be executed
		case 0x0D:
		{
			//If there's no command entered, nothing can be executed
			//so it simply prints out prompt
			if(CliBufferPointer==0)
			{
				UartSendString("\x0D\x0A");
				UartSendString(">");
			}
			else
			{
				//First entered letter determines command.
				//v - show version
				//x - xmodem file transfer
				//b - backup
				//r - restore
				//d - debug
				switch(CliBuffer[0])
				{

					//Version
					case 'v':
					case 'V':
					{
						//Reads serian number and bootloader version from flash
						//Reads firmware version from flash and prints all that is printable
						if(CliBufferPointer==1)
						{
							unsigned char	FlashData;
							UartSendString("\x0D\x0A");
							UartSendString("SN: ");
							for(unsigned char FlashLoop=0;FlashLoop<64;FlashLoop++)
							{
								FlashData=pgm_read_byte(0x7F40+FlashLoop);
								if(FlashData<0x7F)
									UartTxAddByte(FlashData);
							}
							UartSendString("\x0D\x0A");
							UartSendString("BL: ");
							for(unsigned char FlashLoop=0;FlashLoop<64;FlashLoop++)
							{
								FlashData=pgm_read_byte(0x7F00+FlashLoop);
								if(FlashData<0x7F)
									UartTxAddByte(FlashData);
							}
							UartSendString("\x0D\x0A");
							UartSendString("FW: ");
							for(unsigned char FlashLoop=0;FlashLoop<64;FlashLoop++)
							{
								FlashData=pgm_read_byte(0x7700+FlashLoop);
								if(FlashData<0x7F)
									UartTxAddByte(FlashData);
							}
							UartSendString("\x0D\x0A\x0D\x0A");
							UartSendString(">");
						}
						else
							UnknownCommand();
						break;
					}

					//Xmodem
					case 'x':
					case 'X':
					{
						unsigned char XmodemTransferStatus=0;
						#define		TransferInProcess	0
						#define		TransferSectorOk	1
						#define		TransferCompleted	2
						#define		TransferTimeout		3
						#define		TransferOutOfSync	4
						#define		TransferCrcFailed	5
						#define		TransferNoSoh		6
						#define		TransferTooLong		7
						#define		TransferTooShort	8
						#define		TransferExit		9
						#define		TransferCancelled	10
						#define		TransferTooManyRetries	11

						//Tests if command length with parameters is correct
						if(CliBufferPointer==3)
						{
							//Tests second symbol that determines Xmodem function
							//s - sends floppy image to computer
							//r - receives floppy image from computer
							switch(CliBuffer[1])
							{

								//Sending
								case 's':
								case 'S':
								{
									if((CliBuffer[2]<'0')|(CliBuffer[2]>'1'))
									{
										IncorrectDrive();
										break;
									}
									UartSendString("\x0D\x0A");
									UartSendString("Sending RX");
									UartTxAddByte(CliBuffer[2]);
									UartSendString(" image over Xmodem...");
									//Drive number - 3. character of the command.
									//Use only LSB, so there's only two possible drive numbers
									unsigned char	XDriveNumber=(CliBuffer[2]&1);
									unsigned char	XErrorCount=0;
									#define		XErrorMax	10
									#define		XTimeoutMax	300

									if(XDriveNumber==0)
										ActLedPort|=(1<<ActLed0Pin);
									else
										ActLedPort|=(1<<ActLed1Pin);

									unsigned int PacketNumber=1;
									unsigned char PacketCrc=0;
									unsigned long int SectorAddr=XDriveNumber;
									SectorAddr=(SectorAddr*0x80000);
									XmodemTransferStatus=TransferInProcess;
									XmodemTimeout=0;
									//Xmodem send loop
									while(1)
									{
										while(1)
										{
											unsigned char BuffTest=UartIsBufferEmpty();
											_delay_us(1);
											if(BuffTest==0)
											{
												unsigned char NakTest=UartRxGetByte();
												if(NakTest==0x15)
												{
													XErrorCount++;
													if(XErrorCount>=XErrorMax)
													{
														XmodemTransferStatus=TransferTooManyRetries;
													}
													break;
												}
												if(NakTest==0x03)
												{
													XmodemTransferStatus=TransferCancelled;
													break;
												}
												if(NakTest==0x06)
												{
													SectorAddr+=0x80;
													XErrorCount=0;
													XmodemTimeout=0;
													PacketNumber++;
													// 77 tracks, 26 sectors per track makes 2002 overall sectors
													if(PacketNumber==2003)
														XmodemTransferStatus=TransferCompleted;
													break;
												}
											}
											if(XmodemTimeout>=XTimeoutMax)
											{
												XmodemTransferStatus=TransferTimeout;
												break;
											}

										}

										if(XmodemTransferStatus>TransferInProcess)
											break;


										PacketCrc=0;			//re-initialises checksum variable
										RomSectorRead(SectorAddr,0);	//reads desired sector from ROM to buffer

										UartTxAddByte(0x01);		//Sends SOH - packet start
										UartTxAddByte(PacketNumber&0xFF);	//Sends packet number
										UartTxAddByte(0xFF-(PacketNumber&0xFF));
										//Sends 128 bytes - one sector stored in buffer
										for (unsigned char XByteLoop=0;XByteLoop<128;XByteLoop++)
										{
											UartTxAddByte(CopyArray[XByteLoop]);
											PacketCrc+=CopyArray[XByteLoop];	//Adds every byte to checksum
										}
										UartTxAddByte(PacketCrc);	//Sends checksum

									//TODO - make ASCII control code include?

									}

									//Finishing up transfer
									if(XmodemTransferStatus==TransferCompleted)
									{
										UartTxAddByte(0x04);
										_delay_ms(2000);
									}
									ActLedPort&=~((1<<ActLed0Pin)|(1<<ActLed1Pin));
									break;
								}

								//Image receive over Xmodem
								case 'r':
								case 'R':
								{
									if((CliBuffer[2]<'0')|(CliBuffer[2]>'1'))
									{
										IncorrectDrive();
										break;
									}

									UartSendString("\x0D\x0A");
									UartSendString("Receiving RX");
									UartTxAddByte(CliBuffer[2]);
									UartSendString(" image over Xmodem...");
									unsigned char XDriveNumber=(CliBuffer[2]&1);
									if(XDriveNumber==0)
										ActLedPort|=(1<<ActLed0Pin);
									else
										ActLedPort|=(1<<ActLed1Pin);

									RomEraseImage(0xC0000);

									XmodemTimeout=0;
									UartTxAddByte(0x15);

									unsigned int PreferredPacketNum=1;
									unsigned long int SectorAddr=0xC0000;
									unsigned char PreferredCrc=0;
									unsigned char ReceivedBytes=0;

									while(1)
									{
										unsigned char BuffTest=UartIsBufferEmpty();
										_delay_us(1);
										//TODO - get rid of these useless delays

										//Sends NAK every 3 seconds if no data received
										if(XmodemTimeout>=30)
										{
											XmodemTimeout=0;
											UartTxAddByte(0x15);
										}

										//If byte received:
										if(BuffTest==0)
										{
											XmodemTimeout=0;
											unsigned char XRecByte=UartRxGetByte();
											//Process each byte depending on its number in packet
											switch(ReceivedBytes)
											{
												//Byte 0 - should be 0x01, but can be 0x03 (cancel) or 0x04 (EOT)
												case 0:
												{
													switch(XRecByte)
													{
														case 0x01:
														{
															ReceivedBytes++;
															XmodemTransferStatus=TransferInProcess;
															break;
														}

														case 0x03:
														{
															XmodemTransferStatus=TransferCancelled;
															break;
														}

														case 0x04:
														{
															if(PreferredPacketNum<2002)
																XmodemTransferStatus=TransferTooShort;
															else
																XmodemTransferStatus=TransferCompleted;
															break;
														}
													}

													break;
												}
												//Byte1 - packet number
												case 1:
												{
													if(XRecByte==(PreferredPacketNum&0xFF))
														ReceivedBytes++;
													else
														XmodemTransferStatus=TransferOutOfSync;
													break;
												}
												//Byte2 - 0xFF-packet number
												case 2:
												{
													if(XRecByte==(255-(PreferredPacketNum&0xFF)))
														ReceivedBytes++;
													else
														XmodemTransferStatus=TransferOutOfSync;
													break;
												}
												//Bytes 3-131 - data bytes, stored in buffer
												default:
												{
													CopyArray[ReceivedBytes-3]=XRecByte;
													PreferredCrc+=XRecByte;
													ReceivedBytes++;
													break;
												}

												//Byte 131 - CRC
												case 131:
												{
													if(PreferredCrc==XRecByte)
													{
														if(PreferredPacketNum>=2003)
														{
															UartTxAddByte(0x15);
															XmodemTransferStatus=TransferTooLong;
														}
														else
														{
															RomSectorWrite(SectorAddr,0);
															PreferredPacketNum++;
															SectorAddr+=0x80;
															ReceivedBytes=0;
															XmodemTransferStatus=TransferSectorOk;
															UartTxAddByte(0x06);
														}
													}
													else
													{
														ReceivedBytes=0;
														XmodemTransferStatus=TransferCrcFailed;
													}
													PreferredCrc=0;
													break;
												}
											}
										}
										if(XmodemTransferStatus>TransferSectorOk)
											break;
									}

									//TODO - retry on error
									//TODO - program crashes when file too long
									//TODO - exit transfer gracefully

									switch(XmodemTransferStatus)
									{
										case TransferCompleted:
										{
											RomCopyImage(0xC0000,XDriveNumber*0x80000);

											UartTxAddByte(0x06);
											_delay_ms(1000);
											break;
										}

										default:
										{
											UartTxAddByte(0x06);
											_delay_ms(1000);
											break;
										}
									}
										//TODO - merge this with that case statement higher?

									ActLedPort&=~((1<<ActLed0Pin)|(1<<ActLed1Pin));
									break;
								}

								default:
								{
									UnknownCommand();
									break;
								}
							}

							//Prints status messages after transferring image
							switch(XmodemTransferStatus)
							{
								case TransferCompleted:
								{
									UartSendString("Done\x0D\x0A");
									break;
								}

								case TransferTimeout:
								{
									UartSendString("ERROR - TIMEOUT\x0D\x0A");
									break;
								}

								case TransferOutOfSync:
								{
									UartSendString("ERROR - INCORRECT PACKET NUMBER\x0D\x0A");
									break;
								}

								case TransferCrcFailed:
								{
									UartSendString("ERROR - CRC ERROR\x0D\x0A");
									break;
								}

								case TransferTooShort:
								{
									UartSendString("ERROR - FILE TOO SHORT\x0D\x0A");
									break;
								}

								case TransferNoSoh:
								{
									UartSendString("ERROR - INCORRECT PACKET HEADER\x0D\x0A");
									break;
								}

								case TransferTooLong:
								{
									UartSendString("ERROR - FILE TOO LONG\x0D\x0A");
									break;
								}

								case TransferCancelled:
								{
									UartSendString("TRANSFER CANCELLED BY USER\x0D\x0A");
									break;
								}

								default:
								{
									UartTxAddByte(' ');
									HexSend(XmodemTransferStatus);
									UartTxAddByte(' ');
									UartSendString("SOMETHING FUCKED UP\x0D\x0A");
									break;
								}
							}

							UartSendString("\x0D\x0A");
							UartSendString(">");

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
								if(BackupDriveNumber==0)
									ActLedPort|=(1<<ActLed0Pin);
								else
									ActLedPort|=(1<<ActLed1Pin);

								//Erase backup region
								unsigned long int BackupAddr=BackupDriveNumber;
								BackupAddr=BackupAddr*0x80000;
								BackupAddr+=0x100000;
								UartSendString("Copying...\x0D\x0A");

								//Copy image, sector by sector
								BackupAddr=BackupDriveNumber;
								BackupAddr=BackupAddr*0x80000;

								RomCopyImage(BackupAddr,BackupAddr+0x100000);

								ActLedPort&=~((1<<ActLed0Pin)|(1<<ActLed1Pin));
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
								if(BackupDriveNumber==0)
									ActLedPort|=(1<<ActLed0Pin);
								else
									ActLedPort|=(1<<ActLed1Pin);

								//Erase backup region

								unsigned long int RestoreAddr=BackupDriveNumber;
								RestoreAddr=RestoreAddr*0x80000;
								UartSendString("Copying...\x0D\x0A");

								//Copy image, sector by sector
								RestoreAddr=BackupDriveNumber;
								RestoreAddr=RestoreAddr*0x80000;

								RomCopyImage(RestoreAddr+0x100000,RestoreAddr);

								ActLedPort&=~((1<<ActLed0Pin)|(1<<ActLed1Pin));
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
					//These functions are used to watch commands and data
					//that are exchanged between computer and emulator
					case 'd':
					case 'D':
					{
                                                if(CliBufferPointer==2)
                                                {
				       			UartSendString("\x0D\x0A");
							switch(CliBuffer[1])
							{
								//No debug - no data output at data exchange
								case '0':
								{
									SystemStatus&=~((1<<DebugOn)|(1<<DebugVerbose));
									UartSendString("Debug off\x0D\x0A");
									break;
								}
								//Simple debug - command, track and sector numbers are
								//shown at data exchange
								case '1':
								{
									SystemStatus&=~(1<<DebugVerbose);
									SystemStatus|=(1<<DebugOn);
									UartSendString("Debug on\x0D\x0A");
									break;
								}
								//Verbose debug - command, parameters and data are
								//shown at data exchange. This might slow things down.
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
							//UartSendString("DEBUG IS NOT YET IMPLEMENTED\x0D\x0A");
							UartSendString("\x0D\x0A>");
						}
						else
							UnknownCommand();
						break;
					}

					//Statistics - show simple data about read/write packets, errors, etc
					case 's':
					{ 
						if(CliBufferPointer==1)
						{
							UartSendString("\x0D\x0A");
							UartSendString("Session statistics:\x0D\x0A");
							UartSendString("Uptime: ");
							unsigned int UptimeTemp=SystemUptime;
							unsigned int UptimeDigit=UptimeTemp/3600;
							DecSend(UptimeDigit,0);
							UartSendString("h ");
							UptimeTemp=UptimeTemp-(UptimeDigit*3600);
							UptimeDigit=UptimeTemp/60;
							DecSend(UptimeDigit,0);
							UartSendString("m ");
							UptimeTemp=UptimeTemp-(UptimeDigit*60);
							DecSend(UptimeTemp,0);
							UartSendString("s\x0D\x0A");
							UartSendString("Sectors written  read\x0D\x0A");
							UartSendString("DX0:      ");
							DecSend(DxSectorsWritten[0],1);
							UartSendString(" ");
							DecSend(DxSectorsRead[0],1);
							UartSendString("\x0D\x0A");
							UartSendString("DX1:      ");
							DecSend(DxSectorsWritten[1],1);
							UartSendString(" ");
							DecSend(DxSectorsRead[1],1);
							UartSendString("\x0D\x0A");
							UartSendString("\x0D\x0A>");
						}
						else
							UnknownCommand();
						break;
						//TODO - add error statistics
					}

					//Hidden cat command just for fun
					case 'c':
					{ 
						if(CliBufferPointer==1)
						{
							unsigned int TextPointer=&(CatText[0]);
							while(1)
							{
								unsigned char TempChar=pgm_read_byte(TextPointer);
								if(TempChar==0)
									break;
								UartTxAddByte(TempChar);
								TextPointer++;
							}
							UartSendString("\x0D\x0A>");
						}
						else
							UnknownCommand();
						break;
						//TODO - add error statistics
					}

					//If command is not recognised, prints out this help message
					default:
					{
						unsigned int TextPointer=&(HelpText[0]);
						while(1)
						{
							unsigned char TempChar=pgm_read_byte(TextPointer);
							if(TempChar==0)
								break;
							UartTxAddByte(TempChar);
							TextPointer++;
						}
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
			CliBufferPointer=0;		//Drops entered command
	        	UartSendString("^C\x0D\x0A");	//Outputs prompt
			UartSendString(">");
			break;
		}

		//Backspace
		case 0x08:
		{
			if(CliBufferPointer>0)			//If anything has been written in console
			{
		        	UartSendString("\x1B[D\x1B[K");	//Moves cursor back one position and deletes all un right of cursor
				CliBufferPointer--;
			}
			else
				UartTxAddByte(0x07);		//If no - output bell simbol
			break;
		}

		default:
		{
			if(CliBufferPointer<(CliBufferMax))
			{
				if((CliData>=0x20)&(CliData<0x7F))	//Save and output only printable symbols
									//Escape sequences are not filtered out, so
									//pressing arrows generates symbols
				{
					UartTxAddByte(CliData);		//Echoes received symbol
					CliBuffer[CliBufferPointer]=CliData;
					CliBufferPointer++;
				}
			}
			else
			{
				UartTxAddByte(0x07);		//Output bell on unsupported unprintables
			}
		break;
		}
	}
}

//Function that prints message on unsupported commands
void	UnknownCommand(void)
{
	UartSendString("\x0D\x0A");
	UartSendString("UNKNOWN COMMAND\x0D\x0A\x0D\x0A");
	UartSendString(">");
}

//Function that prints out message on drive number >1
void	IncorrectDrive(void)
{
	UartSendString("\x0D\x0A");
	UartSendString("INCORRECT DRIVE NUMBER\x0D\x0A\x0D\x0A");
	UartSendString(">");
}

