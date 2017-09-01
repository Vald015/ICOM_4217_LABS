#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include "inc/hw_i2c.h"
#include "inc/hw_memmap.h"
#include "inc/tm4c123gh6pm.h"
#include "inc/hw_types.h"
#include "inc/hw_gpio.h"
#include "driverlib/i2c.h"
#include "driverlib/sysctl.h"
#include "driverlib/gpio.h"
#include "driverlib/pin_map.h"
#include "driverlib/uart.h"
#include "utils/uartstdio.h"
#include "driverlib/timer.h"
#include "driverlib/interrupt.h"

uint8_t DACValues[24] = {0x00,0x00,0x07,0x10,0x0E,0x20,0x05,0x40,0x0C,0x50,0x03,0x70,0x0A,0x80,0x01,0xA0,0x08,0xB0,0x0F,0xC0,0x06,0xE0,0x0F,0xF0};
int DACindex = 0;
uint32_t period;

#define ALLPINS GPIO_PIN_0|GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3|GPIO_PIN_4|GPIO_PIN_5|GPIO_PIN_6|GPIO_PIN_7

void main(void)
{
	SysCtlClockSet(SYSCTL_SYSDIV_5 | SYSCTL_USE_PLL | SYSCTL_OSC_MAIN | SYSCTL_XTAL_16MHZ);
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOC);
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOD);
	SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER0);

	GPIOPinTypeGPIOOutput(GPIO_PORTD_BASE,GPIO_PIN_0|GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3);
	GPIOPinTypeGPIOOutput(GPIO_PORTC_BASE,GPIO_PIN_4|GPIO_PIN_5|GPIO_PIN_6|GPIO_PIN_7);


	/* Peripheral Configuration */
	TimerConfigure(TIMER0_BASE,TIMER_CFG_PERIODIC);												//Configure timer to count down

	period = (SysCtlClockGet()/2)-1;										//Calculate the period of the timer at 50% duty cycle
	TimerLoadSet(TIMER0_BASE,TIMER_A,period);

	IntEnable(INT_TIMER0A);											//Enable Timer 0 subtimer A
	TimerIntEnable(TIMER0_BASE,TIMER_TIMA_TIMEOUT);					//Enable Timer Interrupt when the timer runs out
	IntMasterEnable();												//Enable master interrupt timer
	TimerEnable(TIMER0_BASE,TIMER_A);								//Enable Timer

	while(1){
	}
}


/* Timer interrupt handler */
void Timer0IntHandler(void)
{
	TimerIntClear(TIMER0_BASE, TIMER_TIMA_TIMEOUT);							//Clear timer interrupt flag
	GPIOPinWrite(GPIO_PORTD_BASE,GPIO_PIN_0|GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3,DACValues[DACindex]);				//Set the output of PORTC the DAC value that the index is pointing in the array.
	GPIOPinWrite(GPIO_PORTC_BASE,GPIO_PIN_4|GPIO_PIN_5|GPIO_PIN_6|GPIO_PIN_7,DACValues[DACindex+1]);				//Set the output of PORTC the DAC value that the index is pointing in the array.
	DACindex = DACindex + 2;
	if(DACindex >22){
		DACindex = 0;
	}
}
