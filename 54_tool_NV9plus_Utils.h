#ifndef __TOOL_NV9PLUS_UTILS_H__
#define __TOOL_NV9PLUS_UTILS_H__


#define TOOL_NV9PLUS_UTILS_SERIAL_DEBUG (0)

/* used to define the length of the response of command 159(dec) == 0x9F(hex) */
#define CMD_159_RESP_LEN (11)


typedef enum _TAG_NV9plus_chan_error
{
    E_MASTER_INHIBIT = 0,    /*    0   0 */
    E_BILL_RETURNED = 1,     /*    0   1 */
    E_VALIDATION_FAIL = 2,   /*    0   2 */
    E_TRANSPORT = 3,         /*    0   3 */
    E_TIMEOUT = 4,           /*    0   4 */
    E_UNSAFE_JAM = 6,        /*    0   6 */
    E_FRAUD = 9,             /*    0   9 */
    E_CASHBOX_REMOVED = 11,  /*    0  11 */
    E_CASHBOX_REPLACED = 12, /*    0  12 */
    E_STACKER_FULL = 14,     /*    0  14 */
    E_SAFE_JAM = 16,         /*    0  16 */
    E_BARCODE = 20,          /*    0  20 */
    E_UNKNOWN = 50           /*    0  ?? */
}T_NV9plus_Error;

typedef enum _TAG_NV9plus_state
{
    NV9_STATE_UNINIT = 0,
    NV9_STATE_INIT_STARTUP = 1,
    NV9_STATE_RUN_NO_ERROR = 2,
    NV9_STATE_RUN_ERROR = 3,
    NV9_STATE_BLOCKED_RETRY = 4,
    NV9_STATE_BLOCKED_NO_RECOVERY = 5
}T_NV9plus_state;

typedef struct _TAG_NV9plus_internals
{
    unsigned short    nCH1_bill;
    unsigned short    nCH2_bill;
    unsigned short    nCH3_bill;
    unsigned short    nCH4_bill;
    unsigned short    nCH5_bill;

    T_NV9plus_Error nBillValidator_err;

    unsigned char   nEvCnt;
    unsigned char   bValidatorHadReset;


    T_NV9plus_state eNV9State;

}T_NV9plus_internal;


/* Exported API's
 *
 */

/* first API to be called before using any of the other functions
   - initialize driver
*/
void tool_NV9plus_utils_init(void);
int  tool_NV9plus_utils_decode_status(unsigned char *arrDataBytes, unsigned char nLen, unsigned char nFirstRxFrame_State );
int  tool_NV9plus_utils_GetChannelContent(unsigned char nChan, unsigned int* nChanBillVal, T_NV9plus_Error* eValidatorErr);
void tool_NV9plus_utils_ResetCashContent(void);


#endif /*__TOOL_NV9PLUS_UTILS_H__*/
