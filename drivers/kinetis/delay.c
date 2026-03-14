#include <linux/printk.h>
#include <linux/delay.h>
#include <linux/random.h>

#include "kinetis/delay.h"
#include "kinetis/basic-timer.h"
#include "kinetis/design_verification.h"
#include "kinetis/test-kinetis.h"

/* The following program is modified by the user according to the hardware device, otherwise the driver cannot run. */

/**
  * @step 1:  Modify the corresponding function according to the modified area and the corresponding function name.
  * @step 2:  Modify four areas: GPIO_PORT/GPIO_PIN/Led_TypeDef/LEDn.
  * @step 3:  .
  * @step 4:  .
  * @step 5:
  */

#ifdef DESIGN_VERIFICATION_DELAY

int t_delay(int argc, char **argv)
{
	u32 time_stamp = 0, i, delay_us, delay_ms, delay_s;
	s32 error_us, error_ms;
	u32 total_error_us = 0, max_error_us = 0;
	u32 total_error_ms = 0, max_error_ms = 0;
	u32 min_delay_us = ~0, max_delay_us = 0;
	u32 min_delay_ms = ~0, max_delay_ms = 0;
	u32 short_delay_tests = 0;

	pr_debug("===== delay test started =====\n");

	/* ===== 边界测试 ===== */
	pr_debug("[boundary test]\n");

	/* 测试0值延时 */
	time_stamp = basic_timer_get_us();
	udelay(0);
	time_stamp = basic_timer_get_us() - time_stamp;
	pr_debug("udelay(0): %u us\n", time_stamp);

	time_stamp = basic_timer_get_ms();
	mdelay(0);
	time_stamp = basic_timer_get_ms() - time_stamp;
	pr_debug("mdelay(0): %u ms\n", time_stamp);

	/* 测试最小单位延时 */
	time_stamp = basic_timer_get_us();
	udelay(1);
	time_stamp = basic_timer_get_us() - time_stamp;
	if (time_stamp >= 1 && time_stamp <= 5) {
		pr_debug("udelay(1): %u us (ok)\n", time_stamp);
	} else {
		pr_err("udelay(1): %u us (out of range!)\n", time_stamp);
		return FAIL;
	}

	time_stamp = basic_timer_get_ms();
	mdelay(1);
	time_stamp = basic_timer_get_ms() - time_stamp;
	if (time_stamp >= 1 && time_stamp <= 3) {
		pr_debug("mdelay(1): %u ms (ok)\n", time_stamp);
	} else {
		pr_err("mdelay(1): %u ms (out of range!)\n", time_stamp);
		return FAIL;
	}

	/* ===== 短延时测试（高频使用场景） ===== */
	pr_debug("[short delay test]\n");

	for (i = 0; i < 20; i++) {
		delay_us = get_random_range(1000, 0);
		time_stamp = basic_timer_get_us();
		udelay(delay_us);
		time_stamp = basic_timer_get_us() - time_stamp;
		error_us = (s32)time_stamp - (s32)delay_us;

		if (delay_us < min_delay_us) {
			min_delay_us = delay_us;
		}
		if (delay_us > max_delay_us) {
			max_delay_us = delay_us;
		}

		if (error_us < 0) {
			error_us = -error_us;
		}
		total_error_us += error_us;
		if (error_us > max_error_us) {
			max_error_us = error_us;
		}
		short_delay_tests++;

		if (error_us <= 3) {
			pr_debug("delay %u us, the result = %u us, error = %d us\n",
				delay_us, time_stamp, (s32)time_stamp - (s32)delay_us);
		} else {
			pr_err("delay %u us, the result = %u us, error = %d us (out of range!)\n",
				delay_us, time_stamp, (s32)time_stamp - (s32)delay_us);
			return FAIL;
		}
	}

	pr_debug("short delay test: count=%u, avg_error=%u us, max_error=%u us\n",
		short_delay_tests, total_error_us / short_delay_tests, max_error_us);

	/* ===== 微秒级随机延时测试 ===== */
	pr_debug("[microsecond delay test]\n");

	for (i = 0; i < 10; i++) {
		delay_us = get_random_range(1000000, 0);
		time_stamp = basic_timer_get_us();
		udelay(delay_us);
		time_stamp = basic_timer_get_us() - time_stamp;
		error_us = (s32)time_stamp - (s32)delay_us;

		if (delay_us < min_delay_us) {
			min_delay_us = delay_us;
		}
		if (delay_us > max_delay_us) {
			max_delay_us = delay_us;
		}

		if (error_us < 0) {
			error_us = -error_us;
		}
		total_error_us += error_us;
		if (error_us > max_error_us) {
			max_error_us = error_us;
		}

		if (error_us <= 5) {
			pr_debug("delay %u us, the result = %u us, error = %d us\n",
				delay_us, time_stamp, (s32)time_stamp - (s32)delay_us);
		} else {
			pr_err("delay %u us, the result = %u us, error = %d us (out of range!)\n",
				delay_us, time_stamp, (s32)time_stamp - (s32)delay_us);
			return FAIL;
		}
	}

	pr_debug("microsecond test: range=%u-%u us, avg_error=%u us, max_error=%u us\n",
		min_delay_us, max_delay_us, total_error_us / (short_delay_tests + 10), max_error_us);

	/* ===== 毫秒级延时测试 ===== */
	pr_debug("[millisecond delay test]\n");

	min_delay_ms = ~0;
	max_delay_ms = 0;

	for (i = 0; i < 15; i++) {
		delay_ms = get_random_range(5000, 0);
		time_stamp = basic_timer_get_ms();
		mdelay(delay_ms);
		time_stamp = basic_timer_get_ms() - time_stamp;
		error_ms = (s32)time_stamp - (s32)delay_ms;

		if (delay_ms < min_delay_ms) {
			min_delay_ms = delay_ms;
		}
		if (delay_ms > max_delay_ms) {
			max_delay_ms = delay_ms;
		}

		if (error_ms < 0) {
			error_ms = -error_ms;
		}
		total_error_ms += error_ms;
		if (error_ms > max_error_ms) {
			max_error_ms = error_ms;
		}

		if (error_ms <= 3) {
			pr_debug("delay %u ms, the result = %u ms, error = %d ms\n",
				delay_ms, time_stamp, (s32)time_stamp - (s32)delay_ms);
		} else {
			pr_err("delay %u ms, the result = %u ms, error = %d ms (out of range!)\n",
				delay_ms, time_stamp, (s32)time_stamp - (s32)delay_ms);
			return FAIL;
		}
	}

	pr_debug("millisecond test: range=%u-%u ms, avg_error=%u ms, max_error=%u ms\n",
		min_delay_ms, max_delay_ms, total_error_ms / 15, max_error_ms);

	/* ===== 秒级延时测试 ===== */
	pr_debug("[second delay test]\n");

	for (i = 0; i < 3; i++) {
		delay_s = get_random_range(10, 0);
		time_stamp = basic_timer_get_ms();
		ssleep(delay_s);
		time_stamp = basic_timer_get_ms() - time_stamp;
		error_ms = (s32)time_stamp - (s32)(delay_s * 1000);

		if (error_ms < 0) {
			error_ms = -error_ms;
		}

		if (error_ms <= 20) {
			pr_debug("delay %u s, the result = %u ms, error = %d ms\n",
				delay_s, time_stamp, error_ms);
		} else {
			pr_err("delay %u s, the result = %u ms, error = %d ms (out of range!)\n",
				delay_s, time_stamp, error_ms);
			return FAIL;
		}
	}

	/* ===== 连续延时测试（检测累积误差） ===== */
	pr_debug("[continuous delay test]\n");

	for (i = 0; i < 5; i++) {
		delay_ms = 100;
		time_stamp = basic_timer_get_ms();
		mdelay(delay_ms);
		time_stamp = basic_timer_get_ms() - time_stamp;
		error_ms = (s32)time_stamp - (s32)delay_ms;
		if (error_ms < 0) {
			error_ms = -error_ms;
		}

		if (error_ms > 3) {
			pr_err("continuous delay[%u]: %u ms expected, %u ms actual, error = %d ms\n",
				i, delay_ms, time_stamp, error_ms);
			return FAIL;
		}
	}

	pr_debug("continuous delay test: 5 x 100ms ok\n");

	pr_debug("===== delay test PASSed =====\n");

	return 0;
}

#endif
