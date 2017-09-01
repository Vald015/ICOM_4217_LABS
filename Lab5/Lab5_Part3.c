/*
 * ICOM 4217 (EMBEDDED SYSTEM DESIGN)
 * Authors: Carlos A. Rodriguez & Osvaldo A. Ramirez
 * LAB5 Part2.3: Generating Colors with RBG LED
 * Instructions:
 * Generate different colors using RGB LED with PWM signal
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
#define PWM_FREQUENCY 1000				//PWM frequency value
int interruptFlag = 0;					//Interruption Fla
int index = 0;							//Index of the duty cycle array

/* Function Declaration */
void getWidth(float width[], int Load);	//Function for setting the high count register of the PWM signal
void checkIndex();						//Check if the index is out of bounce
void PB1_IntHandler(void);				//Pushbutton interruption handler

/* Main code of the program*/
int main (void)
{
	/* Variable Declaration */
	volatile uint32_t load;						//Period of the PWM signal
	volatile uint32_t PWMClock;					//Clock signal for the PWM signal
	float dC[8][3] = {{0,0,1},{0,1,0},{1,0,0},{1,0.117647059,0.850980392},{0.117647059,0.870588235,0.988235294},{0.941176471,0.784313725,0.156862745},{1,0.482352941,0.129411765},{1,1,1}}; //Array containing the different high count register values for each LED

	SysCtlClockSet(SYSCTL_SYSDIV_5|SYSCTL_USE_PLL|SYSCTL_OSC_MAIN|SYSCTL_XTAL_16MHZ);		//Set clock to 40MHz
	SysCtlPWMClockSet(SYSCTL_PWMDIV_2);														//Set PWM signal PreScaler to 2

	SysCtlPeripheralEnable(SYSCTL_PERIPH_PWM0);				//Enable PWM module 0
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOB);			//Enable GPIO PORT B
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);			//Enable GPIO PORT F
	SysCtlPeripheralSleepEnable(SYSCTL_PERIPH_GPIOF);		//Enable the GPIO PORT F to wake Microprocessor from low power mode

	GPIOPinTypePWM(GPIO_PORTB_BASE,GPIO_PIN_5|GPIO_PIN_6|GPIO_PIN_7);		// Configure PORT B PINS 5,6 & 7 as PWM

	GPIOPinConfigure(GPIO_PB5_M0PWM3);		// Configure PORT B PINS 5 as PWM
	GPIOPinConfigure(GPIO_PB6_M0PWM0);		// Configure PORT B PINS 6 as PWM
	GPIOPinConfigure(GPIO_PB7_M0PWM1);		// Configure PORT B PINS 7 as PWM

	/* Tiva buttons configuration */
	HWREG(GPIO_PORTF_BASE + GPIO_O_LOCK) = GPIO_LOCK_KEY;
	HWREG(GPIO_PORTF_BASE + GPIO_O_CR) = 0X01;
	HWREG(GPIO_PORTF_BASE + GPIO_O_LOCK) = 0;
	GPIODirModeSet(GPIO_PORTF_BASE,GPIO_PIN_4,GPIO_DIR_MODE_IN);							//Setup PORT F pin 4 as input pin
	GPIOPadConfigSet(GPIO_PORTF_BASE,GPIO_PIN_4,GPIO_STRENGTH_2MA, GPIO_PIN_TYPE_STD_WPU); 	//Setup PORT F pin 4 as pull up

	GPIOIntRegister(GPIO_PORTF_BASE, PB1_IntHandler); 											//Registers an interrupt handler for a GPIO port. Ensures that the interrupt handler specified is called.
	GPIOIntTypeSet(GPIO_PORTF_BASE, GPIO_PIN_4, GPIO_RISING_EDGE);								//Set up the type of interrupt

	PWMClock = SysCtlClockGet()/2;				//Calculate the clock of the PWM with the PreScaler
	load = (PWMClock/PWM_FREQUENCY) -1;			//Calculate the period for the desired frequency, substract one since it count to zero

	PWMGenConfigure(PWM0_BASE,PWM_GEN_0,PWM_GEN_MODE_DOWN|PWM_GEN_MODE_NO_SYNC);		//Configure the PWM Generator 0 to count down
	PWMGenConfigure(PWM0_BASE,PWM_GEN_1,PWM_GEN_MODE_DOWN|PWM_GEN_MODE_NO_SYNC);		//Configure the PWM Generator 1 to count down

	PWMGenPeriodSet(PWM0_BASE,PWM_GEN_0,load);		//Set the period for the PWM Generator 0
	PWMGenPeriodSet(PWM0_BASE,PWM_GEN_1,load);		//Set the period for the PWM Generator 1

	PWMPulseWidthSet(PWM0_BASE,PWM_OUT_0, load);			//Set the high count register of the Green LED
	PWMPulseWidthSet(PWM0_BASE,PWM_OUT_1, load*dC[0][2]);	//Set the high count register of the Blue LED
	PWMPulseWidthSet(PWM0_BASE,PWM_OUT_3, load); 			//Set the high count register of the Red LED

	PWMOutputState(PWM0_BASE,PWM_OUT_1_BIT,true);			//Since the first sequence starts with the BLUE LED, Only set true to that output state

	PWMGenEnable(PWM0_BASE,PWM_GEN_0);			//Enable PWM Generator 0
	PWMGenEnable(PWM0_BASE,PWM_GEN_1);			//Enable PWM Generator 1
	GPIOIntEnable(GPIO_PORTF_BASE, GPIO_PIN_4); //Enable interrupt for PA4
	SysCtlSleep();					//Send microprocessor to low power mode

	while(1)
	{
		/* If interrupted then excecute */
		if(interruptFlag == 1){
			index++;			//increment dc index
			checkIndex();		//Check if index is within the boundary of the array
			getWidth(dC[index],load);		//change duty cycle function
			interruptFlag =0;				//Turn off interrupt signal flag
			SysCtlSleep();					//Send microprocessor to low power mode
		}
	}
}
/* Function for changing the duty cycle to all the LEDs in the RGB */
void getWidth(float width[], int load)
{
	/* Remember: 0 percentage duty cycle equals grounding the signal */
	int dCPercentage;					//
	if(width[0] == 0){
		PWMOutputState(PWM0_BASE,PWM_OUT_3_BIT,false);
	}else{
		dCPercentage = load*width[0];
		PWMPulseWidthSet(PWM0_BASE,PWM_OUT_3,dCPercentage);
		PWMOutputState(PWM0_BASE,PWM_OUT_3_BIT,true);
	}
	if(width[1] == 0){
		PWMOutputState(PWM0_BASE,PWM_OUT_0_BIT,false);
	}else{
		dCPercentage = load*width[1];
		PWMPulseWidthSet(PWM0_BASE,PWM_OUT_0,dCPercentage);
		PWMOutputState(PWM0_BASE,PWM_OUT_0_BIT,true);
	}
	if(width[2] == 0){
		PWMOutputState(PWM0_BASE,PWM_OUT_1_BIT,false);
	}else{
		dCPercentage = load*width[2];
		PWMPulseWidthSet(PWM0_BASE,PWM_OUT_1,dCPercentage);
		PWMOutputState(PWM0_BASE,PWM_OUT_1_BIT,true);
	}
}

/* Function for fixing the index when its out of the boundary of the array */
void checkIndex()
{
	if(index > 7){
		index = 0;
	}
}

/* Button interrupt handler */
void PB1_IntHandler(void)
{
	SysCtlDelay(4000000); 								//Running at 30ms
	interruptFlag = 1;									//Set interrupt Falg to 1
	GPIOIntClear (GPIO_PORTF_BASE, GPIO_PIN_4);			// Clear
}
