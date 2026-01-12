#include "kinetis/basic-timer.h"
#include "kinetis/design_verification.h"

/* The following program is modified by the user according to the hardware device, otherwise the driver cannot run. */

/**
  * @step 1:  Modify the corresponding function according to the modified area and the corresponding function name.
  * @step 2:  Modify four areas: GPIO_PORT/GPIO_PIN/Led_TypeDef/LEDn.
  * @step 3:  .
  * @step 4:  .
  * @step 5:
  */

static volatile u32 timer_tick_ss;
static volatile u32 timer_tick_us;

#define TIMER_US_PER_SEC 			1000000
#define TIMER_INTERRUPT_PERIOD		1000  // 1 ms

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
void basic_timer_init(void)
{
	timer_tick_ss = 0;
	timer_tick_us = 0;
}

void basic_timer_isr(u32 period)
{
	timer_tick_us += period;

	if (timer_tick_us >= TIMER_US_PER_SEC) {
		timer_tick_us -= TIMER_US_PER_SEC;
		timer_tick_ss++;
	}
}

__weak u32 basic_timer_get_counter(void)
{
#if MCU_PLATFORM_STM32
    return (u32)htim2.Instance->CNT;
#else
	return 0;
#endif
}

static u64 _basic_timer_get_us(void)
{
	u32 ss, us, counter;

	do {
		ss = timer_tick_ss;
		us = timer_tick_us;
		counter = basic_timer_get_counter();

	} while (ss != timer_tick_ss);

	return (u64)ss * TIMER_US_PER_SEC + (u64)us + (u64)counter;
}

u32 basic_timer_get_ss(void)
{
	return timer_tick_ss;
}

u64 basic_timer_get_ms(void)
{
	return _basic_timer_get_us() / 1000;
}

u64 basic_timer_get_us(void)
{
	return _basic_timer_get_us();
}

u64 basic_timer_get_ns(void)
{
	return _basic_timer_get_us() * 1000;
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
void basic_timer_suspend(void)
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
void basic_timer_resume(void)
{

}

/* The above procedure is modified by the user according to the hardware device, otherwise the driver cannot run. */

#ifdef DESIGN_VERIFICATION_BASICTIMER
#include <linux/printk.h>
#include <linux/kernel.h>

#include "kinetis/test-kinetis.h"
#include "kinetis/idebug.h"

#include <unistd.h>
#include <pthread.h>

/* Timer thread control */
static pthread_t p_thread;
static volatile bool p_thread_running = false;

/**
 * @brief Timer thread function - increments tick every 1ms
 */
static void *timer_thread_func(void *arg)
{
	pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
	pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, NULL);

	while (p_thread_running) {
		usleep(TIMER_INTERRUPT_PERIOD);
		basic_timer_isr(TIMER_INTERRUPT_PERIOD);
	}

	return NULL;
}

int basic_timer_thread_start(void)
{
	int ret;

	if (p_thread_running) {
		pr_info("Timer thread is already running\n");
		return 0;
	}

	p_thread_running = true;

	ret = pthread_create(&p_thread, NULL, timer_thread_func, NULL);
	if (ret != 0) {
		p_thread_running = false;
		pr_err("Failed to create timer thread: %d\n", ret);
		return -ret;
	}

	pr_info("Timer thread started successfully\n");
	return 0;
}

int basic_timer_thread_stop(void)
{
	void *thread_ret;

	if (!p_thread_running) {
		pr_info("Timer thread is not running\n");
		return 0;
	}

	p_thread_running = false;

	if (pthread_join(p_thread, &thread_ret) == 0) {
		pr_info("Timer thread stopped successfully\n");
	} else {
		pr_warn("Timer thread stop had issues, but continuing\n");
	}

	return 0;
}

/**
 * @brief Start timer thread
 */
int t_basic_timer_thread_op(int argc, char **argv)
{
	bool on_off = true;
	void *thread_ret;
	int ret;

	if (argc > 1) {
		if (!strcmp(argv[1], "on")) {
			on_off = true;
		} else if (!strcmp(argv[1], "off")) {
			on_off = false;
		} else {
			return -EINVAL;
		}
	}

	if (on_off) {
		ret = basic_timer_thread_start();
		if (ret)
			return ret;
	} else {
		ret = basic_timer_thread_stop();
		if (ret)
			return ret;
	}

	return PASS;
}

int t_basic_timer_get_tick(int argc, char **argv)
{
	u16 times1 = 3, times2 = 10, times3 = 10;
	u16 i = 0;

	if (argc > 1) {
		times1 = simple_strtoul(argv[1], &argv[1], 10);
	}

	if (argc > 2) {
		times2 = simple_strtoul(argv[2], &argv[2], 10);
	}

	if (argc > 3) {
		times3 = simple_strtoul(argv[3], &argv[3], 10);
	}

	for (i = 0; i < times1; i++) {
		pr_debug("Current absolute second = %u\n", basic_timer_get_ss());
	}

	for (i = 0; i < times2; i++) {
		pr_debug("Current absolute millisecond = %llu\n", basic_timer_get_ms());
	}

	for (i = 0; i < times3; i++) {
		pr_debug("Current absolute microsecond = %llu\n", basic_timer_get_us());
	}

	return PASS;
}

#endif
