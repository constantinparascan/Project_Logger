/* C.Parascan 
 * V0.1 - 05.06.2023 - first version
 * V0.2 - 17.10.2023 - minor adaptations and bug-fixing
 *
 *
 * - Com is an intermediate layer interposing between communication drivers ( SIM900 - GPRS / Ethernet W5100 )
 * - It's main porpuse is to handle "fire and forget" requests from application (above) layer so the design of applications can be simplified
 * - This layer is checking if the connection with server is established and only after is accepting requests from application
 *
 * - Frame format is standardized and does NOT change depending on communication type - is bound by the server application
 * - A frame example that is transmitted TO server: 
 *              http://145.239.84.165:35269/gsm/entry_new.php?x=220131329877;1111111111111111111;0726734732;RDS;256;256;256;256;A;0;0;No_approver;BUC_TEST_3;TEST_3;UNDER_TEST_3;NV10;1
 *                                                                  \IMEI            \SIM SN        \ SIM NR  \  \   \   \   \   \
 *                                                                                                             \  CH1 ...  CH4    Frame type 
 *                                                                                                            SIM           \
 *                                                                                                            opperator      \ This is where the money are kept
 *                                                                                                                              CH1 * 1 RON value
 *                                                                                                                              CH2 * 5 RON value
 *                                                                                                                              CH3 * 10 RON value
 *                                                                                                                              CH4 * 50 RON value
 *                                                                                                                              
 * - Server does respond with a string char (max 63 chars) which looks like: 
 *                 CMDAServer is alive***********************************************    "A" -> Alive
 *                 CMDTDEBUG_CITY****************************************************    "T" -> request to update Town name to "DEBUG_CITY"
 *                 CMDPDEBUG_LOCATION************************************************    "P" -> request to update Location name to "DEBUG_LOCATION"
 *                 CMDDDEBUG_DETAILS*************************************************    "D" -> request to update Details name to "DEBUG_DETAILS"
 *                 CMDN1234567890****************************************************    "N" -> request to update SIM NR name to "1234567890"
 *                 CMDVNV10**********************************************************    "V" -> request to update Validator name to "NV10" 
 *                 CMDB**************************************************************    "B" -> reset monetary --> CH1 = 0, CH2 = 0, CH3 = 0, CH4 = 0
 *
 *
 * - Communication between application layer and current COM layer is done via "callback" functions specified below. They act on "nCallbackFlags" 
 *            handeled by the main function that are performing various communication actions.
 */
#include "01_Debug_Utils.h"
#include "03_Com_Service.h"
#include "03_Com_Service_cfg.h"
#include "99_Automat_Board_cfg.h"
#include "99_Automat_Board_Config.h"
#include "60_GSM_SIM900.h"

#include "50_Logger_App_Parallel.h"
#include "50_Logger_App.h"



#define COM_SERV_CONN_STAT_OFFLINE (0)
#define COM_SERV_CONN_STAT_QUERRY_IN_PROGRESS (1)
#define COM_SERV_CONN_STAT_WAIT_RESPONSE (2)
#define COM_SERV_CONN_STAT_ONLINE_FREE (250)
#define COM_SERV_CONN_STAT_ONLINE_BUSY (255)

#define COM_SERV_COM_MESSAGE_MONITORING_DEADLINE_TIMER   (3000)    /* 3000 * 5ms = 15 sec */

/*
 *  Internal management 
 *
 */
typedef struct TAG_ComService_Internals
{
  unsigned char  nEthConnectionStatus;
  unsigned short nEthWaitResponseCounter;
  
  unsigned char  nATConnectionStatus;
  unsigned short nATWaitResponseCounter;

  unsigned char nCom_Service_Request_Message_Transmission;
  unsigned char nCom_Service_Appl_Message_Type;

  unsigned char nCom_Service_Response_Command_Type;
}T_ComService_Internals;



T_ComService_State eComServiceState;

T_ComService_Internals sComServiceInternals;


/* ***************************************************************
 *  Appl Logger specific flags
 *
 */
unsigned char  nFlags_Com_Service_StartupMessageTransmitted = 0;
unsigned short nFlags_Com_Service_ComMonitoring_MessageTx_DeadlineTimer = COM_SERV_COM_MESSAGE_MONITORING_DEADLINE_TIMER;
unsigned char  nFlags_Com_Service_ComMonitoring_Active = 0;

/*
 * Appl Logger specific delays
 */

/* delay used for the "PING" message */
unsigned short nCom_Service_PingToServer_Delay = LOG_APP_PING_TO_SERVER_DELAY;

unsigned short nCom_Service_Get_Signal_Strength_Delay = COM_SERVICE_MAX_WAIT_ITTER_SIGNAL_STRENGTH_DELAY;

unsigned short nCom_Service_Delay_Before_Start_Req = COM_SERVICE_MAX_WAIT_ITTER_BEFORE_START_REQ;

/* used to keep the HW configuration of the board ... */
unsigned char  nCom_HW_Config_TYPE_Local = 0;


/* ************************************************************** */


/*
 * Private API's
 */


/*
 * Callback functions ...
 *
 */
