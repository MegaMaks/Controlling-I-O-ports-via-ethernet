#include "smartUD1.h"

void smart_interrupt(void)
{
	_delay_ms(10);
	if(!(PINA & (1<<PINA0)))
	{		
		if(PORTC&(1<<PORTC0))
		{
			PORTC&=~(1<<0);
			eeprom_write_byte (&setPORTC, PORTC);
		}
		else
		{
			PORTC|=(1<<0);
			eeprom_write_byte (&setPORTC, PORTC);
		}
		chat_protect();
	}
		else if(!(PINA & (1<<PINA1)))
		{
			if(PORTC&(1<<PORTC1))
			{
				PORTC&=~(1<<1);
				eeprom_write_byte (&setPORTC, PORTC);
			}
			else
			{
				PORTC|=(1<<1);
				eeprom_write_byte (&setPORTC, PORTC);
			}
		chat_protect();
		}
		else if(!(PINA & (1<<PINA2)))
		{
			if(PORTC&(1<<PORTC2))
			{
				PORTC&=~(1<<2);
				eeprom_write_byte (&setPORTC, PORTC);
			}
			else
			{
				PORTC|=(1<<2);
				eeprom_write_byte (&setPORTC, PORTC);
			}
		chat_protect();
		}
		else if(!(PINA & (1<<PINA3)))
		{
			if(PORTC&(1<<PORTC3))
			{
				PORTC&=~(1<<3);
				eeprom_write_byte (&setPORTC, PORTC);
			}
			else
			{
				PORTC|=(1<<3);
				eeprom_write_byte (&setPORTC, PORTC);
			}
		chat_protect();
		}
		else if(!(PINA & (1<<PINA4)))
		{
			if(PORTC&(1<<PORTC4))
			{
				PORTC&=~(1<<4);
				eeprom_write_byte (&setPORTC, PORTC);
			}
			else
			{
				PORTC|=(1<<4);
				eeprom_write_byte (&setPORTC, PORTC);
			}
			chat_protect();
		}
		else if(!(PINA & (1<<PINA5)))
		{
			if(PORTC&(1<<PORTC5))
			{
				PORTC&=~(1<<5);
				eeprom_write_byte (&setPORTC, PORTC);
			}
			else
			{
				PORTC|=(1<<5);
				eeprom_write_byte (&setPORTC, PORTC);
			}
			chat_protect();
		}
		else if(!(PINA & (1<<PINA6)))
		{
			if(PORTC&(1<<PORTC6))
			{
				PORTC&=~(1<<6);
				eeprom_write_byte (&setPORTC, PORTC);
			}
			else
			{
				PORTC|=(1<<6);
				eeprom_write_byte (&setPORTC, PORTC);
			}
			chat_protect();
		}
		else if(!(PINA & (1<<PINA7)))
		{
			if(PORTC&(1<<PORTC7))
			{
				PORTC&=~(1<<7);
				eeprom_write_byte (&setPORTC, PORTC);
			}
			else
			{
				PORTC|=(1<<7);
				eeprom_write_byte (&setPORTC, PORTC);
			}
			chat_protect();
		}
}
void chat_protect(void)
{
	PORTD |= (1 << 4);
	_delay_ms(50);
	PORTD &=~ (1 << 4);
	_delay_ms(50);
	_delay_ms(300);
}
