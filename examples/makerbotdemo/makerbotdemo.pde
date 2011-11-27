//******************************************************************************************
//*	Makerbot demo code
//*		(C) 2011 by Mark Sproul
//*		Open source as per standard Arduino code
//*
//*	This is a demo of the Makerbot gen 4 hardware using Arduino.
//*	This is intdended to be a demo of how to use the Makerbot hardware from the Arduino environment
//*	and provide an open source library for using it.
//*	
//*	The main loop is set up as a state machine. The menu display handler sets the state based on what
//*	the user selects from the menu.
//*	Adding things to the menu is very easy
//*
//*	the Stepper motors are run from an interrupt routine similar to the way the Makerbot code does it
//*
//*	Thoughout this code, I try to put most strings in FLASH memory (PROGMEM) in order to conserve RAM
//*
//*		http://wiki.makerbot.com/smd3
//*		http://wiki.makerbot.com/mib1
//*		http://www.makerbot.com/docs/makerbot-gen4-interface-kit/
//******************************************************************************************
//*	Conventions used in this code
//*		Global variables start with a lower case "g" follwed by an UPPER case char
//*		Constants, i.e. #defines or enums start with a "k" (I know "constant start with a "c", but that gets used for c++ classes alot)
//*		Routine names start with an UPPER CASE character (except for setup and loop)
//******************************************************************************************
//*	Edit History
//******************************************************************************************
//*	Nov 17, 2011	<MLS> Libraries working well, strated on demo example
//*	Nov 26, 2011	<MLS> Added jogmode to demo
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
#include	<HardwareSerial.h>
#include	<SD.h>
#include	<LiquidCrystal.h>

#include	"EEPROM.h"

//*	Makerbot Gen 4 libraries
#include	"JogMode.h"
#include	"LCDdisplayUtils.h"
#include	"MakerbotMB2.h"			//*	Makerbot MB2 motherboard
#include	"MakerbotMIB.h"			//*	MIB = MakerBot Interface board
#include	"MIBdisplayDriver.h"	
#include	"SMD3stepperDriver.h"	//*	http://wiki.makerbot.com/smd3

#define	_DEBUG_

//******************************************************************************************
//*	globals
//******************************************************************************************

boolean			gSteppersNeedCalibrated;
short			gWhichHelpMsg	=	0;
boolean			gStateFirstTime	=	false;;


//*	the MIBdisplayDriver library provides the abilty to display a menu, handle that menu
//*	and build a state machine from the menu

//******************************************************************************************
//*	define our state machine
enum {
		kState_Idle	=	0,
		kState_Help,
		kState_Info,
		kState_Home,
		kState_Calibrate,
		kState_ShowSDfiles,
		kState_JOG,
		kState_SetSpeed,
		
		
		//*	etc

	};


//******************************************************************************************
//*	this menu type is defined in MIBdisplayDriver.h and used with the MIB library
//*	this is in PROGMEM, we have LOTS of program space and not much RAM
//******************************************************************************************
TYPE_MenuDef gMainMenu[] PROGMEM =
{
	{	kState_Help,				"Help"},
	{	kState_Info,				"Info"},
	{	kState_Home,				"Home"},
	{	kState_ShowSDfiles,			"Show SD files"},
	{	kState_Calibrate,			"Calibrate"},
	{	kState_JOG,					"Jog"},
	{	kState_SetSpeed,			"Set Speed"},

	{	-1,						"-"}
};
#define	kMenuCount	5


//******************************************************************************************
//*	various string definitions
//*	strings are normally kept in RAM, in order to have them in FLASH, they have to be set up as PROGMEM
//***************************************************************************x***************
prog_char	gPgmNameString[]			PROGMEM	=	"Makerbot/Arduino";
prog_char	gVersionString[]			PROGMEM	=	"V1.0";
prog_char	gTextMsg_GCC_DATE_STR[]		PROGMEM	=	__DATE__;
#if defined(__AVR__)
prog_char	gCPU_Platform[]				PROGMEM	=	" AVR";
#endif
#if defined(__PIC32MX__)
prog_char	gCPU_Platform[]				PROGMEM	=	" PIC32";
#endif
prog_char	gSpaceString[]				PROGMEM	=	" ";
prog_char	gJogModeString[]			PROGMEM	=	"Jog mode";
prog_char	gNotImplementedYet[]		PROGMEM	=	"Not implemented";
prog_char	gTextGoingHome[]			PROGMEM	=	"Going Home";
prog_char	gTextCanceled[]				PROGMEM	=	"Canceled      ";



