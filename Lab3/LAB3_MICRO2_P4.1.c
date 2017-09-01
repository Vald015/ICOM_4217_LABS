/*
 * ICOM 4217 (EMBEDDED SYSTEM DESIGN)
 * Authors: Carlos A. Rodriguez & Osvaldo A. Ramirez
 * LAB3 PART4: 	Reading Keypads Through Interrupts
 * Instructions:
 * Read the input state of a keypad using a scan algorithm and display it on the LCD.
 */

/* include necessary system header files */
#include <stdint.h>
#include <stdbool.h>
#include "inc/hw_types.h"
#include "inc/hw_memmap.h"
#include "driverlib/sysctl.h"
#include "driverlib/gpio.h"

/* function declaration */
void PB1_IntHandler();								//Interrupt keypad handler
void initializeLCD();								//initialize LCD function declaration
void enterCommand(int command,float delayTime);		//enter command function declaration
void writeWord(uint8_t printThis[]);				//writeWord function declaration
void indexFix(int fixMe);							//Fix index function in order to print the right number in the LCD
void changeRow();									//Change row function
void writeRow();

uint8_t numbers [12][1] = {"1","2","3","4","5","6","7","8","9","*","0","#"};	//keypad numbers
uint8_t signal = 0x02;		//send voltage to each row
uint8_t interruptSignal;	//which row was on then the interrupt occurred
uint8_t status =0x00;		//Status is the column that the interrupt occurred.
int index;					//index of the word that wants to be written
int FLAG = 0;				//interrupt flag
int row=0;					//Cursor's current row in LCD

/* define label ALLPINS as all the pins in a port */
#define ALLPINS GPIO_PIN_0|GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3|GPIO_PIN_4|GPIO_PIN_5|GPIO_PIN_6|GPIO_PIN_7

