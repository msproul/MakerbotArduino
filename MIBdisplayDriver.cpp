//******************************************************************************************
//*	MakerBot Interface board driver code for Arduino
//*		(C) 2011 by Mark Sproul
//*		Open source as per standard Arduino code
//*
//******************************************************************************************
//*	theory of operation
//*
//*	The menu def array (TYPE_MenuDef) must be in PROGMEM memory
//*		We have LOTS of program space and not much RAM
//*
//*	MIB_CheckInputButtons
//*		MIB_CheckInputButtons  should be called at least every 10 millisecs
//*		It is ok to call it more often than that
//*		It takes care of checking the buttons up/down state and handles bounce.
//*		It uses an array of button defs that includes the state of the button and an 8bit byte
//*		for keeping track of the button state for debouncing
//*
//*	MIB_ProcessMenuButtons
//*		This take an array of menu definitions (TYPE_MenuDef *) and a count as how many
//*		are in the array, it takes care of scrolling thru the list and keeping track of 
//*		which one is selected. It will update the gMIBcurrentState when a new menu item is selected
//*
//*	MIB_DisplayMenu
//*		takes care of the actual display of the menus on the LCD.
//******************************************************************************************
//*	Edit History
//******************************************************************************************
//*	Oct  2, 2011	<MLS> Adding support for MakerBot Interface board (MIB)
//*	Nov 10,	2011	<MLS> Added MIB_SysBeep which uses tone() on the buzzer pin
//******************************************************************************************


#if (ARDUINO >= 100)
	#include <Arduino.h>
#else
	#include <WProgram.h>
#endif
#include	<HardwareSerial.h>
#include	<LiquidCrystal.h>

#if defined(__AVR__)
	#ifndef __PGMSPACE_H_
		#include	<avr/pgmspace.h>
	#endif
#endif

//*	Makerbot Gen 4 libraries
#include	"MakerbotMB2.h"
#include	"MakerbotMIB.h"
#include	"MIBdisplayDriver.h"


//*	menu handling stuff
short				gMIBcurrentState;
boolean				gMIBupdateMenu;
boolean				gMIBupdateLCDscreen;
boolean				gMIBmenuIsActive;
TYPE_ButtonState	gMIBbuttonArray[kMIB_MaxButtons];
short				gMIBcurrentMenuOffset;
short				gMIBcurrentMenuSelected;

//******************************************************************************************
// initialize the library with the numbers of the interface pins
//*	NOTE: This LCD requires a slight modification to the standard Arduino LCD library
//  int row_offsets[] = { 0x00, 0x40, 0x10, 0x50 };
//******************************************************************************************
LiquidCrystal LCD(	kMIB_LCD_rs_pin,
					kMIB_LCD_enable_pin,
					kMIB_LCD_d4_pin,
					kMIB_LCD_d5_pin,
					kMIB_LCD_d6_pin,
					kMIB_LCD_d7_pin);



//*	static stuff that is ONLY referenced by this file
static unsigned long	gLastButtonCheckTime;