//******************************************************************************************
//*										 123456789 123456	
prog_char	gHelpMessage0[]	PROGMEM	=	"Menu Navitgation\r"
										"            Prev\r"
										"     Back Select\r"
										"            Next\r";

prog_char	gHelpMessage1[]	PROGMEM	=	"Button layout\r"
										"   Y+         Z+\r"
										"X- 0 X+ Back Sel\r"
										"   Y-         Z-\r";

prog_char	gHelpMessage2[]	PROGMEM	=	"Calibrate\r"
										"  Must be done\r"
										"  each time CPU\r"
										"  is reset\r";

prog_char	gHelpMessage3[]	PROGMEM	=	"LEDs\r"
										" BAR = on when \r"
										" steppers active\r";

prog_char	gHelpMessage4[]	PROGMEM	=	"Debug/status\r"
										" via serial\r"
										" at 9600 baud\r";

#define	kHelpScreenCount	5

//******************************************************************************************
void setup()
{
int	ii;

	pinMode(kMB2_ATXpowerSupplyEnable, OUTPUT);
	digitalWrite(kMB2_ATXpowerSupplyEnable, LOW);

	//*	init stuff for menu management
	gMIBcurrentMenuOffset	=	0;
	gSteppersNeedCalibrated	=	true;
	gWhichHelpMsg			=	0;
	gStateFirstTime			=	false;;
	
	
	Serial.begin(9600);
#ifdef _DEBUG_
	Serial.println();
	Serial.println("rebooting");
#endif


	delay(200);	//	give the LCD time to power up
	//*	init the Makerbot Interface Board, this inits the LCD and the buttons
	MIB_Init();

	MIB_SysBeep(1);



	//******************************************************************************************
	//*	init the state machine
	gMIBcurrentState	=	kState_Idle;

	MB2_InitAllSteppers();
#ifdef _DEBUG_
	Serial.print("Stepper count =");
	Serial.println(kMB2_MaxSteppers);
#endif



#ifdef _DEBUG_
	//*	dump the menu out the serial port
	MIB_DumpMenu(gMainMenu, kMenuCount);
#endif

	LCDdisplay_Info(gPgmNameString, gVersionString, gCPU_Platform, gTextMsg_GCC_DATE_STR);

	//*	enable the interrupts for the stepper motors
	StepperInterrupt_Init();
	
}
 




