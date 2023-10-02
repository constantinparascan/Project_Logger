#ifndef __COM_SERVICE_H__
#define __COM_SERVICE_H__


#define COM_SERVICE_DEBUG_ENABLE (1)


/* Define communication mode of the board ... */
/*
 *    1 - Ethernet - W5100
 *    2 - SIMx00 - GPRS
 *    3 - Ethenet + GPRS
 */
#define COM_MODE_ETH     (0x01) 
#define COM_MODE_SIM     (0x02)
#define COM_MODE_ETH_SIM (0x03)
#define COM_SERVICE_COMMUNICATION_MODE (0x02)



typedef enum TAG_ComService_State
{
  COM_SERVICE_STATE_STARTUP = 0,
  COM_SERVICE_STATE_QUERY_SERVER,
  COM_SERVICE_STATE_CONNECTED_TO_SERVER_STARTUP,
  COM_SERVICE_STATE_CONNECTED_TO_SERVER_STARTUP_WAIT,
  COM_SERVICE_STATE_FULL_COM,
  COM_SERVICE_STATE_NO_COM_ERROR

}T_ComService_State;


/*
 * Exported API's
 */
void Com_Service_Init(void);
void Com_Service_main_Eth(void);
void Com_Service_main_AT(void);

unsigned char Com_Service_Client_Request_BillPayment_Generic_Transmission(unsigned char nType);
unsigned char Com_Service_Client_Request_BillPayment_Generic_Transmission_Bill_v2(unsigned short nChan1, unsigned short nChan2, \
                                                                                  unsigned short nChan3, unsigned short nChan4, \
                                                                                  unsigned char  nLastBillType);

unsigned char Com_Service_Client_Request_BillPayment_Generic_Transmission_Bill_v3(unsigned short nChan1, unsigned short nChan2, \
                                                                                  unsigned short nChan3, unsigned short nChan4, \
                                                                                  unsigned char  nLastBillType, unsigned char nFrameType, unsigned char nErrorVal);
                                                                                  
unsigned char Com_Service_Client_Is_Comm_Free(void);

void AT_Command_Callback_Notify_Init_Finished(void);
void AT_Command_Callback_Request_Bill_Status_Transmission(void);

#endif /* __COM_SERVICE_H__ */