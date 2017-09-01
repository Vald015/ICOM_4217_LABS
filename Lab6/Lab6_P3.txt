/*
 * ICOM 4217 (EMBEDDED SYSTEM DESIGN)
 * Authors: Carlos A. Rodriguez & Osvaldo A. Ramirez
 * LAB6 Part 3: Servo-Motor Interface
 * Instructions:
 * The purpose of this section is controlling a servo motor using PWM signal.
 */

/* Header files */
#include <stdint.h>
#include <stdbool.h>
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "inc/tm4c123gh6pm.h"
#include "driverlib/sysctl.h"
#include "driverlib/gpio.h"
#include "driverlib/debug.h"
#include "driverlib/pwm.h"
#include "driverlib/pin_map.h"
#include "inc/hw_gpio.h"
#include "driverlib/timer.h"

//**************************** Variable & Function Declaration ******************************************

/* Constant variables Declaration */
#define ALLPINS GPIO_PIN_0|GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3|GPIO_PIN_4|GPIO_PIN_5|GPIO_PIN_6|GPIO_PIN_7		//define label ALLPINS as all the pins in a port
#define PWM_FREQUENCY 50									//PWM frequency value
#define slopeDC 0.000469444									//Slope for the next PWM formula
#define initialDC 0.0215									//Angle 0 PWM duty cycle

/* Function declaration */
float calculate();									//Function for calculating the next PWM duty cycle for the angle given

/* Global Variable Declaration */
//float angles[12] = {0.00043,0.00064875,0.00152375,0.001305,0.0017425,0.0008675,0.00218,0.001986125,0.00108625};
float angles[10] = {0,22.5,112.5,90,135,45,180,157.5,67.5};			//Angle array
int index=0;													//index of angle array
int flag = 0;												//interrupt flag
float width;												//high count register value (duty cycle)
int counter = 0;											//Counter for the timer
int specialCase = 0;										//Special case flag for the last angle

//*******************************************************************************************************

//---------------------------------- Main Function ------------------------------------------------------

void main(void){

	/* Variable Declaration */
	volatile uint32_t load;									//Period of the PWM signal
	volatile uint32_t PWMClock;								//Clock signal for the PWM signal
	volatile uint32_t timerPeriod;							//Timer period for 1 second

	/* Clock frequency configuration */
	SysCtlClockSet(SYSCTL_SYSDIV_5|SYSCTL_USE_PLL|SYSCTL_XTAL_16MHZ|SYSCTL_OSC_MAIN);	// Set clock frequency to 40 MHz.
	SysCtlPWMClockSet(SYSCTL_PWMDIV_16);												//PWM prescaler of 16 in order to achieve 50Hz

//************************************* Peripheral Configuration ****************************************

	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOE);		//Enable PORT E
	SysCtlPeripheralEnable(SYSCTL_PERIPH_PWM0);			//Enable PWM module 0

	/* Timer Configuration */
	SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER0);		//Enable Timer 0
	TimerConfigure(TIMER0_BASE,TIMER_CFG_PERIODIC);		//Set timer as Periodic
	timerPeriod = (SysCtlClockGet());					//Set timer period to 40M in order to count 1 second
	TimerLoadSet(TIMER0_BASE,TIMER_A,timerPeriod-1);	//Load period

	/*PWM Configuration*/
	GPIOPinTypePWM(GPIO_PORTE_BASE,GPIO_PIN_5);				//Set PORT E pin 5 as PWM
	GPIOPinConfigure(GPIO_PE5_M0PWM5);						//Configure PORT E pin 5 as PWM
	PWMClock = SysCtlClockGet()/16;							//Calculate the PWM clock frequency after PreScaler
	load = (PWMClock/PWM_FREQUENCY) -1;						//Calculate the period of the PWM signal, substract 1 since it counts to zero
	PWMGenConfigure(PWM0_BASE,PWM_GEN_2,PWM_GEN_MODE_DOWN);	//Set PWM signal to count down
	PWMGenPeriodSet(PWM0_BASE,PWM_GEN_2,load);				//Load the Period of the signal

	width = load*calculate();

	PWMPulseWidthSet(PWM0_BASE,PWM_OUT_5, width);			//Load the high count register value
	PWMOutputState(PWM0_BASE,PWM_OUT_5_BIT,true);			//Set output state to false

	/* Interrupt enable */
	IntEnable(INT_TIMER0A);									//Enable Timer 0 subtimer A
	TimerIntEnable(TIMER0_BASE,TIMER_TIMA_TIMEOUT);			//Enable interrupt when done counting
	IntMasterEnable();										//Enable master interrupt enable (Global interrupt Enable)

	/* PWM generator & timer enable */
	PWMGenEnable(PWM0_BASE,PWM_GEN_2);						//Enable the PWM module 0 generator 2

	TimerEnable(TIMER0_BASE,TIMER_A);						//Enable Timer 0 Subtimer A

//*******************************************************************************************************
	//Loop
	while(1){
		//If the flag is raised change PWM signal duty signal
		if (flag){
			TimerDisable(TIMER0_BASE,TIMER_A);				//Disable timer to disable counting while executing the change
			width = load*calculate();						//Calculate new PWM signal duty cycle
			PWMPulseWidthSet(PWM0_BASE,PWM_OUT_5, width);	//Load the high count register value (Duty Cycle)
			flag =0;										//Lower interrupt flag
			if(!specialCase){
				TimerEnable(TIMER0_BASE,TIMER_A);				//Enable timer again
			}
		}
	}
}

//-------------------------------------------------------------------------------------------------------

	/* Timer Interrupt Handler */
	void Timer0IntHandler(void){
		TimerIntClear(TIMER0_BASE, TIMER_TIMA_TIMEOUT);			//Clear timer interrupt flag
		counter++;												//Increase timer by 1
		if(counter == 2){						//If timer equals 2, means it count 2 seconds
			index++;							//Increase index
			flag = 1;							//Raise interrupt flag
			counter =0;							//Reset counter
		}
	}

	/* Function for calculating the new PWM signal duty cycle  */
	float calculate(){
		if(index > 8){
			specialCase = 1;
			index = 0;
		}
		return  slopeDC*angles[index] + initialDC;			// Y = m *(desired duty cycle) + initial duty Cycle
	}
