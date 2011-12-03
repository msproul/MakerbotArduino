//******************************************************************************************
//*	Makerbot SMD3 stepper motor control software for use with Arduino
//*		(C) 2011 by Mark Sproul
//*		Open source as per standard Arduino code
//*
//*	http://wiki.makerbot.com/smd3
//******************************************************************************************
//*	This is set up to use MakerBot gen 4 hardware for stepper motor control and interface
//******************************************************************************************
//*	Edit History
//******************************************************************************************
//*	Oct  2, 2011	<MLS> Started on stepper drivers for Makerbot MB2
//*	Oct 12, 2011	<MLS> Added MB2_GetCurrentStepperLocation
//*	Oct 28, 2011	<MLS> Started on Interrupt code
//*	Nov 12,	2011	<MLS> Adding linear mode for straight line movement
//*	Nov 17,	2011	<MLS> Added MB2_CalibrateAxis
//*	Nov 22,	2011	<MLS> Added ability to flip axis direction
//*	Nov 26,	2011	<MLS> Added MB2_CancelAllMovement
//*	Nov 28,	2011	<MLS> Linear movement working in all directions
//*	Dec  2,	2011	<MLS> Added MB2_EnableAllSteppers
//******************************************************************************************
/*
With direct IO
Going forward
  Test 1 -stepper motors idle  loop counter =50342
  Test 2 -X moving             loop counter =50125
  Test 3 -X and Y moving       loop counter =49766
  Test 4 -X and Y and Z moving loop counter =49409

*/
/*
Going forward
  Test 1 -stepper motors idle  loop counter =50339
  Test 2 -X moving             loop counter =49478
  Test 3 -X and Y moving       loop counter =48438
  Test 4 -X and Y and Z moving loop counter =47375

*/



#if (ARDUINO >= 100)
	#include <Arduino.h>
#else
	#include <WProgram.h>
#endif
#include	"pins_arduino.h"

#include	"MakerbotMB2.h"
#include	"MakerbotMIB.h"
#include	"MIBdisplayDriver.h"	//*	MIB = MakerBot Interface board
#include	"LCDdisplayUtils.h"

//#define _USE_ACCELERATION_

#include	"SMD3stepperDriver.h"

#if defined(__AVR__)
	#define	_USE_INTERRUPTS_FOR_STEPPER_
	#define	_USE_DIRECT_PORT_IO_

	#define ENABLE_STEPPER_DRIVER_INTERRUPT() TIMSK1 |= (1<<OCIE1A)
	#define DISABLE_STEPPER_DRIVER_INTERRUPT() TIMSK1 &= ~(1<<OCIE1A)
#endif

#if defined(__PIC32MX__)
	#define ENABLE_STEPPER_DRIVER_INTERRUPT()
	#define DISABLE_STEPPER_DRIVER_INTERRUPT()
#endif



//															 12345679 123456
static	prog_char	gCalibrateString[]			PROGMEM	=	"Calibrate";
static	prog_char	gTextNoEndstops1[]			PROGMEM	=	"ERROR Endstops";
static	prog_char	gTextNoEndstops2[]			PROGMEM	=	"Not defined";
//static	prog_char	gTextFatalInvalidPin[]		PROGMEM	=	"FATAL: Invalid pin number";


//******************************************************************************************
typedef struct	
	{
		boolean				isActive;
		boolean				currentStepPinState;
		short				currentDirection;
		long				currentLocation;	//*	steps from home (zero)
		long				desiredLocation;	//*	steps from home (zero)

		short				enablePin;		
		short				directionPin;
		short				stepPin;
		boolean				hasEndStopMin;
		boolean				hasEndStopMax;
		short				endStopMinPin;
		short				endStopMaxPin;

		//*	this is for linear line drawing
		boolean				isLinear;
		boolean				isPrimary;		//*	means this axis is the step axis, larger delta
		long				delta;
		long				accumulatedError;
		
		short				forwardDirValue;		//*	1 for normal, 0 for reversed
		short				reverseDirValue;		//*	0 for normal, 1 for reversed
		
	#ifdef _USE_ACCELERATION_
		//*	these are for variable speed and acceleration
		boolean				useAcceleration;
		short				irqCounter;			//*	this is used to count down to when to do the next step
		short				currentIrqCount;			//*	this is used to count down to when to do the next step
		short				minIrqCount;
		short				initialIrqCount;
		short				stepsAtThisRate;
	#endif

	#if defined(_USE_DIRECT_PORT_IO_) && defined(__AVR__)
		//*	this allows to bypass digitalWrite delays but not loose compatibilty
		uint8_t				enablePin_bitMask;
		volatile uint8_t	*enablePin_out;
		
		uint8_t				stepPin_bitMask;
		volatile uint8_t	*stepPin_out;
		
		uint8_t				endStopMinPin_bitMask;
		volatile uint8_t	*endStopMinPin_inReg;

		uint8_t				endStopMaxPin_bitMask;
		volatile uint8_t	*endStopMaxPin_inReg;
	#elif defined(_USE_DIRECT_PORT_IO_) && defined(__PIC32MX__)
	
	#endif
	} TYPE_Stepper;


