#define		CliBufferMax	16

unsigned char	CliStatus=0;
unsigned char	CliBuffer[CliBufferMax];
unsigned char	CliBufferPointer=0;

void	CliInit(void);
void	CliRoutine(unsigned char CliData);
void    UnknownCommand(void);
void    IncorrectDrive(void);

#include	"cli_dxemu.c"
