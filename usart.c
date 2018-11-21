#include "usart.h"
//---------------------------------------------------------------
USART_prop_ptr usartprop;
//---------------------------------------------------------------
void USART_Init( unsigned int ubrr)//Инициализация модуля USART
{
	//Зададим скорость работы USART
	UBRR0H = (unsigned char)(ubrr>>8);
	UBRR0L = (unsigned char)ubrr;
		
	UCSR0B=(1<<RXEN0)|( 1<<TXEN0); //Включаем прием и передачу по USART
	UCSR0B |= (1<<RXCIE0); //Разрешаем прерывание при передаче
	UCSR0A |= (1<<U2X0); // Удвоение частоты
	UCSR0C = (1<<UCSZ01)|(1<<UCSZ00);// ассинхронный режим (UMSEL=0), без контроля четности (UPM1=0 и UPM0=0),
	//1 стоп-бит (USBS=0), 8-бит посылка (UCSZ01=1 и UCSZ00=1)
	usartprop.usart_buf[0]=0;
	usartprop.usart_cnt=0;
	usartprop.is_ip=0;
}
//---------------------------------------------------------------
void USART_Transmit( unsigned char data ) //Функция отправки данных
{
	while ( !(UCSR0A & (1<<UDRE0)) ); //Ожидание опустошения буфера приема
	UDR0 = data; //Начало передачи данных
}
//----------------------------------------
void  USART_TX (uint8_t *str1, uint8_t cnt)
{

	uint8_t i;
	for(i=0;i<cnt;i++)
	USART_Transmit(str1[i]);
}
//----------------------------------------
ISR(USART0_RX_vect)
{
	uint8_t b;
	b=UDR0;
	//если вдруг случайно превысим длину буфера
	if (usartprop.usart_cnt>25)
	{
		usartprop.usart_cnt=0;
	}
	else if (b=='a')
	{
		usartprop.is_ip=1;//статус отправки ARP-запроса
		net_cmd();
	}
	else if (b=='u')
	{
		usartprop.is_ip=2;//статус попытки отправить UDP-пакет
		net_cmd();
	}
	else if (b=='p')
	{
		usartprop.is_ip=4;//статус попытки отправить ICMP-пакет
		net_cmd();
	}
	else if (b=='n')
	{
		usartprop.is_ip=6;//статус попытки отправить NTP-пакет
		net_cmd();
	}
	else
	{
		usartprop.usart_buf[usartprop.usart_cnt]=b;
		usartprop.usart_cnt++;		
	}
}
//----------------------------------------
