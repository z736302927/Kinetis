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

unsigned long read_chip_timer(void);

__weak u32 basic_timer_get_counter(void)
{
#if MCU_PLATFORM_STM32
	return (u32)htim2.Instance->CNT;
#else
	return read_chip_timer();
#endif
}

static u64 _basic_timer_get_us(void)
{
	u64 ss, us, counter;

	do {
		ss = timer_tick_ss;
		us = timer_tick_us;
		counter = basic_timer_get_counter();

	} while (ss != timer_tick_ss);

	return ss * TIMER_US_PER_SEC + us + counter;
}

u32 basic_timer_get_ss(void)
{
	return _basic_timer_get_us() / 1000000;
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
#include <linux/delay.h>
#include <linux/random.h>

#include "kinetis/test-kinetis.h"
#include "kinetis/idebug.h"
#include "kinetis/random-gene.h"

int t_basic_timer_get_tick(int argc, char **argv)
{
	u64 time1, time2, time3;
	u32 i;
	s64 delta;

	pr_debug("===== basic timer test started =====\n");

	/* ===== 秒级计时器测试 ===== */
	pr_debug("[second timer test]\n");

	time1 = basic_timer_get_ss();
	pr_debug("start time (ss): %llu\n", time1);

	ssleep(1);
	time2 = basic_timer_get_ss();
	delta = (s64)time2 - (s64)time1;

	if (delta == 1) {
		pr_debug("after 1s: %llu, delta = %lld s (ok)\n", time2, delta);
	} else {
		pr_err("after 1s: %llu, delta = %lld s (expected 1s)\n", time2, delta);
		return FAIL;
	}

	ssleep(2);
	time3 = basic_timer_get_ss();
	delta = (s64)time3 - (s64)time2;

	if (delta == 2) {
		pr_debug("after 2s: %llu, delta = %lld s (ok)\n", time3, delta);
	} else {
		pr_err("after 2s: %llu, delta = %lld s (expected 2s)\n", time3, delta);
		return FAIL;
	}

	/* ===== 毫秒级计时器测试 ===== */
	pr_debug("[millisecond timer test]\n");

	time1 = basic_timer_get_ms();
	pr_debug("start time (ms): %llu\n", time1);

	mdelay(100);
	time2 = basic_timer_get_ms();
	delta = (s64)time2 - (s64)time1;

	if (delta >= 98 && delta <= 102) {
		pr_debug("after 100ms: %llu, delta = %lld ms (ok)\n", time2, delta);
	} else {
		pr_err("after 100ms: %llu, delta = %lld ms (expected 100ms)\n", time2, delta);
		return FAIL;
	}

	mdelay(500);
	time3 = basic_timer_get_ms();
	delta = (s64)time3 - (s64)time2;

	if (delta >= 498 && delta <= 502) {
		pr_debug("after 500ms: %llu, delta = %lld ms (ok)\n", time3, delta);
	} else {
		pr_err("after 500ms: %llu, delta = %lld ms (expected 500ms)\n", time3, delta);
		return FAIL;
	}

	/* ===== 微秒级计时器测试 ===== */
	pr_debug("[microsecond timer test]\n");

	time1 = basic_timer_get_us();
	pr_debug("start time (us): %llu\n", time1);

	udelay(1000);
	time2 = basic_timer_get_us();
	delta = (s64)time2 - (s64)time1;

	if (delta >= 998 && delta <= 1002) {
		pr_debug("after 1000us: %llu, delta = %lld us (ok)\n", time2, delta);
	} else {
		pr_err("after 1000us: %llu, delta = %lld us (expected 1000us)\n", time2, delta);
		return FAIL;
	}

	mdelay(1000);
	time3 = basic_timer_get_us();
	delta = (s64)time3 - (s64)time2;

	if (delta >= 9998 && delta <= 10002) {
		pr_debug("after 10000us: %llu, delta = %lld us (ok)\n", time3, delta);
	} else {
		pr_err("after 10000us: %llu, delta = %lld us (expected 10000us)\n", time3, delta);
		return FAIL;
	}

	/* ===== 纳秒级计时器测试 ===== */
	pr_debug("[nanosecond timer test]\n");

	time1 = basic_timer_get_ns();
	pr_debug("start time (ns): %llu\n", time1);

	udelay(1000);
	time2 = basic_timer_get_ns();
	delta = (s64)time2 - (s64)time1;

	if (delta >= 998000 && delta <= 1002000) {
		pr_debug("after 1000us: %llu, delta = %lld ns (ok)\n", time2, delta);
	} else {
		pr_err("after 1000us: %llu, delta = %lld ns (expected ~1000000ns)\n", time2, delta);
		return FAIL;
	}

	/* ===== 连续读取测试（检查稳定性） ===== */
	pr_debug("[continuous read test]\n");

	for (i = 0; i < 10; i++) {
		time1 = basic_timer_get_us();
		udelay(1);
		time2 = basic_timer_get_us();

		if (time2 < time1) {
			pr_err("time not monotonic: %llu -> %llu\n", time1, time2);
			return FAIL;
		}
	}
	pr_debug("continuous read test: 10 reads ok\n");

	/* ===== 不同精度转换测试 ===== */
	pr_debug("[precision conversion test]\n");

	time1 = basic_timer_get_us();
	time2 = basic_timer_get_ms();
	time3 = basic_timer_get_ss();

	pr_debug("us = %llu, ms = %llu, ss = %llu\n", time1, time2, time3);

	/* 检查转换关系是否合理 */
	if (time2 * 1000 > time1 || time2 * 1000 + 1000 < time1) {
		pr_err("ms to us conversion error: %llu ms != %llu us\n", time2, time1);
		return FAIL;
	}

	if (time3 * 1000000 > time1 || time3 * 1000000 + 1000000 < time1) {
		pr_err("ss to us conversion error: %llu ss != %llu us\n", time3, time1);
		return FAIL;
	}

	pr_debug("precision conversion test: ok\n");

	/* ===== 随机延时测试 ===== */
	pr_debug("[random delay test]\n");

	for (i = 0; i < 5; i++) {
		u32 delay_ms = get_random_range(100, 0);

		time1 = basic_timer_get_ms();
		mdelay(delay_ms);
		time2 = basic_timer_get_ms();
		delta = (s64)time2 - (s64)time1;

		if (delta >= (s64)delay_ms - 2 && delta <= (s64)delay_ms + 2) {
			pr_debug("delay %u ms: actual %lld ms (ok)\n", delay_ms, delta);
		} else {
			pr_err("delay %u ms: actual %lld ms (out of range!)\n", delay_ms, delta);
			return FAIL;
		}
	}

	/* ===== 长时间计时测试 ===== */
	pr_debug("[long duration test]\n");

	time1 = basic_timer_get_ss();
	ssleep(3);
	time2 = basic_timer_get_ss();
	delta = (s64)time2 - (s64)time1;

	if (delta == 3) {
		pr_debug("long duration: %lld s (ok)\n", delta);
	} else {
		pr_err("long duration: %lld s (expected 3s)\n", delta);
		return FAIL;
	}

	/* ===== 边界测试（最小值） ===== */
	pr_debug("[boundary test - minimum values]\n");

	time1 = basic_timer_get_us();
	udelay(0);
	time2 = basic_timer_get_us();

	if (time2 >= time1) {
		pr_debug("udelay(0): delta = %lld us (ok)\n", (s64)time2 - (s64)time1);
	} else {
		pr_err("udelay(0): time went backwards!\n");
		return FAIL;
	}

	pr_debug("===== basic timer test passed =====\n");

	return 0;
}

#endif
