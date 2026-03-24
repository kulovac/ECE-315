/*
 *  Created on: Mar 24, 2021
 *  Modified by : Shyama Gandhi
 *
 *  The driver has been modified according to the lab needs and the code is adapted from TinyStepper_28BYJ_48 arduino library
 *
 */


#ifndef SRC_STEPPER_H_
#define SRC_STEPPER_H_

#include <limits.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "xparameters.h"
#include "xgpio.h"
#include "xscugic.h"
#include "xil_exception.h"
#include "xil_printf.h"

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

#include "math.h"

#define NO_OF_STEPS_PER_REVOLUTION_HALF_DRIVE	4096
#define NO_OF_STEPS_PER_REVOLUTION_FULL_DRIVE	2048

extern XGpio PModMotorInst;
#define PMOD_MOTOR_BASEADDR 	XPAR_STEPPER_MOTOR_BASEADDR


#define WINDINGS_OFF 0b0000

#define WAVE_DRIVE_1 0b0001
#define WAVE_DRIVE_2 0b0010
#define WAVE_DRIVE_3 0b0100
#define WAVE_DRIVE_4 0b1000

#define FULL_STEP_1  0b0011
#define FULL_STEP_2  0b0110
#define FULL_STEP_3  0b1100
#define FULL_STEP_4  0b1001

#define HALF_STEP_1  0b0001
#define HALF_STEP_2  0b0011
#define HALF_STEP_3  0b0010
#define HALF_STEP_4  0b0110
#define HALF_STEP_5  0b0100
#define HALF_STEP_6  0b1100
#define HALF_STEP_7  0b1000
#define HALF_STEP_8  0b1001

/////////////////////////////////////////////////////
typedef enum {
    FULL_STEP_MODE,
    HALF_STEP_MODE,
    WAVE_DRIVE_MODE
} stepper_mode_t;

extern stepper_mode_t step_mode;

extern int _signals_to_motor[4];

extern float desiredSpeed_InStepsPerSecond;
extern float acceleration_InStepsPerSecondPerSecond;
extern float deceleration_InStepsPerSecondPerSecond;

extern float currentStepPeriod;
extern long currentPosition_InSteps;
extern int stepPhase;

extern int direction_Scaler;
extern long targetPosition_InSteps;
extern long decelerationDistance_InSteps;

extern float ramp_InitialStepPeriod;
extern float desiredStepPeriod;
extern float ramp_NextStepPeriod;
extern float acceleration_InStepsPerUSPerUS;
extern float deceleration_InStepsPerUSPerUS;

extern _Bool startNewMove;

extern unsigned long ramp_LastStepTime;


/////////////////////////////////////////////////////
void Stepper_PMOD_pins_to_output();
void Stepper_Initialize();
void Stepper_setCurrentPositionInSteps(long currentPositionInSteps);
long Stepper_getCurrentPositionInSteps();
void Stepper_SetupStop();
void Stepper_setSpeedInStepsPerSecond(float speedInStepsPerSecond);
void Stepper_setAccelerationInStepsPerSecondPerSecond(float accelerationInStepsPerSecondPerSecond);
void Stepper_setDecelerationInStepsPerSecondPerSecond(float decelerationInStepsPerSecondPerSecond);
void Stepper_moveRelativeInSteps(long distanceToMoveInSteps);
void Stepper_SetupRelativeMoveInSteps(long distanceToMoveInSteps);

_Bool Stepper_processMovement(void);
_Bool Stepper_motionComplete();

void Stepper_SetupMoveInSteps(long absolutePositionToMoveToInSteps);
void Stepper_moveToPositionInSteps(long absolutePositionToMoveToInSteps);
void Stepper_setNextFullStep(int direction);
void Stepper_disableMotor();

float Stepper_getCurrentVelocityInStepsPerSecond();


#endif /* SRC_STEPPER_H_ */