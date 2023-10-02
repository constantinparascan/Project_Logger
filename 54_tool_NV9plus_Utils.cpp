/* C.Parascan v0.1 - 23.11.2022 - first implementation of utility functions
                                  used to collect data from NV9+ cash machine.
              v0.2 - 03.12.2022 - fixes in restore function

              v1.0 - 22.02.2023 - optimization, completion of reset bill, power on notification, PING
*/
#include <string.h> /* used for memcpy and strcmp definition */
#include "54_tool_NV9plus_Utils.h"
#include <Arduino.h>


/* local stored values of response produced by a command "159" (0x9F)*/
unsigned char cmd_159_RawData[CMD_159_RESP_LEN];

/* internal variable keeping content of decoded raw data */
T_NV9plus_internal sNV9plus;

#if(TOOL_NV9PLUS_UTILS_SERIAL_DEBUG == 1)
char NV9Plus_Utils_DebugPrintBuff[50];
#endif

/*
 * Private API's
 */

void tool_NV9plus_utils_ResetCashContent(void);
T_NV9plus_Error tool_NV9plus_utils_getError(unsigned char nChanA, unsigned char nChanB);


/* Init driver ...
 *
 */
void tool_NV9plus_utils_init(void)
{
    unsigned char nIdx = 0;


    #if(TOOL_NV9PLUS_UTILS_SERIAL_DEBUG == 1)
    Serial.begin(9600);
    //printf("\ntool_NV9plus_utils_init  ... ");
    #endif

    /* no events ... */
    sNV9plus.nEvCnt = 0;
    /* no validator internal resets detected ... */
    sNV9plus.bValidatorHadReset = 0;


    /* try to recover content ... if possible .. if not reset internal variables ... */
    if(/*tool_NV9plus_utils_RestoreDataFromFile() == -1*/ 1)
    {
        #if(TOOL_NV9PLUS_UTILS_SERIAL_DEBUG == 1)
        //printf("Restore default ... ");
        #endif

        /* no bills ... and no errors ...*/
        //sNV9plus.nCH1_bill = 0;
        //sNV9plus.nCH2_bill = 0;
        //sNV9plus.nCH3_bill = 0;
        //sNV9plus.nCH4_bill = 0;
        //sNV9plus.nCH5_bill = 0;


        sNV9plus.nBillValidator_err  = E_UNKNOWN;
    }

    sNV9plus.eNV9State = NV9_STATE_INIT_STARTUP;

    /* reset raw data buffer ... */
    for(nIdx = 0; nIdx < CMD_159_RESP_LEN; nIdx ++)
    {
        cmd_159_RawData[nIdx] = 0;
    }

}

/* API used internally to decode the possible channel errors
 *  - input: bytes group position 1..5
 *         : received data bytes from command 159 (0x9F)
 *
 *  - output: decoded error
 */
T_NV9plus_Error tool_NV9plus_utils_getError(unsigned char nChanA, unsigned char nChanB)
{
    T_NV9plus_Error eErrRet = E_UNKNOWN;

    /* error present on channel ... */
    if( nChanA == 0 )
	  {
	    switch( nChanB )
      {
        case 0:
          eErrRet = E_MASTER_INHIBIT;
        break;

        case 1:
          eErrRet = E_BILL_RETURNED;
        break;

        case 2:
          eErrRet = E_VALIDATION_FAIL;
        break;

        case 3:
          eErrRet = E_TRANSPORT;
        break;

        case 4:
          eErrRet = E_TIMEOUT;
        break;

        case 6:
          eErrRet = E_UNSAFE_JAM;
        break;

        case 9:
          eErrRet = E_FRAUD;
        break;

        case 11:
          eErrRet = E_CASHBOX_REMOVED;
        break;

        case 12:
          eErrRet = E_CASHBOX_REPLACED;
        break;

        case 14:
          eErrRet = E_STACKER_FULL;
        break;

        case 16:
          eErrRet = E_SAFE_JAM;
        break;

        case 20:
          eErrRet = E_BARCODE;
        break;

        default:
          eErrRet = E_UNKNOWN;
        break;

      }/* end switch */

    }/* end if nChanA == 0*/
    else
    {
      eErrRet = E_UNKNOWN;
    }

    return eErrRet;

}/* tool_NV9plus_utils_getError */



/* 
 * API used to decode and store content of a raw data bytes received from NV9plus machine
 *
 * v0.1 - first implementation
 * v0.2 - update byte interpretation - only last event is processed ...
 */
