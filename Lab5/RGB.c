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
/* Function Declaration */
void getWidth(float width[], int Load);
void checkIndex();

int index = 0;

int main (void)
{
	volatile uint32_t load;
	volatile uint32_t PWMClock;
	//volatile int PWM_FREQUENCY[5] = {500,1000,2000,4000,8000};
	float dC[8][3] = {{0,0,1},{0,1,0},{1,0,0},{1,0.12,0.85},{0.117,0.87,0.988},{0.94,0.78,0.157},{1,0.48,0.129},{1,1,1}};

	SysCtlClockSet(SYSCTL_SYSDIV_5|SYSCTL_USE_PLL|SYSCTL_OSC_MAIN|SYSCTL_XTAL_16MHZ);
	SysCtlPWMClockSet(SYSCTL_PWMDIV_2);

	SysCtlPeripheralEnable(SYSCTL_PERIPH_PWM0);
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOB);
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);

	GPIOPinTypePWM(GPIO_PORTB_BASE,GPIO_PIN_5|GPIO_PIN_6|GPIO_PIN_7);

	GPIOPinConfigure(GPIO_PB5_M0PWM3);
	GPIOPinConfigure(GPIO_PB6_M0PWM0);
	GPIOPinConfigure(GPIO_PB7_M0PWM1);

	/* Tiva buttons configuration */
	HWREG(GPIO_PORTF_BASE + GPIO_O_LOCK) = GPIO_LOCK_KEY;
	HWREG(GPIO_PORTF_BASE + GPIO_O_CR) = 0X01;
	HWREG(GPIO_PORTF_BASE + GPIO_O_LOCK) = 0;
	GPIODirModeSet(GPIO_PORTF_BASE,GPIO_PIN_4,GPIO_DIR_MODE_IN);
	GPIOPadConfigSet(GPIO_PORTF_BASE,GPIO_PIN_4,GPIO_STRENGTH_2MA, GPIO_PIN_TYPE_STD_WPU); //0 & 4 son los botones

	PWMClock = SysCtlClockGet()/2;
	load = (PWMClock/PWM_FREQUENCY) -1;

	PWMGenConfigure(PWM0_BASE,PWM_GEN_0,PWM_GEN_MODE_DOWN|PWM_GEN_MODE_NO_SYNC);
	PWMGenConfigure(PWM0_BASE,PWM_GEN_1,PWM_GEN_MODE_DOWN|PWM_GEN_MODE_NO_SYNC);

	PWMGenPeriodSet(PWM0_BASE,PWM_GEN_0,load);
	PWMGenPeriodSet(PWM0_BASE,PWM_GEN_1,load);

	PWMPulseWidthSet(PWM0_BASE,PWM_OUT_0, load);			//Green
	PWMPulseWidthSet(PWM0_BASE,PWM_OUT_1, load*dC[0][2]);	//Blue
	PWMPulseWidthSet(PWM0_BASE,PWM_OUT_3, load); 			//Red

	PWMOutputState(PWM0_BASE,PWM_OUT_1_BIT,true);

	PWMGenEnable(PWM0_BASE,PWM_GEN_0);
	PWMGenEnable(PWM0_BASE,PWM_GEN_1);

	while(1)
	{
		/* 0 percentage duty cycle equals grounding the signal */
		//PWMOutputState(PWM0_BASE,PWM_OUT_0_BIT|PWM_OUT_1_BIT,false);
		//To Do
		//*******************Change to interrupts**********************
		if(GPIOPinRead(GPIO_PORTF_BASE,GPIO_PIN_4) == 0){
			SysCtlDelay(4000000);
			index++;
			checkIndex();
			getWidth(dC[index],load);
		}
	}
}
void getWidth(float width[], int load)
{
	if(width[0] == 0){
		PWMOutputState(PWM0_BASE,PWM_OUT_3_BIT,false);
	}else{
		PWMPulseWidthSet(PWM0_BASE,PWM_OUT_3,load * width[0]);
		PWMOutputState(PWM0_BASE,PWM_OUT_3_BIT,true);
	}
	if(width[1] == 0){
		PWMOutputState(PWM0_BASE,PWM_OUT_0_BIT,false);
	}else{
		PWMPulseWidthSet(PWM0_BASE,PWM_OUT_0,load * width[1]);
		PWMOutputState(PWM0_BASE,PWM_OUT_0_BIT,true);
	}
	if(width[2] == 0){
		PWMOutputState(PWM0_BASE,PWM_OUT_1_BIT,false);
	}else{
		PWMPulseWidthSet(PWM0_BASE,PWM_OUT_1,load * width[2]);
		PWMOutputState(PWM0_BASE,PWM_OUT_1_BIT,true);
	}
}

void checkIndex()
{
	if(index > 7){
		index = 0;
	}
}
