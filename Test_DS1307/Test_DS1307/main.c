/*
 * Calendar_ver02.c
 *
 * Created: 9/13/2021 3:36:35 PM
 * Author : VTMKhoi
 * Component: Using KIT Atmega12 ThienMinh interfaced with module RTC DS1307
 */ 

#define F_CPU 7372800UL

#include <avr/io.h>
#include <myDS1307RTC.h>
#include <avr/myLCD.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <stdio.h>

volatile uint8_t	Second=55, Minute=33, Hour=16, Day=1, Date=13, Month=9, Year=21, Mode=1, AP=1;
volatile uint8_t	tData[7], Time_count = 0;
char dis[5];

/*
??i BCD sang th?p phân
*/
uint8_t BCD2Dec(uint8_t BCD)
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
/*
Display on LCD
*/
void Display(void){ //chuong trinh con  hien thi thoi gian doc tu DS1307
	Second 	= BCD2Dec(tData[0] & 0x7F);
	Minute 	= BCD2Dec(tData[1]);
	
	if (Mode !=0) 	Hour = BCD2Dec(tData[2] & 0x1F); //mode 12h
	else 		  	Hour = BCD2Dec(tData[2] & 0x3F); //mode 24h
	
	Date   	= BCD2Dec(tData[4]);
	Month	= BCD2Dec(tData[5]);
	Year	= BCD2Dec(tData[6]);
	
	clr_LCD();		//xoa LCD
	//in gio:phut:giay
	print_LCD("Time: ");
	sprintf(dis, "%i",Hour);
	move_LCD(1,7);  print_LCD(dis); move_LCD(1,9); putChar_LCD(':');
	sprintf(dis, "%i",Minute);
	move_LCD(1,10); print_LCD(dis); move_LCD(1,12);putChar_LCD(':');
	sprintf(dis, "%i",Second);
	move_LCD(1,13); print_LCD(dis);
	if (Mode !=0){ //mode 12h
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

int main(void){	
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
	
	while(1){
		
	}
	return 0;
}

ISR (TIMER0_OVF_vect){           
	Time_count++;
	if(Time_count>=10){ 
		//doc DS1307
		TWI_DS1307_wadr(0x00); 				//set dia chi ve 0
		_delay_ms(1);		   				//cho DS1307 xu li 
		TWI_DS1307_rblock(tData,7); 	//doc ca khoi thoi gian (7 bytes)		
		//hien thi ket qua len LCD
		if(BCD2Dec(tData[0]) !=Second){ 	//chi hien thi ket qua khi da qua 1s
			Second=BCD2Dec(tData[0] & 0x7F);
			sprintf(dis, "%i",Second); 
			move_LCD(1,13); print_LCD("  ");
			move_LCD(1,13); print_LCD(dis);
			if (Second==0) Display(); 		//moi phut cap nhat 1 lan			
		}
		Time_count=0; 
	}
}

