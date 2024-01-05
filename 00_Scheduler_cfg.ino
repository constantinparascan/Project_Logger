/* C.Parascan
 *
 *  This is the configuration file for the Scheduler. 
 *     Here all the cyclic tasks and initialization functions of the drivers and user 
 *     applications are called.
 * 
 *  module version [ see Scheduler.h ] - reviewed and tested on 20.02.2023
 *  module version 1.0.6 - added debugging capability and reset to default of NvM blocks on 29.12.2023
 */

#include <avr/wdt.h>


#include "00_Scheduler.h"
#include "00_Scheduler_cfg.h"

#include "99_Automat_Board_cfg.h"
#include "99_Automat_Board_Config.h"

/* include headers of drivers/applications */
#include "05_Eth_Service_Client.h"
#include "50_Logger_App.h"
#include "50_Logger_App_Parallel.h"
#include "07_USART_Client.h"
#include "09_EEPROM_Driver.h"
#include "09_NvM_Manager.h"
#include "09_NvM_Manager_cfg.h"
#include "60_GSM_SIM900.h"

#include "03_Com_Service.h"


/*
 * Ram mirror of NvM bocks used inside the application
 */
extern unsigned short nNvM_RAM_Mirror_CH1;
extern unsigned short nNvM_RAM_Mirror_CH2;
extern unsigned short nNvM_RAM_Mirror_CH3;
extern unsigned short nNvM_RAM_Mirror_CH4;
extern unsigned short nNvM_RAM_Mirror_CH5;

extern unsigned char arrNvM_RAM_Mirror_Str_SimNR   [ ];
extern unsigned char arrNvM_RAM_Mirror_Str_Town    [ ];
extern unsigned char arrNvM_RAM_Mirror_Str_Place   [ ];
extern unsigned char arrNvM_RAM_Mirror_Str_Details [ ];
extern unsigned char arrNvM_RAM_Mirror_Str_Device  [ ];


unsigned char nNvM_Header[3] = {'N', 'v', 'M'};

/* Local variable keeping the current configuration read-out ... */
unsigned char nHW_Config_TYPE_Local = 0;


/*********************************************************************/
/*
 *
 *                 Task INIT callbacks ...
 *
 */
/*********************************************************************/
/*
 *  General usage 1 ms reccurence task - init function
 */
void init_cyclic_Task_1ms(void)
{

  /* 
   * Detect current HW configuration 
   */
  Logger_App_DetectConfiguration();
  nHW_Config_TYPE_Local = Board_Config_Get_HW_ConfigType();


  EEPROM_driver_init();  

  /*
   *  INIT NvM before any other applications uses RAM immage ... 
   */

  /* Block ID - 0 -is a special block containing only the NvM pattern and later the version ...   */
  NvM_AttachRamImage((unsigned char*)&nNvM_Header[3], 0);

  /* NvM attach to rest of blocks ... Ram immage */
  Logger_App_pre_init_NvM_link();


  //Serial.println("NVM INIT ... ");
  NvM_Init();

  //Serial.println("NVM READ ALL ... during startup ... ");
  NvM_ReadAll();


  /*
   * ... special NvM blocks ... write default values
   *     it's mandatory not to have empty/not written blocks here
   */

  /* no NvM entry for the phone number ... */
  if( ( NvM_Get_Block_Status(NVM_BLOCK_CHAN_6_ID) & 0x80 ) == 0x00 )
  {
    strcpy(arrNvM_RAM_Mirror_Str_SimNR, LOG_APP_SIM_PHONE_NR);
    NvM_Write_Block(NVM_BLOCK_CHAN_6_ID);
  }


  /* no NvM entry for the Town        ... */
  if( ( NvM_Get_Block_Status(NVM_BLOCK_CHAN_7_ID) & 0x80 ) == 0x00 )
  {
    strcpy(arrNvM_RAM_Mirror_Str_Town, LOG_APP_TOWN);
    NvM_Write_Block(NVM_BLOCK_CHAN_7_ID);
  }

  /* no NvM entry for the Place       ... */
  if( ( NvM_Get_Block_Status(NVM_BLOCK_CHAN_8_ID) & 0x80 ) == 0x00 )
  {
    strcpy(arrNvM_RAM_Mirror_Str_Place, LOG_APP_PLACE);
    NvM_Write_Block(NVM_BLOCK_CHAN_8_ID);
  }

  /* no NvM entry for the Details       ... */
  if( ( NvM_Get_Block_Status(NVM_BLOCK_CHAN_9_ID) & 0x80 ) == 0x00 )
  {
    strcpy(arrNvM_RAM_Mirror_Str_Details, LOG_APP_DETAILS);
    NvM_Write_Block(NVM_BLOCK_CHAN_9_ID);
  }

  /* no NvM entry for the Device Type   ... */
  if( ( NvM_Get_Block_Status(NVM_BLOCK_CHAN_10_ID) & 0x80 ) == 0x00 )
  {
    strcpy(arrNvM_RAM_Mirror_Str_Device, LOG_APP_DEVICE_TYPE);
    NvM_Write_Block(NVM_BLOCK_CHAN_10_ID);
  }

  Logger_App_pre_init_Validate_NvM_String_Data();

}/* init_cyclic_Task_1ms */


