#include "03_Com_Service.h"
#include "03_Com_Service_cfg.h"
#include "99_Automat_Board_cfg.h"
#include "05_Eth_Service_Client.h"
#include "60_GSM_SIM900.h"

#include "50_Logger_App_Parallel.h"

#define COM_SERV_CONN_STAT_OFFLINE (0)
#define COM_SERV_CONN_STAT_QUERRY_IN_PROGRESS (1)
#define COM_SERV_CONN_STAT_WAIT_RESPONSE (2)
#define COM_SERV_CONN_STAT_ONLINE_FREE (250)
#define COM_SERV_CONN_STAT_ONLINE_BUSY (255)


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
unsigned char nFlags_Com_Service_StartupMessageTransmitted = 0;

/*
 * Appl Logger specific delays
 */

/* delay used for the "PING" message */
unsigned short nCom_Service_PingToServer_Delay = LOG_APP_PING_TO_SERVER_DELAY;

/* ************************************************************** */


/*
 * Private API's
 */
unsigned char Com_Service_Process_Server_Responses(void);
void Com_Service_Build_Server_Req_Bill_Payment_Generic( unsigned char nFrameType );

unsigned char Com_Service_Process_Server_Response_Service_Availability_ETH(void);
void Com_Service_Build_Server_Request_Service_Availability_ETH(void);


/*
 * Callback functions ...
 *
 */
#define COM_FLAGS_CALLBACK_AT_INIT_FINISH             (0x01)
#define COM_FLAGS_CALLBACK_AT_COM_ERROR               (0x02)
#define COM_FLAGS_CALLBACK_AT_COM_NO_ERROR_RECOVERY   (0x04)

#define COM_FLAGS_CALLBACK_AT_COM_TRANSMIT_DATA_NORMAL (0x10)

unsigned char nCallbackFlags = 0x00;


/*
 * Function called after the SIM900 has been initialized and is ready for communication
 */
void AT_Command_Callback_Notify_Init_Finished(void)
{
  nCallbackFlags |= COM_FLAGS_CALLBACK_AT_INIT_FINISH;
}


/*
 * Function called each time an BILL transmission to server is requested !
 */
void AT_Command_Callback_Request_Bill_Status_Transmission(void)
{
  nCallbackFlags |= COM_FLAGS_CALLBACK_AT_COM_TRANSMIT_DATA_NORMAL;
}

/*  *****************************************
 *
 *    Initialize the Communication Service ...
 *
 *  *****************************************
 */
void Com_Service_Init(void)
{
  #if(COM_SERVICE_DEBUG_ENABLE == 1)
    Serial.begin(9600);
    Serial.println("[I] Com Serv Debug ...");
  #endif

  /* startup state ... */
  eComServiceState = COM_SERVICE_STATE_STARTUP;

  /* initialize Ethernet module communication ... */
  if( COM_SERVICE_COMMUNICATION_MODE & COM_MODE_ETH )
  {
    Eth_Service_Client_init();
  }

  /* initialize SIMx00 module communication ... */
  if( COM_SERVICE_COMMUNICATION_MODE & COM_MODE_SIM )
  {
    AT_Command_Processor_Init();
  }

  sComServiceInternals.nEthConnectionStatus = COM_SERV_CONN_STAT_OFFLINE;
  sComServiceInternals.nATConnectionStatus  = COM_SERV_CONN_STAT_OFFLINE;  

}/* end function Com_Service_Init */


/* V0.1
 *  Cyclic function of communication Service
 *
 */
