//******************************************************************************************
//*	Makerbot Interface Board V1.1 pin definitions
//*		Open source as per standard Arduino code
//*
//*		Thse defines the Makerbot Interface board pins for Arduino Mega
//*		http://wiki.makerbot.com/mib1
//*		http://www.makerbot.com/docs/makerbot-gen4-interface-kit/
//******************************************************************************************
//*	Edit History
//******************************************************************************************
//*	Oct  5, 2011	Created file to define MIB pin numbers
//******************************************************************************************
//#include	"MakerbotMIB.h"


//*	note, "k" stands for constant, this is the convention I was taught and use
//******************************************************************************************
//*	MakerBot Interface Board v1.1 pin definitions
//*	for Arduino mega
//*	
//*	
//*	      Y+                  Z+
//*	X-   Zero   X+   Cancel   OK
//*	      Y-                  Z-
//*	

#define	kMIB_Switch_Pin_OK			35	
#define	kMIB_Switch_Pin_Cancel		36	//*	aka BACK
#define	kMIB_Switch_Pin_XPlus		42
#define	kMIB_Switch_Pin_Xminus		43
#define	kMIB_Switch_Pin_YPlus		44
#define	kMIB_Switch_Pin_Yminus		45
#define	kMIB_Switch_Pin_ZPlus		46
#define	kMIB_Switch_Pin_Zminus		47
#define	kMIB_Switch_Pin_Zero		48	//	aka MODE


//*	these are LED outputs
#define	kMIB_LED_Foo				37
#define	kMIB_LED_Bar				49
#define	kMIB_LED_Debug				13


//*	LCD control pins
#define	kMIB_LCD_rs_pin				33
#define	kMIB_LCD_enable_pin			34
#define	kMIB_LCD_d4_pin				38
#define	kMIB_LCD_d5_pin				39
#define	kMIB_LCD_d6_pin				40
#define	kMIB_LCD_d7_pin				41




//******************************************************************************************
// initialize the library with the numbers of the interface pins
//*	NOTE: This LCD requires a slight modification to the standard Arduino LCD library
//  int row_offsets[] = { 0x00, 0x40, 0x10, 0x50 };
//******************************************************************************************
//*	Proper call for LCD library
//	LiquidCrystal LCD(	kMIB_LCD_rs_pin,
//						kMIB_LCD_enable_pin,
//						kMIB_LCD_d4_pin,
//						kMIB_LCD_d5_pin,
//						kMIB_LCD_d6_pin,
//						kMIB_LCD_d7_pin);
//******************************************************************************************
