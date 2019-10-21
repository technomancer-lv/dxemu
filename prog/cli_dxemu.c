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
						//TODO - fix 0x0D 0x0A problem in makefile
						if(CliBufferPointer==1)
						{
							UartSendString("\x0D\x0A");
							for(unsigned char EeLoop=0;EeLoop<64;EeLoop++)
							{
								EEAR=EeLoop;
								EECR|=(1<<EERE);
								if(EEDR<0x7F)
									UartTxAddByte(EEDR);
							}
							UartSendString("\x0D\x0A");
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
									//TODO - check if 0 or 1 else fail
									UartSendString("\x0D\x0A");
									UartSendString("Receiving RX");
									UartTxAddByte(CliBuffer[2]);
									UartSendString(" image over Xmodem...");
									unsigned char XDriveNumber=(CliBuffer[2]&1);
									if(XDriveNumber==0)
										ActLedPort|=(1<<ActLed0Pin);
									else
										ActLedPort|=(1<<ActLed1Pin);

									unsigned long int SectorAddr=0xC0000;

									RomEraseImage(SectorAddr);

									XmodemTimeout=0;
									UartTxAddByte(0x15);

									while(1)
									{
										unsigned char BuffTest=UartIsBufferEmpty();
										_delay_ms(1);
										if(BuffTest==0)
											break;

										if(XmodemTimeout>=30)
										{
											XmodemTimeout=0;
											UartTxAddByte(0x15);
										}

									}

									unsigned int PreferredPacketNum=1;
									SectorAddr=0xC0000;

									while(1)
									{
										unsigned char PreferredCrc=0;
										unsigned char ReceivedBytes=0;

										while(1)
										{
											unsigned char BuffTest=UartIsBufferEmpty();
											_delay_us(1);
											//TODO - get rid of these useless delays
											if(BuffTest==0)
											{
												unsigned char XRecByte=UartRxGetByte();
												switch(ReceivedBytes)
												{
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

													case 1:
													{
														if(XRecByte==(PreferredPacketNum&0xFF))
															ReceivedBytes++;
														else
															XmodemTransferStatus=TransferOutOfSync;
														break;
													}

													case 2:
													{
														if(XRecByte==(255-(PreferredPacketNum&0xFF)))
															ReceivedBytes++;
														else
															XmodemTransferStatus=TransferOutOfSync;
														break;
													}

													default:
													{
														CopyArray[ReceivedBytes-3]=XRecByte;
														PreferredCrc+=XRecByte;
														ReceivedBytes++;
														break;
													}

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
										switch(XmodemTransferStatus)
										{
											case TransferCompleted:
											{
												//TODO - clear up this messed up region
												//Erase backup region

												unsigned long int RestoreOffset=XDriveNumber;
												RestoreOffset=RestoreOffset*0x80000;

												unsigned long int SectorAddr=0;

												//Copy image, sector by sector
												SectorAddr=0;

												RomCopyImage(SectorAddr+0xC0000,SectorAddr+RestoreOffset);

												//TODO - output error messages
												UartTxAddByte(0x06);
												_delay_ms(1000);
												UartSendString("Done\x0D\x0A");
												break;
											}

											case TransferTimeout:
											{
												UartTxAddByte(0x06);
												_delay_ms(1000);
												UartSendString("ERROR - TIMEOUT\x0D\x0A");
												break;
											}

											case TransferOutOfSync:
											{
												UartTxAddByte(0x06);
												_delay_ms(1000);
												UartSendString("ERROR - INCORRECT PACKET NUMBER\x0D\x0A");
												break;
											}

											case TransferCrcFailed:
											{
												UartTxAddByte(0x06);
												_delay_ms(1000);
												UartSendString("ERROR - CRC ERROR\x0D\x0A");
												break;
											}

											case TransferTooShort:
											{
												UartTxAddByte(0x06);
												_delay_ms(1000);
												UartSendString("ERROR - FILE TOO SHORT\x0D\x0A");
												break;
											}

											case TransferNoSoh:
											{
												UartTxAddByte(0x06);
												_delay_ms(1000);
												UartSendString("ERROR - INCORRECT PACKET HEADER\x0D\x0A");
												break;
											}

											case TransferTooLong:
											{
												UartTxAddByte(0x15);
												_delay_ms(1000);
												UartSendString("ERROR - FILE TOO LONG\x0D\x0A");
												break;
											}

											case TransferCancelled:
											{
												UartTxAddByte(0x06);
												_delay_ms(1000);
												UartSendString("TRANSFER CANCELLED BY USER\x0D\x0A");
												break;
											}
											//TODO - remove these error messages, real solution lower
											//TODO - program crashes when file too long
											//TODO - exit transfer gracefully

											default:
											{
												UartTxAddByte(' ');
												HexSend(XmodemTransferStatus);
												UartTxAddByte(' ');
												UartSendString("SOMETHING FUCKED UP\x0D\x0A");

												break;
											}
										}
										if(XmodemTransferStatus>TransferSectorOk)
											break;
									}


									ActLedPort&=~((1<<ActLed0Pin)|(1<<ActLed1Pin));
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
					//TODO - implement debug data output in main.c
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

					//If command is not recognised, prints out this help message
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
		        			UartSendString("v - show version\x0D\x0A");
		        			UartSendString("s - statistics\x0D\x0A\x0D\x0A");
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
