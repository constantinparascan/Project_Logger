
#ifndef __GSM_SIM900_CFG_H__
#define __GSM_SIM900_CFG_H__


/*
 * GPIO ports used for POWER and RESET lines of SIM900
 */

#define ARDUINO_DIG_OUT_RESET_LINE (40)
#define ARDUINO_DIG_OUT_POWER_LINE (42)

#define MAX_BEFORE_POWER_ON_DELAY           (700)
#define MAX_RESET_LINE_HIGH_LEVEL_DELAY     (700)
#define MAX_POWER_ON_LINE_HIGH_LEVEL_DELAY  (700)


/*
 * Use USART 1 .. .for communication with GSM module
 */
#define AT_USE_SERIAL_PORT (3)


#define AT_SERIAL_COM_SPEED (115200)

/* constant defining how much time we wait for a command to be transmitted from 
   user board to SIMxxx board ...
 */
#define AT_MAX_DELAY_WAIT_TRANSMISSION (500)


#define AT_MAX_DELAY_WAIT_COUNTER (100)

#define AT_MAX_ERROR_RETRY_COUNTER (5)

/*
 * max data that is received from Server that comes with a command
 */
#define AT_MAX_DATA_FROM_SERVER_COMMAND (70)


#endif /* __GSM_SIM900_CFG_H__ */