//******************************************************************************************
void loop()
{
boolean	myUpdateMenu;
char	stringBuff[32];
boolean	updateNeeded;
short	previousState;


	//*	do some house keeping
	
	//*	this sets the LED state for active stepper motor
	MB2_StepperIsActive();

	if (MIB_IsCancelDown())
	{
		MB2_CancelAllMovement();
	}

	MIB_CheckInputButtons();

	updateNeeded	=	false;

	//* now for our state machine
	previousState	=	gMIBcurrentState;
	switch(gMIBcurrentState)
	{
		case kState_Idle:
			myUpdateMenu	=	MIB_ProcessMenuButtons(gMainMenu, kMenuCount);
			if (myUpdateMenu)
			{
				//*	done this way so other routines can force menu update
				gMIBupdateMenu	=	true;
			}
			if (gMIBupdateMenu)
			{
				MIB_DisplayMenu(gMainMenu, gMIBcurrentMenuOffset, gMIBcurrentMenuSelected);
				gMIBupdateMenu	=	false;

				MB2_DisableAllSteppers();	//*	disable the stepper motors so they dont get to hot
			}
			//*	check to see if the state changed
			if (gMIBcurrentState != previousState)
			{
				StateMachineFirstTime(gMIBcurrentState);
			}
			break;
		
		case kState_Help:
			DisplayHelp();
			break;

		case kState_Info:
			if (gMIBupdateLCDscreen)
			{
				LCDdisplay_Info(gPgmNameString, gVersionString, gCPU_Platform, gTextMsg_GCC_DATE_STR);
			
				gMIBupdateLCDscreen	=	false;
				updateNeeded		=	true;
			}
			if (updateNeeded || ((millis() % 1000) == 0))
			{
				FormatTimeString(millis(), stringBuff);
				LCD.setCursor(0, 3);
				LCD.print("up ");
				LCD.print(stringBuff);
			}
			if (MIB_IsCancelDown())
			{
				MIB_SetToMainMenu();
			}
			break;

		case kState_Home:
			LCDdisplay_XYZposition(true, gTextGoingHome);
			LCDdisplay_EndStops(1, kMB2_Limit_XMin, kMB2_Limit_XMax);	//*	line 2 of the LCD screen 
			LCDdisplay_EndStops(2, kMB2_Limit_YMin, kMB2_Limit_YMax);
			LCDdisplay_EndStops(3, kMB2_Limit_ZMin, kMB2_Limit_ZMax);
			delay(250);

			//*	check to see if the user cancelled
			if (MIB_IsCancelDown())
			{
				//*	its been canceled, set the desired location to be the current location
				MB2_CancelMove(kMB2_StepperX);
				MB2_CancelMove(kMB2_StepperY);
				MB2_CancelMove(kMB2_StepperZ);
				MB2_CancelMove(kMB2_StepperB);
				LCD.setCursor(0, 0);
				LCD_print_P(gTextCanceled);
				delay(2000);
				MIB_SetToMainMenu();
			}
			else if (!MB2_StepperIsActive())
			{
				LCDdisplay_XYZposition(true, gTextGoingHome);
				delay(1000);
				MIB_SetToMainMenu();
			}
			break;
			
		case kState_Calibrate:
			Calibrate();
			MIB_SetToMainMenu();
			break;
		
		case kState_ShowSDfiles:
			ShowSDfiles();
			MIB_SetToMainMenu();
			break;
			
		case kState_JOG:
			JogMode(gJogModeString);
			break;

		case kState_SetSpeed:
			SetStepperSpeed();
			break;
		
		default:
			MIB_SysBeep(1);
			LCD.clear();
			LCD_print_P(gNotImplementedYet);
			delay(2000);
			MIB_SetToMainMenu();
			break;
	}


}
//******************************************************************************************
void StateMachineFirstTime(short theNewState)
{
	switch(theNewState)
	{

		case kState_Home:
			MB2_StepperMoveToLocation(kMB2_StepperX, 0);
			MB2_StepperMoveToLocation(kMB2_StepperY, 0);
			MB2_StepperMoveToLocation(kMB2_StepperZ, 0);
			break;
			
		default:
			break;
	}
}



#define	kDeltaORC1	100
//******************************************************************************************
void	SetStepperSpeed(void)
{
boolean	changeOccured;
long	currentLocation;
short	lastButtonHit;

	MIB_CheckInputButtons();

	changeOccured	=	false;
	lastButtonHit	=	-1;
	
	if (gMIBbuttonArray[kMIB_Button_ZPlus].buttonIsDown && gMIBbuttonArray[kMIB_Button_ZPlus].buttonHasChanged)
	{
	#if defined(__AVR__)
		OCR1A			+=	kDeltaORC1;
	#endif
		changeOccured	=	true;

		gMIBbuttonArray[kMIB_Button_ZPlus].buttonHasChanged	=	false;
	}
	else if (gMIBbuttonArray[kMIB_Button_ZMinus].buttonIsDown && gMIBbuttonArray[kMIB_Button_ZMinus].buttonHasChanged)
	{
	#if defined(__AVR__)
		OCR1A			-=	kDeltaORC1;
	#endif
		changeOccured	=	true;

		gMIBbuttonArray[kMIB_Button_ZMinus].buttonHasChanged	=	false;
	}
	else if (gMIBbuttonArray[kMIB_Button_Cancel].buttonIsDown)
	{
		gMIBbuttonArray[kMIB_Button_Cancel].buttonHasChanged	=	false;
		MIB_SetToMainMenu();
	}
	//*	this one is different, we have to check to make sure it has been released
	else if ((gMIBbuttonArray[kMIB_Button_OK_Sel].buttonIsDown) && (gMIBbuttonArray[kMIB_Button_OK_Sel].buttonHasChanged))
	{
		gMIBbuttonArray[kMIB_Button_OK_Sel].buttonHasChanged	=	false;
		MIB_SetToMainMenu();
	}

	if (changeOccured || gMIBupdateLCDscreen)
	{
		LCD.clear();
		LCD.setCursor(0, 0);
	#if defined(__AVR__)
		LCD.print("OCR1A=");
		LCD.print(OCR1A);
	#endif

		gMIBupdateLCDscreen	=	false;
		delay(100);
	}
}