#define COM_FLAGS_CALLBACK_AT_INIT_FINISH             (0x01)
#define COM_FLAGS_CALLBACK_AT_COM_ERROR_REINIT        (0x02)
#define COM_FLAGS_CALLBACK_AT_COM_NO_ERROR_RECOVERY   (0x04)

#define COM_FLAGS_CALLBACK_AT_COM_TRANSMIT_DATA_NORMAL     (0x10)
#define COM_FLAGS_CALLBACK_AT_COM_TRANSMIT_ERROR_FRAME     (0x20)
#define COM_FLAGS_CALLBACK_AT_COM_SERVER_RESPONSE_RECEIVED (0x80)

unsigned char nCallbackFlags = 0x00;


/*
 * Function called after the SIM900 has been initialized and is ready for communication
 */
void AT_Command_Callback_Notify_Init_Finished(void)
{
  nCallbackFlags |= COM_FLAGS_CALLBACK_AT_INIT_FINISH;


  /* reset any previous error flags ... */
  nCallbackFlags &= ~COM_FLAGS_CALLBACK_AT_COM_ERROR_REINIT;
}


/*
 * Function called each time an BILL transmission to server is requested !
 */
void AT_Command_Callback_Request_Bill_Status_Transmission(void)
{
  nCallbackFlags |= COM_FLAGS_CALLBACK_AT_COM_TRANSMIT_DATA_NORMAL;
}

/*
 * Function called each time an ERROR transmission to server is requested !
 */

void AT_Command_Callback_Request_ERROR_Status_Transmission(void)
{
  nCallbackFlags |= COM_FLAGS_CALLBACK_AT_COM_TRANSMIT_ERROR_FRAME;
}

/*
 * Error in communicatation with the GSM provider ... re-initialization 
 *    of the SIM driver has been started ...
 */
void AT_Command_Callback_Notify_No_Communication_Error_Re_Init(void)
{
  nCallbackFlags |= COM_FLAGS_CALLBACK_AT_COM_ERROR_REINIT;
}

void AT_Command_Callback_Notify_Server_Response_Received(void)
{
  nCallbackFlags |= COM_FLAGS_CALLBACK_AT_COM_SERVER_RESPONSE_RECEIVED;
}


/* V0.1
 *
 *    Initialize the Communication Service ...
 *
 */
void Com_Service_Init(void)
{
  #if(COM_SERVICE_DEBUG_ENABLE == 1)
    Serial.println("[I] Com_Serv Debug ...");
  #endif

  /* startup state ... */
  eComServiceState = COM_SERVICE_STATE_STARTUP;

  /* initialize Ethernet module communication ... */
  if( COM_SERVICE_COMMUNICATION_MODE & COM_MODE_ETH )
  {
    /* Eth_Service_Client_init(); */
  }

  /* initialize SIMx00 module communication ... */
  if( COM_SERVICE_COMMUNICATION_MODE & COM_MODE_SIM )
  {
    AT_Command_Processor_Init();
  }

  sComServiceInternals.nEthConnectionStatus = COM_SERV_CONN_STAT_OFFLINE;
  sComServiceInternals.nATConnectionStatus  = COM_SERV_CONN_STAT_OFFLINE;  

  /*
   * Make sure that the Com init function is called before the HW configuration is read-out ... 
   */
   nCom_HW_Config_TYPE_Local = Board_Config_Get_HW_ConfigType();

   /*  ensure no flags requests are set ... 
    */
   nCallbackFlags = 0x00;

   /* 
    * init Delay counters ... 
    */
   nCom_Service_Get_Signal_Strength_Delay = COM_SERVICE_MAX_WAIT_ITTER_SIGNAL_STRENGTH_DELAY;
   nCom_Service_Delay_Before_Start_Req = COM_SERVICE_MAX_WAIT_ITTER_BEFORE_START_REQ;

   nFlags_Com_Service_ComMonitoring_MessageTx_DeadlineTimer = COM_SERV_COM_MESSAGE_MONITORING_DEADLINE_TIMER;
   nFlags_Com_Service_ComMonitoring_Active = 0;

}/* end function Com_Service_Init */


/* V0.1
 *  Cyclic function of communication Service
 *
 */
