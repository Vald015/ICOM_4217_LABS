/*
 * ICOM 4217 (EMBEDDED SYSTEM DESIGN)
 * Authors: Carlos A. Rodriguez & Osvaldo A. Ramirez
 *
 * This program generates a circular scrolling list of messages and display them on the LCD. The list must contain a minimum of 16 messages.
 * For scrolling the list their are provided twopushbuttons(PORT A: pins 5 & 7) connected to the MCU. Two consecutive messages are displayed at the same time on the
 * LCD; the first line of the LCD contains the message and the second line for the following message.
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
void writeLCD(uint8_t string[],uint8_t string2[],int times, int times2);//writeLCD function declaration
void writeWord(uint8_t string[], uint8_t times);//writeWord function declaration
int checkIndex(int indexChecked);//checkIndex fucntion declaration

/* define label ALLPINS as all the pins in a port */
#define ALLPINS GPIO_PIN_0|GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3|GPIO_PIN_4|GPIO_PIN_5|GPIO_PIN_6|GPIO_PIN_7

/* Main code of the program. */
int main(void)
{
	/* Variable declaration */
	uint8_t ui8LED = 0; //Red LED1
	uint8_t ui8LED2 = 64; //Red LED2 ** 64 equals sending a logic 1 to BIN7 **
	uint8_t text[16][16] = {{"Jump"}, {"Walk"},{"Run"},{"Kick"}, {"Dance"}, {"Swim"}, {"Sleep"}, {"Fly"}, {"Punch"}, {"Write"}, {"Read"}, {"Pull"}, {"Push"}, {"Press"}, {"Roll"}, {"Drink"}};//Words to be written in LCD
	int timesArray[16] = {4,4,3,4,5,4,5,3,5,5,4,4,4,5,4,5}; //text[] words lenght
	int index = 0;// current word pointer
	int index2 = 1; // index of the second word to be writen

	/* Clock frequency configuration */
	SysCtlClockSet(SYSCTL_SYSDIV_5|SYSCTL_USE_PLL|SYSCTL_XTAL_16MHZ|SYSCTL_OSC_MAIN);// Set clock frequency to 40 MHz.

	/* Peripheral Configuration */
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA); //Enable the peripheral port to use. (A)
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOB);//Enable the peripheral port to use. (B)
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOE); //Enable the peripheral port to use. (F)

	/* Input/output configuration */
	GPIOPinTypeGPIOOutput(GPIO_PORTA_BASE, GPIO_PIN_4|GPIO_PIN_6);//LED 1 & LED 2
	GPIOPinTypeGPIOInput(GPIO_PORTA_BASE, GPIO_PIN_5|GPIO_PIN_7);//Button 1 & 2 7(Pin 7 Pull Down)
	GPIOPinTypeGPIOOutput(GPIO_PORTB_BASE, ALLPINS);// Set all port B pins as output pins
	GPIOPinTypeGPIOOutput(GPIO_PORTE_BASE, GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3);//PIN1 : R/S (R/S: 1 for data ** 0 for instruction) *** PIN2 : R/W (R/S: 1 for Read ** 0 for write) *** PIN3 : E(Chip Enable signal).

	/* Pin current strength configuration */
	GPIOPadConfigSet(GPIO_PORTA_BASE, GPIO_PIN_4|GPIO_PIN_6, GPIO_STRENGTH_4MA , GPIO_PIN_TYPE_STD ) ;//Set the pin current strength (default is 2mA) and Pin Type to Standard
	GPIOPadConfigSet(GPIO_PORTA_BASE, GPIO_PIN_5, GPIO_STRENGTH_4MA , GPIO_PIN_TYPE_STD_WPD ) ;//Set the pin current strength (default is 2mA) and Pin Type to Pull Down
	GPIOPadConfigSet(GPIO_PORTA_BASE, GPIO_PIN_7, GPIO_STRENGTH_4MA , GPIO_PIN_TYPE_STD_WPU ) ;//Set the pin current strength (default is 2mA) and Pin Type to Pull Up

	initializeLCD();// initialize LCD

	writeLCD(text[index],text[index2],timesArray[index],timesArray[index2]); //write both lines

	/* This while makes sure that we are always checking if any of the button was pressed */
	while(1){

		//Pull Down push button
		//While Pin5 Pressed(1)
		while(!(GPIOPinRead(GPIO_PORTA_BASE, GPIO_PIN_5))){
			//If Pressed
			SysCtlDelay(4000000); // Delay for Debouncing



			index = index +1;//increment index
			index2 = index2 +1;//increment second word index
			index = checkIndex(index);//index
			index2 = checkIndex(index2);//check index2

			writeLCD(text[index],text[index2],timesArray[index],timesArray[index2]); //writing both lines function
			SysCtlDelay(4000000);// Delay for debouncing
		}

		//Pull Up (Push Button)
		//While Pin7 Pressed(0)
		while((GPIOPinRead(GPIO_PORTA_BASE, GPIO_PIN_7))){
			SysCtlDelay(4000000);// Delay for debouncing


			index = index-1;//decrement index
			index2 = index2 -1;//decrement index2
			index = checkIndex(index);//index
			index2 = checkIndex(index2);//check index2

			writeLCD(text[index],text[index2],timesArray[index],timesArray[index2]); //writing both lines function
			SysCtlDelay(4000000);//Delay for debouncing
		}
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
	GPIOPinWrite(GPIO_PORTE_BASE, GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3, 0x08); //R/S & R/W = 0
	GPIOPinWrite(GPIO_PORTB_BASE, ALLPINS, command); //enter command on port
	GPIOPinWrite(GPIO_PORTE_BASE, GPIO_PIN_3, 0x08);//Enable On
	SysCtlDelay(delayTime);//value for the desired delay
	GPIOPinWrite(GPIO_PORTE_BASE, GPIO_PIN_3, 0x00);//Enable Off
}

