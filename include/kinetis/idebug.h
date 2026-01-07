#ifndef __IDEBUG_H
#define __IDEBUG_H

#ifdef __cplusplus
extern "C" {
#endif

/* The following program is modified by the user according to the hardware device, otherwise the driver cannot run. */

/* Includes ------------------------------------------------------------------*/
#include <linux/printk.h>

#include "kinetis/basic-timer.h"


#define RELEASE_VERSION       1


/* The above procedure is modified by the user according to the hardware device, otherwise the driver cannot run. */

#if RELEASE_VERSION
#undef KERN_DEFAULT
#define KERN_DEFAULT	3
#endif

#define ERR_PRINT_TIME  printk("[%05d.%06d] ", basic_timer_get_ss(), basic_timer_get_timer_cnt())
#define DBG_PRINT_TIME  printk("[%05d.%06d] ", basic_timer_get_ss(), basic_timer_get_timer_cnt())
#define kinetis_info(...)     do{if(!(KERN_DEFAULT >= KERN_INFO))break;DBG_PRINT_TIME;printk(__VA_ARGS__); printk("\r\n");}while(0)
#define kinetis_err(...)      do{if(!(KERN_DEFAULT >= KERN_ERR))break;DBG_PRINT_TIME;printk(__VA_ARGS__); printk("\r\n");}while(0)
#define kinetis_dbg_track     do{if(!(KERN_DEFAULT >= KERN_DEBUG))break;DBG_PRINT_TIME;printk("%s,%d",  __FUNCTION__, __LINE__ ); printk("\r\n");}while(0)
#define kinetis_dbg(...)      do{if(!(KERN_DEFAULT >= KERN_DEBUG))break;DBG_PRINT_TIME;printk(__VA_ARGS__); printk("\r\n");}while(0)
#define kinetis_dbg_enter     do{if(!(KERN_DEFAULT >= KERN_DEBUG))break;DBG_PRINT_TIME;printk("enter %s\n", __FUNCTION__); printk("\r\n");}while(0)
#define kinetis_dbg_exit      do{if(!(KERN_DEFAULT >= KERN_DEBUG))break;DBG_PRINT_TIME;printk("exit %s\n", __FUNCTION__); printk("\r\n");}while(0)
#define kinetis_dbg_status    do{if(!(KERN_DEFAULT >= KERN_DEBUG))break;DBG_PRINT_TIME;printk("status %d\n", status); printk("\r\n");}while(0)
#define kinetis_err_miss      do{if(!(KERN_DEFAULT >= KERN_ERR))break;ERR_PRINT_TIME;printk("%s miss\n", __FUNCTION__); printk("\r\n");}while(0)
#define kinetis_err_mem       do{if(!(KERN_DEFAULT >= KERN_ERR))break;ERR_PRINT_TIME;printk("%s mem err\n", __FUNCTION__); printk("\r\n");}while(0)
#define kinetis_err_fun       do{if(!(KERN_DEFAULT >= KERN_ERR))break;ERR_PRINT_TIME;printk("%s err in %d\n", __FUNCTION__, __LINE__); printk("\r\n");}while(0)
#define kinetis_assert(x)     do{if(!(x)){ kinetis_err( "%s:%d assert failed\r\n", __FILE__, __LINE__);while(1);}} while(0)


#ifdef __cplusplus
}
#endif

#endif /* __IDEBUG_H */
