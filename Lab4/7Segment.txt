#include <stdint.h>
#include <stdbool.h>
#include "inc/tm4c123gh6pm.h"
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "driverlib/sysctl.h"
#include "driverlib/interrupt.h"
#include "driverlib/gpio.h"
#include "driverlib/timer.h"

/* define label ALLPINS as all the pins in a port */
#define ALLPINS GPIO_PIN_0|GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3|GPIO_PIN_4|GPIO_PIN_5|GPIO_PIN_6|GPIO_PIN_7
#define Zero  0xC0
#define One  0xF9
#define Two  0xA4
#define Three  0xB0
#define Four 0x99
#define Five 0x92
#define Six 0x82
#define Seven 0xF8
#define Eight 0x80
#define Nine  0x98
#define A   0x88
#define B   0x80
#define C   0xC6
#define D   0xC0
#define E   0x86
#define F   0x8E
/**Variable declaration*/
uint8_t numbers[16] = {Zero, One, Two, Three, Four, Five, Six, Seven, Eight, Nine, A, B, C, D, E, F};
int index = 0;
uint32_t ui32Period;


int main(void){




	/* Clock frequency configuration */
	SysCtlClockSet(SYSCTL_SYSDIV_5|SYSCTL_USE_PLL|SYSCTL_XTAL_16MHZ|SYSCTL_OSC_MAIN); //Set-up the clocking of the MCU

	/* Peripheral Configuration */
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOB); //Enable the peripheral port to use. (B)
	

	/* Input/output configuration */
	GPIOPinTypeGPIOOutput(GPIO_PORTB_BASE, ALLPINS);	// Set all port B pins as output pins

	GPIOPadConfigSet(GPIO_PORTB_BASE, ALLPINS, GPIO_STRENGTH_2MA, GPIO_PIN_TYPE_STD); //Set the pins current strength Pin Type to Pull DOWN for PA7

	/*Timer Configuration*/
	SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER0);
	TimerConfigure(TIMER0_BASE,TIMER_CFG_PERIODIC);

	ui32Period = (SysCtlClockGet()/2);
	TimerLoadSet(TIMER0_BASE,TIMER_A,ui32Period-1);

	IntEnable(INT_TIMER0A);
	TimerIntEnable(TIMER0_BASE,TIMER_TIMA_TIMEOUT);
	IntMasterEnable();

	TimerEnable(TIMER0_BASE,TIMER_A);

	while(1){



	}


}

void Timer0IntHandler(void)
{
	TimerIntClear(TIMER0_BASE, TIMER_TIMA_TIMEOUT);

	if(index ==16){
						index = 0;
					}

			GPIOPinWrite(GPIO_PORTB_BASE, ALLPINS, numbers[index]);

			index++;
}

