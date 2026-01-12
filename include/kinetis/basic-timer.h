#ifndef __K_BASICT_IMER_H
#define __K_BASICT_IMER_H

#ifdef __cplusplus
extern "C" {
#endif

/* The following program is modified by the user according to the hardware device, otherwise the driver cannot run. */

/* Includes ------------------------------------------------------------------*/
#include <linux/types.h>

#include "kinetis/core_common.h"

#if MCU_PLATFORM_STM32
#include "tim.h"
#else
#endif

/* The above procedure is modified by the user according to the hardware device, otherwise the driver cannot run. */

void basic_timer_init(void);
u32 basic_timer_get_ss(void);
u64 basic_timer_get_ms(void);
u64 basic_timer_get_us(void);
u64 basic_timer_get_ns(void);
void basic_timer_suspend(void);
void basic_timer_resume(void);
int basic_timer_thread_start(void);
int basic_timer_thread_stop(void);

/**
  * @brief This function is called to increment a global variable "timerTick"
  *        used as application time base.
  * @note In the default implementation, this variable is incremented each 1ms
  *       in SysTick ISR.
  * @note This function is declared as static inline to be overwritten in case of other
  *      implementations in user file.
  * @retval None
  */

void basic_timer_isr(u32 period);

#ifdef __cplusplus
}
#endif

#endif /* __K_BASIC_TIMER_H */

