#ifndef __USART_CLIENT_H__
#define __USART_CLIENT_H__


void USART_Init(void);
void USART_main(void);

unsigned char USART_Has_Received_Data(void);
unsigned char USART_Transmit_Bytes(unsigned char* p_tx_buff, unsigned char nLen);
unsigned char USART_Receive_Bytes(unsigned char *ptrUserRxBuff, unsigned short* nIdxWrUser, unsigned short nMaxUserBuffSize);

#endif /* __USART_CLIENT_H__ */