/*
 *  General usage 5 ms reccurence task - init function
 */
void init_cyclic_Task_5ms(void)
{
  Com_Service_Init();

  /* 
   * SIM900 driver init
   */
  AT_Command_Processor_Init();


  /* 
   *  - init Application depending on the HW configuration 
   */
  /* PARALLEL configuration ... */ 
  if( nHW_Config_TYPE_Local == AUTOMAT_BOARD_CONFIG_HW_TYPE_PARALLEL )
  {  
    Logger_App_Parallel_init();
  }
  else
  {
    /* CCTALK configuration ... */
    if( nHW_Config_TYPE_Local == AUTOMAT_BOARD_CONFIG_HW_TYPE_CCTALK )
    {
      USART_Init();

      Logger_App_init();
    }
  }
}

/*
 *  General usage 10 ms reccurence task - init function
 */
void init_cyclic_Task_10ms(void)
{

}


/*
 *  General usage 1s reccurence task - init function
 */
void init_Task_1s(void)
{
  /*
   * Alive blink on-board LED ...
   */
  pinMode(LED_BUILTIN, OUTPUT);   

}


/*********************************************************************/
/*
 *
 *                 Task cyclic callbacks ...
 *
 */
/*********************************************************************/


/*
 *  General usage 1 ms reccurence task - cyclic function
 */
void main_cyclic_Task_1ms(void)
{
  /* - EEPROM - */
  EEPROM_driver_main();
  

  /* 
   * execute Main function depending on HW configuration 
   */
  if( nHW_Config_TYPE_Local == AUTOMAT_BOARD_CONFIG_HW_TYPE_PARALLEL )
  {
    Logger_App_Parallel_main_v2();
  }
  else
  {
    if( nHW_Config_TYPE_Local == AUTOMAT_BOARD_CONFIG_HW_TYPE_CCTALK )
    {
      USART_main();

      Logger_App_main_v2();
    }
  }  

}

/*
 *  General usage 5 ms reccurence task - cyclic function
 */
void main_cyclic_Task_5ms(void)
{

  Com_Service_main();

  /* SIM900 - driver !! - */
  AT_Command_Processor_Main();

}

/*
 *  General usage 10 ms reccurence task - cyclic function
 */
void main_cyclic_Task_10ms(void)
{

  /* normally planned to be used in 5ms task because 1 EEPROM byte writting takes 3.8 ms at most ... ?!?  10ms is also OK from CPU load perspective ...  */
  NvM_Main();

}


/*
 *  General usage 1s reccurence task - cyclic function
 */
/*
 *  Alive task 
 *     - blink the LED
 *     - serve the Watchdog !
 *   
 *     - other 1 sec callback functions 
 */