/* Main code of the program. */
int main(void)
{
	/* Variable declaration */

	/* Clock frequency configuration */
	SysCtlClockSet(SYSCTL_SYSDIV_5|SYSCTL_USE_PLL|SYSCTL_XTAL_16MHZ|SYSCTL_OSC_MAIN);// Set clock frequency to 40 MHz.

	/* Peripheral Configuration */
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOB);		//Enable the peripheral port to use. (B)
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOC); 		//Enable the peripheral port to use. (C)
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOE);		//Enable the peripheral port to use. (E)
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF); 		//Enable the peripheral port to use. (F)

	/* Input/output configuration */
	GPIOPinTypeGPIOOutput(GPIO_PORTB_BASE, ALLPINS);										// Set all port B pins as output pins
	GPIOPinTypeGPIOOutput(GPIO_PORTE_BASE,GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3|GPIO_PIN_4);		//Set as Outputs ports PE1,PE2,PE3,PE4 for the keypad
	GPIOPinTypeGPIOOutput(GPIO_PORTF_BASE, GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3);				//PIN1 : R/S (R/S: 1 for data ** 0 for instruction) *** PIN2 : R/W (R/S: 1 for Read ** 0 for write) *** PIN3 : E(Chip Enable signal).

	/*Interrupt Button 1 GPIO PORTA PIN 5*/
	GPIOPinTypeGPIOInput(GPIO_PORTC_BASE,GPIO_PIN_4|GPIO_PIN_5|GPIO_PIN_6); 				//Input configuration PC4,PC5,PC6
	GPIOIntRegister(GPIO_PORTC_BASE, PB1_IntHandler); 										//Registers an interrupt handler for a GPIO port. Ensures that the interrupt handler specified is called.
	GPIOIntTypeSet(GPIO_PORTC_BASE,GPIO_PIN_4| GPIO_PIN_5|GPIO_PIN_6,GPIO_RISING_EDGE); 	//Sets the interrupt type for the specified pin(s). GPIO_FALLING_EDGEsets detection to edge and trigger to falling(high to low transition)
	GPIOIntEnable(GPIO_PORTC_BASE, GPIO_PIN_4| GPIO_PIN_5|GPIO_PIN_6);						//Enable interrupts in GPIO ports PC4,PC5,PC6

	initializeLCD();// initialize LCD


	while(1){
		SysCtlDelay(5000);
		GPIOPinWrite(GPIO_PORTE_BASE,ALLPINS,signal);			//Send signal to rows
		if(FLAG && status != 0x00){								//if the flag was set
			if(status == 0x10){									//First column interrupted
				if(interruptSignal == 16){						//If the star was pressed on the keyboard
					enterCommand(0x01,20400);					//Clear display command
				}else{
					indexFix(0);								//fix index in row 1
					writeRow();
					//writeWord(numbers[index]);					//Write number
				}
			}else{
				if(status == 0x20){								//Second column interrupted
					if(interruptSignal == 16){					//If zero is pressed on keyboard
						interruptSignal =12;					//Fix interrupt signal so that the index formula could work
					}
					indexFix(1);								//fix index in row 2
					writeRow();
					//writeWord(numbers[index]);					//write number

				}else{
					if(status == 0x40){							//Thrid column interrupted
						if(interruptSignal ==16){				//If the number sign was pressed
							changeRow();						//Change row method
						}else{
							indexFix(2);						//Fix index row 3
							writeRow();
							//writeWord(numbers[index]);			//write number
						}
					}
				}
			}
			FLAG = 0;					//Clear interrupt flag
			status = 0x00;				//Clear status
		}
		SysCtlDelay(5000);
		if(signal<= 0x10){				//If the signal is less than the last row
			signal = signal*2;			//send 1 to next row
		}else{
			signal = 0x02;				//if the last row has a 1 then switch to the first row
		}
	}
}
/* Interrupt handler */
void PB1_IntHandler (void)
{
	SysCtlDelay(4000000);												//Debouncing
	status = GPIOIntStatus(GPIO_PORTC_BASE,true);						//Store status of PORT C
	interruptSignal = GPIOPinRead(GPIO_PORTE_BASE, ALLPINS);			//Store status of Port E
	FLAG = 1;															//Setup flag
	GPIOIntClear(GPIO_PORTC_BASE, GPIO_PIN_4| GPIO_PIN_5|GPIO_PIN_6);	// Clear
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
	GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_1|GPIO_PIN_2, 0x00); //R/S & R/W = 0
	GPIOPinWrite(GPIO_PORTB_BASE, ALLPINS, command); 			//enter command on port
	GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_3, 0x08);			//Enable On
	SysCtlDelay(delayTime);										//value for the desired delay
	GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_3, 0x00);			//Enable Off
}

/* Function for writing words in the LCD, string is the word that want to be written, times is the number of characters that the string has */
void writeWord(uint8_t printThis[])
{
	GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_1, 0x02);		//RS enable
	GPIOPinWrite(GPIO_PORTB_BASE, ALLPINS, printThis[0]); 	//write character
	GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_3, 0x08);		//Enable On
	SysCtlDelay(94666);										//Delay 7.1ms(Calculated)
	GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_3, 0x00);		//Enable Off
	GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_1, 0x00);		//RS disable
}

/* Function for fixing index in order to print the right number pressed */
void indexFix(int fixMe){
	index = fixMe + (interruptSignal/4 * 3);		//function to fix the index
}

/* Function for controlling the change of cursor  */
void changeRow(){
	if(row == 0){		//if the cursor is at the first row
		enterCommand(0xC0,20400);		//change cursor to second row
		row = 1;						//change current cursor position index
	}else{
		enterCommand(0x80,20400);		//if cursor is at second row change to first row
		row =0;							//change cursor to first row
	}
}
void writeRow(){
	if(index%2==1){
		enterCommand(0x80,20400);
		writeWord(numbers[index]);
	}else{
		enterCommand(0xC0,20400);
		writeWord(numbers[index]);
	}
}
