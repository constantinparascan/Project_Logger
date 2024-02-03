/* NvM driver
 *   C.Parascan - v1.1
 *
 *     - driver created in order to manage EEPROM block access 
 *     - timing optimization by using local EEPROM driver 
 */

#if(NVM_MANAGER_DEBUG_SERIAL == 1)
#include <EEPROM.h>
#endif

#include "01_Debug_Utils.h"
#include "09_NvM_Manager.h"
#include "09_NvM_Manager_cfg.h"
#include "09_EEPROM_Driver.h"


#define NVM_VALID_BLOCK_PATTERN (0xA5)


/*
 *  Configuration of the NvM blocks array !
 */

typedef struct NvM_Block_Header
{
  unsigned char nLen;
  char *DataPtr;
  int AddressStart;
  int AddressLastEntry;
  unsigned char nDataMultiply;
  unsigned char nFlags;       /* - - - -   - - - 1   <- write request */
                              /* - - - -   - - 1 -   <- read  request */
                              /* - - - -   1 - - -   <- address synchronized with last valid entry */
                              /* - 1 - -   - - - -   <- last read-all opperation succedded         */
                              /* 1 - - -   - - - -   <- block has been written at least once       */

}T_NvM_Block_Header;




typedef enum eNvM_State
{
  NVM_STATE_NONINIT = 0,
  NVM_STATE_INIT,

  NVM_STATE_WAITING,

  NVM_STATE_WRITE_COMMAND_DATA,
  NVM_STATE_WRITE_COMMAND_DATA_WAIT_COMPLETION,
  NVM_STATE_WRITE_COMMAND_VALID_STAMP,
  NVM_STATE_WRITE_COMMAND_VALID_STAMP_WAIT_COMPLETION,
  NVM_STATE_WRITE_COMMAND_OLD_VALID_STAMP_CLEAR,
  NVM_STATE_WRITE_COMMAND_OLD_VALID_STAMP_CLEAR_WAIT_COMPLETION,

  NVM_STATE_READ_COMMAND_VALIDATE_BLOCK,
  NVM_STATE_READ_COMMAND_VALIDATE_BLOCK_WAIT_COMPLETION,
  NVM_STATE_READ_COMMAND_DATA,
  NVM_STATE_READ_COMMAND_DATA_WAIT_COMPLETION
}T_NVM_State;


/* 
 * NvM Internals 
 */
T_NVM_State NvM_State = NVM_STATE_NONINIT;

unsigned char nNvM_WorkingBlockID = 0;

unsigned char nNvM_DataBytesToBeWritten = 0;
unsigned int  nNvM_Write_EEPROM_Address = 0;
unsigned int  nNvM_Write_EEPROM_New_Block_Start_Address = 0;

unsigned char nNvM_DataBytesToBeReadout = 0;
unsigned int  nNvM_Read_EEPROM_Address = 0;

unsigned char *ptrNvM_RAM_Mirror = 0;

/*
 * Local buffers where data can be stored until is fully 
 *   written to EEPROM !
 */

unsigned short int nLocalBuff_DataBlock;

extern T_NvM_Block_Header arrNvM_BlockDef[];


#if(NVM_MANAGER_DEBUG_SERIAL == 1)
char NvM_DebugPrintBuff[30];
#endif

/*
 * All NvM blocks are configured as follows:
 *
 *      [data 0] [data 1] ... [data Len] [pattern 5A] [data 0] [data 1] ... [data Len] [pattern 5A] ...
 *      |                                           | |                                           |
 *      \      first multiplicity entry             / \      second multiplicity entry            / ....
 *
 */

