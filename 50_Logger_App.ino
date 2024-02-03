/* C.Parascan
 *  v1.0 - final bugs related to board reset solved - 03.04.2023
 *  v2.0 - major updates - 10.10.2023
 *                       - remove Eth communication 
 *                       - adapt for Com_Service interraction
 */

#include "01_Debug_Utils.h"

#include "99_Automat_Board_cfg.h"
#include "50_Logger_App.h"
#include "50_Logger_App_cfg.h"
/*#include "05_Eth_Service_Client.h"*/
#include "52_tool_Crypto.h"
#include "54_tool_NV9plus_Utils.h"

#include "09_NvM_Manager.h"
#include "09_NvM_Manager_cfg.h"

#include "03_Com_Service.h"
#include "50_Logger_App_Parallel.h"

/*
 *  NvM interface - RAM mirror
 *
 */
unsigned short nNvM_RAM_Mirror_CH1 = 0;
unsigned short nNvM_RAM_Mirror_CH2 = 0;
unsigned short nNvM_RAM_Mirror_CH3 = 0;
unsigned short nNvM_RAM_Mirror_CH4 = 0;
unsigned short nNvM_RAM_Mirror_CH5 = 0;
unsigned char arrNvM_RAM_Mirror_Str_SimNR   [ LOG_APP_SIMNR_MAX_CHAR       + 1 ] = {0};
unsigned char arrNvM_RAM_Mirror_Str_Town    [ LOG_APP_TOWN_MAX_CHAR        + 1 ] = {0};
unsigned char arrNvM_RAM_Mirror_Str_Place   [ LOG_APP_PLACE_MAX_CHAR       + 1 ] = {0};
unsigned char arrNvM_RAM_Mirror_Str_Details [ LOG_APP_DETAILS_MAX_CHAR     + 1 ] = {0};
unsigned char arrNvM_RAM_Mirror_Str_Device  [ LOG_APP_DEVICE_TYPE_MAX_CHAR + 1 ] = {0};

/* Application local variables
 *
 */


/* channel load ... erros and last bill type
   - counts the number of bills readed by the bill reader 
*/
unsigned short  nCH1_old = 0;
unsigned short  nCH2_old = 0;
unsigned short  nCH3_old = 0;
unsigned short  nCH4_old = 0;
unsigned short  nCH5_old = 0;
T_NV9plus_Error eMachineError_old = E_UNKNOWN;
unsigned char nLastBillType = 0;


/* Communication related varianbles 
 *
 * 
 * Local Buffers used for the USART communication ... 
 */
unsigned short nLoggerMsgBuffIDX_Rx = 0;
unsigned char  arrLoggerReceiveBytes[_MAX_RX_COMM_BUFFER_SIZE_] = {0};

unsigned short itterUSART_withoutRX = 0;
unsigned char new_Comm_Frame = 0;
unsigned char nNew_Comm_Frame_Consumed = 0;
unsigned char nFirstRxFrame_AfterReset = 1;


/* Server commands .... 
 * to be executed by the application logger board
 */
unsigned char nServerCommandCode = 0;



/* Application internal states .... */
T_LoggerAppState LoggerAppState = E_LOGGER_APP_UNININT;


/* Delays and timeouts ... 
 *
 */

/* startup delay ... */
unsigned short nLoggerAppStartupDelay = LOG_APP_STARTUP_DELAY;


/*
 * State FLAGS
 *
 */



T_NV9plus_state eMachineState_old = NV9_STATE_RUN_NO_ERROR;      /* to optimize for new events !!!   <<<<<<<<<  [ToDo] - check if still needed */
unsigned char nTransmissionToServer_IsPreparedForNewTransmission = 0;  /* <<<<< [ToDo] -- - check if still needed */

unsigned char nFlagWaitResponseAfterRequest = 0;  /* <<<<<< [ToDo] --- check if needed after optimization */


/*
 * !!!!  OPTIMIZATION  !!!!
 */
extern T_NV9plus_internal sNV9plus;
/* 
 * this should never be used except for read !!!
 *    due to CPU load limitation 
 *   otherwise use "tool_NV9plus_utils_GetChannelContent" !!!!
 */





#if(LOGGER_APP_DEBUG_SERIAL == 1)
char Logger_DebugPrintBuff[50];
T_LoggerAppState LoggerAppState_Debug = E_LOGGER_APP_UNININT;
#endif


/********************************************************
 *                    Local API's - 
 ********************************************************/
void Logger_App_Outputs_SetReset(void);

void Logger_App_Build_Server_Req_Bill_Payment_Generic(unsigned char nFrameType, unsigned char nErrorValue);
void Logger_App_Build_Server_Req_Bill_Payment(void);
void Logger_App_Build_Server_Reset_Bill_Value_Response(void);
unsigned char Logger_App_Process_Buffer(void);



/*
 * Used for debugging porpuses
 */