/* LCD writng function, string is the first word that wants to be written, string2 the word that wants to be written in the second line, times is the
 * number of characters that the first word has 7 times2 is the number of characters that the second word has  */
void writeLCD(uint8_t string[],uint8_t string2[], int times, int times2)
{
	enterCommand(0x01,20400);//Clear LCD

	writeWord(string,times);//Write current index word

	enterCommand(0xC0,20400);//move cursor to second row

	writeWord(string2,times2);//write second word
}

/* Function for writing both lines in the LCD, string is the word that want to be written, times is the number of characters that the string has */
void writeWord(uint8_t string[], uint8_t times)
{
	GPIOPinWrite(GPIO_PORTE_BASE, GPIO_PIN_1, 0x02);//RS enable

	uint8_t i =0;//initialize for loop
	for(; i < times; i++){
		GPIOPinWrite(GPIO_PORTB_BASE, ALLPINS, string[i]);//write character
		GPIOPinWrite(GPIO_PORTE_BASE, GPIO_PIN_3, 0x08);//Enable On
		SysCtlDelay(94666);//Delay 7.1ms(Calculated)
		GPIOPinWrite(GPIO_PORTE_BASE, GPIO_PIN_3, 0x00);//Enable Off
	}

	GPIOPinWrite(GPIO_PORTE_BASE, GPIO_PIN_1, 0x00);//RS disable
}
/* Function for controlling the current written word; this functions checks if the index is less than 0 then sets the index to the last element in the array, if the
 * index is more than 15 then sets the index to 0. This makes the circular list posible */
int checkIndex(int indexChecked)
{
	if(indexChecked > 15 ){ //check if index is greater than 15
		return 0;	// if its true return 0
	}else{
		if(indexChecked <0){ //check if index is less than 0
			return 15;// if its true return 15
		}
		return indexChecked; // If the index is between 0 and 15 return the same index.
	}
}
