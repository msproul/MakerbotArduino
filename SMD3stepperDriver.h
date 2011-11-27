//******************************************************************************************
//*	Makerbot SMD3 stepper motor control software for use with Arduino
//*		(C) 2011 by Mark Sproul
//*		Open source as per standard Arduino code
//*
//*	http://wiki.makerbot.com/smd3
//******************************************************************************************
//*	This is set up to use MakerBot gen 4 hardware for stepper motor control and interface
//******************************************************************************************
//*	http://wiki.makerbot.com/smd3
//******************************************************************************************
//*	Edit History
//******************************************************************************************
//*	Oct  2, 2011	<MLS> Started working on library for MakerBot Interface board (MIB)
//******************************************************************************************
//#include	"SMD3stepperDriver.h"


#define	kStepperReverse	0
#define	kStepperForward	1

//#define	_ENBABLE_STEPPER_A_
#define	_ENBABLE_STEPPER_B_

enum 
{
	kMB2_StepperX	=	0,
	kMB2_StepperY,
	kMB2_StepperZ,
#ifdef _ENBABLE_STEPPER_A_
	kMB2_StepperA,
#endif
#ifdef _ENBABLE_STEPPER_B_
	kMB2_StepperB,			//*	we arent using stepper B, so conserve memory
#endif
	kMB2_MaxSteppers
};



#ifdef __cplusplus
extern "C"{
#endif

void	MB2_InitAllSteppers(void);
void	MB2_DisableAllSteppers(void);
long	MB2_GetCurrentStepperLocation(short stepperNumber);
long	MB2_GetCurrentStepperDelta(short stepperNumber);
void	MB2_SetCurrentStepperLocation(short stepperNumber, long stepCount);
void	MB2_StepperLineToXY(long xLocation, long yLocation);

void	MB2_StepperMoveToLocation(short stepperNumber, long stepLocation);
void	MB2_CancelMove(short stepperNumber);
void	MB2_CancelAllMovement(void);
int		MB2_GetEndStopMin(short stepperNumber);
int		MB2_GetEndStopMax(short stepperNumber);
boolean	MB2_StepperIsActive(void);
boolean	MB2_AxisIsActive(short stepperNumber);
boolean	MB2_CalibrateAxis(short stepperNumber, short lcdRowNum, const char *axisName);

void			StepperInterrupt_Init(void);
unsigned long	StepperInterrupt_GetIntCounter(void);
void			StepperInterrupt_ZeroIntCounter(void);
void			StepperInterrupt_EnableInterrupts(void);



#ifdef __cplusplus
} // extern "C"
#endif
