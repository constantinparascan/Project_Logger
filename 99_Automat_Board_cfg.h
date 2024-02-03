#ifndef __AUTOMAT_BOARD_CFG__
#define __AUTOMAT_BOARD_CFG__


/*    - DIAGNOSTIC and DEBUG FEATURE
 *    - RESET to default ROUTINES ENABLE !
 */
#define BOARD_DIAGNOSTIC_AND_INITIALIZATION_ENABLE    (0)
#define BOARD_DIAGNOSTIC_NVM_BLOCKS_RESET_IGNORE      (1)
#define BOARD_DIAGNOSTIC_NVM_BLOCKS_RESET_ONLY_CASH   (0) 

#define BOARD_DIAGNOSTIC_SERIAL_DEBUGGING_ENABLE      (1)


/* 
 *  what board are we using ?
 *    
 *          SLOT  1 - production - CCTALK - ETH
 *          SLOT  2 - production - CCTALK - ETH
 *
 *          SLOT  3 - production - PARALLEL - SIM900
 *          SLOT  4 - production - CCTALK + PARALLEL - SIM900
 *      
 *          SLOT  5 - Dev Test Rudolf
 *
 *          SLOT  6 - production - CCTALK + PARALLEL - SIM900
 *          SLOT  7 - production - CCTALK + PARALLEL - SIM900
 *
 *          SLOT 99 - debug
 */
#define BOARD_CONFIG_SLOT (7)    /* <<<------------------------- CONFIG the Board !!! */


/* ---------------------------*/
/* APP specific details       */

#define LOG_APP_VERSION_MAJOR "2"
#define LOG_APP_VERSION_MINOR "0"
#define LOG_APP_VERSION_PATCH "5"


/*
 * Defines the PING time intervals - Alive messages sent to server by the cash machine
 */
#define LOG_APP_PING_TO_SERVER_DELAY (820)    // 820 = 15 min ... seen experimentally ... 
                                              // 60 == 66 sec (900)   /* 15 min - 1 sec decrement - 60 itter/min  * 15 min */


#define LOG_APP_SIMNR_MAX_CHAR        (10)               /* 10 chars MAX - NOT including NULL */
#define LOG_APP_TOWN_MAX_CHAR         (15)               /* 15 chars MAX - NOT including NULL */
#define LOG_APP_PLACE_MAX_CHAR        (20)               /* 20 chars MAX - NOT including NULL */
#define LOG_APP_DETAILS_MAX_CHAR      (25)               /* 25 chars MAX - NOT including NULL */
#define LOG_APP_DEVICE_TYPE_MAX_CHAR  (10)               /* 10 chars MAX - NOT including NULL */



/*
 *   GSM configurations ... 
 *
 */

#define AT_SIM_CARD_PIN_NR     "0000"



/******************************************************
 * 
 *               Board ethernet config slot 1
 *
 ******************************************************/
#if(BOARD_CONFIG_SLOT == 1) 

/*
 *  ETH Configurations 
 */

/* 
 * Defines the MAC address of the board
 */
#define MAC_BYTE_00 (0xDE)
#define MAC_BYTE_01 (0xAD)
#define MAC_BYTE_02 (0xBE)
#define MAC_BYTE_03 (0xEF)
#define MAC_BYTE_04 (0xFE)
#define MAC_BYTE_05 (0xED)     /* --- SN 1 */


/*
 *   Board IP's must be in Router adress space !!!
 */
#define IP_ADDRESS_BYTE_0 (192)
#define IP_ADDRESS_BYTE_1 (168)
#define IP_ADDRESS_BYTE_2 (33)      /* --- 1 */
#define IP_ADDRESS_BYTE_3 (177)

#define DNS_ADDRESS_BYTE_0 (192)
#define DNS_ADDRESS_BYTE_1 (168)
#define DNS_ADDRESS_BYTE_2 (33)     /* --- 1 */
#define DNS_ADDRESS_BYTE_3 (1)

/* 
 * Defines the server IPv4 address
 */
#define SERVER_ADDRESS_DESTINATION "145.239.84.165"        /* -- used in production  -- */
#define SERVER_ADDRESS_DESTINATION_PORT "35269"            /* -- used in production  -- */