#if(LOGGER_APP_DEBUG_SERIAL == 1)
void Logger_App_Debug_Print_State(void)
{
  if( LoggerAppState_Debug != LoggerAppState )
  {

    switch(LoggerAppState)
    {
      case E_LOGGER_APP_UNININT:
        Serial.println("[I] Logg_App -- E_LOGGER_APP_UNININT -- ");
        break;

      case E_LOGGER_APP_INIT:
        Serial.println("[I] Logg_App -- E_LOGGER_APP_INIT -- ");
        break;

      case E_LOGGER_APP_STARTUP_DELAY:
        Serial.println("[I] Logg_App -- E_LOGGER_APP_STARTUP_DELAY -- ");
        break;

      case E_LOGGER_APP_PROCESS_MESSAGES:
        Serial.println("[I] Logg_App -- E_LOGGER_APP_PROCESS_MESSAGES -- ");
        break;

      case E_LOGGER_APP_ETH_INFORM_SERVER:
        Serial.println("[I] Logg_App -- E_LOGGER_APP_ETH_INFORM_SERVER -- ");
        break;

      case E_LOGGER_APP_ETH_INFORM_SERVER_ERROR_STATE:
        Serial.println("[I] Logg_App -- E_LOGGER_APP_ETH_INFORM_SERVER_ERROR_STATE -- ");
        break;

      case E_LOGGER_APP_ETH_PROCESS_SERVER_RESPONSE:
        Serial.println("[I] Logg_App -- E_LOGGER_APP_ETH_PROCESS_SERVER_RESPONSE -- ");
        break;

      case E_LOGGER_APP_ETH_SEND_COMMAND_RESET_BILL_ACK_RESPONSE:
        Serial.println("[I] Logg_App -- E_LOGGER_APP_ETH_SEND_COMMAND_RESET_BILL_ACK_RESPONSE -- ");
        break;

      case E_LOGGER_APP_ETH_SEND_STARTUP_COMMAND:
        Serial.println("[I] Logg_App -- E_LOGGER_APP_ETH_SEND_STARTUP_COMMAND -- ");
        break;

      case E_LOGGER_APP_ETH_SAVE_DATA_TO_EEPROM:
        Serial.println("[I] Logg_App -- E_LOGGER_APP_ETH_SAVE_DATA_TO_EEPROM -- ");
        break;

    }

    LoggerAppState_Debug = LoggerAppState;    
  }

}
#endif



/* 
 * Exported API used to attach local variables to NVM blocks defined on EEPROM driver level !
 *    ... RAM mirror image is locally used to store the EEPROM content
 */
void Logger_App_pre_init_NvM_link(void)
{
  NvM_AttachRamImage((unsigned char *)&nNvM_RAM_Mirror_CH1, 1);
  NvM_AttachRamImage((unsigned char *)&nNvM_RAM_Mirror_CH2, 2);
  NvM_AttachRamImage((unsigned char *)&nNvM_RAM_Mirror_CH3, 3);
  NvM_AttachRamImage((unsigned char *)&nNvM_RAM_Mirror_CH4, 4);
  NvM_AttachRamImage((unsigned char *)&nNvM_RAM_Mirror_CH5, 5);

  NvM_AttachRamImage((unsigned char *)&arrNvM_RAM_Mirror_Str_SimNR   [0],  NVM_BLOCK_CHAN_6_ID);
  NvM_AttachRamImage((unsigned char *)&arrNvM_RAM_Mirror_Str_Town    [0],  NVM_BLOCK_CHAN_7_ID);
  NvM_AttachRamImage((unsigned char *)&arrNvM_RAM_Mirror_Str_Place   [0],  NVM_BLOCK_CHAN_8_ID);
  NvM_AttachRamImage((unsigned char *)&arrNvM_RAM_Mirror_Str_Details [0],  NVM_BLOCK_CHAN_9_ID);
  NvM_AttachRamImage((unsigned char *)&arrNvM_RAM_Mirror_Str_Device  [0],  NVM_BLOCK_CHAN_10_ID);

}




/*
 * Validate strings !!! They will be used in the URL request to server and must not contain spaces or special chars !!!
 * 
 */
void Logger_App_Fix_NvM_String_Data(unsigned char *strInput, unsigned char nMaxLen)
{
  unsigned char nIdx = 0;

  for (nIdx = 0; nIdx < nMaxLen; nIdx ++ )
  {
    if( *(strInput + nIdx) == 0x00 )
      break;
    else
      if( ( ( *(strInput + nIdx) >= '0' ) && ( *(strInput + nIdx) <= '9' ) ) || \
          ( ( *(strInput + nIdx) >= 'a' ) && ( *(strInput + nIdx) <= 'z' ) ) || \
          ( ( *(strInput + nIdx) >= 'A' ) && ( *(strInput + nIdx) <= 'Z' ) ) )
          {
            /* 
             * valid character ... 
             */
          }
          else
          {
            /*
             * invalid char ... fill with '_'
             */
            *(strInput + nIdx) = '_';
          }

  }/* end for ... */

}

void Logger_App_pre_init_Validate_NvM_String_Data(void)
{
  
  Logger_App_Fix_NvM_String_Data( (unsigned char *)&arrNvM_RAM_Mirror_Str_SimNR   [0], NVM_BLOCK_CHAN_6_DATA_LEN );
  Logger_App_Fix_NvM_String_Data( (unsigned char *)&arrNvM_RAM_Mirror_Str_Town    [0], NVM_BLOCK_CHAN_7_DATA_LEN );
  Logger_App_Fix_NvM_String_Data( (unsigned char *)&arrNvM_RAM_Mirror_Str_Place   [0], NVM_BLOCK_CHAN_8_DATA_LEN );
  Logger_App_Fix_NvM_String_Data( (unsigned char *)&arrNvM_RAM_Mirror_Str_Details [0], NVM_BLOCK_CHAN_9_DATA_LEN );
  Logger_App_Fix_NvM_String_Data( (unsigned char *)&arrNvM_RAM_Mirror_Str_Device  [0], NVM_BLOCK_CHAN_10_DATA_LEN );

}