void NvM_Init(void)
{
  unsigned char nIdx = 0;
  unsigned char nSearchIdx = 0;
  unsigned char nSearchFinished = 0;
  unsigned int  nWorkingAddress = 0;
  unsigned char nEEPROM_Data;

  #if(NVM_MANAGER_DEBUG_SERIAL == 1)

  Serial.println("[I] NVM Debug Startup ...");

  unsigned long int nTestData = 0xAABBCCDD;

  Serial.print("size ");
  Serial.println(sizeof(nTestData));
  Serial.print("addr = ");
  Serial.println((unsigned long int)&nTestData, HEX);

  Serial.print("Data n:");
  Serial.print((unsigned char)(*(unsigned char *)&nTestData), HEX);
  Serial.println();

  Serial.print("Data n+1:");
  Serial.print((unsigned char)(*((unsigned char *)&nTestData + 1)), HEX);
  Serial.println();
  
  Serial.print("Data n+2:");
  Serial.print((unsigned char)(*((unsigned char *)&nTestData + 2)), HEX);
  Serial.println();

  Serial.print("Data n+3:");
  Serial.print((unsigned char)(*((unsigned char *)&nTestData + 3)), HEX);
  Serial.println();
 
  #endif


  /* !! this is a time costing operation !!                          */
  /* it makes sure that all blocks have the Address synchronized !!  */
  for (nIdx = 0; nIdx < NVM_BLOCK_COUNT; nIdx ++)
  {
    /* has the adress been synchronized ? */
    if( arrNvM_BlockDef[nIdx].nFlags & NVM_FLAG_ADDR_SYNC )
    {
      continue;
    }
    else
    {
      nSearchFinished = 0;


      #if(NVM_MANAGER_DEBUG_SERIAL == 1)
        Serial.println("[I] NVM ---------------------------");
      #endif


      /* we need to search for last valid entry if any ? */
      for(nSearchIdx = 0; nSearchIdx < arrNvM_BlockDef[nIdx].nDataMultiply; nSearchIdx ++)
      {
        nWorkingAddress = arrNvM_BlockDef[nIdx].AddressStart + (arrNvM_BlockDef[nIdx].nLen + 1)*nSearchIdx + arrNvM_BlockDef[nIdx].nLen;

        /* try to read a valid pattern ... if any */
        /*
         *  !!! EEPROM  !!! access is a timely cost opperation --- 3ms per access
         *
         *       - the EEPROM_driver_Forced_Read_NoWait does not fait for 
         *           previous EEPROM opperation to FINISH !!! 
         *
         *                  - make sure this is placed at the beginning 
         *                    of the SW so no writes are preceding ...
         */          
        //nEEPROM_Data = EEPROM.read(nWorkingAddress);
        nEEPROM_Data = EEPROM_driver_Forced_Read_NoWait(nWorkingAddress);     
        
         #if(NVM_MANAGER_DEBUG_SERIAL == 1)
            Serial.println();
            Serial.print("[I] NVM Info - pattern readout: ");            
            Serial.print(nEEPROM_Data, HEX);
          #endif /* NVM_MANAGER_DEBUG_SERIAL */
           

        if(nEEPROM_Data == NVM_VALID_BLOCK_PATTERN)
        {
          nSearchFinished = 1;

          arrNvM_BlockDef[nIdx].AddressLastEntry = arrNvM_BlockDef[nIdx].AddressStart + (arrNvM_BlockDef[nIdx].nLen + 1)*nSearchIdx;
            
          arrNvM_BlockDef[nIdx].nFlags |= ( NVM_FLAG_ADDR_SYNC | NVM_FLAG_BLOCK_DATA_EXIST );

          #if(NVM_MANAGER_DEBUG_SERIAL == 1)
            Serial.println();
            Serial.print("[I] NVM Info - Block on Init VALID: ID ");
            snprintf(NvM_DebugPrintBuff, 30, "%d  EEPROM_Addr = %d, ", nIdx, arrNvM_BlockDef[nIdx].AddressLastEntry, arrNvM_BlockDef[nIdx].DataPtr );
            Serial.println(NvM_DebugPrintBuff);
          #endif /* NVM_MANAGER_DEBUG_SERIAL */

          /* first valid block shall be taken as last one !! */
          break;          
        }         

      }/* end for */

      if(nSearchFinished == 0)
      {
        /* there is no valid entry in the NvM ... yet ... reset all flags ... and keep Address sync */

        arrNvM_BlockDef[nIdx].nFlags = NVM_FLAG_ADDR_SYNC;

        #if(NVM_MANAGER_DEBUG_SERIAL == 1)
          Serial.println();
          Serial.print("[I] NVM Info - Block on Init NULL: ID ");
          snprintf(NvM_DebugPrintBuff, 30, "%d EEPROM_Addr= %d, %X", nIdx, arrNvM_BlockDef[nIdx].AddressLastEntry, arrNvM_BlockDef[nIdx].DataPtr );
          Serial.println(NvM_DebugPrintBuff);
        #endif /* NVM_MANAGER_DEBUG_SERIAL */

      }

    } /* else ... if( arrNvM_BlockDef[nIdx].nFlags & NVM_FLAG_ADDR_SYNC ) */

  }/* end for */

  /* -------------------- all NVM blocks are now Adress synchronized ----------------------- */

  NvM_State = NVM_STATE_INIT;
  
}/* void NvM_Init(void) */

/* v0.1
 *    - read all data from EEPROM and store result in associated RAM image 
 *      loactions if exists
 *
 *    - !! WARNING !!   --- HUGE time consuming procedure due to EEPROM access time !
 *                          Advice --> to be used during start-up procedure BEFORE scheduler is started !
 *                                     
 *
 */