void Com_Service_main(void)
{
  unsigned short nTempCh1 = 0;
  unsigned short nTempCh2 = 0;
  unsigned short nTempCh3 = 0;
  unsigned short nTempCh4 = 0;
  unsigned char  nLastBillTemp = 0;
  unsigned char nStatus = 0;
  unsigned char nCommandFromServer = 0;

  switch( eComServiceState )
  {

    case COM_SERVICE_STATE_STARTUP:
    {
      /*
       * [ToDo] ... initialize time-outs ... errors ... 
       */

       eComServiceState = COM_SERVICE_STATE_QUERY_SERVER;

      #if(COM_SERVICE_DEBUG_ENABLE == 1)
        Serial.println("[I] State: COM_SERVICE_STATE_QUERY_SERVER ...");
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
         * [ToDo] Send Startup command ... and advance state ...
         */

        #if(COM_SERVICE_DEBUG_ENABLE == 1)
          Serial.println("[I] State: COM_SERVICE_STATE_CONNECTED_TO_SERVER_STARTUP ...");
        #endif

        nCallbackFlags &= ~COM_FLAGS_CALLBACK_AT_INIT_FINISH;

        eComServiceState = COM_SERVICE_STATE_CONNECTED_TO_SERVER_STARTUP;
      }
    }
    break;

    /*
     * make sure "START" frame is transmitted ... 
     */
    case COM_SERVICE_STATE_CONNECTED_TO_SERVER_STARTUP:
      
      nTempCh1 = Logger_App_Parallel_GetChannelValue( 0 );
      nTempCh2 = Logger_App_Parallel_GetChannelValue( 1 );
      nTempCh3 = Logger_App_Parallel_GetChannelValue( 2 );
      nTempCh4 = Logger_App_Parallel_GetChannelValue( 3 );

      nStatus = Com_Service_Client_Request_BillPayment_Generic_Transmission_Bill_v3(nTempCh1, nTempCh2, nTempCh3, nTempCh4, 0, 'S', 0 );
      //nStatus = 1;
      
      if( nStatus > 0 )
      {
        eComServiceState = COM_SERVICE_STATE_CONNECTED_TO_SERVER_STARTUP_WAIT;

        #if(COM_SERVICE_DEBUG_ENABLE == 1)
          Serial.println("[I] State: Sent START command ... goto COM_SERVICE_STATE_CONNECTED_TO_SERVER_STARTUP_WAIT ...");
        #endif

      }

    break;

    /*
     * wait until the AT status becomes available ... 
     */
    case COM_SERVICE_STATE_CONNECTED_TO_SERVER_STARTUP_WAIT:

      if( AT_Command_Processor_Is_Iddle() > 0 )
      {      
        eComServiceState = COM_SERVICE_STATE_FULL_COM;

        #if(COM_SERVICE_DEBUG_ENABLE == 1)
          Serial.println("[I] State: COM_SERVICE_STATE_FULL_COM ...");
        #endif
      }

    break;    


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
       * Monitor PING
       *    
       */
      if( nCom_Service_PingToServer_Delay == 0 )
      {
        /*
         * Send PING Frame ...    [ToDo ----- !!!!! check if this can be moved LAST !!!!! -------------------------------------]
         *
         */

        nTempCh1 = Logger_App_Parallel_GetChannelValue( 0 );
        nTempCh2 = Logger_App_Parallel_GetChannelValue( 1 );
        nTempCh3 = Logger_App_Parallel_GetChannelValue( 2 );
        nTempCh4 = Logger_App_Parallel_GetChannelValue( 3 );

        nStatus = Com_Service_Client_Request_BillPayment_Generic_Transmission_Bill_v3(nTempCh1, nTempCh2, nTempCh3, nTempCh4, 0, 'P', 0 );
        //nStatus = 1;
      
        if( nStatus > 0 )
        {
          /*
           * Reset PING delay ...
           */
          
          nCom_Service_PingToServer_Delay = LOG_APP_PING_TO_SERVER_DELAY;

        }


        #if(COM_SERVICE_DEBUG_ENABLE == 1)
          Serial.println("[I] State: COM_SERVICE_STATE_FULL_COM --> Send PING Req ");
        #endif

      }/* End PING !!  */


      /* 
       * send a normal frame ... ??
       */
      if( nCallbackFlags & COM_FLAGS_CALLBACK_AT_COM_TRANSMIT_DATA_NORMAL )      
      {
        nTempCh1 = Logger_App_Parallel_GetChannelValue( 0 );
        nTempCh2 = Logger_App_Parallel_GetChannelValue( 1 );
        nTempCh3 = Logger_App_Parallel_GetChannelValue( 2 );
        nTempCh4 = Logger_App_Parallel_GetChannelValue( 3 );
        nLastBillTemp = Logger_App_Parallel_GetLastBillType();

        nStatus = Com_Service_Client_Request_BillPayment_Generic_Transmission_Bill_v2(nTempCh1, nTempCh2, nTempCh3, nTempCh4, nLastBillTemp );
        //nStatus = 1;
      
        if( nStatus > 0 )
        {

          nCallbackFlags &= ~COM_FLAGS_CALLBACK_AT_COM_TRANSMIT_DATA_NORMAL;

          #if(COM_SERVICE_DEBUG_ENABLE == 1)
            Serial.println("[I] State: COM_SERVICE_STATE_FULL_COM --> Send DATA Req ");
          #endif

        }

      }


      if( nCallbackFlags & COM_FLAGS_CALLBACK_AT_COM_ERROR )
      {

        nCallbackFlags &= ~COM_FLAGS_CALLBACK_AT_COM_ERROR;

        eComServiceState = COM_SERVICE_STATE_NO_COM_ERROR;

        #if(COM_SERVICE_DEBUG_ENABLE == 1)
          Serial.println("[I] State: COM_SERVICE_STATE_NO_COM_ERROR ... Com Error ...");
        #endif

      }


      /* see if any new commands ... */
      if( AT_Command_Is_New_Command_From_Server() != 0 )
      {
        nCommandFromServer = AT_Command_Read_Last_Command_From_Server();


        /* RESET Monetary command ? */
        if(nCommandFromServer == 'B')
        {

          Logger_App_Parallel_Callback_Command_Reset_Monetary();
         
          nTempCh1 = Logger_App_Parallel_GetChannelValue( 0 );
          nTempCh2 = Logger_App_Parallel_GetChannelValue( 1 );
          nTempCh3 = Logger_App_Parallel_GetChannelValue( 2 );
          nTempCh4 = Logger_App_Parallel_GetChannelValue( 3 );
          
        }/* end if(nCommandFromServer == 'B') */
        else
          if(nCommandFromServer == 'N')
          {
            //strcpy( Logger_App_Parallel_GetDataPointer(0), );
          }


        nStatus = Com_Service_Client_Request_BillPayment_Generic_Transmission_Bill_v3(nTempCh1, nTempCh2, nTempCh3, nTempCh4, 0, 'A', 0 );

        /* if the transmission request has been accepted ... */
        if( nStatus > 0 )
        {
          /* mark command as processed ... */
          AT_Command_Process_Command_From_Server();

          #if(COM_SERVICE_DEBUG_ENABLE == 1)
            Serial.println("[I] State: COM_SERVICE_STATE_FULL_COM --> RESPONSE RESET MONETARY COMMAND ! ");
          #endif
        }


      }/*  if( AT_Command_Is_New_Command_From_Server() != 0 )  */       
      
    }/* end case COM_SERVICE_STATE_FULL_COM */
    break;


    /*
     * in this state we just check from time to time if the server connection is still UP
     *
     * ... after an IDDLE in communication ...
     */
    case COM_SERVICE_STATE_NO_COM_ERROR:
    {
      if( nCallbackFlags & COM_FLAGS_CALLBACK_AT_COM_NO_ERROR_RECOVERY )
      {
        eComServiceState = COM_SERVICE_STATE_FULL_COM;

        #if(COM_SERVICE_DEBUG_ENABLE == 1)
          Serial.println("[I] State: COM_SERVICE_STATE_FULL_COM ... Error recovery ...");
        #endif

      }      
        

    }
    break;


  }/* switch( eComServiceState ) */

}/* void Com_Service_main(void) */



