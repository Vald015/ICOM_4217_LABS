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
#include "inc/tm4c123gh6pm.h"
#include "driverlib/debug.h"
#include "driverlib/pwm.h"
#include "driverlib/pin_map.h"
#include "inc/hw_gpio.h"

#define PWM_FREQUENCY 1000

/* function declaration */
void PB1_IntHandler();								//Interrupt keypad handler
void initializeLCD();								//initialize LCD function declaration
void enterCommand(int command,float delayTime);		//enter command function declaration
void writeWord(uint8_t string[], uint8_t times);				//writeWord function declaration
void indexFix(int fixMe);							//Fix index function in order to print the right number in the LCD
void changeRow();									//Change row function
//void writeRow();
void writeWord2(uint8_t string[], uint8_t times);


uint8_t numbers [12][4] = {"10% ","20% ","30% ","40% ","50% ","60% ","70% ","80% ","90% ","*","0%  ","100%"};	//keypad numbers
int timesArray2[12] = {4,4,4,4,4,4,4,4,4,4,4,4};
uint8_t signal = 0x02;		//send voltage to each row
uint8_t interruptSignal;	//which row was on then the interrupt occurred
uint8_t status =0x00;		//Status is the column that the interrupt occurred.
int index;					//index of the word that wants to be written
int FLAG = 0;				//interrupt flag
int row=0;					//Cursor's current row in LCD
uint8_t text[16][16] = {{"Level Selected: "}};
int timesArray[5] = {16};
int width;

/* define label ALLPINS as all the pins in a port */
#define ALLPINS GPIO_PIN_0|GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3|GPIO_PIN_4|GPIO_PIN_5|GPIO_PIN_6|GPIO_PIN_7

