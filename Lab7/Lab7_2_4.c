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

//Defines for DS1307
#define SLAVE_ADDRESS 0x68
#define SEC 0x00
#define MIN 0x01
#define HRS 0x02
#define DAY 0x03
#define DATE 0x04
#define MONTH 0x05
#define YEAR 0x06
#define CNTRL 0x07

#define ALLPINS GPIO_PIN_0|GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3|GPIO_PIN_4|GPIO_PIN_5|GPIO_PIN_6|GPIO_PIN_7

void initializeLCD();
void enterCommand(int command,float delayTime);
void writeWord(uint8_t string[], uint8_t times);
void writeCharDisp(unsigned char x);

void keysConfig();

uint32_t g_ui32SysClock;
uint8_t text[16][16] = {{"Time: "}, {"Date: "},{"Current Date: "},{"Month: "},{"Day: "}, {"Year: "}, {"Current Time: "},{"Hour: "},{"Min: "}, {"Sec: "}, {"Alarm Config: "},{"Hour: "},{"Min: "}, {"Sec: "},{"Alarm: "},{"WAKE UP"}};
int timesArray[16] = {5,5,14,7,5,6, 14,6, 5, 5, 14,6, 5, 5,7,7 };
int status;
int status2 = 0;
uint8_t current = 0;
int currentPos = 0;
int currentMonth, currentDay, currentYear, currentHour, currentMin, currentSec, alarmHour, alarmMin, alarmSec;
int currentTime[9];
uint32_t period;
int on = 0;

//initialize I2C module 1
void InitI2C3(void)
{
	//enable I2C module 1
	SysCtlPeripheralEnable(SYSCTL_PERIPH_I2C1);
	//reset module
	SysCtlPeripheralReset(SYSCTL_PERIPH_I2C1);
	//enable GPIO peripheral that contains I2C 0
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);


	// Configure the pin muxing for I2C3 functions on port E4 and E5.
	GPIOPinConfigure(GPIO_PA6_I2C1SCL);
	GPIOPinConfigure(GPIO_PA7_I2C1SDA);

	// Select the I2C function for these pins.
	GPIOPinTypeI2CSCL(GPIO_PORTA_BASE, GPIO_PIN_6);
	GPIOPinTypeI2C(GPIO_PORTA_BASE, GPIO_PIN_7);
	I2CTxFIFOConfigSet(I2C1_BASE, I2C_FIFO_CFG_TX_MASTER);

	// Enable and initialize the I2C3 master module. Use the system clock for
	// the I2C1 module. The last parameter sets the I2C data transfer rate.
	// If false the data rate is set to 100kbps and if true the data rate will
	// be set to 400kbps.
	I2CMasterInitExpClk(I2C1_BASE, g_ui32SysClock, false);
	//clear I2C FIFOs
	//HWREG(I2C1_BASE + I2C_O_FIFOCTL) = 80008000;
	I2CRxFIFOFlush(I2C1_BASE);
	I2CTxFIFOFlush(I2C1_BASE);
}
//sends an I2C command to the specified slave
void I2CSend(uint8_t slave_addr, uint8_t num_of_args, ...)
{
	// Tell the master module what address it will place on the bus when
	// communicating with the slave.
	I2CMasterSlaveAddrSet(I2C1_BASE, slave_addr, false);
	//stores list of variable number of arguments
	va_list vargs;
	//specifies the va_list to "open" and the last fixed argument
	//so vargs knows where to start looking
	va_start(vargs, num_of_args);
	//put data to be sent into FIFO
	I2CMasterDataPut(I2C1_BASE, va_arg(vargs, uint32_t));
	//if there is only one argument, we only need to use the
	//single send I2C function
	if(num_of_args == 1)
	{
		//Initiate send of data from the MCU
		I2CMasterControl(I2C1_BASE, I2C_MASTER_CMD_SINGLE_SEND);
		// Wait until MCU is done transferring.
		while(I2CMasterBusy(I2C1_BASE));
		//"close" variable argument list
		va_end(vargs);
	}
	//otherwise, we start transmission of multiple bytes on the
	//I2C bus
	else
	{
		//Initiate send of data from the MCU
		I2CMasterControl(I2C1_BASE, I2C_MASTER_CMD_BURST_SEND_START);
		// Wait until MCU is done transferring.
		while(I2CMasterBusy(I2C1_BASE));
		//send num_of_args-2 pieces of data, using the
		//BURST_SEND_CONT command of the I2C module
		unsigned char i;
		for(i = 1; i < (num_of_args - 1); i++)
		{
			//put next piece of data into I2C FIFO
			I2CMasterDataPut(I2C1_BASE, va_arg(vargs, uint32_t));
			//send next data that was just placed into FIFO
			I2CMasterControl(I2C1_BASE, I2C_MASTER_CMD_BURST_SEND_CONT);
			// Wait until MCU is done transferring.
			while(I2CMasterBusy(I2C1_BASE));
		}
		//put last piece of data into I2C FIFO
		I2CMasterDataPut(I2C1_BASE, va_arg(vargs, uint32_t));
		//send next data that was just placed into FIFO
		I2CMasterControl(I2C1_BASE, I2C_MASTER_CMD_BURST_SEND_FINISH);
		// Wait until MCU is done transferring.
		while(I2CMasterBusy(I2C1_BASE));
		//"close" variable args list
		va_end(vargs);
	}
}

