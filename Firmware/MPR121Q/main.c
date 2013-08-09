/*
    MPR121 Test Code
	April 8, 2010
	by: Jim Lindblom
	
	This example code will both sense for touches, and drive LEDs
	ELE6-11 are used in GPIO mode, to drive 6 LEDs
	ELE0-5 are used as capacative touch sensors
	Triggering a touch sensor will cause a corresponding LED to illuminate
	
	For desired operation, you may need to play around with TOU_THRESH and REL_THRESH threshold values.
	With default settings, you can brush your fingers across the ELE0-5 pins to trigger the LEDs
	
	The 6 anodes of 2 RGB LEDs are tied to ELE6-11, cathodes tied to ground
	
	Tested on a 3.3V 8MHz Arduino Pro
	A4 (PC4) -> SDA
	A5 (PC5) -> SCL
	D2 (PD2) -> IRQ
*/

#include <avr/io.h>
#include <avr/pgmspace.h>
#include <avr/interrupt.h>
#include "types.h"
#include "defs.h"
#include "i2c.h"
#include "mpr121.h"

#define FOSC 8000000
#define MPR121_R 0xB5	// ADD pin is grounded
#define MPR121_W 0xB4	// So address is 0x5A

#define sbi(var, mask)   ((var) |= (uint8_t)(1 << mask))
#define cbi(var, mask)   ((var) &= (uint8_t)~(1 << mask))

///============Function Prototypes=========/////////////////
char mpr121Read(unsigned char address);
void mpr121Write(unsigned char address, unsigned char data);
void mpr121QuickConfig(void);
void mpr121ViewRegisters(void);
int checkInterrupt(void);

///============Initialize Prototypes=====//////////////////
void ioinit(void);
void delay_ms(uint16_t x);	// this is configured for 16MHz, goes much faster on my 8MHz Arduino

/////=========Global Variables======////////////////////

int main(void)
{
	char touchstatus;
	
	ioinit();
	i2cInit();
	delay_ms(100);
	checkInterrupt();
	
	mpr121QuickConfig();
	
	// Setup the GPIO pins to drive LEDs
	mpr121Write(GPIO_EN, 0xFF);	// 0x77 is GPIO enable
	mpr121Write(GPIO_DIR, 0xFF);	// 0x76 is GPIO Dir
	mpr121Write(GPIO_CTRL0, 0xFF);	// Set to LED driver
	mpr121Write(GPIO_CTRL1, 0xFF);	// GPIO Control 1
	mpr121Write(GPIO_CLEAR, 0xFF);	// GPIO Data Clear
	
	// Blink LEDs to begin
	for (int i = 0; i < 5; i++)
	{
		mpr121Write(GPIO_SET, 0xFF);
		delay_ms(500);
		mpr121Write(GPIO_CLEAR, 0xFF);
		delay_ms(500);
	}
	
	while(1)
	{
		while(checkInterrupt())
			;	// NIRQ will go low when a touch or release occurs, wait till then
		touchstatus = mpr121Read(0x00);	// Read touch status
		mpr121Write(GPIO_CLEAR, 0xFF);	// Clear all LEDs
		touchstatus = touchstatus << 2;	// Shift two bits over
		mpr121Write(GPIO_SET, touchstatus);	// Set LED
	}
}

char mpr121Read(unsigned char address)
{
	char data;
	
	i2cSendStart();
	i2cWaitForComplete();
	
	i2cSendByte(MPR121_W);	// write 0xB4
	i2cWaitForComplete();
	
	i2cSendByte(address);	// write register address
	i2cWaitForComplete();
	
	i2cSendStart();
	
	i2cSendByte(MPR121_R);	// write 0xB5
	i2cWaitForComplete();
	i2cReceiveByte(TRUE);
	i2cWaitForComplete();
	
	data = i2cGetReceivedByte();	// Get MSB result
	i2cWaitForComplete();
	i2cSendStop();
	
	cbi(TWCR, TWEN);	// Disable TWI
	sbi(TWCR, TWEN);	// Enable TWI
	
	return data;
}