static	TYPE_Stepper	gStepper[kMB2_MaxSteppers];


static void	StepNstepsDef(TYPE_Stepper *theStepperDef, short direction, short count);
static void	SMD3stepperInit(TYPE_Stepper *theStepperDef, short enablePin, short directionPin, short stepPin, short endStopMinPin, short endStopMaxPin);


//******************************************************************************************
//*	public routines
//******************************************************************************************

//******************************************************************************************
void	MB2_InitAllSteppers(void)
{
short	ii;

	//*	clear the def structures to all zeros
	for (ii=0; ii<kMB2_MaxSteppers; ii++)
	{
		memset(&gStepper[ii], 0, sizeof(TYPE_Stepper));

		gStepper[ii].isActive				=	false;
		gStepper[ii].isLinear				=	false;
		gStepper[ii].currentStepPinState	=	LOW;
		gStepper[ii].hasEndStopMin			=	false;
		gStepper[ii].hasEndStopMax			=	false;
		gStepper[ii].forwardDirValue		=	kStepperForward;
		gStepper[ii].reverseDirValue		=	kStepperReverse;


	#ifdef _USE_ACCELERATION_
		gStepper[ii].useAcceleration		=	false;
	#endif

	}
	SMD3stepperInit(&gStepper[kMB2_StepperX], kMB2_XStepper_Enable, kMB2_XStepper_Direction, kMB2_XStepper_Step, kMB2_Limit_XMin, kMB2_Limit_XMax);
	SMD3stepperInit(&gStepper[kMB2_StepperY], kMB2_YStepper_Enable, kMB2_YStepper_Direction, kMB2_YStepper_Step, kMB2_Limit_YMin, kMB2_Limit_YMax);
	SMD3stepperInit(&gStepper[kMB2_StepperZ], kMB2_ZStepper_Enable, kMB2_ZStepper_Direction, kMB2_ZStepper_Step, kMB2_Limit_ZMin, kMB2_Limit_ZMax);
#ifdef _ENBABLE_STEPPER_A_
	SMD3stepperInit(&gStepper[kMB2_StepperA], kMB2_AStepper_Enable, kMB2_AStepper_Direction, kMB2_AStepper_Step, -1,  -1);
#endif
#ifdef _ENBABLE_STEPPER_B_
	SMD3stepperInit(&gStepper[kMB2_StepperB], kMB2_BStepper_Enable, kMB2_BStepper_Direction, kMB2_BStepper_Step,  -1,  -1);
#endif

	gStepper[kMB2_StepperX].forwardDirValue		=	kStepperReverse;
	gStepper[kMB2_StepperX].reverseDirValue		=	kStepperForward;

	//*	init the end stop switches
	pinMode(kMB2_Limit_XMin,			INPUT);
	pinMode(kMB2_Limit_XMax,			INPUT);
	pinMode(kMB2_Limit_YMin,			INPUT);
	pinMode(kMB2_Limit_YMax,			INPUT);
	pinMode(kMB2_Limit_EStop,			INPUT);
	pinMode(kMB2_Limit_ZMin,			INPUT);
	pinMode(kMB2_Limit_ZMax,			INPUT);
	pinMode(kMB2_Limit_EStop,			INPUT);

#if defined(__AVR__)
	//*	if we are on an AVR processor, turn on the internal pull up resistors
	digitalWrite(kMB2_Limit_XMin,		HIGH);
	digitalWrite(kMB2_Limit_XMax,		HIGH);
	digitalWrite(kMB2_Limit_YMin,		HIGH);
	digitalWrite(kMB2_Limit_YMax,		HIGH);
	digitalWrite(kMB2_Limit_EStop,		HIGH);
	digitalWrite(kMB2_Limit_ZMin,		HIGH);
	digitalWrite(kMB2_Limit_ZMax,		HIGH);
	digitalWrite(kMB2_Limit_EStop,		HIGH);
#endif



}

//******************************************************************************************
void	MB2_EnableAllSteppers(void)
{
int	ii;

	for (ii=0; ii<kMB2_MaxSteppers; ii++)
	{
		digitalWrite(gStepper[ii].enablePin,		LOW);	//*	enable the motor
	}
	
}


//******************************************************************************************
void	MB2_DisableAllSteppers(void)
{
int	ii;

	for (ii=0; ii<kMB2_MaxSteppers; ii++)
	{
		digitalWrite(gStepper[ii].enablePin,		HIGH);	//*	disable the motor
	}
	
}



//******************************************************************************************
long	MB2_GetCurrentStepperLocation(short stepperNumber)
{
long	currentLoc;

	if ((stepperNumber >= 0) && (stepperNumber < kMB2_MaxSteppers))
	{
		currentLoc	=	gStepper[stepperNumber].currentLocation;
	}
	else
	{
		currentLoc	=	0;
	}
	return(currentLoc);
}


