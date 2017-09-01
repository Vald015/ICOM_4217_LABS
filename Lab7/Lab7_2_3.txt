#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include "inc/hw_i2c.h"
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "inc/hw_gpio.h"
#include "driverlib/i2c.h"
#include "driverlib/sysctl.h"
#include "driverlib/gpio.h"
#include "driverlib/pin_map.h"
#include "driverlib/uart.h"
#include "utils/uartstdio.h"

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
void writeCharDisp( unsigned char x);
uint32_t g_ui32SysClock;
uint8_t text[2][6] = {{"Time:"}, {"Date:"}};
int timesArray[2] = {5,5};


//initialize I2C module 1
void InitI2C3(void)
{
    //enable I2C module 0
    SysCtlPeripheralEnable(SYSCTL_PERIPH_I2C1);
    //reset module
    SysCtlPeripheralReset(SYSCTL_PERIPH_I2C1);
    //enable GPIO peripheral that contains I2C1
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);


    // Configure the pin muxing for I2C1 functions on port A6 and A7.
    GPIOPinConfigure(GPIO_PA6_I2C1SCL);
    GPIOPinConfigure(GPIO_PA7_I2C1SDA);

    // Select the I2C function for these pins.
    GPIOPinTypeI2CSCL(GPIO_PORTA_BASE, GPIO_PIN_6);
    GPIOPinTypeI2C(GPIO_PORTA_BASE, GPIO_PIN_7);
    I2CTxFIFOConfigSet(I2C1_BASE, I2C_FIFO_CFG_TX_MASTER);

    // Enable and initialize the I2C1 master module. Use the system clock for
    // the I2C1 module. The last parameter sets the I2C data transfer rate.
    // If false the data rate is set to 100kbps and if true the data rate will
    // be set to 400kbps.
    I2CMasterInitExpClk(I2C1_BASE, g_ui32SysClock, false);
    //clear I2C FIFOs
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

    SetTimeDate(30,41,7,4,21,6,17);

    /*Enable Peripherals*/
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOB);        //Enable the peripheral port to use. (B) // LCD
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);        //Enable the peripheral port to use. (F) //LCD R/W R/S E


    /* LCD pin configuration */
    GPIOPinTypeGPIOOutput(GPIO_PORTB_BASE, ALLPINS);                                        // Set all port B pins as output pins
    GPIOPinTypeGPIOOutput(GPIO_PORTF_BASE, GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3);               //PIN1 : R/S (R/S: 1 for data ** 0 for instruction) *** PIN2 : R/W (R/S: 1 for Read ** 0 for write) *** PIN3 : E(Chip Enable signal).

    initializeLCD();

    unsigned char sec,min,hour,day,date,month,year;
    while(1)
    {
        sec = GetClock(SEC);
        min = GetClock(MIN);
        hour = GetClock(HRS);
        day = GetClock(DAY);
        date = GetClock(DATE);
        month = GetClock(MONTH);
        year = GetClock(YEAR);


        writeWord(text[0],timesArray[0]);

        //write the hour
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

        enterCommand(0x80,493.3);
        SysCtlDelay(1000000);
        enterCommand(0x01,20400);
    }
}

///* LCD initialization function */
void initializeLCD()
{
    SysCtlDelay(533333.3333);       // Starting delay (40ms).

    enterCommand(0x3C,520);         //Function Set command

    enterCommand(0x3C,493.3);       //Function Set

    enterCommand(0x0F,493.3);       //Display ON

    enterCommand(0x01,20400);       //Clear display command

    enterCommand(0x06,20400);       //Entry Mode Set

    enterCommand(0x01,20400);       //Clear display command
}

///* Enter command to LCD function; command is the desired LCD instruction, delayTime is the required time that the chip enable needs to be on in order to
// * successfully send the instruction */
void enterCommand(int command,float delayTime)
{
    GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_1|GPIO_PIN_2, 0x00); //R/S & R/W = 0
    GPIOPinWrite(GPIO_PORTB_BASE, ALLPINS, command);            //enter command on port
    GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_3, 0x08);            //Enable On
    SysCtlDelay(delayTime);                                     //value for the desired delay
    GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_3, 0x00);            //Enable Off
}
/* Function for writing words in the LCD, string is the word that want to be written, times is the number of characters that the string has */
void writeWord(uint8_t string[], uint8_t times)
{
    GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_1, 0x02);//RS enable

    uint8_t i =0;//initialize for loop
    for(; i < times; i++){
        GPIOPinWrite(GPIO_PORTB_BASE, ALLPINS, string[i]);      //write character
        GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_3, 0x08);        //Enable On
        SysCtlDelay(50000);                                     //Delay 7.1ms(Calculated)
        GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_3, 0x00);        //Enable Off
    }
    GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_1, 0x00);            //RS disable
}
void writeCharDisp( unsigned char x){
    GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_1, 0x02);//RS enable
    GPIOPinWrite(GPIO_PORTB_BASE, ALLPINS, x  );
    GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_3, 0x08);//Enable On
    SysCtlDelay(50000);//Delay 7.1ms(Calculated)
    GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_3, 0x00);//Enable Off
    GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_1, 0x00);//RS disable

}

