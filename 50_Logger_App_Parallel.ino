/* C.Parascan
 * 
 *   v0.1 - first implementation 05.06.2023
 *
 */


#include "50_Logger_App_Parallel_cfg.h"
#include "50_Logger_App_Parallel.h"
#include "03_Com_Service.h"

/*
 * Connection with RAM mirror
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


/* 
 * - the following indexes are used by the bill validator channels 
 */
#define CH1_IDX (0)
#define CH2_IDX (1)
#define CH3_IDX (2)
#define CH4_IDX (3)

#define MAX_CHANNELS_IN_USE (4)


/* - keeps the number of LOW pulses detected until an "low" to "high" transition ... */
unsigned char arrChannelLine_PulseCounter_Status[ MAX_CHANNELS_IN_USE ];
/* - keeps the previous status of the channel lines ...                              */
unsigned char arrChannelLine_Old_Status         [ MAX_CHANNELS_IN_USE ];


/* 
 *  - used to indicate the credit on each channel ... 
 */
unsigned short arrChannelCredit[ MAX_CHANNELS_IN_USE ];
unsigned char  arrChannelFlags [ MAX_CHANNELS_IN_USE ];
unsigned char  nChannelBusyFlag_Old_Status;
unsigned char  nLastBillType_Parallel = 0;

/*
 * used to indicate that an NvM write opperation is to be performed when NvM block is free
 */
unsigned char nNvM_Update_Pending_Flags[ MAX_CHANNELS_IN_USE ];



unsigned short nStartupDelayForLineStabilization = STARTUP_DELAY_FOR_STABILIZATION;


void Logger_App_Parallel_Callback_Command_Reset_Monetary(void)
{

  arrChannelCredit[0] = 0;
  Logger_App_Parallel_Update_NvM_Block( 0, 0 );  

  arrChannelCredit[1] = 0;
  Logger_App_Parallel_Update_NvM_Block( 1, 0 );  

  arrChannelCredit[2] = 0;
  Logger_App_Parallel_Update_NvM_Block( 2, 0 );  

  arrChannelCredit[3] = 0;
  Logger_App_Parallel_Update_NvM_Block( 3, 0 );  

  nLastBillType_Parallel = 0;

}



/*
 * - v0.1 
 *
 *  - initialize boards pins and internal variables in preparation 
 *        for parallel opperation.
 */
void Logger_App_Parallel_init (void)
{
    unsigned char nIdx = 0;
    
    /*
     * configure PIN's as input ... prepare for sniffing ... 
     */

    pinMode(VENDOR_CH1,    INPUT);   /* pin 1  on bill validator socket */
    pinMode(VENDOR_CH2,    INPUT);   /* pin 2  on bill validator socket */
    pinMode(VENDOR_CH3,    INPUT);   /* pin 3  on bill validator socket */
    pinMode(VENDOR_CH4,    INPUT);   /* pin 4  on bill validator socket */
    
    pinMode(VENDOR_BUSY,   INPUT);   /* pin 9  on bill validator socket */
    pinMode(VENDOR_ESCROW, INPUT);   /* pin 10 on bill validator socket */
    

    /* 
     * init flags and internal variables
     */
    for (nIdx = 0; nIdx < MAX_CHANNELS_IN_USE; nIdx ++)
    {
        arrChannelLine_PulseCounter_Status[ nIdx ] = 0;  /* no pulses detected yet ... */
        arrChannelLine_Old_Status[ nIdx ] = 1;           /* line "high" previously ... */

        arrChannelCredit[ nIdx ] = 0;              /* !!! [ToDo] - values to be recovered from EEPROM !!!  ???? */

        arrChannelFlags[ nIdx ] = 0;               

        nNvM_Update_Pending_Flags[ nIdx ] = 0;
    }
    

    /*
     * Fill in with NvM Read_All recovered content ... 
     */
    arrChannelCredit[ 0 ] = nNvM_RAM_Mirror_CH1;
    arrChannelCredit[ 1 ] = nNvM_RAM_Mirror_CH2;
    arrChannelCredit[ 2 ] = nNvM_RAM_Mirror_CH3;
    arrChannelCredit[ 3 ] = nNvM_RAM_Mirror_CH4;

    /* not busy ... active low ... contains the value of the pin "BUSY" */
    nChannelBusyFlag_Old_Status = 1;

  #if (LOGGER_APP_PARALLEL_DEBUG_ENABLE >= 1)
  Serial.begin(9600);
  Serial.println("[I] Logger Parallel - init");

  Serial.print("CH Values-> CH1: ");
  Serial.print(arrChannelCredit[ 0 ] );
  Serial.print(" CH2: ");
  Serial.print(arrChannelCredit[ 1 ] );
  Serial.print(" CH3: ");
  Serial.print(arrChannelCredit[ 2 ] );
  Serial.print(" CH4: ");
  Serial.print(arrChannelCredit[ 3 ] );

  Serial.println();

  #endif
    
}/* end Logger_App_Parallel_init */