void NvM_ReadAll(void)
{
  unsigned char nIdx = 0;
  unsigned char nDataIdxCopy = 0;
  unsigned char *ptrDataDest = 0;
  unsigned int  nAddr_EEPROM_Read = 0;
  unsigned char nData_EEPROM_Read = 0;

  for (nIdx = 0; nIdx < NVM_BLOCK_COUNT; nIdx ++)
  {
    /* is there a RAM image attached to this block ? */
    if( (arrNvM_BlockDef[nIdx].DataPtr != NULL) && (arrNvM_BlockDef[nIdx].nFlags & NVM_FLAG_ADDR_SYNC) && (arrNvM_BlockDef[nIdx].nFlags & NVM_FLAG_BLOCK_DATA_EXIST) )
    {
      nAddr_EEPROM_Read = arrNvM_BlockDef[nIdx].AddressLastEntry;
      ptrDataDest = (unsigned char *)arrNvM_BlockDef[nIdx].DataPtr;

      if(ptrDataDest != NULL)
      {

        for (nDataIdxCopy = 0; nDataIdxCopy < arrNvM_BlockDef[nIdx].nLen; nDataIdxCopy++)
        {

        /*
         *  !!! EEPROM  !!! access is a timely cost opperation --- 3ms per access
         *
         *       - the EEPROM_driver_Forced_Read_NoWait does not fait for 
         *           previous EEPROM opperation to FINISH !!! 
         *
         *                  - make sure this is placed at the beginning 
         *                    of the SW so no writes are preceding ...
         */          
          //nData_EEPROM_Read = EEPROM.read(nAddr_EEPROM_Read);
          nData_EEPROM_Read = EEPROM_driver_Forced_Read_NoWait(nAddr_EEPROM_Read);

          *ptrDataDest = nData_EEPROM_Read;

          nAddr_EEPROM_Read ++;
          ptrDataDest ++;  /* addresses goes from high to low ... */
        }

        arrNvM_BlockDef[nIdx].nFlags |= NVM_FLAG_LAST_READ_ALL_OK;

        #if(NVM_MANAGER_DEBUG_SERIAL == 1)
          Serial.println();
          Serial.print("[I] NVM Info - Block Readed: ID");
          snprintf(NvM_DebugPrintBuff, 30, " %d EEPROM_Addr: %d ", nIdx, arrNvM_BlockDef[nIdx].AddressLastEntry);
          Serial.println(NvM_DebugPrintBuff);
        #endif /* NVM_MANAGER_DEBUG_SERIAL */

      } /* end if(ptrDataDest != NULL) */
      else
      {
        #if(NVM_MANAGER_DEBUG_SERIAL == 1)
          Serial.println();
          Serial.print("[I] NVM Info - Block Read NOK - NULL dataPtr: ID");
          snprintf(NvM_DebugPrintBuff, 30, " %d ", nIdx);
          Serial.println(NvM_DebugPrintBuff);
        #endif /* NVM_MANAGER_DEBUG_SERIAL */

      }

    }/* end if( (arrNvM_BlockDef[nIdx].DataPtr != NULL) &&  .... */
    
  }/* end for */

}/* end NvM_ReadAll fcn */


/* NvM manager
 *   - version 1.0
 *   - author: C. Parascan
 *
 *   - Main function used to drive the state-machine of the
 *     NvM manager. 
 */