/*
 *
 */
unsigned char Com_Service_Client_Request_BillPayment_Generic_Transmission_Bill_v2(unsigned short nChan1, unsigned short nChan2, \
                                                                                  unsigned short nChan3, unsigned short nChan4, \
                                                                                  unsigned char  nLastBillType)
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
  strcpy(arrLocalBuff, "AT+HTTPPARA=\"URL\",\"http://145.239.84.165:35269/gsm/entry_new.php?x=" LOG_APP_IMEI_CARD ";" LOG_APP_SIM_SN ";" LOG_APP_SIM_PHONE_NR ";" LOG_APP_SIM_OPERATOR ";");

  //strcat(arrLocalBuff, LOG_APP_IMEI_CARD );  
  //strcat(arrLocalBuff, ";");
  
  //strcat(arrLocalBuff, LOG_APP_SIM_SN );  
  //strcat(arrLocalBuff, ";");

  //strcat(arrLocalBuff, LOG_APP_SIM_PHONE_NR );  
  //strcat(arrLocalBuff, ";");

  //strcat(arrLocalBuff, LOG_APP_SIM_OPERATOR );  
  //strcat(arrLocalBuff, ";");


  snprintf(arrDummy, 50,"%d;%d;%d;%d;%d;0;0;", nChan1, nChan2, nChan3, nChan4, nLastBillType);
  strcat(arrLocalBuff, arrDummy);

  strcat(arrLocalBuff, LOG_APP_VERSION_MAJOR );  
  strcat(arrLocalBuff, ";");

  strcat(arrLocalBuff, LOG_APP_TOWN );  
  strcat(arrLocalBuff, ";");

  strcat(arrLocalBuff, LOG_APP_PLACE );  
  strcat(arrLocalBuff, ";");

  strcat(arrLocalBuff, LOG_APP_DETAILS );  
  strcat(arrLocalBuff, ";");

  strcat(arrLocalBuff, LOG_APP_DEVICE_TYPE );  
  strcat(arrLocalBuff, ";");

  strcat(arrLocalBuff, LOG_APP_DEFAULT_RSSI );  

  strcat(arrLocalBuff, "\"\r");

  #if(COM_SERVICE_DEBUG_ENABLE == 1)
    Serial.println();
    Serial.print("[I] Com send-> ");
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
                                                                                  unsigned char  nLastBillType, unsigned char nFrameType, unsigned char nErrorVal)
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
  strcpy(arrLocalBuff, "AT+HTTPPARA=\"URL\",\"http://145.239.84.165:35269/gsm/entry_new.php?x=" LOG_APP_IMEI_CARD ";" LOG_APP_SIM_SN ";" LOG_APP_SIM_PHONE_NR ";" LOG_APP_SIM_OPERATOR ";");

  //strcat(arrLocalBuff, LOG_APP_IMEI_CARD );  
  //strcat(arrLocalBuff, ";");
  
  //strcat(arrLocalBuff, LOG_APP_SIM_SN );  
  //strcat(arrLocalBuff, ";");

  //strcat(arrLocalBuff, LOG_APP_SIM_PHONE_NR );  
  //strcat(arrLocalBuff, ";");

  //strcat(arrLocalBuff, LOG_APP_SIM_OPERATOR );  
  //strcat(arrLocalBuff, ";");


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

  strcat(arrLocalBuff, LOG_APP_TOWN );  
  strcat(arrLocalBuff, ";");

  strcat(arrLocalBuff, LOG_APP_PLACE );  
  strcat(arrLocalBuff, ";");

  strcat(arrLocalBuff, LOG_APP_DETAILS );  
  strcat(arrLocalBuff, ";");

  strcat(arrLocalBuff, LOG_APP_DEVICE_TYPE );  
  strcat(arrLocalBuff, ";");

  strcat(arrLocalBuff, LOG_APP_DEFAULT_RSSI );  

  strcat(arrLocalBuff, "\"\r");

  #if(COM_SERVICE_DEBUG_ENABLE == 1)
    Serial.println();
    Serial.print("[I] Com send-> ");
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



