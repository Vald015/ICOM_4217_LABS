/*
 * ICOM 4217 (EMBEDDED SYSTEM DESIGN)
 * Authors: Carlos A. Rodriguez & Osvaldo A. Ramirez
 */

/* include necessary system header files */
#include <stdint.h>
#include <stdbool.h>
#include "inc/hw_types.h"
#include "inc/hw_memmap.h"
#include "driverlib/sysctl.h"
#include "driverlib/gpio.h"

/* function declaration */
void initializeLCD();//initialize LCD function declaration
void enterCommand(int command,float delayTime);//enter command function declaration
void writeWord(uint8_t string[], uint8_t times);//writeWord function declaration

/* define label ALLPINS as all the pins in a port */
#define ALLPINS GPIO_PIN_0|GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3|GPIO_PIN_4|GPIO_PIN_5|GPIO_PIN_6|GPIO_PIN_7

/* Main code of the program. */
int main(void)
{
	/* Variable declaration */
	uint8_t text[16][16] = {{"Jump"}};//Words to be written in LCD
	int timesArray[16] = {4}; //text[] words lenght
	int index = 0;// current word pointer

	/* Clock frequency configuration */
	SysCtlClockSet(SYSCTL_SYSDIV_5|SYSCTL_USE_PLL|SYSCTL_XTAL_16MHZ|SYSCTL_OSC_MAIN);// Set clock frequency to 40 MHz.

	/* Peripheral Configuration */
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA); //Enable the peripheral port to use. (A)
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOB);//Enable the peripheral port to use. (B)
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF); //Enable the peripheral port to use. (F)

	/* Input/output configuration */
	GPIOPinTypeGPIOInput(GPIO_PORTA_BASE, GPIO_PIN_5);//Set pull-up button pin as output
	GPIOPinTypeGPIOOutput(GPIO_PORTB_BASE, ALLPINS);// Set all port B pins as output pins
	GPIOPinTypeGPIOOutput(GPIO_PORTF_BASE, GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3);//PIN1 : R/S (R/S: 1 for data ** 0 for instruction) *** PIN2 : R/W (R/S: 1 for Read ** 0 for write) *** PIN3 : E(Chip Enable signal).

	/* Pin current strength configuration */
	GPIOPadConfigSet(GPIO_PORTA_BASE, GPIO_PIN_5, GPIO_STRENGTH_4MA , GPIO_PIN_TYPE_STD_WPD ) ;//Set the pin current strength (default is 2mA) and Pin Type to Pull Down

	initializeLCD();// initialize LCD
	writeWord(text[index],timesArray[index]);

	while(1){

	}
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
	GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_1|GPIO_PIN_2, 0x00); //R/S & R/W = 0
	GPIOPinWrite(GPIO_PORTB_BASE, ALLPINS, command); //enter command on port
	GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_3, 0x08);//Enable On
	SysCtlDelay(delayTime);//value for the desired delay
	GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_3, 0x00);//Enable Off
}

/* Function for writing words in the LCD, string is the word that want to be written, times is the number of characters that the string has */
void writeWord(uint8_t string[], uint8_t times)
{
	GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_1, 0x02);//RS enable

	uint8_t i =0;//initialize for loop
	for(; i < times; i++){
		GPIOPinWrite(GPIO_PORTB_BASE, ALLPINS, string[i]);//write character
		GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_3, 0x08);//Enable On
		SysCtlDelay(94666);//Delay 7.1ms(Calculated)
		GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_3, 0x00);//Enable Off
	}
	GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_1, 0x00);//RS disable
}