/*
 *  - v0.1
 *
 *    - API used to read-out the content of the channel pins ... 
 */
unsigned char Logger_App_Parallel_DigitalRead_Chan(unsigned char nChanNr)
{
    /* iddle value is "1"       */
    unsigned char nPinVal = 1;

    switch(nChanNr)
    {
        case CH1_IDX:
            nPinVal = digitalRead(VENDOR_CH1);
        break;

        case CH2_IDX:
            nPinVal = digitalRead(VENDOR_CH2);
        break;

        case CH3_IDX:
            nPinVal = digitalRead(VENDOR_CH3);
        break;

        case CH4_IDX:
            nPinVal = digitalRead(VENDOR_CH4);
        break;

        default:
            nPinVal = 1;
        break;
    }

    return nPinVal;

}/* Logger_App_Parallel_DigitalRead_Chan */


unsigned char Logger_App_Parallel_HasNewEvents(void)
{
  unsigned char nReturn = 0;
  unsigned char nIdx = 0;

    /* 
     * look for new events
     */
    for (nIdx = 0; nIdx < MAX_CHANNELS_IN_USE; nIdx ++)
    {

      if( arrChannelFlags[ nIdx ] > 0 )
      {
        nReturn ++;

        break;
      }
    }


  return nReturn;
}

void Logger_App_Parallel_ResetEventsStatus(void)
{
  unsigned char nIdx = 0;

    /* 
     * reset events status to "0"
     */
    for (nIdx = 0; nIdx < MAX_CHANNELS_IN_USE; nIdx ++)
    {

      arrChannelFlags[ nIdx ] = 0;
      
    }
}

unsigned short Logger_App_Parallel_GetChannelValue(unsigned char nValue)
{
  unsigned short nReturn = 0;

  if( nValue < MAX_CHANNELS_IN_USE )
  {
    nReturn = arrChannelCredit[ nValue ];
  }

  return nReturn;
}

extern unsigned char arrNvM_RAM_Mirror_Str_SimNR   [ ];
extern unsigned char arrNvM_RAM_Mirror_Str_Town    [ ];
extern unsigned char arrNvM_RAM_Mirror_Str_Place   [ ];
extern unsigned char arrNvM_RAM_Mirror_Str_Details [ ];
extern unsigned char arrNvM_RAM_Mirror_Str_Device  [ ];

unsigned char* Logger_App_Parallel_GetDataPointer(unsigned char nValue)
{
  unsigned char* nReturn = 0;

  switch(nValue)
  {
    case 0:
      nReturn = &arrNvM_RAM_Mirror_Str_SimNR[0];
      break;

    case 1:
      nReturn = &arrNvM_RAM_Mirror_Str_Town[0];
      break;

    case 2:
      nReturn = &arrNvM_RAM_Mirror_Str_Place[0];
      break;

    case 3:
      nReturn = &arrNvM_RAM_Mirror_Str_Details[0];
      break;

    case 4:
      nReturn = &arrNvM_RAM_Mirror_Str_Device[0];
      break;
            
  }

  return nReturn;
}

unsigned char Logger_App_Parallel_GetLastBillType(void)
{
  return nLastBillType_Parallel;
}


