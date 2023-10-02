/* C.Parascan
 *
 * Ethernet driver for Arduino shield W5100
 * 
 *  module version [see Eth_Service_Client.h] - reviewed and tested on 20.02.2023
 *
 */
#include "05_Eth_Service_Client.h"
#include "05_Eth_Service_Client_cfg.h"
#include "99_Automat_Board_cfg.h"
#include <SPI.h>

#include <Ethernet.h>
#include <string.h>


// Initialize the Ethernet client library
// with the IP address and port of the server
// that you want to connect to (port 80 is default for HTTP):
EthernetClient EthClient;

/* current state of the controller ... */
T_ETH_State EthClientState = E_ETH_UNININT;

/* local message buffer */
char strEthTxMessageBuff[ETH_CLIENT_TO_SERVER_BUFFER_LENGTH] = {0};
static char strEthRxMessageBuff[ETH_SERVER_TO_CLIENT_BUFFER_LENGTH] = {0};

static unsigned int nEthLocalDelay = ETH_SERVICE_CLIENT_STARTUP_DELAY;

unsigned char nReceptionTimerLimit = ETH_SERVER_RECEPTION_MAX_TIMEOUT_WAITING;

#if(ETH_SERVICE_CLIENT_DEBUG_SERIAL == 1)
static unsigned char nEth_Debug_Flags = 0;
#endif



/*
 * Initialize the Ethernet board hardware !
 *    - if sucessful we should have an IP, MAC and a DNS configured !
 *
 */
void Eth_Service_Client_init(void)
{
  // Enter a MAC address for your controller below.
  // Newer Ethernet shields have a MAC address printed on a sticker on the shield  
  byte mac[] = { MAC_BYTE_00, MAC_BYTE_01, MAC_BYTE_02, MAC_BYTE_03, MAC_BYTE_04, MAC_BYTE_05 };

  IPAddress ip(IP_ADDRESS_BYTE_0, IP_ADDRESS_BYTE_1, IP_ADDRESS_BYTE_2, IP_ADDRESS_BYTE_3);

  IPAddress myDns(DNS_ADDRESS_BYTE_0, DNS_ADDRESS_BYTE_1, DNS_ADDRESS_BYTE_2, DNS_ADDRESS_BYTE_3);


  #if(ETH_SERVICE_CLIENT_DEBUG_SERIAL == 1)
    Serial.begin(9600); 
    Serial.println("[I] Eth Debug ...");

    nEth_Debug_Flags = 0;
  #endif

  /* start the Ethernet connection: Initialize Ethernet with DHCP  */
  if( Ethernet.begin(mac) == 0 )
  {
      /* Failed to configure Ethernet using DHCP ... */
      #if(ETH_SERVICE_CLIENT_DEBUG_SERIAL == 1)
        Serial.println("[W] Failed to configure Ethernet using DHCP, try to static config !");
      #endif

      /* check for missing hardware ... */
      if ( Ethernet.hardwareStatus() == EthernetNoHardware ) 
      {
          /* [ToDo] - mark error ... missing hardware ... critical error           */
          /* Ethernet shield was not found.  Sorry, can't run without hardware. :( */

          #if(ETH_SERVICE_CLIENT_DEBUG_SERIAL == 1)
            Serial.println("[E] Ethernet HW cannot be initialized ! CRITICAL ERROR !");
          #endif

          EthClientState = E_ETH_ERROR_UNRECOVERABLE;
      }
      else
      {
        /* Ethernet cable is not connected ? */
        if( Ethernet.linkStatus() == LinkOFF )
        {
            /* [ToDo] - recoverable error ... if cable is plugged in again ...       */
            
            #if(ETH_SERVICE_CLIENT_DEBUG_SERIAL == 1)
              Serial.println("[E] Ethernet cable not connected ! CRITICAL - but Rercoverable ERROR !");
            #endif

            EthClientState = E_ETH_ERROR_RECOVERABLE_BY_RESET;
        }
        else
        {
          /* try to configure using IP address instead of DHCP: */
          Ethernet.begin(mac, ip, myDns);

          EthClientState = E_ETH_INIT_WAIT;          
        }

      }/* else if ( Ethernet.hardwareStatus() == EthernetNoHardware )  */
        
  }/* end if Ethernet.begin(mac) */
  else
  {
    #if(ETH_SERVICE_CLIENT_DEBUG_SERIAL == 1)
      Serial.println("[I] DHCP assigned IP: ");
      Serial.println( Ethernet.localIP() );
    #endif
    
    EthClientState = E_ETH_INIT_WAIT;
  }


  /* [ToDo] ... if you want to force the initialization on a defined .. IP .. */
  /* Ethernet.begin(mac, ip, myDns); */

  /* be prepared for the startup delay ... if needed ... */
  nEthLocalDelay = ETH_SERVICE_CLIENT_STARTUP_DELAY;
} /* Eth_Service_Client_init_phase_1 */