/*
 * Fill in the NvM blocks with default values 
 *
 *   ... the range of target NvM block ID's is 6->10 (because the other blocks are cyclically written on each bill validation)
 *   - this special blocks are anly written by a special command received from server
 *     therefore during first startups (virgin controller) it might be completly empty ... and could stay like that indefinetely until
 *     an "update" command is issued !
 *
 */
void Logger_App_Write_Default_Values_NvM(unsigned char nSlotWrite)
{

  switch( nSlotWrite )
  {

    case NVM_BLOCK_CHAN_6_ID:
      NvM_Write_Block(NVM_BLOCK_CHAN_6_ID);
      break;

    case NVM_BLOCK_CHAN_7_ID:
      NvM_Write_Block(NVM_BLOCK_CHAN_7_ID);
      break;

    case NVM_BLOCK_CHAN_8_ID:
      NvM_Write_Block(NVM_BLOCK_CHAN_8_ID);
      break;

    case NVM_BLOCK_CHAN_9_ID:
      NvM_Write_Block(NVM_BLOCK_CHAN_9_ID);
      break;

    case NVM_BLOCK_CHAN_10_ID:
      NvM_Write_Block(NVM_BLOCK_CHAN_10_ID);
      break;

  }

}

/*
 * API used to be called from exterior of module "Logger_App" with
 *        only porpuse to reset to default the monetray registers, and associated flags
 */
void Logger_App_Callback_Command_Reset_Monetary(void)
{

  nCH1_old = 0;
  nCH2_old = 0;
  nCH3_old = 0;
  nCH4_old = 0;
  nCH5_old = 0;

  /* reset also internal "sNV9plus" states ... */
  tool_NV9plus_utils_ResetCashContent();

  nNvM_RAM_Mirror_CH1 = 0; 
  NvM_Write_Block( NVM_BLOCK_CHAN_1_ID );

  nNvM_RAM_Mirror_CH2 = 0;
  NvM_Write_Block( NVM_BLOCK_CHAN_2_ID );

  nNvM_RAM_Mirror_CH3 = 0;
  NvM_Write_Block( NVM_BLOCK_CHAN_3_ID );

  nNvM_RAM_Mirror_CH4 = 0;
  NvM_Write_Block( NVM_BLOCK_CHAN_4_ID);

  nCH1_old = 0;
  nCH2_old = 0;
  nCH3_old = 0;
  nCH4_old = 0;
  nCH5_old = 0;
  eMachineError_old = E_UNKNOWN;
  nLastBillType = 0;

}/* end function Logger_App_Callback_Command_Reset_Monetary */



unsigned short Logger_App_GetChannelValue(unsigned char nChannValue)
{
  unsigned short nReturn = 0;

  switch( nChannValue )
  {
    case 1:
      nReturn = nCH1_old;
      break;
    
    case 2:
      nReturn = nCH2_old;
      break;

    case 3:
      nReturn = nCH3_old;
      break;
    
    case 4:
      nReturn = nCH4_old;
      break;

    case 5:
      nReturn = nCH5_old;
      break;

    default:
      nReturn = sNV9plus.nBillValidator_err;
      break;
  }

  return nReturn;
}

unsigned char Logger_App_Get_LastBillType(void)
{
  return nLastBillType;
}

unsigned char Logger_App_Get_LastErrorValue(void)
{
  return (unsigned char)sNV9plus.nBillValidator_err;
}

char* Logger_App_GetPhoneNumberString(void)
{
  return &arrNvM_RAM_Mirror_Str_SimNR[0];
}

char* Logger_App_GetTownNameString(void)
{
  return &arrNvM_RAM_Mirror_Str_Town[0];
}

char* Logger_App_GetPlaceNameString(void)
{
  return &arrNvM_RAM_Mirror_Str_Place[0];
}

char* Logger_App_GetDetailsNameString(void)
{
  return &arrNvM_RAM_Mirror_Str_Details[0];
}

char* Logger_App_GetDeviceNameString(void)
{
  return &arrNvM_RAM_Mirror_Str_Device[0];
}



/* Exported API used to initialize the Logger App 
 *
 */
