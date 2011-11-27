//******************************************************************************************
//*	Makerbot Motherboard V2.4 pin definitions
//*		Open source as per standard Arduino code
//*
//*		Thse defines the Makerbot Interface board pins for Arduino Mega
//*		http://wiki.makerbot.com/mb2
//******************************************************************************************
//*	Edit History
//******************************************************************************************
//*	Oct  5, 2011	Created file to define MB2.4 pin numbers
//******************************************************************************************
//#include	"MakerbotMB2.h"


//******************************************************************************************
//*	note, "k" stands for constant, this is the convention I was taught and use
//******************************************************************************************


//*	stepper motor connector
//*		pin 1	N/C
//*		pin 2	GND
//*		pin 3	STEP
//*		pin 4	DIR
//*		pin 5	ENABLE
//*		pin 6	N/C
//*	Arduino pin numbers for Arduino Mega
#define	kMB2_XStepper_Enable		26		//*	mega	=	PA4
#define	kMB2_XStepper_Direction		27		//*	mega	=	PA5
#define	kMB2_XStepper_Step			28		//*	mega	=	PA6		0x40


#define	kMB2_YStepper_Enable		23
#define	kMB2_YStepper_Direction		24
#define	kMB2_YStepper_Step			25		//*	mega	=	PA3		0x08

#define	kMB2_ZStepper_Enable		16
#define	kMB2_ZStepper_Direction		17
#define	kMB2_ZStepper_Step			22		//*	mega	=	PA0		0x01

#define	kMB2_AStepper_Enable		3
#define	kMB2_AStepper_Direction		14
#define	kMB2_AStepper_Step			15		//*	mega	=	PJ0	

#define	kMB2_BStepper_Enable		6
#define	kMB2_BStepper_Direction		5
#define	kMB2_BStepper_Step			4


//*	Limit switch pin definitions (inputs)
#define	kMB2_Limit_EStop			2	
#define	kMB2_Limit_XMin				12
#define	kMB2_Limit_XMax				11
#define	kMB2_Limit_YMin				10
#define	kMB2_Limit_YMax				9
#define	kMB2_Limit_ZMin				8
#define	kMB2_Limit_ZMax				7

#define	kMB2_ATXpowerSupplyEnable	29
#define	kMB2_Buzzer					31


//******************************************************************************************
//*
//*	Pin definitions
// On the Ethernet Shield, CS is pin 4. It's set as an output by default.
// Note that even if it's not used as the CS pin, the hardware SS pin 
// (10 on most Arduino boards, 53 on the Mega) must be left as an output 
// or the SD library functions will not work. 
//******************************************************************************************
#define	kMB2_SDcardChipSelect	53



//******************************************************************************************
//*	Pin definitions in Pin # order
//*		
//*		2	kMB2_Limit_EStop
//*		3	kMB2_AStepper_Enable
//*		4	kMB2_BStepper_Step
//*		5	kMB2_BStepper_Direction
//*		6	kMB2_BStepper_Enable
//*		7	kMB2_Limit_ZMax
//*		8	kMB2_Limit_ZMin
//*		9	kMB2_Limit_YMax
//*		10	kMB2_Limit_YMin
//*		11	kMB2_Limit_XMax
//*		12	kMB2_Limit_XMin
//*		13	kMIB_LED_Debug
//*		14	kMB2_AStepper_Direction
//*		15	kMB2_AStepper_Step
//*		16	kMB2_ZStepper_Enable
//*		17	kMB2_ZStepper_Direction
//*		18	rs485	(TX1)
//*		19	rs485	(RX1)
//*		20	used with SD card
//*		21	used with SD card
//*		22	kMB2_ZStepper_Step
//*		23	kMB2_YStepper_Enable
//*		24	kMB2_YStepper_Direction
//*		25	kMB2_YStepper_Step
//*		26	kMB2_XStepper_Enable
//*		27	kMB2_XStepper_Direction
//*		28	kMB2_XStepper_Step
//*		29	kMB2_ATXpowerSupplyEnable		connected to ATX power supply pin 14
//*		30	rs485
//*		31	kMB2_Buzzer
//*		32	rs485
//*		33	kMIB_LCD_rs_pin
//*		34	kMIB_LCD_enable_pin
//*		35	kMIB_Switch_Pin_OK
//*		36	kMIB_Switch_Pin_Cancel
//*		37	kMIB_LED_Foo
//*		38	kMIB_LCD_d4_pin
//*		39	kMIB_LCD_d5_pin
//*		40	kMIB_LCD_d6_pin
//*		41	kMIB_LCD_d7_pin
//*		42	kMIB_Switch_Pin_XPlus
//*		43	kMIB_Switch_Pin_Xminus
//*		44	kMIB_Switch_Pin_YPlus
//*		45	kMIB_Switch_Pin_Yminus
//*		46	kMIB_Switch_Pin_ZPlus
//*		47	kMIB_Switch_Pin_Zminus
//*		48	kMIB_Switch_Pin_Zero
//*		49	kMIB_LED_Bar
//*		50	MISO used with SD card
//*		51	MOSI used with SD card
//*		52	SCK used with SD card
//*		53	kMB2_SDcardChipSelect used with SD card
//******************************************************************************************













