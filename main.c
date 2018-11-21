#include "main.h"
#include <avr/eeprom.h>
//--------------------------------------------------


////check init micro, get id device
uint8_t EEMEM IDNB;
uint8_t EEMEM setPORTC;


unsigned char eepromclear=1;

void port_ini(void)
{
	//������� ����� INT0(PD2) �� ����
	DDRD &= ~(0b00000100);
	DDRC=0xFF;
	DDRA=0x00;
}
void int_ini(void)
{
	//������� ���������� INT0 �� ����������� ������
	EICRA |= (1<<ISC01);
	//�������� ������� ���������� INT0
	EIMSK |= (1<<INT0);
}

void lamp(unsigned char cnt)
{
	unsigned char i;	
	for(i=0;i<cnt;i++)
	{
		PORTD |= (1 << 6);
		_delay_ms(100);
		PORTD &=~ (1 << 6);
		_delay_ms(100);
	}
}
//--------------------------------------------------
ISR(INT0_vect)
{
	net_pool();
}
//--------------------------------------------------
int main(void)
{
	init_timer();
	port_ini();
	int_ini();
	USART_Init (16); //115200
	net_ini();
	sei();
	while (1)
	{
	if(eepromclear==1)
		{
			lamp(5);
			eepromclear=0;

			        if(eeprom_read_byte(&IDNB)==0xff)
			        {
						USART_TX((uint8_t*)"I am new block for Smart House\r\n",33);
						eeprom_write_byte (&IDNB, 0x00);
						eeprom_write_byte (&setPORTC, 0x00);

			        }
			        else
			        {
				        PORTA=eeprom_read_byte(&setPORTC);
				        USART_Transmit(PORTC);
			        }
		}
		else
		{
		PORTD |= (1 << 6);
		}
	
	}
}
//--------------------------------------------------