#ifndef UDP_H_
#define UDP_H_

#include "enc28j60.h"
#include "net.h"
#include "usart.h"
//--------------------------------------------------
#define LOCAL_PORT 333
//--------------------------------------------------
typedef struct udp_pkt {
	uint16_t port_src;//порт отправителя
	uint16_t port_dst;//порт получателя
	uint16_t len;//длина
	uint16_t cs;//контрольная сумма заголовка
	uint8_t data[];//данные
} udp_pkt_ptr;
//--------------------------------------------------
uint8_t udp_read(enc28j60_frame_ptr *frame, uint16_t len);
uint8_t udp_send(uint32_t ip_addr, uint16_t port);
//--------------------------------------------------
#endif /* UDP_H_ */