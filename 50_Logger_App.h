#ifndef __LOGGER_APP_H__
#define __LOGGER_APP_H__

#include <Arduino.h>


/* Debug levels 1 - display most messages
 *              2 - display serial received frame bytes !
 */ 
#define LOGGER_APP_DEBUG_SERIAL (0)

typedef enum TAG_LOGGER_APP
{

  E_LOGGER_APP_UNININT = 0,
  E_LOGGER_APP_INIT,

  E_LOGGER_APP_STARTUP_DELAY,


  E_LOGGER_APP_WAIT_FOR_SERVER_SERVICE_ONLINE_SEND_REQ,   
  E_LOGGER_APP_WAIT_FOR_SERVER_SERVICE_ONLINE_WAIT_RESPONSE,
  E_LOGGER_APP_WAIT_FOR_SERVER_SERVICE_ONLINE_PROCESS_RESPONSE,  

  E_LOGGER_APP_PROCESS_MESSAGES,
  E_LOGGER_APP_PROCESS_PARALLEL_MESSAGES,

  E_LOGGER_APP_ETH_INFORM_SERVER,
  E_LOGGER_APP_ETH_INFORM_SERVER_ERROR_STATE,

  E_LOGGER_APP_ETH_PROCESS_SERVER_RESPONSE,

  E_LOGGER_APP_ETH_SEND_COMMAND_RESET_BILL_ACK_RESPONSE,
  E_LOGGER_APP_ETH_SEND_STARTUP_COMMAND,

  E_LOGGER_APP_ETH_SAVE_DATA_TO_EEPROM

}T_LoggerAppState;

typedef enum TAG_LOGGER_OUTPUT_PIN_STATE
{
  
  E_LOGGER_OUTPUT_INNACTIVE = 0,

  E_LOGGER_OUTPUT_PULSE_REQ_L_H,
  E_LOGGER_OUTPUT_PULSE_REQ_DELAY,
  E_LOGGER_OUTPUT_PULSE_REQ_H_L

}T_LoggerAppOutputPinState;

/* exported API's */
void Logger_App_pre_init_NvM_link(void);
void Logger_App_Write_Default_Values_NvM(unsigned char nSlotWrite);

void Logger_App_init(void);
void Logger_App_cyclic_1sec(void);
void Logger_App_main(void);

unsigned short Logger_App_Get_Channel_Values(unsigned char nChannValue);
unsigned char  Logger_App_Get_LastBillType(void);
unsigned char  Logger_App_Get_LastErrorValue(void);


void Logger_App_Output_Pins_init(void);
void Logger_App_Output_Pins_main(void);


unsigned char Logger_App_DetectConfiguration (void);



#endif /* __LOGGER_APP_H__ */