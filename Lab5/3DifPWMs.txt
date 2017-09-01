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
	int width1;
	int width2;

	SysCtlClockSet(SYSCTL_SYSDIV_5|SYSCTL_USE_PLL|SYSCTL_OSC_MAIN|SYSCTL_XTAL_16MHZ);
	SysCtlPWMClockSet(SYSCTL_PWMDIV_2);

	SysCtlPeripheralEnable(SYSCTL_PERIPH_PWM0);
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOB);

	GPIOPinTypePWM(GPIO_PORTB_BASE,GPIO_PIN_5|GPIO_PIN_6|GPIO_PIN_7);

	GPIOPinConfigure(GPIO_PB5_M0PWM3);
	GPIOPinConfigure(GPIO_PB6_M0PWM0);
	GPIOPinConfigure(GPIO_PB7_M0PWM1);

	PWMClock = SysCtlClockGet()/2;
	load = (PWMClock/PWM_FREQUENCY[1]) -1;

	PWMGenConfigure(PWM0_BASE,PWM_GEN_0,PWM_GEN_MODE_DOWN|PWM_GEN_MODE_NO_SYNC);
	PWMGenConfigure(PWM0_BASE,PWM_GEN_1,PWM_GEN_MODE_DOWN|PWM_GEN_MODE_NO_SYNC);

	PWMGenPeriodSet(PWM0_BASE,PWM_GEN_0,load);
	PWMGenPeriodSet(PWM0_BASE,PWM_GEN_1,load);

	width = (load/2);
	width1 = (load*0.1);
	width2 = (load*0.9);

	PWMPulseWidthSet(PWM0_BASE,PWM_OUT_0, width);
	PWMPulseWidthSet(PWM0_BASE,PWM_OUT_1, width1);
	PWMPulseWidthSet(PWM0_BASE,PWM_OUT_3, width2);

	PWMOutputState(PWM0_BASE,PWM_OUT_0_BIT|PWM_OUT_1_BIT|PWM_OUT_3_BIT,true);

	PWMGenEnable(PWM0_BASE,PWM_GEN_0);
	PWMGenEnable(PWM0_BASE,PWM_GEN_1);

	while(1)
	{
	}
}
