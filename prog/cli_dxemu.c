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
						if(CliBufferPointer==1)
						{
							//TO DO - store bootloader and software version somewhere it can be read from here
							//TO DO - automatically add build number, date and time
				       			UartSendString("\x0D\x0A");
							UartSendString("PCB v1.0\x0D\x0A");
							UartSendString("BLD v0.1\x0D\x0A");
							UartSendString("SW  v0.1\x0D\x0A");
							UartSendString("SN  301\x0D\x0A");
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
									UartSendString("\x0D\x0A");
									UartSendString("Sending RX");
									UartTxAddByte(CliBuffer[2]);
									UartSendString(" image over Xmodem...");
									unsigned char XDriveNumber=(CliBuffer[2]&1);

									while(1)
									{
										unsigned char BuffTest=UartIsBufferEmpty();
										_delay_ms(1);
										if(BuffTest==0)
										{
											unsigned char NakTest=UartRxGetByte();
											if(NakTest==0x15)
												break;
										}
									}

									unsigned char PacketNumber=1;
									unsigned char PacketCrc=0;
									unsigned long int SectorAddr=XDriveNumber;
									SectorAddr=(SectorAddr*80000);

									// 77 tracks, 26 sectors per track makes 2002 overall sectors
									for(unsigned int Xloop=0;Xloop<2002;Xloop++)
									{
										PacketCrc=0;			//re-initialises checksum variable
										RomSectorRead(SectorAddr,0);	//reads desired sector from ROM to buffer

										UartTxAddByte(0x01);		//Sends SOH - packet start

										UartTxAddByte(PacketNumber);	//Sends packet number
										UartTxAddByte(0xFF-PacketNumber);
										//Sends 128 bytes - one sector stored in buffer
										for (unsigned char XByteLoop=0;XByteLoop<128;XByteLoop++)
										{
											UartTxAddByte(CopyArray[XByteLoop]);
											PacketCrc+=CopyArray[XByteLoop];	//Adds every byte to checksum
										}
										UartTxAddByte(PacketCrc);	//Sends checksum

										while(1)
										{
											unsigned char BuffTest=UartIsBufferEmpty();
											_delay_ms(1);
											if(BuffTest==0)
											{
												unsigned char AckTest=UartRxGetByte();
												if(AckTest==0x06)
													break;
											}
										}

									//TO DO - make ASCII control code include?

										SectorAddr+=0x80;
										PacketNumber++;
									}

									UartTxAddByte(0x04);
									_delay_ms(2000);
									UartSendString("Done\x0D\x0A");
									UartSendString("\x0D\x0A");
									UartSendString(">");
									break;
								}

								case 'r':
								case 'R':
								{
									UartSendString("\x0D\x0A");
									UartSendString("Receiving RX");
									UartTxAddByte(CliBuffer[2]);
									UartSendString(" image over Xmodem...");
									unsigned char XDriveNumber=(CliBuffer[2]&1);


								unsigned long int SectorAddr=0xC0000;

								for(unsigned char EraseLoop=0;EraseLoop<64;EraseLoop++)
								{
									RomEraseBlock(SectorAddr);
									SectorAddr+=0x1000;
								}




									XmodemTimeout=0;
									UartTxAddByte(0x15);

									while(1)
									{
										unsigned char BuffTest=UartIsBufferEmpty();
										_delay_ms(1);
										if(BuffTest==0)
										{
											break;
										}

										if(XmodemTimeout>=30)
										{
											XmodemTimeout=0;
											UartTxAddByte(0x15);
										}

									}

									unsigned int PreferredPacketNum=1;
									SectorAddr=0xC0000;
								//	unsigned char PreferredCrc=0;
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

								//	for(unsigned int Xloop=0;Xloop<2002;Xloop++)
									while(1)
									{
										unsigned char PreferredCrc=0;
										unsigned char ReceivedBytes=0;

										while(1)
										{
											unsigned char BuffTest=UartIsBufferEmpty();
											_delay_us(1);
											//TO DO - get rid of these useless delays
											if(BuffTest==0)
											{
												unsigned char XRecByte=UartRxGetByte();
												switch(ReceivedBytes)
												{
													case 0:
													{
														if(XRecByte==0x01)
														{
															ReceivedBytes++;
															XmodemTransferStatus=TransferInProcess;
														}
														else
														{
															if(XRecByte==0x04)
															{
																XmodemTransferStatus=TransferCompleted;
															}
															else
															{
																XmodemTransferStatus=TransferNoSoh;
															}
														}

														break;
													}

													case 1:
													{
														if(XRecByte==(PreferredPacketNum&0xFF))
														{
															ReceivedBytes++;
														}
														else
														{
															XmodemTransferStatus=TransferOutOfSync;
														}
														break;
													}

													case 2:
													{
														if(XRecByte==(255-(PreferredPacketNum&0xFF)))
														{
															ReceivedBytes++;
														}
														else
														{
															XmodemTransferStatus=TransferOutOfSync;
														}
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
															//TO DO - protect write so it doesn't go out of tem array boundaries, otherwise backup will be damaged
															//TO DO - TransferFault can be set if file size exceeds desired
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
											{
												break;
											}
										}
										switch(XmodemTransferStatus)
										{
											case TransferCompleted:
											{
												//TO DO - check received file length (2002 * 128 bytes)
												//TO DO - copy temporary array to working array
												
												
								//Erase backup region
								//RomEnWrite();

								unsigned long int RestoreOffset=XDriveNumber;
								RestoreOffset=RestoreOffset*0x80000;
								//BackupAddr+=0x100000;
								//UartSendString("Erasing ...\x0D\x0A");

								unsigned long int SectorAddr=0;

								for(unsigned char EraseLoop=0;EraseLoop<64;EraseLoop++)
								{
									RomEraseBlock(SectorAddr+RestoreOffset);
									SectorAddr+=0x1000;
								}

								//Copy image, sector by sector
								SectorAddr=0;

								for(unsigned int BackupCopyLoop=0;BackupCopyLoop<2048;BackupCopyLoop++)
								{
									RomSectorRead(SectorAddr+0xC0000,0);
									RomSectorWrite(SectorAddr+RestoreOffset,0);
									SectorAddr+=0x80;
								}

												
												
												
												
												
												UartTxAddByte(0x06);
												_delay_ms(1000);
												UartSendString("Done\x0D\x0A");
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
										if(XmodemTransferStatus>TransferSectorOk)
											break;
									}



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
								unsigned long int BackupAddr=BackupDriveNumber;
								BackupAddr=BackupAddr*0x80000;
								BackupAddr+=0x100000;
								UartSendString("Erasing...\x0D\x0A");

								for(unsigned char EraseLoop=0;EraseLoop<64;EraseLoop++)
								{
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
								//RomEnWrite();

								unsigned long int BackupAddr=BackupDriveNumber;
								BackupAddr=BackupAddr*0x80000;
								//BackupAddr+=0x100000;
								UartSendString("Erasing ...\x0D\x0A");

								for(unsigned char EraseLoop=0;EraseLoop<64;EraseLoop++)
								{
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
						UartSendString("\x0D\x0A");
						UartSendString("0x80000\x0D\x0A");
						RomSectorRead(0x080000,0);
						for (unsigned int testvar=0;testvar<128;testvar++)
						{
							HexSend(CopyArray[testvar]);
							UartTxAddByte(' ');
							if((testvar&7)==7)
								UartSendString("\x0D\x0A");
						}
						UartSendString("\x0D\x0A");
						UartSendString("0xC0000\x0D\x0A");
						RomSectorRead(0x0C0000,0);
						for (unsigned int testvar=0;testvar<128;testvar++)
						{
							HexSend(CopyArray[testvar]);
							UartTxAddByte(' ');
							if((testvar&7)==7)
								UartSendString("\x0D\x0A");
						}
						UartSendString("\x0D\x0A>");
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
