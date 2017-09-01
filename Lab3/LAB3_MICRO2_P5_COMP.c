/*
 * ICOM 4217 (EMBEDDED SYSTEM DESIGN)
 * Authors: Carlos A. Rodriguez & Osvaldo A. Ramirez
 * LAB3 PART5: COMPLEMENTARY TASK
 * Scrolling List with wheel
 * Instructions:
 * The activity consists of genereating a scrolling list of messages and display them on the LCD.
 * To detect movement direction, build an encoder wheel.
 */

/* include necessary system header files */
#include <stdint.h>
#include <stdbool.h>
#include "inc/hw_types.h"
#include "inc/hw_memmap.h"
#include "driverlib/sysctl.h"
#include "driverlib/gpio.h"

/* function declaration */
void PA1_IntHandler();														//Interrupt keypad handler
void initializeLCD();														//initialize LCD function declaration
void writeLCD(uint8_t string[],uint8_t string2[], int times, int times2);	//Write the two lines of the LCD
void enterCommand(int command,float delayTime);								//enter command function declaration
void writeWord(uint8_t string[], uint8_t times);							//writeWord function declaration
void checkIndex();															//Fix when index is higher than 15 or lower than 0

uint8_t text[16][16] = {{"Jump"}, {"Walk"},{"Run"},{"Kick"}, {"Dance"}, {"Swim"}, {"Sleep"}, {"Fly"}, {"Punch"}, {"Write"}, {"Read"}, {"Pull"}, {"Push"}, {"Press"}, {"Roll"}, {"Drink"}};//Words to be written in LCD
int timesArray[16] = {4,4,3,4,5,4,5,3,5,5,4,4,4,5,4,5}; //text[] words lenght
int value[16] = {0,1,-1,0,-1,0,0,1,1,0,0,-1,0,-1,1,0};	//Values for the different types of interrupts
uint8_t status = 0x00;						//Set Interrupt status to zero
int oldA = 0;					//Set old A to 0
int newA = 0;					//Set new A to 0
int oldB = 0;					//Set old B to 0
int newB = 0;					//Set new B to 0
uint8_t index = 0;				//Value array index
int topIdx = 0;					//First line in LCD index
int botIdx = 1;					//Second line in LCD index

/* define label ALLPINS as all the pins in a port */
#define ALLPINS GPIO_PIN_0|GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3|GPIO_PIN_4|GPIO_PIN_5|GPIO_PIN_6|GPIO_PIN_7

/* Main code of the program. */
int main(void)
{
	/* Variable declaration */

	/* Clock frequency configuration */
	SysCtlClockSet(SYSCTL_SYSDIV_5|SYSCTL_USE_PLL|SYSCTL_XTAL_16MHZ|SYSCTL_OSC_MAIN);// Set clock frequency to 40 MHz.

	/* Peripheral Configuration */
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);		//Enable the peripheral port to use. (A)
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOB);		//Enable the peripheral port to use. (B)
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF); 		//Enable the peripheral port to use. (F)

	/* Input/output configuration */
	GPIOPinTypeGPIOOutput(GPIO_PORTB_BASE, ALLPINS);										// Set all port B pins as output pins
	//GPIOPinTypeGPIOOutput(GPIO_PORTE_BASE,GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3|GPIO_PIN_4);		//Set as Outputs ports PE1,PE2,PE3,PE4 for the keypad
	GPIOPinTypeGPIOOutput(GPIO_PORTF_BASE, GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3);				//PIN1 : R/S (R/S: 1 for data ** 0 for instruction) *** PIN2 : R/W (R/S: 1 for Read ** 0 for write) *** PIN3 : E(Chip Enable signal).

	/* Interrupt configuration */
	GPIOPinTypeGPIOInput(GPIO_PORTA_BASE,GPIO_PIN_2|GPIO_PIN_3); 											//Input configuration PC4,PC5,PC6
	GPIOPadConfigSet(GPIO_PORTA_BASE, GPIO_PIN_2|GPIO_PIN_3, GPIO_STRENGTH_4MA , GPIO_PIN_TYPE_STD_WPD ) ;	//Set the pin current strength (default is 4mA) and Pin Type to Pull DOWN
	GPIOIntRegister(GPIO_PORTA_BASE, PA1_IntHandler); 														//Registers an interrupt handler for a GPIO port. Ensures that the interrupt handler specified is called.
	GPIOIntTypeSet(GPIO_PORTA_BASE,GPIO_PIN_2|GPIO_PIN_3,GPIO_BOTH_EDGES); 									//Sets the interrupt type for the specified pin(s). GPIO_FALLING_EDGEsets detection to edge and trigger to falling(high to low transition)
	GPIOIntEnable(GPIO_PORTA_BASE, GPIO_PIN_2|GPIO_PIN_3);													//Enable interrupts in GPIO ports PC4,PC5,PC6

	initializeLCD();							// initialize LCD
	writeLCD(text[topIdx],text[botIdx],timesArray[topIdx],timesArray[botIdx]);	//Write the first two word on list

	while(1){

		if(status != 0){

			if(status == 0x8){ 				//Outside sensor is sensor A
				if(oldA == 0){				//If the last interrupt is 0, set it to one
					newA = 2;				//Set new A as one
				}else{
					newA = 0;				//Else set interrupt to zero
				}
			} else if(status == 0x4){	//Inside sensor is sensor B
				if(oldB == 0){			//If last interrupt is 0, set it to one
					newB = 1;			//Set new B to one
				}else{
					newB = 0;			//Else set interrupt to zero
				}
			}
			index = (oldA+oldB)*4 +newA+newB;		//Get Value array index

			oldA = newA;					//Set oldA to the value of newA
			oldB = newB;					//Set oldB to the value of newB
			status =0;						//Set status to zero
			topIdx += value[index];			//Modify the list array pointer value for the first element to be written in the LCD
			botIdx += value[index];			//Modify the list array pointer value for the second element to be written in the LCD
			checkIndex();					//Fix index
			writeLCD(text[topIdx],text[botIdx],timesArray[topIdx],timesArray[botIdx]);	//Write the topIdx and botIdx
		}
	}
}

/* Interrupt handler */
void PA1_IntHandler (void)
{
	status = GPIOIntStatus(GPIO_PORTA_BASE,true);							//Store the value of the Port A
	GPIOIntClear(GPIO_PORTA_BASE, GPIO_PIN_2|GPIO_PIN_3);					// Clear interrupt flag
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
	GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_3, 0x00);		//Enable Off
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

/* Fix index of the list array. If the index is higher than 15 change it to 0, otherwise if the index is lower than 0 set it to 15*/
void checkIndex(){
	if( topIdx < 0){		//topIndx lower than 0
		topIdx = 15;		//Set topIdx to 15
	}else if(topIdx>15){	//topIdx higher than 15
		topIdx = 0;			//Set topIdx to 0
	}
	if( botIdx < 0){		//botIdx lower than 0
		botIdx = 15;		//Set botIdx to 15
	}else if(botIdx>15){	//botIdx higher than 15
		botIdx = 0;			//Set botIdx to 0
	}
}

