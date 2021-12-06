/*
 * Calendar_01.c
 * Hien thi ngay` duong bang led 7 ?oan
 * Created: 12/5/2021 9:02:18 PM
 * Author : PC
 */ 

#define F_CPU		8000000UL

#include <util/delay.h>
#include <avr/interrupt.h>
#include <stdio.h>
#include <avr/io.h>
#include "myDS1307RTC.h"
#include <avr/myLCD.h>
#include <stdbool.h>
#include <math.h>

/*
	Define MAX7219 - SPI
*/

#define PIN_SCK                   PORTB7
#define PIN_MOSI                  PORTB5
#define PIN_SS                    PORTB4

#define ON                        1
#define OFF                       0

#define MAX7219_LOAD1             PORTB |= (1<<PIN_SS)
#define MAX7219_LOAD0             PORTB &= ~(1<<PIN_SS)

#define MAX7219_MODE_DECODE       0x09
#define MAX7219_MODE_INTENSITY    0x0A
#define MAX7219_MODE_SCAN_LIMIT   0x0B
#define MAX7219_MODE_POWER        0x0C

#define MAX7219_DIGIT0            0x01
#define MAX7219_DIGIT1            0x02
#define MAX7219_DIGIT2            0x03
#define MAX7219_DIGIT3            0x04
#define MAX7219_DIGIT4            0x05
#define MAX7219_DIGIT5            0x06
#define MAX7219_DIGIT6            0x07
#define MAX7219_DIGIT7            0x08
#define MAX7219_CHAR_BLANK        0xF

volatile uint8_t	Second=00, Minute=9, Hour=13, Day=8, Date=5, Month=12, Year=21, Mode=1, AP=1;
volatile uint8_t	tData[7], Time_count = 0;
char dis[5];

//??i BCD sang th?p phân

uint8_t BCDToDec(uint8_t BCD)
{
	uint8_t L, H;
	L=BCD & 0x0F;
	H=(BCD>>4)*10;
	return (H+L);
}
uint8_t Dec2BCD(uint8_t Dec)
{
	uint8_t L, H;
	L=Dec % 10;
	H=(Dec/10)<<4;
	return (H+L);
}
void Decode(void){
	//BCD data converter function from DS1307 to DEC
	Second 	= BCDToDec(tData[0] & 0x7F);

	Minute 	= BCDToDec(tData[1]);
	
	if (Mode != 0)
	{
		Hour = BCDToDec(tData[2] & 0x1F); //Mode for 12h
	}
	else
	{
		Hour = BCDToDec(tData[2] & 0x3F); //Mode for 24h
	}
	Day		= BCDToDec(tData[3]);
	Date   	= BCDToDec(tData[4]);
	Month	= BCDToDec(tData[5]);
	Year	= BCDToDec(tData[6]);
}

void spiSendByte (char databyte)
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
	char digitsInUse = 8;
	char i = digitsInUse;
	// Loop until 0, but don't run for zero
	do {
		// Set each display in use to blank
		MAX7219_writeData(i, MAX7219_CHAR_BLANK);
	} while (--i);
}

/*
Display on LCD
*/
void Display(void)
{ 
	Second 	= BCDToDec(tData[0] & 0x7F);
	Minute 	= BCDToDec(tData[1]);
	
	if (Mode !=0) 	Hour = BCDToDec(tData[2] & 0x1F); //mode 12h
	else 		  	Hour = BCDToDec(tData[2] & 0x3F); //mode 24h
	
	Date   	= BCDToDec(tData[4]);
	Month	= BCDToDec(tData[5]);
	Year	= BCDToDec(tData[6]);
	
	clr_LCD();		//xoa LCD
	//in gio:phut:giay
	print_LCD("Time: ");
	sprintf(dis, "%i",Hour);
	move_LCD(1,7);  print_LCD(dis); move_LCD(1,9); putChar_LCD(':');
	sprintf(dis, "%i",Minute);
	move_LCD(1,10); print_LCD(dis); move_LCD(1,12);putChar_LCD(':');
	sprintf(dis, "%i",Second);
	move_LCD(1,13); print_LCD(dis);
	if (Mode !=0)	//mode 12h
	{ 
		move_LCD(1,16);
		if (bit_is_set(tData[2],5))  putChar_LCD('P'); //kiem tra bit AP, if AP=1
		else putChar_LCD('A');
	}
	//in nam-thang-ngay
	move_LCD(2,1);
	print_LCD("Date: ");
	sprintf(dis, "%i",Year);
	move_LCD(2,7);
	if (Year<10) putChar_LCD('0'); // neu nam <10, in them so 0 ben trai, vi du 09
	print_LCD(dis); move_LCD(2,9); putChar_LCD('-'); //in Nam
	sprintf(dis, "%i",Month);
	move_LCD(2,10); print_LCD(dis); move_LCD(2,12);putChar_LCD('-'); //in thang
	sprintf(dis, "%i",Date);
	move_LCD(2,13); print_LCD(dis);	//in Ngay
}