void Com_Service_main(void)
{
  /* temporary keep retrieved values for further processing ...  */
  unsigned short nTempCh1 = 0;
  unsigned short nTempCh2 = 0;
  unsigned short nTempCh3 = 0;
  unsigned short nTempCh4 = 0;  
  unsigned char  nLastBillTemp = 0;
  unsigned char  nLastErrorTemp = 0;
  unsigned char  nTempRSSI = 0;
  unsigned char  nStatus = 0;
  unsigned char  nCommandFromServer = 0;

  switch( eComServiceState )
  {

    case COM_SERVICE_STATE_STARTUP:
    {
      /*
       *  ... initialize time-outs ... errors ... 
       */

       eComServiceState = COM_SERVICE_STATE_QUERY_SERVER;

      #if(COM_SERVICE_DEBUG_ENABLE == 1)
        Serial.println("[I] Com_Serv State: COM_SERVICE_STATE_QUERY_SERVER ...");
      #endif       
    }    
    break;

    /*
     * init make sure all communication paths are working
     */
    case COM_SERVICE_STATE_QUERY_SERVER:
    {

      /*
       * Has been a server connection established ?
       */
      if( nCallbackFlags & COM_FLAGS_CALLBACK_AT_INIT_FINISH ) 
      {

        /*
         * Prepare to send Startup command ... and advance state ...
         */

        #if(COM_SERVICE_DEBUG_ENABLE == 1)
          Serial.println("[I] Com_Serv State: COM_SERVICE_STATE_CHECK_SIGNAL_STRENGTH ...");
        #endif

        /* reset init state flag ... */
        nCallbackFlags &= ~COM_FLAGS_CALLBACK_AT_INIT_FINISH;

        eComServiceState = COM_SERVICE_STATE_CHECK_SIGNAL_STRENGTH;
      }
    }
    break;


    /*
     * Wait for the RSSI signal strength to be extracted by the SIM900 driver
     *  ... for an limited amount of time
     *  ... otherwise just report-it with value "0"
     */
    case COM_SERVICE_STATE_CHECK_SIGNAL_STRENGTH:
    {
      if( ( nCom_Service_Get_Signal_Strength_Delay > 0 ) && ( AT_Command_Is_New_RSSI_Computed() == 0 ) )
      {
        nCom_Service_Get_Signal_Strength_Delay --;
      }
      else
      {
        eComServiceState = COM_SERVICE_STATE_CHECK_SIGNAL_STRENGTH_WAIT;

        #if(COM_SERVICE_DEBUG_ENABLE == 1)
          Serial.println("[I] Com_Serv State: COM_SERVICE_STATE_CHECK_SIGNAL_STRENGTH_WAIT ...");
        #endif
      }

      break;
    }

    /*
     * After requiring the signal strength from SIM900 ... implement a short delay ...
     */
    case COM_SERVICE_STATE_CHECK_SIGNAL_STRENGTH_WAIT:
    {
      if(nCom_Service_Delay_Before_Start_Req > 0)
      {
        nCom_Service_Delay_Before_Start_Req --;
      }
      else
      {

        /* 
         * if previously a "start" message has been transmitted ... don't repeat transmission !
         */
        if( nFlags_Com_Service_StartupMessageTransmitted > 0)
        {
          eComServiceState = COM_SERVICE_STATE_FULL_COM;

          /* reset message response monitoring --- no message to be monitored yet ...  */
          nFlags_Com_Service_ComMonitoring_Active = 0;

          #if(COM_SERVICE_DEBUG_ENABLE == 1)
            Serial.println("[I] Com_Serv State: COM_SERVICE_STATE_FULL_COM ...");
          #endif
        }
        else
        {
          eComServiceState = COM_SERVICE_STATE_CONNECTED_TO_SERVER_STARTUP;

          #if(COM_SERVICE_DEBUG_ENABLE == 1)
            Serial.println("[I] Com_Serv State: COM_SERVICE_STATE_CONNECTED_TO_SERVER_STARTUP ...");
          #endif
        }
      }

      break;
    }

    /*
     * make sure "START" frame is transmitted ... 
     */
    case COM_SERVICE_STATE_CONNECTED_TO_SERVER_STARTUP:
    {

      /* 
       * Collect current status of channel (monetary) values ... and send them into a "Start" frame.
       */      
      if( nCom_HW_Config_TYPE_Local == AUTOMAT_BOARD_CONFIG_HW_TYPE_PARALLEL )
      {
        nTempCh1 = Logger_App_Parallel_GetChannelValue( 0 );
        nTempCh2 = Logger_App_Parallel_GetChannelValue( 1 );
        nTempCh3 = Logger_App_Parallel_GetChannelValue( 2 );
        nTempCh4 = Logger_App_Parallel_GetChannelValue( 3 );
      }
      else
        if( nCom_HW_Config_TYPE_Local == AUTOMAT_BOARD_CONFIG_HW_TYPE_CCTALK )
        {
          nTempCh1 = Logger_App_GetChannelValue( 1 );
          nTempCh2 = Logger_App_GetChannelValue( 2 );
          nTempCh3 = Logger_App_GetChannelValue( 3 );
          nTempCh4 = Logger_App_GetChannelValue( 4 );
        } 
      
      nTempRSSI = AT_Command_GET_RSSI_Level();

      nStatus = Com_Service_Client_Request_BillPayment_Generic_Transmission_Bill_v3(nTempCh1, nTempCh2, nTempCh3, nTempCh4, 0, 'S', 0, nTempRSSI );
      
      /* go to next state only if there has been a successful start command accepted by the lower layers ... */
      if( nStatus > 0 )
      {
        eComServiceState = COM_SERVICE_STATE_CONNECTED_TO_SERVER_STARTUP_WAIT;

        #if(COM_SERVICE_DEBUG_ENABLE == 1)
          Serial.println("[I] Com_Serv State: Sent START command ... goto COM_SERVICE_STATE_CONNECTED_TO_SERVER_STARTUP_WAIT ...");
        #endif

        /*
         * indicate that the "Start-up" message has been transmitted to SIM900 ... until another reset/power-on no additional
         *   startup messages will be transmitted !
         */
        nFlags_Com_Service_StartupMessageTransmitted ++;

      }

      break;
    }

    /*
     * wait until the AT status becomes available ... 
     */
    case COM_SERVICE_STATE_CONNECTED_TO_SERVER_STARTUP_WAIT:
    {

      /* 
       * Got to "Full Com" state only after SIM900 driver has reached iddle state ...
       */
      if( AT_Command_Processor_Is_Iddle() > 0 )
      {      
        eComServiceState = COM_SERVICE_STATE_FULL_COM;

        /* reset message response monitoring --- no message to be monitored yet ...  */
        nFlags_Com_Service_ComMonitoring_Active = 0;

        #if(COM_SERVICE_DEBUG_ENABLE == 1)
          Serial.println("[I] Com_Serv State: COM_SERVICE_STATE_FULL_COM ...");
        #endif
      }

      break;    
    }


    /*
     *
     *
     *
     *   ToDo !!!! ------------ send automatic ACK - "A" ... command to server in order not to block any pending commands that we missed in the past !!!! !!! 
     *
     *
     *
     */
     

    case COM_SERVICE_STATE_FULL_COM:
    {

      /*
       * Message monitoring error handling ...
       */
      if( nFlags_Com_Service_ComMonitoring_Active > 0 )
      {
        /*
         * there has been a response for the previous transmitterd message ? !
         */
        if( nCallbackFlags & COM_FLAGS_CALLBACK_AT_COM_SERVER_RESPONSE_RECEIVED ) 
        {
          nCallbackFlags &= ~COM_FLAGS_CALLBACK_AT_COM_SERVER_RESPONSE_RECEIVED;

          nFlags_Com_Service_ComMonitoring_MessageTx_DeadlineTimer = COM_SERV_COM_MESSAGE_MONITORING_DEADLINE_TIMER;

          /* stop the monitoring ... monitored message has been received ... */
          nFlags_Com_Service_ComMonitoring_Active = 0;
        }
        else
        {
          if( nFlags_Com_Service_ComMonitoring_MessageTx_DeadlineTimer > 0 ) 
          {
            nFlags_Com_Service_ComMonitoring_MessageTx_DeadlineTimer --;
          }


          /* deadline monitoring has detected an error in communication ... messages not responded by the server !!!! */
          if( nFlags_Com_Service_ComMonitoring_MessageTx_DeadlineTimer == 0 )
          {
            /* !!!! ERROR !!!! ... restart of communication required !!! */

            AT_Command_Request_Reset_SIM();

            /* after reset ... stop the monitorig until restart ... */
            nFlags_Com_Service_ComMonitoring_Active = 0;
            nFlags_Com_Service_ComMonitoring_MessageTx_DeadlineTimer = COM_SERV_COM_MESSAGE_MONITORING_DEADLINE_TIMER;

            #if(COM_SERVICE_DEBUG_ENABLE == 1)
              Serial.println("[I] Com_Serv ERROR message Monitoring !!!! --> Request SIM900 RESET !!!! ");
            #endif

          }/* end if( nFlags_Com_Service_ComMonitoring_MessageTx_DeadlineTimer == 0 ) */

        }/* end else if( nCallbackFlags & COM_FLAGS_CALLBACK_AT_COM_SERVER_RESPONSE_RECEIVED ) */

      }/* if( nFlags_Com_Service_ComMonitoring_Active > 0 ) */




      /* *********************************
       *
       * SEND a normal frame ... 
       *
       * ********************************* */
      if( nCallbackFlags & COM_FLAGS_CALLBACK_AT_COM_TRANSMIT_DATA_NORMAL )       
      {

        if( nCom_HW_Config_TYPE_Local == AUTOMAT_BOARD_CONFIG_HW_TYPE_PARALLEL )
        {
          nTempCh1 = Logger_App_Parallel_GetChannelValue( 0 );
          nTempCh2 = Logger_App_Parallel_GetChannelValue( 1 );
          nTempCh3 = Logger_App_Parallel_GetChannelValue( 2 );
          nTempCh4 = Logger_App_Parallel_GetChannelValue( 3 );
          nLastBillTemp = Logger_App_Parallel_GetLastBillType();
        }
        else
          if( nCom_HW_Config_TYPE_Local == AUTOMAT_BOARD_CONFIG_HW_TYPE_CCTALK )
          {
            nTempCh1 = Logger_App_GetChannelValue( 1 );
            nTempCh2 = Logger_App_GetChannelValue( 2 );
            nTempCh3 = Logger_App_GetChannelValue( 3 );
            nTempCh4 = Logger_App_GetChannelValue( 4 );
            nLastBillTemp = Logger_App_Get_LastBillType();
          }

        nTempRSSI = AT_Command_GET_RSSI_Level();

        nStatus = Com_Service_Client_Request_BillPayment_Generic_Transmission_Bill_v2(nTempCh1, nTempCh2, nTempCh3, nTempCh4, nLastBillTemp, nTempRSSI);

        /* if the COM layer has successfully passed the request downstream to AT layer ... then we erase the request bit, otherwise try again next time ... */      
        if( nStatus > 0 )
        {

          nCallbackFlags &= ~COM_FLAGS_CALLBACK_AT_COM_TRANSMIT_DATA_NORMAL;
          nCallbackFlags &= ~COM_FLAGS_CALLBACK_AT_COM_SERVER_RESPONSE_RECEIVED;

          /*
          * restart response monitoring timer ... 
          */
          nFlags_Com_Service_ComMonitoring_MessageTx_DeadlineTimer = COM_SERV_COM_MESSAGE_MONITORING_DEADLINE_TIMER;
          nFlags_Com_Service_ComMonitoring_Active ++;

          #if(COM_SERVICE_DEBUG_ENABLE == 1)
            Serial.println("[I] Com_Serv State: COM_SERVICE_STATE_FULL_COM --> Send DATA Req ");
          #endif

        }

      }/* end if( nCallbackFlags & COM_FLAGS_CALLBACK_AT_COM_TRANSMIT_DATA_NORMAL )  ... */



      /* ****************************
       *
       * SEND an ERROR frame ... 
       *
       * **************************** */
      if( nCallbackFlags & COM_FLAGS_CALLBACK_AT_COM_TRANSMIT_ERROR_FRAME )       
      {

        /*
         * PARALLEL has no ERROR transmission frames ... YET ... only CCTALK
         */

        if( nCom_HW_Config_TYPE_Local == AUTOMAT_BOARD_CONFIG_HW_TYPE_CCTALK )
        {
          nTempCh1 = Logger_App_GetChannelValue( 1 );
          nTempCh2 = Logger_App_GetChannelValue( 2 );
          nTempCh3 = Logger_App_GetChannelValue( 3 );
          nTempCh4 = Logger_App_GetChannelValue( 4 );
          nLastBillTemp = Logger_App_Get_LastBillType();
          nLastErrorTemp = Logger_App_Get_LastErrorValue();

          nTempRSSI = AT_Command_GET_RSSI_Level();

          nStatus = Com_Service_Client_Request_BillPayment_Generic_Transmission_Bill_v3(nTempCh1, nTempCh2, nTempCh3, nTempCh4, nLastBillTemp, 'E',  nLastErrorTemp, nTempRSSI);
          //nStatus = 1;
      
          if( nStatus > 0 )
          {

            nCallbackFlags &= ~COM_FLAGS_CALLBACK_AT_COM_TRANSMIT_ERROR_FRAME;
            nCallbackFlags &= ~COM_FLAGS_CALLBACK_AT_COM_SERVER_RESPONSE_RECEIVED;

            /*
            * restart response monitoring timer ... 
            */
            nFlags_Com_Service_ComMonitoring_MessageTx_DeadlineTimer = COM_SERV_COM_MESSAGE_MONITORING_DEADLINE_TIMER;
            nFlags_Com_Service_ComMonitoring_Active ++;


            #if(COM_SERVICE_DEBUG_ENABLE == 1)
              Serial.println("[I] Com_Serv State: COM_SERVICE_STATE_FULL_COM --> Send ERROR Req ");
            #endif

          }

        }

      }/* end if( nCallbackFlags & COM_FLAGS_CALLBACK_AT_COM_TRANSMIT_ERROR_FRAME )  ... */


      /* **************************************
       *
       * Monitor PING - send PING frame 
       *    
       * ************************************* */
      if( nCom_Service_PingToServer_Delay == 0 )
      {
        /*
         * Send PING Frame ...  
         *
         */

        if( nCom_HW_Config_TYPE_Local == AUTOMAT_BOARD_CONFIG_HW_TYPE_PARALLEL )
        {
          nTempCh1 = Logger_App_Parallel_GetChannelValue( 0 );
          nTempCh2 = Logger_App_Parallel_GetChannelValue( 1 );
          nTempCh3 = Logger_App_Parallel_GetChannelValue( 2 );
          nTempCh4 = Logger_App_Parallel_GetChannelValue( 3 );
        }
        else
          if( nCom_HW_Config_TYPE_Local == AUTOMAT_BOARD_CONFIG_HW_TYPE_CCTALK )
          {
            nTempCh1 = Logger_App_GetChannelValue( 1 );
            nTempCh2 = Logger_App_GetChannelValue( 2 );
            nTempCh3 = Logger_App_GetChannelValue( 3 );
            nTempCh4 = Logger_App_GetChannelValue( 4 );
          }        

        nTempRSSI = AT_Command_GET_RSSI_Level();

        nStatus = Com_Service_Client_Request_BillPayment_Generic_Transmission_Bill_v3(nTempCh1, nTempCh2, nTempCh3, nTempCh4, 0, 'P', 0 , nTempRSSI);
        //nStatus = 1;  // for debug porpuses ...
      
        /* if the COM layer has successfully passed the request downstream to AT layer ... then we erase the request bit, otherwise try again next time ... */      
        if( nStatus > 0 )
        {
          /*
           * Reset PING delay ...
           */         
          nCom_Service_PingToServer_Delay = LOG_APP_PING_TO_SERVER_DELAY;


          /* reset previous monitoring flag result ... */
          nCallbackFlags &= ~COM_FLAGS_CALLBACK_AT_COM_SERVER_RESPONSE_RECEIVED;

          /*
          * restart response monitoring timer ... 
          */
          nFlags_Com_Service_ComMonitoring_MessageTx_DeadlineTimer = COM_SERV_COM_MESSAGE_MONITORING_DEADLINE_TIMER;
          nFlags_Com_Service_ComMonitoring_Active ++;

        }


        #if(COM_SERVICE_DEBUG_ENABLE == 1)
          Serial.println("[I] Com_Serv State: COM_SERVICE_STATE_FULL_COM --> Send PING Req ");
        #endif


      }/* End PING !!  */



      /* ****************************************************
       *
       *   Check if any new commands from Server !!! ... 
       *       
       * **************************************************** */
      if( AT_Command_Is_New_Command_From_Server() != 0 )
      {
        nCommandFromServer = AT_Command_Read_Last_Command_From_Server();

        /* RESET Monetary command ? */
        if( nCommandFromServer == 'B' )
        {
          if( nCom_HW_Config_TYPE_Local == AUTOMAT_BOARD_CONFIG_HW_TYPE_PARALLEL )
          {
            Logger_App_Parallel_Callback_Command_Reset_Monetary();
         
            nTempCh1 = Logger_App_Parallel_GetChannelValue( 0 );
            nTempCh2 = Logger_App_Parallel_GetChannelValue( 1 );
            nTempCh3 = Logger_App_Parallel_GetChannelValue( 2 );
            nTempCh4 = Logger_App_Parallel_GetChannelValue( 3 );
          }
          else
            if( nCom_HW_Config_TYPE_Local == AUTOMAT_BOARD_CONFIG_HW_TYPE_CCTALK )
            {
              Logger_App_Callback_Command_Reset_Monetary();

              nTempCh1 = Logger_App_GetChannelValue( 1 );
              nTempCh2 = Logger_App_GetChannelValue( 2 );
              nTempCh3 = Logger_App_GetChannelValue( 3 );
              nTempCh4 = Logger_App_GetChannelValue( 4 );
            }

        }/* end if(nCommandFromServer == 'B') */
        else
          if(nCommandFromServer == 'N')
          {
            strcpy( Logger_App_GetPhoneNumberString(), AT_Command_Get_Data_From_Last_Command_From_Server() );
            NvM_Write_Block(NVM_BLOCK_CHAN_6_ID);
          }
          else
            if(nCommandFromServer == 'T')
            {
              strcpy( Logger_App_GetTownNameString(), AT_Command_Get_Data_From_Last_Command_From_Server() );
              NvM_Write_Block(NVM_BLOCK_CHAN_7_ID);
            }
            else
              if(nCommandFromServer == 'P')
              {
                strcpy( Logger_App_GetPlaceNameString(), AT_Command_Get_Data_From_Last_Command_From_Server() );
                NvM_Write_Block(NVM_BLOCK_CHAN_8_ID);
              }
              else
                if(nCommandFromServer == 'D')
                {
                  strcpy( Logger_App_GetDetailsNameString(), AT_Command_Get_Data_From_Last_Command_From_Server() );
                  NvM_Write_Block(NVM_BLOCK_CHAN_9_ID);
                }
                else
                  if(nCommandFromServer == 'V')
                  {
                    strcpy( Logger_App_GetDeviceNameString(), AT_Command_Get_Data_From_Last_Command_From_Server() );
                    NvM_Write_Block(NVM_BLOCK_CHAN_10_ID);
                  }

        nTempRSSI = AT_Command_GET_RSSI_Level();

        nStatus = Com_Service_Client_Request_BillPayment_Generic_Transmission_Bill_v3(nTempCh1, nTempCh2, nTempCh3, nTempCh4, 0, 'A', 0, nTempRSSI );

        /* if the transmission request has been accepted ... */
        if( nStatus > 0 )
        {
          /* mark command as processed ... */
          AT_Command_Process_Command_From_Server();

          #if(COM_SERVICE_DEBUG_ENABLE == 1)
            Serial.println("[I] Com_Serv State: COM_SERVICE_STATE_FULL_COM --> RESPONSE Acknowledge for server COMMAND ! ");
          #endif
        }


      }/*  if( AT_Command_Is_New_Command_From_Server() != 0 )  */     




      /*
       * Any erros in communication signaled by SIM driver ?
       */
      if( nCallbackFlags & COM_FLAGS_CALLBACK_AT_COM_ERROR_REINIT )
      {

        nCallbackFlags &= ~COM_FLAGS_CALLBACK_AT_COM_ERROR_REINIT;

        eComServiceState = COM_SERVICE_STATE_NO_COM_ERROR;

        #if(COM_SERVICE_DEBUG_ENABLE == 1)
          Serial.println("[I] Com_Serv State: COM_SERVICE_STATE_NO_COM_ERROR ... No Communication Error ...");
        #endif

      }

      break;
      
    }/* end case COM_SERVICE_STATE_FULL_COM */


    /*
     * in this state we arrived because an communication error with the GSM provider 
     *    has been detected by the SIM900 driver. The driver will re-initialize it's internal state machine
     *    and re-start !!
     *
     * ... we must wait for the initialization flags to be set by the SIM900 driver.
     */
    case COM_SERVICE_STATE_NO_COM_ERROR:
    {

      /*
       * wait in this state until SIM900 is fully initialized !!!
       */
      if( nCallbackFlags & COM_FLAGS_CALLBACK_AT_INIT_FINISH )
      {

        /* reset init state flag ... */
        nCallbackFlags &= ~COM_FLAGS_CALLBACK_AT_INIT_FINISH;

        /* 
         * re-init Delay counters ... 
         */
        nCom_Service_Get_Signal_Strength_Delay = COM_SERVICE_MAX_WAIT_ITTER_SIGNAL_STRENGTH_DELAY;
        nCom_Service_Delay_Before_Start_Req = COM_SERVICE_MAX_WAIT_ITTER_BEFORE_START_REQ;

        eComServiceState = COM_SERVICE_STATE_CHECK_SIGNAL_STRENGTH;

        #if(COM_SERVICE_DEBUG_ENABLE == 1)
          Serial.println("[I] Com_Serv State: COM_SERVICE_STATE_CHECK_SIGNAL_STRENGTH ... comming from NO comm Error recovery ...");
        #endif

      }      
        
      break;
    }

  }/* switch( eComServiceState ) */

}/* void Com_Service_main(void) */



