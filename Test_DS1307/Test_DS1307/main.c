/*
 * Test_DS1307.c
 *
 * Created: 12/1/2021 9:42:14 PM
 * Author : VTMKhoi
 */ 

#define F_CPU		8000000UL

#include <avr/io.h>
#include <util/delay.h>
#include "myDS1307RTC.h"
#include <avr/myLCD.h>


//------------------------MAX7219--------------------------------
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
#define MAX7219_MODE_TEST         0x0F
#define MAX7219_MODE_NOOP         0x00

#define MAX7219_DIGIT0            0x01
#define MAX7219_DIGIT1            0x02
#define MAX7219_DIGIT2            0x03
#define MAX7219_DIGIT3            0x04
#define MAX7219_DIGIT4            0x05
#define MAX7219_DIGIT5            0x06
#define MAX7219_DIGIT6            0x07
#define MAX7219_DIGIT7            0x08

// dinh nghia cac bien thoi gian
/*signed char Second = 59, Minute = 50, Hour = 11;*/
volatile int16_t Second = 59, Minute = 50, Hour = 11, Day = 7,
Date = 31, Month = 12, Year = 9, Mode = 0, AP = 1, A_Hour = 0, A_Minute = 0, timeZone = 7, lunarDate, lunarMonth, lunarYear, yyyy;
unsigned char font[10]={0xC0,0xF9,0xA4,0xB0,0x99,0x92,0x82,0xF8,0x80,0x90};		//ma hien thi led 7seg tu 0 -> 9
volatile uint8_t tData[7], Time_count = 0, blink=0;	//tData[7]: mang du lieu tam thoi
char dis[5];	//dung cho hàm printf de chuyen doi du lieu sang string va hien thi len LCD

// chuyen doi nhi phan sang thap phan
uint8_t BCDToDec(uint8_t BCD){
	uint8_t L, H;
	L=BCD & 0x0F;
	H=(BCD>>4)*10;
	return (H+L);
}
// chuyen doi thap phan sang nhi phan
uint8_t DecToBCD(uint8_t Dec){
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

void Display(void){
	clr_LCD();
	//=============PRINT HOUR MINUTE AND SECOND================
	if(set_alarm == true){
		clr_LCD;
		print_LCD("Set Alarm: ");
		sprintf(dis, "%i",A_Hour);
		move_LCD(2,7);
		if(A_Hour<10) putChar_LCD('0');
		print_LCD(dis);
		move_LCD(2,9); putChar_LCD(':');
		
		sprintf(dis, "%i",A_Minute);
		move_LCD(2,10);
		if(A_Minute<10) putChar_LCD('0');
		print_LCD(dis);
		print_LCD("      ");
	}
	else{
		
		if(set == false) print_LCD("Time: ");
		if(set == true) print_LCD("SetT: ");
		sprintf(dis, "%i",Hour);
		move_LCD(1,7);
		if(Hour<10) putChar_LCD('0');
		print_LCD(dis);
		move_LCD(1,9); putChar_LCD(':');
		
		sprintf(dis, "%i",Minute);
		move_LCD(1,10);
		if(Minute<10) putChar_LCD('0');
		print_LCD(dis);
		move_LCD(1,12);putChar_LCD(':');
		
		sprintf(dis, "%i",Second);
		move_LCD(1,13);
		if(Second<10) putChar_LCD('0');
		print_LCD(dis);
		if (Mode != 0){ //Mode 12h
			move_LCD(1,16);
			if (bit_is_set(tData[2],5))  putChar_LCD('P'); //AP resister, if AP = 1, print P
			else putChar_LCD('A');	//AP = 0, print A
		}

		//=============PRINT DATE MONTH AND YEAR================
		move_LCD(2,1);
		if(set == false) print_LCD("Date: ");// Date: 00-00-00
		if(set == true) print_LCD("SetD: ");
		if(Date<10) putChar_LCD('0');
		sprintf(dis,"%i",Date);
		print_LCD(dis);
		putChar_LCD('-');
		
		if (Month<10) putChar_LCD('0');
		sprintf(dis, "%i",Month);
		print_LCD(dis);
		putChar_LCD('-');
		
		if(Year<10) putChar_LCD('0');
		sprintf(dis, "%i",Year);
		print_LCD(dis);
		
		print_LCD("  ");
	}
	
}

void Display_7seg (void){
	/********display time -> hh:mm:ss***************/
	
	if (SW_time_date == 0)
	{
		MAX7219_writeData(MAX7219_MODE_DECODE, 0xBF);
		MAX7219_clearDisplay();
		
		MAX7219_writeData(0x08,Day);
		MAX7219_writeData(0x07,0x0F);
		MAX7219_writeData(0x06,(Second%10));
		MAX7219_writeData(0x05,(Second/10));
		MAX7219_writeData(0x04,(Minute%10));
		MAX7219_writeData(0x03,(Minute/10));
		MAX7219_writeData(0x02,(Hour%10));
		MAX7219_writeData(0x01,(Hour/10));
		
	}
	/********display date -> DD:MM:YY***************/
	else if (SW_time_date == 1)
	{
		MAX7219_writeData(MAX7219_MODE_DECODE, 0xFF);
		MAX7219_clearDisplay();
		
		MAX7219_writeData(0x08,((yyyy%1000)%10));
		MAX7219_writeData(0x07,(((yyyy%1000)/10)%10));
		MAX7219_writeData(0x06,(((yyyy%1000)/100)%10));
		MAX7219_writeData(0x05,(yyyy/1000));
		MAX7219_writeData(0x04,(Month%10));
		MAX7219_writeData(0x03,(Month/10));
		MAX7219_writeData(0x02,(Date%10));
		MAX7219_writeData(0x01,(Date/10));
		
	}
	/********display AM LICH *********************/
	else if (SW_time_date==2)
	{
		MAX7219_writeData(MAX7219_MODE_DECODE, 0xFF);
		MAX7219_clearDisplay();
		
		MAX7219_writeData(0x08,((lunarYear%1000)%10));
		MAX7219_writeData(0x07,(((lunarYear%1000)/10)%10));
		MAX7219_writeData(0x06,(((lunarYear%1000)/100)%10));
		MAX7219_writeData(0x05,(lunarYear/1000));
		MAX7219_writeData(0x04,(lunarMonth%10));
		MAX7219_writeData(0x03,(lunarMonth/10));
		MAX7219_writeData(0x02,(lunarDate%10));
		MAX7219_writeData(0x01,(lunarDate/10));
		
	}
	else
	{
		MAX7219_writeData(MAX7219_MODE_DECODE, 0xFF);
		MAX7219_clearDisplay();
		
		MAX7219_writeData(0x04,(A_Minute%10));
		MAX7219_writeData(0x03,(A_Minute/10));
		MAX7219_writeData(0x02,(A_Hour%10));
		MAX7219_writeData(0x01,(A_Hour/10));
	}

}
int main(void)
{
    /* Replace with your application code */
    while (1) 
    {
    }
}