//******************************************************************************************
void	MIB_Init(void)
{
short	ii;

	//*	init the menu handling stuff
	gMIBmenuIsActive		=	true;
	gMIBupdateMenu			=	true;
	gMIBcurrentMenuSelected	=	0;
	gLastButtonCheckTime	=	millis();
	gMIBcurrentMenuOffset	=	0;

	LCD.begin(16, 4);
#ifdef _SETROWOFFSET_SUPPORTED_	
	LCD.setRowOffsets(0x00, 0x40, 0x10, 0x50);
#else
	#warning make sure the LiquidCrystal.cpp is modified
#endif
	//*	make the interface board switch pins inputs
	pinMode(kMIB_Switch_Pin_OK,		INPUT);
	pinMode(kMIB_Switch_Pin_Cancel,	INPUT);
	pinMode(kMIB_Switch_Pin_XPlus,	INPUT);
	pinMode(kMIB_Switch_Pin_Xminus,	INPUT);
	pinMode(kMIB_Switch_Pin_YPlus,	INPUT);
	pinMode(kMIB_Switch_Pin_Yminus,	INPUT);
	pinMode(kMIB_Switch_Pin_ZPlus,	INPUT);
	pinMode(kMIB_Switch_Pin_Zminus,	INPUT);
	pinMode(kMIB_Switch_Pin_Zero,	INPUT);

#if defined(__AVR__)
	//*	if we are on AVR processor, turn on the internal pull up resistors
	digitalWrite(kMIB_Switch_Pin_OK,		HIGH);
	digitalWrite(kMIB_Switch_Pin_Cancel,	HIGH);
	digitalWrite(kMIB_Switch_Pin_XPlus,		HIGH);
	digitalWrite(kMIB_Switch_Pin_Xminus,	HIGH);
	digitalWrite(kMIB_Switch_Pin_YPlus,		HIGH);
	digitalWrite(kMIB_Switch_Pin_Yminus,	HIGH);
	digitalWrite(kMIB_Switch_Pin_ZPlus,		HIGH);
	digitalWrite(kMIB_Switch_Pin_Zminus,	HIGH);
	digitalWrite(kMIB_Switch_Pin_Zero,		HIGH);
#endif

	//*	make the interface board LED pins outputs
	pinMode(kMIB_LED_Foo,			OUTPUT);
	pinMode(kMIB_LED_Bar,			OUTPUT);
	pinMode(kMIB_LED_Debug,			OUTPUT);



	//*	turn the LEDS off
	digitalWrite(kMIB_LED_Foo,		LOW);
	digitalWrite(kMIB_LED_Bar,		LOW);
	digitalWrite(kMIB_LED_Debug,	LOW);


	//*	init the button array
	for (ii=0; ii<kMIB_MaxButtons; ii++)
	{
		gMIBbuttonArray[ii].buttonIsDown		=	false;
		gMIBbuttonArray[ii].buttonHasChanged	=	false;
		gMIBbuttonArray[ii].bitPattern			=	0xff;
	}

	gMIBbuttonArray[kMIB_Button_OK_Sel	].pinNumber	=	kMIB_Switch_Pin_OK;
	gMIBbuttonArray[kMIB_Button_Cancel	].pinNumber	=	kMIB_Switch_Pin_Cancel;
	gMIBbuttonArray[kMIB_Button_XPlus	].pinNumber	=	kMIB_Switch_Pin_XPlus;
	gMIBbuttonArray[kMIB_Button_XMinus	].pinNumber	=	kMIB_Switch_Pin_Xminus;
	gMIBbuttonArray[kMIB_Button_YPlus	].pinNumber	=	kMIB_Switch_Pin_YPlus;
	gMIBbuttonArray[kMIB_Button_YMinus	].pinNumber	=	kMIB_Switch_Pin_Yminus;
	gMIBbuttonArray[kMIB_Button_ZPlus	].pinNumber	=	kMIB_Switch_Pin_ZPlus;
	gMIBbuttonArray[kMIB_Button_ZMinus	].pinNumber	=	kMIB_Switch_Pin_Zminus;
	gMIBbuttonArray[kMIB_Button_Zero	].pinNumber	=	kMIB_Switch_Pin_Zero;

}

//******************************************************************************************
void	MIB_SetToMainMenu(void)
{
	gMIBcurrentState		=	0;
	gMIBmenuIsActive		=	true;
	gMIBupdateMenu			=	true;
	gMIBcurrentMenuOffset	=	0;
	gMIBcurrentMenuSelected	=	0;
	gMIBbuttonArray[kMIB_Button_OK_Sel].buttonHasChanged	=	false;
}