#define SERVER_ADDRESS_GET_URL_COMMAND_X      "/gsm/entry_new.php?x="        /* used in production */
#define SERVER_ADDRESS_GET_URL_AVAILABILITY   "/gsm/index.php"               /* used in production */

/*
 * SIM card details
 */
#define LOG_APP_IMEI_CARD  "220131329868"         /* 20 chars MAX - including NULL  */

#define LOG_APP_SIM_SN "1111111111111111111"      /* 25 chars MAX - including NULL */
#define LOG_APP_SIM_PHONE_NR "0726734732"         /* 10 chars MAX - NOT including NULL */
#define LOG_APP_SIM_OPERATOR "RDS"                /* 15 chars MAX - including NULL */

/* Location specific details  */
#define LOG_APP_TOWN    "IASI"               /* 15 chars MAX - NOT including NULL */
#define LOG_APP_PLACE   "PALAS_IASI"         /* 20 chars MAX - NOT including NULL */
#define LOG_APP_DETAILS "PALAS_PARTER"       /* 25 chars MAX - NOT including NULL */

#define LOG_APP_DEVICE_TYPE "NV9"            /* 10  chars MAX - including NULL */
#define LOG_APP_DEFAULT_RSSI "1"             /* 2   chars MAX - including NULL */

#endif /* #if(BOARD_CONFIG_SLOT == 1) */





/******************************************************
 * 
 *               Board ethernet config slot 2
 *
 ******************************************************/
#if(BOARD_CONFIG_SLOT == 2) 

/*
 *  ETH Configurations 
 */

/* 
 * Defines the MAC address of the board
 */
#define MAC_BYTE_00 (0xDE)
#define MAC_BYTE_01 (0xAD)
#define MAC_BYTE_02 (0xBE)
#define MAC_BYTE_03 (0xEF)
#define MAC_BYTE_04 (0xFE)
#define MAC_BYTE_05 (0xEE)     /* --- SN 2 */


/*
 *   Board IP's must be in Router adress space !!!
 */
#define IP_ADDRESS_BYTE_0 (192)
#define IP_ADDRESS_BYTE_1 (168)
#define IP_ADDRESS_BYTE_2 (34)      /* --- 2 */
#define IP_ADDRESS_BYTE_3 (177)

#define DNS_ADDRESS_BYTE_0 (192)
#define DNS_ADDRESS_BYTE_1 (168)
#define DNS_ADDRESS_BYTE_2 (34)    /* --- 2 */
#define DNS_ADDRESS_BYTE_3 (1)

/* 
 * Defines the server IPv4 address
 */
#define SERVER_ADDRESS_DESTINATION "145.239.84.165"        /* -- used in production  -- */
#define SERVER_ADDRESS_DESTINATION_PORT "35269"            /* -- used in production  -- */

#define SERVER_ADDRESS_GET_URL_COMMAND_X      "/gsm/entry_new.php?x="        /* used in production */
#define SERVER_ADDRESS_GET_URL_AVAILABILITY   "/gsm/index.php"               /* used in production */

/*
 * SIM card details
 */
#define LOG_APP_IMEI_CARD  "220131329869"         /* 20 chars MAX - including NULL  */

#define LOG_APP_SIM_SN "1111111111111111111"      /* 25 chars MAX - including NULL */
#define LOG_APP_SIM_PHONE_NR "0726734732"         /* 10 chars MAX - NOT including NULL */
#define LOG_APP_SIM_OPERATOR "RDS"                /* 15 chars MAX - including NULL */

/* Location specific details  */
#define LOG_APP_TOWN    "BUC_TEST_2"               /* 15 chars MAX - NOT including NULL */
#define LOG_APP_PLACE   "TEST_2"                   /* 20 chars MAX - NOT including NULL */
#define LOG_APP_DETAILS "UNDER_TEST_2"             /* 25 chars MAX - NOT including NULL */

#define LOG_APP_DEVICE_TYPE "NV9"            /* 10  chars MAX - including NULL */
#define LOG_APP_DEFAULT_RSSI "1"             /* 2   chars MAX - including NULL */

#endif /* #if(BOARD_CONFIG_SLOT == 2) */




/******************************************************
 * 
 *               Board SIM900 config slot 3
 *
 ******************************************************/
 /*
  *
  * --- using SIM900 communication module ---
  *
  */
#if(BOARD_CONFIG_SLOT == 3) 

/*
 *  ETH Configurations 
 */

