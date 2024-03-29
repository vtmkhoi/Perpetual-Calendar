/*
 * lich van nien
 *
 * Author : Vo Tan Minh Khoi
			Ngac Bao Nam
			Phung Thi Huong
 */ 

#define F_CPU	8000000UL
#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include "myDS1307RTC.h"
#include <stdbool.h>
#include <math.h>

// Define buttons
#define BTN_DDRD	DDRD
#define BTN_PORTD	PORTD
#define SW			2
#define ADJ			3

#define BTN_DDRB	DDRB
#define BTN_PORTB	PORTB
#define INCR		2

#define DDR_LED_O	DDRA
#define PORT_LED_O	PORTA
#define BIT_LED_O		0

#define PORT_BUZZER_O PORTD
#define DDR_BUZZER_O DDRD
#define BIT_BUZZER_O 7

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
#define MAX7219_CHAR_BLANK        0xF

//****Define digits
#define MAX7219_DIGIT0            0x01
#define MAX7219_DIGIT1            0x02
#define MAX7219_DIGIT2            0x03
#define MAX7219_DIGIT3            0x04
#define MAX7219_DIGIT4            0x05
#define MAX7219_DIGIT5            0x06
#define MAX7219_DIGIT6            0x07
#define MAX7219_DIGIT7            0x08

#define digitsInUse				  8

//--------------------------------------------------------------------------------------

//Define variables
uint8_t Second = 00, Minute = 10, Hour = 14, Day = 4, 
Date = 19, Month = 1, Year = 22, Mode = 0, AP = 1, A_Hour = 0, A_Minute = 0; 
double timeZone = 7;
uint8_t lunarDate, lunarMonth;
uint16_t lunarYear, yyyy;
uint8_t tData[7];					//tData[7]: mang du lieu tam thoi
uint16_t Time_count = 0, blink_count=0;
//char digitsInUse = 8;
bool set = false;					//set = true: cho phep dieu chinh thoi gian
bool EN_alarm = false;				//EN_alarm = true: Set alarm
bool blinkmode = false;				//blinkmode = true: Blink led per 500ms
uint8_t count = 0;
char SW_time_date = 0;

//****chuyen doi nhi phan sang thap phan****//
uint8_t BCDToDec(uint8_t BCD){
	uint8_t L, H;
	L=BCD & 0x0F;
	H=(BCD>>4)*10;
	return (H+L);
}
// chuyen doi thap phan sang nhi phan
uint8_t DecToBCD(uint8_t Dec)
{
	uint8_t L, H;
	L=Dec % 10;
	H=(Dec/10)<<4;
	return (H+L);
}

void Decode(void)
{
    //BCD data converter function from DS1307 to DEC
	Second 	= BCDToDec(tData[0] & 0x7F);
	Minute 	= BCDToDec(tData[1]);
	Hour	= BCDToDec(tData[2] & 0x3F);
	Day		= BCDToDec(tData[3]);
	Date   	= BCDToDec(tData[4]);
	Month	= BCDToDec(tData[5]);
	Year	= BCDToDec(tData[6]);
}

//Write to DS1307 time that want to change
void FixTime()
{
	tData[0] = DecToBCD(Second);
	tData[1] = DecToBCD(Minute);
	tData[2] = DecToBCD(Hour);
	tData[3] = DecToBCD(Day);
	tData[4] = DecToBCD(Date);
	tData[5] = DecToBCD(Month);
	tData[6] = DecToBCD(Year);
	TWI_DS1307_wblock(0x00, tData, 7);
	_delay_ms(1);
	TWI_DS1307_wadr(0x00);
	_delay_ms(1);
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
	char i = digitsInUse;
	// Loop until 0, but don't run for zero
	do {
		// Set each display in use to blank
		MAX7219_writeData(i, MAX7219_CHAR_BLANK);
	} while (--i);
}