void Logger_App_Parallel_Update_NvM_Block(unsigned char nParallelChannel, unsigned short nDataValue)
{
  switch(nParallelChannel)
  {
    case 0:
      if( NvM_Is_Block_Opperation_Free( 1 ) )
      {
        nNvM_RAM_Mirror_CH1 = nDataValue;
        NvM_Write_Block(1);

        nNvM_Update_Pending_Flags[ 0 ] = 0x00;
      }
      else
      {
        nNvM_Update_Pending_Flags[ 0 ] = 0x01;
      }
      break;

    case 1:
      if( NvM_Is_Block_Opperation_Free( 2 ) )
      {
        nNvM_RAM_Mirror_CH2 = nDataValue;
        NvM_Write_Block(2);

        nNvM_Update_Pending_Flags[ 1 ] = 0x00;
      }
      else
      {
        nNvM_Update_Pending_Flags[ 1 ] = 0x01;
      }
      break;

    case 2:
      if( NvM_Is_Block_Opperation_Free( 3 ) )
      {
        nNvM_RAM_Mirror_CH3 = nDataValue;
        NvM_Write_Block(3);

        nNvM_Update_Pending_Flags[ 2 ] = 0x00;
      }
      else
      {
        nNvM_Update_Pending_Flags[ 2 ] = 0x01;
      }

      break;

    case 3:
      if( NvM_Is_Block_Opperation_Free( 4 ) )
      {
        nNvM_RAM_Mirror_CH4 = nDataValue;
        NvM_Write_Block(4);

        nNvM_Update_Pending_Flags[ 3 ] = 0x00;
      }
      else
      {
        nNvM_Update_Pending_Flags[ 3 ] = 0x01;
      }
      break;


    default:
      break;
  }
}

/* 
 *  - v0.1 
 *    
 *   - Main function of the Parallel protocol
 *
 */
void Logger_App_Parallel_main (void)
{
  /* check each channel for credit ... */
  unsigned char nBusyStatus = 1;
  unsigned char nChanStatus = 1;

  unsigned char nIdx;

  if( nStartupDelayForLineStabilization > 0 )
  {
    nStartupDelayForLineStabilization --;
  }
  else
  {

    /* Read-out the "busy" line ... 
    *
    *    - all bill detection takes place when this line is "LOW" ...
    */
    nBusyStatus = digitalRead(VENDOR_BUSY);
      
    #if ( LOGGER_APP_PARALLEL_DEBUG_ENABLE >= 3 )
      Serial.print("[I] - Busy:");
      Serial.println(nBusyStatus);
    #endif


    if(nBusyStatus == 0)
    {
      /*
       * Loop each channel ... 
       */

      for(nIdx = 0; nIdx < MAX_CHANNELS_IN_USE; nIdx ++)
      {

        /* ************************ *
         *  ---  Channel xxx  ---
         * ************************ */
            
        nChanStatus = Logger_App_Parallel_DigitalRead_Chan( nIdx );

        #if ( LOGGER_APP_PARALLEL_DEBUG_ENABLE == 2 )
          Serial.print("[I] - CH");
          Serial.print(nIdx);
          Serial.print(" = ");
          Serial.println(nChanStatus);
        #endif


        /* we might have a pulse ...  */
        if(nChanStatus == 0)
        {
          /* another itteration found the channel lino on "LOW" ... */
          arrChannelLine_PulseCounter_Status[ nIdx ] ++;
        }


        /* consume current line status event .... */
        //arrChannelLine_Old_Status[ nIdx ] = nChanStatus;
            
        /* 
                    END 
            ... CHANNEL xxx processing ... 
        */
           
      }/* end for ... */
                             
    }
    else
    {
      /* this is a transition from "low" to "high" of Busy signal ... */
      if(nChannelBusyFlag_Old_Status == 0)
      {

        for (nIdx = 0; nIdx < MAX_CHANNELS_IN_USE; nIdx ++)
        {

          if( arrChannelLine_PulseCounter_Status[ nIdx ] >= CHANNEL_PULSE_MIN_LOW_COUNT )
          {
            /* valid bill has been detected recorded by the bill validator .... */
            arrChannelCredit[ nIdx ] ++;
                        

            Logger_App_Parallel_Update_NvM_Block( nIdx, arrChannelCredit[ nIdx ] );

            /* signal new event ... */
            arrChannelFlags[ nIdx ] |= 0x01;

            nLastBillType_Parallel = nIdx + 1;


            /*
             * - trigger a Com transmission of BILL channels !
             */
            AT_Command_Callback_Request_Bill_Status_Transmission();

            #if ( LOGGER_APP_PARALLEL_DEBUG_ENABLE == 1 )
              Serial.print("[I] - Bill detected - CH:");
              Serial.println(nIdx);

              Serial.print("[I] - CH1: ");
              Serial.print(arrChannelCredit[0]);

              Serial.print("   CH2: ");
              Serial.print(arrChannelCredit[1]);

              Serial.print("   CH3: ");
              Serial.print(arrChannelCredit[2]);

              Serial.print("   CH4: ");
              Serial.println(arrChannelCredit[3]);
            #endif


            // arrChannelLine_PulseCounter_Status[ nIdx ] = 0;  /* no pulses detected yet ... */
            // arrChannelLine_Old_Status[ nIdx ] = 1;           /* line "high" previously ... */

          }

          arrChannelLine_PulseCounter_Status[ nIdx ] = 0;  /* no pulses detected yet ... */
          //arrChannelLine_Old_Status[ nIdx ] = 1;           /* line "high" previously ... */
          
        }/* end for */

      }/* end if(nChannelBusyFlag_Old_Status == 0) */

        
    }/* end else if(nBusyStatus == 0) */
    
    /* consum the "Busy" status signal ... */
    nChannelBusyFlag_Old_Status = nBusyStatus;

  }/* end else ( nStartupDelayForLineStabilization > 0 ) */

}/* end Logger_App_Parallel_main */