void NvM_Main(void)
{
  unsigned char nIdx = 0;
  unsigned char nEEPROM_Read_Val = 0;
  unsigned char nTemp = 0;

  switch (NvM_State)
  {
    case NVM_STATE_NONINIT:
    {
      /* look for valid NVM header ... */

      /* if NVM is empty mark bloks ! ... skip search ! */
      NvM_State = NVM_STATE_INIT;

    }
    //break;
    
    case NVM_STATE_INIT:
    {

      NvM_State = NVM_STATE_WAITING;

    }
    //break;

    /*
     * search for a block write / read request ... 
     *  
     *  Block priority is given by order inside "arrNvM_BlockDef"
     *                 - lower indexes --> higher priority !     
     */
    case NVM_STATE_WAITING:
    {

      /*
       * -------------------------------------------------------
       *                  Process writtings req 
       * -------------------------------------------------------
       */


      /* Write opperations take precedence over read !     
       *
       */
      for (nIdx = 0; nIdx < NVM_BLOCK_COUNT; nIdx ++)
      {
        
        /* do we have a write request ? */
        if( arrNvM_BlockDef[nIdx].nFlags & NVM_FLAG_WRITE_REQ )
        {
          nNvM_WorkingBlockID = nIdx;

          /* decide where to write the next data entry depending on the multiplicity of the block !! */

          /* 
           *   if we have a block with one level of multiplicity ... 
           */
          if( arrNvM_BlockDef[nIdx].nDataMultiply <= 1 )
          {
            nNvM_Write_EEPROM_Address = arrNvM_BlockDef[nIdx].AddressStart;

            nNvM_Write_EEPROM_New_Block_Start_Address = arrNvM_BlockDef[nIdx].AddressStart;
          }
          /* 
           *   ... multiplicity > 1 means we have to advance to the last entry and check if we can write after or we should start from the beginning
           *                    ... a closed loop list !!
           */
          else
          {
            /* 
             *  if at least one data entry exists ... then we should start writting "after" current block entry ...
             */
            if(arrNvM_BlockDef[nIdx].nFlags & NVM_FLAG_BLOCK_DATA_EXIST)
            {
              if( (arrNvM_BlockDef[nIdx].AddressLastEntry + arrNvM_BlockDef[nIdx].nLen + 1) < \
                  ( arrNvM_BlockDef[nIdx].AddressStart + (arrNvM_BlockDef[nIdx].nLen + 1) * arrNvM_BlockDef[nIdx].nDataMultiply ) )
              {
                nNvM_Write_EEPROM_Address = arrNvM_BlockDef[nIdx].AddressLastEntry + arrNvM_BlockDef[nIdx].nLen + 1;
              }
              else
              {
                nNvM_Write_EEPROM_Address = arrNvM_BlockDef[nIdx].AddressStart;
              }
            }
            /*
             * no previous entry exists ... start from first index ...
             */
            else
            {
              nNvM_Write_EEPROM_Address = arrNvM_BlockDef[nIdx].AddressStart;
            }

          }/* end else several multiplicity blocks .... */

          nNvM_Write_EEPROM_New_Block_Start_Address = nNvM_Write_EEPROM_Address;

          nNvM_DataBytesToBeWritten = arrNvM_BlockDef[nIdx].nLen;
          ptrNvM_RAM_Mirror = arrNvM_BlockDef[nIdx].DataPtr;

          NvM_State = NVM_STATE_WRITE_COMMAND_DATA;

          #if(NVM_MANAGER_DEBUG_SERIAL == 1)
            Serial.println();
            Serial.print("[I] NVM Info - WRITE -prep: ID");
            snprintf(NvM_DebugPrintBuff, 30, " %d on EEPROM_Addr %d ", nIdx, nNvM_Write_EEPROM_Address);
            Serial.println(NvM_DebugPrintBuff);
          #endif /* NVM_MANAGER_DEBUG_SERIAL */


          /* EXIT for loop forced ! */
          break;

        }/* end if ( arrNvM_BlockDef[nIdx].nFlags & NVM_FLAG_WRITE_REQ ) */

      }/* end for ... Write with priority */

      /*
       * -------------------------------------------------------
       *                  Process readings req
       * -------------------------------------------------------
       */


      /* No write request processed ... look for reading requests ... */
      if(NvM_State == NVM_STATE_WAITING)
      {
        /* search for a read request !!! */
        for (nIdx = 0; nIdx < NVM_BLOCK_COUNT; nIdx ++)
        {
          /* do we have a read request ? */
          if( (arrNvM_BlockDef[nIdx].nFlags & NVM_FLAG_READ_REQ) && (arrNvM_BlockDef[nIdx].DataPtr != NULL))
          {
            nNvM_WorkingBlockID = nIdx;


            nNvM_DataBytesToBeReadout = arrNvM_BlockDef[nIdx].nLen;
            ptrNvM_RAM_Mirror = arrNvM_BlockDef[nIdx].DataPtr;

            #if(NVM_MANAGER_DEBUG_SERIAL == 1)
              Serial.println();
              Serial.print("[I] NVM Info - READ -prep: ID");
              snprintf(NvM_DebugPrintBuff, 30, " %d on EEPROM_Addr %d ", nIdx, arrNvM_BlockDef[nIdx].AddressLastEntry);
              Serial.println(NvM_DebugPrintBuff);
            #endif /* NVM_MANAGER_DEBUG_SERIAL */

            NvM_State = NVM_STATE_READ_COMMAND_VALIDATE_BLOCK;

            /* EXIT for loop forced ! */
            break;
          }

        }/* end for */

      }/* end if if(NvM_State == NVM_STATE_WAITING) */
      
    } 
    break;

    /****************************************
     *
     *        Writing procedure .....
     *
     ****************************************/



    /* write a limited number of bytes 
     *  on each itteration in order to make sure there will be no delays introduced in the scheduler task itterrations !!
     *  
     *   1 byte - erase + write according to uC ATmega 2560 - takes 3.4 ms !
     */
    case NVM_STATE_WRITE_COMMAND_DATA:
    {
      if(ptrNvM_RAM_Mirror != NULL)
      {
        /*
         * Has the write request been accepted by the 
         */
        //if( EEPROM_driver_Write_Req(nNvM_Write_EEPROM_Address, (unsigned char)*(ptrNvM_RAM_Mirror - nNvM_DataBytesToBeWritten + 1), nNvM_DataBytesToBeWritten) )
        if( EEPROM_driver_Write_Req(nNvM_Write_EEPROM_Address, (unsigned char*)ptrNvM_RAM_Mirror, nNvM_DataBytesToBeWritten) )
        {
            /* advance address writting pointer */
            nNvM_Write_EEPROM_Address += nNvM_DataBytesToBeWritten;

            #if(NVM_MANAGER_DEBUG_SERIAL == 1)
              Serial.println();
              Serial.print("[I] NVM Request EEPROM to Write: ");
              for(nIdx = 0; nIdx < nNvM_DataBytesToBeWritten; nIdx ++)
              {
                snprintf(NvM_DebugPrintBuff, 30, " %d ", ptrNvM_RAM_Mirror[nIdx]);
              }
              Serial.println(NvM_DebugPrintBuff);
            #endif /* NVM_MANAGER_DEBUG_SERIAL */
          

          /* advance state ... wait block write completion ... */ 
          NvM_State = NVM_STATE_WRITE_COMMAND_DATA_WAIT_COMPLETION;
        }
        else
        {
          /*
           * [ToDo] .... how many retries ??? 
           */
        }

      }/* end if(ptrNvM_RAM_Mirror != NULL) ... */
      else
      {

        /* NULL RAM image ... cannot write valid data !!! ... just skip ... */     
        NvM_State = NVM_STATE_WAITING;   

        /* clear write request ... */
        arrNvM_BlockDef[nNvM_WorkingBlockID].nFlags &= ~NVM_FLAG_WRITE_REQ;
      }

      // if(nNvM_DataBytesToBeWritten <= 0)
      // {
      //   /* we have finished writting all the data !! ... prepare the validation procedure ... */

      //   NvM_State = NVM_STATE_WRITE_COMMAND_VALID_STAMP;
      // }      

    }
    break;

    case NVM_STATE_WRITE_COMMAND_DATA_WAIT_COMPLETION:
    {
      if( EEPROM_driver_Is_Write_Finished() )
      {
        NvM_State = NVM_STATE_WRITE_COMMAND_VALID_STAMP;
      }
      else
      {
        /*
         * [ToDo] ... how many retries ??? 
         */
      }
    }
    break;
    
    case NVM_STATE_WRITE_COMMAND_VALID_STAMP:
    {
      /*
       * if we have a 1 multiplicity level block already written once ... just update the data ... is enough to finish the writting procedure ...
       */
      if( (arrNvM_BlockDef[nNvM_WorkingBlockID].nDataMultiply <= 1) && (arrNvM_BlockDef[nNvM_WorkingBlockID].nFlags & NVM_FLAG_BLOCK_DATA_EXIST) )
      {
        arrNvM_BlockDef[nNvM_WorkingBlockID].AddressLastEntry = arrNvM_BlockDef[nNvM_WorkingBlockID].AddressStart;

        /* clear write request ... */
        arrNvM_BlockDef[nNvM_WorkingBlockID].nFlags &= ~NVM_FLAG_WRITE_REQ;

        NvM_State = NVM_STATE_WAITING;
      }
      /*
       * - in all the other cases: - the block have 1 multiplicity, and has never been written before
       *                           - the block have > 1 multiplicity, always write the valid flag because we move the block to a new position ...
       */    
      else
      {


        /*
         *  !!! EEPROM  !!! access is a timely cost opperation --- 3ms per access
         *
         *   and all internal lib functions start with --> do {} while (!eeprom_is_ready ());   
         *    [ToDo] ... check optimization ....
         */
        //EEPROM.write(nNvM_Write_EEPROM_Address, NVM_VALID_BLOCK_PATTERN );
        //EEPROM_driver_Write_Byte_Fast(nNvM_Write_EEPROM_Address, NVM_VALID_BLOCK_PATTERN )
        nTemp = NVM_VALID_BLOCK_PATTERN;
        
        if( EEPROM_driver_Write_Req(nNvM_Write_EEPROM_Address, &nTemp, 1) == 1 )
        {

          /* 
          * If the block has been written in the past ... then old valid flags needs to be erased ...
          */
          if(arrNvM_BlockDef[nNvM_WorkingBlockID].nFlags & NVM_FLAG_BLOCK_DATA_EXIST)
          {
            NvM_State = NVM_STATE_WRITE_COMMAND_VALID_STAMP_WAIT_COMPLETION;
          }
          else
          /*
          *  no previous entry exists ... there is no previous valid pattern to be erased !
          */
          {

            arrNvM_BlockDef[nNvM_WorkingBlockID].AddressLastEntry = nNvM_Write_EEPROM_New_Block_Start_Address;
            arrNvM_BlockDef[nNvM_WorkingBlockID].nFlags |= NVM_FLAG_BLOCK_DATA_EXIST;

            /* clear write request ... */
            arrNvM_BlockDef[nNvM_WorkingBlockID].nFlags &= ~NVM_FLAG_WRITE_REQ;

            NvM_State = NVM_STATE_WAITING;
          }

        }
        else
        {
          /*
           * [ToDo] ... how many retries ???? 
           */

        }

        
      } /* end else if( (arrNvM_BlockDef[nNvM_WorkingBlockID].nDataMultiply <= 1) &&  .... */

    }
    break;    


    case NVM_STATE_WRITE_COMMAND_VALID_STAMP_WAIT_COMPLETION:
    {
      if( EEPROM_driver_Is_Write_Finished() )
      {
        NvM_State = NVM_STATE_WRITE_COMMAND_OLD_VALID_STAMP_CLEAR;
      }
      else
      {
        /*
         * [ToDo] ... how many retries ??? 
         */
      }
            
    }
    break;

    case NVM_STATE_WRITE_COMMAND_OLD_VALID_STAMP_CLEAR:
    {
      /* 
       * For 1 multiplicity blocks we don't erase anything ... and erase would delete the previous validity flags written above ...
       */
      if( arrNvM_BlockDef[nNvM_WorkingBlockID].nDataMultiply <= 1 )
      {
        arrNvM_BlockDef[nNvM_WorkingBlockID].AddressLastEntry = arrNvM_BlockDef[nNvM_WorkingBlockID].AddressStart;

        /* clear write request ... */
        arrNvM_BlockDef[nNvM_WorkingBlockID].nFlags &= ~NVM_FLAG_WRITE_REQ;

        NvM_State = NVM_STATE_WAITING;
        
      }
      else
      {
        if( arrNvM_BlockDef[nNvM_WorkingBlockID].nFlags & NVM_FLAG_BLOCK_DATA_EXIST )
        {

        /*
         *  !!! EEPROM  !!! access is a timely cost opperation --- 3ms per access
         *
         *   and all internal lib functions start with --> do {} while (!eeprom_is_ready ());   
         *    [ToDo] ... check optimization ....
         */
          //EEPROM.write( arrNvM_BlockDef[nNvM_WorkingBlockID].AddressLastEntry + arrNvM_BlockDef[nNvM_WorkingBlockID].nLen, 0xFF );

          //if( EEPROM_driver_Write_Byte_Fast(arrNvM_BlockDef[nNvM_WorkingBlockID].AddressLastEntry + arrNvM_BlockDef[nNvM_WorkingBlockID].nLen, 0xFF) )
          nTemp = 0xFF;
        
          if( EEPROM_driver_Write_Req(arrNvM_BlockDef[nNvM_WorkingBlockID].AddressLastEntry + arrNvM_BlockDef[nNvM_WorkingBlockID].nLen, &nTemp, 1) == 1 )
          {
            /* nothing to do ... */
          }
          else
          {
            /* 
             * [ToDo] ... 
             * how many retries ??? 
             */

            break;            
          }
        }

        /* ... this is the new block address  */
        arrNvM_BlockDef[nNvM_WorkingBlockID].AddressLastEntry = nNvM_Write_EEPROM_New_Block_Start_Address;

        arrNvM_BlockDef[nNvM_WorkingBlockID].nFlags |= NVM_FLAG_BLOCK_DATA_EXIST;


        /* clear write request ... */
        arrNvM_BlockDef[nNvM_WorkingBlockID].nFlags &= ~NVM_FLAG_WRITE_REQ;

        #if(NVM_MANAGER_DEBUG_SERIAL == 1)
          Serial.println();
          Serial.print("[I] NVM Info - WRITE -done: ID");
          snprintf(NvM_DebugPrintBuff, 30, " %d on EEPROM_Addr %d ", nNvM_WorkingBlockID, arrNvM_BlockDef[nNvM_WorkingBlockID].AddressLastEntry);
          Serial.println(NvM_DebugPrintBuff);
        #endif /* NVM_MANAGER_DEBUG_SERIAL */
      

        NvM_State = NVM_STATE_WRITE_COMMAND_OLD_VALID_STAMP_CLEAR_WAIT_COMPLETION;
      }

    }

    break;    

    case NVM_STATE_WRITE_COMMAND_OLD_VALID_STAMP_CLEAR_WAIT_COMPLETION:
    {
     if( EEPROM_driver_Is_Write_Finished() )
      {
        NvM_State = NVM_STATE_WAITING;
      }
      else
      {
        /*
         * [ToDo] ... how many retries ??? 
         */
      }
            
    }

    break;

    /****************************************
     *
     *        Reading procedure .....
     *
     ****************************************/


    case NVM_STATE_READ_COMMAND_VALIDATE_BLOCK:
    {
      /* check to see if we find the valid marker on the end of the block */

        /*
         *  !!! EEPROM  !!! access is a timely cost opperation --- 3ms per access
         *
         *   and all internal lib functions start with --> do {} while (!eeprom_is_ready ());   
         *    [ToDo] ... check optimization ....
         */
      //nEEPROM_Read_Val = EEPROM.read( arrNvM_BlockDef[nNvM_WorkingBlockID].AddressLastEntry + arrNvM_BlockDef[nNvM_WorkingBlockID].nLen );
      nEEPROM_Read_Val = EEPROM_driver_Forced_Read_NoWait(arrNvM_BlockDef[nNvM_WorkingBlockID].AddressLastEntry + arrNvM_BlockDef[nNvM_WorkingBlockID].nLen);

      if( nEEPROM_Read_Val == NVM_VALID_BLOCK_PATTERN )
      {
        nNvM_Read_EEPROM_Address = arrNvM_BlockDef[nNvM_WorkingBlockID].AddressLastEntry;

        NvM_State = NVM_STATE_READ_COMMAND_DATA;
      }
      else
      {
        /* not a valid block ... don't read !!  */
        arrNvM_BlockDef[nNvM_WorkingBlockID].nFlags &= ~(NVM_FLAG_READ_REQ | NVM_FLAG_BLOCK_DATA_EXIST);

        NvM_State = NVM_STATE_WAITING;
      }
    }
    break;

    case NVM_STATE_READ_COMMAND_VALIDATE_BLOCK_WAIT_COMPLETION:
    /*
     *  [ToDo] ... check if needed !!!
     *
     */    
    break;

    case NVM_STATE_READ_COMMAND_DATA:
    {
      // /* do not exceed the number of bytes to be written in one reccurence .... */
      // for (nIdx = 0; nIdx < NVM_EEPROM_READ_BYTES_IN_ONE_CYCLE; nIdx ++)
      // {
      //   if(nNvM_DataBytesToBeReadout > 0)
      //   {

      //   /*
      //    *  !!! EEPROM  !!! access is a timely cost opperation --- 3ms per access
      //    *
      //    *   and all internal lib functions start with --> do {} while (!eeprom_is_ready ());   
      //    *    [ToDo] ... check optimization ....
      //    */
      //     nEEPROM_Read_Val =  EEPROM.read( nNvM_Read_EEPROM_Address );

      //     *ptrNvM_RAM_Mirror = (unsigned char)nEEPROM_Read_Val;

      //     ptrNvM_RAM_Mirror--;
      //     nNvM_Read_EEPROM_Address ++;

      //     nNvM_DataBytesToBeReadout --;
      //   }
      //   else
      //   {
      //     break;
      //   }

      // }/* end for */

      if( EEPROM_driver_Read_Req(nNvM_Read_EEPROM_Address, nNvM_DataBytesToBeReadout) )
      {
        NvM_State = NVM_STATE_READ_COMMAND_DATA_WAIT_COMPLETION;
      }
      else
      {
        /*
         *  [ToDo] ... how many retries ??? 
         *
         */
      }

    }
    break;

    case NVM_STATE_READ_COMMAND_DATA_WAIT_COMPLETION:
    {

      if( EEPROM_driver_Is_Read_Finished() )
      {
        
        EEPROM_driver_Get_Read_Buffer_Content(ptrNvM_RAM_Mirror, nNvM_DataBytesToBeReadout);
        
        /* mark read command completed ...  */
        arrNvM_BlockDef[nNvM_WorkingBlockID].nFlags &= ~(NVM_FLAG_READ_REQ);

        #if(NVM_MANAGER_DEBUG_SERIAL == 1)
          Serial.println();
          Serial.print("[I] NVM Info - READ -done: ID");
          snprintf(NvM_DebugPrintBuff, 30, " %d on EEPROM_Addr %d ", nNvM_WorkingBlockID, arrNvM_BlockDef[nNvM_WorkingBlockID].AddressLastEntry);
          Serial.println(NvM_DebugPrintBuff);
        #endif /* NVM_MANAGER_DEBUG_SERIAL */

        NvM_State = NVM_STATE_WAITING;
      }

    }
    break;

    default:
    {
      NvM_State = NVM_STATE_NONINIT;
    }
  }

}



