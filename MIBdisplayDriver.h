//******************************************************************************************
//*	Makerbot Interface Board drivers
//*		(C) 2011 by Mark Sproul
//*		Open source as per standard Arduino code
//*
//******************************************************************************************
//*	This is set up to use MakerBot gen 4 hardware for stepper motor control and interface
//*
//*	NOTE: This LCD requires a slight modification to the standard Arduino LCD library
//  int row_offsets[] = { 0x00, 0x40, 0x10, 0x50 };
//******************************************************************************************
//*	Edit History
//******************************************************************************************
//*	Oct  2, 2011	<MLS> Adding support for MakerBot Interface board (MIB)
//*	Nov 10,	2011	<MLS> Added MIB_SysBeep which uses tone() on the buzzer pin
//******************************************************************************************
//#include	"MIBdisplayDriver.h"


//******************************************************************************************
//*	index of buttons into button arrary, this allows the library to handle the buttons including
//*	bounce
enum 
{
	kMIB_Button_OK_Sel	=	0,
	kMIB_Button_Cancel,
	kMIB_Button_XPlus,
	kMIB_Button_XMinus,
	kMIB_Button_YPlus,
	kMIB_Button_YMinus,
	kMIB_Button_ZPlus,
	kMIB_Button_ZMinus,
	kMIB_Button_Zero
};
#define	kMIB_MaxButtons	9



//******************************************************************************************
typedef struct	PROGMEM
	{
		signed char		menuStateValue;
		char			menuName[16];		
		
	} TYPE_MenuDef;

//******************************************************************************************
typedef struct	
	{
		unsigned char	pinNumber;
		unsigned char	bitPattern;
		boolean			buttonHasChanged;
		boolean			buttonIsDown;
	} TYPE_ButtonState;

extern	TYPE_ButtonState	gMIBbuttonArray[];
extern	short				gMIBcurrentState;
extern	boolean				gMIBupdateMenu;
extern	boolean				gMIBmenuIsActive;
extern	boolean				gMIBupdateLCDscreen;
extern	short				gMIBcurrentMenuOffset;
extern	short				gMIBcurrentMenuSelected;


#ifdef __cplusplus
extern "C"{
#endif

void	MIB_Init(void);
void	MIB_SetToMainMenu(void);
void	MIB_DisplayMenu(TYPE_MenuDef *theMenuDef, unsigned char firstDisplayedMenu, unsigned char currentSelectedItem);
void	MIB_DumpMenu(TYPE_MenuDef *theMenuDef, unsigned char menuCount);	//*	for debugging
short	MIB_CountMenuItems(TYPE_MenuDef *theMenuDef);
boolean	MIB_ProcessMenuButtons(TYPE_MenuDef *theMenuDef, short maxMenuCount);
void	MIB_CheckInputButtons(void);
boolean	MIB_IsButtonDown(short whichButton);
boolean	MIB_IsCancelDown(void);
boolean	MIB_IsOKDown(void);
void	MIB_SysBeep(unsigned char whichTone);


#ifdef __cplusplus
} // extern "C"
#endif