//read specified register on slave device
uint32_t I2CReceive(uint32_t slave_addr, uint8_t reg)
{
	//specify that we are writing (a register address) to the
	//slave device
	I2CMasterSlaveAddrSet(I2C1_BASE, slave_addr, false);
	//specify register to be read
	I2CMasterDataPut(I2C1_BASE, reg);
	//send control byte and register address byte to slave device
	I2CMasterControl(I2C1_BASE, I2C_MASTER_CMD_BURST_SEND_START);
	//wait for MCU to finish transaction
	while(I2CMasterBusy(I2C1_BASE));
	//specify that we are going to read from slave device
	I2CMasterSlaveAddrSet(I2C1_BASE, slave_addr, true);
	//send control byte and read from the register we
	//specified
	I2CMasterControl(I2C1_BASE, I2C_MASTER_CMD_SINGLE_RECEIVE);
	//wait for MCU to finish transaction
	while(I2CMasterBusy(I2C1_BASE));
	//return data pulled from the specified register
	return I2CMasterDataGet(I2C1_BASE);
}

//convert dec to bcd
unsigned char dec2bcd(unsigned char val)
{
	return (((val / 10) << 4) | (val % 10));
}

// convert BCD to binary
unsigned char bcd2dec(unsigned char val)
{
	return (((val & 0xF0) >> 4) * 10) + (val & 0x0F);
}

//Set Time
void SetTimeDate(unsigned char sec, unsigned char min, unsigned char hour,unsigned char day, unsigned char date, unsigned char month,unsigned char year)
{
	I2CSend(SLAVE_ADDRESS,8,SEC,dec2bcd(sec),dec2bcd(min),dec2bcd(hour),dec2bcd(day),dec2bcd(date),dec2bcd(month),dec2bcd(year));
}

//Get Time and Date
unsigned char GetClock(unsigned char reg)
{
	unsigned char clockData = I2CReceive(SLAVE_ADDRESS,reg);
	return bcd2dec(clockData);
}