int NvM_Read_Block (unsigned char nBlockID)
{
  int nReturn = 0;

  if(nBlockID < NVM_BLOCK_COUNT)
  {
    arrNvM_BlockDef[nBlockID].nFlags |= NVM_FLAG_READ_REQ;

    nReturn = 1;
  }

  return nReturn;
}


int NvM_Write_Block(unsigned char nBlockID)
{
  int nReturn = 0;

  if( nBlockID < NVM_BLOCK_COUNT) 
  {
    /*  add write request ...  */
    arrNvM_BlockDef[nBlockID].nFlags |= NVM_FLAG_WRITE_REQ;

    nReturn = 1;
  }

  return nReturn;

}


void NvM_WriteAll(void)
{
  unsigned char nIdx;

  for(nIdx =0; nIdx < NVM_BLOCK_COUNT; nIdx ++)
  {
    arrNvM_BlockDef[nIdx].nFlags |= NVM_FLAG_WRITE_REQ;
  }

}

int NvM_AttachRamImage(char *ptrData, char nBlockID)
{
  int nReturn = 0;

  if( ( nBlockID < NVM_BLOCK_COUNT)  && (ptrData != NULL) )
  {
    arrNvM_BlockDef[nBlockID].DataPtr = ptrData;

    nReturn = 1;
  }

  return nReturn;
}