//******************************************************************************************
//*	returns the number of steps to go
long	MB2_GetCurrentStepperDelta(short stepperNumber)
{
long	deltaSteps;

	if ((stepperNumber >= 0) && (stepperNumber < kMB2_MaxSteppers))
	{
		deltaSteps	=	abs(gStepper[stepperNumber].desiredLocation - gStepper[stepperNumber].currentLocation);
	}
	else
	{
		deltaSteps	=	0;
	}
	return(deltaSteps);
}

//******************************************************************************************
void	MB2_SetCurrentStepperLocation(short stepperNumber, long stepLocation)
{

	if ((stepperNumber >= 0) && (stepperNumber < kMB2_MaxSteppers))
	{
		gStepper[stepperNumber].currentLocation	=	stepLocation;
		gStepper[stepperNumber].desiredLocation	=	stepLocation;
	}
}

//******************************************************************************************
//*	this sets up the stuff for the interrupt move code
void	MB2_StepperMoveToLocation(short stepperNumber, long stepLocation)
{
int	forwardDirState;
int	reverseDirState;


	if ((stepperNumber >= 0) && (stepperNumber < kMB2_MaxSteppers))
	{
		forwardDirState	=	gStepper[stepperNumber].forwardDirValue;
		reverseDirState	=	gStepper[stepperNumber].reverseDirValue;
		
		
	#ifdef _USE_INTERRUPTS_FOR_STEPPER_
		uint8_t oldSREG	=	SREG;
	    cli();	//*	disable interrupts

		gStepper[stepperNumber].desiredLocation	=	stepLocation;
		if (gStepper[stepperNumber].desiredLocation != gStepper[stepperNumber].currentLocation)
		{

			gStepper[stepperNumber].isActive	=	true;
			gStepper[stepperNumber].isLinear	=	false;
			if (gStepper[stepperNumber].desiredLocation < gStepper[stepperNumber].currentLocation)
			{
				gStepper[stepperNumber].currentDirection	=	kStepperReverse;
				digitalWrite(gStepper[stepperNumber].directionPin, reverseDirState);	//*	set the directiom
			}
			else
			{
				gStepper[stepperNumber].currentDirection	=	kStepperForward;
				digitalWrite(gStepper[stepperNumber].directionPin, forwardDirState);	//*	set the directiom
			}
			digitalWrite(gStepper[stepperNumber].enablePin, LOW);	//*	enable the motor


			ENABLE_STEPPER_DRIVER_INTERRUPT();

		}
		
		SREG	=	oldSREG;	//*	put the interrupt state back the way it was
	#else
		short	direction;
		long	stepCount;
		if (stepLocation < gStepper[stepperNumber].currentLocation)
		{
			direction	=	kStepperReverse;
			stepCount	=	gStepper[stepperNumber].currentLocation - stepLocation;
		}
		else
		{
			direction	=	kStepperForward;
			stepCount	=	stepLocation - gStepper[stepperNumber].currentLocation;
		}
//++		StepNstepsDef(&gStepper[stepperNumber], direction, stepCount);
	#endif
	}
}