void main(void)
{
	SysCtlClockSet(SYSCTL_SYSDIV_5 | SYSCTL_USE_PLL | SYSCTL_OSC_MAIN | SYSCTL_XTAL_16MHZ);
	g_ui32SysClock =SysCtlClockGet();

	//initialize I2C module 1
	InitI2C3();

	/*Enable Peripherals*/
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOB);        //Enable the peripheral port to use. (B) // LCD
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);        //Enable the peripheral port to use. (F) //LCD R/W R/S E

	/* Buzzer */
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOC);												//Enable GPIO PORT C
	SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER0);

	/* Peripheral Configuration */
	TimerConfigure(TIMER0_BASE,TIMER_CFG_PERIODIC);												//Configure timer to count down
	GPIOPinTypeGPIOOutput(GPIO_PORTC_BASE,GPIO_PIN_4);											//Set Port C PIN 4 as output

	period = ((SysCtlClockGet()/1000)/2)-1;										//Calculate the period of the timer at 50% duty cycle
	TimerLoadSet(TIMER0_BASE,TIMER_A,period);

	IntEnable(INT_TIMER0A);											//Enable Timer 0 subtimer A
	TimerIntEnable(TIMER0_BASE,TIMER_TIMA_TIMEOUT);					//Enable Timer Interrupt when the timer runs out
	IntMasterEnable();												//Enable master interrupt timer


	/* LCD pin configuration */
	GPIOPinTypeGPIOOutput(GPIO_PORTB_BASE, ALLPINS);                                        // Set all port B pins as output pins
	GPIOPinTypeGPIOOutput(GPIO_PORTF_BASE, GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3);               //PIN1 : R/S (R/S: 1 for data ** 0 for instruction) *** PIN2 : R/W (R/S: 1 for Read ** 0 for write) *** PIN3 : E(Chip Enable signal).

	/* Interrupt configuration */
	GPIOPinTypeGPIOInput(GPIO_PORTA_BASE,GPIO_PIN_2|GPIO_PIN_3|GPIO_PIN_4); //UP = A2 DOWN= A3 ENTER = A4                                          //Input configuration PC4,PC5,PC6
	// GPIOPadConfigSet(GPIO_PORTA_BASE, GPIO_PIN_2|GPIO_PIN_3|GPIO_PIN_4, GPIO_STRENGTH_4MA , GPIO_PIN_TYPE_STD_WPD ) ;  //Set the pin current strength (default is 4mA) and Pin Type to Pull DOWN

	GPIOPadConfigSet(GPIO_PORTA_BASE, GPIO_PIN_2, GPIO_STRENGTH_4MA , GPIO_PIN_TYPE_STD_WPD ) ;//Set the pin current strength (default is 2mA) and Pin Type to Pull Down
	GPIOPadConfigSet(GPIO_PORTA_BASE, GPIO_PIN_3, GPIO_STRENGTH_4MA , GPIO_PIN_TYPE_STD_WPU ) ;//Set the pin current strength (default is 2mA) and Pin Type to Pull Up

	/* Tiva buttons configuration */
	HWREG(GPIO_PORTF_BASE + GPIO_O_LOCK) = GPIO_LOCK_KEY;
	HWREG(GPIO_PORTF_BASE + GPIO_O_CR) = 0X01;
	HWREG(GPIO_PORTF_BASE + GPIO_O_LOCK) = 0;
	GPIODirModeSet(GPIO_PORTF_BASE,GPIO_PIN_4,GPIO_DIR_MODE_IN);                            //Setup PORT F pin 4 as input pin
	GPIOPadConfigSet(GPIO_PORTF_BASE,GPIO_PIN_4,GPIO_STRENGTH_2MA, GPIO_PIN_TYPE_STD_WPU);  //Setup PORT F pin 4 as pull up

	initializeLCD();

	unsigned char sec,min,hour,day,date,month,year;
	/*---------------Current Date Configuration-----------------*/


	writeWord(text[2],timesArray[2]);       //Current Date Message
	enterCommand(0xC0,493.3);               //Move Cursor
	writeWord(text[3],timesArray[3]);       //////////////////////Current Month
	enterCommand(0xC7,493.3);
	writeCharDisp(current + 48);
	keysConfig();
	enterCommand(0x01,20400);		//Clear display command

	writeWord(text[2],timesArray[2]);       //Current Date Message
	enterCommand(0xC0,493.3);
	writeWord(text[4],timesArray[4]);       ////////////////Current Day Message
	enterCommand(0xC7,493.3);
	writeCharDisp(current + 48);
	keysConfig();
	enterCommand(0x01,20400);		//Clear display command

	writeWord(text[2],timesArray[2]);       //Current Date Message
	enterCommand(0xC0,493.3);
	writeWord(text[5],timesArray[5]);       //Current Year Message
	enterCommand(0xC7,493.3);
	writeCharDisp(current + 48);
	keysConfig();
	enterCommand(0x01,20400);				//Clear display command

	/*---------------Current Time Configuration----------------- */
	writeWord(text[6],timesArray[6]);       //Current Time Message
	enterCommand(0xC0,493.3);
	writeWord(text[7],timesArray[7]);       ////////Current Hour Message
	enterCommand(0xC7,493.3);
	writeCharDisp(current + 48);
	keysConfig();
	enterCommand(0x01,20400);				//Clear display command

	writeWord(text[6],timesArray[6]);       //Current Time Message
	enterCommand(0xC0,493.3);
	writeWord(text[8],timesArray[8]);       //////////Current Min Message
	enterCommand(0xC7,493.3);
	writeCharDisp(current + 48);
	keysConfig();
	enterCommand(0x01,20400);				//Clear display command

	writeWord(text[6],timesArray[6]);       //Current Time Message
	enterCommand(0xC0,493.3);
	writeWord(text[9],timesArray[9]);       /////////Current Sec Message
	enterCommand(0xC7,493.3);
	writeCharDisp(current + 48);
	keysConfig();
	enterCommand(0x01,20400);				//Clear display command

	/*---------------------Alarm Configuration------------------*/
	writeWord(text[10],timesArray[10]);       //Alarm Config Message
	enterCommand(0xC0,493.3);
	writeWord(text[11],timesArray[11]);       //Alarm Hour Message
	enterCommand(0xC7,493.3);
	writeCharDisp(current + 48);
	keysConfig();
	enterCommand(0x01,20400);				//Clear display command

	writeWord(text[10],timesArray[10]);       //Alarm Config Message
	enterCommand(0xC0,493.3);
	writeWord(text[12],timesArray[12]);       //Alarm Min Message
	enterCommand(0xC7,493.3);
	writeCharDisp(current + 48);
	keysConfig();
	enterCommand(0x01,20400);				//Clear display command

	writeWord(text[10],timesArray[10]);       //Alarm Config Message
	enterCommand(0xC0,493.3);
	writeWord(text[9],timesArray[9]);       /////////alarm Sec Message
	enterCommand(0xC7,493.3);
	writeCharDisp(current + 48);
	keysConfig();
	enterCommand(0x01,20400);				//Clear display command


	SetTimeDate(currentTime[5],currentTime[4],currentTime[3],5,currentTime[1],currentTime[0],currentTime[2]);
	SysCtlDelay(4000000);


	while(1)
	{
		sec = GetClock(SEC);
		min = GetClock(MIN);
		hour = GetClock(HRS);
		day = GetClock(DAY);
		date = GetClock(DATE);
		month = GetClock(MONTH);
		year = GetClock(YEAR);

		while(!(GPIOPinRead(GPIO_PORTF_BASE, GPIO_PIN_4))){
			SysCtlDelay(8000000);
			if(!on){
				if(status2 != 0){
					enterCommand(0x01,20400);
					status2 = 0;
				}else{
					status2 = 1;
				}
			}else{
				on =0;
				TimerDisable(TIMER0_BASE,TIMER_A);
			}
		}
		if(status2 ==0 ){
			/*--------------------- Set current clock values --------------------------*/

			//SetTimeDate(currentSec,currentMin,currentHour,day,5,month,year);
			writeWord(text[0],timesArray[0]);

			writeCharDisp(hour/10 + 48);
			writeCharDisp((hour-((hour/10)*10)) + 48);
			writeCharDisp(10 + 48);   //  ":"

			//Write the MIN
			writeCharDisp(min/10 +48);
			writeCharDisp((min-((min/10)*10)) + 48);
			writeCharDisp(10 + 48);   // ":"

			//Write the SEC
			writeCharDisp(sec/10 +48);
			writeCharDisp((sec-((sec/10)*10)) + 48);
			//writeCharDisp(10 + 48);   // ":"

			//DATE
			enterCommand(0xC0,493.3);
			writeWord(text[1],timesArray[1]);
			writeCharDisp(date/10 +48);
			writeCharDisp((date-((date/10)*10)) + 48);
			writeCharDisp(10 + 48);   // writes semi-colon character = ":"

			//month
			writeCharDisp(month/10 +48);
			writeCharDisp((month-((month/10)*10)) + 48);
			writeCharDisp(10 + 48);   // writes semi-colon character = ":"

			//year
			writeCharDisp(year/10 +48);
			writeCharDisp((year-((year/10)*10)) + 48);

			//enterCommand(0x80,493.3);
			SysCtlDelay(3000000);
			enterCommand(0x01,20400);

		}else if(status2 == 1){
			//show alarm
			writeWord(text[14],timesArray[14]);
			writeCharDisp(currentTime[6]/10 + 48);
			writeCharDisp((currentTime[6]-((currentTime[6]/10)*10)) + 48);
			writeCharDisp(10 + 48);   //  ":"

			//Write the MIN
			writeCharDisp(currentTime[7]/10 +48);
			writeCharDisp((currentTime[7]-((currentTime[7]/10)*10)) + 48);
			writeCharDisp(10 + 48);   //  ":"

			//Write the MIN
			writeCharDisp(currentTime[8]/10 +48);
			writeCharDisp((currentTime[8]-((currentTime[8]/10)*10)) + 48);
			status2 = 2;
		}
		if((hour == currentTime[6]) && (min == currentTime[7])&&(sec == currentTime[8])){
			//writeWord(text[15],timesArray[15]);
			TimerEnable(TIMER0_BASE,TIMER_A);								//Enable Timer
			on = 1;
		}
	}
}