void Display_7seg (void)
{
	if (blinkmode==0)
	{
		/********display time -> hh:mm:ss***************/
		
		if (SW_time_date == 0)
		{
			MAX7219_clearDisplay();
			
			MAX7219_writeData(MAX7219_DIGIT7,(Minute%10));
			MAX7219_writeData(MAX7219_DIGIT6,(Minute/10));
			MAX7219_writeData(MAX7219_DIGIT5,(Hour%10));
			MAX7219_writeData(MAX7219_DIGIT4,(Hour/10));
			MAX7219_writeData(MAX7219_DIGIT3,(Month%10));
			MAX7219_writeData(MAX7219_DIGIT2,(Month/10));
			MAX7219_writeData(MAX7219_DIGIT1,(Date%10));
			MAX7219_writeData(MAX7219_DIGIT0,(Date/10));
			
		}
		/********display date -> DD:MM:YY***************/
		else if (SW_time_date == 1)
		{
			MAX7219_clearDisplay();
			
			MAX7219_writeData(MAX7219_DIGIT7,((yyyy%1000)%10));
			MAX7219_writeData(MAX7219_DIGIT6,(((yyyy%1000)/10)%10));
			MAX7219_writeData(MAX7219_DIGIT5,(((yyyy%1000)/100)%10));
			MAX7219_writeData(MAX7219_DIGIT4,(yyyy/1000));
			MAX7219_writeData(MAX7219_DIGIT3,(Month%10));
			MAX7219_writeData(MAX7219_DIGIT2,(Month/10));
			MAX7219_writeData(MAX7219_DIGIT1,(Date%10));
			MAX7219_writeData(MAX7219_DIGIT0,(Date/10));
			
		}
		/********display AM LICH *********************/
		else if (SW_time_date==2)
		{
			MAX7219_clearDisplay();
			
			MAX7219_writeData(MAX7219_DIGIT7,((lunarYear%1000)%10));
			MAX7219_writeData(MAX7219_DIGIT6,(((lunarYear%1000)/10)%10));
			MAX7219_writeData(MAX7219_DIGIT5,(((lunarYear%1000)/100)%10));
			MAX7219_writeData(MAX7219_DIGIT4,(lunarYear/1000));
			MAX7219_writeData(MAX7219_DIGIT3,(lunarMonth%10));
			MAX7219_writeData(MAX7219_DIGIT2,(lunarMonth/10));
			MAX7219_writeData(MAX7219_DIGIT1,(lunarDate%10));
			MAX7219_writeData(MAX7219_DIGIT0,(lunarDate/10));
			
		}
		else
		{
			MAX7219_clearDisplay();
			
			MAX7219_writeData(MAX7219_DIGIT7,(A_Minute%10));
			MAX7219_writeData(MAX7219_DIGIT6,(A_Minute/10));
			MAX7219_writeData(MAX7219_DIGIT5,(A_Hour%10));
			MAX7219_writeData(MAX7219_DIGIT4,(A_Hour/10));
			MAX7219_writeData(MAX7219_DIGIT3,(Minute%10));
			MAX7219_writeData(MAX7219_DIGIT2,(Minute/10));
			MAX7219_writeData(MAX7219_DIGIT1,(Hour%10));
			MAX7219_writeData(MAX7219_DIGIT0,(Hour/10));
		}
	} 
	else
	{
		if ((count==1)&&(SW_time_date==0))	//blink date
		{
			MAX7219_clearDisplay();
			
			MAX7219_writeData(MAX7219_DIGIT7,(Minute%10));
			MAX7219_writeData(MAX7219_DIGIT6,(Minute/10));
			MAX7219_writeData(MAX7219_DIGIT5,(Hour%10));
			MAX7219_writeData(MAX7219_DIGIT4,(Hour/10));
			MAX7219_writeData(MAX7219_DIGIT3,(Month%10));
			MAX7219_writeData(MAX7219_DIGIT2,(Month/10));
			MAX7219_writeData(MAX7219_DIGIT1,MAX7219_CHAR_BLANK);
			MAX7219_writeData(MAX7219_DIGIT0,MAX7219_CHAR_BLANK);
		}
		if ((count==2)&&(SW_time_date==0))	//blink month
		{
			MAX7219_clearDisplay();
			
			MAX7219_writeData(MAX7219_DIGIT7,(Minute%10));
			MAX7219_writeData(MAX7219_DIGIT6,(Minute/10));
			MAX7219_writeData(MAX7219_DIGIT5,(Hour%10));
			MAX7219_writeData(MAX7219_DIGIT4,(Hour/10));
			MAX7219_writeData(MAX7219_DIGIT3,MAX7219_CHAR_BLANK);
			MAX7219_writeData(MAX7219_DIGIT2,MAX7219_CHAR_BLANK);
			MAX7219_writeData(MAX7219_DIGIT1,(Date%10));
			MAX7219_writeData(MAX7219_DIGIT0,(Date/10));
		}
		if ((count==3)&&(SW_time_date==0))	//blink hour
		{
			MAX7219_clearDisplay();
			
			MAX7219_writeData(MAX7219_DIGIT7,(Minute%10));
			MAX7219_writeData(MAX7219_DIGIT6,(Minute/10));
			MAX7219_writeData(MAX7219_DIGIT5,MAX7219_CHAR_BLANK);
			MAX7219_writeData(MAX7219_DIGIT4,MAX7219_CHAR_BLANK);
			MAX7219_writeData(MAX7219_DIGIT3,(Month%10));
			MAX7219_writeData(MAX7219_DIGIT2,(Month/10));
			MAX7219_writeData(MAX7219_DIGIT1,(Date%10));
			MAX7219_writeData(MAX7219_DIGIT0,(Date/10));
		}
		if ((count==4)&&(SW_time_date==0))	//blink min
		{
			MAX7219_clearDisplay();
			
			MAX7219_writeData(MAX7219_DIGIT7,MAX7219_CHAR_BLANK);
			MAX7219_writeData(MAX7219_DIGIT6,MAX7219_CHAR_BLANK);
			MAX7219_writeData(MAX7219_DIGIT5,(Hour%10));
			MAX7219_writeData(MAX7219_DIGIT4,(Hour/10));
			MAX7219_writeData(MAX7219_DIGIT3,(Month%10));
			MAX7219_writeData(MAX7219_DIGIT2,(Month/10));
			MAX7219_writeData(MAX7219_DIGIT1,(Date%10));
			MAX7219_writeData(MAX7219_DIGIT0,(Date/10));
		}
		if ((count==1)&&(SW_time_date==1))	//blink date
		{
			MAX7219_clearDisplay();
			
			MAX7219_writeData(MAX7219_DIGIT7,((yyyy%1000)%10));
			MAX7219_writeData(MAX7219_DIGIT6,(((yyyy%1000)/10)%10));
			MAX7219_writeData(MAX7219_DIGIT5,(((yyyy%1000)/100)%10));
			MAX7219_writeData(MAX7219_DIGIT4,(yyyy/1000));
			MAX7219_writeData(MAX7219_DIGIT3,(Month%10));
			MAX7219_writeData(MAX7219_DIGIT2,(Month/10));
			MAX7219_writeData(MAX7219_DIGIT1,MAX7219_CHAR_BLANK);
			MAX7219_writeData(MAX7219_DIGIT0,MAX7219_CHAR_BLANK);
		}
		if ((count==2)&&(SW_time_date==1))	//blink month
		{
			MAX7219_clearDisplay();
			
			MAX7219_writeData(MAX7219_DIGIT7,((yyyy%1000)%10));
			MAX7219_writeData(MAX7219_DIGIT6,(((yyyy%1000)/10)%10));
			MAX7219_writeData(MAX7219_DIGIT5,(((yyyy%1000)/100)%10));
			MAX7219_writeData(MAX7219_DIGIT4,(yyyy/1000));
			MAX7219_writeData(MAX7219_DIGIT3,MAX7219_CHAR_BLANK);
			MAX7219_writeData(MAX7219_DIGIT2,MAX7219_CHAR_BLANK);
			MAX7219_writeData(MAX7219_DIGIT1,(Date%10));
			MAX7219_writeData(MAX7219_DIGIT0,(Date/10));
		}
		if ((count==3)&&(SW_time_date==1))	//blink year
		{
			MAX7219_clearDisplay();
			
			MAX7219_writeData(MAX7219_DIGIT7,MAX7219_CHAR_BLANK);
			MAX7219_writeData(MAX7219_DIGIT6,MAX7219_CHAR_BLANK);
			MAX7219_writeData(MAX7219_DIGIT5,MAX7219_CHAR_BLANK);
			MAX7219_writeData(MAX7219_DIGIT4,MAX7219_CHAR_BLANK);
			MAX7219_writeData(MAX7219_DIGIT3,(Month%10));
			MAX7219_writeData(MAX7219_DIGIT2,(Month/10));
			MAX7219_writeData(MAX7219_DIGIT1,(Date%10));
			MAX7219_writeData(MAX7219_DIGIT0,(Date/10));
		}
		if ((count==1)&&(SW_time_date==3))	//blink A_HOUR
		{
			MAX7219_clearDisplay();
			
			MAX7219_writeData(MAX7219_DIGIT7,(A_Minute%10));
			MAX7219_writeData(MAX7219_DIGIT6,(A_Minute/10));
			MAX7219_writeData(MAX7219_DIGIT5,MAX7219_CHAR_BLANK);
			MAX7219_writeData(MAX7219_DIGIT4,MAX7219_CHAR_BLANK);
			MAX7219_writeData(MAX7219_DIGIT3,(Minute%10));
			MAX7219_writeData(MAX7219_DIGIT2,(Minute/10));
			MAX7219_writeData(MAX7219_DIGIT1,(Hour%10));
			MAX7219_writeData(MAX7219_DIGIT0,(Hour/10));

		}
		if ((count==2)&&(SW_time_date==3))	//blink A_MIN
		{
			MAX7219_clearDisplay();
			
			MAX7219_writeData(MAX7219_DIGIT7,MAX7219_CHAR_BLANK);
			MAX7219_writeData(MAX7219_DIGIT6,MAX7219_CHAR_BLANK);
			MAX7219_writeData(MAX7219_DIGIT5,(A_Hour%10));
			MAX7219_writeData(MAX7219_DIGIT4,(A_Hour/10));
			MAX7219_writeData(MAX7219_DIGIT3,(Minute%10));
			MAX7219_writeData(MAX7219_DIGIT2,(Minute/10));
			MAX7219_writeData(MAX7219_DIGIT1,(Hour%10));
			MAX7219_writeData(MAX7219_DIGIT0,(Hour/10));
		}
	}
	

}