void Logger_App_init(void)
{ 

  /*
   * recover NvM information ... 
   *
   */
  unsigned char nStatus = 0;

//  #if(LOGGER_APP_DEBUG_SERIAL >= 1) 
//    Serial.begin(9600);
//  #endif

  /* first step initialization !!!
   * ----->>>> [ToDo] <<<<<----- optimize code make sure "sNV9plus.nCH1_bill" is initialized only once and in the right place
   *                                  "tool_NV9plus_utils_init" and below ...
   */
  tool_NV9plus_utils_init();

  /* *************************
   *    Restore channel 1
   * *************************
   */
  nStatus = NvM_Get_Block_Status(NVM_BLOCK_CHAN_1_ID);

  if( nStatus & NVM_FLAG_LAST_READ_ALL_OK )
  {
    if( nNvM_RAM_Mirror_CH1 < 0xFFFF)
    {
      nCH1_old = nNvM_RAM_Mirror_CH1;
      sNV9plus.nCH1_bill = nNvM_RAM_Mirror_CH1;
    }
    else
    {
      nCH1_old = 0;
    }
  }


  /* *************************
   *    Restore channel 2
   * *************************
   */
  nStatus = NvM_Get_Block_Status(NVM_BLOCK_CHAN_2_ID);

  if( nStatus & NVM_FLAG_LAST_READ_ALL_OK )
  {
    if( nNvM_RAM_Mirror_CH2 < 0xFFFF)
    {
      nCH2_old = nNvM_RAM_Mirror_CH2;
      sNV9plus.nCH2_bill = nNvM_RAM_Mirror_CH2;
    }
    else
    {
      nCH2_old = 0;
    }
  }


  /* *************************
   *    Restore channel 3
   * *************************
   */
  nStatus = NvM_Get_Block_Status(NVM_BLOCK_CHAN_3_ID);

  if( nStatus & NVM_FLAG_LAST_READ_ALL_OK )
  {
    if( nNvM_RAM_Mirror_CH3 < 0xFFFF)
    {
      nCH3_old = nNvM_RAM_Mirror_CH3;
      sNV9plus.nCH3_bill = nNvM_RAM_Mirror_CH3;      
    }
    else
    {
      nCH3_old = 0;
    }
  }

  /* *************************
   *    Restore channel 4
   * *************************
   */
  nStatus = NvM_Get_Block_Status(NVM_BLOCK_CHAN_4_ID);

  if( nStatus & NVM_FLAG_LAST_READ_ALL_OK )
  {
    if( nNvM_RAM_Mirror_CH4 < 0xFFFF)
    {
      nCH4_old = nNvM_RAM_Mirror_CH4;
      sNV9plus.nCH4_bill = nNvM_RAM_Mirror_CH4;
    }
    else
    {
      nCH4_old = 0;
    }
  }


  /* *************************
   *    Restore channel 5
   * *************************
   */
  nStatus = NvM_Get_Block_Status(NVM_BLOCK_CHAN_5_ID);

  if( nStatus & NVM_FLAG_LAST_READ_ALL_OK )
  {
    if( nNvM_RAM_Mirror_CH5 < 0xFFFF)
    {
      nCH5_old = nNvM_RAM_Mirror_CH5;
      sNV9plus.nCH5_bill = nNvM_RAM_Mirror_CH5;      
    }
    else
    {
      nCH5_old = 0;
    }
  }

  #if(LOGGER_APP_DEBUG_SERIAL == 1)
    snprintf(Logger_DebugPrintBuff, 50, "[I] Logg_App Logger INIT CH1= %d, CH2= %d, CH3= %d, CH4= %d ", sNV9plus.nCH1_bill, sNV9plus.nCH2_bill, sNV9plus.nCH3_bill, sNV9plus.nCH4_bill);
    Serial.println(Logger_DebugPrintBuff);


    snprintf(Logger_DebugPrintBuff, 50, ">> %s", arrNvM_RAM_Mirror_Str_SimNR);
    Serial.println(Logger_DebugPrintBuff);

    snprintf(Logger_DebugPrintBuff, 50, ">> %s", arrNvM_RAM_Mirror_Str_Town);
    Serial.println(Logger_DebugPrintBuff);
    
    snprintf(Logger_DebugPrintBuff, 50, ">> %s", arrNvM_RAM_Mirror_Str_Place);
    Serial.println(Logger_DebugPrintBuff);


    snprintf(Logger_DebugPrintBuff, 50, ">> %s", arrNvM_RAM_Mirror_Str_Details);
    Serial.println(Logger_DebugPrintBuff);

    snprintf(Logger_DebugPrintBuff, 50, ">> %s", arrNvM_RAM_Mirror_Str_Device);
    Serial.println(Logger_DebugPrintBuff);

  #endif


  /* init application internal variables ... */
  nTransmissionToServer_IsPreparedForNewTransmission = 1;

  nFlagWaitResponseAfterRequest = 0;

}





/* Message format CLIENT to SERVER    - Generic message format, no error client to server ... 
 *
 *   - formats message to be sent to server containing channel values
 *   - sends last validated bill value
 *   - user can select different types of frames ...  
 */
void Logger_App_Build_Server_Req_Bill_Payment_Generic(unsigned char nFrameType, unsigned char nErrorValue)
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

/*
  Eth_Service_Client_ClearTxBuff();
*/

  //Eth_Service_Client_AddTxData("GET /gsm/entry_new.php?x=");
  //Eth_Service_Client_AddTxData("GET /entry_new.php?x=");
/*  Eth_Service_Client_AddTxData("GET /gsm/entry_new.php?x=");    /* PRODUCTION address ... !!!  */ 
  //Eth_Service_Client_AddTxData("GET /entry_new.php?x=");      /* LOCAL Tests ! address  !!!  */

/*
  Eth_Service_Client_AddTxData( LOG_APP_IMEI_CARD ";");
  Eth_Service_Client_AddTxData( LOG_APP_SIM_SN ";");
  Eth_Service_Client_AddTxData( LOG_APP_SIM_PHONE_NR ";");
  Eth_Service_Client_AddTxData( LOG_APP_SIM_OPERATOR ";");
*/

  /* 
   * Add Bills channel 1
   * Add Bills channel 2
   * Add Bills channel 3
   * Add Bills channel 4
   * Add type of event
   */ 
/*
  snprintf(arrDummy, 50,"%d;%d;%d;%d;", nCH1_old, nCH2_old, nCH3_old, nCH4_old);
  Eth_Service_Client_AddTxData(arrDummy);   
*/
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
      snprintf(arrDummy, 50,"%d;0;0;", nLastBillType);
    }
    break;

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

    case 'E':
    {
      snprintf(arrDummy, 50,"E;%d;0;", nErrorValue);
    }
    break;       
  }

/* Eth_Service_Client_AddTxData(arrDummy); */


  

  #if(LOGGER_APP_DEBUG_SERIAL == 1)
    Serial.println("[I] Logg_App ----------------------------");
    Serial.println(arrDummy);
    Serial.println("[I] Logg_App ----------------------------");  
  #endif
