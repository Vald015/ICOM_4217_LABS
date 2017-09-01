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

//#define PWM_FREQUENCY 8000

int main (void)
{
	volatile uint32_t load;
	volatile uint32_t PWMClock;
	volatile int PWM_FREQUENCY[5] = {500,1000,2000,4000,8000};
	int width;

	SysCtlClockSet(SYSCTL_SYSDIV_5|SYSCTL_USE_PLL|SYSCTL_OSC_MAIN|SYSCTL_XTAL_16MHZ);
	SysCtlPWMClockSet(SYSCTL_PWMDIV_2);

	SysCtlPeripheralEnable(SYSCTL_PERIPH_PWM0);

	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOB);

	GPIOPinTypePWM(GPIO_PORTB_BASE,GPIO_PIN_5);

	GPIOPinConfigure(GPIO_PB5_M0PWM3);


	PWMClock = SysCtlClockGet()/2;
	load = (PWMClock/PWM_FREQUENCY[1]) -1;

	PWMGenConfigure(PWM0_BASE,PWM_GEN_1,PWM_GEN_MODE_DOWN|PWM_GEN_MODE_NO_SYNC);

	PWMGenPeriodSet(PWM0_BASE,PWM_GEN_1,load);

	width = (load/2);

	PWMPulseWidthSet(PWM0_BASE,PWM_OUT_3, width);

	PWMOutputState(PWM0_BASE,PWM_OUT_3_BIT,true);

	PWMGenEnable(PWM0_BASE,PWM_GEN_1);

	while(1)
	{
	}
}
