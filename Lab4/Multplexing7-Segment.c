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
#define A     0x88
#define B     0x80
#define C   0xC6
#define D   0xC0
#define E   0x86
#define F   0x8E

/**Variable declaration*/
uint8_t numbers[16] = {Zero, One, Two, Three, Four, Five, Six, Seven, Eight, Nine, A, B, C, D, E, F};
int index = 0;
int index1 = 0;
int status = 0;
uint32_t period;
uint32_t period2;
int current = 0;
int checkTimerA;
int checkTimerB;

int main(void){

	/* Clock frequency configuration */
	SysCtlClockSet(SYSCTL_SYSDIV_5|SYSCTL_USE_PLL|SYSCTL_XTAL_16MHZ|SYSCTL_OSC_MAIN); //Set-up the clocking of the MCU

	/* Peripheral Configuration */
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA); //Enable the peripheral port to use. (A)
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOB); //Enable the peripheral port to use. (B)


	/* Input/output configuration */
	GPIOPinTypeGPIOOutput(GPIO_PORTA_BASE,  GPIO_PIN_2|GPIO_PIN_3);//Set pins of Port A for Control Lines
	GPIOPinTypeGPIOOutput(GPIO_PORTB_BASE, ALLPINS);// Set all port B pins as output pins for the 7-Segment


	/*Timer Configuration*/
	SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER0);//Enable the timer0 peripheral
	SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER1);//Enable the timer1 peripheral
	TimerConfigure(TIMER0_BASE,TIMER_CFG_PERIODIC);//Configres the operating mode of the timer(Full-width periodic timer)
	TimerConfigure(TIMER1_BASE,TIMER_CFG_PERIODIC);////Configres the operating mode of the timer(Full-width periodic timer)

	GPIOPadConfigSet(GPIO_PORTA_BASE, GPIO_PIN_2|GPIO_PIN_3, GPIO_STRENGTH_4MA , GPIO_PIN_TYPE_STD_WPD);//Set the current for pins in port A(Control Lines

	period = (SysCtlClockGet()/60/2);//Load value for Timer0
	period2 = (SysCtlClockGet()/1/2);//Load value for Timer1
	TimerLoadSet(TIMER0_BASE,TIMER_A,period-1);//Configure the timer load vlaue(period)
	TimerLoadSet(TIMER1_BASE,TIMER_A,period2-1);////Configure the timer load vlaue(period)

	IntEnable(INT_TIMER1A);//Enable Timer1A interruption
	IntEnable(INT_TIMER0A);//Enable Timer0A interruption
	TimerIntEnable(TIMER1_BASE,TIMER_TIMA_TIMEOUT);//Enable individual timer interrupt for TimerA timeout interrupt
	TimerIntEnable(TIMER0_BASE,TIMER_TIMA_TIMEOUT);//Enable individual timer interrupt for TimerA timeout interrupt
	IntMasterEnable();//Allows the processor to respont to interrupts

	TimerEnable(TIMER0_BASE,TIMER_A);//Enables operation of the timer module.(TIMER0)
	TimerEnable(TIMER1_BASE,TIMER_A);//Enables operation of the timer module.(TIMER1)

	while(1){
		checkTimerA = TimerValueGet(TIMER0_BASE,TIMER_A);//Current timer value for Timer0
		checkTimerB = TimerValueGet(TIMER1_BASE,TIMER_A);//Current timer value for Timer1
		if(status){
			if(current == 0){
				GPIOPinWrite(GPIO_PORTA_BASE, GPIO_PIN_2|GPIO_PIN_3, 0x0C);
				GPIOPinWrite(GPIO_PORTB_BASE, ALLPINS, numbers[index1]);
				GPIOPinWrite(GPIO_PORTA_BASE, GPIO_PIN_2|GPIO_PIN_3, 0x08);
				current = 1;
			}else if(current == 1){
				GPIOPinWrite(GPIO_PORTA_BASE, GPIO_PIN_2|GPIO_PIN_3, 0x0C);
				GPIOPinWrite(GPIO_PORTB_BASE, ALLPINS, numbers[index]);
				GPIOPinWrite(GPIO_PORTA_BASE, GPIO_PIN_2|GPIO_PIN_3, 0x04);
				current = 0;
			}
		}
	}
}

void Timer0IntHandler(void)
{
	TimerIntClear(TIMER0_BASE, TIMER_TIMA_TIMEOUT);
	status = 1;
}

void Timer1IntHandler(void)
{
	TimerIntClear(TIMER1_BASE, TIMER_TIMA_TIMEOUT);
	index++;
	if(index > 15 ){
		if(index1 == 15){
			index1 = 0;
		}else{
			index1++;
		}
		index = 0;
	}
}