//******************************************************************************************
//*	this sets up the stuff for the interrupt move code
//*	this sets up a LINEAR move
void	MB2_StepperLineToXY(long xLocation, long yLocation)
{
long	deltaX, deltaY;
int		forwardDirState;
int		reverseDirState;

#if defined(__AVR__)
uint8_t oldSREG;

	oldSREG	=	SREG;

    cli();	//*	disable interrupts
#endif

	gStepper[kMB2_StepperX].desiredLocation		=	xLocation;
	gStepper[kMB2_StepperY].desiredLocation		=	yLocation;

	
	if (gStepper[kMB2_StepperX].desiredLocation != gStepper[kMB2_StepperX].currentLocation)
	{
		forwardDirState	=	gStepper[kMB2_StepperX].forwardDirValue;
		reverseDirState	=	gStepper[kMB2_StepperX].reverseDirValue;

		gStepper[kMB2_StepperX].isActive	=	true;
		if (gStepper[kMB2_StepperX].desiredLocation < gStepper[kMB2_StepperX].currentLocation)
		{
			gStepper[kMB2_StepperX].currentDirection	=	kStepperReverse;
			digitalWrite(gStepper[kMB2_StepperX].directionPin, reverseDirState);	//*	set the directiom
		}
		else
		{
			gStepper[kMB2_StepperX].currentDirection	=	kStepperForward;
			digitalWrite(gStepper[kMB2_StepperX].directionPin, forwardDirState);	//*	set the directiom
		}
		digitalWrite(gStepper[kMB2_StepperX].enablePin, LOW);	//*	enable the motor
	}
	

	if (gStepper[kMB2_StepperY].desiredLocation != gStepper[kMB2_StepperY].currentLocation)
	{
		forwardDirState	=	gStepper[kMB2_StepperY].forwardDirValue;
		reverseDirState	=	gStepper[kMB2_StepperY].reverseDirValue;

		gStepper[kMB2_StepperY].isActive	=	true;
		if (gStepper[kMB2_StepperY].desiredLocation < gStepper[kMB2_StepperY].currentLocation)
		{
			gStepper[kMB2_StepperY].currentDirection	=	kStepperReverse;
			digitalWrite(gStepper[kMB2_StepperY].directionPin, reverseDirState);	//*	set the directiom
		}
		else
		{
			gStepper[kMB2_StepperY].currentDirection	=	kStepperForward;
			digitalWrite(gStepper[kMB2_StepperY].directionPin, forwardDirState);	//*	set the directiom
		}
		digitalWrite(gStepper[kMB2_StepperY].enablePin, LOW);	//*	enable the motor
	}

	deltaX	=	abs(gStepper[kMB2_StepperX].desiredLocation - gStepper[kMB2_StepperX].currentLocation);
	deltaY	=	abs(gStepper[kMB2_StepperY].desiredLocation - gStepper[kMB2_StepperY].currentLocation);

	gStepper[kMB2_StepperX].delta				=	deltaX;
	gStepper[kMB2_StepperY].delta				=	deltaY;

	gStepper[kMB2_StepperX].accumulatedError	=	0;
	gStepper[kMB2_StepperY].accumulatedError	=	0;

	//*	we only have to worry about it if both deltas are none zero
	//*	also, we dont have to worry about direction since that was already set above, we can assume positive (i.e. left to right, bottom to top)
	if ((deltaX != 0) && (deltaY != 0))
	{
		gStepper[kMB2_StepperX].isLinear	=	true;
		gStepper[kMB2_StepperY].isLinear	=	true;

		if (deltaX >= deltaY)
		{
			gStepper[kMB2_StepperX].isPrimary	=	true;
			gStepper[kMB2_StepperY].isPrimary	=	false;
		}
		else
		{
			gStepper[kMB2_StepperX].isPrimary	=	false;
			gStepper[kMB2_StepperY].isPrimary	=	true;
		}
	}

	ENABLE_STEPPER_DRIVER_INTERRUPT();
#if defined(__AVR__)
	SREG	=	oldSREG;	//*	put the interrupt state back the way it was
#endif
}


//******************************************************************************************
//*	Cancel Move, set the DESIRED location to be the same as the CURENT location
//******************************************************************************************
void	MB2_CancelMove(short stepperNumber)
{
	if ((stepperNumber >= 0) && (stepperNumber < kMB2_MaxSteppers))
	{
	#ifdef _USE_INTERRUPTS_FOR_STEPPER_
		uint8_t oldSREG	=	SREG;
	    cli();	//*	disable interrupts
	#endif


		gStepper[stepperNumber].desiredLocation	=	gStepper[stepperNumber].currentLocation;
		gStepper[stepperNumber].isActive		=	false;
		gStepper[stepperNumber].isLinear		=	false;
		

	#ifdef _USE_INTERRUPTS_FOR_STEPPER_
		SREG	=	oldSREG;	//*	put the interrupt state back the way it was
	#endif
	}
}

//******************************************************************************************
void	MB2_CancelAllMovement(void)
{
int		ii;
		for (ii=0; ii<kMB2_MaxSteppers; ii++)
		{
			MB2_CancelMove(ii);
		}
		MB2_DisableAllSteppers();
}

//******************************************************************************************
//*	Returns true if any stepper is active
boolean	MB2_StepperIsActive(void)
{
boolean	isActive;
short	ii;

	isActive	=	false;
	for (ii=0; ii<kMB2_MaxSteppers; ii++)
	{
		if (gStepper[ii].isActive)
		{
			isActive	=	true;
			break;
		}
	}
	
	if (isActive)
	{
		digitalWrite(kMIB_LED_Bar, HIGH);
	}
	else
	{
		//*	if nothing is going on, disable the interrupts
		DISABLE_STEPPER_DRIVER_INTERRUPT();
		digitalWrite(kMIB_LED_Bar, LOW);
	}
	return(isActive);
}

//******************************************************************************************
//*	Returns true if specified stepper is active
boolean	MB2_AxisIsActive(short stepperNumber)
{
	return(gStepper[stepperNumber].isActive);
}


//******************************************************************************************
//*	Returns state (HIGH/LOW) of endstop
int	MB2_GetEndStopMin(short stepperNumber)
{
#ifdef _USE_DIRECT_PORT_IO_
	if (*gStepper[stepperNumber].endStopMinPin_inReg & gStepper[stepperNumber].endStopMinPin_bitMask)
	{
		return HIGH;
	}
	return LOW;
#else
	return(digitalRead(gStepper[stepperNumber].endStopMinPin));
#endif
}

