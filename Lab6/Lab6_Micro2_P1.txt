

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
#define motorRight 0x10
#define motorLeft 0x20
#define motorOff 0x00

void PB1_IntHandler();
void initializeLCD();								//initialize LCD function declaration
void enterCommand(int command,float delayTime);		//enter command function declaration
void writeWord(uint8_t string[], uint8_t times);				//writeWord function declaration

uint8_t status =0x00;		//Status is the column thats the interrupt occurred.
int button = 0;
uint8_t text[16][16] = {{"Status: "}, {"Moving Right "}, {"Moving Left "}, {"Motor Off "}};
int timesArray[5] = {8,13,12,10};

void main(void){

	SysCtlClockSet(SYSCTL_SYSDIV_5|SYSCTL_USE_PLL|SYSCTL_XTAL_16MHZ|SYSCTL_OSC_MAIN);		// Set clock frequency to 40 MHz.

	/* System peripheral enable */
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);		//Enable the peripheral port to use/ (A) // Buttons for Interruption
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOB);		//Enable the peripheral port to use. (B) // LCD
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOC); 		//Enable the peripheral port to use. (C) //DC MOTOR
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOE); 		//Enable the peripheral port to use. (E) //LCD R/W R/S E


	/* LCD pin configuration */
	GPIOPinTypeGPIOOutput(GPIO_PORTB_BASE, ALLPINS);										// Set all port B pins as output pins
	GPIOPinTypeGPIOOutput(GPIO_PORTE_BASE, GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3);				//PIN1 : R/S (R/S: 1 for data ** 0 for instruction) *** PIN2 : R/W (R/S: 1 for Read ** 0 for write) *** PIN3 : E(Chip Enable signal).

	/* DC Motor pin configuration */
	GPIOPinTypeGPIOOutput(GPIO_PORTC_BASE, GPIO_PIN_4|GPIO_PIN_5);							//DC MOTOR

	/*Interrupt Buttons GPIO PORTA PIN 5 & 7*/
	GPIOPinTypeGPIOInput(GPIO_PORTA_BASE, GPIO_PIN_4|GPIO_PIN_5|GPIO_PIN_7);							//Input configuration PA5, PA7

	GPIOIntRegister(GPIO_PORTA_BASE, PB1_IntHandler); 										//Registers an interrupt handler for a GPIO port. Ensures that the interrupt handler specified is called.
	//GPIOIntTypeSet(GPIO_PORTA_BASE,GPIO_PIN_4|GPIO_PIN_5|GPIO_PIN_7,GPIO_RISING_EDGE);
	//Sets the interrupt type for the specified pin(s). GPIO_FALLING_EDGEsets detection to edge and trigger to falling(high to low transition)
	GPIOIntTypeSet(GPIO_PORTA_BASE, GPIO_PIN_5, GPIO_RISING_EDGE);
	GPIOIntTypeSet(GPIO_PORTA_BASE, GPIO_PIN_7, GPIO_FALLING_EDGE);
	GPIOIntTypeSet(GPIO_PORTA_BASE, GPIO_PIN_4, GPIO_FALLING_EDGE);

	GPIOIntEnable(GPIO_PORTA_BASE, GPIO_PIN_4|GPIO_PIN_5|GPIO_PIN_7);




	initializeLCD();
	writeWord(text[0],timesArray[0]);			//Write number selected
	while(1){
		SysCtlDelay(4000000);   // Delay for Debouncing
		if(status!=0){
			GPIOIntClear(GPIO_PORTA_BASE, GPIO_PIN_4|GPIO_PIN_5|GPIO_PIN_7);
			if(button == 0x20){  				   				// Button A5 Pressed
				status = 0x00;
				enterCommand(0x01,20400);
				writeWord(text[0],timesArray[0]);			//Write number selected
				enterCommand(0xC0,5000);
				writeWord(text[2],timesArray[2]);			//Write number selected
				GPIOPinWrite(GPIO_PORTC_BASE,GPIO_PIN_4|GPIO_PIN_5,motorLeft); 				//Motor Left 0x20


			}
			else if(button == 0x80){			    			//Button A7 Pressed
				status = 0x00;
				enterCommand(0x01,20400);
				writeWord(text[0],timesArray[0]);			//Write number selected
				enterCommand(0xC0,5000);
				writeWord(text[1],timesArray[1]);			//Write number selected
				GPIOPinWrite(GPIO_PORTC_BASE,GPIO_PIN_4|GPIO_PIN_5,motorRight); 			// Motor Right 0x10

			}
			else if(button == 0x10){			  				//Button Launchapd Pressed
				status = 0x00;
				enterCommand(0x01,20400);
				writeWord(text[0],timesArray[0]);			//Write number selected
				enterCommand(0xC0,5000);
				writeWord(text[3],timesArray[3]);			//Write number selected
				GPIOPinWrite(GPIO_PORTC_BASE,GPIO_PIN_4|GPIO_PIN_5,motorOff); 				// Motor Off 0x00

			}
			button = 0;
			status = 0;

		}


	}


}
/* Interrupt handler */
void PB1_IntHandler (void)
{
	SysCtlDelay(4000000);												//Debouncing
	status = 1;
	button = GPIOIntStatus(GPIO_PORTA_BASE,true);
	GPIOIntClear(GPIO_PORTA_BASE, GPIO_PIN_4|GPIO_PIN_5|GPIO_PIN_7);	// Clear
}

/* LCD initialization function */
void initializeLCD()
{
	SysCtlDelay(533333.3333);		// Starting delay (40ms).

	enterCommand(0x3C,520);			//Function Set command

	enterCommand(0x3C,493.3);		//Function Set

	enterCommand(0x0F,493.3);		//Display ON

	enterCommand(0x01,20400);		//Clear display command

	enterCommand(0x06,20400);		//Entry Mode Set

	enterCommand(0x01,20400);		//Clear display command
}

/* Enter command to LCD function; command is the desired LCD instruction, delayTime is the required time that the chip enable needs to be on in order to
 * successfully send the instruction */
void enterCommand(int command,float delayTime)
{
	GPIOPinWrite(GPIO_PORTE_BASE, GPIO_PIN_1|GPIO_PIN_2, 0x00); //R/S & R/W = 0
	GPIOPinWrite(GPIO_PORTB_BASE, ALLPINS, command); 			//enter command on port
	GPIOPinWrite(GPIO_PORTE_BASE, GPIO_PIN_3, 0x08);			//Enable On
	SysCtlDelay(delayTime);										//value for the desired delay
	GPIOPinWrite(GPIO_PORTE_BASE, GPIO_PIN_3, 0x00);			//Enable Off
}

/* Function for writing words in the LCD, string is the word that want to be written, times is the number of characters that the string has */
void writeWord(uint8_t string[], uint8_t times)
{
	GPIOPinWrite(GPIO_PORTE_BASE, GPIO_PIN_1, 0x02);//RS enable

	uint8_t i =0;//initialize for loop
	for(; i < times; i++){
		GPIOPinWrite(GPIO_PORTB_BASE, ALLPINS, string[i]);//write character
		GPIOPinWrite(GPIO_PORTE_BASE, GPIO_PIN_3, 0x08);//Enable On
		SysCtlDelay(50000);//Delay 7.1ms(Calculated)
		GPIOPinWrite(GPIO_PORTE_BASE, GPIO_PIN_3, 0x00);//Enable Off
	}
	GPIOPinWrite(GPIO_PORTE_BASE, GPIO_PIN_1, 0x00);//RS disable
}
