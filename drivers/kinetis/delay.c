#include <linux/printk.h>
#include <linux/jiffies.h>

#include "kinetis/delay.h"
#include "kinetis/basic-timer.h"
#include "kinetis/idebug.h"
#include "kinetis/random-gene.h"
#include "kinetis/design_verification.h"

/* The following program is modified by the user according to the hardware device, otherwise the driver cannot run. */

/**
  * @step 1:  Modify the corresponding function according to the modified area and the corresponding function name.
  * @step 2:  Modify four areas: GPIO_PORT/GPIO_PIN/Led_TypeDef/LEDn.
  * @step 3:  .
  * @step 4:  .
  * @step 5:
  */

#ifdef DESIGN_VERIFICATION_DELAY
#include "kinetis/test-kinetis.h"

#include <linux/delay.h>

int t_delay(int argc, char **argv)
{
	u32 time_stamp = 0, i, delay_us, delay_ms, delay_s;

	for (i = 0; i < 10; i++) {
		delay_us = get_random_range(500, 1500);
		time_stamp = basic_timer_get_us();
		udelay(delay_us);
		time_stamp = basic_timer_get_us() - time_stamp;
		pr_debug("Delay %u us, The result = %u us.\n", delay_us, time_stamp);
	}

	for (i = 0; i < 10; i++) {
		delay_ms = get_random_range(500, 1500);
		time_stamp = basic_timer_get_ms();
		mdelay(delay_ms);
		time_stamp = basic_timer_get_ms() - time_stamp;
		pr_debug("Delay %u ms, The result = %u ms.\n", delay_ms, time_stamp);
	}

	for (i = 0; i < 10; i++) {
		delay_s = get_random_range(1, 10);
		time_stamp = basic_timer_get_ss();
		ssleep(delay_s);
		time_stamp = basic_timer_get_ss() - time_stamp;
		pr_debug("Delay %u s, The result = %u s.\n", delay_s, time_stamp);
	}

	return PASS;
}

#endif
