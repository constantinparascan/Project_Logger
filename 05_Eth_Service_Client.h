/*
 *  C.Parascan
 *  module version 1.0 - reviewed and tested on 20.02.2023
 */

#ifndef __ETH_SERVICE_CLIENT__
#define __ETH_SERVICE_CLIENT__

#include <Arduino.h>


/*
 * Static module version
 */
#define _ETH_SERVICE_CLIENT_VERSION_MAJOR_ (1)
#define _ETH_SERVICE_CLIENT_VERSION_MINOR_ (0)
#define _ETH_SERVICE_CLIENT_VERSION_PATCH_ (0)


/*
 * Enables/diasables serial debugging of the module code ...
 */
#define ETH_SERVICE_CLIENT_DEBUG_SERIAL   (0)


/*
 * Internal states of Eth service client 
 */
typedef enum TAG_ETH_SERVICE_STATE
{
    E_ETH_UNININT = 0,
    E_ETH_INIT_WAIT,
    E_ETH_CONNECT_TO_SERVER,
    E_ETH_WAIT_COMMAND,
    
    E_ETH_SERVER_TRANSMISSION,
    E_ETH_SERVER_RECEPTION,
    E_ETH_SERVER_RECEPTION_FINISHED,

    E_ETH_ERROR_RECOVERABLE_BY_RESET,
    E_ETH_ERROR_UNRECOVERABLE
}T_ETH_State;

/* exported API's */
void Eth_Service_Client_init(void);
void Eth_Service_Client_main(void);

unsigned char Eth_Service_Client_AddTxData(char* strData);
void Eth_Service_Client_StackBuffRequest(void);
void Eth_Service_Client_StackNULLRequest(void);

unsigned char Eth_Service_Client_SendRequest(void);
unsigned char Eth_Service_Client_IsReadyForNewCommand(void);

unsigned short Eth_Service_Client_Get_RxBuffer(char **RxBuff);
void Eth_Service_Client_ClearTxBuff(void);

#endif /* __ETH_SERVICE_CLIENT__ */