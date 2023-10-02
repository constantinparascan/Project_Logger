#ifndef __USART_CLIENT_CFG_H__
#define __USART_CLIENT_CFG_H__

/*
  Docu:

  https://www.arduino.cc/reference/en/language/functions/communication/serial/

  */


/* enable debug printing informations ...*/
/* #define DEBUG_PRINT_ENABLE  */
/* #define DEBUG_EMULATE_HW    */

#define USART_RX_BUFFER_LENGTH (100)   /* should be less than 255 !!! as index is unsigned char !!! */

#define USART_TX_BUFFER_LENGTH (30)    /* should be less than 255 !!! as index is unsigned char !!! */

#endif