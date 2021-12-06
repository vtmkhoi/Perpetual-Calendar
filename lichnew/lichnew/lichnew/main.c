/*
 * lichnew.c
 *
 * Created: 11/29/2021 11:17:27 AM
 * Author : DELL
 */ 


#define F_CPU		8000000UL
#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <stdio.h>
#include "myDS1307RTC.h"
#include <avr/myLCD.h>
#include <stdbool.h>
#include <math.h>
//-- dinh nghia Button
#define BTN_DDR		DDRD
#define BTN_PORT	PORTD
#define BTN_PIN		PIND
#define	incr		0
#define decr		1
#define mode		2
#define save		3
#define exit		4
#define alarm		5
#define SW			6
#define BUZ_LED		7


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

#define MAX7219_CHAR_BLANK        0xF
#define MAX7219_CHAR_NEGATIVE     0xA




//--------------------------------------------------------------------------------------

// dinh nghia cac bien thoi gian
/*signed char Second = 59, Minute = 50, Hour = 11;*/
volatile int16_t Second = 50, Minute = 59, Hour = 23, Day = 2, 
Date = 6, Month = 12, Year = 21, Mode = 0, AP = 1, A_Hour = 0, A_Minute = 0, 
timeZone = 7, lunarDate, lunarMonth, lunarYear, yyyy;

//Su dung bien dong (volatile) de tro thanh bien tuy chon
//Mode: chon che do 12h hoac 24h, Mode nam o bit 6 cua thanh ghi HOURS
//Mode = 1: 12h, = 0 24h
//AP:bien chi AM hay PM trong mode 12h, AP nam o bit thu 5 c?a thanh ghi HOURS
// AP=1:PM, AP=0:AM

unsigned char font[10]={0xC0,0xF9,0xA4,0xB0,0x99,0x92,0x82,0xF8,0x80,0x90};		//ma hien thi led 7seg tu 0 -> 9
volatile uint8_t tData[7];	//tData[7]: mang du lieu tam thoi
volatile uint16_t Time_count = 0;
char dis[5];	//dung cho hàm printf de chuyen doi du lieu sang string va hien thi len LCD
bool set = false;		//set = true: cho phep dieu chinh thoi gian
bool set_alarm = false;
bool EN_alarm = false;
volatile uint8_t count = 0;
volatile char SW_time_date = 0;

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
		MAX7219_clearDisplay();
		
		MAX7219_writeData(MAX7219_DIGIT7,MAX7219_CHAR_BLANK);
		MAX7219_writeData(MAX7219_DIGIT6,MAX7219_CHAR_BLANK);
		MAX7219_writeData(MAX7219_DIGIT5,(Second%10));
		MAX7219_writeData(MAX7219_DIGIT4,(Second/10));
		MAX7219_writeData(MAX7219_DIGIT3,(Minute%10));
		MAX7219_writeData(MAX7219_DIGIT2,(Minute/10));
		MAX7219_writeData(MAX7219_DIGIT1,(Hour%10));
		MAX7219_writeData(MAX7219_DIGIT0,(Hour/10));
		
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
		
		MAX7219_writeData(MAX7219_DIGIT3,(A_Minute%10));
		MAX7219_writeData(MAX7219_DIGIT2,(A_Minute/10));
		MAX7219_writeData(MAX7219_DIGIT1,(A_Hour%10));
		MAX7219_writeData(MAX7219_DIGIT0,(A_Hour/10));
	}

}
//Write to DS1307 time that want to change
void FixTime(){ 
	tData[0] = DecToBCD(Second); 
	tData[1] = DecToBCD(Minute); 
	if (Mode != 0) tData[2] = DecToBCD(Hour)|(Mode << 6)|(AP << 5); //mode 12h
	else tData[2] = DecToBCD(Hour);
	tData[3] = DecToBCD(Day);
	tData[4] = DecToBCD(Date);
	tData[5] = DecToBCD(Month); 
	tData[6] = DecToBCD(Year); 
	TWI_DS1307_wblock(0x00, tData, 7); 
	_delay_ms(1);
	TWI_DS1307_wadr(0x00); 
	_delay_ms(1);			
 }

