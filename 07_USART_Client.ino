#include "07_USART_Client.h"
#include "07_USART_Client_cfg.h"

#include <Arduino.h>
#include <string.h>

#define NULL_PTR (0)




/* RX pointers to actual buffers to be used by the client application */
static unsigned char USART_RX_Buff[USART_RX_BUFFER_LENGTH] = {0};
static unsigned char nUSART_RX_Idx =  0;
static unsigned char nNewDataFlag = 0;


static unsigned char USART_TX_Buff[USART_TX_BUFFER_LENGTH] = {0};
static unsigned char nUSART_Tx_Idx = 0;
static unsigned char nUSART_Tx_TransmissionRequest = 0;
static unsigned char nUSART_Tx_TransmissionLength = 0;

/* USART RX ISR
 * v0.1
 *    - if a new byte is received from USART ... then we move-it in the RX buffer and we 
 *       update the "new data" flag !
 */
ISR(USART1_RX_vect)
{
  /* Clear Rx complete flag by writting 1 ... */
  //UCSR1A |= (1 << RXC1);

  USART_RX_Buff[nUSART_RX_Idx] = UDR1;

  if( nUSART_RX_Idx < (USART_RX_BUFFER_LENGTH - 1) )
  {
    nUSART_RX_Idx ++;
  }

  nNewDataFlag = 1;
}


/* Init function of the USART driver
 * v0.1
 *
 *  - configure Baud rate and enable Rx ISR 
 *
 */
void USART_Init(void)
{

   /* 
    * Clear error flags ...
    */

    /*  U2Xn: Double the USART Transmission Speed
     *
     */
    UCSR1A |= (1 << U2X1);

    /*  RXCIEn: RX Complete Interrupt Enable - Enable Rx interrupt generation
     *  RXENn: Receiver Enable - Enable USART Tx and Rx
     *  TXENn: Transmitter Enable
     */
    UCSR1B |= (1 << RXCIE1) | (1 << RXEN1) | (1 << TXEN1);

    /* Usart working mode: asynchronous, parity disabled, 1 stop bit, 8 bit length
     *
     */
    UCSR1C |= (1 << UCSZ11) | (1 << UCSZ10);

    /* Usart speed - 9600
     *   
     */
    UBRR1H = 0x00;
    //UBRR1L = 0xB9;
    UBRR1L = 0xCF;

    nNewDataFlag = 0;
}


/* 
 *   Transmit function ... 
 *   v1.0
 *   
 *   - transmit a number of characters on specific uart stream 
 *   - update error counters if any errors detected
 */

unsigned char USART_Transmit_Bytes(unsigned char* p_tx_buff, unsigned char nLen)
{
  unsigned char nRetCharCopy = 0;

  if( (p_tx_buff != NULL) && (nLen > 0) )
  {

    if( nLen >= USART_TX_BUFFER_LENGTH )
    {
      memcpy((unsigned char *)USART_TX_Buff, (unsigned char *)p_tx_buff, USART_TX_BUFFER_LENGTH);

      nUSART_Tx_TransmissionLength = USART_TX_BUFFER_LENGTH;
    }
    else
    {
      memcpy((unsigned char *)USART_TX_Buff, (unsigned char *)p_tx_buff, nLen);

      nUSART_Tx_TransmissionLength = nLen;
    }

    nUSART_Tx_TransmissionRequest = 1;
    nUSART_Tx_Idx = 0;
    nRetCharCopy = nUSART_Tx_TransmissionLength;
  }
  
  return nRetCharCopy;  
}

/*
 * - This API is used to return if new data has been received from USART since last call !
 *  v0.1
 *
 *   The NEW data flag is reset in this function ! So the new data is detected only between 2 function calls.
 *      - returns 1 if new data is available
 *      - returns 0 if no data is available since last call !
 */
unsigned char USART_Has_Received_Data(void)
{
  unsigned char nReturn = 0;
  
  if(nNewDataFlag)
  {
    nReturn = 1;
  }  
  
  nNewDataFlag = 0;

  return nReturn;

}


/*
 *   Move from local buffer to user buffer ...
 *   v1.0
 *
 *   - receive characters from specific uart stream
 *
 */

unsigned char USART_Receive_Bytes(unsigned char *ptrUserRxBuff, unsigned short* nIdxWrUser, unsigned short nMaxUserBuffSize)
{


  int nReturn = 0;

  /* Serial.write(USART_RX_Buff, nUSART_RX_Idx); */

  if(ptrUserRxBuff != NULL)
  {

    /*  ----- CHECK FOR ANY RX BYTES ----- */
    if (nUSART_RX_Idx > 0)
    {
      
      /* disable USART Rx interrupts ... */
      UCSR1B &= ~(1 << RXCIE1);

      /*
       *     !!! .....   Critical section    .....    !!!
       */ 


      /* we have room in destination buffer for the whole received data ... 
       */
      if( nUSART_RX_Idx < ( nMaxUserBuffSize - *nIdxWrUser) )
      {
        memcpy((unsigned char *)ptrUserRxBuff, (unsigned char *)USART_RX_Buff, nUSART_RX_Idx);

        *nIdxWrUser += nUSART_RX_Idx;

        nReturn = nUSART_RX_Idx;
      }
      else
      {

        /* We don't have room in destination buffer for the whole received data .... only parts will be copied 
         *     ... all the rest will be lost !!! 
         */
        if( nMaxUserBuffSize > *nIdxWrUser )
        {
          nReturn = (nMaxUserBuffSize - *nIdxWrUser - 1);

          if(nReturn > 0)
          {

            *nIdxWrUser += nReturn;

            memcpy((unsigned char *)ptrUserRxBuff, (unsigned char *)USART_RX_Buff, nReturn);
          }
        }
        else
        {
          /* don't copy anyhing ... no room to move the bytes */
        }
      }
      


      /*
       *  Reset USART receive buffer index ... start from the beginning
       */
      nUSART_RX_Idx = 0;


      /* re-enable USART Rx interrupts ... */
      UCSR1B |= (1 << RXCIE1);


      /*
       *   !!!!   .....    Critical section END     .....     !!!!
       */

    } /*  if (nUSART_RX_Idx > 0) */
 
  } /* if(ptrUserRxBuff != NULL) */


   return nReturn;
}


/* USART cyclic function
 * v0.1
 *
 *   - function used mainly to ensure Transmission of data 
 */
void USART_main(void)
{

    /* check transmission request */
    if( nUSART_Tx_TransmissionRequest )
    {
      if( UCSR1A & 0x20 )
      {
        UDR1 = USART_TX_Buff[ nUSART_Tx_Idx ];

        nUSART_Tx_Idx ++;
        nUSART_Tx_TransmissionLength --;        
      }
    }

    if(nUSART_Tx_TransmissionLength <= 0)
    {
      nUSART_Tx_TransmissionRequest = 0;
    }
}