unsigned char NvM_Get_Block_Status(unsigned char nBlockID)
{
  if(nBlockID < NVM_BLOCK_COUNT)  
    return arrNvM_BlockDef[nBlockID].nFlags;
  else
    return 0;
}

unsigned char NvM_Is_Block_Opperation_Free(unsigned char nBlockID)
{
  unsigned char nReturn = 0;

  if(nBlockID < NVM_BLOCK_COUNT)  
  {
    if( ( arrNvM_BlockDef[nBlockID].nFlags & ( NVM_FLAG_WRITE_REQ | NVM_FLAG_READ_REQ ) ) == 0x00 )
    {
      nReturn = 0x01;
    }
  }

  return nReturn;  
}


#if(NVM_MANAGER_DEBUG_SERIAL == 1)
/* 
 * used in debug mode ....
 */
void NvM_Print_Memory_Range(unsigned int nAddrStart, unsigned int nAddrEnd, unsigned int nCountDisp)
{
  unsigned char nDataEEPROM = 0;
  unsigned int nIdx = 0;
  unsigned int nLocalCoutDisp = nCountDisp;

  Serial.println("[I] NVM EEPROM DUMP: ");
  snprintf(NvM_DebugPrintBuff, 30, " [dec]%5d [hex]%04X :", nAddrStart, nAddrStart);
  Serial.print(NvM_DebugPrintBuff);

  for (nIdx = nAddrStart; nIdx <= nAddrEnd; nIdx ++)
  {

    /*
    *  !!! EEPROM  !!! access is a timely cost opperation --- 3ms per access
    *
    *   and all internal lib functions start with --> do {} while (!eeprom_is_ready ());   
    *    [ToDo] ... check optimization ....
    */

    nDataEEPROM = EEPROM.read(nIdx);


    snprintf(NvM_DebugPrintBuff, 30, " %02X ", nDataEEPROM);
    Serial.print(NvM_DebugPrintBuff);
    

    if(nLocalCoutDisp > 0)
    {
      nLocalCoutDisp --;
    }
    else
    {
      nLocalCoutDisp = nCountDisp;

      Serial.println();
      snprintf(NvM_DebugPrintBuff, 30, " [dec]%5d [hex]%04X :", nIdx + 1, nIdx + 1);
      Serial.print(NvM_DebugPrintBuff);
    }


  }

  Serial.println();
}

void NvM_Fill_Memory_Range(unsigned int nAddrStart, unsigned int nAddrEnd, unsigned char nPattern)
{
  unsigned int nIdx = 0;

  for (nIdx = nAddrStart; nIdx <= nAddrEnd; nIdx ++)
  {

    /*
     *  !!! EEPROM  !!! access is a timely cost opperation --- 3ms per access
     *
     *   and all internal lib functions start with --> do {} while (!eeprom_is_ready ());   
     *    [ToDo] ... check optimization ....
     */

    EEPROM.write(nIdx, nPattern);
  }
}

#endif /* #if(NVM_MANAGER_DEBUG_SERIAL == 1) */