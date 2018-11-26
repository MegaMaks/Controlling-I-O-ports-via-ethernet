#include "main.h"
#include <avr/eeprom.h>
//--------------------------------------------------


////check init micro, get id device
uint8_t EEMEM IDNB;
uint8_t EEMEM setPORTC;


unsigned char eepromclear=1;

void port_init(void)
{
	//DDRD PD2 in(int0) PD4,PD5,PD6 out for LED
	//DDRD &= ~(0b00000100);
	DDRD=0x70;
	DDRC=0xFF;
	DDRA=0x00;
	PORTA=0xFF;
}
void interrupt_init(void)
{
	EICRA |= (1<<ISC01);
	EIMSK |= (1<<INT0);
	PCMSK0=0xff;
	PCICR|= (1<<PCIE0);
	PCIFR|= (1<<PCIF0);
}

void lamp(unsigned char cnt)
{
	unsigned char i;	
	for(i=0;i<cnt;i++)
	{
		PORTD |= (1 << 6);
		_delay_ms(50);
		PORTD &=~ (1 << 6);
		_delay_ms(50);
		PORTD |= (1 << 5);
		_delay_ms(50);
		PORTD &=~ (1 << 5);
		_delay_ms(50);
		PORTD |= (1 << 4);
		_delay_ms(50);
		PORTD &=~ (1 << 4);
		_delay_ms(50);
	}
}


//--------------------------------------------------
ISR(INT0_vect)
{
	net_pool();
}

ISR(PCINT0_vect)
{ 
	smart_interrupt();
}
//--------------------------------------------------
int main(void)
{
	init_timer();
	port_init();
	interrupt_init();
	USART_Init (16); //115200
	net_ini();
	sei();
	while (1)
	{
	if(eepromclear==1)
	{
		lamp(3);
		eepromclear=0;

		if(eeprom_read_byte(&IDNB)==0xff)
		{
			USART_TX((uint8_t*)"I am new block for Smart House\r\n",33);
			eeprom_write_byte (&IDNB, 0x00);
			eeprom_write_byte (&setPORTC, 0x00);

		}
		else
		{
			PORTC=eeprom_read_byte(&setPORTC);
			USART_TX((uint8_t*)"I am turned on\r\n",17);
		}
	}
	else
	{
		PORTD |= (1 << 6);
	}
	
	}
}
//--------------------------------------------------