//-----------------------DOI DUONG LICH - AM LICH---------------------

double jdFromDate(uint8_t dd, uint8_t mm, uint16_t yy)
{
	long double a, y, m, jd;
	a = floorf((14 - mm) / 12);
	y = yy+4800-a;
	m = mm+12*a-3;
	jd = dd + floorf((153*m+2)/5) + 365*y + floorf(y/4) - floorf(y/100) + floorf(y/400) - 32045;
	if (jd < 2299161) {
		jd = dd + floorf((153*m+2)/5) + 365*y + floorf(y/4) - 32083;
	}
	return jd;
}

double getNewMoonDay(long double k)
{
	long double T, T2, T3, dr, Jd1, M, Mpr, F, C1, deltat, JdNew;
	T = k/1236.85; // Time in Julian centuries from 1900 January 0.5
	T2 = T * T;
	T3 = T2 * T;
	dr = M_PI/180;
	Jd1 = 2415020.75933 + 29.53058868*k + 0.0001178*T2 - 0.000000155*T3;
	Jd1 = Jd1 + 0.00033*sinf((166.56 + 132.87*T - 0.009173*T2)*dr); // Mean new moon
	M = 359.2242 + 29.10535608*k - 0.0000333*T2 - 0.00000347*T3; // Sun's mean anomaly
	Mpr = 306.0253 + 385.81691806*k + 0.0107306*T2 + 0.00001236*T3; // Moon's mean anomaly
	F = 21.2964 + 390.67050646*k - 0.0016528*T2 - 0.00000239*T3; // Moon's argument of latitude
	C1=(0.1734 - 0.000393*T)*sinf(M*dr) + 0.0021*sinf(2*dr*M);
	C1 = C1 - 0.4068*sinf(Mpr*dr) + 0.0161*sinf(dr*2*Mpr);
	C1 = C1 - 0.0004*sinf(dr*3*Mpr);
	C1 = C1 + 0.0104*sinf(dr*2*F) - 0.0051*sinf(dr*(M+Mpr));
	C1 = C1 - 0.0074*sinf(dr*(M-Mpr)) + 0.0004*sinf(dr*(2*F+M));
	C1 = C1 - 0.0004*sinf(dr*(2*F-M)) - 0.0006*sinf(dr*(2*F+Mpr));
	C1 = C1 + 0.0010*sinf(dr*(2*F-Mpr)) + 0.0005*sinf(dr*(2*Mpr+M));
	if (T < -11) {
		deltat= 0.001 + 0.000839*T + 0.0002261*T2 - 0.00000845*T3 - 0.000000081*T*T3;
		} else {
		deltat= -0.000278 + 0.000265*T + 0.000262*T2;
	};
	JdNew = Jd1 + C1 - deltat;
	return floorf(JdNew + 0.5 + timeZone/24);
}

