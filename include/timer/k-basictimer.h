#ifndef __K_BASICTIMER_H
#define __K_BASICTIMER_H

#ifdef __cplusplus
extern "C" {
#endif

/*The following program is modified by the user according to the hardware device, otherwise the driver cannot run.*/

/* Includes ------------------------------------------------------------------*/
#include "core/core_common.h"

#define BASIC_32BIT_TIMER

extern volatile uint32_t timerTick_ss;
#ifdef BASIC_16BIT_TIMER
extern volatile uint32_t timerTick_us;
#endif


/*The above procedure is modified by the user according to the hardware device, otherwise the driver cannot run.*/


void BasicTimer_InitTick(void);
uint32_t BasicTimer_GetSSTick(void);
uint64_t BasicTimer_GetMSTick(void);
uint32_t BasicTimer_GetUSTick(void);
void BasicTimer_SuspendTick(void);
void BasicTimer_ResumeTick(void);

/**
  * @brief This function is called to increment a global variable "timerTick"
  *        used as application time base.
  * @note In the default implementation, this variable is incremented each 1ms
  *       in SysTick ISR.
 * @note This function is declared as static inline to be overwritten in case of other
  *      implementations in user file.
  * @retval None
  */
static inline void BasicTimer_IncSSTick(void)
{
#ifdef BASIC_16BIT_TIMER

    if (timerTick_us >= 1000000) {
        timerTick_us = 0;
        timerTick_ss++;
    }

#else
    timerTick_ss++;
#endif
}

static inline void BasicTimer_IncUSTick(void)
{
#ifdef BASIC_16BIT_TIMER
    timerTick_us += 1000;
#endif
}


#ifdef __cplusplus
}
#endif

#endif /* __K_BASICTIMER_H */