/*

  Eth_Service_Client_AddTxData( LOG_APP_VERSION_MAJOR ";");
  Eth_Service_Client_AddTxData( LOG_APP_TOWN ";");
  Eth_Service_Client_AddTxData( LOG_APP_PLACE ";");
  Eth_Service_Client_AddTxData( LOG_APP_DETAILS ";");
  Eth_Service_Client_AddTxData( LOG_APP_DEVICE_TYPE ";");
  Eth_Service_Client_AddTxData( LOG_APP_DEFAULT_RSSI);

  Eth_Service_Client_AddTxData( " HTTP/1.1");
*/
  /* [ToDo] ... check the return status !!!  */  

  //Eth_Service_Client_StackBuffRequest();

 /* Eth_Service_Client_SendRequest(); */
}



/* Message format CLIENT to SERVER    - no error, Bill update 
 *
 *   - formats message to be sent to server containing channel values
 *   - sends last validated bill value
 */
void Logger_App_Build_Server_Req_Bill_Payment(void)
{
  char arrDummy[50];


  /*
   * Example of a request:
   *
   * http://192.168.0.109/entry.php?x=862462030142276;1111111111111111111;0726734732;RDS;0;0;0;0;P;0;0;No_approver;IASI;PALAS_IASI;PALAS_PARTER;NV9;1
   */

/*
  Eth_Service_Client_ClearTxBuff();
*/
  //Eth_Service_Client_AddTxData("GET /gsm/entry_new.php?x=");
  //Eth_Service_Client_AddTxData("GET /entry_new.php?x=");
/*  Eth_Service_Client_AddTxData("GET /gsm/entry_new.php?x=");    /* PRODUCTION address ... !!!  */
  //Eth_Service_Client_AddTxData("GET /entry_new.php?x=");      /* LOCAL Tests ! address  !!!  */

/*
  Eth_Service_Client_AddTxData( LOG_APP_IMEI_CARD ";");
  Eth_Service_Client_AddTxData( LOG_APP_SIM_SN ";");
  Eth_Service_Client_AddTxData( LOG_APP_SIM_PHONE_NR ";");
  Eth_Service_Client_AddTxData( LOG_APP_SIM_OPERATOR ";");
*/

  /* 
   * Add Bills channel 1
   * Add Bills channel 2
   * Add Bills channel 3
   * Add Bills channel 4
   * Add type of event
   /* 
   

                                                   1 = bill of type 1 validated
                                                   2 = bill of type 2 validated
                                                   3 = bill of type 3 validated
                                                   4 = bill of type 4 validated
                                                   S = Start dupa o pana de curent
                                                   B = blocat
                                                   P = ping
                                                   O = usa aparatului deschisa
                                                   C = usa aparatului inchisa
                                                   E = error
  */ 

  /* example:   "100;200;300;400;2;0;0;"       
  */

  /* add type of errors --> 0         */
  /* add reset confirmation --> 0     */

/*
  snprintf(arrDummy, 50,"%d;%d;%d;%d;%d;0;0;", nCH1_old, nCH2_old, nCH3_old, nCH4_old, nLastBillType);
  Eth_Service_Client_AddTxData(arrDummy);
*/
  #if(LOGGER_APP_DEBUG_SERIAL == 1)
    Serial.println("[I] Logg_App ----------------------------");
    Serial.println(arrDummy);
    Serial.println("[I] Logg_App ----------------------------");  
  #endif

/*

  Eth_Service_Client_AddTxData( LOG_APP_VERSION_MAJOR ";");
  Eth_Service_Client_AddTxData( LOG_APP_TOWN ";");
  Eth_Service_Client_AddTxData( LOG_APP_PLACE ";");
  Eth_Service_Client_AddTxData( LOG_APP_DETAILS ";");
  Eth_Service_Client_AddTxData( LOG_APP_DEVICE_TYPE ";");
  Eth_Service_Client_AddTxData( LOG_APP_DEFAULT_RSSI);

  Eth_Service_Client_AddTxData( " HTTP/1.1");
*/
  /* [ToDo] ... check the return status !!!  */  

  //Eth_Service_Client_StackBuffRequest();

/*  Eth_Service_Client_SendRequest(); */

}



/* Message format CLIENT to SERVER    - no error, Bill value reset 
 *
 *   - formats message to be sent to server containing 
 */
