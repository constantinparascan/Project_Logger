/* C.Parascan v0.1 - this functions have been implemented according to document: 
 *                    "ccTalk Encryption - Bill Validators V1.2.PDF"
 *                     
 * --------------------------------------------------------------------------------        
 *  - 31.10.2022 - first integration into project. Minor cleanup and adding the 
 *                     possibility to call the brute force attack function 
 */

#include "52_tool_Crypto.h"


/* constant values specified inside the documentation .pdf file ... */
unsigned char tapArray[ 10 ] = { 7, 4, 5, 3, 1, 2, 3, 2, 6, 1 };
const int rotatePlaces = 12;
const unsigned char feedMasher = 0x63;



/* 
    detected KEY ... based on a previous brute force attack performed on a known
        request !
 */
unsigned char secArray[ 6 ] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06};  /* 0 .. 9 values accepted only !!!! */




/********************************************************
 * Encrypt message stored in "buffer" of length "n" 
 *
 *         result is stored back in "buffer"
 ********************************************************/
void tool_crypto_encrypt( unsigned char *buffer, int n )
{
	int i, j;
	int b, c, c1;

	unsigned char xorKey;

	/* initial XOR */
	xorKey = ~( 16 * secArray[ 0 ] + secArray[ 4 ] );

	for ( i = 0; i < n; ++i )
		buffer[ i ] ^= xorKey;

	/* conditional reverse bits */
	for ( i = 0; i < n; ++i )
	{
		if ( secArray[ 3 ] & ( 1 << ( i % 4 ) ) )
		{
			/* reverse bits in buffer[ i ] */
			b = 0;

			for ( j = 0; j < 8; ++j )
			{
				if ( buffer[ i ] & ( 1 << j ) )
					b += ( 128 >> j);
			}

			buffer[ i ] = b;
		}
	}


	/* rotate clockwise through bit 0 of buffer[ n - 1 ] */
	for ( i = 0; i < rotatePlaces; ++i )
	{
		c1 = ( buffer[ n - 1 ] & 0x01 ) ? 128 : 0;
		
		/* xor taps */
		for ( j = 0; j < n; ++j )
		{
			if ( buffer[ j ] & ( 1 << tapArray[ ( secArray[ 1 ] + j ) % 10 ] ) )
				c1 ^= 128;
		}

		for ( j = 0; j < n; ++j )
		{
			c = ( buffer[ j ] & 0x01 ) ? 128 : 0;

			/* xor feed masher */
			if ( ( secArray[ 5 ] ^ feedMasher ) & ( 1 << ( ( i + j ) % 8 ) ) )
				c ^= 128;

			/* shift buffer right */
			buffer[ j ] = ( buffer[ j ] >> 1 ) + c1;
			c1 = c;
		}
	}


	/* final XOR */
	xorKey = 16 * secArray[ 2 ] + secArray[ 2 ];
		
	for ( i = 0; i < n; ++i )
		buffer[ i ] ^= xorKey;

}/* tool_crypto_encrypt */


/********************************************************
 * Decrypt message stored in "buffer" of length "n" 
 *
 * result is stored back in "buffer"
 * 
 ********************************************************/

void tool_crypto_decrypt( unsigned char *buffer, int n )
{
	int i, j;
	int b, c, c1;
	unsigned char xorKey;

	/* initial XOR */
	xorKey = 16 * secArray[ 2 ] + secArray[ 2 ];
	for ( i = 0; i < n; ++i )
		buffer[ i ] ^= xorKey;


	/* rotate anti-clockwise through bit 7 of buffer[ 0 ] */
	for ( i = rotatePlaces - 1; i >= 0; --i )
	{
		c1 = ( buffer[ 0 ] & 0x80 ) ? 1 : 0;

		/* xor taps */
		for ( j = 0; j < n; ++j )
		{
			if ( buffer[ j ] & ( 1 << ( tapArray[ ( secArray[ 1 ] + j ) % 10 ] - 1 ) ) )
				c1 ^= 1;
		}
		
		for ( j = n - 1; j >= 0; --j )
		{
			c = ( buffer[ j ] & 0x80 ) ? 1 : 0;

			/* xor feed masher */
			if ( ( secArray[ 5 ] ^ feedMasher ) & ( 1 << ( ( i + j - 1 ) % 8 ) ) )
				c ^= 1;


			/* shift buffer left */
			buffer[ j ] = ( buffer[ j ] << 1 ) + c1;
			c1 = c;
		}
	}

	/* conditional reverse bits */
	for ( i = 0; i < n; ++i )
	{
		if ( secArray[ 3 ] & ( 1 << ( i % 4 ) ) )
		{
			/* reverse bits in buffer[ i ] */
			b = 0;

			for ( j = 0; j < 8; ++j )
			{
				if ( buffer[ i ] & ( 1 << j ) )
					b += ( 128 >> j );
			}
			
			buffer[ i ] = b;
		}
	}

	/* final XOR */
	xorKey = ~( 16 * secArray[ 0 ] + secArray[ 4 ] );

	for ( i = 0; i < n; ++i )
		buffer[ i ] ^= xorKey;

} /* tool_crypto_decrypt */