void keysConfig(){
	status = 1;
	while(status){
		while(!(GPIOPinRead(GPIO_PORTA_BASE, GPIO_PIN_2))){
			SysCtlDelay(4000000);
			current++;
			enterCommand(0xC7,493.3);
			writeCharDisp(current/10 +48);
			writeCharDisp((current-((current/10)*10)) + 48);
			//writeCharDisp(current + 48);
		}
		while(GPIOPinRead(GPIO_PORTA_BASE, GPIO_PIN_3)){
			SysCtlDelay(4000000);
			current--;
			enterCommand(0xC7,493.3);
			writeCharDisp(current/10 +48);
			writeCharDisp((current-((current/10)*10)) + 48);
			//writeCharDisp(current + 48);
		}
		/*------------------------Pressed F4---------------------------*/
		while(!(GPIOPinRead(GPIO_PORTF_BASE, GPIO_PIN_4))){
			SysCtlDelay(4000000);                                	//Running at 30ms
			currentTime[currentPos] = current;
			currentPos++;
			current = 0;
			status = 0;
		}
	}
}

/* LCD initialization function */
void initializeLCD()
{
	SysCtlDelay(533333.3333);// Starting delay (40ms).

	enterCommand(0x3C,520);//Function Set command

	enterCommand(0x3C,493.3);//Function Set

	enterCommand(0x0F,493.3);//Display ON

	enterCommand(0x01,20400);//Clear display command

	enterCommand(0x06,20400);//Entry Mode Set

	enterCommand(0x01,20400);//Clear display command
}

