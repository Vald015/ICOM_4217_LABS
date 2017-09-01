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
#include "driverlib/debug.h"
#include "driverlib/adc.h"

main(void) {

	uint32_t ADCValues[4];
	volatile uint32_t x;
	volatile uint32_t y;
	volatile uint32_t z;

	SysCtlClockFreqSet((SYSCTL_XTAL_25MHZ | SYSCTL_OSC_MAIN|SYSCTL_USE_PLL|SYSCTL_CFG_VCO_480),120000000);

	SysCtlClockSet(SYSCTL_SYSDIV_5 | SYSCTL_USE_PLL | SYSCTL_OSC_MAIN| SYSCTL_XTAL_16MHZ);
	SysCtlPeripheralEnable(SYSCTL_PERIPH_ADC0);
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOE);
	ADCHardwareOversampleConfigure(ADC0_BASE,64);

	//ADCReferenceSet(ADC0_BASE, ADC_REF_EXT_3V); //Set reference to the internal reference
	// You can set it to 1V or 3 V
	GPIOPinTypeADC(GPIO_PORTE_BASE, GPIO_PIN_0|GPIO_PIN_1|GPIO_PIN_2); //Configure GPIO as ADC

	//ADCSequenceDisable(ADC0_BASE, 1); //It is always a good practice to disable ADC prior to usage ,else the ADC may not be accurate due to previous initializations
	ADCSequenceConfigure(ADC0_BASE, 1, ADC_TRIGGER_PROCESSOR, 0); //Use the 3rd Sample sequencer

	ADCSequenceStepConfigure(ADC0_BASE, 1, 0,ADC_CTL_CH3 );
	ADCSequenceStepConfigure(ADC0_BASE, 1, 1,ADC_CTL_CH2 );
	ADCSequenceStepConfigure(ADC0_BASE, 1, 2,ADC_CTL_CH1 | ADC_CTL_IE | ADC_CTL_END);
	//Configure ADC to read from channel 8 ,trigger the interrupt to end data capture //

//	ADCIntRegister(ADC0_BASE,3,&ISR_ADC_Read);
//	ADCIntEnable(ADC0_BASE,3);
	ADCSequenceEnable(ADC0_BASE, 1);   //Enable the ADC
	//IntMasterEnable();
	//ADCIntClear(ADC0_BASE, 3);     //Clear interrupt to proceed to  data capture

	while (1) {
		ADCIntClear(ADC0_BASE, 1);     //Clear interrupt to proceed to  data capture
		ADCProcessorTrigger(ADC0_BASE, 1);   //Ask processor to trigger ADC
		while (!ADCIntStatus(ADC0_BASE, 1, false))
		{ //Do nothing until interrupt is triggered
		}

		//ADCIntClear(ADC0_BASE, 3); //Clear Interrupt to proceed to next data capture
		ADCSequenceDataGet(ADC0_BASE,1, ADCValues); //pui32ADC0Value is the value read
		x = ADCValues[0];
		y = ADCValues[1];
		z = ADCValues[2];
		//SysCtlDelay(SysCtlClockGet() / 12);
	} //Suitable delay
}