/*
 * Main function of the Eth driver !!!
 *
 *   - is called ciclically !
 *
 *   - will send messages to a statically configured server see -> "Eth_Service_Client_cfg.h"
 *   - waits for the server response and store-its into local buffer
 */
void Eth_Service_Client_main(void)
{
    int nRxDataLen = 0;

    switch ( EthClientState )
    {
      /*
       * driver is not initialized
       */
      case E_ETH_UNININT:

        /* 
         * a previous Init has failed ... wait until retry ...
         */
        if(nEthLocalDelay > 0)
        {
          nEthLocalDelay --;
        }
        else
        {
          Eth_Service_Client_init();
        }

        /*
         *  state driven inside "init" function ...
         *  no need to chenge state !!!
         */        
      break;

      /* 
       * implement startup delay ... 
       */
      case E_ETH_INIT_WAIT:

        if(nEthLocalDelay > 0)
        {
          nEthLocalDelay --;
        }
        else
        {
          EthClientState = E_ETH_WAIT_COMMAND;
        }            
      break;


      /* 
       * Wait for user commands !
       *    - state is advanced inside user request API's
       */
      case E_ETH_WAIT_COMMAND:

        /* wait indefinitly ... ? ... maybe PING from time to time ...*/

          #if(ETH_SERVICE_CLIENT_DEBUG_SERIAL == 1)
            if(nEth_Debug_Flags < 1)
            {
              Serial.println("[I] Eth Wait Commands ... ! ");
              nEth_Debug_Flags ++;
            }
          #endif

      break;

      /*
       * Starts a new connection to Server
       */
      case E_ETH_CONNECT_TO_SERVER:
        if( EthClient.connect(SERVER_ADDRESS_DESTINATION, SERVER_ADDRESS_DESTINATION_PORT) )
        {
            #if(ETH_SERVICE_CLIENT_DEBUG_SERIAL == 1)
              Serial.print("[I] Connected to: " );
              Serial.println(EthClient.remoteIP());

              nEth_Debug_Flags = 0;
            #endif

          EthClientState = E_ETH_SERVER_TRANSMISSION;
        }
        else
        {
          #if(ETH_SERVICE_CLIENT_DEBUG_SERIAL == 1)                      
            Serial.print("[E] Connection failed to:       ");
            Serial.println(SERVER_ADDRESS_DESTINATION);
          #endif

          EthClientState = E_ETH_ERROR_RECOVERABLE_BY_RESET;        
        }
      break;


      /*
       * Send request to W5100 board for client - server communication
       */
      case E_ETH_SERVER_TRANSMISSION:

        Eth_Service_Client_StackBuffRequest();

        #if(ETH_SERVICE_CLIENT_DEBUG_SERIAL == 1)
          Serial.write(strEthTxMessageBuff, strlen(strEthTxMessageBuff));
          Serial.println();
        #endif

        EthClientState = E_ETH_SERVER_RECEPTION;

        /* deadline reception monitoring time ! */
        nReceptionTimerLimit = ETH_SERVER_RECEPTION_MAX_TIMEOUT_WAITING;

      break;
      

      /*  Limit here to most 5-10 itterations !! 
       *      - if not sucessfull ... transition immediately to finished !!!!!
       *      - communication data chunks are limited to buffer defined "ETH_SERVER_TO_CLIENT_BUFFER_LENGTH"
       */
      case E_ETH_SERVER_RECEPTION:
        {
          /* check to see if W5100 has received anything from Server */
          nRxDataLen = EthClient.available();

          if( nRxDataLen > 0 )
          {
            /* limit data that we can locally process ... */
            if( nRxDataLen > ETH_SERVER_TO_CLIENT_BUFFER_LENGTH )
              nRxDataLen = ETH_SERVER_TO_CLIENT_BUFFER_LENGTH;
              
            /* read one chunk of data from W5100 into local buffer */
            EthClient.read(strEthRxMessageBuff, nRxDataLen);

            #if(ETH_SERVICE_CLIENT_DEBUG_SERIAL == 1)
              Serial.write(strEthRxMessageBuff, nRxDataLen);
              Serial.println("[--> ************************************<--]");
            #endif            


            /* [ToDo] --- check !!!!!! */

            /* Reset reception delay ... there is still data to be received ... */
            /* deadline reception monitoring time ! */
            nReceptionTimerLimit = ETH_SERVER_RECEPTION_MAX_TIMEOUT_WAITING;
          }

          /* if connection lost or communication complete ... advance state */
          if(EthClient.connected() == 0)
          {
            EthClientState = E_ETH_SERVER_RECEPTION_FINISHED;
          }


          /* force transition to finished even if not completed */
          /* maybee server is stuck or any other reason         */
          if(nReceptionTimerLimit > 0)
          {
            nReceptionTimerLimit --;
          }
          else
          {
            EthClientState = E_ETH_SERVER_RECEPTION_FINISHED;
          }
        
        }
      break;

      /*
       * Transmission + Reception - finished
       *     [ToDo]- callback and notification functions can be executed !
       */
      case E_ETH_SERVER_RECEPTION_FINISHED:

        // EthClient.stop();

        /* Re-init ... prepare for a new transmission when requested ... */

        #if(ETH_SERVICE_CLIENT_DEBUG_SERIAL == 1)
          Serial.println("[I] Reception finished ... ");
        #endif

        EthClientState = E_ETH_WAIT_COMMAND;

      break;


      case E_ETH_ERROR_RECOVERABLE_BY_RESET:

        EthClient.stop();

        #if(ETH_SERVICE_CLIENT_DEBUG_SERIAL == 1)
          Serial.println("[I] Error recoverable by reset ... re-init ... ");
        #endif


        // ??? 
        // state is changed inside the init function ...
        //
        EthClientState = E_ETH_UNININT;
      break;

      case E_ETH_ERROR_UNRECOVERABLE:

        // ??? shall we retry ? after a while ??? 

        EthClient.stop();
        EthClientState = E_ETH_UNININT;

      break;

      default:
          /* unknown state ... go to init ?*/
          EthClientState = E_ETH_INIT_WAIT;
      break;

    }/* end switch */
}