double getSunLongitude(long double jdn)
{
	long double T, T2, dr, M, L0, DL, L;
	T = (jdn - 2451545.5 - timeZone/24) / 36525; // Time in Julian centuries from 2000-01-01 12:00:00 GMT
	T2 = T*T;
	dr = M_PI/180; // degree to radian
	M = 357.52910 + 35999.05030*T - 0.0001559*T2 - 0.00000048*T*T2; // mean anomaly, degree
	L0 = 280.46645 + 36000.76983*T + 0.0003032*T2; // mean longitude, degree
	DL = (1.914600 - 0.004817*T - 0.000014*T2)*sinf(dr*M);
	DL = DL + (0.019993 - 0.000101*T)*sinf(dr*2*M) + 0.000290*sinf(dr*3*M);
	L = L0 + DL; // true longitude, degree
	L = L*dr;
	L = L - M_PI*2*(floorf(L/(M_PI*2))); // Normalize to (0, 2*PI)
	return floorf(L / M_PI * 6);
}

double getLunarMonth11(uint16_t yy)
{
	long double k, off, nm, sunLong;
	off = jdFromDate(31, 12, yy) - 2415021;
	k = floorf(off / 29.530588853);
	nm = getNewMoonDay(k);
	sunLong = getSunLongitude(nm); // sun longitude at local midnight
	if (sunLong >= 9) {
		nm = getNewMoonDay(k-1);
	}
	return nm;
}

