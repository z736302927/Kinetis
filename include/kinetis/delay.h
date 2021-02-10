#ifndef __K_DELAY_H
#define __K_DELAY_H

#ifdef __cplusplus
extern "C" {
#endif

/* The following program is modified by the user according to the hardware device, otherwise the driver cannot run. */

/* Includes ------------------------------------------------------------------*/

#include <linux/types.h>

#define DELAY_32BIT_TIMER

#ifdef DELAY_32BIT_TIMER
#define DELAY_TIMER_UNIT              (u32)4294967295uL
#else
#define DELAY_TIMER_UNIT              (u16)65535u
#endif

void delay_enable_timer(void);
void delay_disable_timer(void);
void delay_init(void);
void delay_set_flag(void);
void delay_clear_flag(void);
void udelay(u32 delay);
void mdelay(u32 delay);
void sdelay(u32 delay);
/* The above procedure is modified by the user according to the hardware device, otherwise the driver cannot run. */


#ifdef __cplusplus
}
#endif

#endif /* __K_DELAY_H */
