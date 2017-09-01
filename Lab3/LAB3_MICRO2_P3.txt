/*
 * ICOM 4217 (EMBEDDED SYSTEM DESIGN)
 * Authors: Carlos A. Rodriguez & Osvaldo A. Ramirez
 * LAB3 PART3: Software Debouncing
 * Instructions:
 * Read the input state of a pin using a software debouncing technique.
 */
/* include necessary system header files */
#include <stdint.h>
#include <stdbool.h>
#include "inc/hw_types.h"
#include "inc/hw_memmap.h"
#include "driverlib/sysctl.h"
#include "driverlib/gpio.h"

/* function declaration */
void initializeLCD();								//initialize LCD function declaration
void enterCommand(int command,float delayTime);		//enter command function declaration
void writeWord(uint8_t number[], uint8_t times);	//Write display function
void PB1_IntHandler();								//Interrupt handler

/* define label ALLPINS as all the pins in a port */
#define ALLPINS GPIO_PIN_0|GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3|GPIO_PIN_4|GPIO_PIN_5|GPIO_PIN_6|GPIO_PIN_7

uint8_t number[2] = {"0"};					//numbers to be written
uint8_t pushFlag = 0;						//Interrupt flag

/* Main code of the program. */
int main (void)
{
	/* Clock frequency configuration */
	SysCtlClockSet(SYSCTL_SYSDIV_5|SYSCTL_USE_PLL|SYSCTL_XTAL_16MHZ|SYSCTL_OSC_MAIN);// Set clock frequency to 40 MHz.

	/* Peripheral Configuration */

	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA); //Enable the peripheral port to use. (A)

	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOB);//Enable the peripheral port to use. (B)

	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF); //Enable the peripheral port to use. (F).

	/* Input/output configuration */

	GPIOPinTypeGPIOOutput(GPIO_PORTB_BASE, ALLPINS);// Set all port B pins as output pins

	GPIOPinTypeGPIOOutput(GPIO_PORTF_BASE, GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3);//PIN1 : R/S (R/S: 1 for data ** 0 for instruction) *** PIN2 : R/W (R/S: 1 for Read ** 0 for write) *** PIN3 : E(Chip Enable signal).

	/*Interrupt Button 1 GPIO PORTA PIN 5*/

	GPIOPinTypeGPIOInput(GPIO_PORTA_BASE, GPIO_PIN_5); //Input configuration

	GPIOPadConfigSet(GPIO_PORTA_BASE, GPIO_PIN_5, GPIO_STRENGTH_4MA, GPIO_PIN_TYPE_STD_WPD); //Set the pin current strength Pin Type to Pull Up

	GPIOIntRegister(GPIO_PORTA_BASE, PB1_IntHandler); //Registers an interrupt handler for a GPIO port. Ensures that the interrupt handler specified is called.

	GPIOIntTypeSet(GPIO_PORTA_BASE, GPIO_PIN_5, GPIO_RISING_EDGE); //Sets the interrupt type for the specified pin(s). GPIO_FALLING_EDGEsets detection to edge and trigger to falling(high to low transition)

	GPIOIntEnable(GPIO_PORTA_BASE, GPIO_PIN_5);     // Enable interrupt for PF4

	initializeLCD();// initialize LCD
	writeWord(number,1);							//Write cero to the LCD display


	while(1){
		if(pushFlag == 1){						//If interrupt flag was set
			enterCommand(0x01,20400);			//Clear LCD
			number[0]++;						//Increase number
			writeWord(number,1);				//Write current number to Display
			pushFlag = 0;						//Set interrupt flag to zero
		}
	}
}
/* Interrupt flag handler */
void PB1_IntHandler (void)
{
	SysCtlDelay(400000); 								//Running at 30ms
	pushFlag = 1;										//Set interrupt flag
	GPIOIntClear (GPIO_PORTA_BASE, GPIO_PIN_5);			// Clear

}

/* LCD initialization function */
void initializeLCD()
{

	SysCtlDelay(533333.3333);// Starting delay (40ms).

	enterCommand(0x3C,520);//Function Set command

	enterCommand(0x3C,493.3);//Function Set

	enterCommand(0x0F,493.3);//Display ON

	enterCommand(0x01,20400);//Clear display command

	enterCommand(0x06,20400);//Entry Mode Set

	enterCommand(0x01,20400);//Clear display command

}

/* Enter command to LCD function; command is the desired LCD instruction, delayTime is the required time that the chip enable needs to be on in order to
 * successfully send the instruction */
void enterCommand(int command,float delayTime)
{

	GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3, 0x08); //R/S & R/W = 0

	GPIOPinWrite(GPIO_PORTB_BASE, ALLPINS, command); //enter command on port

	GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_3, 0x08);//Enable On

	SysCtlDelay(delayTime);//value for the desired delay

	GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_3, 0x00);//Enable Off

}

/* Function for writing both lines in the LCD, string is the word that want to be written, times is the number of characters that the string has */
void writeWord(uint8_t number[], uint8_t times)
{
	GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_1, 0x02);//RS enable

	uint8_t i =0;//initialize for loop

	for(; i < times; i++){

		GPIOPinWrite(GPIO_PORTB_BASE, ALLPINS, number[i]);//write character

		GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_3, 0x08);//Enable On

		SysCtlDelay(94666);//Delay 7.1ms(Calculated)

		GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_3, 0x00);//Enable Off

	}
	GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_1, 0x00);//RS disable
}