void main_cyclic_Task_1s(void)
{
	static uint8_t LED_Val = 0;

  /* Alive Heart Beat - 1 sec -  */
	if(LED_Val == 0)
	{
  	digitalWrite(LED_BUILTIN, HIGH);

  	LED_Val = 1;
  }
  else
  {
   	digitalWrite(LED_BUILTIN, LOW);

   	LED_Val = 0;
  }
 
  /* call applications 1 sec cyclic function */
  Com_Service_cyclic_1sec();

  /*
   *  Keep board alive !!
   */
  wdt_reset(); // Reset the watchdog

}



#if (BOARD_DIGNOSTIC_AND_INITIALIZATION_ENABLE == 0)
/*****************************************************
 *
 *          OS Scheduler TCB configuration 
 *
 *****************************************************/

/*
 *
 * T_TaskControlBlock { Task_func_ptr_Type ptrTask;
                        Task_func_ptr_Type ptrInitFcn;
	                      short Task_MaxItter;
	                      short Task_CurrentItter;
                      };
 */
T_TaskControlBlock TCB_OS[MAX_TASK_LIST_LEN] = 
{
    {main_cyclic_Task_1ms, init_cyclic_Task_1ms, 1, 0},      /* HIGHEST prio task */
   	{main_cyclic_Task_5ms, init_cyclic_Task_5ms, 5, 3},
	  {main_cyclic_Task_10ms, init_cyclic_Task_10ms, 10, 5},
	  {main_cyclic_Task_1s, init_Task_1s, 1000, 999}           /* LOWEST prio task */
};

#else  
/***************************************************************************
 *
 *          OS Scheduler TCB configuration  DEBUG CONFIG ENABLED !!!!
 *
 ***************************************************************************/


/* !!! DIGNOSTIC enabled !!!  
 *
 *  - this feature is used for various initializations required outside normal board functionality
 *  - current usage: - set EEPROM value to a default value
 *  - 
 */

#define DEBUG_EEPROM_DEFAULT_RESTORE_VALUE (0)

