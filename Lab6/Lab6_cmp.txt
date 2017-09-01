/*
 * ICOM 4217 (EMBEDDED SYSTEM DESIGN)
 * Authors: Carlos A. Rodriguez & Osvaldo A. Ramirez
 * LAB6 Part 5 Complementary Task: Stepper Motor Characterization
 * Instructions:
 * This activity consists in determining the maximum input signal frequency in which the stepper
 * motor can work without missing steps. The system shall use keys UP and DOWN to increase and reduce the
 * motor speed, and START/STOP to turn the motor on and off.
 *
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

//****************************** Global Variable & Function Declaration****************************

void PB1_IntHandler();
void initializeLCD();								//initialize LCD function declaration
void enterCommand(int command,float delayTime);		//enter command function declaration
void writeWord(uint8_t string[], uint8_t times);				//writeWord function declaration
void moveStep(int value[], int steps);									//Function for moving the stepper motor to the left
void moveStepRight(int value[],int steps);								//Function for moving the stepper motor to the right
float calculate(float number);
void checkIndex(void);

#define ALLPINS GPIO_PIN_0|GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3|GPIO_PIN_4|GPIO_PIN_5|GPIO_PIN_6|GPIO_PIN_7 // define ALLPINS as all the pins in a port

/* Full-Step sequence */
#define Orange 1			// 0001 (Send a '1' to the A coil)
#define Yellow 2			// 0010 (Send a '1' to the B coil)
#define Pink   4			// 0100 (Send a '1' to the A' coil)
#define Blue   8			// 1000 (Send a '1' to the B' coil)

/* Half-Step sequence */
#define Half1  3			// 0011 (Send a '1' to the A & B coils)
#define Half2  6			// 0110 (Send a '1' to the B & A' coils)
#define Half3  12			// 1100 (Send a '1' to the A' & B' coils)
#define Half4  9			// 1001 (Send a '1' to the A & B' coils)

int value[8] = {Orange, Half1, Yellow, Half2, Pink, Half3 , Blue, Half4} ; 	//Half-Step sequence
int value1[8] = {Orange, Yellow, Pink,Blue}; 								//Full-Step sequence ** 32 full step = 90 degrees **
int index = 0;																//value arrays index for when the stepper is moving left
int index2 = 3;																//value arrays index for when the stepper is moving right
int counter = 0;															//Steps taken counter
int started = 0;															//Cero if its doing nothing, 1 if it was pressed for the first time
uint8_t status =0x00;
uint8_t statusVel =0x00;
int start = 0;
int current =0;

uint8_t text[16][16] = {{"Start"}, {"Stop "}, {"Up "}, {"Down "}, {"Current Vel: "}};
int times[9] = {5,5,3,5,12};
uint8_t text1[16][16] = {{"100"},{"50 "},{"20 "},{"10 "},{"5  "},{"2  "},{"1  "},{"0.5"},{"0.2"},{"0.1"}};

int indexMs = 0;
float msArray[10] ={100,50,20,10,5,2,1,0.5,0.2,0.1};

//*************************************************************************************************

//------------------------------------Main Function------------------------------------------------

void main(void){

	volatile uint32_t load;
	volatile uint32_t PWMClock;

	SysCtlClockSet(SYSCTL_SYSDIV_5|SYSCTL_USE_PLL|SYSCTL_XTAL_16MHZ|SYSCTL_OSC_MAIN);		// Set clock frequency to 40 MHz.

	/* System peripheral enable */
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);		//Enable the peripheral port to use (A) // Buttons for Interruption
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOB);		//Enable the peripheral port to use (B)
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOD);		//Enable the peripheral port to use (D)
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOE); 		//Enable the peripheral port to use. (E) //LCD R/W R/S E
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);

	/* LCD pin configuration */
	GPIOPinTypeGPIOOutput(GPIO_PORTB_BASE, ALLPINS);										// Set all port B pins as output pins
	GPIOPinTypeGPIOOutput(GPIO_PORTD_BASE,GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3|GPIO_PIN_0);
	GPIOPinTypeGPIOOutput(GPIO_PORTE_BASE, GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3);

	/*Pins for Steps configuration*/
	//GPIOPinTypeGPIOOutput(GPIO_PORTB_BASE, GPIO_PIN_0|GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3); 	//Set PORT B PINS 0,1,2,3 as output pins for controlling the stepper motor

	/*Interrupt Buttons GPIO PORTA PIN 4, 5 & 7*/
	GPIOPinTypeGPIOInput(GPIO_PORTA_BASE, GPIO_PIN_4|GPIO_PIN_5|GPIO_PIN_7);

	//Input configuration  PA4, PA5, PA7

	GPIOIntRegister(GPIO_PORTA_BASE, PB1_IntHandler); 										//Registers an interrupt handler for a GPIO port. Ensures that the interrupt handler specified is called.
	//GPIOIntTypeSet(GPIO_PORTA_BASE,GPIO_PIN_4|GPIO_PIN_5|GPIO_PIN_7,GPIO_RISING_EDGE);
	//Sets the interrupt type for the specified pin(s). GPIO_FALLING_EDGEsets detection to edge and trigger to falling(high to low transition)
	GPIOIntTypeSet(GPIO_PORTA_BASE, GPIO_PIN_5, GPIO_RISING_EDGE);
	GPIOIntTypeSet(GPIO_PORTA_BASE, GPIO_PIN_7, GPIO_FALLING_EDGE);
	GPIOIntTypeSet(GPIO_PORTA_BASE, GPIO_PIN_4, GPIO_FALLING_EDGE);


	/* Interrupt & PWM signal Enable */
	GPIOIntEnable(GPIO_PORTA_BASE, GPIO_PIN_4|GPIO_PIN_5|GPIO_PIN_7);

	initializeLCD();
	enterCommand(0xC0,20400);		//Change DDRAM
	writeWord(text[4], times[4]);

