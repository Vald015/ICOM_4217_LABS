/*
 * ICOM 4217 (EMBEDDED SYSTEM DESIGN)
 * Authors: Carlos A. Rodriguez & Osvaldo A. Ramirez
 * LAB4 PART5: COMPLEMENTARY TASK
 * Digital Tachometer
 */

/* Header files */
#include <stdint.h>
#include <stdbool.h>
#include "inc/tm4c123gh6pm.h"
#include "inc/hw_types.h"
#include "inc/hw_memmap.h"
#include "driverlib/interrupt.h"
#include "driverlib/sysctl.h"
#include "driverlib/gpio.h"
#include "driverlib/timer.h"

/* function declaration */
void PA1_IntHandler();															//Interrupt keypad handler
void initializeLCD();															//initialize LCD function declaration
void writeLCD(uint8_t string[],uint8_t string2[], int times, int times2);		//Write the two lines of the LCD
void enterCommand(int command,float delayTime);									//enter command function declaration
void writeWord(uint8_t string[], uint8_t times);								//writeWord function declaration
void checkDirection();															//Fix when index is higher than 15 or lower than 0
void writeRpm(void);

/* Function Declaration */
uint8_t text[16][16] = {{"CW "},{"CCW"},{"Speed:"},{"RPM"}};			//Words to be written in LCD
int timesArray[5] = {3,3,6,3,4}; 										//text[] words lenght
int value[16] = {0,1,-1,0,-1,0,0,1,1,0,0,-1,0,-1,1,0};					//Values for the different types of interrupts
uint8_t status = 0x00;													//Set Interrupt status to zero
int oldA = 0;					//Set old A to 0
int newA = 0;					//Set new A to 0
int oldB = 0;					//Set old B to 0
int newB = 0;					//Set new B to 0
uint8_t index = 0;				//Value array index
int direction = 0;				//Direction of the rotation
int rpm = 0000;					//Rpm of the wheeel
int times = 0;					//How many times the system have interrupted
int msCounter = 0;				//millisecond conunter

int secPerRev= 24;				//number of sectors per Revolution


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
	GPIOPinTypeGPIOInput(GPIO_PORTA_BASE,GPIO_PIN_2|GPIO_PIN_3); 											//Input configuration PA2,PA3
	GPIOPadConfigSet(GPIO_PORTA_BASE, GPIO_PIN_2|GPIO_PIN_3, GPIO_STRENGTH_4MA , GPIO_PIN_TYPE_STD_WPD ) ;	//Set the pin current strength (default is 4mA) and Pin Type to Pull DOWN
	GPIOIntRegister(GPIO_PORTA_BASE, PA1_IntHandler); 														//Registers an interrupt handler for a GPIO port. Ensures that the interrupt handler specified is called.
	GPIOIntTypeSet(GPIO_PORTA_BASE,GPIO_PIN_2|GPIO_PIN_3,GPIO_BOTH_EDGES); 									//Sets the interrupt type for the specified pin(s). GPIO_FALLING_EDGEsets detection to edge and trigger to falling(high to low transition)

	/*Timer Configuration*/
	SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER0);							//Enable Timer 0 peripheral
	TimerConfigure(TIMER0_BASE,TIMER_CFG_PERIODIC);							//Set the timer to count down

	uint32_t period = (SysCtlClockGet()/1000);								//Calcultate the period of the timer
	TimerLoadSet(TIMER0_BASE,TIMER_A,period-1);								//Load the timer with the calculated period

	/* Enable all interrupts */
	IntEnable(INT_TIMER0A);																	//Enable timer A interrupts
	TimerIntEnable(TIMER0_BASE,TIMER_TIMA_TIMEOUT);											//Enable timeout interrupts
	IntMasterEnable();																		//Enable master interrupts

	GPIOIntEnable(GPIO_PORTA_BASE, GPIO_PIN_2|GPIO_PIN_3);													//Enable interrupts in GPIO ports PC4,PC5,PC6

	initializeLCD();	// initialize LCD
	writeWord(text[2],timesArray[2]);							//write Speed on the LCD
	//writeWord(text[4],4);										//write the current RPM
	enterCommand(0x8A,20400);									//Move cursor
	writeWord(text[3],timesArray[3]);							//write RPM
	writeRpm();													//Write current RPM

	while(1){

		if(status != 0){
			times++;								//Times interrupted counter increase
			if(times == 1){
				TimerEnable(TIMER0_BASE,TIMER_A);	//If times equal 1, Enable timer
			}else if(times == 2){ 					//If times equal 2
				TimerDisable(TIMER0_BASE,TIMER_A); 	//Disable timer
				rpm = 60000 / (msCounter*secPerRev);//Calculate RPM
				msCounter = 0;						//millisecond per sector counter equal to 0
				times = 0;							//Times counter equal to zero
				writeRpm();							//Write new RPM
			}

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

			direction = value[index];				//Set direction equal to the value of index in the values array
			checkDirection();						//Fix index
		}
	}
}

/* Interrupt handler */
void PA1_IntHandler (void)
{
	status = GPIOIntStatus(GPIO_PORTA_BASE,true);	//Store the value of the Port A
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

/* Function for writing words in the LCD, string is the word that want to be written, times is the number of characters that the string has */
void writeWord(uint8_t string[], uint8_t times)
{
	GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_1, 0x02);//RS enable

	uint8_t i =0;//initialize for loop
	for(; i < times; i++){
		GPIOPinWrite(GPIO_PORTB_BASE, ALLPINS, string[i]);//write character
		GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_3, 0x08);//Enable On
		SysCtlDelay(50000);//Delay 7.1ms(Calculated)
		GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_3, 0x00);//Enable Off
	}

	GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_1, 0x00);//RS disable
}

/* Check if the wheel goes counterclockwise or clockwise*/
void checkDirection(){
	enterCommand(0xC0,5000);				//Move cursor to second line

	if(direction < 0){
		writeWord(text[1],timesArray[1]);	//If direction equals zero write CCW
	}else if(direction > 0){
		writeWord(text[0],timesArray[0]);	//If direction equals one write CW
	}
}

/* Function for writing the new rpm value in the LCD */
void writeRpm(void){
	enterCommand(0x86,5000);		//move cursor

	/* Change from int to ASCII */
	text[4][0] ='0' + ((rpm/1000)%10);		//Get the first digit
	text[4][1] ='0' + ((rpm/100)%10);		//get second digit
	text[4][2] ='0' + ((rpm/10)%10);		//get thrid digit
	text[4][3] ='0' + (rpm%10);				//get fourth digit

	writeWord(text[4],timesArray[4]);		//write the value to the LCD
	enterCommand(0x8D,5000);
}

/* Timer interrupt handler */
void Timer0IntHandler(void)
{
	TimerIntClear(TIMER0_BASE, TIMER_TIMA_TIMEOUT);				//Clear timer interrupt flag
	msCounter++;												//Increment timer counter
}



