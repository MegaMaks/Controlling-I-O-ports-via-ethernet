#include "net.h"
//--------------------------------------------------
unsigned int tim_cnt=0;//счетчик тиков таймера
uint32_t clock_cnt=0;//счетчик секунд
char str1[60]={0};
uint32_t ping_cnt=0;//счетчик отправленных пингов
//--------------------------------------------------
uint8_t net_buf[ENC28J60_MAXFRAME];
extern uint8_t macaddr[6];
extern USART_prop_ptr usartprop;
extern ntp_prop_ptr ntpprop;
//--------------------------------------------------
void init_timer(void)
{
	TCCR0A |= (1<<WGM01); // устанавливаем режим СТС (сброс по совпадению)
	OCR0A = 0xFF; //записываем в регистр число для сравнения
	TIMSK0 |= (1<<OCIE0A);	//устанавливаем бит разрешения прерывания 0-ого счетчика по совпадению с OCR0A
	TCCR0B |= (0<<CS02)|(1<<CS01)|(1<<CS00);//установим предделитель 64.
	//тем самым получаем - частота тактирования / предделитель / 256 = 976,5625(около милисекунды)
}
//--------------------------------------------------
ISR (TIMER0_COMPA_vect)
{
	tim_cnt++;
	//считаем секунды и записываем их в clock_cnt
	if(tim_cnt>=1000)
	{
		tim_cnt=0;
		clock_cnt++;
		if (ntpprop.set)
		{
			ntpprop.ntp_timer--;
			if ((ntpprop.ntp_timer<0)&&(ntpprop.ntp_cnt>0))
			{
				  ntpprop.ntp_timer = 5;
				  ntpprop.ntp_cnt--;
				  sprintf(str1,"ntp_cnt: %d\r\n",ntpprop.ntp_cnt);
				  USART_TX((uint8_t*)str1,strlen(str1));
				  ntp_request(ntpprop.ip_dst,ntpprop.port_dst);
			}
			else if (ntpprop.ntp_cnt<=0)
			{
				//сбросим все флаги и счетчики
				ntpprop.set=0;
				ntpprop.ntp_cnt=0;
				ntpprop.ntp_timer=0;
			}
		}
	}
}
//-----------------------------------------
void net_ini(void)
{
	enc28j60_ini();
	ntpprop.set=0;
	ntpprop.ntp_cnt=0;
	ntpprop.ntp_timer=0;
}
//--------------------------------------------------
uint16_t checksum(uint8_t *ptr, uint16_t len, uint8_t type)
{
	uint32_t sum=0;
	if(type==1)
	{
		sum+=IP_UDP;
		sum+=len-8;
	}
	if(type==2)
	{
		sum+=IP_TCP;
		sum+=len-8;
	}
	while(len>1)
	{
 		sum += (uint16_t) (((uint32_t)*ptr<<8)|*(ptr+1));
		ptr+=2;
		len-=2;
	}
 	if(len)	sum+=((uint32_t)*ptr)<<8;
 	while (sum>>16)	sum=(uint16_t)sum+(sum>>16);
	return ~be16toword((uint16_t)sum);
}
//--------------------------------------------------
void eth_send(enc28j60_frame_ptr *frame, uint16_t len)
{
	memcpy(frame->addr_src,macaddr,6);
	enc28j60_packetSend((void*)frame,len + sizeof(enc28j60_frame_ptr));
}
//--------------------------------------------------
uint8_t ip_send(enc28j60_frame_ptr *frame, uint16_t len)
{
	uint8_t res=0;
	ip_pkt_ptr *ip_pkt = (void*)frame->data;
	//Заполним заголовок пакета IP
	ip_pkt->len=be16toword(len);
	ip_pkt->fl_frg_of=0;
	ip_pkt->ttl=128;
	ip_pkt->cs = 0;
	ip_pkt->ipaddr_dst = ip_pkt->ipaddr_src;
	ip_pkt->ipaddr_src = IP_ADDR;
	ip_pkt->cs = checksum((void*)ip_pkt,sizeof(ip_pkt_ptr),0);
	//отправим фрейм
	eth_send(frame,len);
	return res;	
}
//--------------------------------------------------
uint8_t icmp_read(enc28j60_frame_ptr *frame, uint16_t len)
{
	uint8_t res=0;
	ip_pkt_ptr *ip_pkt = (void*)frame->data;
	icmp_pkt_ptr *icmp_pkt = (void*)ip_pkt->data;
	//Отфильтруем пакет по длине и типу сообщения - эхо-запрос
	if (len>=sizeof(icmp_pkt_ptr))
	{
		if (icmp_pkt->msg_tp==ICMP_REQ)
		{		icmp_pkt->msg_tp=ICMP_REPLY;
			icmp_pkt->cs=0;
			icmp_pkt->cs=checksum((void*)icmp_pkt,len,0);
			memcpy(frame->addr_dest,frame->addr_src,6);
			ip_send(frame,len+sizeof(ip_pkt_ptr));
			sprintf(str1,"%ld.%ld.%ld.%ld-%ld.%ld.%ld.%ld icmp request\r\n",
			ip_pkt->ipaddr_dst & 0x000000FF,(ip_pkt->ipaddr_dst>>8) & 0x000000FF,
			(ip_pkt->ipaddr_dst>>16) & 0x000000FF, ip_pkt->ipaddr_dst>>24,
			ip_pkt->ipaddr_src & 0x000000FF,(ip_pkt->ipaddr_src>>8) & 0x000000FF,
			(ip_pkt->ipaddr_src>>16) & 0x000000FF, ip_pkt->ipaddr_src>>24);
			USART_TX((uint8_t*)str1,strlen(str1));
		}
		else if (icmp_pkt->msg_tp==ICMP_REPLY)
		{
			sprintf(str1,"%ld.%ld.%ld.%ld-%ld.%ld.%ld.%ld icmp reply\r\n",
			ip_pkt->ipaddr_src & 0x000000FF,(ip_pkt->ipaddr_src>>8) & 0x000000FF,
			(ip_pkt->ipaddr_src>>16) & 0x000000FF, ip_pkt->ipaddr_src>>24,
			ip_pkt->ipaddr_dst & 0x000000FF,(ip_pkt->ipaddr_dst>>8) & 0x000000FF,
			(ip_pkt->ipaddr_dst>>16) & 0x000000FF, ip_pkt->ipaddr_dst>>24);
			USART_TX((uint8_t*)str1,strlen(str1));
		}	
	}
	return res;
}
//--------------------------------------------------
uint8_t ip_read(enc28j60_frame_ptr *frame, uint16_t len)
{
	uint8_t res=0;
	ip_pkt_ptr *ip_pkt = (void*)(frame->data);	
	if((ip_pkt->verlen==0x45)&&(ip_pkt->ipaddr_dst==IP_ADDR))
	{
		//длина данных
		len = be16toword(ip_pkt->len) - sizeof(ip_pkt_ptr);
		if (ip_pkt->prt==IP_ICMP)
		{
			icmp_read(frame,len);
		}
		else if (ip_pkt->prt==IP_TCP)
		{
			tcp_read(frame,len);
		}
		else if (ip_pkt->prt==IP_UDP)
		{
			udp_read(frame,len);
		}
	}
	return res;
}
//--------------------------------------------------
uint8_t icmp_request(uint32_t ip_addr)
{
	uint8_t res=0;
	uint16_t len;
	enc28j60_frame_ptr *frame=(void*) net_buf;
	ip_pkt_ptr *ip_pkt = (void*)(frame->data);
	icmp_pkt_ptr *icmp_pkt = (void*)ip_pkt->data;
	//Заполним заголовок пакета ICMP
	icmp_pkt->msg_tp = 8;
	icmp_pkt->msg_cd = 0;
	icmp_pkt->id = be16toword(1);
	icmp_pkt->num = be16toword(ping_cnt);
	ping_cnt++;
	strcpy((char*)icmp_pkt->data,"abcdefghijklmnopqrstuvwabcdefghi");
	icmp_pkt->cs = 0;
	len = strlen((char*)icmp_pkt->data) + sizeof(icmp_pkt_ptr);
	icmp_pkt->cs=checksum((void*)icmp_pkt,len,0);
	//Заполним заголовок пакета IP
	len+=sizeof(ip_pkt_ptr);
	ip_pkt->len=be16toword(len);
	ip_pkt->id = 0;
	ip_pkt->ts = 0;
	ip_pkt->verlen = 0x45;
	ip_pkt->fl_frg_of=0;
	ip_pkt->ttl=128;
	ip_pkt->cs = 0;
	ip_pkt->prt=IP_ICMP;
	ip_pkt->ipaddr_dst = ip_addr;
	ip_pkt->ipaddr_src = IP_ADDR;
	ip_pkt->cs = checksum((void*)ip_pkt,sizeof(ip_pkt_ptr),0);
	//Заполним заголовок пакета Ethernet
	memcpy(frame->addr_src,macaddr,6);
	frame->type=ETH_IP;
	enc28j60_packetSend((void*)frame,len + sizeof(enc28j60_frame_ptr));	
	return res;
}
//--------------------------------------------------
uint16_t port_extract(char* ip_str, uint8_t len)
{
	uint16_t port=0;
	int ch1=':';
	char *ss1;
	uint8_t offset = 0;
	ss1=strchr(ip_str,ch1);
	offset=ss1-ip_str+1;
	ip_str+=offset;
	port = atoi(ip_str);
	return port;
}
//--------------------------------------------------
uint32_t ip_extract(char* ip_str, uint8_t len)
{
	uint32_t ip=0;
	uint8_t offset = 0;
	uint8_t i;
	char ss2[5]={0};
	char *ss1;
	int ch1='.';
	int ch2=':';
	for(i=0;i<3;i++)
	{
		ss1=strchr(ip_str,ch1);
		offset=ss1-ip_str+1;
		strncpy(ss2,ip_str,offset);
		ss2[offset]=0;
		ip |= ((uint32_t)atoi(ss2))<<(i*8);
		ip_str+=offset;
		len-=offset;
	}
	ss1=strchr(ip_str,ch2);
	if (ss1!=NULL)
	{
		offset=ss1-ip_str+1;
		strncpy(ss2,ip_str,offset);
		ss2[offset]=0;
		ip |= ((uint32_t)atoi(ss2))<<24;
		return ip;
	}
	strncpy(ss2,ip_str,len);
	ss2[len]=0;
	ip |= ((uint32_t)atoi(ss2))<<24;
	return ip;
}
//--------------------------------------------------
void eth_read(enc28j60_frame_ptr *frame, uint16_t len)
{
	uint8_t res=0;
	if (len>=sizeof(enc28j60_frame_ptr))
	{
		if(frame->type==ETH_ARP)
		{
			res=arp_read(frame,len-sizeof(enc28j60_frame_ptr));
			if(res==1)
			{
				arp_send(frame);
			}
			else if(res==2)
			{
				arp_table_fill(frame);
				if((usartprop.is_ip==3)||(usartprop.is_ip==5)||(usartprop.is_ip==7))//статус отправки UDP-, ICMP- или NTP пакета
				{
					memcpy(frame->addr_dest,frame->addr_src,6);
					net_cmd();
				}
			}
		}
		else if(frame->type==ETH_IP)
		{
			ip_read(frame,len-sizeof(ip_pkt_ptr));
		}
		else
		{
			sprintf(str1,"%02X:%02X:%02X:%02X:%02X:%02X-%02X:%02X:%02X:%02X:%02X:%02X; %d; %04X",
			frame->addr_src[0],frame->addr_src[1],frame->addr_src[2],frame->addr_src[3],frame->addr_src[4],frame->addr_src[5],
			frame->addr_dest[0],frame->addr_dest[1],frame->addr_dest[2],frame->addr_dest[3],frame->addr_dest[4],frame->addr_dest[5],
			len, be16toword(frame->type));
			USART_TX((uint8_t*)str1,strlen(str1));
			USART_TX((uint8_t*)"\r\n",2);
		}		
	}
}
//--------------------------------------------------
void net_pool(void)
{
	uint16_t len;
	enc28j60_frame_ptr *frame=(void*)net_buf;
	while ((len=enc28j60_packetReceive(net_buf,sizeof(net_buf))))
	{
		eth_read(frame,len);
	}
}
//--------------------------------------------------
void net_cmd(void)
{
	static uint32_t ip=0;
	static uint16_t port=0;
	enc28j60_frame_ptr *frame=(void*)net_buf;
	if(usartprop.is_ip==1)//статус отправки ARP-запроса
	{
		ip=ip_extract((char*)usartprop.usart_buf,usartprop.usart_cnt);
		arp_request(ip);
		usartprop.is_ip=0;
		usartprop.usart_cnt=0;
	}
	else if(usartprop.is_ip==2)//статус попытки отправить UDP-пакет
	{
		ip=ip_extract((char*)usartprop.usart_buf,usartprop.usart_cnt);
		usartprop.is_ip=3;//статус отправки UDP-пакета
		usartprop.usart_cnt=0;
		arp_request(ip);//узнаем mac-адрес
	}
	else if(usartprop.is_ip==3)//статус отправки UDP-пакета
	{
		port=port_extract((char*)usartprop.usart_buf,usartprop.usart_cnt);
		udp_send(ip,port);
		usartprop.is_ip=0;
	}
	  else if(usartprop.is_ip==4)//статус попытки отправить ICMP-пакет
	  {
		  ip=ip_extract((char*)usartprop.usart_buf,usartprop.usart_cnt);
		  usartprop.is_ip=5;//статус отправки ICMP-пакета
		  usartprop.usart_cnt=0;
		  arp_request(ip);//узнаем mac-адрес
	  }
	  else if(usartprop.is_ip==5)//статус отправки ICMP-пакета
	  {
		  icmp_request(ip);
		  usartprop.is_ip=0;
	  }
	  else if(usartprop.is_ip==6)//статус попытки отправить NTP-пакет
	  {
		  ip=ip_extract((char*)usartprop.usart_buf,usartprop.usart_cnt);
		  ntpprop.ip_dst = ip;
		  usartprop.is_ip=7;//статус отправки NTP-пакета
		  usartprop.usart_cnt=0;
		  arp_request(ip);//узнаем mac-адрес
	  }
	  else if(usartprop.is_ip==7)//статус отправки NTP-пакета
	  {
		port=port_extract((char*)usartprop.usart_buf,usartprop.usart_cnt);
		ntpprop.port_dst = port;
		ntpprop.ntp_cnt = 10; //10 попыток
		ntpprop.ntp_timer = 5;//5 секунд до следующей попытки
		ntpprop.set=1;//флаг запроса времени взведен
		memcpy(ntpprop.macaddr_dst,frame->addr_dest,6);
		ntp_request(ntpprop.ip_dst,ntpprop.port_dst);
		usartprop.is_ip=0;
	  }
}
//--------------------------------------------------
