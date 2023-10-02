/* C.Parascan
 * 
 *   v0.1 - first implementation 05.06.2023
 *
 */
#ifndef __LOGGER_APP_PARALLEL_H__
#define __LOGGER_APP_PARALLEL_H__


#define LOGGER_APP_PARALLEL_DEBUG_ENABLE (0)


void Logger_App_Parallel_init (void);
void Logger_App_Parallel_main (void);
void Logger_App_Parallel_main_v2(void);

unsigned char Logger_App_Parallel_HasNewEvents(void);
void Logger_App_Parallel_ResetEventsStatus(void);

unsigned short Logger_App_Parallel_GetChannelValue(unsigned char nValue);
unsigned char Logger_App_Parallel_GetLastBillType(void);


void Logger_App_Parallel_Callback_Command_Reset_Monetary(void);

#endif /* __LOGGER_APP_PARALLEL_H__ */