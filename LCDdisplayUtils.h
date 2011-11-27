//******************************************************************************************
//*	LCDdisplayUtils.cpp
//*		(C) 2011 by Mark Sproul
//*		Open source as per standard Arduino code
//*
//******************************************************************************************
//*	Edit History
//******************************************************************************************
//*	Oct  2, 2011	<MLS> Adding support for MakerBot Interface board (MIB)
//******************************************************************************************
//#include	"LCDdisplayUtils.h"


#ifndef LiquidCrystal_h
	#include	<LiquidCrystal.h>
#endif

extern	LiquidCrystal	LCD;
extern	prog_char	gXlabelString[];
extern	prog_char	gYlabelString[];
extern	prog_char	gZlabelString[];

extern	unsigned long	gLastLCDupdateTime;

#ifdef __cplusplus
extern "C"{
#endif


void	LCD_print_P(const char *stringPtr);
void	LCDdisplay_Info(const char *line1, const char *line2a, const char *line2b, const char *line3);
void	LCDdisplay_XYZposition(boolean eraseScreen, const char *titleProgMem);
void	LCDdisplay_EndStops(short lcdRow, short minPin, short maxPin);

#ifdef __cplusplus
} // extern "C"
#endif
