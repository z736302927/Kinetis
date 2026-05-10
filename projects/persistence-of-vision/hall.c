
#include <linux/printk.h>
#include <linux/slab.h>

#include "config.h"
#include "hall.h"

u32 hall_get_angle(struct hall_device *dev)
{
	u32 column;

	if (dev->period_ticks == 0) {
		return 0;
	}

	/* Map to column number */
	column = (dev->read_rotated_time(dev) * dev->angular_resolution) / dev->period_ticks;
	if (column >= dev->angular_resolution) {
		column = dev->angular_resolution - 1;
	}

	return column;
}

s32 hall_get_rpm(struct hall_device *dev)
{
	if (dev->period_ticks == 0) {
		return 0;
	}

	return 60000000 / dev->period_ticks;
}

void hall_hall_isr(struct hall_device *dev, u32 capture_value)
{
	dev->period_ticks = capture_value;
}

struct hall_device *hall_alloc_dev(u32 angular_resolution, u32(*read_rotated_time)(struct hall_device *dev))
{
	struct hall_device *dev;

	dev = kzalloc(sizeof(*dev), GFP_KERNEL);
	if (!dev) {
		return NULL;
	}

	/*
	 * TIM2 configuration for input capture:
	 * - Clock: 200MHz / (199+1) = 1MHz (1us tick)
	 * - Channel 1: Input capture on hall sensor GPIO
	 * - Interrupt on capture event
	 *
	 * This would be done via STM32 HAL:
	 *   htim2.Init.Prescaler = POV_TIM_PRESCALER;
	 *   htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
	 *   htim2.Init.Period = 0xFFFFFFFF;
	 *   HAL_TIM_IC_Start_IT(&htim2, TIM_CHANNEL_1);
	 */
#if KINETIS_FAKE_SIM
	/* ~66.7ms per revolution */
	hall_hall_isr(dev, 60000000 / POV_TARGET_RPM);
	pr_info("initialized in simulation mode\n");
#endif

	dev->angular_resolution = angular_resolution;
	dev->read_rotated_time = read_rotated_time;

	return dev;
}

#if KINETIS_FAKE_SIM

#include <kinetis/basic-timer.h>

u32 hall_read_rotated_time(struct hall_device *dev)
{
	return basic_timer_get_us() % dev->period_ticks;
}
#endif /* KINETIS_FAKE_SIM */
