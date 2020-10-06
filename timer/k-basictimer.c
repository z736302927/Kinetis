#include "timer/k-basictimer.h"

/*The following program is modified by the user according to the hardware device, otherwise the driver cannot run.*/

/**
  * @step 1:  Modify the corresponding function according to the modified area and the corresponding function name.
  * @step 2:  Modify four areas: GPIO_PORT/GPIO_PIN/Led_TypeDef/LEDn.
  * @step 3:  .
  * @step 4:  .
  * @step 5:
  */

#include "tim.h"
#include "core/idebug.h"

volatile uint32_t timerTick_ss;
#ifdef BASIC_16BIT_TIMER
volatile uint32_t timerTick_us;
#endif

/**
  * @brief This function configures the source of the time base:
  *        The time source is configured to have 1ms time base with a dedicated
  *        Tick interrupt priority.
  * @note This function is called  automatically at the beginning of program after
  *       reset by BasicTimer_Init() or at any time when clock is reconfigured  by BasicTimer_RCC_ClockConfig().
  * @note In the default implementation, SysTick timer is the source of time base.
  *       It is used to generate interrupts at regular time intervals.
  *       Care must be taken if BasicTimer_Delay() is called from a peripheral ISR process,
  *       The SysTick interrupt must have higher priority (numerically lower)
  *       than the peripheral interrupt. Otherwise the caller ISR process will be blocked.
  *       The function is declared as static inline  to be overwritten  in case of other
  *       implementation  in user file.
  * @param TickPriority  Tick interrupt priority.
  * @retval HAL status
  */
void BasicTimer_InitTick(void)
{
    timerTick_ss = 0;
#ifdef BASIC_16BIT_TIMER
    timerTick_us = 0;
#endif
}

/**
  * @brief Provide a tick value in millisecond.
  * @note This function is declared as static inline to be overwritten in case of other
  *       implementations in user file.
  * @retval tick value
  */
uint32_t BasicTimer_GetSSTick(void)
{
    return timerTick_ss;
}

uint64_t BasicTimer_GetMSTick(void)
{
    return timerTick_ss * 1000 + BasicTimer_GetUSTick() / 1000;
}

uint32_t BasicTimer_GetUSTick(void)
{
#ifdef BASIC_16BIT_TIMER
    return timerTick_us;
#else
    return (uint32_t)htim2.Instance->CNT;
#endif
}

/**
  * @brief Suspend Tick increment.
  * @note In the default implementation , SysTick timer is the source of time base. It is
  *       used to generate interrupts at regular time intervals. Once BasicTimer_SuspendTick()
  *       is called, the SysTick interrupt will be disabled and so Tick increment
  *       is suspended.
  * @note This function is declared as static inline to be overwritten in case of other
  *       implementations in user file.
  * @retval None
  */
void BasicTimer_SuspendTick(void)
{

}

/**
  * @brief Resume Tick increment.
  * @note In the default implementation , SysTick timer is the source of time base. It is
  *       used to generate interrupts at regular time intervals. Once BasicTimer_ResumeTick()
  *       is called, the SysTick interrupt will be enabled and so Tick increment
  *       is resumed.
  * @note This function is declared as static inline to be overwritten in case of other
  *       implementations in user file.
  * @retval None
  */
void BasicTimer_ResumeTick(void)
{

}

/*The above procedure is modified by the user according to the hardware device, otherwise the driver cannot run.*/

#ifdef DESIGN_VERIFICATION_BASICTIMER
#include "dv/k-test.h"
#include "stdlib.h"

int t_BasicTimer_GetTick(int argc, char **argv)
{
    uint16_t times1 = 3, times2 = 10, times3 = 10;
    uint16_t i = 0;

    if (argc > 1)
        times1 = strtoul(argv[1], &argv[1], 10);

    if (argc > 2)
        times2 = strtoul(argv[2], &argv[2], 10);

    if (argc > 3)
        times3 = strtoul(argv[3], &argv[3], 10);

    for (i = 0; i < times1; i++)
        kinetis_debug_trace(KERN_DEBUG, "Current absolute second = %lu", BasicTimer_GetSSTick());

    for (i = 0; i < times2; i++)
        kinetis_debug_trace(KERN_DEBUG, "Current absolute millisecond = %llu", BasicTimer_GetMSTick());

    for (i = 0; i < times3; i++)
        kinetis_debug_trace(KERN_DEBUG, "Current absolute microsecond = %lu", BasicTimer_GetUSTick());

    return PASS;
}

#endif