double getLeapMonthOffset(long double a11)
{
	long double k, last, arc, i;
	k = floorf((a11 - 2415021.076998695) / 29.530588853 + 0.5);
	last = 0;
	i = 1; // We start with the month following lunar month 11
	arc = getSunLongitude(getNewMoonDay(k+i));
	do {
		last = arc;
		i++;
		arc = getSunLongitude(getNewMoonDay(k+i));
	} while (arc != last && i < 14);
	return i-1;
}

double convertSolar2Lunar(uint8_t dd, uint8_t mm, uint16_t yy)
{
	long double k, dayNumber, monthStart, a11, b11, diff, leapMonthDiff;
	dayNumber = jdFromDate(dd, mm, yy);
	k = floorf((dayNumber - 2415021.076998695) / 29.530588853);
	monthStart = getNewMoonDay(k+1);
	if (monthStart > dayNumber) {
		monthStart = getNewMoonDay(k);
	}
	a11 = getLunarMonth11(yy);
	b11 = a11;
	if (a11 >= monthStart) {
		lunarYear = yy;
		a11 = getLunarMonth11(yy-1);
		} else {
		lunarYear = yy+1;
		b11 = getLunarMonth11(yy+1);
	}
	lunarDate = dayNumber-monthStart+1;
	diff = floorf((monthStart - a11)/29);
	lunarMonth = diff+11;
	if (b11 - a11 > 365) {
		leapMonthDiff = getLeapMonthOffset(a11);
		if (diff >= leapMonthDiff) {
			lunarMonth = diff + 10;
		}
	}
	if (lunarMonth > 12) {
		lunarMonth = lunarMonth - 12;
	}
	if (lunarMonth >= 11 && diff < 4) {
		lunarYear -= 1;
	}
	return 0;
}
//--------------------------------------------------------------------

