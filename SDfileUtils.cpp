//******************************************************************************************
//*	SD File utility routines
//*		Open source as per standard Arduino code
//*
//******************************************************************************************
//*	Edit History
//******************************************************************************************
//*	Nov  1,	2011	<MLS> Started working on SD file utilities
//******************************************************************************************



#if (ARDUINO >= 100)
	#include <Arduino.h>
#else
	#include <WProgram.h>
#endif
#include	<SD.h>

#include	"SDfileUtils.h"

boolean		gSDcardsNeedsInit;

#ifdef _BOARD_MEGA_
	#define	kSDchipSelect	53
#else
	#define	kSDchipSelect	53
#endif

//******************************************************************************************
void InitSDcard(void)
{

	if (gSDcardsNeedsInit)
	{
		pinMode(kSDchipSelect, OUTPUT);

		if (SD.begin(kSDchipSelect))
		{
			gSDcardsNeedsInit	=	false;
		}
		else
		{
			gSDcardsNeedsInit	=	true;
		}
	}
}


//******************************************************************************************
//*	returns line length
//******************************************************************************************
short	ReadOneLineFromFile(File *myFile, char *lineBuffer, short maxLineLen)
{
char	theChar;
short	bufIdx;
boolean	keepGoing;

	bufIdx			=	0;
	keepGoing		=	true;
	lineBuffer[0]	=	0;
	while (keepGoing && myFile->available())
	{
		theChar	=	myFile->read();
		
		if ((theChar == 0x0d) || (theChar == 0x0a))
		{
			keepGoing	=	false;
			lineBuffer[bufIdx++]	=	0;
		}
		else if ((theChar >= 0x20) && (theChar != '"'))
		{
			if (bufIdx < (maxLineLen - 1))
			{
				lineBuffer[bufIdx++]	=	theChar;
			}
		}
	}
	return(strlen(lineBuffer));
}

//******************************************************************************************
short	CountLinesInFile(char *theFileName)
{
short	lineCount;
File	theFileDesc;
char	lineBuffer[80];

	lineCount		=	0;
	theFileDesc	=	SD.open(theFileName);
	if (theFileDesc)
	{
		while (theFileDesc.available())
		{
			ReadOneLineFromFile(&theFileDesc, lineBuffer, 79);
			if (lineBuffer[0] != '#')
			{
				lineCount++;
			}
		}
		theFileDesc.close();			// close the file
	}
	return(lineCount);
}

