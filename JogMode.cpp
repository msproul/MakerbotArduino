//******************************************************************************************
//*	Jog Mode - for use with Makerbot Gen 4 library
//*		(C) 2011 by Mark Sproul
//*		Open source as per standard Arduino code
//*
//*		http://wiki.makerbot.com/smd3
//*		http://wiki.makerbot.com/mib1
//*		http://www.makerbot.com/docs/makerbot-gen4-interface-kit/
//******************************************************************************************
//*	This is set up to use MakerBot gen 4 hardware for stepper motor control and interface
//******************************************************************************************
//*	Edit History
//******************************************************************************************
//*	Nov  7, 2011	<MLS> Separated jog mode to its own file
//******************************************************************************************

#if defined(__AVR__)
	#ifndef __PGMSPACE_H_
		#include	<avr/pgmspace.h>
	#endif
#endif

#if (ARDUINO >= 100)
	#include <Arduino.h>
#else
	#include <WProgram.h>
#endif

//*	Makerbot Gen 4 libraries
#include	"JogMode.h"
#include	"LCDdisplayUtils.h"
#include	"MakerbotMB2.h"
#include	"MIBdisplayDriver.h"	//*	MIB = MakerBot Interface board
#include	"SMD3stepperDriver.h"



#define	kJogDeltaSteps	20
//******************************************************************************************
//*	this brings up a display on the LCD screen and allows the user to move the head in x,y,z by using
//*	the buttons on the control panel
//*	returns what button was hit (-1 for none)
//******************************************************************************************
short	Jog_Display(const char *title)
{
boolean	changeOccured;
long	currentLocation;
short	lastButtonHit;

	MIB_CheckInputButtons();

	changeOccured	=	false;
	lastButtonHit	=	-1;
	
	if (gMIBbuttonArray[kMIB_Button_XPlus].buttonIsDown)
	{
		lastButtonHit	=	kMIB_Button_XPlus;
		currentLocation	=	MB2_GetCurrentStepperLocation(kMB2_StepperX);
		MB2_StepperMoveToLocation(kMB2_StepperX, currentLocation + kJogDeltaSteps);
		changeOccured	=	true;
		//*	setting buttonIsDown to false will make this repeat over and over
		gMIBbuttonArray[kMIB_Button_XPlus].buttonIsDown			=	false;
	}
	else if (gMIBbuttonArray[kMIB_Button_XMinus].buttonIsDown)
	{
		lastButtonHit	=	kMIB_Button_XMinus;
		currentLocation	=	MB2_GetCurrentStepperLocation(kMB2_StepperX);
		MB2_StepperMoveToLocation(kMB2_StepperX, currentLocation - kJogDeltaSteps);

		changeOccured	=	true;
		gMIBbuttonArray[kMIB_Button_XMinus].buttonIsDown		=	false;
	}

	if (gMIBbuttonArray[kMIB_Button_YPlus].buttonIsDown)
	{
		lastButtonHit	=	kMIB_Button_YPlus;
		currentLocation	=	MB2_GetCurrentStepperLocation(kMB2_StepperY);
		MB2_StepperMoveToLocation(kMB2_StepperY, currentLocation + kJogDeltaSteps);
		changeOccured	=	true;
		gMIBbuttonArray[kMIB_Button_YPlus].buttonIsDown			=	false;
	}
	else if (gMIBbuttonArray[kMIB_Button_YMinus].buttonIsDown)
	{
		lastButtonHit	=	kMIB_Button_YMinus;
		currentLocation	=	MB2_GetCurrentStepperLocation(kMB2_StepperY);
		MB2_StepperMoveToLocation(kMB2_StepperY, currentLocation - kJogDeltaSteps);
		changeOccured	=	true;
		gMIBbuttonArray[kMIB_Button_YMinus].buttonIsDown		=	false;
	}

	if (gMIBbuttonArray[kMIB_Button_ZPlus].buttonIsDown)
	{
		lastButtonHit	=	kMIB_Button_ZPlus;
		currentLocation	=	MB2_GetCurrentStepperLocation(kMB2_StepperZ);
		MB2_StepperMoveToLocation(kMB2_StepperZ, currentLocation + kJogDeltaSteps);
		changeOccured	=	true;
		gMIBbuttonArray[kMIB_Button_ZPlus].buttonIsDown			=	false;
	}
	else if (gMIBbuttonArray[kMIB_Button_ZMinus].buttonIsDown)
	{
		lastButtonHit	=	kMIB_Button_ZMinus;
		currentLocation	=	MB2_GetCurrentStepperLocation(kMB2_StepperZ);
		MB2_StepperMoveToLocation(kMB2_StepperZ, currentLocation - kJogDeltaSteps);
		changeOccured	=	true;
		gMIBbuttonArray[kMIB_Button_ZMinus].buttonIsDown		=	false;
	}

	//*	some routines that call Jog_Display use the zero button
	if (gMIBbuttonArray[kMIB_Button_Zero].buttonIsDown)
	{
		lastButtonHit	=	kMIB_Button_Zero;
		gMIBbuttonArray[kMIB_Button_Zero].buttonHasChanged	=	false;
	}



	//*	this one is different, we have to check to make sure it has been released
	if ((gMIBbuttonArray[kMIB_Button_OK_Sel].buttonIsDown) && (gMIBbuttonArray[kMIB_Button_OK_Sel].buttonHasChanged))
	{
		lastButtonHit	=	kMIB_Button_OK_Sel;
		gMIBbuttonArray[kMIB_Button_OK_Sel].buttonHasChanged	=	false;
	}

	if (gMIBbuttonArray[kMIB_Button_Cancel].buttonIsDown)
	{
		lastButtonHit	=	kMIB_Button_Cancel;
		gMIBbuttonArray[kMIB_Button_Cancel].buttonHasChanged	=	false;
	}

	if (((millis() - gLastLCDupdateTime) > 200) || gMIBupdateLCDscreen)
	{
		LCDdisplay_XYZposition(true, title);
		LCDdisplay_EndStops(1, kMB2_Limit_XMin, kMB2_Limit_XMax);	//*	line 2 of the LCD screen 
		LCDdisplay_EndStops(2, kMB2_Limit_YMin, kMB2_Limit_YMax);
		LCDdisplay_EndStops(3, kMB2_Limit_ZMin, kMB2_Limit_ZMax);

		gMIBupdateLCDscreen	=	false;
		gLastLCDupdateTime	=	millis();
	}
	if (changeOccured)
	{
//		PnpDelay(100);
	}
	
	
	return(lastButtonHit);
}


//******************************************************************************************
void	JogMode(const char *title)
{
short	lastButtonHit;

	lastButtonHit	=	Jog_Display(title);
	if ((lastButtonHit == kMIB_Button_OK_Sel) || (lastButtonHit == kMIB_Button_Cancel))
	{
		MIB_SetToMainMenu();
	}
}