void init_cyclic_DIAG_Task_1ms(void)
{

  Serial.begin(9600); 
  Serial.println("[I] !!! DEBUG & SERVICE MODE ACTIVE !!!  ");

  EEPROM_driver_init();  

  /*
   *  INIT NvM before any other applications uses RAM immage ... 
   */

  /* Block ID - 0 -is a special block containing only the NvM pattern and later the version ...   */
  NvM_AttachRamImage((unsigned char*)&nNvM_Header[3], 0);

  /* NvM attach to rest of blocks ... Ram immage */
  Logger_App_pre_init_NvM_link();


  //Serial.println("NVM INIT ... ");
  NvM_Init();

  //Serial.println("NVM READ ALL ... during startup ... ");
  NvM_ReadAll();


  nNvM_RAM_Mirror_CH1 = DEBUG_EEPROM_DEFAULT_RESTORE_VALUE;
  NvM_Write_Block(NVM_BLOCK_CHAN_1_ID);
  Serial.print("[I] Set NvM Block 1 - CH1 value to: ");
  Serial.print(DEBUG_EEPROM_DEFAULT_RESTORE_VALUE);
  Serial.println();
  
  nNvM_RAM_Mirror_CH2 = DEBUG_EEPROM_DEFAULT_RESTORE_VALUE;
  NvM_Write_Block(NVM_BLOCK_CHAN_2_ID);
  Serial.print("[I] Set NvM Block 2 - CH2 value to: ");
  Serial.print(DEBUG_EEPROM_DEFAULT_RESTORE_VALUE);
  Serial.println();

  nNvM_RAM_Mirror_CH3 = DEBUG_EEPROM_DEFAULT_RESTORE_VALUE;
  NvM_Write_Block(NVM_BLOCK_CHAN_3_ID);
  Serial.print("[I] Set NvM Block 3 - CH3 value to: ");
  Serial.print(DEBUG_EEPROM_DEFAULT_RESTORE_VALUE);
  Serial.println();


  nNvM_RAM_Mirror_CH4 = DEBUG_EEPROM_DEFAULT_RESTORE_VALUE;
  NvM_Write_Block(NVM_BLOCK_CHAN_4_ID);
  Serial.print("[I] Set NvM Block 4 - CH4 value to: ");
  Serial.print(DEBUG_EEPROM_DEFAULT_RESTORE_VALUE);
  Serial.println();


  nNvM_RAM_Mirror_CH5 = DEBUG_EEPROM_DEFAULT_RESTORE_VALUE;
  NvM_Write_Block(NVM_BLOCK_CHAN_5_ID);
  Serial.print("[I] Set NvM Block 5 - CH5 value to: ");
  Serial.print(DEBUG_EEPROM_DEFAULT_RESTORE_VALUE);
  Serial.println();


 /* ------------------------------------------------------------------- */

  strcpy(arrNvM_RAM_Mirror_Str_SimNR, LOG_APP_SIM_PHONE_NR);
  NvM_Write_Block(NVM_BLOCK_CHAN_6_ID);
  Serial.print("[I] Set NvM Block 6 value to: " LOG_APP_SIM_PHONE_NR);
  Serial.println();

  strcpy(arrNvM_RAM_Mirror_Str_Town, LOG_APP_TOWN);
  NvM_Write_Block(NVM_BLOCK_CHAN_7_ID);
  Serial.print("[I] Set NvM Block 7 value to: " LOG_APP_TOWN);
  Serial.println();

  
  strcpy(arrNvM_RAM_Mirror_Str_Place, LOG_APP_PLACE);
  NvM_Write_Block(NVM_BLOCK_CHAN_8_ID);
  Serial.print("[I] Set NvM Block 8 value to: " LOG_APP_PLACE);
  Serial.println();

  strcpy(arrNvM_RAM_Mirror_Str_Details, LOG_APP_DETAILS);
  NvM_Write_Block(NVM_BLOCK_CHAN_9_ID);
  Serial.print("[I] Set NvM Block 9 value to: " LOG_APP_DETAILS);
  Serial.println();


  strcpy(arrNvM_RAM_Mirror_Str_Device, LOG_APP_DEVICE_TYPE);
  NvM_Write_Block(NVM_BLOCK_CHAN_10_ID);
  Serial.print("[I] Set NvM Block 10 value to: " LOG_APP_DEVICE_TYPE);
  Serial.println();



  /*
   * ... special NvM blocks ... write default values
   *     it's mandatory not to have empty/not written blocks here
   */

  /* no NvM entry for the phone number ... */
/*  
  if( ( NvM_Get_Block_Status(NVM_BLOCK_CHAN_6_ID) & 0x80 ) == 0x00 )
  {
    strcpy(arrNvM_RAM_Mirror_Str_SimNR, LOG_APP_SIM_PHONE_NR);
    NvM_Write_Block(NVM_BLOCK_CHAN_6_ID);
  }
*/

  /* no NvM entry for the Town        ... */
/*  
  if( ( NvM_Get_Block_Status(NVM_BLOCK_CHAN_7_ID) & 0x80 ) == 0x00 )
  {
    strcpy(arrNvM_RAM_Mirror_Str_Town, LOG_APP_TOWN);
    NvM_Write_Block(NVM_BLOCK_CHAN_7_ID);
  }
*/

  /* no NvM entry for the Place       ... */
/*
  if( ( NvM_Get_Block_Status(NVM_BLOCK_CHAN_8_ID) & 0x80 ) == 0x00 )
  {
    strcpy(arrNvM_RAM_Mirror_Str_Place, LOG_APP_PLACE);
    NvM_Write_Block(NVM_BLOCK_CHAN_8_ID);
  }
*/
  /* no NvM entry for the Details       ... */
/*  
  if( ( NvM_Get_Block_Status(NVM_BLOCK_CHAN_9_ID) & 0x80 ) == 0x00 )
  {
    strcpy(arrNvM_RAM_Mirror_Str_Details, LOG_APP_DETAILS);
    NvM_Write_Block(NVM_BLOCK_CHAN_9_ID);
  }
*/

  /* no NvM entry for the Device Type   ... */
/*  
  if( ( NvM_Get_Block_Status(NVM_BLOCK_CHAN_10_ID) & 0x80 ) == 0x00 )
  {
    strcpy(arrNvM_RAM_Mirror_Str_Device, LOG_APP_DEVICE_TYPE);
    NvM_Write_Block(NVM_BLOCK_CHAN_10_ID);
  }
*/
  Logger_App_pre_init_Validate_NvM_String_Data();

}

