#ifndef __EEPROM_DRIVER_H__
#define __EEPROM_DRIVER_H__

#define EEPROM_DRIVER_DEBUG_ENABLE (0)

/* Exported API's*/
void EEPROM_driver_init(void);
void EEPROM_driver_main(void);

unsigned char EEPROM_driver_Write_Req(unsigned int nAddr, unsigned char *srcData, unsigned char nLen);
unsigned char EEPROM_driver_Write_Byte_Fast(unsigned int nAddr, unsigned char srcData);
unsigned char EEPROM_driver_Is_Write_Finished(void);

unsigned char EEPROM_driver_Read_Req(unsigned int nAddr, unsigned char nLen);
unsigned char EEPROM_driver_Forced_Read_NoWait(unsigned int uiAddress);
unsigned char EEPROM_driver_Is_Read_Finished(void);
void EEPROM_driver_Get_Read_Buffer_Content(unsigned char *destData, unsigned char nLen);

#if(EEPROM_DRIVER_DEBUG_ENABLE == 1)
void EEPROM_driver_Debug_Write_Pattern_1(void);
void EEPROM_driver_Debug_Write_Pattern_2(void);
void EEPROM_driver_Debug_Read_Pattern(void);
void EEPROM_driver_Debug_Print_Read_Pattern(void);
#endif


#endif /* __EEPROM_DRIVER_H__ */