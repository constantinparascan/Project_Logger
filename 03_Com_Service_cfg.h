#ifndef __COM_SERVICE_CFG_H__
#define __COM_SERVICE_CFG_H__


/*
 *  ... how much time we wait for a "discovery" message is waited to be answered !!
 *  ... after this amount of time an re-transmission is issued ...
 */
#define COM_SERVICE_MAX_WAIT_ITTER_SERVER_CONNECTION (500)

/*
 * ... how much time we wait for a message to be transmitted and a response to be received
 * ... until a re-transmission will be done ... 
 */
#define COM_SERVICE_MAX_WAIT_ITTER_SERVER_MESSAGE_RESPONSE (30)

#endif /* __COM_SERVICE_CFG_H__ */