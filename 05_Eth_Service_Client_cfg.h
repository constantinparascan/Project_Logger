/*
 *  C.Parascan
 *
 *    Static configuration of the ETH Service Client module
 *
 *  module version [see Eth_Service_Client.h] - reviewed and tested on 20.02.2023
 */


#ifndef __ETH_SERVICE_CLIENT_CFG__
#define __ETH_SERVICE_CLIENT_CFG__


/* startup delay in main function cycles ... */
#define ETH_SERVICE_CLIENT_STARTUP_DELAY (200)

/* data buffer where the message to server is built and packed for transmission / reception */
#define ETH_CLIENT_TO_SERVER_BUFFER_LENGTH (250)       /* maximum bytes for Tx buffer */
#define ETH_SERVER_TO_CLIENT_BUFFER_LENGTH (250)       /* maximum bytes for Rx buffer */

#define ETH_SERVER_RECEPTION_MAX_TIMEOUT_WAITING (10)  /* maximum number of itterations that we wait to receive from server a response !!! */

#endif /* __ETH_SERVICE_CLIENT_CFG__ */