//	while(1){
//		GPIOPinWrite(GPIO_PORTD_BASE,GPIO_PIN_0|GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3,Orange);
//	}

	while(1){
		if(status == 16){
			checkIndex();
			current = indexMs;
//			enterCommand(0xCD,20400);		//Change DDRAM
//			writeWord(text1[current], 3);
				if(start == 0){
//					enterCommand(0x80,20400);		//Change DDRAM
//					writeWord(text[0], times[0]);
					start = 1;
					moveStep(value1,256);				//90*8=720 degrees (Therefore,32*8=256)
					start = 0;
//					enterCommand(0x80,20400);		//Change DDRAM
//					writeWord(text[1], times[1]);
				}
			status = 0x00;
		}
	}
}


//-------------------------------------------------------------------------------------------------

/* Interrupt handler */
void PB1_IntHandler (void)
{
	SysCtlDelay(4000000);//Debouncing

	status = GPIOIntStatus(GPIO_PORTA_BASE,true);
	if(status == 16){
		if(start == 1){
			start =0;
		}
	}else if(status == 32){
		//enterCommand(0x46,20400);		//Change DDRAM
		//writeWord(text[2], times[2]);
		indexMs++;
	}else if(status == 128){
		//enterCommand(0x46,20400);		//Change DDRAM
		//writeWord(text[3], times[3]);
		indexMs--;
	}
	GPIOIntClear(GPIO_PORTA_BASE, GPIO_PIN_4|GPIO_PIN_5|GPIO_PIN_7);	// Clear
}

/* Funtion for moving the stepper motor to the left. */
void moveStep(int a[],int steps){

	while(counter!=steps){
		if(start){
			GPIOPinWrite(GPIO_PORTD_BASE,GPIO_PIN_0|GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3, a[index]);		//Send signal to stepper motor control pins
			float delay = calculate(msArray[current]);
			SysCtlDelay(delay);		//delay
			if(index == 3){				//Check if index equals 3
				index = 0;				//if its true change index to 0 since its going left
				counter++;				//Increase counter since the stepper motor took a step
			}
			else{
				index++;				//increase counter since its going left
			}
		}else{
			counter = steps;
		}
	}
	start = 0;
	counter = 0;					//reset counter value to 0
}

/* Funtion for moving the stepper motor to the right.
 * Note: When the stepper is going right the combination of signals for controlling the Stepper motor is inverse.
 * Therefore the index starts at 3 and decreases and so on.
 */
void moveStepRight(int a[],int steps){

	while(counter!=steps){
		if(start){
			GPIOPinWrite(GPIO_PORTD_BASE, GPIO_PIN_0|GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3, a[index2]);		//Send signal to stepper motor control pins
			SysCtlDelay(133333.33);		//10ms delay
			if(index2 == 0){			//Check if index equals 0
				index2 = 3;				//If its true change index to 3 since its going right
				counter++;				//Increase counter since the stepper motor took a step
			}
			else{
				index2--;				//Decrease counter since its going right
			}
		}else{
			counter = steps;
		}
	}

	counter = 0;					//reset counter value to 0
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
		GPIOPinWrite(GPIO_PORTB_BASE, ALLPINS, string[i]);		//write character
		GPIOPinWrite(GPIO_PORTE_BASE, GPIO_PIN_3, 0x08);		//Enable On
		SysCtlDelay(50000);										//Delay 7.1ms(Calculated)
		GPIOPinWrite(GPIO_PORTE_BASE, GPIO_PIN_3, 0x00);		//Enable Off
	}
	GPIOPinWrite(GPIO_PORTE_BASE, GPIO_PIN_1, 0x00);			//RS disable
}

float calculate(float number){
	float ms = (number/4) *0.001;

	return (SysCtlClockGet()*ms)/3;
}

void checkIndex(void){
	if(indexMs > 9){
		indexMs = 0;
	}else if(indexMs < 0){
		indexMs = 9;
	}
}


