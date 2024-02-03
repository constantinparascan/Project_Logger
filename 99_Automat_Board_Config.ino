/*
 * v0.1 - used for specific HW/Application configuration
 *
 *     Contains board configuration specific API's.
 */



/* 3 = PARALLEL
 * 2 = CCTALK
 * 1 = Not Used
 * 0 = ERROR - deffective config pins
 */
unsigned char nHW_Config_TYPE = 0;

/* 0 = SIM GSM
 * 1 = Ethernet W5100
 */
unsigned char nHW_Config_COMM_TYPE = 0;

/*********************************************************/

/*
 * Setters and ... Getters 
 */
unsigned char Board_Config_Get_HW_ConfigType(void)
{
  return nHW_Config_TYPE;
}


/********************************************************/


/* 
 * GPIO pins used for the Boad configuration of functionality
 *
 *   - currently supported only Parallel/Serial communication with bill validator:
 *
 *          CONFIG_2    CONFIG_1
 *         ----------------------
 *             1           1         -> parallel   -- no jumper connected
 *             1           0         -> cctalk     -- jumper connected on PL2 and PL4
 *             0           1         -> N/A
 */
#define BOARD_TYPE_CONFIG_PIN_CONFIG_1      (45)    /* PL4 */
#define BOARD_TYPE_CONFIG_PIN_CONFIG_2      (49)    /* PL0 */
#define BOARD_TYPE_CONFIG_PIN_SOURCE_GPIO   (47)    /* PL2 */


/* v0.1
 * 
 * GPIO pins used for the Boad configuration of functionality
 *
 *   - currently supported only Parallel/Serial communication with bill validator:
 *
 *          CONFIG_2    CONFIG_1
 *         ----------------------
 *             1           1         -> parallel   -- no jumper connected
 *             1           0         -> cctalk     -- jumper connected on PL2 and PL4
 *             0           1         -> N/A
 *             0           0         -> undefined -- HW ERROR ?!?
 */


/* delay 62.5ns on a 16MHz AtMega */
#define NOP __asm__ __volatile__ ("nop\n\t")

unsigned char Logger_App_DetectConfiguration (void)
{
  int nReadOut = 0;

  #if(AUTOMAT_BOARD_CONFIG_DEBUG_ENABLE >= 1)
    //Serial.begin(9600);
  #endif

  pinMode(BOARD_TYPE_CONFIG_PIN_CONFIG_1,   INPUT_PULLUP);
  pinMode(BOARD_TYPE_CONFIG_PIN_CONFIG_2,   INPUT_PULLUP);
  pinMode(BOARD_TYPE_CONFIG_PIN_SOURCE_GPIO, OUTPUT);


  /* no selection ... make sure all read-outs are high !! - validate HW sanity - */
  digitalWrite(BOARD_TYPE_CONFIG_PIN_SOURCE_GPIO, HIGH);

  NOP;
  NOP;
  NOP;
  NOP;


  nReadOut =  digitalRead(BOARD_TYPE_CONFIG_PIN_CONFIG_1);
  nReadOut &= digitalRead(BOARD_TYPE_CONFIG_PIN_CONFIG_2);

  /* must be 1 in order to have a valid HW */
  if( nReadOut )
  {
    /* write "0" on GPIO source pin ... */
    digitalWrite(BOARD_TYPE_CONFIG_PIN_SOURCE_GPIO, LOW);

    NOP;
    NOP;
    NOP;
    NOP;

    nReadOut =  digitalRead(BOARD_TYPE_CONFIG_PIN_CONFIG_2);


    nReadOut = nReadOut << 1;
    nReadOut |= digitalRead(BOARD_TYPE_CONFIG_PIN_CONFIG_1);

  }  


  /*
   * Configuration read-out is complete !
   *
   * make PIN as input ... protect for possible short circuits !! */
  pinMode(BOARD_TYPE_CONFIG_PIN_SOURCE_GPIO, INPUT);

  #if(AUTOMAT_BOARD_CONFIG_DEBUG_ENABLE == 1)

    if( (nReadOut & 0x03) == 0x03 )
      Serial.println(" CONFIG BOARD -- PARALLEL ");

    if( (nReadOut & 0x03) == 0x02 )
      Serial.println(" CONFIG BOARD -- CCTALK ");

    if( (nReadOut & 0x03) == 0x01 )
      Serial.println(" CONFIG BOARD -- NOT USED ");    
  #endif


  nHW_Config_TYPE = (unsigned char)(nReadOut & 0x03);

}/* end function Logger_App_DetectConfiguration */



#if ( BOARD_DIAGNOSTIC_AND_INITIALIZATION_ENABLE == 1 )

#define USART3_RX_BUFFER_LENGTH (100)
#define USART3_TX_BUFFER_LENGTH (100)

static unsigned char  arrDiag_USART0_Rx_Buff[100] = {0};
static unsigned short nUSART0_RX_Idx = 0;

static unsigned char arrDiag_USART0_Tx_Buff[100] = {0};
static unsigned short nUSART0_TX_Idx = 0;

static unsigned char arrDiag_USART3_Rx_Buff[USART3_RX_BUFFER_LENGTH] = {0};
static unsigned short nUSART3_RX_Idx = 0;

static unsigned char arrDiag_USART3_Tx_Buff[USART3_TX_BUFFER_LENGTH] = {0};
static unsigned short nUSART3_TX_Idx = 0;


ISR(USART3_RX_vect)
{

  USART_RX_Buff[nUSART_RX_Idx] = UDR1;

  if( nUSART3_RX_Idx < ( USART3_RX_BUFFER_LENGTH - 1 ) )
  {
    nUSART3_RX_Idx ++;
  }

}




#endif /* ( BOARD_DIAGNOSTIC_AND_INITIALIZATION_ENABLE == 1 ) */