void Init_Timer0(void)
{
	//Timer 0 is 8bit-timer register
	//Initialize Timer0 to 1s - overflow interrupt--------------------
    TCCR0=(1<<CS02)|(0<<CS01)|(1<<CS00);	//prescaler, clk/1024
    TIMSK=(1<<TOIE0);						
    sei();                      			
}


void Init_buttons(void)
{
	BTN_DDRD	&= ~((1<<SW)|(1<<ADJ));		
	BTN_DDRB	&= ~(1<<INCR);
}

void Init_IO(void)
{
	DDR_LED_O		|=(1<<BIT_LED_O);
	DDR_BUZZER_O	|= (1<<BIT_BUZZER_O);
	//PORT_BUZZER_O	|= (1<<BIT_BUZZER_O);
}

void Init_interupt(void)
{
	MCUCR	= (1<<ISC11)|(1<<ISC10)|(1<<ISC01)|(1<<ISC00);
	MCUCSR	= (1<<ISC2);
	GICR	= (1<<INT2)|(1<<INT1)|(1<<INT0);
	sei();
}

void Init_start_cond()
{
	PORT_LED_O |= (1<<BIT_LED_O);
	_delay_ms(500);
	PORT_LED_O &= ~(1<<BIT_LED_O);
	_delay_ms(500);
	PORT_LED_O |= (1<<BIT_LED_O);
	_delay_ms(500);
	PORT_LED_O &= ~(1<<BIT_LED_O);
	_delay_ms(500);
	PORT_LED_O |= (1<<BIT_LED_O);
	_delay_ms(500);
	PORT_LED_O &= ~(1<<BIT_LED_O);
	_delay_ms(500);
}

void set_Alarm(void)
{
	PORT_BUZZER_O &= ~(1<<BIT_BUZZER_O);
	_delay_ms(70);
	PORT_BUZZER_O |= (1<<BIT_BUZZER_O);
	_delay_ms(50);
	PORT_BUZZER_O &= ~(1<<BIT_BUZZER_O);
	_delay_ms(70);
	PORT_BUZZER_O |= (1<<BIT_BUZZER_O);
	_delay_ms(1000);
}

//Main program
int main(void)
{	
	//MAX7219 init
	// SCK MOSI CS/LOAD/SS
	DDRB |= (1 << PIN_SCK) | (1 << PIN_MOSI) | (1 << PIN_SS);
	// SPI Enable, Master mode
	SPCR |= (1 << SPE) | (1 << MSTR)| (1<<SPR1);
	// Scan limit runs from 0.
	MAX7219_writeData(MAX7219_MODE_SCAN_LIMIT, 0x07);
	MAX7219_writeData(MAX7219_MODE_INTENSITY, 0x0F);
	MAX7219_writeData(MAX7219_MODE_POWER, ON);
	MAX7219_writeData(MAX7219_MODE_DECODE, 0xFF);
	//FixTime();
	Init_buttons();
	Init_IO();
	Init_Timer0();
	Init_interupt();
	sei();
	TWI_Init(); 	
	TWI_DS1307_rblock(tData,7); 
	Decode(); 					//BCD data converter function from DS1307 to DEC
	_delay_ms(1);	
	Init_start_cond();			/**BLINK 3 SECOND BEFORE LOOPING**/

	while(1)
	{
		yyyy=Year+2000;
		convertSolar2Lunar(Date, Month, yyyy);	
		Display_7seg();
		if (Hour == A_Hour && Minute == A_Minute && EN_alarm == true)
		{	
			Display_7seg();
			set_Alarm();
		}
	}
	return 0;
}

