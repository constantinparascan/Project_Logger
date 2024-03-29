#include "09_NvM_Manager.h"
#include "09_NvM_Manager_cfg.h"

/* 
 *   !! WARNING !!
 *       NO block shall exceed 254 bytes in length !!!!
 */


/* ---- Block 0 ---- */
#define NVM_BLOCK_NVM_PRIVATE_DATA_LEN (3)            /* 3 bytes NOT including 1 byte - Flags                */
#define NVM_BLOCK_NVM_PRIVATE_ADDRESS_PTR (0)         /* EEPROM start address                                */
#define NVM_BLOCK_NVM_PRIVATE_DATA_MULTIPLY (1)       /* how many duplicates for this block exists in memory */

/* ---- Block 1 ---- */
#define NVM_BLOCK_CHAN_1_DATA_LEN (2)           /* 2 bytes NOT including 1 byte - Flags - for each entry  */
#define NVM_BLOCK_CHAN_1_ADDRESS_PTR (4)        /* EEPROM start address                                   */
#define NVM_BLOCK_CHAN_1_DATA_MULTIPLY (10)     /* how many duplicates for this block exists in memory    */

/* ---- Block 2 ---- */
#define NVM_BLOCK_CHAN_2_DATA_LEN (2)
#define NVM_BLOCK_CHAN_2_ADDRESS_PTR (34)
#define NVM_BLOCK_CHAN_2_DATA_MULTIPLY (10)

/* ---- Block 3 ---- */
#define NVM_BLOCK_CHAN_3_DATA_LEN (2)
#define NVM_BLOCK_CHAN_3_ADDRESS_PTR (64)
#define NVM_BLOCK_CHAN_3_DATA_MULTIPLY (10)

/* ---- Block 4 ---- */
#define NVM_BLOCK_CHAN_4_DATA_LEN (2)
#define NVM_BLOCK_CHAN_4_ADDRESS_PTR (94)
#define NVM_BLOCK_CHAN_4_DATA_MULTIPLY (10)

/* ---- Block 5 ---- */
#define NVM_BLOCK_CHAN_5_DATA_LEN (2)
#define NVM_BLOCK_CHAN_5_ADDRESS_PTR (124)
#define NVM_BLOCK_CHAN_5_DATA_MULTIPLY (10)


/* ---- Block 6 ---- -- Telephone number -- */
#define NVM_BLOCK_CHAN_6_DATA_LEN (10)
#define NVM_BLOCK_CHAN_6_ADDRESS_PTR (200)
#define NVM_BLOCK_CHAN_6_DATA_MULTIPLY (1)


/* ---- Block 7 ---- -- Town --             */
#define NVM_BLOCK_CHAN_7_DATA_LEN (15)
#define NVM_BLOCK_CHAN_7_ADDRESS_PTR (230)
#define NVM_BLOCK_CHAN_7_DATA_MULTIPLY (1)

/* ---- Block 8 ---- -- Place --            */
#define NVM_BLOCK_CHAN_8_DATA_LEN (20)
#define NVM_BLOCK_CHAN_8_ADDRESS_PTR (260)
#define NVM_BLOCK_CHAN_8_DATA_MULTIPLY (1)

/* ---- Block 9 ---- -- Details --          */
#define NVM_BLOCK_CHAN_9_DATA_LEN (25)
#define NVM_BLOCK_CHAN_9_ADDRESS_PTR (300)
#define NVM_BLOCK_CHAN_9_DATA_MULTIPLY (1)

/* ---- Block 10 ---- -- Device Type     -- */
#define NVM_BLOCK_CHAN_10_DATA_LEN (10)
#define NVM_BLOCK_CHAN_10_ADDRESS_PTR (350)
#define NVM_BLOCK_CHAN_10_DATA_MULTIPLY (1)



#define NVM_EMPTY_FLAGS     (0)
#define NVM_NULL_RAM_MIRROR (0)




