#ifndef __IDEBUG_H
#define __IDEBUG_H

#ifdef __cplusplus
 extern "C" {
#endif
   
/*The following program is modified by the user according to the hardware device, otherwise the driver cannot run.*/

/* Includes ------------------------------------------------------------------*/
#include "stm32f4xx_hal.h"
#include "stdio.h"

extern int dbg_level;

#define RELEASE_VERSION       0

#define DEBUG_LEVEL_INFO      0x01
#define DEBUG_LEVEL_DBG       0x02
#define DEBUG_LEVEL_ERR       0x04

#if RELEASE_VERSION
#undef DEBUG
#endif

#ifdef DEBUG

#define OS_TIME_MS()    0

/*The above procedure is modified by the user according to the hardware device, otherwise the driver cannot run.*/

#define ERR_PRINT_TIME  printf("[E: %d.%03d] ",  OS_TIME_MS()/1000, OS_TIME_MS()%1000)
#define DBG_PRINT_TIME  printf("[E: %d.%03d] ",  OS_TIME_MS()/1000, OS_TIME_MS()%1000)
#define p_info(...)     do{if(!(dbg_level & DEBUG_LEVEL_INFO))break;printf("[I: %d.%03d] ",  OS_TIME_MS()/1000, OS_TIME_MS()%1000);printf(__VA_ARGS__); printf("\r\n");}while(0)
#define p_err(...)      do{if(!(dbg_level & DEBUG_LEVEL_ERR))break;printf("[E: %d.%03d] ",  OS_TIME_MS()/1000, OS_TIME_MS()%1000);printf(__VA_ARGS__); printf("\r\n");}while(0)
#define p_dbg_track     do{if(!(dbg_level & DEBUG_LEVEL_DBG))break;printf("[D: %d.%03d] ",  OS_TIME_MS()/1000, OS_TIME_MS()%1000);printf("%s,%d",  __FUNCTION__, __LINE__ ); printf("\r\n");}while(0)
#define p_dbg(...)      do{if(!(dbg_level & DEBUG_LEVEL_DBG))break;printf("[D: %d.%03d] ",  OS_TIME_MS()/1000, OS_TIME_MS()%1000);printf(__VA_ARGS__); printf("\r\n");}while(0)
#define p_dbg_enter     do{if(!(dbg_level & DEBUG_LEVEL_DBG))break;printf("[D: %d.%03d] ",  OS_TIME_MS()/1000, OS_TIME_MS()%1000);printf("enter %s\n", __FUNCTION__); printf("\r\n");}while(0)
#define p_dbg_exit      do{if(!(dbg_level & DEBUG_LEVEL_DBG))break;printf("[D: %d.%03d] ",  OS_TIME_MS()/1000, OS_TIME_MS()%1000);printf("exit %s\n", __FUNCTION__); printf("\r\n");}while(0)
#define p_dbg_status    do{if(!(dbg_level & DEBUG_LEVEL_DBG))break;printf("[D: %d.%03d] ",  OS_TIME_MS()/1000, OS_TIME_MS()%1000);printf("status %d\n", status); printf("\r\n");}while(0)
#define p_err_miss      do{if(!(dbg_level & DEBUG_LEVEL_ERR))break;printf("[E: %d.%03d] ",  OS_TIME_MS()/1000, OS_TIME_MS()%1000);printf("%s miss\n", __FUNCTION__); printf("\r\n");}while(0)
#define p_err_mem       do{if(!(dbg_level & DEBUG_LEVEL_ERR))break;printf("[E: %d.%03d] ",  OS_TIME_MS()/1000, OS_TIME_MS()%1000);printf("%s mem err\n", __FUNCTION__); printf("\r\n");}while(0)
#define p_err_fun       do{if(!(dbg_level & DEBUG_LEVEL_ERR))break;printf("[E: %d.%03d] ",  OS_TIME_MS()/1000, OS_TIME_MS()%1000);printf("%s err in %d\n", __FUNCTION__, __LINE__); printf("\r\n");}while(0)
#define dump_hex(tag, buff, size) do{  \
    int dump_hex_i;\
    if(!(dbg_level & DEBUG_LEVEL_INFO))break;\
    printf("%s[", tag);\
    for (dump_hex_i = 0; dump_hex_i < size; dump_hex_i++)\
    {\
        printf("%02x ", ((char*)buff)[dump_hex_i]);\
    }\
    printf("]\r\n");\
}while(0)

#define p_hex(X, Y) dump_hex("", (unsigned char*)X, Y)

#define assert(x) \
    do { \
        if (!(x)) { \
           p_err( "%s:%d assert failed\r\n", __FILE__, __LINE__); \
           while(1); \
        } \
    } while(0)


#else
#define p_info(...)
#define p_err(...)
#define p_err_miss
#define p_err_mem
#define p_err_fun

#define p_dbg_track
#define p_dbg(...)
#define p_dbg_exit
#define p_dbg_enter
#define p_dbg_status
#define dump_hex(tag, buff, size)

#define p_hex(X, Y)

#define assert(x)
#endif



#ifdef __cplusplus
}
#endif

#endif /* __IDEBUG_H */