ISR(TIMER0_OVF_vect){ 	
	Time_count++;			//thoi gian doc ds1307
	blink_count++;			//thoi gian blink
	if(Time_count>=10){ 	//1s Exactly
		                
		if(set == false )
		{
			//Read DS1307
			TWI_DS1307_wadr(0x00); 				
			_delay_ms(1);		   				
			TWI_DS1307_rblock(tData,7); 
					
			//Print result on 7Seg led		
			if(BCDToDec(tData[0]) !=Second)
			{ 
				Decode();			
				Display_7seg();
			} 
		}
		Time_count=0; 
	}
	if (blink_count>=15)	//blink 500ms
	{
		if(set == true ){
				blinkmode^=1;
				Display_7seg();
		}
		blink_count=0;
	}
}


//SW mode button
ISR(INT0_vect){
	
	if(set==false){
		SW_time_date++;
		if(SW_time_date > 3){
			SW_time_date = 0;
		}
	}
	if(SW_time_date==0 && set==true) {
		SW_time_date = 0;
		count=0;
		blinkmode=0;
		FixTime();
		set=false;
	}
	if(SW_time_date==1 && set==true) {
		SW_time_date = 1;
		count=0;
		blinkmode=0;
		FixTime();
		set=false;
	}
	if(SW_time_date==3 && set==true) {
		SW_time_date = 3;
		count=0;
		blinkmode=0;
		EN_alarm=true;
		set=false;
	}
}


//Set time button
ISR(INT1_vect){

	if (SW_time_date==0)
	{
		set = true;
		count++;
		if(count > 4) {
			count = 0;
			blinkmode=0;
			set=false;
		}
	}
	if (SW_time_date==1)
	{
		set = true;
		count++;
		if(count > 3) {
			count = 0;
			blinkmode=0;
			set=false;
		}
	}
	if (SW_time_date==3)
	{
		set = true;
		count++;
		if(count > 2) {
			count = 0;
			blinkmode=0;
			EN_alarm=false;
			set=false;
		}
	}
}

//increase button
ISR(INT2_vect){
	if (EN_alarm == true && set==false)
	{
		EN_alarm=false;
		BTN_PORTD |= (1<<BIT_BUZZER_O);
	}
	if((set == true) && (SW_time_date==0)){		//increase dd, mm, h, min
		if(count == 1) {
			Date++;
			if(Month == 4 || Month == 6  || Month == 9  || Month == 11)
			{
				if(Date > 30)
				Date=1;
			}
			else if(Month == 1 || Month == 3  || Month == 5  || Month == 7 || Month == 8  || Month == 10  || Month == 12)
			{
				if(Date >31)
				Date=1;
			}
			
			else if(yyyy/4 == 0 && yyyy/400 == 0)
			{
				if(Date > 29)
				Date=1;
			}
			else
			{
				if(Date > 28)
				Date=1;
			}
		}
		if(count == 2) 
		{
			Month++;
			if(Month > 12) Month = 1;
		}
		if(count == 3) 
		{
			Hour++;
			if(Hour > 23) Hour = 0;
		}
		if(count == 4) 
		{
			Minute++;
			if(Minute > 59) Minute = 0;
		}
	}
	
	if((set == true) && (SW_time_date==1))			//increase dd, mm, yyyy
	{		
		if(count == 1) 
		{
			Date++;
			if(Month == 4 || Month == 6  || Month == 9  || Month == 11)
			{
				if(Date > 30)
				Date=1;
			}
			else if(Month == 1 || Month == 3  || Month == 5  || Month == 7 || Month == 8  || Month == 10  || Month == 12)
			{
				if(Date >31)
				Date=1;
			}
			
			else if(yyyy/4 == 0 && yyyy/400 == 0)
			{
				if(Date > 29)
				Date=1;
			}
			else
			{
				if(Date > 28)
				Date=1;
			}
		}
		if(count == 2) 
		{
			Month++;
			if(Month > 12) Month = 1;
		}
		if(count == 3) 
		{
			Year++;
			if(Year > 99) Year = 0;
		}
	}
	
	if((set == true) && (SW_time_date==3))			//increase alarm
	{		

		if(count == 1) {
			A_Hour++;
			if(A_Hour > 23) A_Hour = 0;
		}
		if(count == 2) {
			A_Minute++;
			if(A_Minute > 59) A_Minute = 0;
		}
	}
}