//******************************************************************************************
//*	Returns state (HIGH/LOW) of endstop
int	MB2_GetEndStopMax(short stepperNumber)
{
#ifdef _USE_DIRECT_PORT_IO_
	if (*gStepper[stepperNumber].endStopMaxPin_inReg & gStepper[stepperNumber].endStopMaxPin_bitMask)
	{
		return HIGH;
	}
	return LOW;
#else
	return(digitalRead(gStepper[stepperNumber].endStopMaxPin));
#endif
}


//******************************************************************************************
//*	CalibrateAxis will move the stepper to "left" until the minimum is hit 
//*		and then to the "right" until max is hit
//*		lcdRowNum and axisName are for LCD display
//*		axisName is program Memory
//******************************************************************************************
boolean	MB2_CalibrateAxis(short stepperNumber, short lcdRowNum, const char *axisName)
{
long	loopCounter;
boolean	minReached;
boolean	maxReached;
boolean	keepGoing;
long	currentLocation;
long	desiredLocation;
long	deltaSteps;
short	myMinStopPin;
short	myMaxStopPin;


	minReached	=	false;
	maxReached	=	false;
	keepGoing	=	true;
	if ((stepperNumber >= 0) && (stepperNumber < kMB2_MaxSteppers))
	{
		LCDdisplay_XYZposition(true, gCalibrateString);

		if ((gStepper[stepperNumber].hasEndStopMin == false) || (gStepper[stepperNumber].hasEndStopMax == false))
		{
			LCD.clear();
			LCD_print_P(gTextNoEndstops1);
			LCD.setCursor(0, 1);
			LCD_print_P(gTextNoEndstops2);
			delay(5000);
			return(false);
		}
		myMinStopPin	=	gStepper[stepperNumber].endStopMinPin;
		myMaxStopPin	=	gStepper[stepperNumber].endStopMaxPin;

		//*	this is going to find the mins and maxs of each access
		loopCounter		=	0;
		currentLocation	=	MB2_GetCurrentStepperLocation(stepperNumber);
		MB2_StepperMoveToLocation(stepperNumber, (currentLocation - 100));

		while (keepGoing && (MB2_GetEndStopMin(stepperNumber) != LOW))
		{
			deltaSteps	=	MB2_GetCurrentStepperDelta(stepperNumber);

			if (deltaSteps < 50)
			{
				currentLocation	=	MB2_GetCurrentStepperLocation(stepperNumber);
				MB2_StepperMoveToLocation(stepperNumber, (currentLocation - 100));
			}
			
			if ((loopCounter % 50) == 0)
			{
				LCD.setCursor(0, lcdRowNum);
				LCD_print_P(axisName);
				LCD.print(MB2_GetCurrentStepperLocation(stepperNumber));
				LCDdisplay_EndStops(lcdRowNum, myMinStopPin, myMaxStopPin);	//*	line 2 of the LCD screen 
			}
			
			loopCounter++;
			
			if (MIB_IsCancelDown())
			{
				keepGoing	=	false;
			}
		}
		MB2_CancelMove(stepperNumber);
		
		if (MB2_GetEndStopMin(stepperNumber) == LOW)
		{
			minReached	=	true;		//*	the limit switch was hit
		}
		else
		{
			return(false);						//*	cancel was hit, just return
		}


		LCDdisplay_XYZposition(true, gCalibrateString);
		LCDdisplay_EndStops(lcdRowNum, myMinStopPin, myMaxStopPin);	//*	line 2 of the LCD screen 
		delay(500);
		MB2_SetCurrentStepperLocation(stepperNumber, 0);
		LCDdisplay_XYZposition(true, gCalibrateString);
		LCDdisplay_EndStops(lcdRowNum, myMinStopPin, myMaxStopPin);	//*	line 2 of the LCD screen 
		
		loopCounter	=	0;
		while (keepGoing && (MB2_GetEndStopMax(stepperNumber) != LOW))
		{
			deltaSteps	=	MB2_GetCurrentStepperDelta(stepperNumber);

			if (deltaSteps < 50)
			{
				currentLocation	=	MB2_GetCurrentStepperLocation(stepperNumber);
				MB2_StepperMoveToLocation(stepperNumber, (currentLocation + 100));
			}
			
			if ((loopCounter % 50) == 0)
			{
				LCD.setCursor(0, lcdRowNum);
				LCD_print_P(axisName);
				LCD.print(MB2_GetCurrentStepperLocation(stepperNumber));
				LCDdisplay_EndStops(lcdRowNum, myMinStopPin, myMaxStopPin);	//*	line 2 of the LCD screen 
			}
			
			loopCounter++;
			
			if (MIB_IsCancelDown())
			{
				keepGoing	=	false;
			}
		}
		MB2_CancelMove(stepperNumber);


		if (MB2_GetEndStopMax(stepperNumber) == LOW)
		{
			maxReached	=	true;		//*	the limit switch was hit
		}
		else
		{
			return(false);						//*	cancel was hit, just return
		}
		LCDdisplay_XYZposition(true, gCalibrateString);
		LCDdisplay_EndStops(1, myMinStopPin, myMaxStopPin);	//*	line 2 of the LCD screen 

	}

	return(minReached && maxReached);
}



