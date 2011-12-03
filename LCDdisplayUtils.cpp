//******************************************************************************************
//*	LCDdisplayUtils.cpp for use with Makerbot MIB2 interface
//*		(C) 2011 by Mark Sproul
//*		Open source as per standard Arduino code
//*
//******************************************************************************************
//*	Edit History
//******************************************************************************************
//*	Oct  2, 2011	<MLS> Adding support for MakerBot Interface board (MIB)
//******************************************************************************************


#if (ARDUINO >= 100)
	#include <Arduino.h>
#else
	#include <WProgram.h>
#endif
#include	<LiquidCrystal.h>
#if defined(__AVR__)
	#ifndef __PGMSPACE_H_
		#include	<avr/pgmspace.h>
	#endif
#endif


//*	Makerbot Gen 4 libraries
#include	"MIBdisplayDriver.h"
#include	"SMD3stepperDriver.h"
#include	"LCDdisplayUtils.h"

prog_char	gXlabelString[]	PROGMEM	=	"X: ";
prog_char	gYlabelString[]	PROGMEM	=	"Y: ";
prog_char	gZlabelString[]	PROGMEM	=	"Z: ";

unsigned long	gLastLCDupdateTime;


//******************************************************************************************
//*	this is so we can print from program memory
void	LCD_print_P(const char *stringPtr)
{
char	stringBuff[32];
short	ii;
char	theChar;

	ii	=	0;
	while ((theChar = pgm_read_byte_near((long)stringPtr + ii)) && (ii<16))
	{
		stringBuff[ii++]	=	theChar;
	}
	stringBuff[ii]	=	0;
	LCD.print(stringBuff);
	
}

//******************************************************************************************
void	LCDdisplay_Info(const char *line1, const char *line2a, const char *line2b, const char *line3)
{
short	lcdRowNum;

	LCD.clear();
	
	lcdRowNum	=	0;
	LCD.setCursor(0, lcdRowNum);
	LCD_print_P(line1);
	lcdRowNum++;

	LCD.setCursor(0, lcdRowNum);
	LCD_print_P(line2a);
	LCD_print_P(line2b);

	lcdRowNum++;

	LCD.setCursor(0, lcdRowNum);
	LCD_print_P(line3);
	lcdRowNum++;

}

//******************************************************************************************
void	LCDdisplay_PrintRightJustified(long theNumber)
{
char	numstring[10];

	sprintf(numstring, "%6ld ", theNumber);
	LCD.print(numstring);
}


//******************************************************************************************
void	LCDdisplay_XYZposition(boolean eraseScreen, const char *titleProgMem)
{
short	lcdRowNum;

	if (eraseScreen)
	{
		LCD.clear();
		LCD.noBlink();
	}
	
	lcdRowNum	=	0;
	LCD.setCursor(0, lcdRowNum);
	//*		   123456789 123456
	LCD_print_P(titleProgMem);

	lcdRowNum++;
	LCD.setCursor(0, lcdRowNum);
	LCD_print_P(gXlabelString);
	LCDdisplay_PrintRightJustified(MB2_GetCurrentStepperLocation(kMB2_StepperX));

	lcdRowNum++;
	LCD.setCursor(0, lcdRowNum);
	LCD_print_P(gYlabelString);
	LCDdisplay_PrintRightJustified(MB2_GetCurrentStepperLocation(kMB2_StepperY));

	lcdRowNum++;
	LCD.setCursor(0, lcdRowNum);
	LCD_print_P(gZlabelString);
	LCDdisplay_PrintRightJustified(MB2_GetCurrentStepperLocation(kMB2_StepperZ));
	lcdRowNum++;
}


//******************************************************************************************
void	LCDdisplay_EndStops(short lcdRow, short minPin, short maxPin)
{
short	stopStateMin;
short	stopStateMqx;

	LCD.setCursor(12, lcdRow);
	stopStateMin	=	digitalRead(minPin);
	stopStateMqx	=	digitalRead(maxPin);
	LCD.print(stopStateMin);
	LCD.print(" ");
	LCD.print(stopStateMqx);
}