void Logger_App_Build_Server_Reset_Bill_Value_Response(void)
{
  char arrDummy[50];


  /*
   * Example of a request:
   *
   * http://192.168.0.109/entry.php?x=862462030142276;1111111111111111111;0726734732;RDS;0;0;0;0;A;0;1;2;IASI;PALAS_IASI;PALAS_PARTER;NV9;1
   */

/*
  Eth_Service_Client_ClearTxBuff();

  Eth_Service_Client_AddTxData("GET /gsm/entry_new.php?x=");    /* PRODUCTION address ... !!!  */

  //Eth_Service_Client_AddTxData("GET /entry_new.php?x=");      /* LOCAL Tests ! address  !!!  */
/*  
  Eth_Service_Client_AddTxData( LOG_APP_IMEI_CARD ";");
  Eth_Service_Client_AddTxData( LOG_APP_SIM_SN ";");
  Eth_Service_Client_AddTxData( LOG_APP_SIM_PHONE_NR ";");
  Eth_Service_Client_AddTxData( LOG_APP_SIM_OPERATOR ";");
*/
  /* 
   * Add Bills channel 1
   * Add Bills channel 2
   * Add Bills channel 3
   * Add Bills channel 4
   * Add type of event
   /* 
   

                                                   1 = bill of type 1 validated
                                                   2 = bill of type 2 validated
                                                   3 = bill of type 3 validated
                                                   4 = bill of type 4 validated
                                                   S = Start dupa o pana de curent
                                                   B = blocat
                                                   P = ping
                                                   O = usa aparatului deschisa
                                                   C = usa aparatului inchisa
                                                   E = error
  */ 

  /* example:   "100;200;300;400;2;0;0;"       
  */

  /* add type of errors --> 0         */
  /* add reset confirmation --> 0     */

  //snprintf(arrDummy, 50,"%d;%d;%d;%d;%d;0;0;");
/*  
  Eth_Service_Client_AddTxData("0;0;0;0;A;0;1;");


  Eth_Service_Client_AddTxData( LOG_APP_VERSION_MAJOR ";");
  Eth_Service_Client_AddTxData( LOG_APP_TOWN ";");
  Eth_Service_Client_AddTxData( LOG_APP_PLACE ";");
  Eth_Service_Client_AddTxData( LOG_APP_DETAILS ";");
  Eth_Service_Client_AddTxData( LOG_APP_DEVICE_TYPE ";");
  Eth_Service_Client_AddTxData( LOG_APP_DEFAULT_RSSI);

  Eth_Service_Client_AddTxData( " HTTP/1.1");
*/
  /* [ToDo] ... check the return status !!!  */  

  //Eth_Service_Client_StackBuffRequest();

/*  Eth_Service_Client_SendRequest(); */
}



unsigned char Logger_App_Process_Buffer(void)
{

  unsigned char nReturn = 0;
  /* ------------------------------------------------------------
                   ! decode frame !

      encryped request is     28  00  CB  D8  FF                                               ( idx 0 ... 4 )
      encrypted response is   01  0B  8F  A6  D6  21  97  18  4F  57  B1  4B  E6  0B  17  18   ( idx 5 ... 20)
  */

  /*
   *  ... check that the size of the frame >= 21 bytes !!!!
   */

  if( (nLoggerMsgBuffIDX_Rx < _MAX_RX_COMM_BUFFER_SIZE_) && (nLoggerMsgBuffIDX_Rx >= 20) )
  {
    #if(LOGGER_APP_DEBUG_SERIAL == 1)
    //Serial.print(" ... decrypt 1 ... ");
    //Serial.print(arrLoggerReceiveBytes[0], HEX);
    //Serial.print(arrLoggerReceiveBytes[1], HEX);
    //Serial.print(arrLoggerReceiveBytes[2], HEX);
    //Serial.print(arrLoggerReceiveBytes[3], HEX);
    //Serial.print(arrLoggerReceiveBytes[4], HEX);
    //Serial.println();
    #endif

    /*
     * this is how an encrypted communication frame between main board and the NV9+ 
     *     - make sure the current received buffer contains a new frame that we look for ,,,
     */
    if( ( arrLoggerReceiveBytes[0] == 0x28 ) && ( arrLoggerReceiveBytes[1] == 0x00 ) && \
        ( arrLoggerReceiveBytes[2] == 0xCB ) && ( arrLoggerReceiveBytes[3] == 0xD8 ) && \
        ( arrLoggerReceiveBytes[4] == 0xFF ) )
    {

      /* decrypt only the NV9+ response */
      tool_crypto_decrypt(&arrLoggerReceiveBytes[7], 14);

      /* 
       * Event counter and the channel content is in the decoded buffer ... 
       */
      if( tool_NV9plus_utils_decode_status(&arrLoggerReceiveBytes[9], 11, nFirstRxFrame_AfterReset ) > 0 )  
      {
        nReturn = 1;         
      }

      /* reset first frame flag ... */
      nFirstRxFrame_AfterReset = 0;

      #if(LOGGER_APP_DEBUG_SERIAL == 1)
      //Serial.print(" ... decrypt 3 ... ");
      //Serial.print(arrLoggerReceiveBytes[9], HEX);
      //Serial.print(arrLoggerReceiveBytes[10], HEX);
      //Serial.print(arrLoggerReceiveBytes[11], HEX);
      //Serial.print(arrLoggerReceiveBytes[12], HEX);
      //Serial.print(arrLoggerReceiveBytes[13], HEX);
      //Serial.print(arrLoggerReceiveBytes[14], HEX);
      //Serial.print(arrLoggerReceiveBytes[15], HEX);
      //Serial.print(arrLoggerReceiveBytes[16], HEX);
      //Serial.print(arrLoggerReceiveBytes[17], HEX);
      //Serial.print(arrLoggerReceiveBytes[18], HEX);
      //Serial.print(arrLoggerReceiveBytes[19], HEX);
      //Serial.print(arrLoggerReceiveBytes[20], HEX);
      //Serial.println();
      #endif
    }
  }  

  return nReturn;

}



/* v0.2
 *
 *   !!! Main Logger state machine !!!
 *
 *  - function needs cyclic execution in order to process it's state machine
 *  - function has been adapted to be executed togather with the "COM" layer - 11.10.2023
 */
