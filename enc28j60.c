#include "enc28j60.h"
//--------------------------------------------------
static uint8_t Enc28j60Bank;
static int gNextPacketPtr;
uint8_t macaddr[6]=MAC_ADDR;
//--------------------------------------------------
void SPI_ini(void)
{
	DDRB|=(1<<SS)|(1<<MOSI)|(1<<SCK);//ножки SPI на выход
	DDRB&=~(1<<MISO);//ножка MISO на вход
	SS_DESELECT();
	SPCR=(1<<SPE)|(1<<MSTR);//Enable SPI, Master
	SPSR|=(1<<SPI2X);//Double SPI Speed (fosc/2=8MHz)
}
//--------------------------------------------------
uint8_t SPI_ChangeByte(uint8_t bt)
{
	SPDR = bt;//отправим байт в регистр данных
	while (!(SPSR&(1<<SPIF)))//подождЄм, пока данные передадутс€
	;
	return SPDR;
}
//--------------------------------------------------
void SPI_SendByte(uint8_t bt)
{
	SPI_ChangeByte(bt);
}
//--------------------------------------------------
uint8_t SPI_ReceiveByte(void)
{
	uint8_t bt = SPI_ChangeByte(0xFF);
	return bt; //вернЄм регистр данных
}
//--------------------------------------------------
void enc28j60_writeOp(uint8_t op,uint8_t addres, uint8_t data)
{
	SS_SELECT();
	SPI_SendByte(op|(addres&ADDR_MASK));
	SPI_SendByte(data);
	SS_DESELECT();
}
//--------------------------------------------------
static uint8_t enc28j60_readOp(uint8_t op,uint8_t addres)
{
	uint8_t result;
	SS_SELECT();
	SPI_SendByte(op|(addres&ADDR_MASK));
	SPI_SendByte(0x00);
	//пропускаем ложный байт
 	if(addres & 0x80) SPI_ReceiveByte();
 	result = SPI_ReceiveByte();
 	SS_DESELECT();
 	return result;
}
//--------------------------------------------------
static void enc28j60_readBuf(uint16_t len,uint8_t* data)
{
	SS_SELECT();
	SPI_SendByte(ENC28J60_READ_BUF_MEM);
	while(len--){
		SPI_SendByte(0x00);
		*data++=SPDR;
	}
	SS_DESELECT();
}
//--------------------------------------------------
static void enc28j60_writeBuf(uint16_t len,uint8_t* data)
{
	SS_SELECT();
	SPI_SendByte(ENC28J60_WRITE_BUF_MEM);
	while(len--)
		SPI_SendByte(*data++);
	SS_DESELECT();
}
//--------------------------------------------------
static void enc28j60_SetBank(uint8_t addres)
{
	if ((addres&BANK_MASK)!=Enc28j60Bank)
	{
		enc28j60_writeOp(ENC28J60_BIT_FIELD_CLR,ECON1,ECON1_BSEL1|ECON1_BSEL0);
		Enc28j60Bank = addres&BANK_MASK;
		enc28j60_writeOp(ENC28J60_BIT_FIELD_SET,ECON1,Enc28j60Bank>>5);
	}
}
//--------------------------------------------------
static void enc28j60_writeRegByte(uint8_t addres,uint8_t data)
{
	enc28j60_SetBank(addres);
	enc28j60_writeOp(ENC28J60_WRITE_CTRL_REG,addres,data);
}
//--------------------------------------------------
static uint8_t enc28j60_readRegByte(uint8_t addres)
{
	enc28j60_SetBank(addres);
	return enc28j60_readOp(ENC28J60_READ_CTRL_REG,addres);
}
//--------------------------------------------------
static void enc28j60_writeReg(uint8_t addres,uint16_t data)
{
	enc28j60_writeRegByte(addres, data);
	enc28j60_writeRegByte(addres+1, data>>8);
}
//--------------------------------------------------
static void enc28j60_writePhy(uint8_t addres,uint16_t data)
{
	enc28j60_writeRegByte(MIREGADR, addres);
	enc28j60_writeReg(MIWR, data);
	while(enc28j60_readRegByte(MISTAT)&MISTAT_BUSY)
	;
}
//--------------------------------------------------
void enc28j60_ini(void)
{
	SPI_ini();
	enc28j60_writeOp(ENC28J60_SOFT_RESET,0,ENC28J60_SOFT_RESET);
	_delay_ms(2);
	//проверим, что всЄ перезагрузилось
	while(!enc28j60_readOp(ENC28J60_READ_CTRL_REG,ESTAT)&ESTAT_CLKRDY)
	;
	//настроим буферы
	enc28j60_writeReg(ERXST,RXSTART_INIT);
	enc28j60_writeReg(ERXRDPT,RXSTART_INIT);
	enc28j60_writeReg(ERXND,RXSTOP_INIT);
	enc28j60_writeReg(ETXST,TXSTART_INIT);
	enc28j60_writeReg(ETXND,TXSTOP_INIT);
	//Enable Broadcast
	enc28j60_writeRegByte(ERXFCON,enc28j60_readRegByte(ERXFCON)|ERXFCON_BCEN);
	//настраиваем канальный уровень
	enc28j60_writeRegByte(MACON1,MACON1_MARXEN|MACON1_TXPAUS|MACON1_RXPAUS);
	enc28j60_writeRegByte(MACON2,0x00);
	enc28j60_writeOp(ENC28J60_BIT_FIELD_SET,MACON3,MACON3_PADCFG0|MACON3_TXCRCEN|MACON3_FRMLNEN);
	enc28j60_writeReg(MAIPG,0x0C12);
	enc28j60_writeRegByte(MABBIPG,0x12);//промежуток между фреймами
	enc28j60_writeReg(MAMXFL,MAX_FRAMELEN);//максимальный размер фрейма
	enc28j60_writeRegByte(MAADR5,macaddr[0]);//Set MAC addres
	enc28j60_writeRegByte(MAADR4,macaddr[1]);
	enc28j60_writeRegByte(MAADR3,macaddr[2]);
	enc28j60_writeRegByte(MAADR2,macaddr[3]);
	enc28j60_writeRegByte(MAADR1,macaddr[4]);
	enc28j60_writeRegByte(MAADR0,macaddr[5]);
	//настраиваем физический уровень
	enc28j60_writePhy(PHCON2,PHCON2_HDLDIS);//отключаем loopback
	enc28j60_writePhy(PHLCON,PHLCON_LACFG2| //светодиоды
		PHLCON_LBCFG2|PHLCON_LBCFG1|PHLCON_LBCFG0|
		PHLCON_LFRQ0|PHLCON_STRCH);
	enc28j60_SetBank(ECON1);
	enc28j60_writeOp(ENC28J60_BIT_FIELD_SET,EIE,EIE_INTIE|EIE_PKTIE);
	enc28j60_writeOp(ENC28J60_BIT_FIELD_SET,ECON1,ECON1_RXEN);//разрешаем приЄм пакетов
	//¬ключим делитель частоты генератора 2, то есть частота будет 12,5 MHz
	enc28j60_writeRegByte(ECOCON,0x02);
	_delay_us(15);
}
//--------------------------------------------------
uint16_t enc28j60_packetReceive(uint8_t *buf,uint16_t buflen)
{
	uint16_t len=0;
	if(enc28j60_readRegByte(EPKTCNT)>0)
	{
		enc28j60_writeReg(ERDPT,gNextPacketPtr);
		//считаем заголовок
		struct{
			uint16_t nextPacket;
			uint16_t byteCount;
			uint16_t status;
		} header;
		enc28j60_readBuf(sizeof header,(uint8_t*)&header);
		gNextPacketPtr=header.nextPacket;
		len=header.byteCount-4;//remove the CRC count
		if(len>buflen)	len=buflen;
		if((header.status&0x80)==0)	len=0;
		else	enc28j60_readBuf(len, buf);
		buf[len]=0;
		if(gNextPacketPtr-1>RXSTOP_INIT)
			enc28j60_writeReg(ERXRDPT,RXSTOP_INIT);
		else
			enc28j60_writeReg(ERXRDPT,gNextPacketPtr-1);
		enc28j60_writeOp(ENC28J60_BIT_FIELD_SET,ECON2,ECON2_PKTDEC);
		
	}
	return len;
}
//--------------------------------------------------
void enc28j60_packetSend(uint8_t *buf,uint16_t buflen)
{
	while(enc28j60_readOp(ENC28J60_READ_CTRL_REG,ECON1)&ECON1_TXRTS)
	{
		if(enc28j60_readRegByte(EIR)& EIR_TXERIF)
		{
			enc28j60_writeOp(ENC28J60_BIT_FIELD_SET,ECON1,ECON1_TXRST);
			enc28j60_writeOp(ENC28J60_BIT_FIELD_CLR,ECON1,ECON1_TXRST);
		}
	}
	enc28j60_writeReg(EWRPT,TXSTART_INIT);
	enc28j60_writeReg(ETXND,TXSTART_INIT+buflen);
	enc28j60_writeBuf(1,(uint8_t*)"\x00");
	enc28j60_writeBuf(buflen,buf);
	enc28j60_writeOp(ENC28J60_BIT_FIELD_SET,ECON1,ECON1_TXRTS);
}
//--------------------------------------------------