#if 0

/********************************************************
 * Function used to apply a brute force attack on a known 
 *                                 text cypher input string !
 * - according to algorithm specs: "ccTalk Encryption - Bill Validators V1.2.PDF"
 *      the key is only 6 bytes long ... and the key bytes can
 *      take values from 0 ... 9. This means that the all possible
 *      combinations are 000.000 - 999.999.
 * 
 *     (OBS:) Also some improvements can be done - because some bytes have
 *      special meaning ... but this has not been taken into consideration here
 *      ... there is still room for improvements !!!
 *
 ********************************************************/
int tool_crypto_brute_force_attack(void)
{

	/* We should base our supposition on the fact that the response of 11 data bytes length is cause by a 0x9F request : 
			"Read buffered bill events"
	   This request - if not encrypted  should look like: 0x28 x00 0x31 0x9F 0x5D
	       ... the destination and the length of the data is not encrypted - according to "ccTalk Encryption - Bill Validators V1.2.PDF".
	       - therefore the only encrypted bytes are:    [CRC-16 LSB] [Header] [CRC-16 MSB] !

	*/
	unsigned char buffer_message_encrypted_input[3] = {0xCB, 0xD8, 0xFF};
	unsigned char buffer_message_encrypted[3];


    //unsigned char buffer_message_encrypted_2[14] = {0x65, 0xF6, 0x96, 0x2B, 0x17, 0x38, 0x4F, 0x7F, 0xB1, 0x6B, 0xE6, 0x23, 0x17, 0x38};
    //unsigned char buffer_message_encrypted_2[14] = {0x4E, 0x0F, 0x56, 0x27, 0x17, 0x28, 0x5F, 0x7F, 0xB1, 0x6B, 0xE6, 0x23, 0x17, 0x38};
    unsigned char buffer_message_encrypted_2[14] = {0x52, 0x72, 0xD6, 0x2F, 0x17, 0x28, 0x4F, 0x67, 0xB1, 0x6B, 0xE6, 0x23, 0x17, 0x38};

    int i;

    tool_crypto_decrypt(buffer_message_encrypted_2, 14);

    printf("\n\n Message: ");
    for (i = 0; i< 14; i++)
    {
    	printf(" %X ", buffer_message_encrypted_2[i]);
    }


	while( (secArray[0] < 9) || (secArray[1] < 9) || (secArray[2] < 9) || ( secArray[3] < 9) || (secArray[4] < 9) || (secArray[5] < 9))
	{
		/* the buffer is always overriten by the decryption function ... */
		/* make sure is re-initialized ...                               */

	    buffer_message_encrypted[0] = buffer_message_encrypted_input[0];
	    buffer_message_encrypted[1] = buffer_message_encrypted_input[1];
	    buffer_message_encrypted[2] = buffer_message_encrypted_input[2];

	     tool_crypto_decrypt(buffer_message_encrypted, 3);
	 

	 	 /* STOP condition !!! -- the decrypted plain text is actually what we supposed that the plain text command: 0x28 x00 0x31 0x9F 0x5D */
	     if( (buffer_message_encrypted[0] == 0x31) && (buffer_message_encrypted[1] == 0x9F) && (buffer_message_encrypted[2] == 0x5D) )
	     {
	     	printf("\n\n !!! SUCCESS !!! \n\n KEY = %d %d %d %d %d %d", secArray[0], secArray[1], secArray[2], secArray[3], secArray[4], secArray[5]);

	     	break;
	     }

	     /*
	      * base do the key format limitation .... only 0...9 and only 6 bytes ... gives us 000 000 to 999 999 combinations !
	      *
	      */


		if(secArray[5] <= 9)
		{
			secArray[5] ++;	
		}     
		else
		{
			secArray[5] = 0;

			if(	secArray[4] <= 9)
			{
				secArray[4] ++;
			}
			else
			{
				secArray[4] = 0;

				if( secArray[3] <= 9)
				{
					secArray[3] ++;
				}
				else
				{
					secArray[3] = 0;

					if(secArray[2] <= 9)
					{
						secArray[2] ++;
					}
					else
					{
						secArray[2] = 0;

						if ( secArray[1] <= 9 )
						{
							secArray[1] ++;
						}
						else
						{
							secArray[1] = 0;

							if (secArray[0] <= 9)
							{
								secArray[0] ++;
							}
							else
							{
								break;

								printf(" /n/n --- no key found --- !!!!!");
							}
						}
					}
				}
			}
		
	    }


	    printf("\n KEY = %d %d %d %d %d %d", secArray[0], secArray[1], secArray[2], secArray[3], secArray[4], secArray[5]);

	}


  return 0;
} /* tool_crypto_brute_force_attack */

#endif