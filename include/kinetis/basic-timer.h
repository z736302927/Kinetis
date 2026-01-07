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

#define BASIC_32BIT_TIMER

/* The above procedure is modified by the user according to the hardware device, otherwise the driver cannot run. */

void basic_timer_init(void);
u32 basic_timer_get_ss(void);
u64 basic_timer_get_ms(void);
u64 basic_timer_get_us(void);
u64 basic_timer_get_ns(void);
void basic_timer_suspend(void);
void basic_timer_resume(void);

/**
  * @brief This function is called to increment a global variable "timerTick"
  *        used as application time base.
  * @note In the default implementation, this variable is incremented each 1ms
  *       in SysTick ISR.
  * @note This function is declared as static inline to be overwritten in case of other
  *      implementations in user file.
  * @retval None
  */
extern void basic_timer_inc_ss(void);
extern void basic_timer_inc_us(void);

static inline void basic_timer_inc_tick(void)
{
    basic_timer_inc_us();
    basic_timer_inc_ss();
}

/**
  * @brief Provide a timer value in microsecond.
  * @note You need to set the timer resolution to 1us
  * @retval tick value
  */
static inline u32 basic_timer_get_timer_cnt(void)
{
#ifdef BASIC_16BIT_TIMER
    return timer_tick_us;
#else
#if MCU_PLATFORM_STM32
    return (u32)htim2.Instance->CNT;
#else
#endif
#endif
}

#ifdef __cplusplus
}
#endif

#endif /* __K_BASIC_TIMER_H */

