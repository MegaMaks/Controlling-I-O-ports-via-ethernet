#ifndef USART_H_
#define USART_H_

#include "main.h"

void USART_Init( unsigned int ubrr);
void USART_Transmit( unsigned char data );
void  USART_TX (uint8_t *str1, uint8_t cnt);
//----------------------------------------------
typedef struct USART_prop{
	uint8_t usart_buf[25];
	uint8_t usart_cnt;
	uint8_t is_ip;
} USART_prop_ptr;
//----------------------------------------------
#endif /* USART_H_ */