void Logger_App_main_v2(void)
{
  unsigned char  nReceiveCountBytes = 0;
  unsigned short nChanValTemp = 0;

  #if(LOGGER_APP_DEBUG_SERIAL == 1)
    Logger_App_Debug_Print_State();
  #endif

  switch(LoggerAppState)
  {

    /* 
     * currently nothing to do ... in this state ... go to init 
     */
    case E_LOGGER_APP_UNININT:

    /*
     *
     */
    case E_LOGGER_APP_INIT:
      LoggerAppState = E_LOGGER_APP_STARTUP_DELAY;

      /* startup delay required for all other layers to complete initialization ... */
      /*                  delay = 1ms * LOG_APP_STARTUP_DELAY                       */
      nLoggerAppStartupDelay = LOG_APP_STARTUP_DELAY;

    break;


    /* Startup delay used to 
     *   wait for all other initialization of internal devices
     *   like ETH, that can take several reccurences ... 
     */
    case E_LOGGER_APP_STARTUP_DELAY:

      /* if we need to continue in this waiting state ... */
      if(nLoggerAppStartupDelay > 0)
      {
        nLoggerAppStartupDelay --;
      }
      /* or we can go on ... */
      else
      {

        /*
         * Send a startup message ...
         */
        LoggerAppState = E_LOGGER_APP_PROCESS_MESSAGES;

        /* 
         * Internal variable initialization
         */
        
        itterUSART_withoutRX = 0;
        nLoggerMsgBuffIDX_Rx = 0;
        nNew_Comm_Frame_Consumed = 0;

        nFirstRxFrame_AfterReset = 1;
      }

    break;


    /*
     *  ToDo ----->>> split this HUGE STATE !!!!
     *
     */


    /******************************************************/
    /*     
    /*               Main processing state
    /*    
    /******************************************************/ 
    case E_LOGGER_APP_PROCESS_MESSAGES:

      /* returns 0 if no USART bytes have been received since last call !
       *   otherwise the user buffer index will also be updated ... indicating the last free position
       */
      nReceiveCountBytes = USART_Has_Received_Data();

      /************************************/
      /*      frame identification and    */
      /*      separation                  */
      /*                                  */
      /************************************/
      
      /* 
       * If we have something in the Rx buffer ... then prepare to detect a frame gap
       */      
      if(nReceiveCountBytes > 0)
      {
 
        #if(LOGGER_APP_DEBUG_SERIAL == 1)
          //Serial.println(" - Data received - ");
        #endif
        
        /* new data received .... reset inter-frame gap detector ... */
        itterUSART_withoutRX = 0;


        /*
         * indicates that there is no new frame that can be 
         *   processed by the user
         *
         */
        nNew_Comm_Frame_Consumed = 0;



        /* 
         * check to see if communication established with the server
         */
        nTransmissionToServer_IsPreparedForNewTransmission = Com_Service_Client_Is_Comm_Free(); 
      }
      else
      /************************************************************
       *
       *   ALL processing is done during communication gaps !
       *
       ************************************************************/
      {
        /* increment silence counter */
        if(itterUSART_withoutRX < _MAX_INTER_FRAME_TIME_)
        {
          /* a new itteration has ended without any Rx communication on the bus ... */
          itterUSART_withoutRX ++;
        }      
        /*
         *
         * Communication GAP detected !!! 
         *
         */
        else
        {
          /* an inter frame delay has been detected and the frame has not been consumed ... */
          if(  nNew_Comm_Frame_Consumed == 0 )
          {

            /* 
             *  look in the USART buffer for the full frame !
             */
            (void)USART_Receive_Bytes(arrLoggerReceiveBytes, &nLoggerMsgBuffIDX_Rx, _MAX_RX_COMM_BUFFER_SIZE_);


            #if(LOGGER_APP_DEBUG_SERIAL == 2)
              unsigned char idx;            
              for(idx = 0; idx < nLoggerMsgBuffIDX_Rx; idx ++)
              {
                Serial.print(arrLoggerReceiveBytes[idx], HEX);
                Serial.print(" ");
              }
              Serial.write(arrLoggerReceiveBytes, nLoggerMsgBuffIDX_Rx);
              Serial.println();
            #endif


            /* 
             * Try to identify frame and extract data if valid ... 
             */
            if( Logger_App_Process_Buffer() > 0 )
            {

              #if(LOGGER_APP_DEBUG_SERIAL == 1)
                snprintf(Logger_DebugPrintBuff, 50, "[I] Logg_App Log CH1=%d, CH1_O=%d, CH2=%d, CH2_O=%d, CH3=%d, CH3_O=%d, CH4=%d, CH4_O=%d, ERR=%d, ERR_O=%d", sNV9plus.nCH1_bill, nCH1_old, sNV9plus.nCH2_bill, nCH2_old, sNV9plus.nCH3_bill, nCH3_old, sNV9plus.nCH4_bill, nCH4_old, sNV9plus.nBillValidator_err, eMachineError_old);
                Serial.println(Logger_DebugPrintBuff);
              #endif


              /* !!! extract channel values !!! 
               *
               *    --- we have a CHANGE in the CHANNEL BILL content ??? 
               */
              if( ( ( nCH1_old < sNV9plus.nCH1_bill ) || ( nCH2_old < sNV9plus.nCH2_bill ) || \
                    ( nCH3_old < sNV9plus.nCH3_bill ) || ( nCH4_old < sNV9plus.nCH4_bill ) || \
                    (eMachineError_old != sNV9plus.nBillValidator_err) ) )
              {

                nLastBillType = 0;

                /*
                 *
                 * Check to see if there is a new bill accepted !
                 *
                 */
                if(nCH1_old != sNV9plus.nCH1_bill)
                {
                  nLastBillType = 1;
                  nNvM_RAM_Mirror_CH1 = sNV9plus.nCH1_bill;

                  /* updates must be stored in NvM also ... */
                  NvM_Write_Block(1);
                }
                else
                {
                  if(nCH2_old != sNV9plus.nCH2_bill)
                  {
                    nLastBillType = 2;
                    nNvM_RAM_Mirror_CH2 = sNV9plus.nCH2_bill;

                    /* updates must be stored in NvM also ... */                    
                    NvM_Write_Block(2);
                  }
                  else
                  {
                    if(nCH3_old != sNV9plus.nCH3_bill)
                    {
                      nLastBillType = 3;
                      nNvM_RAM_Mirror_CH3 = sNV9plus.nCH3_bill;

                      /* updates must be stored in NvM also ... */                      
                      NvM_Write_Block(3);
                    }
                    else
                    {
                      nLastBillType = 4;
                      nNvM_RAM_Mirror_CH4 = sNV9plus.nCH4_bill;

                      /* updates must be stored in NvM also ... */
                      NvM_Write_Block(4);
                    }
                  }
                }


                /* *******************  */
                /*                      */
                /*   we have new data   */
                /*  ... must inform     */
                /*    server            */
                /*                      */
                /* *******************  */

                /* Has been an error detected ? */
                if(sNV9plus.eNV9State > NV9_STATE_RUN_NO_ERROR)
                {

                  #if(LOGGER_APP_DEBUG_SERIAL == 1)
                    Serial.println("[I] Logg_App ---------- ERROR ------------");
                  #endif

                  AT_Command_Callback_Request_ERROR_Status_Transmission();
                  
                }
                else
                /* inform server about a new bill reception ... */
                {

                  #if(LOGGER_APP_DEBUG_SERIAL == 1)
                    Serial.println("[I] Logg_App ---------- DATA BILLS ------------");
                  #endif

                  AT_Command_Callback_Request_Bill_Status_Transmission();
                }

                nCH1_old = sNV9plus.nCH1_bill;
                nCH2_old = sNV9plus.nCH2_bill;
                nCH3_old = sNV9plus.nCH3_bill;
                nCH4_old = sNV9plus.nCH4_bill;
                nCH5_old = sNV9plus.nCH5_bill;

                eMachineError_old = sNV9plus.nBillValidator_err;
                eMachineState_old = sNV9plus.eNV9State;

                nTransmissionToServer_IsPreparedForNewTransmission = 0;

              } /* end if (nCH1_old < sNV9plus.nCH1_bill) || (nCH2_old < sNV9plus.nCH2_bill) ||  ....*/

            }/* end if(Logger_App_Process_Buffer() > 0) */


            nLoggerMsgBuffIDX_Rx = 0;

            /* indicates that the last received frame has been processed already !  ... and we must wait for a new frame communication */
            nNew_Comm_Frame_Consumed = 1;
          }

        }


      }/* end else if(nReceiveCountBytes > 0) */

    break;


    default: 
      LoggerAppState = E_LOGGER_APP_INIT;
    break;

  }/* end switch LoggerAppState */


}/* void Logger_App_main_v2(void) */