#pragma mark -

volatile unsigned long	gInterruptCounter;


//******************************************************************************************
void	StepperInterrupt_EnableInterrupts(void)
{
	ENABLE_STEPPER_DRIVER_INTERRUPT();
}


//******************************************************************************************
unsigned long	StepperInterrupt_GetIntCounter(void)
{
	return(gInterruptCounter);
}

//******************************************************************************************
void	StepperInterrupt_ZeroIntCounter(void)
{
	gInterruptCounter	=	0;
}




//******************************************************************************************
//*	private routines
//******************************************************************************************

//******************************************************************************************
static void	SMD3stepperInit(TYPE_Stepper	*theStepperDef,
							short			enablePin,
							short			directionPin,
							short			stepPin,
							short			endStopMinPin,
							short			endStopMaxPin)
{
uint8_t port;

	theStepperDef->enablePin		=	enablePin;
	theStepperDef->directionPin		=	directionPin;
	theStepperDef->stepPin			=	stepPin;
	theStepperDef->endStopMinPin	=	endStopMinPin;
	theStepperDef->endStopMaxPin	=	endStopMaxPin;


	theStepperDef->currentLocation	=	0;
	theStepperDef->desiredLocation	=	0;

	pinMode(enablePin,			OUTPUT);
	pinMode(directionPin,		OUTPUT);
	pinMode(stepPin,			OUTPUT);
	
	digitalWrite(enablePin,		HIGH);	//*	disable the motor
	digitalWrite(stepPin,		LOW);
	digitalWrite(directionPin,	LOW);

#ifdef _USE_DIRECT_PORT_IO_
	//*	get the port definition for this pin so we can use it in the ISR
	theStepperDef->enablePin_bitMask	=	digitalPinToBitMask(enablePin);
	port								=	digitalPinToPort(enablePin);
	theStepperDef->enablePin_out		=	portOutputRegister(port);
	if (port == NOT_A_PIN)
	{
		//*	fatal error
	}

	theStepperDef->stepPin_bitMask	=	digitalPinToBitMask(stepPin);
	port							=	digitalPinToPort(stepPin);
	theStepperDef->stepPin_out		=	portOutputRegister(port);
	if (port == NOT_A_PIN)
	{
		//*	fatal error
	}

#endif


	if (endStopMinPin >= 0)
	{
	#ifdef _USE_DIRECT_PORT_IO_
		//*	get the port info for the Minimum Endstop
		theStepperDef->endStopMinPin_bitMask	=	digitalPinToBitMask(endStopMinPin);
		port									=	digitalPinToPort(endStopMinPin);
		theStepperDef->endStopMinPin_inReg		=	portInputRegister(port);
	#endif
		theStepperDef->hasEndStopMin			=	true;
	}

	if (endStopMaxPin >= 0)
	{
	#ifdef _USE_DIRECT_PORT_IO_
		//*	get the port info for the Max Endstop
		theStepperDef->endStopMaxPin_bitMask	=	digitalPinToBitMask(endStopMaxPin);
		port									=	digitalPinToPort(endStopMaxPin);
		theStepperDef->endStopMaxPin_inReg		=	portInputRegister(port);
	#endif
		theStepperDef->hasEndStopMax			=	true;
	}
}




#pragma mark -


//******************************************************************************************
void StepperInterrupt_Init(void)
{
	gInterruptCounter	=	0;

#if defined(__AVR__)
	// waveform generation = 0100 = CTC
	TCCR1B	&=	~(1 << WGM13);
	TCCR1B	|=	(1 << WGM12);
	TCCR1A	&=	~(1 << WGM11);
	TCCR1A	&=	~(1 << WGM10);

	// output mode = 00 (disconnected)
	TCCR1A	&=	~(3 << COM1A0);
	TCCR1A	&=	~(3 << COM1B0);
	TCCR1B	=	(TCCR1B & ~(0x07 << CS10)) | (2 << CS10);	// 2MHz timer

//	OCR1A	=	2000;
	OCR1A	=	3000;
	ENABLE_STEPPER_DRIVER_INTERRUPT();
	sei();
#endif

}



#if defined(__AVR__)