T_NvM_Block_Header arrNvM_BlockDef[NVM_BLOCK_COUNT] =
{
  {
   NVM_BLOCK_NVM_PRIVATE_DATA_LEN,
   NVM_NULL_RAM_MIRROR,
   NVM_BLOCK_NVM_PRIVATE_ADDRESS_PTR, 
   NVM_BLOCK_NVM_PRIVATE_ADDRESS_PTR, 
   NVM_BLOCK_NVM_PRIVATE_DATA_MULTIPLY,   
   NVM_EMPTY_FLAGS
   },

  {
   NVM_BLOCK_CHAN_1_DATA_LEN,
   NVM_NULL_RAM_MIRROR,
   NVM_BLOCK_CHAN_1_ADDRESS_PTR, 
   NVM_BLOCK_CHAN_1_ADDRESS_PTR, 
   NVM_BLOCK_CHAN_1_DATA_MULTIPLY,
   NVM_EMPTY_FLAGS
   },

  {
   NVM_BLOCK_CHAN_2_DATA_LEN,
   NVM_NULL_RAM_MIRROR,
   NVM_BLOCK_CHAN_2_ADDRESS_PTR,
   NVM_BLOCK_CHAN_2_ADDRESS_PTR,
   NVM_BLOCK_CHAN_2_DATA_MULTIPLY,
   NVM_EMPTY_FLAGS
   },

  {
   NVM_BLOCK_CHAN_3_DATA_LEN,
   NVM_NULL_RAM_MIRROR,
   NVM_BLOCK_CHAN_3_ADDRESS_PTR,
   NVM_BLOCK_CHAN_3_ADDRESS_PTR,
   NVM_BLOCK_CHAN_3_DATA_MULTIPLY,
   NVM_EMPTY_FLAGS
   },

  {
   NVM_BLOCK_CHAN_4_DATA_LEN,
   NVM_NULL_RAM_MIRROR,
   NVM_BLOCK_CHAN_4_ADDRESS_PTR,
   NVM_BLOCK_CHAN_4_ADDRESS_PTR,
   NVM_BLOCK_CHAN_4_DATA_MULTIPLY,
   NVM_EMPTY_FLAGS
   },

  {
   NVM_BLOCK_CHAN_5_DATA_LEN,
   NVM_NULL_RAM_MIRROR,
   NVM_BLOCK_CHAN_5_ADDRESS_PTR,
   NVM_BLOCK_CHAN_5_ADDRESS_PTR,
   NVM_BLOCK_CHAN_5_DATA_MULTIPLY,
   NVM_EMPTY_FLAGS
   },

  {
   NVM_BLOCK_CHAN_6_DATA_LEN,
   NVM_NULL_RAM_MIRROR,
   NVM_BLOCK_CHAN_6_ADDRESS_PTR,
   NVM_BLOCK_CHAN_6_ADDRESS_PTR,
   NVM_BLOCK_CHAN_6_DATA_MULTIPLY,
   NVM_EMPTY_FLAGS
   },

  {
   NVM_BLOCK_CHAN_7_DATA_LEN,
   NVM_NULL_RAM_MIRROR,
   NVM_BLOCK_CHAN_7_ADDRESS_PTR,
   NVM_BLOCK_CHAN_7_ADDRESS_PTR,
   NVM_BLOCK_CHAN_7_DATA_MULTIPLY,
   NVM_EMPTY_FLAGS
   },
   
  {
   NVM_BLOCK_CHAN_8_DATA_LEN,
   NVM_NULL_RAM_MIRROR,
   NVM_BLOCK_CHAN_8_ADDRESS_PTR,
   NVM_BLOCK_CHAN_8_ADDRESS_PTR,
   NVM_BLOCK_CHAN_8_DATA_MULTIPLY,
   NVM_EMPTY_FLAGS
   },

  {
   NVM_BLOCK_CHAN_9_DATA_LEN,
   NVM_NULL_RAM_MIRROR,
   NVM_BLOCK_CHAN_9_ADDRESS_PTR,
   NVM_BLOCK_CHAN_9_ADDRESS_PTR,
   NVM_BLOCK_CHAN_9_DATA_MULTIPLY,
   NVM_EMPTY_FLAGS
   },

  {
   NVM_BLOCK_CHAN_10_DATA_LEN,
   NVM_NULL_RAM_MIRROR,
   NVM_BLOCK_CHAN_10_ADDRESS_PTR,
   NVM_BLOCK_CHAN_10_ADDRESS_PTR,
   NVM_BLOCK_CHAN_10_DATA_MULTIPLY,
   NVM_EMPTY_FLAGS
   }

};