/* Enter command to LCD function; command is the desired LCD instruction, delayTime is the required time that the chip enable needs to be on in order to
 * successfully send the instruction */
void enterCommand(int command,float delayTime)
{
	GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_1|GPIO_PIN_2, 0x00); //R/S & R/W = 0
	GPIOPinWrite(GPIO_PORTB_BASE, ALLPINS, command); //enter command on port
	GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_3, 0x08);//Enable On
	SysCtlDelay(delayTime);//value for the desired delay
	GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_3, 0x00);        //Enable Off
}

/* Function for writing words in the LCD, string is the word that want to be written, times is the number of characters that the string has */
void writeWord(uint8_t string[], uint8_t times)
{
	GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_1, 0x02);//RS enable

	uint8_t i =0;//initialize for loop
	for(; i < times; i++){
		GPIOPinWrite(GPIO_PORTB_BASE, ALLPINS, string[i]);//write character
		GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_3, 0x08);//Enable On
		SysCtlDelay(94666);//Delay 7.1ms(Calculated)
		GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_3, 0x00);//Enable Off
	}

	GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_1, 0x00);//RS disable
}
void writeCharDisp(unsigned char x){
	GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_1, 0x02);//RS enable
	GPIOPinWrite(GPIO_PORTB_BASE, ALLPINS, x  );
	GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_3, 0x08);//Enable On
	SysCtlDelay(50000);//Delay 7.1ms(Calculated)
	GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_3, 0x00);//Enable Off
	GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_1, 0x00);//RS disable

}

/* Timer interrupt handler */
void Timer0IntHandler(void)
{
	TimerIntClear(TIMER0_BASE, TIMER_TIMA_TIMEOUT);					//Clear timer interrupt flag
	if(GPIOPinRead(GPIO_PORTC_BASE,GPIO_PIN_4)){					//Check pin status
		GPIOPinWrite(GPIO_PORTC_BASE, GPIO_PIN_4,0);				//If the buzzer is on ,turn it off
	}else{
		GPIOPinWrite(GPIO_PORTC_BASE,GPIO_PIN_4,16);				//If the buzzer if off, turn it on
	}
}
