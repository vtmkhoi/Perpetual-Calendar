#include <avr/io.h>

#ifndef cbi
#define cbi(port, bit)		(port) &= ~(1 << (bit))		//clear bit
#endif

#ifndef sbi
#define sbi(port, bit)		(port) |= (1 << (bit))		//set bit
#endif


#define TWI_DDR		DDRC
#define TWI_PORT	PORTC
#define TWI_PIN 	PINC

#define SCL_BIT		0
#define SDA_BIT		1


#define DS1307_SLA	0x68		// dia chi I2C Slave mac d?nh cua DS1307:0x68=1101000

// dinh nghia gia tri cho thanh toc do TWBR ung voi tan so xung giu nhip 8MHz
// #define _222K		10
// #define _100K		32
#define _400K		12
#define _100K		72

#define TWI_W		0	// bit Write
#define TWI_R		1	// bit Read

#define	TWI_START	(1 << TWINT)|(1 << TWSTA)|(1 << TWEN)	//goi Start condidition
#define TWI_STOP	(1 << TWINT)|(1 << TWSTO)|(1 << TWEN)	//goi STOP condition

#define TWI_Clear_TWINT	(1 << TWINT)|(1 << TWEN)	//xoa TWIN de bat dau doc, doc xong thi NOT ACK
#define TWI_Read_ACK	(1 << TWINT)|(1 << TWEN)|(1 << TWEA)	//xoa TWIN de bat dau doc, sau khi doc set ACK

//khoi tao TWI
void TWI_Init(void){
	TWSR = 0x00;	//Prescaler=1 chia t?n cho 1
	TWBR = _100K;
	TWCR = (1 << TWINT) | (1 << TWEN);
}

//chon dia chi thanh ghi can thao tac, dummy write
//Addr: dia thi thanh ghi can ghi
uint8_t TWI_DS1307_wadr(uint8_t Addr){
	TWCR = TWI_START;						//goi START condition
	while((TWCR & 0x80) == 0x00);			//cho TWINT bit=1
	if((TWSR & 0xF8) != 0x08) return TWSR;	//neu goi Start co loi thi thoat

	TWDR = (DS1307_SLA << 1) + TWI_W;		//dia chi DS va bit W
	TWCR = TWI_Clear_TWINT;					//xoa TWINT, bat dau goi SLA
	while((TWCR & 0x80) == 0x00);			//cho TWINT bit=1
	if((TWSR & 0xF8) != 0x18) return TWSR;	//neu co loi truyen SLA, thoat

	TWDR = Addr;							//goi dia chi thanh ghi can ghi vao
	TWCR = TWI_Clear_TWINT;					//xoa TWINT de bat dau goi
	while((TWCR & 0x80) == 0x00);			//check and wait for TWINT bit=1
	if((TWSR & 0xF8) != 0x28) return TWSR;	//neu du lieu goi ko thanh cong thi thoat
	
	TWCR = TWI_STOP;						//STOP condition
	return 0;
}

//ghi 1 mang dat vao DS1307
//Addr: dia thi thanh ghi can ghi
//Data[]: mang du lieu
//len: so luong byte can ghi
uint8_t TWI_DS1307_wblock(uint8_t Addr, uint8_t Data[], uint8_t len){
	
	TWCR = TWI_START;
	while((TWCR & 0x80) == 0x00);
	if((TWSR & 0xF8) != 0x08) return TWSR;
	
	TWDR = (DS1307_SLA << 1) + TWI_W;
	TWCR = TWI_Clear_TWINT;
	while((TWCR & 0x80) == 0x00);
	if((TWSR & 0xF8) != 0x18) return TWSR;
	
	TWDR = Addr;
	TWCR = TWI_Clear_TWINT;
	while((TWCR & 0x80) == 0x00);
	if((TWSR & 0xF8) != 0x28) return TWSR;

	for (uint8_t i = 0; i < len; i++){
		TWDR = Data[i];							//chuan bi xuat du lieu
		TWCR = TWI_Clear_TWINT;					//xoa TWINT, bat dau send
		while((TWCR & 0x80) == 0x00);			//cho TWINT bit=1
		if((TWSR & 0xF8) != 0x28) return TWSR;	//neu status ko phai la 0x28 thi return
	}
	
	TWCR = TWI_STOP;							//STOP condition
	return 0;
}

//doc 1 mang tu DS1307
uint8_t TWI_DS1307_rblock(uint8_t Data[], uint8_t len){
	uint8_t i;
	
	
	TWCR = TWI_START;
	while (((TWCR & 0x80) == 0x00) || ((TWSR & 0xF8) != 0x08));

	TWDR = (DS1307_SLA << 1) + TWI_R;
	TWCR = TWI_Clear_TWINT;
	while (((TWCR & 0x80) == 0x00) || ((TWSR & 0xF8) != 0x40));
	
	for (i = 0; i < len-1; i++){
		TWCR = TWI_Read_ACK;
		while (((TWCR & 0x80) == 0x00) || ((TWSR & 0xF8) != 0x50));
		Data[i] = TWDR;
	}
	
	TWCR = TWI_Clear_TWINT;
	while (((TWCR & 0x80) == 0x00) || ((TWSR & 0xF8) != 0x58));
	Data[len-1] = TWDR;
	
	TWCR = TWI_STOP;
	return 0;
}
