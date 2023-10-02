/* EEPROM driver
 *
 *  v0.1 C.Parascan - 15.06.2023
 *
 *    - driver created to improve time access for writting/reading asynchronous from EEPROM
 *    - this driver differs from the arduino normal implementation by the missing blocking 
 *          waiting delays. Instead an state-machine was done that takes care of this 
 *          needed waiting time-outs.
 */
#include "09_EEPROM_Driver.h"

#define EEPROM_FLAGS_WRITE_REQUEST           (0x80)
#define EEPROM_FLAGS_WRITE_REQUEST_FINISHED  (0x40)

#define EEPROM_FLAGS_READ_REQUEST            (0x08)
#define EEPROM_FLAGS_READ_REQUEST_FINISHED   (0x04)

/*
 * Structure that is used to keep the asynchronous several bytes long
 *  read-write requests ...
 */
typedef struct TAG_EEPROM_Internals
{
    unsigned int  nReadAddress;     /* current read address                             */
    unsigned char nReqReadLen;      /* defines how many more bytes needs to be read-out */
    unsigned char nReadIdx;         /* current index inside read-buffer used to deposit EEPROM read-out bytes */
    unsigned char arrReadBuff[100]; /* read-out buffer                                  */

    unsigned int  nWriteAddress;     /* current write address                            */
    unsigned char nReqWriteLen;      /* defines how many more bytes needs to be written  */
    unsigned char nWriteIdx;         /* current index inside write-buffer used to write to EEPROM */
    unsigned char arrWriteBuff[100]; /* write-out buffer                                  */


    unsigned char nFlags;            /* EEPROM status falgs                               */
                                     /*
                                      *   1*** ****  write request or write busy
                                      *   *1** ****  write complete
                                      *
                                      *   **** 1***  read request or read busy
                                      *   **** *1**  read complete
                                      */

}T_EEPROM_Internals;


/*  EEPROM internal states 
 *
 */
typedef enum TAG_EEPROM_State
{
  EEPROM_DRIVER_STATE_IDDLE = 0,
  EEPROM_DRIVER_STATE_WRITE_OPERATION_BUSY,
  EEPROM_DRIVER_STATE_READ_OPPERATION_BUSY
}T_EEPROM_State;


T_EEPROM_State      eEEPROM_State;
T_EEPROM_Internals  sEEPROM_Internals;


/* C.Parascan - v0.1
 *    - init function of the EEPROM driver 
 */
void EEPROM_driver_init(void)
{
  sEEPROM_Internals.nReadAddress = 0x0000;
  sEEPROM_Internals.nReadIdx = 0;
  sEEPROM_Internals.nReqReadLen = 0;

  sEEPROM_Internals.nWriteAddress = 0x0000;
  sEEPROM_Internals.nWriteIdx = 0;
  sEEPROM_Internals.nReqWriteLen = 0;

  sEEPROM_Internals.nFlags = 0x00;

  eEEPROM_State = EEPROM_DRIVER_STATE_IDDLE;

  #if(EEPROM_DRIVER_DEBUG_ENABLE == 1)
  Serial.begin(9600);
  Serial.println("[I] EEPROM driver init ");
  #endif

}

/* C.Parascan - v0.1
 *    - cyclic function of the EEPROM driver 
 */
