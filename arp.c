#include "arp.h"
//--------------------------------------------------
extern char str1[60];
extern unsigned int tim_cnt;//счетчик тиков таймера
extern uint32_t clock_cnt;//счетчик секунд
extern uint8_t net_buf[ENC28J60_MAXFRAME];
extern uint8_t macaddr[6];
uint8_t macbroadcast[6]=MAC_BROADCAST;
uint8_t macnull[6]=MAC_NULL;
arp_record_ptr arp_rec[5];
uint8_t current_arp_index=0;
extern USART_prop_ptr usartprop;
//--------------------------------------------------
uint8_t arp_read(enc28j60_frame_ptr *frame, uint16_t len)
{
	uint8_t res=0;
	arp_msg_ptr *msg=(void*)(frame->data);
	if (len>=sizeof(arp_msg_ptr))
	{
		if ((msg->net_tp==ARP_ETH)&&(msg->proto_tp==ARP_IP))
		{
			if (msg->ipaddr_dst==IP_ADDR)
			{
				if (msg->op==ARP_REQUEST)
				{
					res=1;
				}
				else if (msg->op==ARP_REPLY)
				{
					sprintf(str1,"reply\r\nmac_src %02X:%02X:%02X:%02X:%02X:%02X\r\n",
					msg->macaddr_src[0],msg->macaddr_src[1],msg->macaddr_src[2],msg->macaddr_src[3],msg->macaddr_src[4],msg->macaddr_src[5]);
					USART_TX((uint8_t*)str1,strlen(str1));
					sprintf(str1,"ip_src %ld.%ld.%ld.%ld\r\n",
					msg->ipaddr_src & 0x000000FF,(msg->ipaddr_src>>8) & 0x000000FF,
					(msg->ipaddr_src>>16) & 0x000000FF, msg->ipaddr_src>>24);
					USART_TX((uint8_t*)str1,strlen(str1));
					sprintf(str1,"mac_dst %02X:%02X:%02X:%02X:%02X:%02X\r\n",
					msg->macaddr_dst[0],msg->macaddr_dst[1],msg->macaddr_dst[2],msg->macaddr_dst[3],msg->macaddr_dst[4],msg->macaddr_dst[5]);
					USART_TX((uint8_t*)str1,strlen(str1));
					sprintf(str1,"ip_dst %ld.%ld.%ld.%ld\r\n",
					msg->ipaddr_dst & 0x000000FF,(msg->ipaddr_dst>>8) & 0x000000FF,
					(msg->ipaddr_dst>>16) & 0x000000FF, msg->ipaddr_dst>>24);
					USART_TX((uint8_t*)str1,strlen(str1));
					res=2;
				}
			}
		}
	}
	return res;
}
//--------------------------------------------------
void arp_send(enc28j60_frame_ptr *frame)
{
	arp_msg_ptr *msg = (void*)frame->data;
	msg->op = ARP_REPLY;
	memcpy(msg->macaddr_dst,msg->macaddr_src,6);
	memcpy(msg->macaddr_src,macaddr,6);
	msg->ipaddr_dst = msg->ipaddr_src;
	msg->ipaddr_src = IP_ADDR;
	memcpy(frame->addr_dest,frame->addr_src,6);
	eth_send(frame,sizeof(arp_msg_ptr));
	sprintf(str1,"%02X:%02X:%02X:%02X:%02X:%02X(%ld.%ld.%ld.%ld)-",
  	  frame->addr_dest[0],frame->addr_dest[1],frame->addr_dest[2],frame->addr_dest[3],frame->addr_dest[4],frame->addr_dest[5],
	  msg->ipaddr_dst & 0x000000FF,(msg->ipaddr_dst>>8) & 0x000000FF,
	  (msg->ipaddr_dst>>16) & 0x000000FF, msg->ipaddr_dst>>24);
	USART_TX((uint8_t*)str1,strlen(str1));
	sprintf(str1,"%02X:%02X:%02X:%02X:%02X:%02X(%ld.%ld.%ld.%ld) arp request\r\n",
	  frame->addr_src[0],frame->addr_src[1],frame->addr_src[2],frame->addr_src[3],frame->addr_src[4],frame->addr_src[5],
	  msg->ipaddr_src & 0x000000FF,(msg->ipaddr_src>>8) & 0x000000FF,
	  (msg->ipaddr_src>>16) & 0x000000FF, msg->ipaddr_src>>24);
	USART_TX((uint8_t*)str1,strlen(str1));	
}
//--------------------------------------------------
uint8_t arp_request(uint32_t ip_addr)
{
	uint8_t i, j;
	enc28j60_frame_ptr *frame=(void*) net_buf;
	uint32_t ip;
	//проверим принадлежность адреса к локальной сети
	if( ((ip_addr ^ IP_ADDR) & IP_MASK) == 0 ) ip=ip_addr;
	else ip = IP_GATE;
	//проверим, может такой адрес уже есть в таблице ARP, а заодно и удалим оттуда просроченные записи
	for (j=0;j<5;j++)
	{
		//Если записи уже более 12 часов, то удалим её
		if((clock_cnt-arp_rec[j].sec>43200))
		{
			memset(arp_rec+(sizeof(arp_record_ptr)*j),0,sizeof(arp_record_ptr));
		}
		if (arp_rec[j].ipaddr==ip)
		{
			//смотрим ARP-таблицу
			for (i=0;i<5;i++)
			{
				sprintf(str1,"%ld.%ld.%ld.%ld - %02X:%02X:%02X:%02X:%02X:%02X - %lu\r\n",
				arp_rec[i].ipaddr & 0x000000FF,(arp_rec[i].ipaddr>>8) & 0x000000FF,
				(arp_rec[i].ipaddr>>16) & 0x000000FF, arp_rec[i].ipaddr>>24,
				arp_rec[i].mac_addr[0],arp_rec[i].mac_addr[1],arp_rec[i].mac_addr[2],
				arp_rec[i].mac_addr[3],arp_rec[i].mac_addr[4],arp_rec[i].mac_addr[5],
				arp_rec[i].sec);
				USART_TX((uint8_t*)str1,strlen(str1));
			}
			memcpy(frame->addr_dest,arp_rec[j].mac_addr,6);
			if((usartprop.is_ip==3)||(usartprop.is_ip==5)||(usartprop.is_ip==7))//статус отправки UDP-, ICMP- или NTP пакета
			{
				net_cmd();
			}
			return 0;
		}
	}
	arp_msg_ptr *msg = (void*)frame->data;
	msg->net_tp=ARP_ETH;
	msg->proto_tp=ARP_IP;
	msg->macaddr_len=6;
	msg->ipaddr_len=4;
	msg->op=ARP_REQUEST;
	memcpy(msg->macaddr_src,macaddr,6);
	msg->ipaddr_src = IP_ADDR;
	memcpy(msg->macaddr_dst,macnull,6);
	msg->ipaddr_dst = ip;
	memcpy(frame->addr_dest,macbroadcast,6);
	memcpy(frame->addr_src,macaddr,6);
	frame->type=ETH_ARP;
	enc28j60_packetSend((void*)frame,sizeof(arp_msg_ptr) + sizeof(enc28j60_frame_ptr));	
	return 1;
}
//--------------------------------------------------
void arp_table_fill(enc28j60_frame_ptr *frame)
{
	uint8_t i;
	arp_msg_ptr *msg = (void*)frame->data;
	//добавить запись
	arp_rec[current_arp_index].ipaddr=msg->ipaddr_src;
	memcpy(arp_rec[current_arp_index].mac_addr,msg->macaddr_src,6);
	arp_rec[current_arp_index].sec=clock_cnt;
	if(current_arp_index<4) current_arp_index++;
	else current_arp_index=0;
	//смотрим ARP-таблицу
	for (i=0;i<5;i++)
	{
		sprintf(str1,"%ld.%ld.%ld.%ld - %02X:%02X:%02X:%02X:%02X:%02X - %lu\r\n",
		arp_rec[i].ipaddr & 0x000000FF,(arp_rec[i].ipaddr>>8) & 0x000000FF,
		(arp_rec[i].ipaddr>>16) & 0x000000FF, arp_rec[i].ipaddr>>24,
		arp_rec[i].mac_addr[0],arp_rec[i].mac_addr[1],arp_rec[i].mac_addr[2],
		arp_rec[i].mac_addr[3],arp_rec[i].mac_addr[4],arp_rec[i].mac_addr[5],
		arp_rec[i].sec);
		USART_TX((uint8_t*)str1,strlen(str1));
	}
}
//--------------------------------------------------
