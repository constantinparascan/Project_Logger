/* C.Parascan
 *
 *  This is the configuration file for the Scheduler. 
 *     Here all the cyclic tasks and initialization functions of the drivers and user 
 *     applications are called.
 * 
 *  module version [ see Scheduler.h ] - reviewed and tested on 20.02.2023
 *
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

  //Logger_App_Output_Pins_init();

  //AT_Command_Processor_Init();

}

unsigned short nDebug_1 = 0;

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
  
  /* PARALLEL HW specific
   */
  //Logger_App_Parallel_main();


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

//  unsigned char buff[30];
//  unsigned short nBuffWriteIdx = 0;
//  unsigned char nLen = 0;


//  Logger_App_Output_Pins_main(); <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

//  AT_Command_Processor_Main();

  /* to be used in 5ms task because 1 EEPROM byte writting takes 3.8 ms at most ... ?!?  -- [ToDo] - check if 10ms is also OK ...  */
  NvM_Main();

}


/*
 *  General usage 1s reccurence task - cyclic function
 */
/*
 *  Alive task 
 *     - blink the LED
 *     - [ToDo] serve the Watchdog !
 *   
 *     - other 1 sec callback functions 
 */



unsigned char nDEBUG = 100;

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

//   if(nDEBUG > 0)
//   {
//     nDEBUG --;
//   }
 
//     if(nDEBUG == 0)
//     {
// //      nDEBUG = 10;

//         nNvM_RAM_Mirror_CH1 = 250;
//         nNvM_RAM_Mirror_CH2 = 250;
//         nNvM_RAM_Mirror_CH3 = 250;
//         nNvM_RAM_Mirror_CH4 = 250;        

//         NvM_Write_Block(1);
//         NvM_Write_Block(2);
//         NvM_Write_Block(3);
//         NvM_Write_Block(4);

//         Serial.println(">>>>>>>>>>>>>>>> 250 <<<<<<<<<<<< ");
//       //NvM_Print_Memory_Range(0, 200, 16);
//     }
//   }

//     }
//     else
//     {


  /*
   *  Keep board alive !!
   */
  wdt_reset(); // Reset the watchdog

}




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
    {main_cyclic_Task_1ms, init_cyclic_Task_1ms, 1, 0},   /* HIGHEST prio task */
   	{main_cyclic_Task_5ms, init_cyclic_Task_5ms, 5, 3},
	  {main_cyclic_Task_10ms, init_cyclic_Task_10ms, 10, 5},
	  {main_cyclic_Task_1s, init_Task_1s, 1000, 999}           /* LOWEST prio task */
};