//******************************************************************************************
void	Calibrate(void)
{
boolean	calibrateOK_X;
boolean	calibrateOK_Y;
boolean	calibrateOK_Z;

	calibrateOK_X	=	MB2_CalibrateAxis(kMB2_StepperX, 1, gXlabelString);
	calibrateOK_Y	=	MB2_CalibrateAxis(kMB2_StepperY, 2, gYlabelString);
	calibrateOK_Z	=	MB2_CalibrateAxis(kMB2_StepperZ, 3, gZlabelString);

	MB2_DisableAllSteppers();	//*	disable the stepper motors so they dont get to hot

	if (calibrateOK_X && calibrateOK_Y && calibrateOK_Z)
	{
		gSteppersNeedCalibrated	=	false;
	}
	


	//*	wait for the OK button or CANCEL button
	while ((MIB_IsOKDown() == false) && (MIB_IsCancelDown() == false) && (MIB_IsButtonDown(kMIB_Button_Zero) == false))
	{
		//*	do nothing
	}

}

//******************************************************************************************
void	ShowSDfiles(void)
{
// This code is just copied from SdFile.cpp in the SDFat library
dir_t		p;
Sd2Card		card;
SdVolume	volume;
SdFile		root;
uint8_t		flags;
int			lcdRowNum;
char		fileName[16];
int			fnIdx;
uint8_t		ii;
boolean		okFlag;

	Serial.println("ShowSDfiles");
	
	okFlag		=	false;
	lcdRowNum	=	0;
	LCD.clear();
	LCD.setCursor(0, lcdRowNum++);

	flags	=	LS_R | LS_DATE | LS_SIZE;

	if (card.init(SPI_HALF_SPEED, kMB2_SDcardChipSelect))
	{
		// initialize a FAT volume
		if (volume.init(&card))
		{
			if (root.openRoot(&volume))
			{
				fnIdx	=	0;
				root.rewind();
				while (root.readDir(p) > 0) 
				{
					okFlag		=	true;
					// done if past last used entry
					if (p.name[0] == DIR_NAME_FREE) break;

					// skip deleted entry and entries for . and  ..
					if (p.name[0] == DIR_NAME_DELETED || p.name[0] == '.') continue;

					// only list subdirectories and files
					if (!DIR_IS_FILE_OR_SUBDIR(&p)) continue;

					if (DIR_IS_SUBDIR(&p))	 continue;

					if (p.name[0] == '_') continue;

					for (ii = 0; ii < 11; ii++)
					{
						if (p.name[ii] == ' ') continue;
						if (ii == 8)
						{
							fileName[fnIdx++]	=	'.';
						}
						fileName[fnIdx++]	=	p.name[ii];

					}
					fileName[fnIdx++]	=	0;
					if (strstr(fileName, ".XY") != NULL)
					{
						LCD.print(">");
					}
					else
					{
						LCD.print(" ");
					}
					LCD.print(fileName);
					Serial.print(fileName);
					fnIdx	=	0;
					
					
					// print modify date/time if requested
					if (flags & LS_DATE)
					{
						Serial.print("\t");
						root.printFatDate(p.lastWriteDate);
						Serial.print(' ');
						root.printFatTime(p.lastWriteTime);
					}
					// print size if requested
					if (!DIR_IS_SUBDIR(&p) && (flags & LS_SIZE))
					{
						Serial.print("\t");
						Serial.print(p.fileSize);
					}
					Serial.println();
					LCD.setCursor(0, lcdRowNum++);
				}
			}
		}
	}

	if (okFlag == false)
	{
		//*	one error messge no mater what failed
		LCD.print("SDcard failed");
	}
	
	delay(3000);
	while (!MIB_IsOKDown() && !MIB_IsCancelDown())
	{
		//*	wait for user input
	}
}