void init_cyclic_DIAG_Task_5ms(void)
{

}

void init_cyclic_DIAG_Task_10ms(void)
{

}

void init_cyclic_DIAG_Task_1s(void)
{
  /*
   * Alive blink on-board LED ...
   */
  pinMode(LED_BUILTIN, OUTPUT);   
}

void main_cyclic_DIAG_Task_1ms(void)
{
  /* - EEPROM - */
  EEPROM_driver_main(); 
}

void main_cyclic_DIAG_Task_5ms(void)
{

}

void main_cyclic_DIAG_Task_10ms(void)
{
  static unsigned char nDebugService_NvM_Complete = 0;

  /* to be used in 5ms task because 1 EEPROM byte writting takes 3.8 ms at most ... ?!?  -- [ToDo] - check if 10ms is also OK ...  */
  NvM_Main();


  /*
   * Memory blocks updated ... display "Complete" message ...
   */
  if( ( ( NvM_Get_Block_Status(NVM_BLOCK_CHAN_1_ID) & 0x01 ) == 0x00 ) && \
      ( ( NvM_Get_Block_Status(NVM_BLOCK_CHAN_2_ID) & 0x01 ) == 0x00 ) && \
      ( ( NvM_Get_Block_Status(NVM_BLOCK_CHAN_3_ID) & 0x01 ) == 0x00 ) && \
      ( ( NvM_Get_Block_Status(NVM_BLOCK_CHAN_4_ID) & 0x01 ) == 0x00 ) && \
      ( ( NvM_Get_Block_Status(NVM_BLOCK_CHAN_5_ID) & 0x01 ) == 0x00 ) && \
      ( ( NvM_Get_Block_Status(NVM_BLOCK_CHAN_6_ID) & 0x01 ) == 0x00 ) && \
      ( ( NvM_Get_Block_Status(NVM_BLOCK_CHAN_7_ID) & 0x01 ) == 0x00 ) && \
      ( ( NvM_Get_Block_Status(NVM_BLOCK_CHAN_8_ID) & 0x01 ) == 0x00 ) && \
      ( ( NvM_Get_Block_Status(NVM_BLOCK_CHAN_9_ID) & 0x01 ) == 0x00 ) && \
      ( ( NvM_Get_Block_Status(NVM_BLOCK_CHAN_10_ID) & 0x01 ) == 0x00 ) && \      
      ( nDebugService_NvM_Complete == 0 ) \      
      )
    {
      Serial.println("[I] Memory update of NvM blocks 1, 2, 3, 4, 5 -- COMPLETE ! ");
      nDebugService_NvM_Complete ++;
    }
}

void main_cyclic_DIAG_Task_1s(void)
{

	static uint8_t LED_Val = 0;

  /* Alive Heart Beat - 1 sec -  */
	if(LED_Val == 0)
	{
  	digitalWrite(LED_BUILTIN, HIGH);

  	LED_Val = 1;
  }
  else
  {
   	digitalWrite(LED_BUILTIN, LOW);

   	LED_Val = 0;
  }


  /*
   *  Keep board alive !!
   */
  wdt_reset(); // Reset the watchdog
}

/*
 * Factory reset and other diagnostic routines !
 *
 */
T_TaskControlBlock TCB_OS[MAX_TASK_LIST_LEN] = 
{
    {main_cyclic_DIAG_Task_1ms, init_cyclic_DIAG_Task_1ms, 1, 0},
   	{main_cyclic_DIAG_Task_5ms, init_cyclic_DIAG_Task_5ms, 5, 3},
	  {main_cyclic_DIAG_Task_10ms, init_cyclic_DIAG_Task_10ms, 10, 5},
	  {main_cyclic_DIAG_Task_1s, init_cyclic_DIAG_Task_1s, 1000, 999} 
};

#endif /* else (BOARD_DIGNOSTIC_AND_INITIALIZATION_ENABLE == 0) */





