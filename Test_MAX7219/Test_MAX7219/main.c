/*
 * max7219test.c
 *
 * Created: 11/13/2021 11:09:13 AM
 * Author : VTMKhoi
 */ 

/*
Define SPI protocol
*/
#define F_CPU 8000000UL
#define PIN_SCK                   PORTB7
#define PIN_MOSI                  PORTB5
#define PIN_SS                    PORTB4

/*
Used for function ---MAX7219_MODE_POWER---
Two option for power mode
*/
#define ON                        1
#define OFF                       0

#define MAX7219_LOAD1             PORTB |= (1<<PIN_SS)
#define MAX7219_LOAD0             PORTB &= ~(1<<PIN_SS)

/*
Define register address 0
*/
#define MAX7219_MODE_DECODE       0x09
#define MAX7219_MODE_INTENSITY    0x0A
#define MAX7219_MODE_SCAN_LIMIT   0x0B
#define MAX7219_MODE_POWER        0x0C
#define MAX7219_MODE_TEST         0x0F
#define MAX7219_MODE_NOOP         0x00

/*
Define LED Digit, using 8 leds
*/
#define MAX7219_DIGIT0            0x01
#define MAX7219_DIGIT1            0x02
#define MAX7219_DIGIT2            0x03
#define MAX7219_DIGIT3            0x04
#define MAX7219_DIGIT4            0x05
#define MAX7219_DIGIT5            0x06
#define MAX7219_DIGIT6            0x07
#define MAX7219_DIGIT7            0x08

#define MAX7219_CHAR_BLANK        0xF
#define MAX7219_CHAR_NEGATIVE     0xA

#include <avr/io.h>
#include <util/delay.h>



char digitsInUse = 8;

void spiSendByte (uint8_t databyte)
{
	// Copy data into the SPI data register
	SPDR = databyte;
	// Wait until transfer is complete
	while (!(SPSR & (1 << SPIF)));
}

void MAX7219_writeData(uint8_t data_register, uint8_t data)
{
	MAX7219_LOAD0;
	// Send the register where the data will be stored
	spiSendByte(data_register);
	// Send the data to be stored
	spiSendByte(data);
	MAX7219_LOAD1;
}

void MAX7219_clearDisplay()
{
	char i = digitsInUse;
	// Loop until 0, but don't run for zero
	do {
		// Set each display in use to blank
		MAX7219_writeData(i, MAX7219_CHAR_BLANK);
	} while (--i);
}
void init_SPI()
{
	// SCK MOSI CS/LOAD/SS
	DDRB |= (1 << PIN_SCK) | (1 << PIN_MOSI) | (1 << PIN_SS);

	// SPI Enable, Master mode
	SPCR |= (1 << SPE) | (1 << MSTR)| (1<<SPR1);
}
int main(void)
{
	init_SPI();
	// Decode mode to "Font Code-B"
	MAX7219_clearDisplay();
	MAX7219_writeData(MAX7219_MODE_DECODE, 0xFD);

	// Scan limit runs from 0.
	MAX7219_writeData(MAX7219_MODE_SCAN_LIMIT, 0x07);
	MAX7219_writeData(MAX7219_MODE_INTENSITY, 0x01);
	MAX7219_writeData(MAX7219_MODE_POWER, ON);
	MAX7219_writeData(0x01,1);		//"1"
	MAX7219_writeData(0x02,0x0F);	//symbol "t"
	MAX7219_writeData(0x03,3);		//"3"
	MAX7219_writeData(0x04,4);
	MAX7219_writeData(0x05,5);
	MAX7219_writeData(0x06,6);
	MAX7219_writeData(0x07,7);
	MAX7219_writeData(0x08,8);
	/*
	D7_D6 : D5_D4 : D3_D2-D1_D0: hour_hour : min_min : sec_sec - symbol "t"_date
	*/
	while(1)
	{
		;
	}
}