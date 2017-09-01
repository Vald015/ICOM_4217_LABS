#include <stdint.h>
#include <stdbool.h>
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "driverlib/gpio.h"
#include "driverlib/pin_map.h"
#include "driverlib/sysctl.h"
#include "driverlib/uart.h"

void writeChar(uint8_t string[]);

#define BAUDRATE 9600
uint8_t text[16][16] = {{"Hello World!-"}};

int main (void){
	/* MCU configuration */
	SysCtlClockSet(SYSCTL_SYSDIV_4 | SYSCTL_USE_PLL | SYSCTL_OSC_MAIN |SYSCTL_XTAL_16MHZ); // Main clock configuration

	SysCtlPeripheralEnable(SYSCTL_PERIPH_UART3);		//Enable UART 5 module
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOC);		//Enable Port E

	GPIOPinConfigure(GPIO_PC6_U3RX);					//Receiver
	GPIOPinConfigure(GPIO_PC7_U3TX);					//Transmitter
	GPIOPinTypeUART(GPIO_PORTC_BASE , GPIO_PIN_6|GPIO_PIN_7);	//Set pin E4 and E5 as UART

	UARTConfigSetExpClk(UART3_BASE, SysCtlClockGet() , BAUDRATE , (UART_CONFIG_WLEN_8 | UART_CONFIG_STOP_ONE | UART_CONFIG_PAR_NONE));

	writeChar(text[0]);

	while(1){
		if(UARTCharsAvail(UART3_BASE))
			UARTCharPut(UART3_BASE, UARTCharGet(UART3_BASE));
	}
}

void writeChar(uint8_t string[]){
	int desicion = 1;
	int i = 0;
	while(desicion){
		if(string[i] == '-'){
			desicion = 0;
		}else{
			UARTCharPut(UART3_BASE,string[i]);
			i++;
		}
	}
}


