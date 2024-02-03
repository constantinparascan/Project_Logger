#include "60_GSM_SIM900.h"
#include "60_GSM_SIM900_cfg.h"

#include "01_Debug_Utils.h"

/* used for COM callbacks
 */
#include "03_Com_Service.h"

#include <avr/io.h>
#include <avr/interrupt.h>



#define USART_RX_AT_BUFFER_LENGTH (500)
#define USART_TX_AT_BUFFER_LENGTH (500)

#define MAX_AT_SIM_COMM_SILENCE_TIMER_DELAY (100) /* 5ms * 100 = 500ms */

/*************************************************/
/*
 *    Communication buffers ... 
 */

volatile unsigned char *ptrAT_Command_Tx;
volatile unsigned short nAT_Command_Tx_Idx = 0;
volatile unsigned char  nAT_Command_Tx_Complete = 0;

volatile unsigned char arr_AT_Generic_BuffTx[USART_TX_AT_BUFFER_LENGTH] = {0};

/* pointer to expected response ... string terminated with NULL char */
volatile unsigned char *ptrAT_Response_Rx;

volatile unsigned char  arr_AT_Resp_BuffRx[USART_RX_AT_BUFFER_LENGTH] = {0};
volatile unsigned short nAT_Resp_Rx_Idx = 0;
volatile unsigned char  nAT_Resp_Rx_Complete = 0;
volatile unsigned char  nAT_Resp_Rx_Buffer_Full = 0;
volatile unsigned short nAT_Silence_In_Comm_Rx_Timer = MAX_AT_SIM_COMM_SILENCE_TIMER_DELAY;
volatile unsigned short nAT_Delay_AfterCommand = 0;

/**************************************************/

/*
 * internal state of the AT command processor ... 
 */
static volatile T_GSM_SIM900_STATE eAT_Processor_State;

unsigned short nAT_Power_On_Before_Startup_Delay = MAX_BEFORE_POWER_ON_DELAY;
unsigned short nAT_Power_On_Switch_Delay = MAX_POWER_ON_LINE_HIGH_LEVEL_DELAY;
unsigned short nAT_Reset_On_Switch_Delay = MAX_RESET_LINE_HIGH_LEVEL_DELAY;
unsigned short nAT_After_Reset_Delay     = MAX_AFTER_RESET_TIME_DELAY;


/*
 * Error Flags ...
 */

#define AT_ERROR_FLAG_CRITICAL_UNRECOVERABLE_FULL_RESET (0x8000)
#define AT_ERROR_FLAG_CRITICAL_RECOVERABLE_SIM900_RESET (0x4000)

#define AT_ERROR_FLAG_TX_ERROR_IDX             ( 0 )
#define AT_ERROR_FLAG_RX_ERROR_IDX             ( 1 )
#define AT_ERROR_FLAG_COMMAND_RESP_ERROR_IDX   ( 2 )
#define AT_ERROR_FLAG_TX_STEP1_ERROR_IDX             ( 3 )
#define AT_ERROR_FLAG_RX_STEP1_ERROR_IDX             ( 4 )
#define AT_ERROR_FLAG_COMMAND_RESP_STEP1_ERROR_IDX   ( 5 )

#define AT_ERROR_FLAG_TX_ERROR                 ( 0x0001 << AT_ERROR_FLAG_TX_ERROR_IDX )               /* could not transmit message to SIM900          */
#define AT_ERROR_FLAG_RX_ERROR                 ( 0x0001 << AT_ERROR_FLAG_RX_ERROR_IDX )               /* could not receive any message from SIM900     */
#define AT_ERROR_FLAG_COMMAND_RESP_ERROR       ( 0x0001 << AT_ERROR_FLAG_COMMAND_RESP_ERROR_IDX )     /* SIM900 command has been responded with ERROR  */
#define AT_ERROR_FLAG_TX_STEP1_ERROR                 ( 0x0001 << AT_ERROR_FLAG_TX_STEP1_ERROR_IDX )               /* could not transmit message to SIM900          */
#define AT_ERROR_FLAG_RX_STEP1_ERROR                 ( 0x0001 << AT_ERROR_FLAG_RX_STEP1_ERROR_IDX )               /* could not receive any message from SIM900     */
#define AT_ERROR_FLAG_COMMAND_RESP_STEP1_ERROR       ( 0x0001 << AT_ERROR_FLAG_COMMAND_RESP_STEP1_ERROR_IDX )     /* SIM900 command has been responded with ERROR  */


static unsigned short nAT_ERROR_Flags = 0x0000;
static unsigned char  nAT_arrErrorCount[8] = {0};   /* each flag indicating an error has a counter attached !!! */
                                                    /* since there are only 8 possible errors -> short - we use only 8 counters */

/*
 * Flag indicating that a transmission is complete and a server response has been received
 */
unsigned char nIsGSM_Command_Complete = 0;

unsigned char nLastServerResponse_Code = 0;
unsigned char nLastServerResponse_Code_Not_Processed = 0;
unsigned char arrLastServerResponse_Data[ AT_MAX_DATA_FROM_SERVER_COMMAND ] = {0};
unsigned char nLastServerResponse_Data_Len = 0;



unsigned char nstrProcessIdx;

/* index used to loop inside init AT command sequence ...  arrAT_GsmInitCmdSet */
unsigned char  nAT_Command_Init_Sequence = 0;
/* index used to loop inside a normal frame transmission sequence ... arrAT_Gsm_COMAND_Step1 */
unsigned char  nAT_Command_Step1_Sequence_Idx = 0;
unsigned short nAT_Command_Transmission_Wait_Counter = 0;
unsigned short nAT_Command_Response_Wait_Counter = 0;


unsigned short nAT_RSSI_Extractor_Counter = AT_RSSI_DELAY_COUNTER;
unsigned char  nAT_RSSI_Signal_Quality_Value = 0;
unsigned char  nAT_RSSI_Signal_Quality_Update_Flag = 0;


/*************************************************/
/*                USART Interrupts               */


/* Communication Interrupts are available only if no Debugging and Initialization active */
#if ( BOARD_DIAGNOSTIC_AND_INITIALIZATION_ENABLE == 0 )

/*  USART Tx ISR
 *  v0.1
 *
 *  USART 3 Data Register Empty ISR 
 * 
 *    - used to transmit an string buffer - NULL terminated
 *    - after the Tx buffer is completely transmitted the ISR is disabled 
 *        and must be re-enabled by the next transmission sequence !
 */
ISR(USART3_UDRE_vect)
{

  /*
   * check to see if there is something to transmit ! 
   *
   *   ... all transmitted strings needs to pe NULL terminated 
   */
  if( *(ptrAT_Command_Tx + nAT_Command_Tx_Idx) != '\0' )
	{
    /* fill in data to be transmitted */
		UDR3 = *(ptrAT_Command_Tx + nAT_Command_Tx_Idx);

		nAT_Command_Tx_Idx ++;
	}
	else
  /*
   * ... all buffer has been transmitted ...
   * ... close Transmitter
   */
	{
		/* clear UDRIE - USART Data Register Empty Interrupt Eanable -->  UDRIE3 <- 0  */
		/* clear Transmitter Enable                                  -->  TXEN3  <- 0  */

		/*        1101 0111                                   */
    //nTemp = UCSR3B;
    //nTemp = nTemp & 0xD7;
		UCSR3B &= 0xD7;

		/* complete frame transmited ... mark flag */
		nAT_Command_Tx_Complete = 1;


    /*
     * [ ----------------------------------------------    ToDo ] ---> OPTIMIZATION !!! Reset reception .... so you won't see the ECHO !!!
     *                     - shorter string processing means lower execution time !!!     
     *
     */
    

    /* rewind Tx character idx used by the logic */
    //nAT_Command_Tx_Idx = 0;
	}

} /* ISR(USART3_UDRE_vect) */



/* USART RX ISR
 * v0.1
 *    - if a new byte is received from USART ... then we move-it in the Rx buffer and 
 *       update the "new data" flag !
 */
volatile unsigned char nLocal_USART_Rx;
ISR(USART3_RX_vect)
{
 
  nLocal_USART_Rx = UDR3;

  /* if we did not reaced the buffer limit ... */
  /*
   * ALL the "SPECIAL" charcaters are filtered out !!!!
   */
  if( ( nAT_Resp_Rx_Idx < (USART_RX_AT_BUFFER_LENGTH - 1) ) && (nLocal_USART_Rx > 0x1F) && (nLocal_USART_Rx < 0x7F) )
  {

    /* new character received ... store-it in the buffer ... */
    arr_AT_Resp_BuffRx[nAT_Resp_Rx_Idx] = nLocal_USART_Rx;
    arr_AT_Resp_BuffRx[nAT_Resp_Rx_Idx + 1] = '\0';

    nAT_Resp_Rx_Idx ++;

    /*
     * something new received ... reset Silence timer ... 
     *
     *   ... the silence timer is reset only here ... this is intended to mark that a full unprocessed buffer 
     *       is not taken into consideration for reception 
     */
    nAT_Silence_In_Comm_Rx_Timer = MAX_AT_SIM_COMM_SILENCE_TIMER_DELAY;
  }
  else
  {
    /* !!!  buffer overrun !!!  */
    /* mark as communication error ... buffer too small !!!! */

    nAT_Resp_Rx_Buffer_Full = 1;        
  }   

} /* ISR(USART3_RX_vect) */

#endif /* ( BOARD_DIAGNOSTIC_AND_INITIALIZATION_ENABLE == 0 ) */



/* ============================================================================== */
/* Private API's */

void AT_Command_Processor_Enable_Transmission(void);
void AT_Send_Command_Expect_Response_InitSeq(unsigned char *strCmd, unsigned char *strResp);
unsigned char AT_Command_Decode_Rx_Buff(void);
unsigned char AT_Command_Decode_Rx_Buff_v2(void);
unsigned short AT_Command_Decode_CSQ_Rx_Buff(unsigned char* strResp, unsigned short nMaxIdx);

void AT_Command_Error_Handling(void);

unsigned char ArrDebugEcho[255];
void AT_Debug_Echo(void);


/*************************************************/


/*
 * Configure USART [3] for serial communication 
 *       - RX - used on interrupt
 *       - TX - used on interrupt
 */