/***************************************************/
/*   Eth Buffer utilities ... 
 *
 */ 

void Eth_Service_Client_ClearTxBuff(void)
{
  /* no data in the buffer ... null termintor */
  strEthTxMessageBuff[0] = '\0'; 
}

/*
 * Functions used by the user to format the "Client to Server" request !
 *    - returns "1" if data has been added to the Tx buffer
 */
unsigned char Eth_Service_Client_AddTxData(char* strData)
{
  unsigned char nReturn = 0;

  /* check the len of the 2 strings is not greater then the length of the buffer  */
  if( strlen(strEthTxMessageBuff) + strlen(strData) < ETH_CLIENT_TO_SERVER_BUFFER_LENGTH )
  {
    strcat(strEthTxMessageBuff, strData);

    nReturn = 1;
  }

  return nReturn;
}


/*
 * Function used to send a data buffer and therefore
 *   a server request to W5100 extension board using EthClient 
 */
void Eth_Service_Client_StackBuffRequest(void)
{
  EthClient.println(strEthTxMessageBuff);
  EthClient.println("Host: " SERVER_ADDRESS_DESTINATION);
  EthClient.println("Connection: close");
  EthClient.println();

  // clean Tx buffer
  // strEthTxMessageBuff[0] = '\0';
}

void Eth_Service_Client_StackNULLRequest(void)
{
  EthClient.println();  
}


/*
 * Function used by user to retrieve the Rx data buffer of the 
 *     last valid communication
 *
 *  - returns the size of the buffer
 */
unsigned short Eth_Service_Client_Get_RxBuffer(char **RxBuff)
{
  if( RxBuff != NULL ) 
  {
    *RxBuff = &strEthRxMessageBuff[0];
  }  

  //Serial.println();
  //Serial.println("!!!!!!!!!!!!!!!!!");
  //Serial.write(strEthRxMessageBuff, ETH_SERVER_TO_CLIENT_BUFFER_LENGTH);

  return ETH_SERVER_TO_CLIENT_BUFFER_LENGTH;
}


/***************************************************/
/*   Eth comm utilities ... 
 *
 */ 

/* 
 * Function used by the client to trigger a transmission request
 *     - this function will work only if there is no transmission/reception in progress already
 *
 *     - returns 1 if request has been accepted
 */
unsigned char Eth_Service_Client_SendRequest(void)
{
  unsigned char nReturn = 0;

    if( EthClientState == E_ETH_WAIT_COMMAND )
    {
      EthClientState = E_ETH_CONNECT_TO_SERVER;

      nReturn = 1;
    }    

    return nReturn;
}


/*
 * Function used by te user to check if the Eth driver 
 *    is in "working" state (sending/receiving) data
 *    or is free and a new transmission request can be issued !
 *
 *   - returns 1 if Eth peripheral is "free"
 *   - returns 0xFF if Eth peripheral is in Error state !!!
 */
unsigned char Eth_Service_Client_IsReadyForNewCommand(void)
{
  unsigned char nReturn = 0;

  if(EthClientState == E_ETH_WAIT_COMMAND)
  {
    nReturn = 1;
  }
  else
  {

    if( (EthClientState == E_ETH_ERROR_RECOVERABLE_BY_RESET) || (EthClientState == E_ETH_ERROR_UNRECOVERABLE) )
    {
      nReturn = 0xFF;
    }

  }
  
  return nReturn;
}