/* 
 *  - v0.1 
 *    
 *   - Main function of the Parallel protocol
 *
 */
void Logger_App_Parallel_main_v2 (void)
{
  /* check each channel for credit ... */
  unsigned char nBusyStatus = 1;
  unsigned char nChanStatus = 1;

  unsigned char nIdx;

  if( nStartupDelayForLineStabilization > 0 )
  {
    nStartupDelayForLineStabilization --;
  }
  else
  {

    /*
     * Check to see if any NvM block write requests are still pending
     */
    for(nIdx = 0; nIdx < MAX_CHANNELS_IN_USE; nIdx ++)
    {
      if( nNvM_Update_Pending_Flags[ nIdx ] > 0 )
      {
        Logger_App_Parallel_Update_NvM_Block( nIdx, arrChannelCredit[ nIdx ] );
      }
    }

    /*
     * Loop each channel ... to detect possible credit !!!
     */

    for(nIdx = 0; nIdx < MAX_CHANNELS_IN_USE; nIdx ++)
    {

      /* ************************ *
       *  ---  Channel xxx  ---
       * ************************ */
            
        nChanStatus = Logger_App_Parallel_DigitalRead_Chan( nIdx );


        /* we might have a pulse ...  */
        if( nChanStatus == 0 )
        {
          /* another itteration found the channel lino on "LOW" ... */
          arrChannelLine_PulseCounter_Status[ nIdx ] ++;

        }
        else
        {
          /*
           * A transition from Low to high of channel ... 
           */
          if(arrChannelLine_Old_Status[ nIdx ] == 0 )
          {
              
            if( arrChannelLine_PulseCounter_Status[ nIdx ] >= CHANNEL_PULSE_MIN_LOW_COUNT )
            {
              /* valid bill has been detected recorded by the bill validator .... */
              arrChannelCredit[ nIdx ] ++;
                          

              Logger_App_Parallel_Update_NvM_Block( nIdx, arrChannelCredit[ nIdx ] );

              /* signal new event ... */
              arrChannelFlags[ nIdx ] |= 0x01;

              nLastBillType_Parallel = nIdx + 1;


              /*
              * - trigger a Com transmission of BILL channels !
              */
              AT_Command_Callback_Request_Bill_Status_Transmission();

              #if ( LOGGER_APP_PARALLEL_DEBUG_ENABLE == 1 )
                Serial.print("[I] - Bill detected - CH:");
                Serial.println(nIdx);

                Serial.print("[I] - CH1: ");
                Serial.print(arrChannelCredit[0]);

                Serial.print("   CH2: ");
                Serial.print(arrChannelCredit[1]);

                Serial.print("   CH3: ");
                Serial.print(arrChannelCredit[2]);

                Serial.print("   CH4: ");
                Serial.println(arrChannelCredit[3]);
              #endif

            }

            arrChannelLine_PulseCounter_Status[ nIdx ] = 0;  /* no pulses detected yet ... */
            arrChannelLine_Old_Status[ nIdx ] = 1;           /* line "high" previously ... */

          }/* if(arrChannelLine_Old_Status[ nIdx ] == 0 ) */

        }


        /* consume current line status event .... */
        arrChannelLine_Old_Status[ nIdx ] = nChanStatus;
            
        /* 
                    END 
            ... CHANNEL xxx processing ... 
        */
           
    }/* end for ... */
                             

  }/* end else ( nStartupDelayForLineStabilization > 0 ) */

}/* end Logger_App_Parallel_main */