void AT_Command_Processor_Init(void)
{

    unsigned char nIdx = 0;

    /* disable power reduction ... */
    PRR0 = 0x00;
    PRR1 = 0x00;


    /*  U2X3: Double the USART Transmission Speed
     *
     */
     UCSR3A |= ( (1 << TXC3) | (1 << U2X3) );

    /* Usart working mode: asynchronous, parity disabled, 1 stop bit, 8 bit length
     *
     */
     UCSR3C |= (1 << UCSZ31) | (1 << UCSZ30);


    /* Fosc = 16.0000MHz
    /*=======================
     * Speed = 9600
     * U2X3 = 1
     * UBRR = 207  = 0xCF;
     * Error = 0.2%
     */
//     UBRR3H = 0x00;
//     UBRR3L = 0xCF;

    /*=========================
     * Speed = 115200
     * U2X3 = 1
     * UBRR = 16  = 0x10;
     * Error = 2.1%
     */
    UBRR3H = 0x00;
    UBRR3L = 0x10;



    /* Init Tx Indexes ... 
     */
    nAT_Command_Tx_Idx = 0;
    nAT_Command_Tx_Complete = 0;

    /* init Rx Indexes and flags ...
     */
    nAT_Resp_Rx_Idx = 0;
    nAT_Resp_Rx_Complete = 0;
    nAT_Resp_Rx_Buffer_Full = 0;
    nAT_Silence_In_Comm_Rx_Timer = MAX_AT_SIM_COMM_SILENCE_TIMER_DELAY;


    /*
     * ... during startup ... request faster the RSSI signal value ... 
     */    
    nAT_RSSI_Extractor_Counter = 10;

    /*
     * Startup sequence will start from index "0" of command array ... 
     */
    nAT_Command_Init_Sequence = 0;

    /* startup state .... */    
    eAT_Processor_State = AT_STATE_INIT_BEFORE_STARTUP_DELAY;



    /**********************************
     *
     *   ENABLE USART TX, RX    !!!
     *
     **********************************/


    /*  RXCIEn: RX Complete Interrupt Enable - Enable Rx interrupt generation
     *  TXCIEn: TX Complete Interrupt Enable --- only later when we want to transmit something to SIM900 module
     *  RXENn: Receiver Enable - Enable USART Tx and Rx
     *  TXENn: Transmitter Enable --- only later when we want to send something to the SIM900 module
     */
     UCSR3B |= ( (1 << RXCIE3) /* | (1 << TXCIE3) | (1 << UDRIE) */| (1 << RXEN3) | (1 << TXEN3) );



    /* 
     * Configure HW control lines as OUTPUT
     */
    pinMode(ARDUINO_DIG_OUT_RESET_LINE,    OUTPUT);  
    pinMode(ARDUINO_DIG_OUT_POWER_LINE,    OUTPUT);  

    digitalWrite( ARDUINO_DIG_OUT_RESET_LINE, 0 );
    digitalWrite( ARDUINO_DIG_OUT_POWER_LINE, 0 );

    nAT_Power_On_Before_Startup_Delay = MAX_BEFORE_POWER_ON_DELAY;

    /*
    * reset error flags 
    */    
    nAT_ERROR_Flags = 0x0000;

    for(nIdx = 0; nIdx < 8; nIdx ++)
    {
      nAT_arrErrorCount[nIdx] = 0;
    }


    #if(AT_COMMAND_PROCESSOR_GSM_SIM900_DEBUG_SERIAL_ENABLE >= 1)
      Serial.println("[I] GSM SIM900 Debug ... ----------------- ");
    #endif

}/* AT_Command_Processor_Init */


/* Used to activate the TX interrupt ... that will process the Tx buffer
 *
 */
void AT_Command_Processor_Enable_Transmission(void)
{
	//UCSR3B |= ( (1 << UDRIE3) | (1 << TXEN3) );
  UCSR3B |= ( (1 << RXCIE3) | (1 << UDRIE3) | (1 << RXEN3) | (1 << TXEN3) );
}


/* Reset Reception buffer indexes and flags
 *  ... prepare to receive new data ... 
 */
void AT_Command_Processor_Reset_Reception(void)
{
  nAT_Resp_Rx_Complete = 0;
  nAT_Resp_Rx_Idx = 0;
  arr_AT_Resp_BuffRx[0] = '\0';
  arr_AT_Resp_BuffRx[1] = '\0';
  
  nAT_Resp_Rx_Buffer_Full = 0;

  nAT_Silence_In_Comm_Rx_Timer = MAX_AT_SIM_COMM_SILENCE_TIMER_DELAY;
}

/*=================================================================================*/

#define MAX_INIT_SEQUENCE_LENGTH (19)
#define MAX_COMMAND_STEP_1 (3)

#define NO_SKIP_NEXT_CMD      (0)
#define SKIP_NEXT_CMD_IF_TRUE (1)

typedef struct Tag_AT_GSM_INIT_COMMAND_SET
{
	unsigned char *str_TxCommand;                 /* command to be sent to SIM900  */
	unsigned char *str_Exp_Rx_Resp_OK;            /* expected response in case command has been sucessfully executed */
  unsigned char *str_Exp_Rx_Resp_Retry_1;       /* not used ... */
  unsigned char *str_Exp_Rx_Resp_Continue;      /* in case this response is received ... next command is executed ... this providees flexibility in case several responses req different actions ... */

	unsigned short nWaitRespTimer;                /* delay required to wait for the SIM900 to provide an answer on the RX line ... */

  unsigned char nSkipNextCmdIfOK;

  unsigned short nDelayAfterCommand;            /* delay between commands ... */

}T_AT_GsmInitCmdSet;


/*
 * after POWER ON - OK sequence that worked:
 *
 *  AT             -> OK
 *  AT+CPIN=0000   -> OK    ----------> wait 10-15 sec !!!! for connection to occure
 *
 *  AT+CGATT?      -> +CGATT: 1  OK
 *
 *  AT+SAPBR=3,1,"Contype","GPRS"    -> OK
 *  AT+SAPBR=3,1,"APN","net"         -> OK
 *  AT+SAPBR=1,1                     -> OK
 *
 *  AT+HTTPINIT                      -> OK
 *  AT+HTTPPARA="CID",1              -> OK
 *  AT+HTTPACTION=0                  -> OK
 *                                      +HTTPACTION:0,200,1159
 *  AT+HTTPREAD                      -> +HTTPREAD:1159 <!DOCTYPE .....</html> OK
 */

/*
 * INIT sequence 
 *      - contains AT commands one after another with the corresponding waiting time
 */
const T_AT_GsmInitCmdSet arrAT_GsmInitCmdSet[ MAX_INIT_SEQUENCE_LENGTH ] = \
{ \
	{"AT\r", "OK", "", "", 200, NO_SKIP_NEXT_CMD, 200 }, \
  {"AT+CFUN=1,1\r", "", "", "", 500, NO_SKIP_NEXT_CMD, 1000 },\
  {"AT\r", "OK", "", "", 200, NO_SKIP_NEXT_CMD, 200 }, \
  {"AT+CPIN?\r", "+CPIN: READY", "", "+CPIN: SIM PIN", 200, SKIP_NEXT_CMD_IF_TRUE, 200 }, \
	{"AT+CPIN="AT_SIM_CARD_PIN_NR"\r", "OK", "", "", 1000, NO_SKIP_NEXT_CMD, 5000 }, \

/* wait until connected to an opperator ....
   +COPS: 0 OK --> no connection
   +COPS: 0,0,"orange"
   --> new modules SIM900  --> +COPS:0,0,"ora"
   {"AT+COPS?\r", "orange", "", "", 5000, NO_SKIP_NEXT_CMD, 1000}, \
*/

  {"AT+COPS?\r", "ora", "", "", 5000, NO_SKIP_NEXT_CMD, 200 }, \
  /* This is STUPID for ORANGE !!!!
   *
   *    this idiots change from sim to sim the COPS prams from "ora" to "orange" to "RO ORANGE" ... I wonder how many combinations are possible !!!!
   */


	{"AT+CGATT?\r", "+CGATT: 1","", "", 1000, SKIP_NEXT_CMD_IF_TRUE, 200 },  \
	{"AT+CGATT=1\r", "OK", "", "", 1000, NO_SKIP_NEXT_CMD, 1000 },  \

  {"AT+SAPBR=3,1,\"Contype\",\"GPRS\"\r", "OK", "", "", 200, NO_SKIP_NEXT_CMD, 200 }, \
  {"AT+SAPBR=3,1,\"APN\",\"net\"\r", "OK", "", "", 200, NO_SKIP_NEXT_CMD, 200 }, \
 /*
  * Orange uses APN "internet" or "net", "net-fix"
  */

  /* check registration status ... */
  {"AT+CREG?\r","", "", "", 200, NO_SKIP_NEXT_CMD, 100},  \
  {"AT+CGREG?\r","", "", "", 200, NO_SKIP_NEXT_CMD, 100}, \

  {"AT+SAPBR=1,1\r", "OK", "", "", 500, NO_SKIP_NEXT_CMD, 200 }, \
 /* <-_______________________________________________------------------------ particularity module 7 !!! --- response ??? unknown received  !!!! */
 /* {"AT+SAPBR=1,1\r", "", "", "", 500, NO_SKIP_NEXT_CMD, 500 }, \*/
  {"AT+SAPBR=2,1\r", "OK", "", "", 500, NO_SKIP_NEXT_CMD, 200 }, \

  {"AT+HTTPINIT\r", "OK", "", "", 1000, NO_SKIP_NEXT_CMD, 100 }, \
  {"AT+HTTPPARA=\"CID\",1\r", "OK", "", "", 200, NO_SKIP_NEXT_CMD, 200 }, \
  {"AT+HTTPPARA=\"URL\",\"http://145.239.84.165:35269/gsm/index.php\"\r", "OK", "", "", 200, NO_SKIP_NEXT_CMD, 200 }, \
  {"AT+HTTPACTION=0\r", "OK", "", "", 8000, NO_SKIP_NEXT_CMD, 200 }, \
  /*
   * +HTTPACTION:0,603,0
   *
   *    0 -> GET
   *    603 -> error code -> DNS Error
   *    0 -> the length of data got
   *
   *   <<<<<<<< RETRY this step until OK is received !!!!! <<<<<<<    >>>>  +HTTPACTION:0,200,1159    -> "200" means OK !!!

   *
   *   >>>> WAIT for "+HTTPACTION:***,200,****" ... if anything else is received ... retry .... or wait some more !!! --> nAT_Silence_In_Comm_Rx_Timer  <<<< RESET !!!
   *

   */
  {"AT+HTTPREAD\r", "!DOCTYPE", "", "", 5000, NO_SKIP_NEXT_CMD, 100 }
};

/*
 *  a simple URL AT command transmission sequence ...
 *
 */
T_AT_GsmInitCmdSet arrAT_Gsm_COMAND_Step1 [ MAX_COMMAND_STEP_1 ] = 
{
  {"", "OK", "", "", 100, NO_SKIP_NEXT_CMD, 100 },
  {"AT+HTTPACTION=0\r", "OK", "", "", 100, NO_SKIP_NEXT_CMD, 100 },
  {"AT+HTTPREAD\r", "****", "", "", 200, NO_SKIP_NEXT_CMD, 100 }         /* <<<<<<<<<<<<< robustness actions required !!!!    [TODO !!!!] */
                                                                         /*
                                                                          *   >>>>> search for CMD or *** !!!! or nothing to search !!!! no !DOCTYPE is returned
                                                                          */  
};


T_AT_GsmInitCmdSet arrAT_Gsm_COMMAND_RSSI[ 1 ] = 
{
  {"AT+CSQ\r", "OK", "", "", 100, NO_SKIP_NEXT_CMD, 0}
};

unsigned char arrAT_Gsm_COMMAND_Generic_HTTP_Request[200];

void AT_Send_Command_Expect_Response_InitSeq(unsigned char *strCmd, unsigned char *strResp)
{

  /*
   * update local pointers to the command and response ... 
   */
	ptrAT_Command_Tx = strCmd;
	ptrAT_Response_Rx = strResp;

  /* start from Idx 0 of first character to be sent         */
  nAT_Command_Tx_Idx = 0;

  /* reset Tx complete flag ...  */
  nAT_Command_Tx_Complete = 0;

  /* flag indicating a new request has been issued to SIMxxx */
//	nIsGSM_Command_Request = 1;

  AT_Command_Processor_Enable_Transmission();

}/* AT_Send_Command_Expect_Response */

/**************************************************************************/


/*
 * API used to trigger an HTTP request
 */