/* 
 * Defines the MAC address of the board
 */
#define MAC_BYTE_00 (0xDE)
#define MAC_BYTE_01 (0xAD)
#define MAC_BYTE_02 (0xBE)
#define MAC_BYTE_03 (0xEF)
#define MAC_BYTE_04 (0xFE)
#define MAC_BYTE_05 (0x00)     /* --- SN 3 */


/*
 *   Board IP's must be in Router adress space !!!
 */
#define IP_ADDRESS_BYTE_0 (192)
#define IP_ADDRESS_BYTE_1 (168)
#define IP_ADDRESS_BYTE_2 (34)      /* --- 3 */
#define IP_ADDRESS_BYTE_3 (177)

#define DNS_ADDRESS_BYTE_0 (192)
#define DNS_ADDRESS_BYTE_1 (168)
#define DNS_ADDRESS_BYTE_2 (34)    /* --- 3 */
#define DNS_ADDRESS_BYTE_3 (1)

/* 
 * Defines the server IPv4 address
 */
#define SERVER_ADDRESS_DESTINATION "145.239.84.165"        /* -- used in production  -- */
#define SERVER_ADDRESS_DESTINATION_PORT "35269"            /* -- used in production  -- */

#define SERVER_ADDRESS_GET_URL_COMMAND_X      "/gsm/entry_new.php?x="        /* used in production */
#define SERVER_ADDRESS_GET_URL_AVAILABILITY   "/gsm/index.php"               /* used in production */


/*
 * SIM card details
 */
#define LOG_APP_IMEI_CARD  "220131329877"         /* 20 chars MAX - including NULL  */

#define LOG_APP_SIM_SN "1111111111111111111"      /* 25 chars MAX - including NULL */
#define LOG_APP_SIM_PHONE_NR "0726734732"         /* 10 chars MAX - NOT including NULL */
#define LOG_APP_SIM_OPERATOR "ORANGE"             /* 15 chars MAX - including NULL */

/* Location specific details  */
#define LOG_APP_TOWN    "BUC2_TEST_3"               /* 15 chars MAX - NOT including NULL */
#define LOG_APP_PLACE   "TEST_3"                   /* 20 chars MAX - NOT including NULL */
#define LOG_APP_DETAILS "UNDER_TEST_3"             /* 25 chars MAX - NOT including NULL */

#define LOG_APP_DEVICE_TYPE "NV10"            /* 10  chars MAX - including NULL */
#define LOG_APP_DEFAULT_RSSI "1"              /* 2   chars MAX - including NULL */

#endif /* #if(BOARD_CONFIG_SLOT == 3) */



/******************************************************
 * 
 *               Board SIM900 config slot 4
 *
 ******************************************************/
 /*
  *
  * --- using SIM900 communication module ---
  *
  */
#if(BOARD_CONFIG_SLOT == 4) 

/*
 *  ETH Configurations 
 */

/* 
 * Defines the MAC address of the board
 */
#define MAC_BYTE_00 (0xDE)
#define MAC_BYTE_01 (0xAD)
#define MAC_BYTE_02 (0xBE)
#define MAC_BYTE_03 (0xEF)
#define MAC_BYTE_04 (0xFE)
#define MAC_BYTE_05 (0x00)     /* --- SN 3 */


/*
 *   Board IP's must be in Router adress space !!!
 */
#define IP_ADDRESS_BYTE_0 (192)
#define IP_ADDRESS_BYTE_1 (168)
#define IP_ADDRESS_BYTE_2 (34)      /* --- 3 */
#define IP_ADDRESS_BYTE_3 (177)

#define DNS_ADDRESS_BYTE_0 (192)
#define DNS_ADDRESS_BYTE_1 (168)
#define DNS_ADDRESS_BYTE_2 (34)     /* --- 3 */
#define DNS_ADDRESS_BYTE_3 (1)

/* 
 * Defines the server IPv4 address
 */
#define SERVER_ADDRESS_DESTINATION "145.239.84.165"        /* -- used in production  -- */
#define SERVER_ADDRESS_DESTINATION_PORT "35269"            /* -- used in production  -- */

#define SERVER_ADDRESS_GET_URL_COMMAND_X      "/gsm/entry_new.php?x="        /* used in production */
#define SERVER_ADDRESS_GET_URL_AVAILABILITY   "/gsm/index.php"               /* used in production */