int tool_NV9plus_utils_decode_status( unsigned char *arrDataBytes, unsigned char nLen, unsigned char nFirstRxFrame_State )
{
    int nReturn = 0;
    unsigned char nNrOfEventsInQueue = 0;
    unsigned char nIdxEv = 0;
    unsigned char *arrDataChannelBytes = 0;

    /* 
     * Check the validity of the input data ... 
     */
    if( ( sNV9plus.eNV9State >= NV9_STATE_INIT_STARTUP ) && ( nLen >= CMD_159_RESP_LEN ) )
    {
        #if(TOOL_NV9PLUS_UTILS_SERIAL_DEBUG == 1)
          snprintf(NV9Plus_Utils_DebugPrintBuff, 50, "Frame: %2X %2X %2X %2X %2X %2X %2X %2X %2X %2X %2X", arrDataBytes[0], arrDataBytes[1], arrDataBytes[2], \
                                                                                                           arrDataBytes[3], arrDataBytes[4], arrDataBytes[5], \
                                                                                                           arrDataBytes[6], arrDataBytes[7], arrDataBytes[8], \
                                                                                                           arrDataBytes[9], arrDataBytes[10]);
          Serial.println(NV9Plus_Utils_DebugPrintBuff);
        #endif

        /* an internal validator reset has occured ? */
        if(arrDataBytes[0] == 0)
        {
          sNV9plus.nEvCnt = 0;
          sNV9plus.bValidatorHadReset = 1;

          sNV9plus.eNV9State = NV9_STATE_INIT_STARTUP;
        }
        else
        {
          /* we have received valid data ... and it is not a first frame after a reset or power-on of the board
            ... because in case of a separate board reset ... if the NV9 continues to send old status
            ... a set of false transaction will appear ... therefore after init, first Rx frame is used to synchonize counter events
            ... and it is discarded ...                
          */
          if( (arrDataBytes[0] > sNV9plus.nEvCnt) && (nFirstRxFrame_State == 0) )
          {

            #if(TOOL_NV9PLUS_UTILS_SERIAL_DEBUG == 1)
              snprintf(NV9Plus_Utils_DebugPrintBuff, 50, "Frame: %2X %2X %2X %2X %2X %2X %2X %2X %2X %2X %2X", arrDataBytes[0], arrDataBytes[1], arrDataBytes[2], \
                                                                                                               arrDataBytes[3], arrDataBytes[4], arrDataBytes[5], \
                                                                                                               arrDataBytes[6], arrDataBytes[7], arrDataBytes[8], \
                                                                                                               arrDataBytes[9], arrDataBytes[10]);
              Serial.println(NV9Plus_Utils_DebugPrintBuff);
            #endif


            nNrOfEventsInQueue = arrDataBytes[0] - sNV9plus.nEvCnt;

            /* if nr of lost events > 5 ... that means that we have lost a lot of events before .... we can only process last 5 */
            if(nNrOfEventsInQueue > 5)
              nNrOfEventsInQueue = 5;

            #if(TOOL_NV9PLUS_UTILS_SERIAL_DEBUG == 1)
              snprintf(NV9Plus_Utils_DebugPrintBuff, 50, "New Events: %d ", nNrOfEventsInQueue);
              Serial.println(NV9Plus_Utils_DebugPrintBuff);
            #endif


            /* data channels starts from byte "1" ... */
            arrDataChannelBytes = &arrDataBytes[1];

            /* process all entries in the queue ... in reverse order as they appeared */
            for (nIdxEv = nNrOfEventsInQueue; nIdxEv > 0 ; nIdxEv --)
            {
              /* this is the first channel byte */
              if( arrDataChannelBytes[ (nIdxEv - 1) * 2 ] == 0 )
              {


                /* [ToDo] ... check to see if there is an error ... or just pending !! ...  */
                /* [ToDo]   .... OPTIMIZE  --- just coppy the error .... */
                /*
                 *
                 *
                 *
                 */
                //sNV9plus.nBillValidator_err = tool_NV9plus_utils_getError(0, arrDataChannelBytes[ (nIdxEv - 1) * 2 + 1]);
                sNV9plus.nBillValidator_err = arrDataChannelBytes[ (nIdxEv - 1) * 2 + 1];

                sNV9plus.eNV9State = NV9_STATE_RUN_ERROR;

                #if(TOOL_NV9PLUS_UTILS_SERIAL_DEBUG == 1)
                  snprintf(NV9Plus_Utils_DebugPrintBuff, 50, "\n\nError Detected: %d \n\n", sNV9plus.nBillValidator_err);
                  Serial.println(NV9Plus_Utils_DebugPrintBuff);
                #endif

                nReturn = 1;

              }
              else
              {
                /* this is a valid channel entry info ... with a validated bill */
                if( (arrDataChannelBytes[ (nIdxEv - 1) * 2 ] <= 5) && (arrDataChannelBytes[ (nIdxEv - 1) * 2 + 1] == 0) )
                {

                  /* 
                   * increment local bills value depending on the channel validation ...
                   */
                  switch( arrDataChannelBytes[ (nIdxEv - 1) * 2 ] )
                  {
                    case 1:
                      sNV9plus.nCH1_bill += 1;

                      nReturn = 1;

                      #if(TOOL_NV9PLUS_UTILS_SERIAL_DEBUG == 1)
                        Serial.print("CH1: ");
                        Serial.println(sNV9plus.nCH1_bill);
                      #endif

                    break;

                    case 2:
                      sNV9plus.nCH2_bill += 1;

                      nReturn = 1;

                      #if(TOOL_NV9PLUS_UTILS_SERIAL_DEBUG == 1)
                        Serial.print("CH2: ");
                        Serial.println(sNV9plus.nCH2_bill);
                      #endif

                    break;

                    case 3:
                      sNV9plus.nCH3_bill += 1;

                      nReturn = 1;

                      #if(TOOL_NV9PLUS_UTILS_SERIAL_DEBUG == 1)
                        Serial.print("CH3: ");
                        Serial.println(sNV9plus.nCH3_bill);
                      #endif

                    break;

                    case 4:
                      sNV9plus.nCH4_bill += 1;

                      nReturn = 1;

                      #if(TOOL_NV9PLUS_UTILS_SERIAL_DEBUG == 1)
                        Serial.print("CH4: ");
                        Serial.println(sNV9plus.nCH4_bill);
                      #endif
                        
                    break;

                    case 5:
                      sNV9plus.nCH5_bill += 1;

                      nReturn = 1;

                      #if(TOOL_NV9PLUS_UTILS_SERIAL_DEBUG == 1)
                        Serial.print("CH5: ");
                        Serial.println(sNV9plus.nCH5_bill);
                      #endif

                    break;

                  }/* end switch( arrDataChannelBytes[ (nIdxEv - 1) * 2 ] ) */

                  /* reset errors  */
                  sNV9plus.eNV9State = NV9_STATE_RUN_NO_ERROR;

                  sNV9plus.nBillValidator_err = E_UNKNOWN;

                }/* end if if( (arrDataChannelBytes[ (nIdxEv - 1) * 2 ] <= 5) && (arrDataChannelBytes[ (nIdxEv - 1) * 2 + 1] == 0) ) */
                else
                {
                  /* the cannel is in pending state ... nothing to do ... wait for a final validation ...*/
                }

              }/* end if( arrDataChannelBytes[ (nIdxEv - 1) * 2 ] == 0 ) */

            }/* end for ... */


            sNV9plus.nEvCnt = arrDataBytes[0];
                
            /* copy locally last received data bytes ... */
            //memcpy(cmd_159_RawData, arrDataBytes, CMD_159_RESP_LEN);

          }/* end if(arrDataBytes[0] > sNV9plus.nEvCnt) ... */
          else
          {
            /*
             * - this is the first frame received after a reset of the board
             *    ... load the event countes in the state-machine variables
             */
            if( (arrDataBytes[0] > sNV9plus.nEvCnt) && (nFirstRxFrame_State >= 1 ) )
            {
              sNV9plus.nEvCnt = arrDataBytes[0];

              /* reset errors  */
              sNV9plus.eNV9State = NV9_STATE_RUN_NO_ERROR;

              sNV9plus.nBillValidator_err = E_UNKNOWN;

              nReturn = 1;

            }
          }

        }/* else if(arrDataBytes[0] == 0) ... */

    }/* format of the data bytes array is correct */
    else
    {
        /* not all pre-conditions of API call met !!!  */
        nReturn = -1;
    }

    return nReturn;
}