unsigned char AT_Command_Processor_Send_URL_Message(char* strMessage)
{
  unsigned char nReturn = 0;

  /* request is accepted only if AT internal state machine is iddle ...
   */
  if( eAT_Processor_State == AT_STATE_PROCESSOR_IDDLE )
  {
    if( strlen(strMessage) < USART_TX_AT_BUFFER_LENGTH )
    {
      strcpy(arr_AT_Generic_BuffTx, strMessage);
    }

		/* start from index 0 ... */
    nAT_Command_Step1_Sequence_Idx = 0;

    eAT_Processor_State = AT_STATE_PROCESSOR_SEND_STEP1_PRE_COMMAND_SEQ;

    /* restart counter ... after a communication we wait again until we req RSSI value ... */
    nAT_RSSI_Extractor_Counter = AT_RSSI_DELAY_COUNTER;

    nReturn = 1;
  }

  return nReturn;
}/* end AT_Command_Processor_Send_URL_Message */


/* 
 * API used to trigger an RSSI (Received Signal Strength Indicator) value retrieval 
 */
unsigned char AT_Command_Request_RSSI(void)
{
  unsigned char nReturn = 0;

  if( eAT_Processor_State == AT_STATE_PROCESSOR_IDDLE )
  {
    eAT_Processor_State = AT_STATE_PROCESSOR_SEND_RSSI_LEVEL;

    nReturn = 1;
  }

  return nReturn;
}/* end AT_Command_Request_RSSI */


/*
 * API used to indicate if a new value of RSSI has been retrieved
 */
unsigned char AT_Command_Is_New_RSSI_Computed(void)
{
  return nAT_RSSI_Signal_Quality_Update_Flag;
}


/* 
 * API used to retrieve the RSSI value from local buffer after receiveing it from SIM900
 * 
 *    - map the RSSI value received from SIM900 : 0,1, ... 31, 99 to a value that can be used by the current server application
 */
unsigned char AT_Command_GET_RSSI_Level(void)
{
  unsigned char nMapped_RSSI_Signal_Value = 0;

  nAT_RSSI_Signal_Quality_Update_Flag = 0;


  /* Server mappings:
   *
      'rssi_semnal_fslab' < 14; // sub aceasta valoare, pentru $app['frecventa_semnal_slab'] consecutive se va da un mesaj 
      'rssi_semnal_slab'  < 18;
      'rssi_semnal_mediu' < 23;
      'rssi_semnal_bun'   < 26;
      'rssi_semnal_fbun'  < 39;   
   *
   */

  /* 
   * !! verry week signal !!
   */
  if( (nAT_RSSI_Signal_Quality_Value > 90) || (nAT_RSSI_Signal_Quality_Value <= 1) )
  {
    /* actually any value < 14 will do ... */
    nMapped_RSSI_Signal_Value = 1;
  }
  else
  {

   /* 
    * !! week signal !!
    */
    /* RSSI <= -100 dBm --> according to SIM900 book ... -100 dBm = 7*/
    if( nAT_RSSI_Signal_Quality_Value <= 7 )
    {
      /* actually any value < 18 will do ... */
      nMapped_RSSI_Signal_Value = 16;
    }
    else
    {

     /* 
      * !! medium signal !!
      */
      /* RSSI <= -84 dBm --> according to SIM900 book ... -84 dBm = 15*/
      if( nAT_RSSI_Signal_Quality_Value <= 15 )
      {
        /* actually any value < 23 will do ... */
        nMapped_RSSI_Signal_Value = 20;
      }
      else
      {
        /* 
         * !! good signal !!
         */
         /* RSSI <= -70 dBm --> according to SIM900 book ... -70 dBm = 22*/
        if( nAT_RSSI_Signal_Quality_Value <= 22 )
        {
          /* actually any value < 26 will do ... */
          nMapped_RSSI_Signal_Value = 25;
        }
        else
        {
          /* 
           * !! excellent signal !!
           */
          nMapped_RSSI_Signal_Value = 35; 
        }
      }
    }
  }

  return nMapped_RSSI_Signal_Value;
  
}/* end AT_Command_GET_RSSI_Level */


/* Re-implementation of "strstr" with the addition that we are not sensitive to string "case" of the letters
 *    - also I'm aware that by limiting the range with the mask 0x1F it is possible to match special chars or
 *      or different chars that are not equal to what we are looking for ... but !! it's fast and it will fail
 *      for sure since GSM suppliers will have no special chars in their name !
 */
char * AT_Command_strstr_case_insensitive(const char *str, const char *substring)
{
    const char *a;
    const char *b;

    b = substring;

    if (*b == 0) 
    {
        return (char *) str;
    }

    for ( ; *str != 0; str += 1) 
    {
        /* check if chars are different, check if Lower 5 bits are the same - in case of upper and lower case litterals */
        if ( (*str != *b) && ( (*str & 0x1F) != (*b & 0x1F) ) )
        {
            continue;
        }

        a = str;
        while (1) 
        {
            if (*b == 0) 
            {
                return (char *) str;
            }

            /* check if chars are different, check if Lower 5 bits are the same - in case of upper and lower case litterals */
            if ( (*a != *b) && ( (*a & 0x1F) != (*b & 0x1F) ) )
            {
                break;
            }
            else
            {
              a++;
              b++;
            }            
        }

        b = substring;
    }

    return NULL;
}

/* 
 * Function that checks the status of error flags and sets the "reset" request flag if needed 
 */
void AT_Command_Error_Handling(void)
{
  
  if( ( nAT_arrErrorCount[AT_ERROR_FLAG_COMMAND_RESP_ERROR_IDX] >= AT_MAX_ERROR_RETRY_COUNTER ) || ( nAT_arrErrorCount[AT_ERROR_FLAG_COMMAND_RESP_STEP1_ERROR_IDX] >= AT_MAX_ERROR_RETRY_COUNTER ) )
  { 
    nAT_ERROR_Flags |= AT_ERROR_FLAG_CRITICAL_RECOVERABLE_SIM900_RESET;
  }

  if( ( nAT_arrErrorCount[AT_ERROR_FLAG_TX_ERROR_IDX] >= AT_MAX_ERROR_RETRY_COUNTER ) || \
      ( nAT_arrErrorCount[AT_ERROR_FLAG_RX_ERROR_IDX] >= AT_MAX_ERROR_RETRY_COUNTER ) || \
      ( nAT_arrErrorCount[AT_ERROR_FLAG_TX_STEP1_ERROR_IDX] >= AT_MAX_ERROR_RETRY_COUNTER ) || \
      ( nAT_arrErrorCount[AT_ERROR_FLAG_RX_STEP1_ERROR_IDX] >= AT_MAX_ERROR_RETRY_COUNTER ) )
      {


        nAT_ERROR_Flags |= AT_ERROR_FLAG_CRITICAL_UNRECOVERABLE_FULL_RESET;

        nAT_ERROR_Flags |= AT_ERROR_FLAG_CRITICAL_RECOVERABLE_SIM900_RESET;         /* <<<< remove this and leave for full reset !!! */
      }


  #if(AT_COMMAND_PROCESSOR_GSM_SIM900_DEBUG_SERIAL_ENABLE >= 2)
    Serial.println();  
    Serial.print("[I] SIM900 Error counters drop: ");
    Serial.print(nAT_arrErrorCount[0]);
    Serial.print(" ");
    Serial.print(nAT_arrErrorCount[1]);
    Serial.print(" ");    
    Serial.print(nAT_arrErrorCount[2]);
    Serial.print(" ");    
    Serial.print(nAT_arrErrorCount[3]);
    Serial.print(" ");    
    Serial.print(nAT_arrErrorCount[4]);
    Serial.print(" ");    
    Serial.print(nAT_arrErrorCount[5]);
    Serial.print(" ");    
    Serial.print(nAT_arrErrorCount[6]);
    Serial.print(" ");    
    Serial.print(nAT_arrErrorCount[7]);
    Serial.println(); 
  #endif


}

void AT_Command_Request_Reset_SIM(void)
{
  nAT_ERROR_Flags |= AT_ERROR_FLAG_CRITICAL_RECOVERABLE_SIM900_RESET;


  #if(AT_COMMAND_PROCESSOR_GSM_SIM900_DEBUG_SERIAL_ENABLE == 1)
    Serial.println("[I] SIM900 ERROR !!! - RESET requested from COM due to deadline message monitoring ...");
  #endif

}

/*=================================================================================*/


/* 
 * SIM900 driver main function
 *
 *    - implements the full communication protocol with SIM900 using AT commands
 *    - must be ciclically called inside a scheduler - currently 5ms reccurence task
 */