void Init_btn(void){

    //------------------Initialize button------------
	BTN_DDR  &= ~((1<<mode)|(1<<incr)|(1<<decr)|(1<<save)|(1<<exit)|(1<<alarm)|(1<<SW));		// set input cho button setting
	BTN_DDR |= (1<<BUZ_LED);																// set output cho buzzer + led
	BTN_PORT |= (1 << mode)|(1 << incr)|(1 << decr)|(1 << save)|(1 << exit)|(1<<alarm)|(1<<SW);	// set dien tro keo len cho button setting

	//------------------Initialize data and ctrl port------------
// 	DDR_7SEG |= 0x7F;		// 0x7F = 0b01111111 // set output for 7seg data pin
// 	CTRL_DDR |= (1 << sec_year0)|(1 << sec_year1)|(1 << min_month0)|(1 << min_month1)|(1 << h_date0)|(1<< h_date1);		// set output for 7seg ctrl pin
}
void Check_btn(void){
	
	/***********Switch between time and date************/

	// choose displaying time mode or date mode
	//	Default: SW_time_date = false -> display time on 7seg led
	if ((BTN_PIN & (1 << SW)) == 0){
		while((BTN_PIN & (1 << SW)) == 0){
			//while den khi tha nut nhan ra
		}
		SW_time_date++;
		if(SW_time_date > 3) SW_time_date = 0;
	}
// 	if ((BTN_PIN & (1 << SW)) == 0){
// 		while((BTN_PIN & (1 << SW)) == 0){
// 		//while den khi tha nut nhan ra
// 		}
// 		if (SW_time_date == false)
// 		{
// 			SW_time_date = true;
// 		}
// 		else
// 		{
// 			SW_time_date = false;
// 		}
// 	}

	/*****************setting time******************/
  	if((BTN_PIN & (1 << mode)) == 0){ 	//setting time
    	while((BTN_PIN & (1 << mode)) == 0){
		 //while den khi tha nut nhan ra
		}		
		set = true;
	  	count++;
		if(count > 6) count = 1;
	  	clr_LCD();
		Display();		
		if(count == 1){
			move_LCD(2,14);
			SW_time_date=1;
		}
		else if(count == 2){
			move_LCD(2,11);
			SW_time_date=1;
		}
		else if(count == 3){
			move_LCD(2,8);
			SW_time_date=1;
		}
		else if(count == 4){
			move_LCD(1,8);
			SW_time_date=0;
		}
		else if(count == 5){
			move_LCD(1,11);
			SW_time_date=0;
		}
		else if(count == 6){
			move_LCD(1,14);
			SW_time_date=0;
		}
	}

	if(((BTN_PIN & (1 << incr)) == 0) && set == true){ 
		while(((BTN_PIN & (1 << incr)) == 0)){	
			// while den khi tha nut nhan ra
		}
		if(count == 1) { 
			Year++;
			if(Year > 99) Year = 0; 
		}
		else if(count == 2) { 
			Month++; 
			if(Month > 12) Month = 1; 
		}
		else if(count == 3) { 
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
		else if(count == 4) {
			 Hour++;  
			 if(Hour > 23) Hour = 0; 
		}
		else if(count == 5) { 
			Minute++; 
			if(Minute > 59) Minute = 0;
		}
		else if(count == 6) { 
			Second++; 
			if(Second > 59) Second = 0;
		}
		Display();	
	}
					
	if(((BTN_PIN & (1 << decr)) == 0) && set == true) {
		while((BTN_PIN & (1 << decr))== 0){		
			// while den khi tha nut nhan ra
		}	
		if(count == 1) {
			 Year--;
			 if(Year < 0) Year = 99;  
		}
		else if(count == 2) { 
			Month--; 
			if(Month < 1 )  Month = 12; 
		}
		else if(count == 3) { 
			Date--;  
			if(Month == 4 || Month == 6  || Month == 9  || Month == 11)
			{
				if(Date <1)
				Date=30;
			}
			else if(Month == 1 || Month == 3  || Month == 5  || Month == 7 || Month == 8  || Month == 10  || Month == 12)
			{
				if(Date <1)
				Date=31;
			}
			
			else if(yyyy/4 == 0 && yyyy/400 == 0)
			{
				if(Date <1)
				Date=29;
			}
			else
			{
				if(Date <1)
				Date=28;
			}
		}
		else if(count == 4) {
			 Hour--;  
			 if(Hour < 0 )  Hour = 23 ;
		}
		else if(count == 5) { 
			Minute--; 
			if(Minute < 0 )  Minute = 59; 
		}
		else if(count == 6) { 
			Second--; 
			if(Second < 0 ) Second = 59; 
		}
		Display();				
	}
								 
	if(((BTN_PIN & (1 << save)) == 0) && set == true) {	
		while((BTN_PIN & (1 << save))== 0){		// while den khi tha nut nhan ra
		}
		count = 0; 
		FixTime(); 
// 		clr_LCD(); 
// 		print_LCD("Saving......."); 
// 		_delay_ms(500); 
		Display(); 
		set = false; 
	}
	  
	if(((BTN_PIN & (1 << exit)) == 0) && set == true ) { 
		while((BTN_PIN & (1 << exit))== 0){		// while den khi tha nut nhan ra
		}
		count = 0;    
// 		clr_LCD();  
// 		print_LCD("Exit....."); 
// 		_delay_ms(500); 
		Display(); 
		set = false;	
	}
	/*******************setting alarm******************/
	if((BTN_PIN & (1 << alarm)) == 0){ 	//set bao thuc
		while((BTN_PIN & (1 << alarm)) == 0){ //while den khi tha nut nhan ra

		}
		set_alarm = true;
		SW_time_date=3;
		count++;
		if(count > 2) count = 1;
		clr_LCD();
		Display();
		if(count == 1){
			move_LCD(2,8);
		}
		if(count == 2){
			move_LCD(2,11);
		}
	}

	if(((BTN_PIN & (1 << incr)) == 0) && set_alarm == true){
		while(((BTN_PIN & (1 << incr)) == 0)){		// while den khi tha nut nhan ra
		}
		if(count == 1) {
			A_Hour++;
			if(A_Hour > 23) A_Hour = 0;
		}
		if(count == 2) {
			A_Minute++;
			if(A_Minute > 59) A_Minute = 0;
		}
		Display();
	}
	
	if(((BTN_PIN & (1 << decr)) == 0) && set_alarm == true) {
		while((BTN_PIN & (1 << decr))== 0){		// while den khi tha nut nhan ra
		}
		if(count == 1 ){
			A_Hour--;
			if(A_Hour < 0 )  A_Hour = 23 ;
		}
		if(count == 2) {
			A_Minute--;
			if(A_Minute < 0 )  A_Minute = 59;
		}
		Display();
	}
	
	if(((BTN_PIN & (1 << save)) == 0) && set_alarm == true) {
		while((BTN_PIN & (1 << save))== 0){		// while den khi tha nut nhan ra
		}
		count = 0;
		EN_alarm = true;
// 		clr_LCD();
// 		print_LCD("Saving.......");
// 		_delay_ms(500);
		set_alarm = false;
		Display();
	}
	if(((BTN_PIN & (1 << exit)) == 0) && set_alarm == true ) {
		while((BTN_PIN & (1 << exit))== 0){		// while den khi tha nut nhan ra
		}
		count = 0;
		EN_alarm = false;
// 		clr_LCD();
// 		print_LCD("Exit.....");
// 		_delay_ms(500);
		set_alarm = false;
		Display();
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

double getNewMoonDay(long double k, int timeZone)
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

double getSunLongitude(long double jdn, int timeZone)
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

double getLunarMonth11(uint16_t yy, int timeZone)
{
	long double k, off, nm, sunLong;
	off = jdFromDate(31, 12, yy) - 2415021;
	k = floorf(off / 29.530588853);
	nm = getNewMoonDay(k, timeZone);
	sunLong = getSunLongitude(nm, timeZone); // sun longitude at local midnight
	if (sunLong >= 9) {
		nm = getNewMoonDay(k-1, timeZone);
	}
	return nm;
}

double getLeapMonthOffset(long double a11, int timeZone)
{
	long double k, last, arc, i;
	k = floorf((a11 - 2415021.076998695) / 29.530588853 + 0.5);
	last = 0;
	i = 1; // We start with the month following lunar month 11
	arc = getSunLongitude(getNewMoonDay(k+i, timeZone), timeZone);
	do {
		last = arc;
		i++;
		arc = getSunLongitude(getNewMoonDay(k+i, timeZone), timeZone);
	} while (arc != last && i < 14);
	return i-1;
}

double convertSolar2Lunar(uint8_t dd, uint8_t mm, uint16_t yy, int timeZone)
{
	long double k, dayNumber, monthStart, a11, b11, diff, leapMonthDiff;
	dayNumber = jdFromDate(dd, mm, yy);
	k = floorf((dayNumber - 2415021.076998695) / 29.530588853);
	monthStart = getNewMoonDay(k+1, timeZone);
	if (monthStart > dayNumber) {
		monthStart = getNewMoonDay(k, timeZone);
	}
	a11 = getLunarMonth11(yy, timeZone);
	b11 = a11;
	if (a11 >= monthStart) {
		lunarYear = yy;
		a11 = getLunarMonth11(yy-1, timeZone);
		} else {
		lunarYear = yy+1;
		b11 = getLunarMonth11(yy+1, timeZone);
	}
	lunarDate = dayNumber-monthStart+1;
	diff = floorf((monthStart - a11)/29);
	lunarMonth = diff+11;
	if (b11 - a11 > 365) {
		leapMonthDiff = getLeapMonthOffset(a11, timeZone);
		if (diff >= leapMonthDiff) {
			lunarMonth = diff + 10;
// 			if (diff == leapMonthDiff) {
// 				lunarLeap = 1;
// 			}
		}
	}
	if (lunarMonth > 12) {
		lunarMonth = lunarMonth - 12;
	}
	if (lunarMonth >= 11 && diff < 4) {
		lunarYear -= 1;
	}
}
//--------------------------------------------------------------------
char digitsInUse = 8;

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

void Init_Timer0(void){
	//Initialize Timer0 to 1s - overflow interrupt--------------------
    TCCR0=(1<<CS02)|(0<<CS01)|(1<<CS00);	//prescaler, clk/1024
	
    TIMSK=(1<<TOIE0);						
    sei();                      			
	//----------------------------------------------------------------
}

//Main program
int main(void){	
	
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
	
	FixTime();
	Init_btn();// KEY_PORT=0xF8;

	//Initialize LCD-----------------------
	init_LCD();
	clr_LCD();	
	
	move_LCD(1,1);
	print_LCD("Starting....");
	
	move_LCD(2,1);
	print_LCD(" REAL TIME CLOCK ");
	_delay_ms(1000);
	//------------------------------------
	Init_Timer0();
	
	TWI_Init(); 
			
	TWI_DS1307_rblock(tData,7); 
	
	Decode(); 	//BCD data converter function from DS1307 to DEC
	Display(); 	//Print on LCD
	_delay_ms(1);	
	
	//************************************************************************************
	while(1){
		Check_btn();
		yyyy=Year+2000;
		convertSolar2Lunar(Date, Month, yyyy, timeZone);	
		Display_7seg();
		if (Hour == A_Hour && Minute == A_Minute && EN_alarm == true)
		{	
			Check_btn();
			Display_7seg();
			BTN_PORT |= (1<<BUZ_LED);
		}
	}
	return 0;
}

char data[5];

ISR(TIMER0_OVF_vect){ 	
	Time_count++;
	if(Time_count>=10){ 	//1s Exactly
		                
		if(set == false && set_alarm == false){
			//Read DS1307
			TWI_DS1307_wadr(0x00); 				
			_delay_ms(1);		   				
			TWI_DS1307_rblock(tData,7); 
					
			//Print result on LCD + 7Seg led		
			if(BCDToDec(tData[0]) !=Second){ 
				Decode();			
				Display();
				Display_7seg();
			} 
		}
		Time_count=0; 
	}
	if ((Time_count>5)&&(Time_count<10)&&(count==1))	//blink year
	{
		MAX7219_writeData(MAX7219_MODE_DECODE, 0xFF);
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
	if ((Time_count>5)&&(Time_count<10)&&(count==2))	//blink month
	{
		MAX7219_writeData(MAX7219_MODE_DECODE, 0xFF);
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
	if ((Time_count>5)&&(Time_count<10)&&(count==3))	//blink date
	{
		MAX7219_writeData(MAX7219_MODE_DECODE, 0xFF);
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
	if ((Time_count>5)&&(Time_count<10)&&(count==4))	//blink hour
	{
		MAX7219_writeData(MAX7219_MODE_DECODE, 0xFF);
		MAX7219_clearDisplay();
		
		MAX7219_writeData(MAX7219_DIGIT7,MAX7219_CHAR_BLANK);
		MAX7219_writeData(MAX7219_DIGIT6,MAX7219_CHAR_BLANK);
		MAX7219_writeData(MAX7219_DIGIT5,(Second%10));
		MAX7219_writeData(MAX7219_DIGIT4,(Second/10));
		MAX7219_writeData(MAX7219_DIGIT3,(Minute%10));
		MAX7219_writeData(MAX7219_DIGIT2,(Minute/10));
		MAX7219_writeData(MAX7219_DIGIT1,MAX7219_CHAR_BLANK);
		MAX7219_writeData(MAX7219_DIGIT0,MAX7219_CHAR_BLANK);
	}
	if ((Time_count>5)&&(Time_count<10)&&(count==5))	//blink min
	{
		MAX7219_writeData(MAX7219_MODE_DECODE, 0xFF);
		MAX7219_clearDisplay();
		
		MAX7219_writeData(MAX7219_DIGIT7,MAX7219_CHAR_BLANK);
		MAX7219_writeData(MAX7219_DIGIT6,MAX7219_CHAR_BLANK);
		MAX7219_writeData(MAX7219_DIGIT5,(Second%10));
		MAX7219_writeData(MAX7219_DIGIT4,(Second/10));
		MAX7219_writeData(MAX7219_DIGIT3,MAX7219_CHAR_BLANK);
		MAX7219_writeData(MAX7219_DIGIT2,MAX7219_CHAR_BLANK);
		MAX7219_writeData(MAX7219_DIGIT1,(Hour%10));
		MAX7219_writeData(MAX7219_DIGIT0,(Hour/10));
	}
	if ((Time_count>5)&&(Time_count<10)&&(count==6))	//blink sec
	{
		MAX7219_writeData(MAX7219_MODE_DECODE, 0xFF);
		MAX7219_clearDisplay();
		
		MAX7219_writeData(MAX7219_DIGIT7,MAX7219_CHAR_BLANK);
		MAX7219_writeData(MAX7219_DIGIT6,MAX7219_CHAR_BLANK);
		MAX7219_writeData(MAX7219_DIGIT5,MAX7219_CHAR_BLANK);
		MAX7219_writeData(MAX7219_DIGIT4,MAX7219_CHAR_BLANK);
		MAX7219_writeData(MAX7219_DIGIT3,(Minute%10));
		MAX7219_writeData(MAX7219_DIGIT2,(Minute/10));
		MAX7219_writeData(MAX7219_DIGIT1,(Hour%10));
		MAX7219_writeData(MAX7219_DIGIT0,(Hour/10));
	}
}


