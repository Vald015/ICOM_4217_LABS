#include <stdint.h>
#include <stdbool.h>
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "driverlib/gpio.h"
#include "driverlib/pin_map.h"
#include "driverlib/sysctl.h"
#include "driverlib/uart.h"
#include "inc/hw_ints.h"
#include "driverlib/interrupt.h"

#define ALLPINS GPIO_PIN_0|GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3|GPIO_PIN_4|GPIO_PIN_5|GPIO_PIN_6|GPIO_PIN_7


void writeChar(uint8_t string[]);
void writeWord(uint8_t string[], uint8_t times);    //write a word on the LCD
void initializeLCD();                               //initialize LCD function declaration
void enterCommand(int command,float delayTime);
void writeCharDisp();                               //write a char on the LCD
void changeMessage();                               //change the message from Uppercase to Lowercase

char current;                                      //current character
uint8_t message[16];
int counter = 0;

uint8_t text[16][16] = {{"Hello World "}};
int ready = 0;


#define BAUDRATE 9600

int main (void){
	/* MCU configuration */
	SysCtlClockSet(SYSCTL_SYSDIV_4 | SYSCTL_USE_PLL | SYSCTL_OSC_MAIN |SYSCTL_XTAL_16MHZ); // Main clock configuration

	SysCtlPeripheralEnable(SYSCTL_PERIPH_UART3);		//Enable UART 5 module
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOC);		//Enable Port E
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOB);        //Enable the peripheral port to use. (B) // LCD
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);       	//Enable the peripheral port to use. (F) //LCD R/W R/S E

	/* LCD pin configuration */
	GPIOPinTypeGPIOOutput(GPIO_PORTB_BASE, ALLPINS);                                        // Set all port B pins as output pins
	GPIOPinTypeGPIOOutput(GPIO_PORTF_BASE, GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3);               //PIN1 : R/S (R/S: 1 for data ** 0 for instruction) *** PIN2 : R/W (R/S: 1 for Read ** 0 for write) *** PIN3 : E(Chip Enable signal).

	/* UART Pin configuration */
	GPIOPinConfigure(GPIO_PC6_U3RX);        //Receiver
	GPIOPinConfigure(GPIO_PC7_U3TX);        //Transmitter
	GPIOPinTypeUART(GPIO_PORTC_BASE, GPIO_PIN_6 | GPIO_PIN_7);

	UARTConfigSetExpClk(UART3_BASE, SysCtlClockGet() , BAUDRATE , (UART_CONFIG_WLEN_8 | UART_CONFIG_STOP_ONE | UART_CONFIG_PAR_NONE));

	initializeLCD();

	IntMasterEnable();
	IntEnable(INT_UART3);
	UARTIntEnable(UART3_BASE, UART_INT_RX | UART_INT_RT);
	//UARTIntEnable(UART3_BASE, UART_INT_RX);

	//writeWord(text[0],12);
	while(1){

	}
}

void UARTIntHandler(void){
	uint32_t status;
	status = UARTIntStatus(UART3_BASE,true); 		//get insterrupt status
	UARTIntClear(UART3_BASE,status);  				//clear the asserted interrupts
	while(UARTCharsAvail(UART3_BASE)){
		current = UARTCharGetNonBlocking(UART3_BASE);
		if(ready){
			UARTCharPutNonBlocking(UART3_BASE,current);   //echo character
		}
		changeMessage();
		counter++;
		writeCharDisp();
	}
}

void writeCharDisp()
{
	if(counter == 16){
		counter = 0;
		GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_1, 0x02);//RS enable
		GPIOPinWrite(GPIO_PORTB_BASE, ALLPINS, current );
		GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_3, 0x08);//Enable On
		SysCtlDelay(50000);//Delay 7.1ms(Calculated)
		GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_3, 0x00);//Enable Off
		GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_1, 0x00);//RS disable
		uint8_t i =0;
		for(; i <16; i++){
			UARTCharPut(UART3_BASE,message[i]);
		}

		//enterCommand(0x01,20400);       //Clear display command
		ready = 1;

	}
	else{

		GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_1, 0x02);//RS enable
		GPIOPinWrite(GPIO_PORTB_BASE, ALLPINS, current );
		GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_3, 0x08);//Enable On
		SysCtlDelay(50000);//Delay 7.1ms(Calculated)
		GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_3, 0x00);//Enable Off
		GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_1, 0x00);//RS disable
	}

}

void changeMessage(){
	if(current >= 0x61 && current <=0x7A){
		message[counter] = current - 0x20;
	}
	else
		message[counter] = current;
}

void writeChar(uint8_t string[]){
	int desicion = 1;
	int i = 0;
	while(desicion){
		if(string[i] == '-'){
			desicion = 0;
		}else{
			UARTCharPut(UART5_BASE,string[i]);
			i++;
		}
	}
}

/* LCD initialization function */
void initializeLCD()
{
	SysCtlDelay(533333.3333);       // Starting delay (40ms).

	enterCommand(0x3C,520);         //Function Set command

	enterCommand(0x3C,493.3);       //Function Set

	enterCommand(0x0F,493.3);       //Display ON

	enterCommand(0x01,20400);       //Clear display command

	enterCommand(0x06,20400);       //Entry Mode Set

	enterCommand(0x01,20400);       //Clear display command
}

/* Enter command to LCD function; command is the desired LCD instruction, delayTime is the required time that the chip enable needs to be on in order to
 * successfully send the instruction */
void enterCommand(int command,float delayTime)
{
	GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_1|GPIO_PIN_2, 0x00); //R/S & R/W = 0
	GPIOPinWrite(GPIO_PORTB_BASE, ALLPINS, command);            //enter command on port
	GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_3, 0x08);            //Enable On
	SysCtlDelay(delayTime);                                     //value for the desired delay
	GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_3, 0x00);            //Enable Off
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
