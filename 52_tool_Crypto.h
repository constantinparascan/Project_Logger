#ifndef __TOOL_CRYPTO_H__
#define __TOOL_CRYPTO_H__


/* 
 -----  Exported API's -----
*/

/* API used to encrypt a byte array located in the *buffer (must be pre-allocated)
   - the result is stored also in the *buffer  - I/O
   - the length of the message is specified by "n"
*/ 
void tool_crypto_encrypt( unsigned char *buffer, int n );


/* API used to decrypt a byte array located in the *buffer (must be pre-allocated)
   - the result is stored also in the *buffer  - I/O
   - the length of the message is specified by "n"
*/ 
void tool_crypto_decrypt( unsigned char *buffer, int n );


#endif /* __TOOL_CRYPTO_H__ */