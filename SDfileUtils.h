//******************************************************************************************
//*	SD File utility routines
//*		Open source as per standard Arduino code
//*
//******************************************************************************************
//*	Edit History
//******************************************************************************************
//*	Nov  1,	2011	<MLS> Started working on SD file utilities
//******************************************************************************************
//#include	"SDfileUtils.h"


#ifndef __SD_H__
	#include	<SD.h>
#endif

extern	boolean		gSDcardsNeedsInit;

#ifdef __cplusplus
extern "C"{
#endif

void	InitSDcard(void);
short	ReadOneLineFromFile(File *myFile, char *lineBuffer, short maxLineLen);
short	CountLinesInFile(char *theFileName);
void	DumpFileToSerialPort(char *theFileName);


#ifdef __cplusplus
} // extern "C"
#endif