void AT_Command_Processor_Main(void)
{

  unsigned char* strLocalPtr = 0;
  unsigned char nIdx = 0;

  /*
   *  Process ... state independent timers ...
   *     - timer is reset each time a new character is received over USART
   */
  if( nAT_Silence_In_Comm_Rx_Timer > 0 )
  {
    nAT_Silence_In_Comm_Rx_Timer --;
  }


  /*
   * handle errors !!!
   */
  AT_Command_Error_Handling();


  /*
   * Force a state-machine reset in case of cummulated unrecoverable errors
   */
  if( nAT_ERROR_Flags & AT_ERROR_FLAG_CRITICAL_RECOVERABLE_SIM900_RESET )
  {
    eAT_Processor_State = AT_STATE_PROCESSOR_RESET_SIM_BOARD;

    #if(AT_COMMAND_PROCESSOR_GSM_SIM900_DEBUG_SERIAL_ENABLE >= 1)
      Serial.println();
      Serial.print("[I] SIM900 RESTART !!! ---- due to ERRORS !!! ---- ");
      Serial.println(nAT_ERROR_Flags, HEX);

      Serial.print(nAT_arrErrorCount[0]);
      Serial.print(" ");
      Serial.print(nAT_arrErrorCount[1]);
      Serial.print(" ");
      Serial.print(nAT_arrErrorCount[2]);
      Serial.print(" ");
      Serial.print(nAT_arrErrorCount[3]);
      Serial.print(" ");
      Serial.print(nAT_arrErrorCount[4]);
      Serial.print(" ");
      Serial.print(nAT_arrErrorCount[5]);
      Serial.print(" ");
      Serial.print(nAT_arrErrorCount[6]);
      Serial.print(" ");
      Serial.print(nAT_arrErrorCount[7]);
      Serial.println();
    #endif    
  }


  /*
   * AT Command State Machine ... 
   */
	switch (eAT_Processor_State)
	{

    /*
     * short delay during initial - POWER ON -
     *    - used to stabilize the power supply for the SIM900 board ...  5ms (main task reccurence) * MAX_BEFORE_POWER_ON_DELAY (100) = 500ms
     */
    case AT_STATE_INIT_BEFORE_STARTUP_DELAY:
    {
      if( nAT_Power_On_Before_Startup_Delay > 0 )
      {
        nAT_Power_On_Before_Startup_Delay --;
      }
      else
      {
        eAT_Processor_State = AT_STATE_INIT_POWER_ON_START;
      }
    }
    break;
		

    /*
     * - after power-on ... the SIM900 is in shut-down mode 
     * - toggle the POWER ON / RESET  line in order to generate a START-UP opperation on SIM900
     */
    case AT_STATE_INIT_POWER_ON_START:
      {
        digitalWrite( ARDUINO_DIG_OUT_POWER_LINE, 1 );

        nAT_Power_On_Switch_Delay = MAX_POWER_ON_LINE_HIGH_LEVEL_DELAY;

        #if(AT_COMMAND_PROCESSOR_GSM_SIM900_DEBUG_SERIAL_ENABLE == 1)
          Serial.println("[I] SIM900 POWER ON --> Line High ");
        #endif

        eAT_Processor_State = AT_STATE_INIT_POWER_ON_WAIT_STARTUP;
      }
      break;
    
    /*
     * - keep - power-on request line to "high" at least 1 sec - in order for the SIM900 to start-up ...
     */
    case AT_STATE_INIT_POWER_ON_WAIT_STARTUP:
      {
        if( nAT_Power_On_Switch_Delay > 0 )
        {
          nAT_Power_On_Switch_Delay --;
        }
        else
        {
          digitalWrite( ARDUINO_DIG_OUT_POWER_LINE, 0 );

          #if(AT_COMMAND_PROCESSOR_GSM_SIM900_DEBUG_SERIAL_ENABLE == 1)
            Serial.println("[I] SIM900 POWER ON --> Line Low ");
          #endif


          nAT_After_Reset_Delay = MAX_AFTER_RESET_TIME_DELAY;

          eAT_Processor_State = AT_STATE_INIT_AFTER_RESET_DELAY;

        }
      }
      break;


    /*
     * Reset state for the SIM900 board ... by triggering the "Reset" pin we re-initialize the SIM board.
     */
    case AT_STATE_INIT_RESET:
      {
        digitalWrite( ARDUINO_DIG_OUT_RESET_LINE, 1 );

        nAT_Reset_On_Switch_Delay = MAX_RESET_LINE_HIGH_LEVEL_DELAY;

        #if(AT_COMMAND_PROCESSOR_GSM_SIM900_DEBUG_SERIAL_ENABLE == 1)
          Serial.println("[I] SIM900 RESET HIGH --> Line High ");
        #endif

        eAT_Processor_State = AT_STATE_INIT_RESET_WAIT_STARTUP;
      }
      break;

    /*
     * Reset request must be kept at least 1sec ... 
     */
    case AT_STATE_INIT_RESET_WAIT_STARTUP:
      {
        if( nAT_Reset_On_Switch_Delay > 0 )
        {
          nAT_Reset_On_Switch_Delay --;
        }
        else
        {
          digitalWrite( ARDUINO_DIG_OUT_RESET_LINE, 0 );

          #if(AT_COMMAND_PROCESSOR_GSM_SIM900_DEBUG_SERIAL_ENABLE == 1)
            Serial.println("[I] SIM900 RESET LOW --> Line High ");
          #endif

          nAT_After_Reset_Delay = MAX_AFTER_RESET_TIME_DELAY;

          eAT_Processor_State = AT_STATE_INIT_AFTER_RESET_DELAY;

        }
      }
      break;



    /*
     * After a Reset or Power On opperation a small delay in communication 
     *     is required since internal OS of SIM900 is starting-up
     */
    case AT_STATE_INIT_AFTER_RESET_DELAY:
      {
        if( nAT_After_Reset_Delay > 0 )
        {
          nAT_After_Reset_Delay --;
        }
        else
        {

          /* reset error flags and counters */
          nAT_ERROR_Flags = 0x0000;

          for(nIdx = 0; nIdx < 8; nIdx ++)
          {
            nAT_arrErrorCount[nIdx] = 0;
          }

          eAT_Processor_State = AT_STATE_INIT_SEQ_SEND_COMMAND;
        }                    
      }
      break;



    /***************************************************************
     *
     *                    INIT sequence ... 
     *
     **************************************************************/


		/* 
		 * ... this state is used to start the initialisation phase of 
		 *     the SIM900/xxx
		 * ... starts sending pre-programmed commands to GSM module
		 *
		 */
		case AT_STATE_INIT_SEQ_SEND_COMMAND:
		{

      #if(AT_COMMAND_PROCESSOR_GSM_SIM900_DEBUG_SERIAL_ENABLE == 1)
        Serial.print("[I] SIM900 Send: ");
        Serial.println( (const char *)arrAT_GsmInitCmdSet[nAT_Command_Init_Sequence].str_TxCommand );
      #endif

			/* prepare transmission buffers ... */
			AT_Send_Command_Expect_Response_InitSeq( arrAT_GsmInitCmdSet[nAT_Command_Init_Sequence].str_TxCommand, \
                                               arrAT_GsmInitCmdSet[nAT_Command_Init_Sequence].str_Exp_Rx_Resp_OK );



      /* load maximum time that we can wait for a transmission to be done to SIMxxx board      ... otherwise ERROR ... */
      nAT_Command_Transmission_Wait_Counter = AT_MAX_DELAY_WAIT_TRANSMISSION;   /* 500 * 5ms = 2.5 sec */

      /* load maximum time that we can wait for a respone to be received from the SIMxxx board ... otherwise ERROR ... */
			nAT_Command_Response_Wait_Counter = arrAT_GsmInitCmdSet[ nAT_Command_Init_Sequence ].nWaitRespTimer;


      /* Reset counters and flags ... prepare for transmission ...
       */
      AT_Command_Processor_Reset_Reception();


      /* advance state */
			eAT_Processor_State = AT_STATE_INIT_SEQ_SEND_COMMAND_WAIT_TRANSMISSION;

			break;

		}/* AT_STATE_INIT_SEQ_SEND_COMMAND */


    /* 
     *  Wait for transmission to be finished ... 
     *
     */
    case AT_STATE_INIT_SEQ_SEND_COMMAND_WAIT_TRANSMISSION:
    {
      /* transmission complete ... advance state ... */
      if(nAT_Command_Tx_Complete)
      {
        eAT_Processor_State = AT_STATE_INIT_SEQ_WAIT_RESPONSE;


        /*
         * successfull transmission means -> resetting the TX error flags and counters 
         */
          nAT_ERROR_Flags &= ~AT_ERROR_FLAG_TX_ERROR;
          nAT_arrErrorCount[ AT_ERROR_FLAG_TX_ERROR_IDX ] = 0;

        #if(AT_COMMAND_PROCESSOR_GSM_SIM900_DEBUG_SERIAL_ENABLE == 1)
          Serial.println("[I] SIM900 Init TX complete ... Wait response ... ");
        #endif
      }
      else
      {

        /* are we still waiting for the transmission ???? ...  !!*/
        /*
         * 115200 -> 14400 bytes/s 
         *        -> 7200 bytes/ 500ms  --> should be more then enough to send all commands !!!
         *
         */
        if(nAT_Command_Transmission_Wait_Counter > 0)
        {
          nAT_Command_Transmission_Wait_Counter --;
        }
        else
        {
          /* 
              !!! Transmission Error !!!  
              REPEAT TRANSMISSION ?? .... how many times ? It will be treated sepparately by the error handling mechanism ...


              increment error counter and make sure error flag is set ... 
          */

          nAT_ERROR_Flags |= AT_ERROR_FLAG_TX_ERROR;
          nAT_arrErrorCount[ AT_ERROR_FLAG_TX_ERROR_IDX ] ++;
          
          eAT_Processor_State = AT_STATE_INIT_SEQ_SEND_COMMAND;


          /*
           *  ... STOP retransmission ... and re-init fully AT module if cummulated TX errors
           * 
           * Failed transmission is the fault of Arduino board and not the problem of SIM module ... 
           *   ... check of Reset manager is possible ... (reset request module that checks if allowed a watchdog reset ... )
           */

          #if(AT_COMMAND_PROCESSOR_GSM_SIM900_DEBUG_SERIAL_ENABLE == 1)
            Serial.println("[I] SIM900 Init ERROR TX delay timeout ... Retry command ...");
          #endif
          
        }

      }/* end ELSE if(nAT_Command_Tx_Complete)*/
            
      break;
      
    }/* case AT_STATE_INIT_SEQ_SEND_COMMAND_WAIT_TRANSMISSION */

		/*
		 * Wait for the GSM module to process comand and return a response
		 *
		 *     - usually the response is pretty quick ... "OK"
		 *     - sometimes longer amount of time is needed for the response ... 
     *          ATTENTION - even if there is no communication between SIM and the main Arduino board 
     *          that doesn't mean that the SIM module has finished the processing ... 
     *          which means we must still wait for a response ... some commands take really long time to execute !!
     *     - sometimes when SIM module is crushing ... no response at all is returned ... 
     *          which means a timeout is necessary to ensure that Arduino will not be stuck 
     *          waiting for a response that doesn't come ... 
		 */		
		case AT_STATE_INIT_SEQ_WAIT_RESPONSE:
		{
      /*
       * ... is there a response in the Rx buffer even before the timout expiration ?
       */
      if( strstr(arr_AT_Resp_BuffRx, "ERROR") != NULL_PTR  )
      {
        /*
         * there is a response that has been received ... ERROR ... advance state ...
         */

        #if(AT_COMMAND_PROCESSOR_GSM_SIM900_DEBUG_SERIAL_ENABLE == 1)
          Serial.println("[I] SIM900 Init -- Hit ERROR ...");
        #endif

        eAT_Processor_State = AT_STATE_INIT_SEQ_PROCESS_RESPONSE;
      }
      else
      {
        /* if we have a positive response ... OK ... then don't hurry ... might not be the final answer ... */
        if( strstr(arr_AT_Resp_BuffRx, "OK") != NULL_PTR )
        {

          #if(AT_COMMAND_PROCESSOR_GSM_SIM900_DEBUG_SERIAL_ENABLE == 1)
            Serial.println("[I] SIM900 Init -- Hit OK ...");
          #endif

          /* do we have the expected answer in the response ? ... or parts of it ? */
          if( AT_Command_strstr_case_insensitive(arr_AT_Resp_BuffRx, arrAT_GsmInitCmdSet[nAT_Command_Init_Sequence].str_Exp_Rx_Resp_OK) )
          {

            #if(AT_COMMAND_PROCESSOR_GSM_SIM900_DEBUG_SERIAL_ENABLE == 1)
              Serial.println("[I] SIM900 Init -- Hit OK and Partial resp ...");
            #endif

            /* go to full answer processing */
            eAT_Processor_State = AT_STATE_INIT_SEQ_PROCESS_RESPONSE;            
          }
          else
          {

            /* still waiting ... ? for the accepted answer ?  according to pre-programmed command response deadline ... */
            if(nAT_Command_Response_Wait_Counter > 0)
            {
              nAT_Command_Response_Wait_Counter --;
            }
            else
            {

            /*
              * response waiting timeout has been finished ... most likely no response has been received OR an 
              *     an response != OK or ERROR !!!
              */
              eAT_Processor_State = AT_STATE_INIT_SEQ_PROCESS_RESPONSE;

              /*
              * After the RX mandatory waiting timout ... we must wait for a RX gap in communication   <---------------------------------------[ToDo] needed ? ... will determine dealys !!!!
              *  that will indicate a command execution finished ...
              */
              nAT_Silence_In_Comm_Rx_Timer = MAX_AT_SIM_COMM_SILENCE_TIMER_DELAY;
            }

          }/* end else if( AT_Command_strstr_case_insensitive(arr_AT_Resp_BuffRx, arrAT_GsmInitCmdSet[nAT_Command_Init_Sequence].str_Exp_Rx_Resp_OK) ) */

        }/* if( strstr(arr_AT_Resp_BuffRx, "OK") != NULL_PTR ) */
        else
        {


          /*
           * sometimes while waiting for the final "OK" ... it might be that there is a expected partial answer .. and afterwards OK will be issued
           * ... OR ... 
           * ... verry long answers that do not fit in the response buffer ... will never receive "OK" ... therefore the only way to see the 
           *     correct answer has been received is to look on the partial response !
           */
          /* do we have the expected answer in the response ? ... or parts of it ? */
          if( AT_Command_strstr_case_insensitive(arr_AT_Resp_BuffRx, arrAT_GsmInitCmdSet[nAT_Command_Init_Sequence].str_Exp_Rx_Resp_OK) )
          {
            /* go to full answer processing */
            eAT_Processor_State = AT_STATE_INIT_SEQ_PROCESS_RESPONSE;            
          }


          /* ... not OK, not ERROR ... command is in processing state on SIM900 .... */
          /* still waiting ... ? according to pre-programmed command response deadline ... */
          if(nAT_Command_Response_Wait_Counter > 0)
          {
            nAT_Command_Response_Wait_Counter --;
          }
          else
          {

            /*
            * response waiting timeout has been finished ... most likely no response has been received OR an 
            *     an response != OK or ERROR !!!
            */
            eAT_Processor_State = AT_STATE_INIT_SEQ_PROCESS_RESPONSE;

            /*
            * After the RX mandatory waiting timout ... we must wait for a RX gap in communication     <--------------------------------------- [ToDo] needed ? ... will determine dealys !!!!
            *  that will indicate a command execution finished ...
            */
            nAT_Silence_In_Comm_Rx_Timer = MAX_AT_SIM_COMM_SILENCE_TIMER_DELAY;
          }

        }/* else if( strstr(arr_AT_Resp_BuffRx, "OK") != NULL_PTR ) */

      } /* else if( strstr(arr_AT_Resp_BuffRx, "ERROR") != NULL_PTR  )  */

			break;

		} /* end case AT_STATE_INIT_SEQ_WAIT_RESPONSE */



    /*
     * Some commands are responded by the SIM module with "OK" ... and a pause follows ... after which the final response is given  
     */
		case AT_STATE_INIT_SEQ_PROCESS_RESPONSE:
		{
      /* We are in a communication silence timeout gap 
       *   ... we should check the response ... 
       */
      if( nAT_Silence_In_Comm_Rx_Timer == 0 )
      {
        
        #if(AT_COMMAND_PROCESSOR_GSM_SIM900_DEBUG_SERIAL_ENABLE == 1)
          Serial.println();        
          Serial.print("[I] SIM900 Init Received: ");
          Serial.println((const char *)arr_AT_Resp_BuffRx);
        #endif


        /* in all cases ... prepare a delay after current command ... */
        nAT_Delay_AfterCommand = arrAT_GsmInitCmdSet[nAT_Command_Init_Sequence].nDelayAfterCommand;
        nAT_Command_Response_Wait_Counter = arrAT_GsmInitCmdSet[ nAT_Command_Init_Sequence ].nWaitRespTimer;        /* ------------------------------------------*/


        /* check for "OK", "ERROR" or Expected result 
         * .... or an empty response buffer. ... 
         */
        if( strstr(arr_AT_Resp_BuffRx, "ERROR") || (strlen(arr_AT_Resp_BuffRx) < 2) )
        {
          /*
           *    - increment command error counter 
           *    - error handling will be done in sepparate location ... 
           */

            nAT_ERROR_Flags |= AT_ERROR_FLAG_COMMAND_RESP_ERROR;
            nAT_arrErrorCount[AT_ERROR_FLAG_COMMAND_RESP_ERROR_IDX] ++;
          

          /*
           * this is cleary an error state ... go to delay state in case the error handling funtion allows for retry !!
           */

				  eAT_Processor_State = AT_STATE_PROCESSOR_DELAY_AFTER_COMMAND;

          #if(AT_COMMAND_PROCESSOR_GSM_SIM900_DEBUG_SERIAL_ENABLE == 1)
            Serial.print("[I] SIM900 Init Received: ERROR or NO RESP !!! - Retry");
            Serial.println((const char *)arr_AT_Resp_BuffRx);
          #endif

        }
        else
        {
            /* 
             * command has been correctly executed ... 
             */


            /* reset reception error flags ...
             */
            nAT_ERROR_Flags &= ~AT_ERROR_FLAG_RX_ERROR;
            nAT_arrErrorCount[AT_ERROR_FLAG_RX_ERROR_IDX] = 0;


            /* is it the expected command to go to next state ? .... */
            /* we don't search for the answer ... in case of reset request .... it might also be "III" ....  */
            //if( strstr( arr_AT_Resp_BuffRx, arrAT_GsmInitCmdSet[nAT_Command_Init_Sequence].str_Exp_Rx_Resp_OK) || strstr( arr_AT_Resp_BuffRx, strupr(arrAT_GsmInitCmdSet[nAT_Command_Init_Sequence].str_Exp_Rx_Resp_OK)))
            if( AT_Command_strstr_case_insensitive(arr_AT_Resp_BuffRx, arrAT_GsmInitCmdSet[nAT_Command_Init_Sequence].str_Exp_Rx_Resp_OK) || \
                ( strlen(arrAT_GsmInitCmdSet[nAT_Command_Init_Sequence].str_Exp_Rx_Resp_OK) < 2 ) )
            {

              /*
               * reset any previous error marking of the response ...
               */
              nAT_ERROR_Flags &= ~AT_ERROR_FLAG_COMMAND_RESP_ERROR;
              nAT_arrErrorCount[AT_ERROR_FLAG_COMMAND_RESP_ERROR_IDX] = 0;            


              /*  If sucessful response ... does the user requested to skip the next command ?
               */              
              if( arrAT_GsmInitCmdSet[nAT_Command_Init_Sequence].nSkipNextCmdIfOK == SKIP_NEXT_CMD_IF_TRUE )
              {
                nAT_Command_Init_Sequence += 2;

                #if(AT_COMMAND_PROCESSOR_GSM_SIM900_DEBUG_SERIAL_ENABLE == 1)
                  Serial.println("[I] SIM900 Index + 2 ...");
                  Serial.println(nAT_Command_Init_Sequence);
                #endif
              }
              else
              {            
                nAT_Command_Init_Sequence ++;

                #if(AT_COMMAND_PROCESSOR_GSM_SIM900_DEBUG_SERIAL_ENABLE == 1)
                  Serial.println("[I] SIM900 Index + 1 ...");
                  Serial.println(nAT_Command_Init_Sequence);
                #endif                
              }

              /* check to see if init sequence is over ... and we reached the end of command list sequence ...*/
              if(nAT_Command_Init_Sequence >=  MAX_INIT_SEQUENCE_LENGTH)
              {
                /* wait additional commands from user ... 

                        ... INIT is over ... 
                */
                eAT_Processor_State = AT_STATE_PROCESSOR_IDDLE;


                /* [ToDo] ... This callback can be optimized since it is called also below .... 
                 */
                AT_Command_Callback_Notify_Init_Finished();

                #if(AT_COMMAND_PROCESSOR_GSM_SIM900_DEBUG_SERIAL_ENABLE == 1)
                  Serial.println("[I] SIM900 Init FINISH - goto IDDLE ...");
                #endif
              }
              else
              {
                /* init sequence is in progress ... 
                  ... RESTART INIT commands Transmission ....
                */

                eAT_Processor_State = AT_STATE_PROCESSOR_DELAY_AFTER_COMMAND;	

                #if(AT_COMMAND_PROCESSOR_GSM_SIM900_DEBUG_SERIAL_ENABLE == 1)
                  Serial.println("[I] SIM900 Init OK - NEXT Command ...");
                #endif
              }        

            }/* end if expected command response is received ... */
            else					
            {
              /* 
               * The response is not the one from the expected one ... butt still valid ... and correct ... then we must also treat-it 
               * 
               *  ... for example some responses can have different values ... like ... asking for PIN ... SIM900 can response with UNLOCKED or with PIN required 
               */
              if( ( strlen(arrAT_GsmInitCmdSet[nAT_Command_Init_Sequence].str_Exp_Rx_Resp_Continue) > 1 ) && \
                    strstr( arr_AT_Resp_BuffRx, arrAT_GsmInitCmdSet[nAT_Command_Init_Sequence].str_Exp_Rx_Resp_Continue) )
              {


                /*
                * reset any previous error marking of the response ...
                */
                nAT_ERROR_Flags &= ~AT_ERROR_FLAG_COMMAND_RESP_ERROR;
                nAT_arrErrorCount[AT_ERROR_FLAG_COMMAND_RESP_ERROR_IDX] = 0;            


                /* response recognized ... goto next command ... */
                nAT_Command_Init_Sequence ++;

                /* check to see if init sequence is over */				
                if(nAT_Command_Init_Sequence >=  MAX_INIT_SEQUENCE_LENGTH)
                {
                  /* wait additional commands from user ... 
                     ... INIT is over ... 
                  */
                  eAT_Processor_State = AT_STATE_PROCESSOR_IDDLE;    

                  AT_Command_Callback_Notify_Init_Finished();

                  #if(AT_COMMAND_PROCESSOR_GSM_SIM900_DEBUG_SERIAL_ENABLE == 1)
                    Serial.println("[I] SIM900 Init FINISH - goto IDDLE ...");
                  #endif
                }
                else
                {
                  /* init sequence is in progress ... 
                    ... RESTART INIT commands Transmission ....
                  */

                  eAT_Processor_State = AT_STATE_PROCESSOR_DELAY_AFTER_COMMAND;	

                  #if(AT_COMMAND_PROCESSOR_GSM_SIM900_DEBUG_SERIAL_ENABLE == 1)
                    Serial.println("[I] SIM900 Init OK - NEXT Command ...");
                  #endif
                }                
              }/* end if .... The respone is the one user expected !!!  - arrAT_GsmInitCmdSet[nAT_Command_Init_Sequence].str_Exp_Rx_Resp_Continue ... */
              else
              {
                #if(AT_COMMAND_PROCESSOR_GSM_SIM900_DEBUG_SERIAL_ENABLE == 1)
                  Serial.println("[I] SIM900 Init OK - BUTT ... not the expected answer ... retry ..."); 
                  Serial.println((const char *)arr_AT_Resp_BuffRx);
                #endif


                /*
                 * Current sequence command has been completed with error !!! 
                 */                  
                nAT_ERROR_Flags |= AT_ERROR_FLAG_COMMAND_RESP_ERROR;
                nAT_arrErrorCount[AT_ERROR_FLAG_COMMAND_RESP_ERROR_IDX] ++;


                #if(AT_COMMAND_PROCESSOR_GSM_SIM900_DEBUG_SERIAL_ENABLE >= 1)
                  Serial.println();  
                  Serial.print("[I] SIM900 Error counters drop: ");
                  Serial.print(nAT_arrErrorCount[0]);
                  Serial.print(" ");
                  Serial.print(nAT_arrErrorCount[1]);
                  Serial.print(" ");    
                  Serial.print(nAT_arrErrorCount[2]);
                  Serial.print(" ");    
                  Serial.print(nAT_arrErrorCount[3]);
                  Serial.print(" ");    
                  Serial.print(nAT_arrErrorCount[4]);
                  Serial.print(" ");    
                  Serial.print(nAT_arrErrorCount[5]);
                  Serial.print(" ");    
                  Serial.print(nAT_arrErrorCount[6]);
                  Serial.print(" ");    
                  Serial.print(nAT_arrErrorCount[7]);
                  Serial.println(); 
                #endif



                eAT_Processor_State = AT_STATE_PROCESSOR_DELAY_AFTER_COMMAND;	

              }/* end else ... no response recognized ... */           

            }/* end else if( strstr( arr_AT_Resp_BuffRx, arrAT_GsmInitCmdSet[nAT_Command_Init_Sequence].str_Exp_Rx_Resp_OK) ) */

        }/* else if strstr(arr_AT_Resp_BuffRx, "ERROR") || (strlen(arr_AT_Resp_BuffRx) < 2)   */

      }/* if( nAT_Silence_In_Comm_Rx_Timer == 0 ) */
      else
      {
        /*
         * .. nothing to do ... wait for a communication gap to happen ... SIM900 is still throwing data on serial port ... so ... wait to finish !!!
         */
      }

			break;

		}/* AT_STATE_INIT_SEQ_PROCESS_RESPONSE */



    /*
     * Short delay after executing a command ... until next-one ... 
     */
    case AT_STATE_PROCESSOR_DELAY_AFTER_COMMAND:
    {
      if( nAT_Delay_AfterCommand > 0 )
      {
        nAT_Delay_AfterCommand --;        
      }
      else
      {
        eAT_Processor_State = AT_STATE_INIT_SEQ_SEND_COMMAND;
      }

      break;
    }    


    /*
     * State used before any reset of SIM900 board ... due to error accumulation 
     *    in order to notify upper layers about corrective communication actions  !!!
     *   
     */
		case AT_STATE_PROCESSOR_RESET_SIM_BOARD:
		{
		    /*
		     * Startup sequence will start from index "0" of command array ... 
		     */
		    nAT_Command_Init_Sequence = 0;

        /*
        * ... during startup ... request faster the RSSI signal value ... 
        */    
        nAT_RSSI_Extractor_Counter = 10;


        /* reset error flags and counters */
        nAT_ERROR_Flags = 0x0000;

        for(nIdx = 0; nIdx < 8; nIdx ++)
        {
          nAT_arrErrorCount[nIdx] = 0;
        }

        eAT_Processor_State = AT_STATE_INIT_POWER_ON_START;

        /*
         * notify COM layer that an re-initialization procedure has been started inside SIM900 driver !!
         */
        AT_Command_Callback_Notify_No_Communication_Error_Re_Init();

			break;
		}


		/************************************************************************
     *
     *
		 * ... IDDLE nothing to do ... wait for any activity requests ... 
     *
     *
		 ************************************************************************/
		case AT_STATE_PROCESSOR_IDDLE:
		{

      /*
       * From time to time ... check the signal strength value ... 
       */
      if(nAT_RSSI_Extractor_Counter > 0 )
      {
        nAT_RSSI_Extractor_Counter --;
      }
      else      
      {
        eAT_Processor_State = AT_STATE_PROCESSOR_SEND_RSSI_LEVEL;
      }

      break;
		}



    /***************************************************************
     *
     *        Generic SEND - RECEIVE sequence ... 
     *
     **************************************************************/


    /* Process transmission of commands ... 
     *
     */
    case AT_STATE_PROCESSOR_SEND_STEP1_PRE_COMMAND_SEQ:
    {
      
      /*
       * Check to see if the command shall be taken out from  
       *   arrAT_Gsm_COMAND_Step1                 - or from
       *   arrAT_Gsm_COMMAND_Generic_HTTP_Request
       */
      if( strlen( arrAT_Gsm_COMAND_Step1[nAT_Command_Step1_Sequence_Idx].str_TxCommand ) >= 2 )      
      {

			  /* prepare transmission buffers ... */
			  AT_Send_Command_Expect_Response_InitSeq( arrAT_Gsm_COMAND_Step1[nAT_Command_Step1_Sequence_Idx].str_TxCommand, \
                                                 arrAT_Gsm_COMAND_Step1[nAT_Command_Step1_Sequence_Idx].str_Exp_Rx_Resp_OK );

        #if(AT_COMMAND_PROCESSOR_GSM_SIM900_DEBUG_SERIAL_ENABLE > 1)
          Serial.println("[I] SIM900 STATE - AT_STATE_PROCESSOR_SEND_STEP1_PRE_COMMAND_SEQ");
          Serial.print("[I] SIM900 Step 1 ... Send: ");
          Serial.println((const char *)arrAT_Gsm_COMAND_Step1[nAT_Command_Step1_Sequence_Idx].str_TxCommand);
        #endif

      }
      else
      {
        AT_Send_Command_Expect_Response_InitSeq( arr_AT_Generic_BuffTx, \
                                                 arrAT_Gsm_COMAND_Step1[nAT_Command_Step1_Sequence_Idx].str_Exp_Rx_Resp_OK );

        #if(AT_COMMAND_PROCESSOR_GSM_SIM900_DEBUG_SERIAL_ENABLE > 1)
          Serial.println("[I] SIM900 STATE - AT_STATE_PROCESSOR_SEND_STEP1_PRE_COMMAND_SEQ");
          Serial.print("[I] SIM900 Step 1 ... Send: ");
          Serial.println((const char *)arr_AT_Generic_BuffTx);
        #endif
        
      }


      /* load maximum time that we can wait for a transmission to be done to SIMxxx board      ... otherwise ERROR ... */
      nAT_Command_Transmission_Wait_Counter = AT_MAX_DELAY_WAIT_TRANSMISSION;    /* 500 * 5ms = 2.5 sec */

      /* load maximum time that we can wait for a respone to be received from the SIMxxx board ... otherwise ERROR ... */
			nAT_Command_Response_Wait_Counter = arrAT_Gsm_COMAND_Step1[ nAT_Command_Step1_Sequence_Idx ].nWaitRespTimer;

      /* Reset counters and flags ... prepare for transmission ... in the reception buffer we shall have ECHO !!!
       */      
      AT_Command_Processor_Reset_Reception();

      /* advance state */
			eAT_Processor_State = AT_STATE_PROCESSOR_SEND_STEP1_PRE_COMMAND_SEQ_WAIT_TRANSMISSION;

			break;

    }/* AT_STATE_PROCESSOR_SEND_STEP1_PRE_COMMAND_SEQ */


    /*
     * State used to wait for the Arduino to SIM9xx command transmission
     *   completes ...     
     */
    case AT_STATE_PROCESSOR_SEND_STEP1_PRE_COMMAND_SEQ_WAIT_TRANSMISSION:
    {
      /* transmission complete ... advance state ... */
      if(nAT_Command_Tx_Complete)
      {
        eAT_Processor_State = AT_STATE_PROCESSOR_SEND_STEP1_PRE_COMMAND_SEQ_WAIT_RESPONSE;

        /*
         * successfull transmissin means -> resetting the TX error flags and counters 
         */
          nAT_ERROR_Flags &= ~AT_ERROR_FLAG_TX_STEP1_ERROR;
          nAT_arrErrorCount[ AT_ERROR_FLAG_TX_STEP1_ERROR_IDX ] = 0;

          #if(AT_COMMAND_PROCESSOR_GSM_SIM900_DEBUG_SERIAL_ENABLE == 1)
            Serial.print("[I] SIM900 Step1 TX complete ... Wait response ... ");
          #endif
      }
      else
      {
       
        /*
         * 115200 -> 14400 bytes/s 
         *        -> 7200 bytes/ 500ms  --> should be more then enough to send all commands !!!
         *
         *
         * are we still waiting for the transmission ???? ... can be an communication configuration ERROR ?? */
        if(nAT_Command_Transmission_Wait_Counter > 0)
        {
          nAT_Command_Transmission_Wait_Counter --;
        }
        else
        {
          /* 
              !!! Transmission Error !!!      

              REPEAT TRANSMISSION ?? .... how many times ? It will be decided in the error handling function !

              increment error counter and make sure error flag is set ...
          */

          nAT_ERROR_Flags |= AT_ERROR_FLAG_TX_STEP1_ERROR;
          nAT_arrErrorCount[ AT_ERROR_FLAG_TX_STEP1_ERROR_IDX ] ++;

          eAT_Processor_State = AT_STATE_PROCESSOR_SEND_STEP1_PRE_COMMAND_SEQ;


          /*
          * [ToDo] ... STOP retransmission ... and re-init fully AT module
          * 
          * Failed transmission is the fault of Arduino board and not the problem of SIM module ... 
          *   ... check of Reset namager is possible ... (reset request module that checks if allowed a watchdog reset ... )
          */

          #if(AT_COMMAND_PROCESSOR_GSM_SIM900_DEBUG_SERIAL_ENABLE == 1)
            Serial.print("[I] SIM900 Step1 ERROR Tx delay timeout ... Retry command ...");
          #endif

        }
        
      }/* end else if(nAT_Command_Tx_Complete) */

      break;

    }/* AT_STATE_PROCESSOR_SEND_STEP1_PRE_COMMAND_SEQ_WAIT_TRANSMISSION */


		/*
		 * Wait for the GSM module to process comand and return a response
		 *
		 *     - usually the response is pretty quick ... "OK"
		 *     - sometimes longer amount of time is needed for the response ... 
     *          ATTENTION - even if there is no communication between SIM and the main Arduino board 
     *          that doesn't mean that the SIM module has finished the processing ... 
     *          which means we must still wait for a response ... some commands take really long time to execute !!
     *     - sometimes when SIM module is crushing ... no response at all is returned ... 
     *          which means a timeout is necessary to ensure that Arduino will not be stuck 
     *          waiting for a response that doesn't come ... 
		 */		
    case AT_STATE_PROCESSOR_SEND_STEP1_PRE_COMMAND_SEQ_WAIT_RESPONSE:
    {

      /*
       * ... is there a response in the Rx buffer even before the timout expiration ?
       * 
       * ... the STEP 1 commands ... mostly all require short responses with "OK" contained in the server reply !! ... therefore no 
       *     further complications needed in this state !
       */
      if( ( strstr(arr_AT_Resp_BuffRx, "ERROR") != NULL_PTR ) || ( strstr(arr_AT_Resp_BuffRx, "OK") != NULL_PTR ) )
      {
        /*
         * there is a response that has been received ... advance state ...
         */

        eAT_Processor_State = AT_STATE_PROCESSOR_SEND_STEP1_PRE_COMMAND_SEQ_PROCESS_RESPONSE;
      }
      else
      {
        /*  still waiting ... ?  */
        if(nAT_Command_Response_Wait_Counter > 0)
        {
          nAT_Command_Response_Wait_Counter --;
        }
        else
        {

          /*
          * response timeout has been finished ... most likely no response has been received OR an 
          *     an response != OK or ERROR !!!
          */
          eAT_Processor_State = AT_STATE_PROCESSOR_SEND_STEP1_PRE_COMMAND_SEQ_PROCESS_RESPONSE;


          /*
          * After the RX mandatory waiting timout ... we must wait for a RX gap in communication
          *  that will indicate a command execution finished ...
          */
          nAT_Silence_In_Comm_Rx_Timer = MAX_AT_SIM_COMM_SILENCE_TIMER_DELAY;        
        }
      }

			break;

    }/* AT_STATE_PROCESSOR_SEND_STEP1_PRE_COMMAND_SEQ_WAIT_RESPONSE */


    /*
     * Some commands are responded by the SIM module with "OK" ... and a pause follows ... after which the final response is given  
     */
    case AT_STATE_PROCESSOR_SEND_STEP1_PRE_COMMAND_SEQ_PROCESS_RESPONSE:
    {
      /* We are in a communication silence timeout gap 
       *   ... we should check the response ... 
       */
      if( nAT_Silence_In_Comm_Rx_Timer == 0 )
      {
        
        #if(AT_COMMAND_PROCESSOR_GSM_SIM900_DEBUG_SERIAL_ENABLE == 1)
          Serial.println();
          Serial.print("[I] SIM900 Step1 Received: ");
          Serial.println((const char *)arr_AT_Resp_BuffRx);
        #endif

        nAT_Delay_AfterCommand = arrAT_Gsm_COMAND_Step1[ nAT_Command_Step1_Sequence_Idx ].nDelayAfterCommand;

        /* check for "OK", "ERROR" or Expected result 
        */
        if( ( strstr(arr_AT_Resp_BuffRx, "ERROR") != NULL_PTR ) || ( strlen(arr_AT_Resp_BuffRx) < 2 ) )
        {
          /*
           *    - increment command error counter 
           *    - error handling in separate function ...  
           */

          nAT_ERROR_Flags |= AT_ERROR_FLAG_COMMAND_RESP_STEP1_ERROR;
          nAT_arrErrorCount[AT_ERROR_FLAG_COMMAND_RESP_STEP1_ERROR_IDX] ++;


          /*
           * this is cleary an error state ... go to delay state in case the error handling funtion allows for retry !! ... the nAT_Command_Step1_Sequence_Idx is not incremented ... !!! 
           */

			  	eAT_Processor_State = AT_STATE_PROCESSOR_SEND_STEP1_PRE_COMMAND_DELAY_AFTER_COMMAND;

          #if(AT_COMMAND_PROCESSOR_GSM_SIM900_DEBUG_SERIAL_ENABLE == 1)
            Serial.print("[I] SIM900 Step1 Received: ERROR !!! - Retry");
            Serial.println((const char *)arr_AT_Resp_BuffRx);
          #endif

        }
        else
        {

          /* 
           * command has been correctly executed ... 
           */

          /* reset reception error flags ...
           */
          nAT_ERROR_Flags &= ~AT_ERROR_FLAG_RX_STEP1_ERROR;
          nAT_arrErrorCount[AT_ERROR_FLAG_RX_STEP1_ERROR_IDX] = 0;

          nAT_ERROR_Flags &= ~AT_ERROR_FLAG_COMMAND_RESP_STEP1_ERROR;
          nAT_arrErrorCount[AT_ERROR_FLAG_COMMAND_RESP_STEP1_ERROR_IDX] = 0;            


          if( /*strstr(arr_AT_Resp_BuffRx, "OK" ) || */strstr( arr_AT_Resp_BuffRx, arrAT_Gsm_COMAND_Step1[ nAT_Command_Step1_Sequence_Idx ].str_Exp_Rx_Resp_OK) )
          {

            nAT_Command_Step1_Sequence_Idx ++;

            eAT_Processor_State = AT_STATE_PROCESSOR_SEND_STEP1_PRE_COMMAND_DELAY_AFTER_COMMAND;

          }/* if( strstr(arr_AT_Resp_BuffRx, "OK" ) .... ) */
          else
          {
            /*
             * [ToDo] ... there is no OK and no ERROR ... 
             * ... we must wait again ... but how much time we should do this ? 
             *
             *
             */

            eAT_Processor_State = AT_STATE_PROCESSOR_SEND_STEP1_PRE_COMMAND_WAIT_CORRECT_RESPONSE_FROM_COMMAND;
          }

        }/* else if( strstr(arr_AT_Resp_BuffRx, "ERROR") )  */

      }/* if( nAT_Silence_In_Comm_Rx_Timer == 0 ) */
      else
      {
        /*
         * .. nothing to do ... wait for a communication gap to happen ... 
         *
         *  ... continous Rx communication is mostly impossible ...
         *
         */
      }

			break;

    }/* AT_STATE_PROCESSOR_SEND_STEP1_PRE_COMMAND_SEQ_PROCESS_RESPONSE */



    /*
     * Additional waiting time ... 
     */
    case AT_STATE_PROCESSOR_SEND_STEP1_PRE_COMMAND_WAIT_CORRECT_RESPONSE_FROM_COMMAND:
    {
      if( nAT_Delay_AfterCommand > 0 )
      {
        nAT_Delay_AfterCommand --;


        /* check the possibility for the correct response reception ... */
        if( strstr( arr_AT_Resp_BuffRx, arrAT_Gsm_COMAND_Step1[ nAT_Command_Step1_Sequence_Idx ].str_Exp_Rx_Resp_OK) )
        {

            /* correct response ... go to next command in the queue ... */
            nAT_Command_Step1_Sequence_Idx ++;

            eAT_Processor_State = AT_STATE_PROCESSOR_SEND_STEP1_PRE_COMMAND_DELAY_AFTER_COMMAND;


            nAT_Delay_AfterCommand = arrAT_Gsm_COMAND_Step1[ nAT_Command_Step1_Sequence_Idx ].nDelayAfterCommand;

            #if(AT_COMMAND_PROCESSOR_GSM_SIM900_DEBUG_SERIAL_ENABLE == 1)
              Serial.println();
              Serial.print("[I] SIM900 Step1 Received Lastly: ");
              Serial.println((const char *)arr_AT_Resp_BuffRx);
            #endif

        }

      }
      else
      {
          /* .... !!!! not the expected response !!!! .... */

          nAT_ERROR_Flags |= AT_ERROR_FLAG_COMMAND_RESP_STEP1_ERROR;
          nAT_arrErrorCount[AT_ERROR_FLAG_COMMAND_RESP_STEP1_ERROR_IDX] ++; 

          nAT_Delay_AfterCommand = arrAT_Gsm_COMAND_Step1[ nAT_Command_Step1_Sequence_Idx ].nDelayAfterCommand;

          eAT_Processor_State = AT_STATE_PROCESSOR_SEND_STEP1_PRE_COMMAND_DELAY_AFTER_COMMAND;
      }

      break;
    }


    case AT_STATE_PROCESSOR_SEND_STEP1_PRE_COMMAND_DELAY_AFTER_COMMAND:
    {
      if( nAT_Delay_AfterCommand > 0 )
      {
        nAT_Delay_AfterCommand --;        
      }
      else
      {

        /* check to see if additional commands are required for the HTTP request sequence is over */				
        if( nAT_Command_Step1_Sequence_Idx >=  MAX_COMMAND_STEP_1 )
        {
          /*  
             ... Transmission of new Command is over ... decode server response ...
           */
          eAT_Processor_State = AT_STATE_PROCESSOR_SEND_STEP1_PRE_COMMAND_SEQ_DECODE_RESPONSE;


          #if(AT_COMMAND_PROCESSOR_GSM_SIM900_DEBUG_SERIAL_ENABLE == 1)
            Serial.println("[I] SIM900 Step1 FINISH - goto DECODE !!! ...");
          #endif

        }
        else					
        {
          /* HTTP command sequence is in progress ... 
                ... RESTART STEP1 commands Transmission ....
          */
          eAT_Processor_State = AT_STATE_PROCESSOR_SEND_STEP1_PRE_COMMAND_SEQ;	

          #if(AT_COMMAND_PROCESSOR_GSM_SIM900_DEBUG_SERIAL_ENABLE == 1)
            Serial.println("[I] SIM900 Step1 OK - NEXT Command ...");
          #endif

        }

      }

      break;
      
    } /* AT_STATE_PROCESSOR_SEND_STEP1_PRE_COMMAND_DELAY_AFTER_COMMAND */


    case AT_STATE_PROCESSOR_SEND_STEP1_PRE_COMMAND_SEQ_DECODE_RESPONSE:
    {

      /*
       * Mark communication complete !!!
       */
      nIsGSM_Command_Complete = 1;      


      /* reset error flags ...
       */
      nAT_ERROR_Flags &= ~AT_ERROR_FLAG_RX_STEP1_ERROR;
      nAT_arrErrorCount[AT_ERROR_FLAG_RX_STEP1_ERROR_IDX] = 0;

      nAT_ERROR_Flags &= ~AT_ERROR_FLAG_TX_STEP1_ERROR;
      nAT_arrErrorCount[AT_ERROR_FLAG_TX_STEP1_ERROR_IDX] = 0;            

      nAT_ERROR_Flags &= ~AT_ERROR_FLAG_COMMAND_RESP_STEP1_ERROR;
      nAT_arrErrorCount[AT_ERROR_FLAG_COMMAND_RESP_STEP1_ERROR_IDX] = 0;            


      nLastServerResponse_Code = AT_Command_Decode_Rx_Buff_v2();


      /* 
       * 'A' is Alive  -- nothing to execute ...
       *   ... if anything else ... then mark as a new command ... 
       */
      if(nLastServerResponse_Code > 'A')
      {
        nLastServerResponse_Code_Not_Processed = 1;
        AT_Command_Callback_Notify_Server_Response_Received();
      }
      else
        if(nLastServerResponse_Code == 'A')
        {
          AT_Command_Callback_Notify_Server_Response_Received();
        }

      
      eAT_Processor_State = AT_STATE_PROCESSOR_IDDLE;

      break;

    }/* AT_STATE_PROCESSOR_SEND_STEP1_PRE_COMMAND_SEQ_DECODE_RESPONSE */



    /* **********************************************
     * 
     *
     *         RSSI level calculation
     *
     *
     ************************************************ */


    case AT_STATE_PROCESSOR_SEND_RSSI_LEVEL:
    {
      #if(AT_COMMAND_PROCESSOR_GSM_SIM900_DEBUG_SERIAL_ENABLE == 1)
        Serial.print("[I] SIM900 Send RSSI: ");
        Serial.println( (const char *)arrAT_Gsm_COMMAND_RSSI [ 0 ].str_TxCommand );
      #endif


			/* prepare transmission buffers ... only one command .... */
			AT_Send_Command_Expect_Response_InitSeq( arrAT_Gsm_COMMAND_RSSI[ 0 ].str_TxCommand, \
                                               arrAT_Gsm_COMMAND_RSSI[ 0 ].str_Exp_Rx_Resp_OK );

      /* load maximum time that we can wait for a respone to be received from the SIMxxx board ... otherwise ERROR ... */
			nAT_Command_Response_Wait_Counter = arrAT_Gsm_COMMAND_RSSI[ 0 ].nWaitRespTimer;

      /* load maximum time that we can wait for a transmission to be done to SIMxxx board      ... otherwise ERROR ... */
      nAT_Command_Transmission_Wait_Counter = AT_MAX_DELAY_WAIT_TRANSMISSION;

      /* Reset counters and flags ... prepare for transmission ...
       */
      AT_Command_Processor_Reset_Reception();

      /* advance state */
			eAT_Processor_State = AT_STATE_PROCESSOR_SEND_RSSI_LEVEL_WAIT_TRANSMISSION;
    
      break;

    }/* AT_STATE_PROCESSOR_SEND_RSSI_LEVEL */


    case AT_STATE_PROCESSOR_SEND_RSSI_LEVEL_WAIT_TRANSMISSION:
    {
      /* transmission complete ... advance state ... */
      if(nAT_Command_Tx_Complete)
      {
        eAT_Processor_State = AT_STATE_PROCESSOR_SEND_RSSI_LEVEL_WAIT_RESPONSE;
      }
      else
      {
       /* are we still waiting for the transmission ???? ... can be an communication configuration ERROR ?? */
        if(nAT_Command_Transmission_Wait_Counter > 0)
        {
          nAT_Command_Transmission_Wait_Counter --;
        }
        else
        {
          /* 
              !!! Transmission Error !!!      [ToDo --- currently retry indefinitely !!!]

              REPEAT TRANSMISSION ?? .... how many times ? 
          */
          eAT_Processor_State = AT_STATE_PROCESSOR_SEND_RSSI_LEVEL;


          /*
          * [ToDo] ... STOP retransmission ... and re-init fully AT module
          * 
          * Failed transmission is the fault of Arduino board and not the problem of SIM module ... 
          *   ... check of Reset namager is possible ... (reset request module that checks if allowed a watchdog reset ... )
          */

          #if(AT_COMMAND_PROCESSOR_GSM_SIM900_DEBUG_SERIAL_ENABLE == 1)
            Serial.print("[I] SIM900 RSSI command ERROR Transmission delay ... Retry ...");
          #endif          
        }
      }
      
      break;
    }/* AT_STATE_PROCESSOR_SEND_RSSI_LEVEL_WAIT_TRANSMISSION */


    case AT_STATE_PROCESSOR_SEND_RSSI_LEVEL_WAIT_RESPONSE:
    {

      /*
       * ... is there a response in the Rx buffer even before the timout expiration ?
       */
      if( strstr(arr_AT_Resp_BuffRx, "ERROR") || strstr(arr_AT_Resp_BuffRx, "OK") )
      {
        /*
         * there is a response that has been received ... advance state ...
         */

        eAT_Processor_State = AT_STATE_PROCESSOR_SEND_RSSI_LEVEL_PROCESS_RESPONSE;
      }
      else
      {
        /*  still waiting ... ?  */
        if(nAT_Command_Response_Wait_Counter > 0)
        {
          nAT_Command_Response_Wait_Counter --;
        }
        else
        {
          /*
          * response timeout has been finished ... 
          */
          eAT_Processor_State = AT_STATE_PROCESSOR_SEND_RSSI_LEVEL_PROCESS_RESPONSE;
        }
        
      }

      break;

    }/* AT_STATE_PROCESSOR_SEND_RSSI_LEVEL_WAIT_RESPONSE */


    case AT_STATE_PROCESSOR_SEND_RSSI_LEVEL_PROCESS_RESPONSE:
    {

      /* We are in a communication silence timeout gap 
       *   ... we should check the response ... 
       */
        
      nAT_RSSI_Signal_Quality_Value = AT_Command_Decode_CSQ_Rx_Buff(arr_AT_Resp_BuffRx, nAT_Resp_Rx_Idx);
      nAT_RSSI_Signal_Quality_Update_Flag = 1;


      nAT_RSSI_Extractor_Counter = AT_RSSI_DELAY_COUNTER;

      eAT_Processor_State = AT_STATE_PROCESSOR_IDDLE;

      break;

    }/* AT_STATE_PROCESSOR_SEND_RSSI_LEVEL_PROCESS_RESPONSE */

		default:
      break;

	}/* switch (eAT_Processor_State) */
  
}/* void AT_Command_Processor_Main(void) */