/*
 * SIM card details
 */
#define LOG_APP_IMEI_CARD  "220201750827"         /* 20 chars MAX - including NULL  */

#define LOG_APP_SIM_SN "1111111111111111112"      /* 25 chars MAX - including NULL */
#define LOG_APP_SIM_PHONE_NR "0726734732"         /* 10 chars MAX - NOT including NULL */
#define LOG_APP_SIM_OPERATOR "ORANGE"                /* 15 chars MAX - including NULL */

/* Location specific details  */
#define LOG_APP_TOWN    "BUC_TEST_4"               /* 15 chars MAX - NOT including NULL */
#define LOG_APP_PLACE   "TEST_4"                   /* 20 chars MAX - NOT including NULL */
#define LOG_APP_DETAILS "UNDER_TEST_4"             /* 25 chars MAX - NOT including NULL */

#define LOG_APP_DEVICE_TYPE "NV10"            /* 10  chars MAX - including NULL */
#define LOG_APP_DEFAULT_RSSI "1"              /* 2   chars MAX - including NULL */

#endif /* #if(BOARD_CONFIG_SLOT == 4) */


/******************************************************
 * 
 *               Board SIM900 config slot 5 - RUDOLF
 *
 ******************************************************/
 /*
  *
  * --- using SIM900 communication module ---
  *
  */
#if(BOARD_CONFIG_SLOT == 5) 

/*
 *  ETH Configurations 
 */

/* 
 * Defines the MAC address of the board
 */
#define MAC_BYTE_00 (0xDE)
#define MAC_BYTE_01 (0xAD)
#define MAC_BYTE_02 (0xBE)
#define MAC_BYTE_03 (0xEF)
#define MAC_BYTE_04 (0xFE)
#define MAC_BYTE_05 (0x00)     /* --- SN 3 */


/*
 *   Board IP's must be in Router adress space !!!
 */
#define IP_ADDRESS_BYTE_0 (192)
#define IP_ADDRESS_BYTE_1 (168)
#define IP_ADDRESS_BYTE_2 (34)      /* --- 3 */
#define IP_ADDRESS_BYTE_3 (177)

#define DNS_ADDRESS_BYTE_0 (192)
#define DNS_ADDRESS_BYTE_1 (168)
#define DNS_ADDRESS_BYTE_2 (34)     /* --- 3 */
#define DNS_ADDRESS_BYTE_3 (1)

/* 
 * Defines the server IPv4 address
 */
#define SERVER_ADDRESS_DESTINATION "145.239.84.165"        /* -- used in production  -- */
#define SERVER_ADDRESS_DESTINATION_PORT "35269"            /* -- used in production  -- */

#define SERVER_ADDRESS_GET_URL_COMMAND_X      "/gsm/entry_new.php?x="        /* used in production */
#define SERVER_ADDRESS_GET_URL_AVAILABILITY   "/gsm/index.php"               /* used in production */


/*
 * SIM card details
 */
#define LOG_APP_IMEI_CARD  "220201750828"         /* 20 chars MAX - including NULL  */

#define LOG_APP_SIM_SN "1111111111111111112"      /* 25 chars MAX - including NULL */
#define LOG_APP_SIM_PHONE_NR "0726734732"         /* 10 chars MAX - NOT including NULL */
#define LOG_APP_SIM_OPERATOR "ORANGE"                /* 15 chars MAX - including NULL */

/* Location specific details  */
#define LOG_APP_TOWN    "BUC_TEST_5"               /* 15 chars MAX - NOT including NULL */
#define LOG_APP_PLACE   "TEST_5"                   /* 20 chars MAX - NOT including NULL */
#define LOG_APP_DETAILS "UNDER_TEST_5"             /* 25 chars MAX - NOT including NULL */

#define LOG_APP_DEVICE_TYPE "NV10"            /* 10  chars MAX - including NULL */
#define LOG_APP_DEFAULT_RSSI "1"              /* 2   chars MAX - including NULL */

#endif /* #if(BOARD_CONFIG_SLOT == 5) */



/******************************************************
 * 
 *               Board SIM900 config slot 6
 *
 ******************************************************/
 /*
  *
  * --- using SIM900 communication module ---
  *
  */
#if(BOARD_CONFIG_SLOT == 6) 