void EEPROM_driver_main(void)
{
  switch(eEEPROM_State)
  {

      /*
       * Iddle state ... here we wait for requests !!!
       */
      case EEPROM_DRIVER_STATE_IDDLE:
      {

        /* 
         * nothing to do ... just wait ... 
         */

        #if(EEPROM_DRIVER_DEBUG_ENABLE == 1)

          //Serial.println();
          //Serial.print("FLAGS: ");
          //Serial.print(sEEPROM_Internals.nFlags);

        #endif

      }
      break;


      /*
       *  WRITE consecutive bytes ...
       *
       */
      case EEPROM_DRIVER_STATE_WRITE_OPERATION_BUSY:
      {
          /* is eeprom busy ? */ 
          if( EECR & (1 << EEPE) )
          {
            /* cannot write .. just wait ... for next time */            
          }
          else
          {

            /* still something to write ...
             */
            if(sEEPROM_Internals.nReqWriteLen > 0)
            {

              /* Set up address and Data Registers */
              EEAR = sEEPROM_Internals.nWriteAddress;
              EEDR = sEEPROM_Internals.arrWriteBuff[ sEEPROM_Internals.nWriteIdx ];


              #if(EEPROM_DRIVER_DEBUG_ENABLE == 1)
                Serial.println();
                Serial.print("EEP addr & data: ");
                Serial.print(sEEPROM_Internals.nWriteAddress);
                Serial.print(" ");
                Serial.print(sEEPROM_Internals.arrWriteBuff[ sEEPROM_Internals.nWriteIdx ]);
              #endif

              /* Write logical one to EEMPE */
              EECR |= ( 1 << EEMPE );
    
              /* Start eeprom write by setting EEPE */
              EECR |= ( 1 << EEPE );

              sEEPROM_Internals.nWriteAddress ++;
              sEEPROM_Internals.nWriteIdx ++;
              sEEPROM_Internals.nReqWriteLen --;
            }
            else
            {
              /* 
               * Mark completion of the write request ... 
               */
              sEEPROM_Internals.nFlags &= ~EEPROM_FLAGS_WRITE_REQUEST;
              sEEPROM_Internals.nFlags |=  EEPROM_FLAGS_WRITE_REQUEST_FINISHED;

              /* goto IDDLE ... and wait new requests ...
               */
              eEEPROM_State = EEPROM_DRIVER_STATE_IDDLE;
            }

          }/*  end else EECR & (1 << EEPE) ---> EEPROM not busy  */
      }
      break;


      /* READ consecutive bytes
       *
       */
      case EEPROM_DRIVER_STATE_READ_OPPERATION_BUSY:
      {
        /* is eeprom busy ? */ 
        if( EECR & (1 << EEPE) )
        {
          /* cannot read .. just wait ... */            
        }
        else
        {

          /* still something to read ...
          */
          if(sEEPROM_Internals.nReqReadLen > 0)
          {

            /* Set up address and Data Registers */
            EEAR = sEEPROM_Internals.nReadAddress;

            /* Write logical one to EERE */
            EECR |= ( 1 << EERE );
    

            /*  Small delay determined by below instructions 
             */
            sEEPROM_Internals.nReqReadLen  --;
            sEEPROM_Internals.nReadAddress ++;


            sEEPROM_Internals.arrReadBuff[ sEEPROM_Internals.nReadIdx ] = EEDR;

            #if(EEPROM_DRIVER_DEBUG_ENABLE == 1)
              //Serial.println();
              //Serial.print(sEEPROM_Internals.nReadAddress - 1);
              //Serial.print(" ");
              //Serial.print(sEEPROM_Internals.arrReadBuff[ sEEPROM_Internals.nReadIdx ] );
            #endif


            sEEPROM_Internals.nReadIdx ++;

          }
          else
          {
            /* 
             * Mark completion of the write request ... 
             */
            sEEPROM_Internals.nFlags &= ~EEPROM_FLAGS_READ_REQUEST;
            sEEPROM_Internals.nFlags |=  EEPROM_FLAGS_READ_REQUEST_FINISHED;

            /* goto IDDLE ... and wait new requests ...
             */
            eEEPROM_State = EEPROM_DRIVER_STATE_IDDLE;
          }

        }/*  end else EECR & (1 << EEPE) ---> EEPROM not busy  */

      } /* end case EEPROM_DRIVER_STATE_READ_OPPERATION_BUSY */
      break;


  }/* end switch */

}/* end function EEPROM_driver_main */

/* 
 * Write request 
 */
unsigned char EEPROM_driver_Write_Req(unsigned int nAddr, unsigned char *srcData, unsigned char nLen)
{
  unsigned char nReturn = 0;

  /* if not in a busy state ...
   */
  if( ( sEEPROM_Internals.nFlags & ( EEPROM_FLAGS_WRITE_REQUEST | EEPROM_FLAGS_READ_REQUEST ) ) == 0x00 )
  {

    if( nLen > 1 )
    {
      memcpy(sEEPROM_Internals.arrWriteBuff, srcData, nLen);
    }
    else
    {
      sEEPROM_Internals.arrWriteBuff[0] = *srcData;      
    }

    sEEPROM_Internals.nWriteAddress = nAddr;
    sEEPROM_Internals.nWriteIdx = 0;
    sEEPROM_Internals.nReqWriteLen = nLen;

    sEEPROM_Internals.nFlags &= ~EEPROM_FLAGS_WRITE_REQUEST_FINISHED;
    sEEPROM_Internals.nFlags |=  EEPROM_FLAGS_WRITE_REQUEST;

    eEEPROM_State = EEPROM_DRIVER_STATE_WRITE_OPERATION_BUSY;    

    nReturn = 1;
  }

  return nReturn;

}/* end EEPROM_driver_Write_Req */

unsigned char EEPROM_driver_Write_Byte_Fast(unsigned int nAddr, unsigned char srcData)
{
  unsigned char nReturn = 0;

  /* if not in a busy state ...
   */
  if( ( sEEPROM_Internals.nFlags & ( EEPROM_FLAGS_WRITE_REQUEST | EEPROM_FLAGS_READ_REQUEST ) ) == 0x00 )
  {
    if( EECR & (1 << EEPE) == 0 )
    {

      EEAR = nAddr;
      EEDR = srcData;

      /* Write logical one to EEMPE */
      EECR |= ( 1 << EEMPE );
    
      /* Start eeprom write by setting EEPE */
      EECR |= ( 1 << EEPE );

      sEEPROM_Internals.nWriteIdx = 0;
      sEEPROM_Internals.nReqWriteLen = 0;

      sEEPROM_Internals.nFlags &= ~EEPROM_FLAGS_WRITE_REQUEST_FINISHED;
      sEEPROM_Internals.nFlags |=  EEPROM_FLAGS_WRITE_REQUEST;

      eEEPROM_State = EEPROM_DRIVER_STATE_WRITE_OPERATION_BUSY;    

      nReturn = 1;
    }
        
  }

  return nReturn;

}/* end EEPROM_driver_Write_Byte_Fast */