unsigned short AT_Command_Decode_CSQ_Rx_Buff(unsigned char* strResp, unsigned short nMaxIdx)
{
  unsigned short nRSSI_Local_Value = 0;  
  unsigned short nIdx = 0;


  #if(AT_COMMAND_PROCESSOR_GSM_SIM900_DEBUG_SERIAL_ENABLE == 1)
    Serial.println();
    Serial.print("[I] SIM900 RSSI Received: ");
    Serial.println((const char *)strResp);
  #endif


  /*
   * ... try to decode  ..... +CSQ: 14,0 .... 
   */

  if( ( nMaxIdx > 0 ) && ( nMaxIdx < USART_RX_AT_BUFFER_LENGTH) )
  {
    for(nIdx = 0; nIdx < nMaxIdx - 4; nIdx ++)
    {
      if( ( strResp[nIdx] == ':' ) && ( strResp[nIdx + 4] == ',' ) )
      {
        if( (strResp[nIdx + 1] >= '0') && (strResp[nIdx + 1] <= '9') )
        {
          nRSSI_Local_Value = (unsigned short)(strResp[nIdx + 1] - '0');
        }

        if( (strResp[nIdx + 2] >= '0') && (strResp[nIdx + 2] <= '9') )
        {
          nRSSI_Local_Value = nRSSI_Local_Value * 10;
          nRSSI_Local_Value += (unsigned short)(strResp[nIdx + 2] - '0');
        }

        if( (strResp[nIdx + 3] >= '0') && (strResp[nIdx + 3] <= '9') )
        {
          nRSSI_Local_Value = nRSSI_Local_Value * 10;
          nRSSI_Local_Value += (unsigned short)(strResp[nIdx + 3] - '0');
        }

        break;
      }
        
    }
  }


  #if(AT_COMMAND_PROCESSOR_GSM_SIM900_DEBUG_SERIAL_ENABLE >= 1)
      Serial.print("[I] SIM900 Signal Strength: ");
      Serial.print(nRSSI_Local_Value);
      Serial.println();    
  #endif

  return nRSSI_Local_Value;
}