/*
 *
 */
unsigned char Com_Service_Client_Request_BillPayment_Generic_Transmission_Bill_v2(unsigned short nChan1, unsigned short nChan2, \
                                                                                  unsigned short nChan3, unsigned short nChan4, \
                                                                                  unsigned char  nLastBillType, \
                                                                                  unsigned char nRSSI)
{
  char arrLocalBuff[255];
  char arrDummy[50];
  unsigned nReturn = 0;  

/*
  if( eComServiceState != COM_SERVICE_STATE_FULL_COM )
  {
    return nReturn;  
  }  
*/
  strcpy(arrLocalBuff, "AT+HTTPPARA=\"URL\",\"http://" SERVER_ADDRESS_DESTINATION ":" SERVER_ADDRESS_DESTINATION_PORT SERVER_ADDRESS_GET_URL_COMMAND_X LOG_APP_IMEI_CARD ";" LOG_APP_SIM_SN ";" );
  strcat(arrLocalBuff, arrNvM_RAM_Mirror_Str_SimNR);
  strcat(arrLocalBuff, ";" LOG_APP_SIM_OPERATOR ";");

  snprintf(arrDummy, 50,"%d;%d;%d;%d;%d;0;0;", nChan1, nChan2, nChan3, nChan4, nLastBillType);
  strcat(arrLocalBuff, arrDummy);

  strcat(arrLocalBuff, LOG_APP_VERSION_MAJOR );  
  strcat(arrLocalBuff, ";");

  strcat(arrLocalBuff, arrNvM_RAM_Mirror_Str_Town );  
  strcat(arrLocalBuff, ";");

  strcat(arrLocalBuff, arrNvM_RAM_Mirror_Str_Place );  
  strcat(arrLocalBuff, ";");

  strcat(arrLocalBuff, arrNvM_RAM_Mirror_Str_Details );  
  strcat(arrLocalBuff, ";");

  strcat(arrLocalBuff, arrNvM_RAM_Mirror_Str_Device );  
  strcat(arrLocalBuff, ";");

  snprintf( arrDummy, 50,"%d", nRSSI );
  strcat(arrLocalBuff, arrDummy );

  strcat(arrLocalBuff, "\"\r");

  #if(COM_SERVICE_DEBUG_ENABLE == 1)
    Serial.println();
    Serial.print("[I] Com_Serv Com send-> ");
    Serial.println(arrLocalBuff);
  #endif


  /*
   * reset PING interval ...
   */

  nCom_Service_PingToServer_Delay = LOG_APP_PING_TO_SERVER_DELAY;

  nReturn = AT_Command_Processor_Send_URL_Message(arrLocalBuff);

  return nReturn;
}