/*
 * API used for the retrieval of channel data and error ...
 *
 * v0.1 - first implementation
 */
  
int tool_NV9plus_utils_GetChannelContent(unsigned char nChan, unsigned int* nChanBillVal, T_NV9plus_Error* eValidatorErr)
{

    int nReturn = 0;


    if(sNV9plus.eNV9State >= NV9_STATE_INIT_STARTUP)
    {
        switch(nChan)
        {
            case 1:
                *nChanBillVal = sNV9plus.nCH1_bill;
                break;

            case 2:
                *nChanBillVal = sNV9plus.nCH2_bill;
                break;

            case 3:
                *nChanBillVal = sNV9plus.nCH3_bill;
                break;

            case 4:
                *nChanBillVal = sNV9plus.nCH4_bill;
                break;

            case 5:
                *nChanBillVal = sNV9plus.nCH5_bill;
                break;

            default:
                nReturn = -1;
                *nChanBillVal = 0;
        }

        *eValidatorErr = sNV9plus.nBillValidator_err;

    }
    else
        nReturn = -1;


    return nReturn;
    
}


/* API used to reset internal variables of the driver ... used when the 
 *     cash machine is emptied of money !
 *
 * v0.1 - first implementation
 */
void tool_NV9plus_utils_ResetCashContent(void)
{

    /* no bills ... and no errors ...*/
    sNV9plus.nCH1_bill = 0;
    sNV9plus.nCH2_bill = 0;
    sNV9plus.nCH3_bill = 0;
    sNV9plus.nCH4_bill = 0;
    sNV9plus.nCH5_bill = 0;
 
    sNV9plus.nBillValidator_err  = E_UNKNOWN;

    /* no events ... */    
    sNV9plus.nEvCnt = 0;

    /* reset validator internal resets ... */
    sNV9plus.bValidatorHadReset = 0;    

}