//******************************************************************************************
void	MIB_DisplayMenu(TYPE_MenuDef *theMenuDef, unsigned char firstDisplayedMenu, unsigned char currentSelectedItem)
{
short	ii;
char	stringBuff[32];
short	lcdRowNum;

	LCD.clear();
	LCD.noCursor();
	LCD.noBlink();

	lcdRowNum	=	0;
	ii			=	firstDisplayedMenu;
	while ((lcdRowNum < 4) && (pgm_read_byte_far(&theMenuDef[ii].menuStateValue) >= 0))
	{
		LCD.setCursor(0, lcdRowNum);
		lcdRowNum++;
		
		if (ii == currentSelectedItem)
		{
			strcpy(stringBuff, ">");
		}
		else
		{
			strcpy(stringBuff, " ");
		}
		strcat_P(stringBuff, theMenuDef[ii].menuName);
		LCD.print(stringBuff);
		
//		Serial.println(stringBuff);
		
		ii++;
	}
}

//******************************************************************************************
//*	this dumps the menu to the serial port for debuggin
short	MIB_CountMenuItems(TYPE_MenuDef *theMenuDef)
{
short	menuCount;
char	stringBuff[32];

	menuCount	=	0;
	while (1)
	{
		strcpy_P(stringBuff, theMenuDef[menuCount].menuName);
		if (stringBuff[0] == '-')
		{
			break;
		}
		menuCount++;
	}
	return(menuCount);
}


//******************************************************************************************
//*	this dumps the menu to the serial port for debuggin
void	MIB_DumpMenu(TYPE_MenuDef *theMenuDef, unsigned char menuCount)
{
short	ii;
char	stringBuff[32];
short	myMenuState;

	Serial.println();
	for (ii=0; ii<menuCount; ii++)
	{
		strcpy_P(stringBuff, theMenuDef[ii].menuName);
		while (strlen(stringBuff) < 16)
		{
			strcat(stringBuff, " ");
		}
		Serial.print(stringBuff);
		
		myMenuState	=	pgm_read_byte_far(&theMenuDef[ii].menuStateValue);
		Serial.print(myMenuState);


		Serial.println();
	}



	Serial.println();
}


//******************************************************************************************
//*	returns TRUE if menu needs to be updated
boolean	MIB_ProcessMenuButtons(TYPE_MenuDef *theMenuDef, short maxMenuCount)
{
boolean	updateMenu;

	updateMenu	=	false;
	
	if (gMIBbuttonArray[kMIB_Button_OK_Sel].buttonIsDown && gMIBbuttonArray[kMIB_Button_OK_Sel].buttonHasChanged)
	{
	//*	Select button

//-		Serial.println("Select button pressed");

		gMIBcurrentState	=	pgm_read_byte_far(&theMenuDef[gMIBcurrentMenuSelected].menuStateValue);
		gMIBmenuIsActive	=	false;
		LCD.clear();
		gMIBupdateLCDscreen	=	true;
		
		//*	setting buttonHasChanged to false will make this only hapen once for each button press
		gMIBbuttonArray[kMIB_Button_OK_Sel].buttonHasChanged	=	false;
		
	}
	else if (gMIBbuttonArray[kMIB_Button_Cancel].buttonIsDown && gMIBbuttonArray[kMIB_Button_Cancel].buttonHasChanged)
	{
	//*	cancel button
	
		gMIBcurrentMenuSelected	=	0;
		gMIBcurrentMenuOffset	=	0;
		updateMenu				=	true;
		gMIBcurrentState		=	0;
		
		//*	we want this to be a one time status
		gMIBbuttonArray[kMIB_Button_Cancel].buttonHasChanged	=	false;
	}
	else if (gMIBbuttonArray[kMIB_Button_ZMinus].buttonIsDown && gMIBbuttonArray[kMIB_Button_ZMinus].buttonHasChanged)
	{
		gMIBcurrentMenuSelected++;
		
		updateMenu	=	true;
		
		//*	we want this to be a one time status
		gMIBbuttonArray[kMIB_Button_ZMinus].buttonHasChanged	=	false;

	}
	else if (gMIBbuttonArray[kMIB_Button_ZPlus].buttonIsDown && gMIBbuttonArray[kMIB_Button_ZPlus].buttonHasChanged)
	{
		gMIBcurrentMenuSelected--;
		updateMenu	=	true;

		//*	we want this to be a one time status
		gMIBbuttonArray[kMIB_Button_ZPlus].buttonHasChanged	=	false;

	}

	if (gMIBcurrentMenuSelected < 0)
	{
		gMIBcurrentMenuSelected	=	0;
		MIB_SysBeep(0);
	}
	if (gMIBcurrentMenuSelected >= maxMenuCount)
	{
		gMIBcurrentMenuSelected	=	maxMenuCount - 1;
		MIB_SysBeep(0);
	}
	
	if ((gMIBcurrentMenuSelected - gMIBcurrentMenuOffset) >= 4)
	{
		gMIBcurrentMenuOffset++;
		updateMenu	=	true;
	}
	if (gMIBcurrentMenuSelected < gMIBcurrentMenuOffset)
	{
		gMIBcurrentMenuOffset--;
		updateMenu	=	true;
	}
	if (gMIBcurrentMenuOffset < 0)
	{
		gMIBcurrentMenuOffset	=	0;
		updateMenu	=	true;
	}

	return(updateMenu);
}
			




