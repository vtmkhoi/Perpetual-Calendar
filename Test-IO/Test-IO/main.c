/*
 * Test-IO.c
 *
 * Created: 1/17/2022 7:50:23 PM
 * Author : PC
 */ 

#define F_CPU		8000000UL

#include <avr/io.h>
#include <util/delay.h>

// Output Port pin LED_O
#define PORT_LED_O      PORTA
#define DDR_LED_O       DDRA
#define BIT_LED_O       0

#define PORT_BUZZER_O PORTD
#define DDR_BUZZER_O DDRD
#define BIT_BUZZER_O 7

void Init_btn(void)
{
	DDR_LED_O |= (1<<BIT_LED_O);
	DDR_BUZZER_O |= (1<<BIT_BUZZER_O);
	PORT_BUZZER_O |= (1<<BIT_BUZZER_O);
}
int main(void)
{
	Init_btn();
    /* Replace with your application code */
    while (1) 
    {
		PORT_BUZZER_O &= ~(1<<BIT_BUZZER_O);
		_delay_ms(70);
		PORT_BUZZER_O |= (1<<BIT_BUZZER_O);
		_delay_ms(50);
		PORT_BUZZER_O &= ~(1<<BIT_BUZZER_O);
		_delay_ms(70);
		PORT_BUZZER_O |= (1<<BIT_BUZZER_O);
		_delay_ms(1000);
;
    }
}