/* Message format CLIENT to SERVER    - send a generic request to server in order to check service availability 
 *
 *   - "/gsm/index.php"
 */
void Com_Service_Build_Server_Request_Service_Availability_ETH(void)
{
   /*
   * Example of a request:
   *
   * http://145.239.84.165:35269/gsm/index.php
   */


  Eth_Service_Client_ClearTxBuff();
  Eth_Service_Client_AddTxData( SERVER_ADDRESS_GET_URL_AVAILABILITY ); 

  Eth_Service_Client_AddTxData( " HTTP/1.1");

  Eth_Service_Client_SendRequest(); 
}

/*
 * Server response should contain at least 51 alfa numeric characters ... 
 *
 *  [ToDo] ... search should stop if a "\0" char is reached ... 
 */
unsigned char Com_Service_Process_Server_Response_Service_Availability_ETH(void)
{
  unsigned char nReturn = 0;

  char *ServerRespData = NULL;
  unsigned short nBufferLen = 0;
  unsigned short nCharsCount = 0;
  unsigned short nIdx = 0;


  nBufferLen = Eth_Service_Client_Get_RxBuffer(&ServerRespData);

  //#if(LOGGER_APP_DEBUG_SERIAL == 1)
  //  Serial.print("Server:  ");
  //  Serial.write(ServerRespData, nBufferLen);
  //  Serial.println();    
  //#endif

  for(nIdx = 0; nIdx < nBufferLen; nIdx ++)  
  {
    if( ( ServerRespData[nIdx] >= 32 )  && ( ServerRespData[nIdx] <= 126 ) )   /* al numeric and alfa-numeric chars ... from ascii table .. */
    {
      nCharsCount ++;
    }

    if(ServerRespData[nIdx] == '\0')
      break;
  }
  
  if(nCharsCount > 50)
  {
    nReturn = 1;
  }
  else
  {
    nReturn = 0;
  }


  return nReturn;
}