void mpr121Write(unsigned char address, unsigned char data)
{
	i2cSendStart();
	i2cWaitForComplete();
	
	i2cSendByte(MPR121_W);	// write 0xB4
	i2cWaitForComplete();
	
	i2cSendByte(address);	// write register address
	i2cWaitForComplete();
	
	i2cSendByte(data);
	i2cWaitForComplete();
	
	i2cSendStop();
}

// MPR121 Quick Config
// This will configure all registers as described in AN3944
// Input: none
// Output: none
void mpr121QuickConfig(void)
{
	// Section A
	// This group controls filtering when data is > baseline.
	mpr121Write(MHD_R, 0x01);
	mpr121Write(NHD_R, 0x01);
	mpr121Write(NCL_R, 0x00);
	mpr121Write(FDL_R, 0x00);
	
	// Section B
	// This group controls filtering when data is < baseline.
	mpr121Write(MHD_F, 0x01);
	mpr121Write(NHD_F, 0x01);
	mpr121Write(NCL_F, 0xFF);
	mpr121Write(FDL_F, 0x02);
	
	// Section C
	// This group sets touch and release thresholds for each electrode
	mpr121Write(ELE0_T, TOU_THRESH);
	mpr121Write(ELE0_R, REL_THRESH);
	mpr121Write(ELE1_T, TOU_THRESH);
	mpr121Write(ELE1_R, REL_THRESH);
	mpr121Write(ELE2_T, TOU_THRESH);
	mpr121Write(ELE2_R, REL_THRESH);
	mpr121Write(ELE3_T, TOU_THRESH);
	mpr121Write(ELE3_R, REL_THRESH);
	mpr121Write(ELE4_T, TOU_THRESH);
	mpr121Write(ELE4_R, REL_THRESH);
	mpr121Write(ELE5_T, TOU_THRESH);
	mpr121Write(ELE5_R, REL_THRESH);
	/*mpr121Write(ELE6_T, TOU_THRESH);
	mpr121Write(ELE6_R, REL_THRESH);
	mpr121Write(ELE7_T, TOU_THRESH);
	mpr121Write(ELE7_R, REL_THRESH);
	mpr121Write(ELE8_T, TOU_THRESH);
	mpr121Write(ELE8_R, REL_THRESH);
	mpr121Write(ELE9_T, TOU_THRESH);
	mpr121Write(ELE9_R, REL_THRESH);
	mpr121Write(ELE10_T, TOU_THRESH);
	mpr121Write(ELE10_R, REL_THRESH);
	mpr121Write(ELE11_T, TOU_THRESH);
	mpr121Write(ELE11_R, REL_THRESH);*/
	
	// Section D
	// Set the Filter Configuration
	// Set ESI2
	mpr121Write(FIL_CFG, 0x04);
	
	// Section E
	// Electrode Configuration
	// Enable 6 Electrodes and set to run mode
	// Set ELE_CFG to 0x00 to return to standby mode
	// mpr121Write(ELE_CFG, 0x0C);	// Enables all 12 Electrodes
	mpr121Write(ELE_CFG, 0x06);		// Enable first 6 electrodes
	
	// Section F
	// Enable Auto Config and auto Reconfig
	/*mpr121Write(ATO_CFG0, 0x0B);
	mpr121Write(ATO_CFGU, 0xC9);	// USL = (Vdd-0.7)/vdd*256 = 0xC9 @3.3V
	mpr121Write(ATO_CFGL, 0x82);	// LSL = 0.65*USL = 0x82 @3.3V
	mpr121Write(ATO_CFGT, 0xB5);*/	// Target = 0.9*USL = 0xB5 @3.3V
}

int checkInterrupt(void)
{
	if ((PIND & (1<<2)) == 0)
		return 0;
	else
		return 1;
}

/*********************
 ****Initialize****
 *********************/
 
void ioinit (void)
{
    //1 = output, 0 = input
	DDRB = 0b01100000; //PORTB4, B5 output
    DDRC = 0b00010011; //PORTC4 (SDA), PORTC5 (SCL), PORTC all others are inputs
    DDRD = 0b11111010; //PORTD (RX on PD0), IRQ on PD2
	PORTC = 0b00110000; //pullups on the I2C bus
}