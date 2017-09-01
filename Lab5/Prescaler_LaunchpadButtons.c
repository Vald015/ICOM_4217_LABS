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

#define PWM_FREQUENCY 8000

int main (void)
{
	volatile uint32_t load;
	volatile uint32_t PWMClock;
	int width;

	SysCtlClockSet(SYSCTL_SYSDIV_5|SYSCTL_USE_PLL|SYSCTL_OSC_MAIN|SYSCTL_XTAL_16MHZ);
	SysCtlPWMClockSet(SYSCTL_PWMDIV_2);

	SysCtlPeripheralEnable(SYSCTL_PERIPH_PWM1);
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOD);
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);

	GPIOPinTypePWM(GPIO_PORTD_BASE,GPIO_PIN_0);
	GPIOPinConfigure(GPIO_PD0_M1PWM0);

	/* Tiva buttons configuration */
	HWREG(GPIO_PORTF_BASE + GPIO_O_LOCK) = GPIO_LOCK_KEY;
	HWREG(GPIO_PORTF_BASE + GPIO_O_CR) = 0X01;
	HWREG(GPIO_PORTF_BASE + GPIO_O_LOCK) = 0;
	GPIODirModeSet(GPIO_PORTF_BASE,GPIO_PIN_4,GPIO_DIR_MODE_IN);
	GPIOPadConfigSet(GPIO_PORTF_BASE,GPIO_PIN_4,GPIO_STRENGTH_2MA, GPIO_PIN_TYPE_STD_WPU); //0 & 4 son los botones

	GPIOPinTypeGPIOOutput(GPIO_PORTF_BASE, GPIO_PIN_1);

	PWMClock = SysCtlClockGet()/2;
	load = (PWMClock/PWM_FREQUENCY) -1;

	PWMGenConfigure(PWM1_BASE,PWM_GEN_0,PWM_GEN_MODE_DOWN);
	PWMGenPeriodSet(PWM1_BASE,PWM_GEN_0,load);
	width = (load/2);
	PWMPulseWidthSet(PWM1_BASE,PWM_OUT_0, width);
	PWMOutputState(PWM1_BASE,PWM_OUT_0_BIT,true);
	PWMGenEnable(PWM1_BASE,PWM_GEN_0);
	while(1)
	{
		if(GPIOPinRead(GPIO_PORTF_BASE,GPIO_PIN_4) == 0){
			SysCtlDelay(4000000);
			if(GPIOPinRead(GPIO_PORTF_BASE,GPIO_PIN_1) == 0){
				GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_1, 2);
			}else{
				GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_1, 0);
			}
	}
	}

}