/* Message format CLIENT to SERVER    - Generic message format, no error client to server ... 
 *
 *   - formats message to be sent to server containing channel values
 *   - sends last validated bill value
 *   - user can select different types of frames ...  
 */
void Com_Service_Build_Server_Req_Bill_Payment_Generic( unsigned char nFrameType )
{
  char arrDummy[50];


  /*
   * Example of a request:
   *
   * http://192.168.0.109/entry.php?x=862462030142276;1111111111111111111;0726734732;RDS;0;0;0;0;P;0;0;No_approver;IASI;PALAS_IASI;PALAS_PARTER;NV9;1
   * 
   * new value:
   *   http://145.239.84.165:35269/gsm/entry_new.php?x=862462030142276;1111111111111111111;0726734731;RDS;0;0;0;0;P;0;0;No_approver;IASI;PALAS_IASI;PALAS_PARTER;NV9;1
   */


  Eth_Service_Client_ClearTxBuff();

  //Eth_Service_Client_AddTxData("GET /gsm/entry_new.php?x=");
  //Eth_Service_Client_AddTxData("GET /entry_new.php?x=");
  //----------------------------------------- Eth_Service_Client_AddTxData("GET /gsm/entry_new.php?x=");    /* PRODUCTION address ... !!!  */
  Eth_Service_Client_AddTxData( SERVER_ADDRESS_GET_URL_COMMAND_X );
  //Eth_Service_Client_AddTxData("GET /entry_new.php?x=");      /* LOCAL Tests ! address  !!!  */

  Eth_Service_Client_AddTxData( LOG_APP_IMEI_CARD ";");
  Eth_Service_Client_AddTxData( LOG_APP_SIM_SN ";");
  Eth_Service_Client_AddTxData( LOG_APP_SIM_PHONE_NR ";");
  Eth_Service_Client_AddTxData( LOG_APP_SIM_OPERATOR ";");


  /* 
   * Add Bills channel 1
   * Add Bills channel 2
   * Add Bills channel 3
   * Add Bills channel 4
   * Add type of event
   */ 

  snprintf(arrDummy, 50,"%d;%d;%d;%d;", Logger_App_Get_Channel_Values(1), Logger_App_Get_Channel_Values(2), Logger_App_Get_Channel_Values(3), Logger_App_Get_Channel_Values(4));
  Eth_Service_Client_AddTxData(arrDummy);   

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
      snprintf( arrDummy, 50,"%d;0;0;", Logger_App_Get_LastBillType() );
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
      /* values of nCH1_old, .... already reset previously .... */
      snprintf(arrDummy, 50,"A;0;1;");
    }   
    break;

    case 'S':
    {
      /* values of nCH1_old, .... already reset previously .... */
      snprintf(arrDummy, 50,"S;0;0;");
    }   
    break;

    /*
     * Error frame 
     */
    case 'E':
    {
      snprintf(arrDummy, 50,"E;%d;0;", Logger_App_Get_LastErrorValue() );
    }
    break;       
  }

  Eth_Service_Client_AddTxData(arrDummy);


  

  #if(LOGGER_APP_DEBUG_SERIAL == 1)
    Serial.println("----------------------------");
    Serial.println(arrDummy);
    Serial.println("----------------------------");  
  #endif


  Eth_Service_Client_AddTxData( LOG_APP_VERSION_MAJOR ";");
  Eth_Service_Client_AddTxData( LOG_APP_TOWN ";");
  Eth_Service_Client_AddTxData( LOG_APP_PLACE ";");
  Eth_Service_Client_AddTxData( LOG_APP_DETAILS ";");
  Eth_Service_Client_AddTxData( LOG_APP_DEVICE_TYPE ";");
  Eth_Service_Client_AddTxData( LOG_APP_DEFAULT_RSSI);

  Eth_Service_Client_AddTxData( " HTTP/1.1");

  /* [ToDo] ... check the return status !!!  */  

  //Eth_Service_Client_StackBuffRequest();

  Eth_Service_Client_SendRequest(); 


   /*
   * Change State
   */
  
}


