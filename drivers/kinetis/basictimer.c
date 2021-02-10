#include "kinetis/basictimer.h"

#include "tim.h"

/* The following program is modified by the user according to the hardware device, otherwise the driver cannot run. */

/**
  * @step 1:  Modify the corresponding function according to the modified area and the corresponding function name.
  * @step 2:  Modify four areas: GPIO_PORT/GPIO_PIN/Led_TypeDef/LEDn.
  * @step 3:  .
  * @step 4:  .
  * @step 5:
  */

volatile u32 timer_tick_ss;
#ifdef BASIC_16BIT_TIMER
volatile u32 timer_tick_us;
#endif

/**
  * @brief This function configures the source of the time base:
  *        The time source is configured to have 1ms time base with a dedicated
  *        Tick interrupt priority.
  * @note This function is called  automatically at the beginning of program after
  *       reset by basic_timer_Init() or at any time when clock is reconfigured  by basic_timer_RCC_ClockConfig().
  * @note In the default implementation, SysTick timer is the source of time base.
  *       It is used to generate interrupts at regular time intervals.
  *       Care must be taken if basic_timer_Delay() is called from a peripheral ISR process,
  *       The SysTick interrupt must have higher priority (numerically lower)
  *       than the peripheral interrupt. Otherwise the caller ISR process will be blocked.
  *       The function is declared as static inline  to be overwritten  in case of other
  *       implementation  in user file.
  * @param TickPriority  Tick interrupt priority.
  * @retval HAL status
  */
void basic_timer_init_tick(void)
{
    timer_tick_ss = 0;
#ifdef BASIC_16BIT_TIMER
    timer_tick_us = 0;
#endif
}

/**
  * @brief Provide a tick value in millisecond.
  * @note This function is declared as static inline to be overwritten in case of other
  *       implementations in user file.
  * @retval tick value
  */
u32 basic_timer_get_ss_tick(void)
{
    return timer_tick_ss;
}

u64 basic_timer_get_ms_tick(void)
{
    return timer_tick_ss * 1000 + basic_timer_get_us_tick() / 1000;
}

u32 basic_timer_get_us_tick(void)
{
#ifdef BASIC_16BIT_TIMER
    return timer_tick_us;
#else
    return (u32)htim2.Instance->CNT;
#endif
}

/**
  * @brief Suspend Tick increment.
  * @note In the default implementation , SysTick timer is the source of time base. It is
  *       used to generate interrupts at regular time intervals. Once basic_timer_SuspendTick()
  *       is called, the SysTick interrupt will be disabled and so Tick increment
  *       is suspended.
  * @note This function is declared as static inline to be overwritten in case of other
  *       implementations in user file.
  * @retval None
  */
void basic_timer_suspend_tick(void)
{

}

/**
  * @brief Resume Tick increment.
  * @note In the default implementation , SysTick timer is the source of time base. It is
  *       used to generate interrupts at regular time intervals. Once basic_timer_ResumeTick()
  *       is called, the SysTick interrupt will be enabled and so Tick increment
  *       is resumed.
  * @note This function is declared as static inline to be overwritten in case of other
  *       implementations in user file.
  * @retval None
  */
void basic_timer_resume_tick(void)
{

}

/* The above procedure is modified by the user according to the hardware device, otherwise the driver cannot run. */

#ifdef DESIGN_VERIFICATION_BASICTIMER
#include "kinetis/test.h"
#include "kinetis/idebug.h"
#include "stdlib.h"

int t_basic_timer_get_tick(int argc, char **argv)
{
    u16 times1 = 3, times2 = 10, times3 = 10;
    u16 i = 0;

    if (argc > 1)
        times1 = strtoul(argv[1], &argv[1], 10);

    if (argc > 2)
        times2 = strtoul(argv[2], &argv[2], 10);

    if (argc > 3)
        times3 = strtoul(argv[3], &argv[3], 10);

    for (i = 0; i < times1; i++)
        kinetis_debug_trace(KERN_DEBUG, "Current absolute second = %lu", basic_timer_get_ss_tick());

    for (i = 0; i < times2; i++)
        kinetis_debug_trace(KERN_DEBUG, "Current absolute millisecond = %llu", basic_timer_get_ms_tick());

    for (i = 0; i < times3; i++)
        kinetis_debug_trace(KERN_DEBUG, "Current absolute microsecond = %lu", basic_timer_get_us_tick());

    return PASS;
}

#endif

