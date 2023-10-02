#ifndef __NVM_MANAGER_CFG_H__
#define __NVM_MANAGER_CFG_H__

/* Total number of NvM blocks ... 
 */
#define NVM_BLOCK_COUNT (11)

#define NVM_EEPROM_WRITE_BYTES_IN_ONE_CYCLE (1)
#define NVM_EEPROM_READ_BYTES_IN_ONE_CYCLE (4)

/*
 * NVM block definitions !!
 */

/* ---- Block 0 ---- */
#define NVM_BLOCK_NVM_PRIVATE_ID  (0)

/* ---- Block 1 ---- */
#define NVM_BLOCK_CHAN_1_ID       (1)

/* ---- Block 2 ---- */
#define NVM_BLOCK_CHAN_2_ID       (2)

/* ---- Block 3 ---- */
#define NVM_BLOCK_CHAN_3_ID       (3)

/* ---- Block 4 ---- */
#define NVM_BLOCK_CHAN_4_ID       (4)

/* ---- Block 5 ---- */
#define NVM_BLOCK_CHAN_5_ID       (5)


/* ---- Block 6 ----    Telephone number */
#define NVM_BLOCK_CHAN_6_ID       (6)

/* ---- Block 7 ----    Town             */
#define NVM_BLOCK_CHAN_7_ID       (7)

/* ---- Block 8 ----    Place            */
#define NVM_BLOCK_CHAN_8_ID       (8)

/* ---- Block 9 ----    Details          */
#define NVM_BLOCK_CHAN_9_ID       (9)

/* ---- Block 10 ----    Devide ID       */
#define NVM_BLOCK_CHAN_10_ID      (10)



#endif /* __NVM_MANAGER_CFG_H__ */