/*
 * One step Rx buffer decode message ... 
 */
unsigned char AT_Command_Decode_Rx_Buff(void)
{

  unsigned char nReturn = 0;
  unsigned short nIdx = 0;


  if( nAT_Resp_Rx_Idx < USART_RX_AT_BUFFER_LENGTH )
  {

    #if(AT_COMMAND_PROCESSOR_GSM_SIM900_DEBUG_SERIAL_ENABLE == 1)
      //Serial.print("[I] SIM900 Server:  ");
      //Serial.write(arr_AT_Resp_BuffRx, nAT_Resp_Rx_Idx);
      //Serial.println();    
    #endif


    /* look inside the receive buffer ... */
    for( nIdx = 0; nIdx < nAT_Resp_Rx_Idx; nIdx ++ )  
    {
      
      /* 
       * An valid server response usually starts with several "***"
       */
      if( (arr_AT_Resp_BuffRx[nIdx] == 0) || (arr_AT_Resp_BuffRx[nIdx] == '*'))
        break;


      /*
       * Look for "CMD" ... chars ...
       */
      if( nIdx < (nAT_Resp_Rx_Idx - 5)  )
      {
        if( ( arr_AT_Resp_BuffRx[nIdx] == 'C' ) && ( arr_AT_Resp_BuffRx[nIdx + 1] == 'M' ) && ( arr_AT_Resp_BuffRx[nIdx + 2] == 'D' ) )
        {
          /* Normal Alive response ... */
          if( arr_AT_Resp_BuffRx[nIdx + 3] == 'A' ) 
          {
            nReturn = 'A';

            #if(AT_COMMAND_PROCESSOR_GSM_SIM900_DEBUG_SERIAL_ENABLE == 1)
              Serial.println("[I] SIM900 Server response --> CMDA ");
            #endif

            break;
          }

          /* reset cash request from server .... */
          if( arr_AT_Resp_BuffRx[nIdx + 3] == 'B') 
          {
            nReturn = 'B';

            #if(AT_COMMAND_PROCESSOR_GSM_SIM900_DEBUG_SERIAL_ENABLE == 1)
              Serial.println("[I] SIM900 Server response --> CMDB ");
            #endif

            break;
          }

        }

      }/* end if if( nIdx < (nAT_Resp_Rx_Idx - 5)  ) */

    }/* end for(nIdx = 0; nIdx < nAT_Resp_Rx_Idx; nIdx ++)  */


  }/* end if( nAT_Resp_Rx_Idx < USART_RX_AT_BUFFER_LENGTH ) */


  
  return nReturn;
}