//******************************************************************************************
void	FormatTimeString(unsigned long argMilliSeconds, char *timeString)
{
unsigned long	totalSeconds;
short			upTimeHours;
short			upTimeMinutes;
short			upTimeSeconds;

	totalSeconds	=	argMilliSeconds / 1000;
	upTimeHours		=	totalSeconds / 3600;
	upTimeMinutes	=	(totalSeconds % 3600) / 60;
	upTimeSeconds	=	totalSeconds % 60;
	
	sprintf(timeString, "%02d:%02d:%02d", upTimeHours, upTimeMinutes, upTimeSeconds);
}





//******************************************************************************************
void	DisplayHelpScreen(const char *stringPtr)
{
char	lcdLineBuff[18];
short	ii;
short	cc;
short	lcdRowNum;
char	theChar;
boolean	keepGoing;

	keepGoing	=	true;
	ii			=	0;
	cc			=	0;
	lcdRowNum	=	0;
	while (keepGoing && (lcdRowNum < 4))
	{
		theChar	=	pgm_read_byte_near(stringPtr + ii++);
		if ((theChar >= 0x20) && (cc < 16))
		{
			lcdLineBuff[cc++]	=	theChar;
		}
		else if ((theChar == 0x0d) || (cc >= 16))
		{
			//*	end of line, display it

			lcdLineBuff[cc++]	=	0;
			LCD.setCursor(0, lcdRowNum);
			LCD.print(lcdLineBuff);
			
			cc	=	0;
			lcdRowNum++;
		}
		else if (theChar == 0)
		{
			keepGoing	=	false;
		}
	}
}



//******************************************************************************************
void	DisplayHelp(void)
{


	if (gMIBupdateLCDscreen)
	{
		LCD.clear();

		switch(gWhichHelpMsg)
		{
			case 0:	DisplayHelpScreen(gHelpMessage0);	break;
			case 1:	DisplayHelpScreen(gHelpMessage1);	break;
			case 2:	DisplayHelpScreen(gHelpMessage2);	break;
			case 3:	DisplayHelpScreen(gHelpMessage3);	break;
			case 4:	DisplayHelpScreen(gHelpMessage4);	break;
		}
		gMIBupdateLCDscreen	=	false;
	}


	if (gMIBbuttonArray[kMIB_Button_Cancel].buttonIsDown && gMIBbuttonArray[kMIB_Button_Cancel].buttonHasChanged)
	{
	//*	CANCEL button
		MIB_SetToMainMenu();
		
		gWhichHelpMsg	=	0;	//*	this is so that it comes back to the 1st menu the next time
	}
	else if (gMIBbuttonArray[kMIB_Button_ZMinus].buttonIsDown && gMIBbuttonArray[kMIB_Button_ZMinus].buttonHasChanged)
	{
	//*	UP button
	
		gWhichHelpMsg++;
		gMIBupdateLCDscreen	=	true;
		//*	we want this to be a one time status
		gMIBbuttonArray[kMIB_Button_ZMinus].buttonHasChanged	=	false;

	}
	else if (gMIBbuttonArray[kMIB_Button_ZPlus].buttonIsDown && gMIBbuttonArray[kMIB_Button_ZPlus].buttonHasChanged)
	{
		gWhichHelpMsg--;
		gMIBupdateLCDscreen	=	true;

		//*	we want this to be a one time status
		gMIBbuttonArray[kMIB_Button_ZPlus].buttonHasChanged	=	false;

	}
	
	if (gWhichHelpMsg < 0)
	{
		gWhichHelpMsg	=	0;
		MIB_SysBeep(0);
	}
	else if (gWhichHelpMsg >= kHelpScreenCount)
	{
		gWhichHelpMsg	=	kHelpScreenCount - 1;
		MIB_SysBeep(0);
	}
}