/*
 *  ETH Configurations 
 */

/* 
 * Defines the MAC address of the board
 */
#define MAC_BYTE_00 (0xDE)
#define MAC_BYTE_01 (0xAD)
#define MAC_BYTE_02 (0xBE)
#define MAC_BYTE_03 (0xEF)
#define MAC_BYTE_04 (0xFE)
#define MAC_BYTE_05 (0x00)     /* --- SN 3 */


/*
 *   Board IP's must be in Router adress space !!!
 */
#define IP_ADDRESS_BYTE_0 (192)
#define IP_ADDRESS_BYTE_1 (168)
#define IP_ADDRESS_BYTE_2 (34)      /* --- 3 */
#define IP_ADDRESS_BYTE_3 (177)

#define DNS_ADDRESS_BYTE_0 (192)
#define DNS_ADDRESS_BYTE_1 (168)
#define DNS_ADDRESS_BYTE_2 (34)     /* --- 3 */
#define DNS_ADDRESS_BYTE_3 (1)

/* 
 * Defines the server IPv4 address
 */
#define SERVER_ADDRESS_DESTINATION "145.239.84.165"        /* -- used in production  -- */
#define SERVER_ADDRESS_DESTINATION_PORT "35269"            /* -- used in production  -- */

#define SERVER_ADDRESS_GET_URL_COMMAND_X      "/gsm/entry_new.php?x="        /* used in production */
#define SERVER_ADDRESS_GET_URL_AVAILABILITY   "/gsm/index.php"               /* used in production */


/*
 * SIM card details
 */
#define LOG_APP_IMEI_CARD  "220201750829"         /* 20 chars MAX - including NULL  */

#define LOG_APP_SIM_SN "1111111111111111112"      /* 25 chars MAX - including NULL */
#define LOG_APP_SIM_PHONE_NR "0726734732"         /* 10 chars MAX - NOT including NULL */
#define LOG_APP_SIM_OPERATOR "ORANGE"                /* 15 chars MAX - including NULL */

/* Location specific details  */
#define LOG_APP_TOWN    "BUC_TEST_6"               /* 15 chars MAX - NOT including NULL */
#define LOG_APP_PLACE   "TEST_6"                   /* 20 chars MAX - NOT including NULL */
#define LOG_APP_DETAILS "UNDER_TEST_6"             /* 25 chars MAX - NOT including NULL */

#define LOG_APP_DEVICE_TYPE "NV10"            /* 10  chars MAX - including NULL */
#define LOG_APP_DEFAULT_RSSI "1"              /* 2   chars MAX - including NULL */

#endif /* #if(BOARD_CONFIG_SLOT == 6) */



/******************************************************
 * 
 *               Board SIM900 config slot 7
 *
 ******************************************************/
 /*
  *
  * --- using SIM900 communication module ---
  *
  */
#if(BOARD_CONFIG_SLOT == 7) 

/*
 *  ETH Configurations 
 */

/* 
 * Defines the MAC address of the board
 */
#define MAC_BYTE_00 (0xDE)
#define MAC_BYTE_01 (0xAD)
#define MAC_BYTE_02 (0xBE)
#define MAC_BYTE_03 (0xEF)
#define MAC_BYTE_04 (0xFE)
#define MAC_BYTE_05 (0x00)     /* --- SN 3 */


/*
 *   Board IP's must be in Router adress space !!!
 */
#define IP_ADDRESS_BYTE_0 (192)
#define IP_ADDRESS_BYTE_1 (168)
#define IP_ADDRESS_BYTE_2 (34)      /* --- 3 */
#define IP_ADDRESS_BYTE_3 (177)

#define DNS_ADDRESS_BYTE_0 (192)
#define DNS_ADDRESS_BYTE_1 (168)
#define DNS_ADDRESS_BYTE_2 (34)     /* --- 3 */
#define DNS_ADDRESS_BYTE_3 (1)

/* 
 * Defines the server IPv4 address
 */
#define SERVER_ADDRESS_DESTINATION "145.239.84.165"        /* -- used in production  -- */
#define SERVER_ADDRESS_DESTINATION_PORT "35269"            /* -- used in production  -- */

#define SERVER_ADDRESS_GET_URL_COMMAND_X      "/gsm/entry_new.php?x="        /* used in production */
#define SERVER_ADDRESS_GET_URL_AVAILABILITY   "/gsm/index.php"               /* used in production */