/* [ToDo] --- this function should be able to stack up to 10 commands ... in case you shuld process them later
 *    
 *
 */

unsigned char Com_Service_Process_Server_Responses(void)
{
  unsigned char nReturn = 0;

  char *ServerRespData = NULL;
  unsigned short nBufferLen = 0;
  unsigned short nIdx = 0;


  nBufferLen = Eth_Service_Client_Get_RxBuffer(&ServerRespData);

  #if(LOGGER_APP_DEBUG_SERIAL == 1)
    //Serial.print("Server:  ");
    //Serial.write(ServerRespData, nBufferLen);
    //Serial.println();    
  #endif


  for(nIdx = 0; nIdx < nBufferLen; nIdx ++)  
  {
    //Serial.write(ServerRespData[nIdx]);
    
    /*
     *   ... look inside the receive buffer 
     *   ... if null char or "*" then ... stop
     *
     *     [ToDo] .... !!!!!! make sure that we should stop also on NULL char !!!!
     */
    if( (ServerRespData[nIdx] == 0) || (ServerRespData[nIdx] == '*'))
      break;


    /* the message is not at the end of the buffer
    */
    if(nIdx < (nBufferLen - 5))
    {
      if( (ServerRespData[nIdx] == 'C') && (ServerRespData[nIdx + 1] == 'M') && (ServerRespData[nIdx + 2] == 'D') )
      {
        /* 
         * Normal Alive response ...           CMDA 
         */
        if( ServerRespData[nIdx + 3] == 'A') 
        {
          nReturn = 1;

          #if(LOGGER_APP_DEBUG_SERIAL == 1)
            Serial.println("Server response --> CMDA ");
          #endif

          /* no command */
          sComServiceInternals.nCom_Service_Response_Command_Type = 0;          

          break;
        }

        /*
         * reset cash request from server ....    CMDB
         */
        if( ServerRespData[nIdx + 3] == 'B') 
        {
          nReturn = 1;

          #if(LOGGER_APP_DEBUG_SERIAL == 1)
            Serial.println("Server response --> CMDB ");
          #endif

          /* reset command received ... */
          sComServiceInternals.nCom_Service_Response_Command_Type = 'B';

          break;
        }

      }/* if( (ServerRespData[nIdx] == 'C') && (ServerRespData[nIdx + 1] == 'M') && (ServerRespData[nIdx + 2] == 'D') ) */

    }/* if(nIdx < (nBufferLen - 5)) */

  }/* for(nIdx = 0; nIdx < nBufferLen; nIdx ++)   */

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

}

