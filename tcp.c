#include "tcp.h"
//--------------------------------------------------
extern char str1[60];
extern uint8_t net_buf[ENC28J60_MAXFRAME];
extern uint8_t macaddr[6];
//--------------------------------------------------
uint8_t tcp_send(uint32_t ip_addr, uint16_t port, uint8_t op)
{
	uint8_t res=0;
	uint16_t len=0;
	uint32_t num_seg=0;
	uint16_t sz_data=0;
	enc28j60_frame_ptr *frame=(void*) net_buf;
	ip_pkt_ptr *ip_pkt = (void*)(frame->data);
	tcp_pkt_ptr *tcp_pkt = (void*)(ip_pkt->data);
	//
	data_cmd_ptr *data_cmd=(void*)(tcp_pkt->data);
	if (op==TCP_OP_SYNACK)
	{
		//�������� ��������� ������ TCP
		tcp_pkt->port_dst = be16toword(port);
		tcp_pkt->port_src = be16toword(LOCAL_PORT_TCP);
		tcp_pkt->num_ask = be32todword(be32todword(tcp_pkt->bt_num_seg) + 1);
		tcp_pkt->bt_num_seg = rand();
		tcp_pkt->fl = TCP_SYN | TCP_ACK;
		tcp_pkt->size_wnd = be16toword(8192);
		tcp_pkt->urg_ptr = 0;
		len = sizeof(tcp_pkt_ptr)+4;
		tcp_pkt->len_hdr = len << 2;
		tcp_pkt->data[0]=2;//Maximum Segment Size (2)
		tcp_pkt->data[1]=4;//Length
		tcp_pkt->data[2]=0x05;
		tcp_pkt->data[3]=0x82;
		tcp_pkt->cs = 0;
		tcp_pkt->cs=checksum((uint8_t*)tcp_pkt-8, len+8, 2);
		//�������� ��������� ������ IP
		sprintf(str1,"len:%d\r\n", len);
		USART_TX((uint8_t*)str1,strlen(str1));
		len+=sizeof(ip_pkt_ptr);
		sprintf(str1,"len:%d\r\n", len);
		USART_TX((uint8_t*)str1,strlen(str1));
		ip_pkt->len=be16toword(len);
		ip_pkt->id = 0;
		ip_pkt->ts = 0;
		ip_pkt->verlen = 0x45;
		ip_pkt->fl_frg_of=0;
		ip_pkt->ttl=128;
		ip_pkt->cs = 0;
		ip_pkt->prt=IP_TCP;
		ip_pkt->ipaddr_dst = ip_addr;
		ip_pkt->ipaddr_src = IP_ADDR;
		ip_pkt->cs = checksum((void*)ip_pkt,sizeof(ip_pkt_ptr),0);
		//�������� ��������� Ethernet
		memcpy(frame->addr_dest,frame->addr_src,6);
		memcpy(frame->addr_src,macaddr,6);
		frame->type=ETH_IP;
		len+=sizeof(enc28j60_frame_ptr);
		enc28j60_packetSend((void*)frame,len);
		sprintf(str1,"len:%d\r\n", len);
		USART_TX((uint8_t*)str1,strlen(str1));
		USART_TX((uint8_t*)"SYN ACK\r\n",9);
	}
	else if (op==TCP_OP_ACK_OF_FIN)
	{
		//�������� ��������� ������ TCP
		tcp_pkt->port_dst = be16toword(port);
		tcp_pkt->port_src = be16toword(LOCAL_PORT_TCP);
		num_seg = tcp_pkt->num_ask;
		tcp_pkt->num_ask = be32todword(be32todword(tcp_pkt->bt_num_seg) + 1);
		tcp_pkt->bt_num_seg = num_seg;
		tcp_pkt->fl = TCP_ACK;
		tcp_pkt->size_wnd = be16toword(8192);
		tcp_pkt->urg_ptr = 0;
		len = sizeof(tcp_pkt_ptr);
		tcp_pkt->len_hdr = len << 2;
		tcp_pkt->cs = 0;
		tcp_pkt->cs=checksum((uint8_t*)tcp_pkt-8, len+8, 2);
		//�������� ��������� ������ IP
		sprintf(str1,"len:%d\r\n", len);
		USART_TX((uint8_t*)str1,strlen(str1));
		len+=sizeof(ip_pkt_ptr);
		sprintf(str1,"len:%d\r\n", len);
		USART_TX((uint8_t*)str1,strlen(str1));
		ip_pkt->len=be16toword(len);
		ip_pkt->id = 0;
		ip_pkt->ts = 0;
		ip_pkt->verlen = 0x45;
		ip_pkt->fl_frg_of=0;
		ip_pkt->ttl=128;
		ip_pkt->cs = 0;
		ip_pkt->prt=IP_TCP;
		ip_pkt->ipaddr_dst = ip_addr;
		ip_pkt->ipaddr_src = IP_ADDR;
		ip_pkt->cs = checksum((void*)ip_pkt,sizeof(ip_pkt_ptr),0);
		//�������� ��������� Ethernet
		memcpy(frame->addr_dest,frame->addr_src,6);
		memcpy(frame->addr_src,macaddr,6);
		frame->type=ETH_IP;
		len+=sizeof(enc28j60_frame_ptr);
		enc28j60_packetSend((void*)frame,len);
		USART_TX((uint8_t*)"ACK OF FIN\r\n",12);
		tcp_pkt->fl = TCP_FIN|TCP_ACK;
		len = sizeof(tcp_pkt_ptr);
		tcp_pkt->cs = 0;
		tcp_pkt->cs=checksum((uint8_t*)tcp_pkt-8, len+8, 2);
		len+=sizeof(ip_pkt_ptr);
		len+=sizeof(enc28j60_frame_ptr);
		enc28j60_packetSend((void*)frame,len);
	}
	else if (op==TCP_OP_ACK_OF_DATA)
	{
		//�������� ��������� ������ TCP
		sz_data = be16toword(ip_pkt->len)-20-(tcp_pkt->len_hdr>>2);
		tcp_pkt->port_dst = be16toword(port);
		tcp_pkt->port_src = be16toword(LOCAL_PORT_TCP);
		num_seg = tcp_pkt->num_ask;
		tcp_pkt->num_ask = be32todword(be32todword(tcp_pkt->bt_num_seg) + sz_data);
		tcp_pkt->bt_num_seg = num_seg;
		tcp_pkt->fl = TCP_ACK;
		tcp_pkt->size_wnd = be16toword(8192);
		tcp_pkt->urg_ptr = 0;
		len = sizeof(tcp_pkt_ptr);
		tcp_pkt->len_hdr = len << 2;
		tcp_pkt->cs = 0;
		tcp_pkt->cs=checksum((uint8_t*)tcp_pkt-8, len+8, 2);
		//�������� ��������� ������ IP
		sprintf(str1,"len:%d\r\n", len);
		USART_TX((uint8_t*)str1,strlen(str1));
		len+=sizeof(ip_pkt_ptr);
		sprintf(str1,"len:%d\r\n", len);
		USART_TX((uint8_t*)str1,strlen(str1));
		ip_pkt->len=be16toword(len);
		ip_pkt->id = 0;
		ip_pkt->ts = 0;
		ip_pkt->verlen = 0x45;
		ip_pkt->fl_frg_of=0;
		ip_pkt->ttl=128;
		ip_pkt->cs = 0;
		ip_pkt->prt=IP_TCP;
		ip_pkt->ipaddr_dst = ip_addr;
		ip_pkt->ipaddr_src = IP_ADDR;
		ip_pkt->cs = checksum((void*)ip_pkt,sizeof(ip_pkt_ptr),0);
		//�������� ��������� Ethernet
		memcpy(frame->addr_dest,frame->addr_src,6);
		memcpy(frame->addr_src,macaddr,6);
		frame->type=ETH_IP;
		len+=sizeof(enc28j60_frame_ptr);
		enc28j60_packetSend((void*)frame,len);
		//���� ������ "Hello!!!", �� �������� �����
		if (!strcmp((char*)tcp_pkt->data,"POWER"))
		{
			strcpy((char*)tcp_pkt->data,"Power !!!\r\n");
			tcp_pkt->fl = TCP_ACK|TCP_PSH;
			sprintf(str1,"hdr_len:%d\r\n",sizeof(tcp_pkt_ptr));
			USART_TX((uint8_t*)str1,strlen(str1));
			len = sizeof(tcp_pkt_ptr);
			tcp_pkt->len_hdr = len << 2;
			len+=strlen((char*)tcp_pkt->data);
			tcp_pkt->cs = 0;
			tcp_pkt->cs=checksum((uint8_t*)tcp_pkt-8, len+8, 2);
			//�������� ��������� ������ IP
			len+=sizeof(ip_pkt_ptr);
			ip_pkt->len=be16toword(len);
			ip_pkt->cs = 0;
			ip_pkt->cs = checksum((void*)ip_pkt,sizeof(ip_pkt_ptr),0);
			len+=sizeof(enc28j60_frame_ptr);
			enc28j60_packetSend((void*)frame,len);
			PORTD|=(1<<4);

		}	

		if (data_cmd->cmd==LIGHT)
		{
			PORTC=data_cmd->cmd_value;
			eeprom_write_byte (&setPORTC, PORTC);
			tcp_pkt->data[0]=data_cmd->cmd;
			tcp_pkt->data[1]=PORTC;
			tcp_pkt->fl = TCP_ACK|TCP_PSH;
			sprintf(str1,"hdr_len:%d\r\n",sizeof(tcp_pkt_ptr));
			USART_TX((uint8_t*)str1,strlen(str1));
			len = sizeof(tcp_pkt_ptr);
			tcp_pkt->len_hdr = len << 2;
			len+=strlen((char*)tcp_pkt->data);
			tcp_pkt->cs = 0;
			tcp_pkt->cs=checksum((uint8_t*)tcp_pkt-8, len+8, 2);
			//�������� ��������� ������ IP
			len+=sizeof(ip_pkt_ptr);
			ip_pkt->len=be16toword(len);
			ip_pkt->cs = 0;
			ip_pkt->cs = checksum((void*)ip_pkt,sizeof(ip_pkt_ptr),0);
			len+=sizeof(enc28j60_frame_ptr);
			enc28j60_packetSend((void*)frame,len);
			PORTD|=(1<<4);
	}else if (data_cmd->cmd==LIGHT_STATUS)
	{
		tcp_pkt->data[0]=data_cmd->cmd;
		tcp_pkt->data[1]=PORTC;
		tcp_pkt->fl = TCP_ACK|TCP_PSH;
		sprintf(str1,"hdr_len:%d\r\n",sizeof(tcp_pkt_ptr));
		USART_TX((uint8_t*)str1,strlen(str1));
		len = sizeof(tcp_pkt_ptr);
		tcp_pkt->len_hdr = len << 2;
		len+=strlen((char*)tcp_pkt->data);
		tcp_pkt->cs = 0;
		tcp_pkt->cs=checksum((uint8_t*)tcp_pkt-8, len+8, 2);
		//�������� ��������� ������ IP
		len+=sizeof(ip_pkt_ptr);
		ip_pkt->len=be16toword(len);
		ip_pkt->cs = 0;
		ip_pkt->cs = checksum((void*)ip_pkt,sizeof(ip_pkt_ptr),0);
		len+=sizeof(enc28j60_frame_ptr);
		enc28j60_packetSend((void*)frame,len);
		PORTD|=(1<<4);
	}

	}	
	return res;
}
//--------------------------------------------------
uint8_t tcp_read(enc28j60_frame_ptr *frame, uint16_t len)
{
	uint8_t res=0;
	uint16_t len_data=0;
	uint16_t i=0;
	ip_pkt_ptr *ip_pkt = (void*)(frame->data);
	tcp_pkt_ptr *tcp_pkt = (void*)(ip_pkt->data);
	len_data = be16toword(ip_pkt->len)-20-(tcp_pkt->len_hdr>>2);
	sprintf(str1,"%ld.%ld.%ld.%ld-%ld.%ld.%ld.%ld %d tcp\r\n",
		ip_pkt->ipaddr_src & 0x000000FF,(ip_pkt->ipaddr_src>>8) & 0x000000FF,
		(ip_pkt->ipaddr_src>>16) & 0x000000FF, ip_pkt->ipaddr_src>>24,
		ip_pkt->ipaddr_dst & 0x000000FF,(ip_pkt->ipaddr_dst>>8) & 0x000000FF,
		(ip_pkt->ipaddr_dst>>16) & 0x000000FF, ip_pkt->ipaddr_dst>>24, len_data);
	USART_TX((uint8_t*)str1,strlen(str1));
	//���� ���� ������, �� ������� �� � ������������ ���������
	if (len_data)
	{
	  for (i=0;i<len_data;i++)
	  {
		  USART_Transmit(tcp_pkt->data[i]);
	  }
	  USART_TX((uint8_t*)"\r\n",2);
	  //���� ������� ���� �������������, �� ���������� ���� ������
	  if (tcp_pkt->fl&TCP_ACK)
	  {
		  tcp_send(ip_pkt->ipaddr_src, be16toword(tcp_pkt->port_src), TCP_OP_ACK_OF_DATA);
	  }
	}
	if (tcp_pkt->fl == TCP_SYN)
	{
		tcp_send(ip_pkt->ipaddr_src, be16toword(tcp_pkt->port_src), TCP_OP_SYNACK);
	}
	else if (tcp_pkt->fl == (TCP_FIN|TCP_ACK))
	{
		tcp_send(ip_pkt->ipaddr_src, be16toword(tcp_pkt->port_src), TCP_OP_ACK_OF_FIN);
	}
	else if (tcp_pkt->fl == (TCP_PSH|TCP_ACK))
	{
		//���� ������ ���
		if(!be16toword(ip_pkt->len)-20-(tcp_pkt->len_hdr>>2))
		{
			tcp_send(ip_pkt->ipaddr_src, be16toword(tcp_pkt->port_src), TCP_OP_ACK_OF_FIN);
		}
	}
	else if (tcp_pkt->fl == TCP_ACK)
	{
		USART_TX((uint8_t*)"ACK\r\n",5);
	}
	return res;
}
//--------------------------------------------------