/*
 *
 */
unsigned char Com_Service_Client_Request_BillPayment_Generic_Transmission_Bill_v3(unsigned short nChan1, unsigned short nChan2, \
                                                                                  unsigned short nChan3, unsigned short nChan4, \
                                                                                  unsigned char  nLastBillType, unsigned char nFrameType, unsigned char nErrorVal, \
                                                                                  unsigned char nRSSI)
{
  char arrLocalBuff[255];
  char arrDummy[50];
  unsigned nReturn = 0;  

/*
  if( eComServiceState != COM_SERVICE_STATE_FULL_COM )
  {
    return nReturn;  
  }  
*/
  //strcpy(arrLocalBuff, "AT+HTTPPARA=\"URL\",\"http://145.239.84.165:35269/gsm/entry_new.php?x=" LOG_APP_IMEI_CARD ";" LOG_APP_SIM_SN ";" LOG_APP_SIM_PHONE_NR ";" LOG_APP_SIM_OPERATOR ";");

  strcpy(arrLocalBuff, "AT+HTTPPARA=\"URL\",\"http://" SERVER_ADDRESS_DESTINATION ":" SERVER_ADDRESS_DESTINATION_PORT SERVER_ADDRESS_GET_URL_COMMAND_X LOG_APP_IMEI_CARD ";" LOG_APP_SIM_SN ";" );
  strcat(arrLocalBuff, arrNvM_RAM_Mirror_Str_SimNR);
  strcat(arrLocalBuff, ";" LOG_APP_SIM_OPERATOR ";");


  snprintf(arrDummy, 50,"%d;%d;%d;%d;", nChan1, nChan2, nChan3, nChan4);
  strcat(arrLocalBuff, arrDummy);


  /*   - 1 byte
                                                   1 = bill of type 1 validated
                                                   2 = bill of type 2 validated
                                                   3 = bill of type 3 validated
                                                   4 = bill of type 4 validated
                                                   S = Start dupa o pana de curent
                                                   A = Confirmare reset monetar
                                                   B = blocat
                                                   P = ping
                                                   O = usa aparatului deschisa
                                                   C = usa aparatului inchisa
                                                   E = error
        - 1 byte   
                                Type of error 0 .. 20

        - 1 byte
                                1 = reset monetar
      
  */ 

  switch(nFrameType)
  {
    /*
     * display last bill type ... no errors
     */
    case 0:
    {
      snprintf( arrDummy, 50,"%d;0;0;", nLastBillType );
    }
    break;

    /*
     * ping ... 
     */
    case 'P':
    {
      snprintf(arrDummy, 50,"P;0;0;");
    }
    break;

    case 'A':
    {
      /* values of CH1, .... already reset previously .... */
      snprintf(arrDummy, 50,"A;0;1;");
    }   
    break;

    case 'S':
    {
      /* values of nCH1, .... already reset previously .... */
      snprintf(arrDummy, 50,"S;0;0;");
    }   
    break;

    /*
     * Error frame 
     */
    case 'E':
    {
      snprintf(arrDummy, 50,"E;%d;0;", nErrorVal);
    }
    break;       
  }

  strcat(arrLocalBuff, arrDummy);



  strcat(arrLocalBuff, LOG_APP_VERSION_MAJOR );  
  strcat(arrLocalBuff, ";");

  //strcat(arrLocalBuff, LOG_APP_TOWN ); 
  strcat(arrLocalBuff, arrNvM_RAM_Mirror_Str_Town);
  strcat(arrLocalBuff, ";");

  //strcat(arrLocalBuff, LOG_APP_PLACE );
  strcat(arrLocalBuff, arrNvM_RAM_Mirror_Str_Place);
  strcat(arrLocalBuff, ";");

  //strcat(arrLocalBuff, LOG_APP_DETAILS );  
  strcat(arrLocalBuff, arrNvM_RAM_Mirror_Str_Details );  
  strcat(arrLocalBuff, ";");

  //strcat(arrLocalBuff, LOG_APP_DEVICE_TYPE );  
  strcat(arrLocalBuff, arrNvM_RAM_Mirror_Str_Device );  
  strcat(arrLocalBuff, ";");

  snprintf( arrDummy, 50,"%d", nRSSI );
  strcat(arrLocalBuff, arrDummy );

  strcat(arrLocalBuff, "\"\r");

  #if(COM_SERVICE_DEBUG_ENABLE == 1)
    Serial.println();
    Serial.print("[I] Com_Serv Com send-> ");
    Serial.println(arrLocalBuff);
  #endif


  /*
   * reset PING interval ...
   */

  nCom_Service_PingToServer_Delay = LOG_APP_PING_TO_SERVER_DELAY;

  nReturn = AT_Command_Processor_Send_URL_Message(arrLocalBuff);

  return nReturn;
}