//******************************************************************************************
//*	Stepper motor interrput routine
//*		It is very important that this ISR (Interrupt Services Routine) executes as quickly as possible
//*		For that reason, it trys NOT to call subroutines and may be a bit longer than normal
//******************************************************************************************
//*	This looks through the array of stepper motors and steps them if needed
//*	It also checks the endstops and WILL not step if the endstop is activated in the direction of travel
//******************************************************************************************
//*	There are 2 types of stepping, direct and linear, direct means each axis is independent
//*	linear means we are trying to draw a staight line between 2 points
//******************************************************************************************
ISR(TIMER1_COMPA_vect)
{
short	ii;
boolean	okToStep;

	//*	debugging
	digitalWrite(kMIB_LED_Debug, HIGH);

	gInterruptCounter++;

	//*	go through the steppers and see which ones need servicing
	for (ii=0; ii<kMB2_MaxSteppers; ii++)
	{
		if (gStepper[ii].isActive && (gStepper[ii].isLinear == false))
		{
			//*	make sure the stepper motor is enabled (pin set to LOW)
		#ifdef _USE_DIRECT_PORT_IO_
			*gStepper[ii].enablePin_out	&=	~gStepper[ii].enablePin_bitMask;
		#else
			digitalWrite(gStepper[ii].enablePin, LOW);	//*	enable the motor
		#endif

			okToStep	=	true;

		#ifdef _USE_ACCELERATION_
			//*	this part handles acceleration
			if (gStepper[ii].useAcceleration)
			{
				gStepper[ii].irqCounter--;
				if (gStepper[ii].irqCounter <= 0)
				{
					//*	its time to step, check acceleration (for the next step)
					gStepper[ii].stepsAtThisRate++;
					if (gStepper[ii].stepsAtThisRate >= 20)
					{
						gStepper[ii].currentIrqCount--;
					}
					if (gStepper[ii].currentIrqCount <= gStepper[ii].minIrqCount)

					gStepper[ii].irqCounter	=	gStepper[ii].currentIrqCount;
				}
				else
				{
					okToStep	=	false;
				}
			}
		#endif
		
		
			//*	does it have ends stops (min)
			if ((gStepper[ii].hasEndStopMin) && (gStepper[ii].currentDirection == kStepperReverse))
			{
				//*	check the end stops
			#ifdef _USE_DIRECT_PORT_IO_
				//*	if in reverse and the minimum endpoint is LOW
				if (!(*gStepper[ii].endStopMinPin_inReg & gStepper[ii].endStopMinPin_bitMask))
			#else
				if (digitalRead(gStepper[ii].endStopMinPin) == LOW)
			#endif
				{
					okToStep	=	false;
				}
			}
		
			//*	does it have ends stops (max)
			if ((gStepper[ii].hasEndStopMax) && (gStepper[ii].currentDirection == kStepperForward))
			{
			#ifdef _USE_DIRECT_PORT_IO_
				//*	if in reverse and the Maximum endpoint is LOW
				if (!(*gStepper[ii].endStopMaxPin_inReg & gStepper[ii].endStopMaxPin_bitMask))
			#else
				if (digitalRead(gStepper[ii].endStopMaxPin) == LOW)
			#endif
				{
					okToStep	=	false;
				}
			}
			if (okToStep)
			{
				if (gStepper[ii].currentStepPinState)
				{
					//*	its high, set it low
				#ifdef _USE_DIRECT_PORT_IO_
					*gStepper[ii].stepPin_out	&=	~gStepper[ii].stepPin_bitMask;
				#else
					digitalWrite(gStepper[ii].stepPin, LOW);
				#endif
					gStepper[ii].currentStepPinState	=	LOW;
				}
				else
				{
					//*	if its currently low, set it HIGH
				#ifdef _USE_DIRECT_PORT_IO_
					*gStepper[ii].stepPin_out	|=	gStepper[ii].stepPin_bitMask;
				#else
					digitalWrite(gStepper[ii].stepPin, HIGH);
				#endif
					gStepper[ii].currentStepPinState	=	HIGH;	

					//*	update the position, the A3977 does the step on the LOW to HIGH transition
					if (gStepper[ii].currentDirection == kStepperForward)
					{
						gStepper[ii].currentLocation++;
					}
					else
					{
						gStepper[ii].currentLocation--;
					}
				}
					
			}
			if (gStepper[ii].currentLocation == gStepper[ii].desiredLocation)
			{
				gStepper[ii].isActive	=	false;
			}
		}
	}
	
	//******************************************************************************************
	//*	now check for LINEAR stepping
	//*	in order to speed up the ISR, check linear FIRST, before checking active, 
	//*	linear should get cleared at the same time at the same time active does
	if (gStepper[kMB2_StepperX].isLinear || gStepper[kMB2_StepperY].isLinear)
	{
	TYPE_Stepper	*stepperX;
	TYPE_Stepper	*stepperY;
	
		stepperX	=	&gStepper[kMB2_StepperX];
		stepperY	=	&gStepper[kMB2_StepperY];
	
		if (stepperX->isActive || stepperY->isActive)
		{
			//*	we are going to do them together
			if ((stepperX->currentStepPinState) || (stepperY->currentStepPinState))
			{
				//*	its high, set it BOTH x and Y low
			#ifdef _USE_DIRECT_PORT_IO_
				*stepperX->stepPin_out	&=	~stepperX->stepPin_bitMask;
				*stepperY->stepPin_out	&=	~stepperY->stepPin_bitMask;
			#else
				digitalWrite(stepperX->stepPin, LOW);
				digitalWrite(stepperY->stepPin, LOW);
			#endif
				stepperX->currentStepPinState	=	LOW;
				stepperY->currentStepPinState	=	LOW;
			}
			else
			{
				//*	check for end stops
				okToStep	=	true;
					
				//*	does it have ends stops (min)
				if ((stepperX->hasEndStopMin) && (stepperX->currentDirection == kStepperReverse))
				{
					//*	check the end stops
				#ifdef _USE_DIRECT_PORT_IO_
					//*	if in reverse and the minimum endpoint is LOW
					if (!(*stepperX->endStopMinPin_inReg & stepperX->endStopMinPin_bitMask))
				#else
					if (digitalRead(stepperX->endStopMinPin) == LOW)
				#endif
					{
						okToStep	=	false;
					}
				}
			
				//*	does it have ends stops (max)
				if ((stepperX->hasEndStopMax) && (stepperX->currentDirection == kStepperForward))
				{
				#ifdef _USE_DIRECT_PORT_IO_
					//*	if in reverse and the Maximum endpoint is LOW
					if (!(*stepperX->endStopMaxPin_inReg & stepperX->endStopMaxPin_bitMask))
				#else
					if (digitalRead(stepperX->endStopMaxPin) == LOW)
				#endif
					{
						okToStep	=	false;
					}
				}
				
				if (okToStep)
				{
					
					//******************************************************************************************
					//*	now for the tricky part
					if (stepperX->isPrimary)
					{
						//*	move X
						//*	if its currently low, set it HIGH
					#ifdef _USE_DIRECT_PORT_IO_
						*stepperX->stepPin_out	|=	stepperX->stepPin_bitMask;
					#else
						digitalWrite(stepperX->stepPin, HIGH);
					#endif
						stepperX->currentStepPinState	=	HIGH;	

						//*	update the position, the A3977 does the step on the LOW to HIGH transition
						if (stepperX->currentDirection == kStepperForward)
						{
							stepperX->currentLocation++;
						}
						else
						{
							stepperX->currentLocation--;
						}
					
					
						//*	now figure out if we need to move Y
						stepperY->accumulatedError	+=	stepperY->delta;
						if (stepperY->accumulatedError > stepperX->delta)
						{
							stepperY->accumulatedError	-=	stepperX->delta;

						#ifdef _USE_DIRECT_PORT_IO_
							*stepperY->stepPin_out	|=	stepperY->stepPin_bitMask;
						#else
							digitalWrite(stepperY->stepPin, HIGH);
						#endif
							stepperY->currentStepPinState	=	HIGH;	

							//*	update the position, the A3977 does the step on the LOW to HIGH transition
							if (stepperY->currentDirection == kStepperForward)
							{
								stepperY->currentLocation++;
							}
							else
							{
								stepperY->currentLocation--;
							}

						}
					}
					else if (stepperY->isPrimary)
					{
						//*	stepper Y is primary

						//*	move Y
						//*	if its currently low, set it HIGH
					#ifdef _USE_DIRECT_PORT_IO_
						*stepperY->stepPin_out	|=	stepperY->stepPin_bitMask;
					#else
						digitalWrite(stepperY->stepPin, HIGH);
					#endif
						stepperY->currentStepPinState	=	HIGH;	

						//*	update the position, the A3977 does the step on the LOW to HIGH transition
						if (stepperY->currentDirection == kStepperForward)
						{
							stepperY->currentLocation++;
						}
						else
						{
							stepperY->currentLocation--;
						}
					
					
						//*	now figure out if we need to move X
						stepperX->accumulatedError	+=	stepperX->delta;
						if (stepperX->accumulatedError > stepperY->delta)
						{
							stepperX->accumulatedError	-=	stepperY->delta;

						#ifdef _USE_DIRECT_PORT_IO_
							*stepperX->stepPin_out	|=	stepperX->stepPin_bitMask;
						#else
							digitalWrite(stepperX->stepPin, HIGH);
						#endif
							stepperX->currentStepPinState	=	HIGH;	

							//*	update the position, the A3977 does the step on the LOW to HIGH transition
							if (stepperX->currentDirection == kStepperForward)
							{
								stepperX->currentLocation++;
							}
							else
							{
								stepperX->currentLocation--;
							}
						}
					}
					
					
					if (gStepper[kMB2_StepperX].currentLocation == gStepper[kMB2_StepperX].desiredLocation)
					{
						gStepper[kMB2_StepperX].isActive	=	false;
						gStepper[kMB2_StepperX].isLinear	=	false;
						
						//*	leave the other axis in case it has one more step to go
						gStepper[kMB2_StepperY].isLinear	=	false;
					}

					if (gStepper[kMB2_StepperY].currentLocation == gStepper[kMB2_StepperY].desiredLocation)
					{
						gStepper[kMB2_StepperY].isActive	=	false;
						gStepper[kMB2_StepperY].isLinear	=	false;

						//*	leave the other axis in case it has one more step to go
						gStepper[kMB2_StepperX].isLinear	=	false;
					}
				}
			}
		}
	}
	
	//*	debugging
	digitalWrite(kMIB_LED_Debug, LOW);
}
#endif
