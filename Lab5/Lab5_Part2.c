/*
 * ICOM 4217 (EMBEDDED SYSTEM DESIGN)
 * Authors: Carlos A. Rodriguez & Osvaldo A. Ramirez
 * LAB5 Part2.2: PWM Signal Generation
 * Instructions:
 * Produce a square wave signal using PWM module
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

/* Global Variable Declaration */
int index = 0;

/* Function Declaration */
void checkIndex(void);

/* Main Code of the program */
int main (void)
{
	/* Variable Declaration */
	volatile uint32_t load;										//load is the PWM high count register desired value
	volatile uint32_t PWMClock;									//PWM clock frequency
	volatile int PWM_FREQUENCY[5] = {500,1000,2000,4000,8000};	//Different PWM frequencies
	int width;													//width allows controlling the duty cycle of the signal

	SysCtlClockSet(SYSCTL_SYSDIV_5|SYSCTL_USE_PLL|SYSCTL_OSC_MAIN|SYSCTL_XTAL_16MHZ);		//Set clock frequency to 40Mhz
	SysCtlPWMClockSet(SYSCTL_PWMDIV_2);														//PWM prescaler 2 in order to be able to use 500Hz

	SysCtlPeripheralEnable(SYSCTL_PERIPH_PWM1);			//Enable PWM module 1
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOD);		//Enable PORT D
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);		//Enable PORT F

	GPIOPinTypePWM(GPIO_PORTD_BASE,GPIO_PIN_0);			//Set Pin 0 of PORT D as PWM
	GPIOPinConfigure(GPIO_PD0_M1PWM0);					//Configure Pin 0 of PORT D as PWM

	/* Tiva buttons configuration */
	HWREG(GPIO_PORTF_BASE + GPIO_O_LOCK) = GPIO_LOCK_KEY;
	HWREG(GPIO_PORTF_BASE + GPIO_O_CR) = 0X01;
	HWREG(GPIO_PORTF_BASE + GPIO_O_LOCK) = 0;
	GPIODirModeSet(GPIO_PORTF_BASE,GPIO_PIN_4,GPIO_DIR_MODE_IN);							//Set Pin 4 PORT F as input
	GPIOPadConfigSet(GPIO_PORTF_BASE,GPIO_PIN_4,GPIO_STRENGTH_2MA, GPIO_PIN_TYPE_STD_WPU); 	//Configure Pin 4 of PORT F as Pull Up

	PWMClock = SysCtlClockGet()/2;							//Set PWMClock equal to the system clock divided by 2 since we had a PreScaler
	load = (PWMClock/PWM_FREQUENCY[index]) -1;				//Get the period for the desired frequency, substract one since it count zero

	PWMGenConfigure(PWM1_BASE,PWM_GEN_0,PWM_GEN_MODE_DOWN|PWM_GEN_MODE_NO_SYNC);	//Configure the PWM generator to count down
	PWMGenPeriodSet(PWM1_BASE,PWM_GEN_0,load);										//Load the desired period depending on the frequency
	width = (load/2);																//Set the high count register at 50% duty cycle
	PWMPulseWidthSet(PWM1_BASE,PWM_OUT_0, width);									//Load the duty cycle
	PWMOutputState(PWM1_BASE,PWM_OUT_0_BIT,true);									//Set output state if module 1 output 1 to true
	PWMGenEnable(PWM1_BASE,PWM_GEN_0);												//Enable Module 1 and generator generator 0

	while(1)
	{
		/* Button pressed */
		if(GPIOPinRead(GPIO_PORTF_BASE,GPIO_PIN_4) == 0){
			SysCtlDelay(4000000);					//Debouncing delay
			index++;								//increment frequency array index
			checkIndex();							//Check if index is out of bounce
			load = (PWMClock/PWM_FREQUENCY[index])-1;			//Calculate the new period for the new frequency
			PWMGenPeriodSet(PWM1_BASE,PWM_GEN_0,load);			//Load the new period
			width = (load/2);									//Calculate the new high count register value
			PWMPulseWidthSet(PWM1_BASE,PWM_OUT_0, width);		//Load the high count register with the new value
		}
	}
}
/* Check Index function fixes the index whenever it goes out of limits */
void checkIndex(){
	if(index == 5){
		index = 0;
	}
}
