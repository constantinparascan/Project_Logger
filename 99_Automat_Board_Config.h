#ifndef __AUTOMAT_BOARD_CONFIG_H__
#define __AUTOMAT_BOARD_CONFIG_H__

#define AUTOMAT_BOARD_CONFIG_DEBUG_ENABLE (0)


#define AUTOMAT_BOARD_CONFIG_HW_TYPE_PARALLEL (0x03)
#define AUTOMAT_BOARD_CONFIG_HW_TYPE_CCTALK   (0x02)
#define AUTOMAT_BOARD_CONFIG_HW_TYPE_NOT_USED (0x01)

/*
 * Exported API's related to HW configuration
 */
unsigned char Board_Config_Get_HW_ConfigType(void);


#endif /* __AUTOMAT_BOARD_CONFIG_H__ */