/*
 * One step Rx buffer decode message ... 
 */
unsigned char AT_Command_Decode_Rx_Buff_v2(void)
{

  unsigned char nReturn = 0;
  unsigned short nIdx = 0;
  unsigned char  nCopyIdx = 0;


  /*
   * if no errors in response communication buffer ...
   */
  if( nAT_Resp_Rx_Idx < USART_RX_AT_BUFFER_LENGTH  )
  {

    #if(AT_COMMAND_PROCESSOR_GSM_SIM900_DEBUG_SERIAL_ENABLE == 1)
      Serial.print("[I] SIM900 Server:  ");
      Serial.write((char*)arr_AT_Resp_BuffRx, nAT_Resp_Rx_Idx);
      Serial.println();    
    #endif


    /* response buffer looks like : CMDx********* */
    /* look inside the receive buffer ... for the first 'C' */
    for( nIdx = 0; nIdx < nAT_Resp_Rx_Idx; nIdx ++ )  
    {
      
      /* 
       * An valid server response usually starts with 'C'
       */
      if( arr_AT_Resp_BuffRx[nIdx] != 'C' ) 
        continue;
      else
        break;
    }

    /* response is long enough ... */
    if( ( nIdx + 3 ) < nAT_Resp_Rx_Idx )
    {

      /*
      * Look for "CMD" ... chars ...
      */
      if( ( arr_AT_Resp_BuffRx[nIdx] == 'C' ) && ( arr_AT_Resp_BuffRx[nIdx + 1] == 'M' ) && ( arr_AT_Resp_BuffRx[nIdx + 2] == 'D' ) )
      {

        nReturn = arr_AT_Resp_BuffRx[nIdx + 3];          

        #if(AT_COMMAND_PROCESSOR_GSM_SIM900_DEBUG_SERIAL_ENABLE == 1)
          Serial.println();
          Serial.print("[I] SIM900 Server response --> CMD - ");
          Serial.println(nReturn);
        #endif


        if(arr_AT_Resp_BuffRx[nIdx + 3] > 'A')
        {

          /*
          * copy command data to local buffer ... 
          */
          nIdx += 4;
          nCopyIdx = 0;
          
          while( nIdx < nAT_Resp_Rx_Idx )
          {
            /* 
            * we shall add a forced NULL character at the end 
            */
            if( ( nCopyIdx < (AT_MAX_DATA_FROM_SERVER_COMMAND - 1) ) &&  ( arr_AT_Resp_BuffRx[ nIdx ] != '*' ) )
            {

              arrLastServerResponse_Data[ nCopyIdx ] = arr_AT_Resp_BuffRx[ nIdx ];

              nCopyIdx ++;
              nIdx ++;
            }
            else
              break;

          }/* end while ...*/

          /* add the NULL terminator ... */
          arrLastServerResponse_Data[ nCopyIdx ] = 0;
          nLastServerResponse_Data_Len = nCopyIdx;

        }/* if(arr_AT_Resp_BuffRx[nIdx + 3] > 'A') */
        else
        {
          arrLastServerResponse_Data[0] = 0;
          nLastServerResponse_Data_Len = 0;
        }

      }/*  if( ( arr_AT_Resp_BuffRx[nIdx] == 'C' ) && ( arr_AT_Resp_BuffRx[nIdx + 1] == 'M' ) && ( arr_AT_Resp_BuffRx[nIdx + 2] == 'D' ) ) */

    }/* if( ( nIdx + 3 ) < nAT_Resp_Rx_Idx ) */

  }/* end if( nAT_Resp_Rx_Idx < USART_RX_AT_BUFFER_LENGTH ) */

  
  return nReturn;

}/* unsigned char AT_Command_Decode_Rx_Buff_v2(void) */


unsigned char AT_Command_Is_New_Command_From_Server(void)
{
  return nLastServerResponse_Code_Not_Processed;
}


unsigned char AT_Command_Read_Last_Command_From_Server(void)
{
  unsigned char nReturn = 0;

  if( nLastServerResponse_Code_Not_Processed > 0 )
  {
    nReturn = nLastServerResponse_Code;
  }

  return nReturn;
}

unsigned char* AT_Command_Get_Data_From_Last_Command_From_Server(void)
{
  return &arrLastServerResponse_Data[0];
}

void AT_Command_Process_Command_From_Server(void)
{
  nLastServerResponse_Code_Not_Processed = 0;

  nLastServerResponse_Data_Len = 0;
}


unsigned char AT_Command_Processor_Is_Iddle(void)
{
  unsigned char nReturn = 0;

  if( eAT_Processor_State == AT_STATE_PROCESSOR_IDDLE )
  {
    nReturn = 1;
  }

  return nReturn;  
}