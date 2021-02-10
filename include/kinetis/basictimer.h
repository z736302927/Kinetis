#ifndef __K_BASICTIMER_H
#define __K_BASICTIMER_H

#ifdef __cplusplus
extern "C" {
#endif

/* The following program is modified by the user according to the hardware device, otherwise the driver cannot run. */

/* Includes ------------------------------------------------------------------*/

#include <linux/types.h>

#define BASIC_32BIT_TIMER

extern volatile u32 timer_tick_ss;
#ifdef BASIC_16BIT_TIMER
extern volatile u32 timer_tick_us;
#endif


/* The above procedure is modified by the user according to the hardware device, otherwise the driver cannot run. */


void basic_timer_init_tick(void);
u32 basic_timer_get_ss_tick(void);
u64 basic_timer_get_ms_tick(void);
u32 basic_timer_get_us_tick(void);
void basic_timer_suspend_tick(void);
void basic_timer_resume_tick(void);

/**
  * @brief This function is called to increment a global variable "timerTick"
  *        used as application time base.
  * @note In the default implementation, this variable is incremented each 1ms
  *       in SysTick ISR.
 * @note This function is declared as static inline to be overwritten in case of other
  *      implementations in user file.
  * @retval None
  */
static inline void basic_timer_IncSSTick(void)
{
#ifdef BASIC_16BIT_TIMER

    if (timer_tick_us >= 1000000) {
        timer_tick_us = 0;
        timer_tick_ss++;
    }

#else
    timer_tick_ss++;
#endif
}

static inline void basic_timer_IncUSTick(void)
{
#ifdef BASIC_16BIT_TIMER
    timer_tick_us += 1000;
#endif
}


#ifdef __cplusplus
}
#endif

#endif /* __K_BASICTIMER_H */

