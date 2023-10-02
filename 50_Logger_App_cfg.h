#ifndef __LOGGER_APP_CFG_H__
#define __LOGGER_APP_CFG_H__




/* -------------------------------------------- */
/* 
 *  This defines the inter communication delay ... thet is used to separate the frames
 *      - if the delay in communication is grater than this amount of imer callbac itterations
 *        then we considere that a new communication frame is issued.
 */

#define LOG_APP_STARTUP_DELAY (50)     /* delay for the startup of application - unsigned short                                */


#define _MAX_INTER_FRAME_TIME_ (4)    /* inter frame gap - determins copy of the USART raw data to local buff - unsigned short */

#define _MAX_ITTER_WITHOUT_RX_COMM_ (200)

/* used to specify the memory size allocated for the local Rx buffer */
#define _MAX_RX_COMM_BUFFER_SIZE_ (300)


#define _SHUTDOWN_REQUEST_DEBOUNCE_ (30)


/* -------------------------------------------- */
/*
 *    OUTPUT pins ...
 */
#define OUTPUT_PIN_RESET_BILL (13)
#define OUTPUT_PIN_RESET_BILL_DELAY_ITTER (10)


/* -------------------------------------------- */
/*
 *    HW CONFIG pins ...
 */
#define HW_CONFIG_PIN_1  (27)
#define HW_CONFIG_PIN_2  (28)
#define HW_CONFIG_PIN_3  (29)

#endif /* __LOGGER_APP_CFG_H__ */