unsigned char Com_Service_Client_Request_BillPayment_Generic_Transmission(unsigned char nType)
{
  unsigned char nReturn = 0;

  if(eComServiceState == COM_SERVICE_STATE_FULL_COM )
  {
    sComServiceInternals.nCom_Service_Request_Message_Transmission = 1;

    sComServiceInternals.nCom_Service_Appl_Message_Type = nType;

    nReturn = 1;
  }

  return nReturn;
}

unsigned char Com_Service_Client_Is_Comm_Free(void)
{
  unsigned char nReturn = 0;
  
  if( ( eComServiceState == COM_SERVICE_STATE_FULL_COM ) && ( sComServiceInternals.nEthConnectionStatus == COM_SERV_CONN_STAT_ONLINE_FREE ) )
  {
    nReturn = 1;
  }

  return nReturn;

}


/*
 * Used to count the "LOG_APP_PING_TO_SERVER_DELAY" - time until the cash machine sends an alive PING 
 *     message to server !
 */
void Com_Service_cyclic_1sec(void)
{
  /*
   * decrement counter until it reaches "0" - on each seccond itteration
   */
  if( nCom_Service_PingToServer_Delay > 0 )
  {
    nCom_Service_PingToServer_Delay --;
  }

  #if(COM_SERVICE_DEBUG_ENABLE == 1)
    Serial.print("[I] Com_Serv Ping Counter: ");
    Serial.println(nCom_Service_PingToServer_Delay);
  #endif


}

