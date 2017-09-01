

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
#define motorRight 0x20
#define motorLeft 0x40
#define motorOff 0x00
#define DESIRED_PWM_FRECUENCY 1000 //In Hz
#define DUTY_CYCLE 0.50  //% Duty Cycle (decimal value)

void PB1_IntHandler();
void initializeLCD();
void writeSpeed();
void setMode();
void enterCommand(int command,float delayTime);		//enter command function declaration
void writeWord(uint8_t string[], uint8_t times);				//writeWord function declaration

uint8_t status =0x00;		//Status is the column thats the interrupt occurred.
int button = 0;
uint8_t text[16][16] = {{"Ready "}, {"Moving Right "}, {"Moving Left "}, {"Motor Off "}};
int timesArray[5] = {6,13,12,10};
int speedIndex = 0; // Speed index
volatile uint32_t pwmClockFreq = 0;
volatile uint32_t pwmLoadValue = 0;
float levels[6] = {0.01, 0.20, 0.40, 0.60,0.80,1};  // Increment for levels
int currentStatus = 0;
uint8_t text2[16][16] = {{"Motor Off "}, {"Move Left "}, {"Move Right "}, {"Speed = 0%"}, {"Speed = 20%"},{"Speed = 40%"},{"Speed = 60%"},{"Speed = 80%"},{"Speed = 100%"}};
int timesArray2[9] = {10,10,11,10,11,11,11,11,12};

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
	GPIOPinTypeGPIOOutput(GPIO_PORTC_BASE, GPIO_PIN_5|GPIO_PIN_6);							//DC MOTOR

	/*Interrupt Buttons GPIO PORTA PIN 5 & 7*/
	GPIOPinTypeGPIOInput(GPIO_PORTA_BASE, GPIO_PIN_4|GPIO_PIN_5|GPIO_PIN_7);

	//------PWM Initialization---

	//Clock the PWM module by the system clock
	SysCtlPWMClockSet(SYSCTL_PWMDIV_16); //Divide system clock by 32 to run the PWM at 1.25MHz

	//Enabling PWM1 module and Port D
	SysCtlPeripheralEnable(SYSCTL_PERIPH_PWM0);
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOC); //Port where the PWM pin will be selected


	//Selecting PWM generator 0 and port C pin 0 (PC5/4) as a PWM output pin for module 0
	GPIOPinTypePWM(GPIO_PORTC_BASE, GPIO_PIN_5|GPIO_PIN_4);
	GPIOPinConfigure(GPIO_PC4_M0PWM6); //Select PWM for pin 4
	GPIOPinConfigure(GPIO_PC5_M0PWM7); //Select PWM for pin 5

	pwmClockFreq = SysCtlClockGet() / 16;
	pwmLoadValue = (pwmClockFreq / DESIRED_PWM_FRECUENCY) - 1; //Load Value = (PWMGeneratorClock * DesiredPWMPeriod) - 1
	PWMGenConfigure(PWM0_BASE, PWM_GEN_3, PWM_GEN_MODE_DOWN); //Set a count-down generator type
	PWMGenPeriodSet(PWM0_BASE, PWM_GEN_3, pwmLoadValue); //Set PWM period determined by the load value

	PWMPulseWidthSet(PWM0_BASE, PWM_OUT_6, pwmLoadValue * 0.01);
	PWMPulseWidthSet(PWM0_BASE, PWM_OUT_7, pwmLoadValue * 0.01);
	PWMOutputState(PWM0_BASE, PWM_OUT_6_BIT, false);
	PWMOutputState(PWM0_BASE, PWM_OUT_7_BIT, false);

	PWMGenEnable(PWM0_BASE, PWM_GEN_3); //Enable PWM Generator

	//---End PWM Initialization



	//Input configuration PA5, PA7

	GPIOIntRegister(GPIO_PORTA_BASE, PB1_IntHandler); 										//Registers an interrupt handler for a GPIO port. Ensures that the interrupt handler specified is called.
	//GPIOIntTypeSet(GPIO_PORTA_BASE,GPIO_PIN_4|GPIO_PIN_5|GPIO_PIN_7,GPIO_RISING_EDGE);
	//Sets the interrupt type for the specified pin(s). GPIO_FALLING_EDGEsets detection to edge and trigger to falling(high to low transition)
	GPIOIntTypeSet(GPIO_PORTA_BASE, GPIO_PIN_5, GPIO_RISING_EDGE);
	GPIOIntTypeSet(GPIO_PORTA_BASE, GPIO_PIN_7, GPIO_FALLING_EDGE);
	GPIOIntTypeSet(GPIO_PORTA_BASE, GPIO_PIN_4, GPIO_FALLING_EDGE);

	GPIOIntEnable(GPIO_PORTA_BASE, GPIO_PIN_4|GPIO_PIN_5|GPIO_PIN_7);




	initializeLCD();
	writeWord(text[0],timesArray[0]);
	while(1){
		SysCtlDelay(4000);   // Delay for Debouncing
		if(status!=0){
			status = 0;
			if(button == 0x20 && speedIndex>0){  				   				// Button A5 Pressed decrease velocity if velocity != 0
				status = 0x00;
				speedIndex--;
				if(speedIndex==0){//Motor stopped
					currentStatus = 1;

					PWMOutputState(PWM0_BASE, PWM_OUT_6_BIT, false);
					PWMOutputState(PWM0_BASE, PWM_OUT_7_BIT, false);
				}else{//Set PWMs to new speed
					PWMPulseWidthSet(PWM0_BASE, PWM_OUT_6, pwmLoadValue * levels[speedIndex]);
					PWMPulseWidthSet(PWM0_BASE, PWM_OUT_7, pwmLoadValue * levels[speedIndex]);
				}

				writeSpeed();


			}
			else if(button == 0x80){			    			//Button A7 Pressed Increase Velocity
				status = 0x00;
				speedIndex++; // increase speed index
				writeSpeed();// write current action to LCD
				PWMPulseWidthSet(PWM0_BASE, PWM_OUT_6, pwmLoadValue * levels[speedIndex]);
				PWMPulseWidthSet(PWM0_BASE, PWM_OUT_7, pwmLoadValue * levels[speedIndex]);
			}
			else if(button == 0x10){							//Button A4 Pressed Change Off-Left-Right
																										SysCtlDelay(4000000);//Debouncing
				status = 0x00;
				button = 0x00;

				setMode();									//Change operation mode of the Motor(OFF-Left-Right)
				writeSpeed();
			}
			button = 0;
			status = 0;

		}


	}


}
/* Interrupt handler */
void PB1_IntHandler (void)
{
	SysCtlDelay(4000000);//Debouncing
	button = GPIOIntStatus(GPIO_PORTA_BASE,true);
	GPIOIntClear(GPIO_PORTA_BASE, GPIO_PIN_4|GPIO_PIN_5|GPIO_PIN_7);	// Clear
	status = 1;


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

// Writes the current speed and action to LCD
void writeSpeed(void){
	enterCommand(0x01,20400);		//Clear display command   //clear the LCD
	switch(currentStatus){
	case 1:
		writeWord(text2[0],timesArray2[0]);	// write current action to LCD
		break;
	case 2:
		writeWord(text2[1],timesArray2[1]);	// write current action to LCD
		break;
	case 3:
		writeWord(text2[2],timesArray2[2]);	// write current action to LCD
		break;
	default :
		break;
	}
	enterCommand(0xC0,20400);		//Clear display command);    // move to bottom line of LCD
	switch(speedIndex){
	case 0:
		writeWord(text2[3],timesArray2[3]);	// write current action to LCD
		break;
	case 1:
		writeWord(text2[4],timesArray2[4]);	// write current action to LCD
		break;
	case 2:
		writeWord(text2[5],timesArray2[5]);// write current action to LCD
		break;
	case 3:
		writeWord(text2[6],timesArray2[6]);// write current action to LCD
		break;
	case 4:
		writeWord(text2[7],timesArray2[7]);// write current action to LCD
		break;
	case 5:
		writeWord(text2[8],timesArray2[8]);// write current action to LCD
		break;
	default :
		break;
	}
}

// Sets Current Action
void setMode(void){
	currentStatus++;  //go to next action
	if(currentStatus >3){  // if last action was done, go to first one
		currentStatus = 1;
	}
	switch(currentStatus){
	case 1:
		PWMOutputState(PWM0_BASE, PWM_OUT_6_BIT, false);
		PWMOutputState(PWM0_BASE, PWM_OUT_7_BIT, false);
		break;
	case 2:
		PWMOutputState(PWM0_BASE, PWM_OUT_6_BIT, true);
		PWMOutputState(PWM0_BASE, PWM_OUT_7_BIT, false);
		break;
	case 3:
		PWMOutputState(PWM0_BASE, PWM_OUT_6_BIT, false);
		PWMOutputState(PWM0_BASE, PWM_OUT_7_BIT, true);
		break;
	default :
		break;
	}
}