//******************************************************************************************
//*	this function can be called as often as you want but at least every 5 milliseconds.
//*	It will only execute once ever 5 milliseconds.
//*	it is NOT designed to be called from an interrupt routine
//******************************************************************************************
void	MIB_CheckInputButtons(void)
{
unsigned long	currentMillis;
short			ii;
short			pinNumber;
short			pinValue;
boolean			previousButtonState;

	if ((millis() - gLastButtonCheckTime) > 5)
	{
		gLastButtonCheckTime	=	millis();

		for (ii=0; ii<kMIB_MaxButtons; ii++)
		{
			previousButtonState	=	gMIBbuttonArray[ii].buttonIsDown;

			pinNumber			=	gMIBbuttonArray[ii].pinNumber;
			pinValue			=	digitalRead(pinNumber);
			
			//*	shift the bit array to the left one and add the new bit
			gMIBbuttonArray[ii].bitPattern	=	(gMIBbuttonArray[ii].bitPattern << 1) + pinValue;
			
			if (gMIBbuttonArray[ii].bitPattern == 0xff)
			{
				//*	all ones and its UP
				gMIBbuttonArray[ii].buttonIsDown	=	false;
			}
			else if (gMIBbuttonArray[ii].bitPattern == 0x00)
			{
				//*	all zeros and its DOWN
				gMIBbuttonArray[ii].buttonIsDown	=	true;
			}
			//else dont change anything
			
			if (gMIBbuttonArray[ii].buttonIsDown != previousButtonState)
			{
				gMIBbuttonArray[ii].buttonHasChanged	=	true;
			}
		}
	}
}


//******************************************************************************************
boolean	MIB_IsButtonDown(short whichButton)
{
	MIB_CheckInputButtons();
	if ((whichButton >= 0) && (whichButton < kMIB_MaxButtons))
	{
		return(gMIBbuttonArray[whichButton].buttonIsDown);
	}
	else
	{
		return(false);
	}
}

//******************************************************************************************
boolean	MIB_IsCancelDown(void)
{
	MIB_CheckInputButtons();
	return(gMIBbuttonArray[kMIB_Button_Cancel].buttonIsDown);
}

//******************************************************************************************
boolean	MIB_IsOKDown(void)
{
	MIB_CheckInputButtons();
	return(gMIBbuttonArray[kMIB_Button_OK_Sel].buttonIsDown);
}



//******************************************************************************************
void	MIB_SysBeep(unsigned char whichTone)
{
	switch(whichTone)
	{
		case 0:
			tone(kMB2_Buzzer, 600, 75);
			break;
		
		case 1:
			tone(kMB2_Buzzer, 500, 500);
			break;
		
		default:
			tone(kMB2_Buzzer, 600, 500);
			break;
	}
}