/* Main code of the program. */
int main(void)
{

	volatile uint32_t load;
	volatile uint32_t PWMClock;
	float dC[12] = {0.1,0.2,0.3,0.4,0.5,0.6,0.7,0.8,0.9,0,0,1};
	/* Variable declaration */

	/* Clock frequency configuration */
	SysCtlClockSet(SYSCTL_SYSDIV_5|SYSCTL_USE_PLL|SYSCTL_XTAL_16MHZ|SYSCTL_OSC_MAIN);// Set clock frequency to 40 MHz.
	SysCtlPWMClockSet(SYSCTL_PWMDIV_2);

	/* Peripheral Configuration */
	SysCtlPeripheralEnable(SYSCTL_PERIPH_PWM0);

	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOB);		//Enable the peripheral port to use. (B) // LCD
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOC); 		//Enable the peripheral port to use. (C) //Columns
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOE); 		//Enable the peripheral port to use. (E)//LCD R/W R/S E
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);		//Enable the peripheral port to use. (F) // Rows
	/* Input/output configuration */
	GPIOPinTypeGPIOOutput(GPIO_PORTF_BASE,GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3|GPIO_PIN_4);		//Set as Outputs ports PE1,PE2,PE3,PE4 for the keypad
	GPIOPinTypeGPIOOutput(GPIO_PORTB_BASE, ALLPINS);										// Set all port B pins as output pins
	GPIOPinTypeGPIOOutput(GPIO_PORTE_BASE, GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3);				//PIN1 : R/S (R/S: 1 for data ** 0 for instruction) *** PIN2 : R/W (R/S: 1 for Read ** 0 for write) *** PIN3 : E(Chip Enable signal).

	/*PWM Config*/

	GPIOPinTypePWM(GPIO_PORTE_BASE,GPIO_PIN_5);
	GPIOPinConfigure(GPIO_PE5_M0PWM5);

	PWMClock = SysCtlClockGet()/2;
	load = (PWMClock/PWM_FREQUENCY) -1;

	PWMGenConfigure(PWM0_BASE,PWM_GEN_2,PWM_GEN_MODE_DOWN);
	PWMGenPeriodSet(PWM0_BASE,PWM_GEN_2,load);

	width = load*dC[index];
	PWMPulseWidthSet(PWM0_BASE,PWM_OUT_5, width);
	PWMOutputState(PWM0_BASE,PWM_OUT_5_BIT,false);
	PWMGenEnable(PWM0_BASE,PWM_GEN_2);


	/*Interrupt Button 1 GPIO PORTA PIN 5*/
	GPIOPinTypeGPIOInput(GPIO_PORTC_BASE,GPIO_PIN_4|GPIO_PIN_5|GPIO_PIN_6); 				//Input configuration PC4,PC5,PC6
	GPIOIntRegister(GPIO_PORTC_BASE, PB1_IntHandler); 										//Registers an interrupt handler for a GPIO port. Ensures that the interrupt handler specified is called.
	GPIOIntTypeSet(GPIO_PORTC_BASE,GPIO_PIN_4| GPIO_PIN_5|GPIO_PIN_6,GPIO_RISING_EDGE); 	//Sets the interrupt type for the specified pin(s). GPIO_FALLING_EDGEsets detection to edge and trigger to falling(high to low transition)
	GPIOIntEnable(GPIO_PORTC_BASE, GPIO_PIN_4| GPIO_PIN_5|GPIO_PIN_6);						//Enable interrupts in GPIO ports PC4,PC5,PC6

	initializeLCD();// initialize LCD
	writeWord2(text[0],timesArray[0]);


	while(1){
		SysCtlDelay(5000);
		GPIOPinWrite(GPIO_PORTF_BASE,ALLPINS,signal);			//Send signal to rows
		if(FLAG && status != 0x00){								//if the flag was set
			if(status == 0x10){									//First column interrupted
				if(interruptSignal == 16){						//If the star was pressed on the keyboard
					enterCommand(0x01,20400);					//Clear display command
					writeWord2(text[0],timesArray[0]);
					PWMOutputState(PWM0_BASE,PWM_OUT_5_BIT,false);

				}else{
					indexFix(0);								//fix index in row 1
					//writeRow();		//write the value to the LCD
					enterCommand(0xC0,5000);
					writeWord(numbers[index],timesArray2[index]);					//Write number
					int width = load*dC[index];
					PWMPulseWidthSet(PWM0_BASE,PWM_OUT_5, width);
					PWMOutputState(PWM0_BASE,PWM_OUT_5_BIT,true);
				}
			}else{
				if(status == 0x20){								//Second column interrupted
					if(interruptSignal == 16){					//If zero is pressed on keyboard
						interruptSignal =12;					//Fix interrupt signal so that the index formula could work
					}
					indexFix(1);								//fix index in row 2
					if(index == 10){
						PWMOutputState(PWM0_BASE,PWM_OUT_5_BIT,false);
					}else{
					//writeRow();	//write the value to the LCD
						int width = load*dC[index];
						PWMPulseWidthSet(PWM0_BASE,PWM_OUT_5, width);
						PWMOutputState(PWM0_BASE,PWM_OUT_5_BIT,true);
					}
					enterCommand(0xC0,5000);
					writeWord(numbers[index],timesArray2[index]);				//write number

				}else{
					if(status == 0x40){							//Thrid column interrupted

						indexFix(2);						//Fix index row 3
						//writeRow();
						//write the value to the LCD
						enterCommand(0xC0,5000);
						if(index==14){
							index = 11;
							writeWord(numbers[index],timesArray2[index]);
							int width = load*dC[index];
							PWMPulseWidthSet(PWM0_BASE,PWM_OUT_5, width);
							PWMOutputState(PWM0_BASE,PWM_OUT_5_BIT,true);
						}
						else{
							writeWord(numbers[index],timesArray2[index]);
						int width = load*dC[index];
						PWMPulseWidthSet(PWM0_BASE,PWM_OUT_5, width);
						PWMOutputState(PWM0_BASE,PWM_OUT_5_BIT,true);
						//write number
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
	status = GPIOIntStatus(GPIO_PORTC_BASE ,true);						//Store status of PORT C
	interruptSignal = GPIOPinRead(GPIO_PORTF_BASE, ALLPINS);			//Store status of Port E
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


/* Function for writing words in the LCD, string is the word that want to be written, times is the number of characters that the string has */
void writeWord2(uint8_t string[], uint8_t times)
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
