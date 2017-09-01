/* Tiva Workshop Lab*/
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

#define PWM_FREQUENCY 1000

int main (void)
{
	volatile uint32_t load;
	volatile uint32_t PWMClock;
	float dutyCycle[11] = {0,0.1,0.2,0.3,0.4,0.5,0.6,0.7,0.8,0.9,1};
	int index = 5;
	//volatile uint16_t adjust;
	//adjust = 500;

	SysCtlClockSet(SYSCTL_SYSDIV_5|SYSCTL_USE_PLL|SYSCTL_OSC_MAIN|SYSCTL_XTAL_16MHZ);
	SysCtlPWMClockSet(SYSCTL_PWMDIV_1);

	SysCtlPeripheralEnable(SYSCTL_PERIPH_PWM1);
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOD);
	//SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);

	GPIOPinTypePWM(GPIO_PORTD_BASE,GPIO_PIN_0);
	GPIOPinConfigure(GPIO_PD0_M1PWM0);

	PWMClock = SysCtlClockGet()/1;
	load = (PWMClock/PWM_FREQUENCY) -1;

	PWMGenConfigure(PWM1_BASE,PWM_GEN_0,PWM_GEN_MODE_DOWN);
	PWMGenPeriodSet(PWM1_BASE,PWM_GEN_0,load);
	int width = load*dutyCycle[index];
	//PWMPulseWidthSet(PWM1_BASE,PWM_OUT_0, load*adjust/1000);
	PWMPulseWidthSet(PWM1_BASE,PWM_OUT_0, width);
	PWMOutputState(PWM1_BASE,PWM_OUT_0_BIT,true);
	PWMGenEnable(PWM1_BASE,PWM_GEN_0);
	while(1)
	{

	}

}