void Display_7seg (void)
{
	MAX7219_clearDisplay();
	
	MAX7219_writeData(0x08,MAX7219_CHAR_BLANK);
	MAX7219_writeData(0x07,MAX7219_CHAR_BLANK);
	MAX7219_writeData(0x06,(Second%10));
	MAX7219_writeData(0x05,(Second/10));
	MAX7219_writeData(0x04,(Minute%10));
	MAX7219_writeData(0x03,(Minute/10));
	MAX7219_writeData(0x02,(Hour%10));
	MAX7219_writeData(0x01,(Hour/10));
}
void Init_MAX()
{
	//MAX7219 init
	// SCK MOSI CS/LOAD/SS
	DDRB |= (1 << PIN_SCK) | (1 << PIN_MOSI) | (1 << PIN_SS);

	// SPI Enable, Master mode
	SPCR |= (1 << SPE) | (1 << MSTR)| (1<<SPR1);

	// Decode mode to "Font Code-B"
	MAX7219_writeData(MAX7219_MODE_DECODE, 0xFF);

	// Scan limit runs from 0.
	MAX7219_writeData(MAX7219_MODE_SCAN_LIMIT, 0x07);
	MAX7219_writeData(MAX7219_MODE_INTENSITY, 0x05);
	MAX7219_writeData(MAX7219_MODE_POWER, ON);
	MAX7219_writeData(MAX7219_MODE_DECODE, 0xFF);
}
int main(void)
{	
	//khoi dong LCD-----------------------
	init_LCD();
	clr_LCD();	
	_delay_ms(1000);
	//------------------------------------
	
	//khoi dong Timer0 lam bo dinh thi 1s------------------------------------------------
	TCCR0=(1<<CS02)|(0<<CS01)|(1<<CS00);	//CS02=1, CS01=0, CS00=1: chon Prescaler=1024 
    TIMSK=(1<<TOIE0);						//cho phep ngat khi co tran o T/C0
    sei();                      			//set bit I cho phep ngat toan cuc
	//----------------------------------------------------------------
	
	//khoi dong gia tri ghi vao DS1307---------------------------------------------------
	tData[0]=Dec2BCD(Second); 
	tData[1]=Dec2BCD(Minute); 
	if (Mode!=0) tData[2]=Dec2BCD(Hour)|(Mode<<6)|(AP<<5); //mode 12h
	else tData[2]=Dec2BCD(Hour);
	tData[3]=Dec2BCD(Day);
	tData[4]=Dec2BCD(Date);
	tData[5]=Dec2BCD(Month); 
	tData[6]=Dec2BCD(Year); 		
	TWI_Init(); //khoi dong TWI		
	TWI_DS1307_wblock(0x00, tData, 7); //ghi lien tiep cac bien thoi gian vao DS1307
	//----------------------------------------------------------------------------------
	_delay_ms(1);	//cho DS1307 xu li 
	
	//doc va hien thi thoi gian lan dau tien******************************************
	TWI_DS1307_wadr(0x00); //set dia chi ve 0
	_delay_ms(1);			//cho DS1307 xu li 
	TWI_DS1307_rblock(tData,7); //doc ca khoi thoi gian (7 bytes)	
	Display(); // hien thi ket qua len LCD	
	//************************************************************************************
	Init_MAX();
	Decode();
	while(1)
	{
		Display_7seg();
	}
	return 0;
}

ISR (TIMER0_OVF_vect){           
	Time_count++;
	if(Time_count>=10)
	{ 
		//doc DS1307
		TWI_DS1307_wadr(0x00); 				//set dia chi ve 0
		_delay_ms(1);		   				//cho DS1307 xu li 
		TWI_DS1307_rblock(tData,7); 	//doc ca khoi thoi gian (7 bytes)		
		//hien thi ket qua len LCD
		if(BCDToDec(tData[0]) !=Second)	//chi hien thi ket qua khi da qua 1s
		{ 	
			Second=BCDToDec(tData[0] & 0x7F);
			sprintf(dis, "%i",Second); 
			move_LCD(1,13); print_LCD("  ");
			move_LCD(1,13); print_LCD(dis);
			Decode();
			Display_7seg();
			if (Second==0) Display(); 		//moi phut cap nhat 1 lan			
		}
		Time_count=0; 
	}
}



