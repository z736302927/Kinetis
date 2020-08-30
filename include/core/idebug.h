#ifndef __IDEBUG_H
#define __IDEBUG_H

#ifdef __cplusplus
extern "C" {
#endif

/*The following program is modified by the user according to the hardware device, otherwise the driver cannot run.*/

/* Includes ------------------------------------------------------------------*/
#include "core_common.h"

#define RELEASE_VERSION       0
#define DEBUG

#define KERN_DEFAULT	7	/* the default kernel loglevel */

/*The above procedure is modified by the user according to the hardware device, otherwise the driver cannot run.*/

#define KERN_EMERG	    0	/* system is unusable */
#define KERN_ALERT	    1	/* action must be taken immediately */
#define KERN_CRIT	    2	/* critical conditions */
#define KERN_ERR	    3	/* error conditions */
#define KERN_WARNING	4	/* warning conditions */
#define KERN_NOTICE	    5	/* normal but significant condition */
#define KERN_INFO	    6	/* informational */
#define KERN_DEBUG	    7	/* debug-level messages */

#if RELEASE_VERSION
#undef DEBUG
#endif

#ifdef DEBUG
void kinetis_debug_trace(int dbg_level, const char *format, ...);
void kinetis_dump_buffer(void *Buffer, int Size);

#define ERR_PRINT_TIME  printf("[%05d.%06d] ", BasicTimer_GetSSTick(), BasicTimer_GetUSTick())
#define DBG_PRINT_TIME  printf("[%05d.%06d] ", BasicTimer_GetSSTick(), BasicTimer_GetUSTick())
#define kinetis_info(...)     do{if(!(KERN_DEFAULT >= KERN_INFO))break;DBG_PRINT_TIME;printf(__VA_ARGS__); printf("\r\n");}while(0)
#define kinetis_err(...)      do{if(!(KERN_DEFAULT >= KERN_ERR))break;DBG_PRINT_TIME;printf(__VA_ARGS__); printf("\r\n");}while(0)
#define kinetis_dbg_track     do{if(!(KERN_DEFAULT >= KERN_DEBUG))break;DBG_PRINT_TIME;printf("%s,%d",  __FUNCTION__, __LINE__ ); printf("\r\n");}while(0)
#define kinetis_dbg(...)      do{if(!(KERN_DEFAULT >= KERN_DEBUG))break;DBG_PRINT_TIME;printf(__VA_ARGS__); printf("\r\n");}while(0)
#define kinetis_dbg_enter     do{if(!(KERN_DEFAULT >= KERN_DEBUG))break;DBG_PRINT_TIME;printf("enter %s\n", __FUNCTION__); printf("\r\n");}while(0)
#define kinetis_dbg_exit      do{if(!(KERN_DEFAULT >= KERN_DEBUG))break;DBG_PRINT_TIME;printf("exit %s\n", __FUNCTION__); printf("\r\n");}while(0)
#define kinetis_dbg_status    do{if(!(KERN_DEFAULT >= KERN_DEBUG))break;DBG_PRINT_TIME;printf("status %d\n", status); printf("\r\n");}while(0)
#define kinetis_err_miss      do{if(!(KERN_DEFAULT >= KERN_ERR))break;ERR_PRINT_TIME;printf("%s miss\n", __FUNCTION__); printf("\r\n");}while(0)
#define kinetis_err_mem       do{if(!(KERN_DEFAULT >= KERN_ERR))break;ERR_PRINT_TIME;printf("%s mem err\n", __FUNCTION__); printf("\r\n");}while(0)
#define kinetis_err_fun       do{if(!(KERN_DEFAULT >= KERN_ERR))break;ERR_PRINT_TIME;printf("%s err in %d\n", __FUNCTION__, __LINE__); printf("\r\n");}while(0)
#define kinetis_assert(x)     do{if(!(x)){ kinetis_err( "%s:%d assert failed\r\n", __FILE__, __LINE__);while(1);}} while(0)
#else
static inline void kinetis_debug_trace(int dbg_level, const char *format, ...)
{

}

static inline void kinetis_dump_buffer(void *Buffer, int Size)
{

}

#define kinetis_info(...)
#define kinetis_err(...)
#define kinetis_err_miss
#define kinetis_err_mem
#define kinetis_err_fun
#define kinetis_dbg_track
#define kinetis_dbg(...)
#define kinetis_dbg_exit
#define kinetis_dbg_enter
#define kinetis_dbg_status
#define kinetis_assert(x)
#endif

#ifdef __cplusplus
}
#endif

#endif /* __IDEBUG_H */
