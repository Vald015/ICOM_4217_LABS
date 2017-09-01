
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

/* define label ALLPINS as all the pins in a port */
#define ALLPINS GPIO_PIN_0|GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3|GPIO_PIN_4|GPIO_PIN_5|GPIO_PIN_6|GPIO_PIN_7
void moveStep(int value[], int steps);
void moveStepRight(int value[],int steps);

/* Global Variable Declaration */
#define Orange 1
#define Yellow 2
#define Pink   4
#define Blue   8
#define Half1  3
#define Half2  6
#define Half3  12
#define Half4  9

int value[8] = {Orange, Half1, Yellow, Half2, Pink,Half3,Blue,Half4} ; //Half Step
int value1[8] = {Orange, Yellow, Pink,Blue} ; //Full Step - 32 full step = 90 degrees
int index = 0;
int index2 = 3;
int counter = 0;

void main(void){
	SysCtlClockSet(SYSCTL_SYSDIV_5|SYSCTL_USE_PLL|SYSCTL_XTAL_16MHZ|SYSCTL_OSC_MAIN);		// Set clock frequency to 40 MHz.

	/* System peripheral enable */
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOB);		//Enable the peripheral port to use/ (B) // Signals for Steps

	/*Pins for Steps configuration*/
	GPIOPinTypeGPIOOutput(GPIO_PORTB_BASE, GPIO_PIN_0|GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3);

	moveStep(value1,96);//270 degrees
	SysCtlDelay(13333333);//1 seg
	moveStep(value1,64);//180 degrees
	SysCtlDelay(13333333);//1 seg
	moveStepRight(value1,32);//90 degrees

	while(1){


	}

}

void moveStep(int value[],int steps){

	while(counter!=steps){
		GPIOPinWrite(GPIO_PORTB_BASE, GPIO_PIN_0|GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3, value[index]);
		SysCtlDelay(133333.33);//10ms
		if(index == 3){//Check if
			index = 0;
			counter++;

		}
		else{
			index++;
		}
	}
	counter = 0;
}
void moveStepRight(int value[],int steps){

	while(counter!=steps){
		GPIOPinWrite(GPIO_PORTB_BASE, GPIO_PIN_0|GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3, value[index2]);
		SysCtlDelay(133333.33);//10ms
		if(index2 == 0){
			index2 = 3;
			counter++;

		}
		else{
			index2--;
		}
	}

	counter = 0;
}


