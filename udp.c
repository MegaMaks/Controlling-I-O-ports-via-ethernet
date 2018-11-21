#include "udp.h"
//--------------------------------------------------
extern char str1[60];
extern uint8_t net_buf[ENC28J60_MAXFRAME];
//--------------------------------------------------
uint8_t udp_send(uint32_t ip_addr, uint16_t port)
{
	uint8_t res=0;
	uint16_t len;
	enc28j60_frame_ptr *frame=(void*) net_buf;
	ip_pkt_ptr *ip_pkt = (void*)(frame->data);
	udp_pkt_ptr *udp_pkt = (void*)(ip_pkt->data);
	udp_pkt->port_dst = be16toword(port);
	udp_pkt->port_src = be16toword(LOCAL_PORT);
	strcpy((char*)udp_pkt->data,"UDP Reply:\r\nHello to UDP Client!!!\r\n");
	len = strlen((char*)udp_pkt->data) + sizeof(udp_pkt_ptr);
	udp_pkt->len = be16toword(len);
	udp_pkt->cs=0;
	udp_pkt->cs=checksum((uint8_t*)udp_pkt-8, len+8, 1);
	ip_pkt->ipaddr_src = ip_addr;
	ip_pkt->prt = IP_UDP;
	ip_pkt->id = 0;
	ip_pkt->ts = 0;
	ip_pkt->verlen = 0x45;
	frame->type=ETH_IP;
	ip_send(frame,len+sizeof(ip_pkt_ptr));
	
	return res;
}
//--------------------------------------------------
uint8_t udp_reply(enc28j60_frame_ptr *frame, uint16_t len)
{
	uint8_t res=0;
	uint16_t port;
	ip_pkt_ptr *ip_pkt = (void*)(frame->data);
	udp_pkt_ptr *udp_pkt = (void*)(ip_pkt->data);
	port = udp_pkt->port_dst;
	udp_pkt->port_dst = udp_pkt->port_src;
	udp_pkt->port_src = port;
	strcpy((char*)udp_pkt->data,"UDP Reply:\r\nHello from UDP Server to UDP Client!!!\r\n");
	len = strlen((char*)udp_pkt->data) + sizeof(udp_pkt_ptr);
	udp_pkt->len = be16toword(len);
	udp_pkt->cs=0;
	udp_pkt->cs=checksum((uint8_t*)udp_pkt-8, len+8, 1);
	memcpy(frame->addr_dest,frame->addr_src,6);
	ip_send(frame,len+sizeof(ip_pkt_ptr));
	return res;
}
//-------------------------------
uint8_t udp_read(enc28j60_frame_ptr *frame, uint16_t len)
{
	uint8_t res=0;
	ip_pkt_ptr *ip_pkt = (void*)(frame->data);
	udp_pkt_ptr *udp_pkt = (void*)(ip_pkt->data);
	if(be16toword(udp_pkt->port_src)==123)
	{
		ntp_read(frame,len);
		return 0;
	}	
	sprintf(str1,"%02X:%02X:%02X:%02X:%02X:%02X-%02X:%02X:%02X:%02X:%02X:%02X; %d; ip\r\n",
		frame->addr_src[0],frame->addr_src[1],frame->addr_src[2],
		frame->addr_src[3],frame->addr_src[4],frame->addr_src[5],
		frame->addr_dest[0],frame->addr_dest[1],frame->addr_dest[2],
		frame->addr_dest[3],frame->addr_dest[4],frame->addr_dest[5],len);
	USART_TX((uint8_t*)str1,strlen(str1));
		sprintf(str1,"%ld.%ld.%ld.%ld-%ld.%ld.%ld.%ld udp request\r\n",
		ip_pkt->ipaddr_src & 0x000000FF,(ip_pkt->ipaddr_src>>8) & 0x000000FF,
		(ip_pkt->ipaddr_src>>16) & 0x000000FF, ip_pkt->ipaddr_src>>24,
		ip_pkt->ipaddr_dst & 0x000000FF,(ip_pkt->ipaddr_dst>>8) & 0x000000FF,
		(ip_pkt->ipaddr_dst>>16) & 0x000000FF, ip_pkt->ipaddr_dst>>24);
	USART_TX((uint8_t*)str1,strlen(str1));
	sprintf(str1,"%u-%u\r\n", be16toword(udp_pkt->port_src),be16toword(udp_pkt->port_dst));
	USART_TX((uint8_t*)str1,strlen(str1));
	USART_TX(udp_pkt->data,len-sizeof(udp_pkt_ptr));
	USART_TX((uint8_t*)"\r\n",2);
	udp_reply(frame,len);
	return res;
}
//--------------------------------------------------