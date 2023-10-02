#ifndef __NVM_MANAGER_H__
#define __NVM_MANAGER_H__

#define NVM_MANAGER_DEBUG_SERIAL (0)

/*
 * Flags used by the NvM_Get_Block_Status and also internally !
 */
#define NVM_FLAG_WRITE_REQ         (0x01)
#define NVM_FLAG_READ_REQ          (0x02)
#define NVM_FLAG_ADDR_SYNC         (0x08)
#define NVM_FLAG_LAST_READ_ALL_OK  (0x40)
#define NVM_FLAG_BLOCK_DATA_EXIST  (0x80)



void NvM_Init(void);
void NvM_Main(void);

int NvM_AttachRamImage(char *ptrData, char nBlockID);

int NvM_Read_Block (unsigned char nBlockID);
int NvM_Write_Block(unsigned char nBlockID);

unsigned char NvM_Get_Block_Status(unsigned char nBlockID);
unsigned char NvM_Is_Block_Opperation_Free(unsigned char nBlockID);

void NvM_ReadAll(void);
void NvM_WriteAll(void);


#if(NVM_MANAGER_DEBUG_SERIAL == 1)
void NvM_Print_Memory_Range(unsigned int nAddrStart, unsigned int nAddrEnd, unsigned int nCountDisp);
void NvM_Fill_Memory_Range(unsigned int nAddrStart, unsigned int nAddrEnd, unsigned char nPattern);
#endif

#endif /* __NVM_MANAGER_H__ */