/***********************************************************************************************************
 *
 *                    OUTPUT PINS LOGIC / used to reset (cut power supply of NV10)
 *                           if an reset command is issued by the server !
 *
 ***********************************************************************************************************/

T_LoggerAppOutputPinState LoggerAppOutputState_PIN_RESET = E_LOGGER_OUTPUT_INNACTIVE;
unsigned short nOutputPinResetBill_Delay = OUTPUT_PIN_RESET_BILL_DELAY_ITTER;


void Logger_App_Output_Pins_init(void)
{
  /* configure pins ... */
//  pinMode(OUTPUT_PIN_RESET_BILL, OUTPUT);
//  digitalWrite(OUTPUT_PIN_RESET_BILL, LOW);

  nOutputPinResetBill_Delay = OUTPUT_PIN_RESET_BILL_DELAY_ITTER;

  /* startup value ... */
  LoggerAppOutputState_PIN_RESET = E_LOGGER_OUTPUT_INNACTIVE;
}


/* Start a reset pulse ... 
 */
void Logger_App_Outputs_SetReset(void)
{

  LoggerAppOutputState_PIN_RESET = E_LOGGER_OUTPUT_PULSE_REQ_L_H;
} 

/*
 * Function used to actuate board outputs depending on 
 *  main function logic ... 
 */
void Logger_App_Output_Pins_main(void)
{

  switch( LoggerAppOutputState_PIN_RESET )
  {
    case E_LOGGER_OUTPUT_INNACTIVE:

      digitalWrite(OUTPUT_PIN_RESET_BILL, LOW);    
      break;

    case E_LOGGER_OUTPUT_PULSE_REQ_L_H:

      /* activate PIN !!! */
      digitalWrite(OUTPUT_PIN_RESET_BILL, HIGH);
      nOutputPinResetBill_Delay = OUTPUT_PIN_RESET_BILL_DELAY_ITTER;

      LoggerAppOutputState_PIN_RESET = E_LOGGER_OUTPUT_PULSE_REQ_DELAY;      
      break;

    case E_LOGGER_OUTPUT_PULSE_REQ_DELAY:
      /* counter -- */
      if(nOutputPinResetBill_Delay > 0)
      {
        nOutputPinResetBill_Delay --;
      }
      else
      {
        LoggerAppOutputState_PIN_RESET = E_LOGGER_OUTPUT_PULSE_REQ_H_L;
      }
      break;

    case E_LOGGER_OUTPUT_PULSE_REQ_H_L:

      /* de-activate pin ... */
      digitalWrite(OUTPUT_PIN_RESET_BILL, LOW);

      LoggerAppOutputState_PIN_RESET = E_LOGGER_OUTPUT_INNACTIVE;
      break;

    default:
      break;

  }/* end switch( LoggerAppOutputState_PIN_RESET ) */
  
}/* end void Logger_App_Output_Pins_main(void) */


