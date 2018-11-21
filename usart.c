#include "usart.h"
//---------------------------------------------------------------
USART_prop_ptr usartprop;
//---------------------------------------------------------------
void USART_Init( unsigned int ubrr)//������������� ������ USART
{
	//������� �������� ������ USART
	UBRR0H = (unsigned char)(ubrr>>8);
	UBRR0L = (unsigned char)ubrr;
		
	UCSR0B=(1<<RXEN0)|( 1<<TXEN0); //�������� ����� � �������� �� USART
	UCSR0B |= (1<<RXCIE0); //��������� ���������� ��� ��������
	UCSR0A |= (1<<U2X0); // �������� �������
	UCSR0C = (1<<UCSZ01)|(1<<UCSZ00);// ������������ ����� (UMSEL=0), ��� �������� �������� (UPM1=0 � UPM0=0),
	//1 ����-��� (USBS=0), 8-��� ������� (UCSZ01=1 � UCSZ00=1)
	usartprop.usart_buf[0]=0;
	usartprop.usart_cnt=0;
	usartprop.is_ip=0;
}
//---------------------------------------------------------------
void USART_Transmit( unsigned char data ) //������� �������� ������
{
	while ( !(UCSR0A & (1<<UDRE0)) ); //�������� ����������� ������ ������
	UDR0 = data; //������ �������� ������
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
	//���� ����� �������� �������� ����� ������
	if (usartprop.usart_cnt>25)
	{
		usartprop.usart_cnt=0;
	}
	else if (b=='a')
	{
		usartprop.is_ip=1;//������ �������� ARP-�������
		net_cmd();
	}
	else if (b=='u')
	{
		usartprop.is_ip=2;//������ ������� ��������� UDP-�����
		net_cmd();
	}
	else if (b=='p')
	{
		usartprop.is_ip=4;//������ ������� ��������� ICMP-�����
		net_cmd();
	}
	else if (b=='n')
	{
		usartprop.is_ip=6;//������ ������� ��������� NTP-�����
		net_cmd();
	}
	else
	{
		usartprop.usart_buf[usartprop.usart_cnt]=b;
		usartprop.usart_cnt++;		
	}
}
//----------------------------------------
