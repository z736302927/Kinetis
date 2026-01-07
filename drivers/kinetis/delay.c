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
	u32 time_stamp = 0;

	time_stamp = basic_timer_get_us();
	udelay(1000);
	time_stamp = basic_timer_get_us() - time_stamp;
	pr_debug("Delay 1000 us, The result = %u us.\n", time_stamp);

	time_stamp = basic_timer_get_ms();
	mdelay(1000);
	time_stamp = basic_timer_get_ms() - time_stamp;
	pr_debug("Delay 1000 ms, The result = %u ms.\n", time_stamp);

	time_stamp = basic_timer_get_ss();
	ssleep(3);
	time_stamp = basic_timer_get_ss() - time_stamp;
	pr_debug("Delay 3 s, The result = %u s.\n", time_stamp);

	return PASS;
}

#endif