/*
 * SIM card details
 */
#define LOG_APP_IMEI_CARD  "220201750830"         /* 20 chars MAX - including NULL  */

#define LOG_APP_SIM_SN "1111111111111111112"      /* 25 chars MAX - including NULL */
#define LOG_APP_SIM_PHONE_NR "0726734732"         /* 10 chars MAX - NOT including NULL */
#define LOG_APP_SIM_OPERATOR "ORANGE"                /* 15 chars MAX - including NULL */

/* Location specific details  */
#define LOG_APP_TOWN    "BUC_TEST_7"               /* 15 chars MAX - NOT including NULL */
#define LOG_APP_PLACE   "TEST_7"                   /* 20 chars MAX - NOT including NULL */
#define LOG_APP_DETAILS "UNDER_TEST_7"             /* 25 chars MAX - NOT including NULL */

#define LOG_APP_DEVICE_TYPE "NV10"            /* 10  chars MAX - including NULL */
#define LOG_APP_DEFAULT_RSSI "1"              /* 2   chars MAX - including NULL */

#endif /* #if(BOARD_CONFIG_SLOT == 7) */





/******************************************************
 * 
 *               Board ethernet config slot 99 - DEBUG
 *
 ******************************************************/
#if(BOARD_CONFIG_SLOT == 99) 

/*
 *  ETH Configurations 
 */

/* 
 * Defines the MAC address of the board
 */
#define MAC_BYTE_00 (0xDE)
#define MAC_BYTE_01 (0xAD)
#define MAC_BYTE_02 (0xBE)
#define MAC_BYTE_03 (0xEF)
#define MAC_BYTE_04 (0xFE)
#define MAC_BYTE_05 (0xED)


/*
 *   Board IP's must be in Router adress space !!!
 */
/*  Used with the local router */
#define IP_ADDRESS_BYTE_0 (192)
#define IP_ADDRESS_BYTE_1 (168)
#define IP_ADDRESS_BYTE_2 (0)
#define IP_ADDRESS_BYTE_3 (107)

#define DNS_ADDRESS_BYTE_0 (192)
#define DNS_ADDRESS_BYTE_1 (168)
#define DNS_ADDRESS_BYTE_2 (0)
#define DNS_ADDRESS_BYTE_3 (1)

/* 
 * Defines the server IPv4 address
 */
//#define SERVER_ADDRESS_DESTINATION "79.112.87.94"          /* -- used for tests with external connection  -- */
//#define SERVER_ADDRESS_DESTINATION_PORT (7777)             /* -- used for tests with external connection  -- */

#define SERVER_ADDRESS_DESTINATION "192.168.0.109"       /* -- used for local tests -- */
#define SERVER_ADDRESS_DESTINATION_PORT "80"             /* -- used for local tests -- */

#define SERVER_ADDRESS_GET_URL_COMMAND_X      "/entry_new.php?x="        /* used for local tests */
#define SERVER_ADDRESS_GET_URL_AVAILABILITY   "/index.php"               /* used in production */

/*
 * SIM card details
 */
#define LOG_APP_IMEI_CARD  "862462030142276"      /* 20 chars MAX - including NULL -- used for local tests - absolete ! */

#define LOG_APP_SIM_SN "1111111111111111111"      /* 25 chars MAX - including NULL */
#define LOG_APP_SIM_PHONE_NR "0726734732"         /* 10 chars MAX - NOT including NULL */
#define LOG_APP_SIM_OPERATOR "RDS"                /* 15 chars MAX - including NULL */

/* Location specific details  */
#define LOG_APP_TOWN    "IASI_DEBUG"               /* 15 chars MAX - NOT including NULL */
#define LOG_APP_PLACE   "PALAS_IASI_DEBUG"         /* 20 chars MAX - NOT including NULL */
#define LOG_APP_DETAILS "PALAS_PARTER_DEBUG"       /* 25 chars MAX - NOT including NULL */

#define LOG_APP_DEVICE_TYPE "NV9"            /* 10  chars MAX - including NULL */
#define LOG_APP_DEFAULT_RSSI "1"             /* 2   chars MAX - including NULL */

#endif /* #if(BOARD_CONFIG_SLOT == 99) */




#endif /* __AUTOMAT_BOARD_CFG__ */