unsigned char EEPROM_driver_Is_Write_Finished(void)
{
  unsigned char nReturn = 0;

  if( sEEPROM_Internals.nFlags | EEPROM_FLAGS_WRITE_REQUEST_FINISHED )
  {
    nReturn = 1;
  }

  return nReturn;
}

/* 
 * Read request 
 */
unsigned char EEPROM_driver_Read_Req(unsigned int nAddr, unsigned char nLen)
{
  unsigned char nReturn = 0;

  if( ( sEEPROM_Internals.nFlags & ( EEPROM_FLAGS_WRITE_REQUEST | EEPROM_FLAGS_READ_REQUEST ) ) == 0x00 )
  {

    sEEPROM_Internals.nReadAddress = nAddr;    
    sEEPROM_Internals.nReadIdx = 0;
    sEEPROM_Internals.nReqReadLen = nLen;
    
    sEEPROM_Internals.nFlags &= ~EEPROM_FLAGS_READ_REQUEST_FINISHED;
    sEEPROM_Internals.nFlags |=  EEPROM_FLAGS_READ_REQUEST;

    eEEPROM_State = EEPROM_DRIVER_STATE_READ_OPPERATION_BUSY;

    nReturn = 1;
  }

  return nReturn;

}/* end EEPROM_driver_Read_Req */

unsigned char EEPROM_driver_Is_Read_Finished(void)
{
  unsigned char nReturn = 0;

  if( sEEPROM_Internals.nFlags | EEPROM_FLAGS_READ_REQUEST_FINISHED )
  {
    nReturn = 1;
  }

  return nReturn;
}

void EEPROM_driver_Get_Read_Buffer_Content(unsigned char *destData, unsigned char nLen)
{
  if(nLen > 1)
  {
    memcpy(destData, sEEPROM_Internals.arrReadBuff, nLen);
  }
  else
  {
    *destData = sEEPROM_Internals.arrReadBuff[0];
  }
}


unsigned char EEPROM_driver_Forced_Read_NoWait(unsigned int uiAddress)
{
  /* Do not wait for completion of previous write ... if any ... */
  //while(EECR & (1<<EEPE));

  /* Set up address register */
  EEAR = uiAddress;
  /* Start eeprom read by writing EERE */
  EECR |= (1<<EERE);
  /* Return data from Data Register */

  /*
   * small dealy ...
   */
  __asm__("nop\n\t");
  __asm__("nop\n\t");
  __asm__("nop\n\t");

  return EEDR;
}


/* *********************************************************
 *
 *
 *  DEBUG functions !!!!
 *
 *
 * *********************************************************/

#if(EEPROM_DRIVER_DEBUG_ENABLE == 1)

void EEPROM_driver_Debug_Write_Pattern_1(void)
{
  unsigned char arr[32] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, \
                           0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F};

  unsigned char nStatus = 0;

  nStatus = EEPROM_driver_Write_Req(0, arr, 32);

  if( nStatus == 1 )
  {
    Serial.println(" Write Accepted ... Pattern 1");
  }
}

void EEPROM_driver_Debug_Write_Pattern_2(void)
{
  unsigned char arr[32] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, \
                           0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

  unsigned char nStatus = 0;

  nStatus = EEPROM_driver_Write_Req(0, arr, 32);

  if( nStatus == 1 )
  {
    Serial.println(" Write Accepted ... Pattern 2");
  }
}

unsigned char arr_readout[32];

void EEPROM_driver_Debug_Read_Pattern(void)
{
  unsigned char nStatus = 0;

  nStatus = EEPROM_driver_Read_Req (0, 32);

  if(nStatus == 1)
  {
    Serial.println(" Read Accepted ... ");
  }
}

void EEPROM_driver_Debug_Print_Read_Pattern(void)
{
  unsigned char nIdx = 0;

  if(EEPROM_driver_Is_Read_Finished() == 1)
  {
    EEPROM_driver_Get_Read_Buffer_Content(arr_readout, 32);

    Serial.println("EEPROM Data: ");
    for(nIdx = 0; nIdx < 32; nIdx ++)
    {
      Serial.print(arr_readout[nIdx]);
    }
    Serial.println();
  }
}

#endif /* #if(EEPROM_DRIVER_DEBUG_ENABLE == 1) */