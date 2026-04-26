/**
 * @file host_angle.c
 * @brief Hall sensor angle detection implementation
 * @note Maps TIM2 input capture to display column (0~719) and RPM
 */

#include <linux/kernel.h>
#include <linux/printk.h>
#include <linux/errno.h>
#include <linux/delay.h>
#include <linux/string.h>

#include "host_angle.h"

#if KINETIS_FAKE_SIM
#include <pthread.h>
#include <kinetis/basic-timer.h>
#endif

/*********************************************************************
 * Internal: Calculate column from timer position
 *********************************************************************/

static void host_angle_update_column(struct host_angle_device *dev)
{
	if (dev->period_ticks == 0)
		return;

	/* Current position within the revolution as a fraction */
	u32 elapsed;

#if MCU_PLATFORM_STM32
	elapsed = dev->current_tick - dev->last_capture;
#else
	elapsed = dev->current_tick % dev->period_ticks;
#endif

	/* Map to column number */
	u32 col = (elapsed * POV_DISPLAY_COLS) / dev->period_ticks;
	if (col >= POV_DISPLAY_COLS)
		col = POV_DISPLAY_COLS - 1;

	dev->current_column = (u16)col;
}

/*********************************************************************
 * Initialization
 *********************************************************************/

int host_angle_init(struct host_angle_device *dev)
{
	if (!dev)
		return -EINVAL;

	memset(dev, 0, sizeof(*dev));

#if MCU_PLATFORM_STM32
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
	pr_info("Host angle: TIM2 input capture configured\n");
#else
	dev->sim_period_ms = 60000 / POV_TARGET_RPM;  /* ~66.7ms per revolution */
	pr_info("Host angle: initialized in simulation mode\n");
#endif

	dev->initialized = 1;
	return 0;
}

/*********************************************************************
 * Public API
 *********************************************************************/

int host_angle_get_column(struct host_angle_device *dev)
{
	if (!dev || !dev->initialized)
		return -1;

#if KINETIS_FAKE_SIM
	/* In simulation, compute column from elapsed time */
	u64 now_ms = basic_timer_get_ms();
	u32 period_ms = dev->sim_period_ms > 0 ? dev->sim_period_ms : 67;
	u64 elapsed = now_ms % period_ms;
	dev->current_column = (u16)((elapsed * POV_DISPLAY_COLS) / period_ms);
#endif

	return dev->current_column;
}

s32 host_angle_get_rpm(struct host_angle_device *dev)
{
	if (!dev || !dev->initialized)
		return 0;

#if KINETIS_FAKE_SIM
	/* In simulation, return the configured RPM */
	if (dev->sim_period_ms > 0)
		dev->measured_rpm = 60000 / dev->sim_period_ms;
#endif

	return dev->measured_rpm;
}

int host_angle_is_new_column(struct host_angle_device *dev)
{
	static u16 last_column = 0xFFFF;

	if (!dev || !dev->initialized)
		return 0;

	int col = host_angle_get_column(dev);
	if (col < 0)
		return 0;

	if ((u16)col != last_column) {
		last_column = (u16)col;
		return 1;
	}

	return 0;
}

/*********************************************************************
 * ISR Handler (hardware mode)
 *********************************************************************/

void host_angle_hall_isr(struct host_angle_device *dev)
{
	if (!dev)
		return;

#if MCU_PLATFORM_STM32
	u32 capture = HAL_TIM_ReadCapturedValue(&htim2, TIM_CHANNEL_1);

	if (dev->last_capture != 0) {
		dev->period_ticks = capture - dev->last_capture;

		/* Calculate RPM: (60 * tick_freq) / period_ticks */
		u32 tick_freq = POV_TIM_CLOCK_HZ / (POV_TIM_PRESCALER + 1);
		dev->measured_rpm = (s32)((60ULL * tick_freq) / dev->period_ticks);
	}

	dev->last_capture = capture;
	dev->current_tick = capture;
	host_angle_update_column(dev);
#endif
}

/*********************************************************************
 * Simulation
 *********************************************************************/

#if KINETIS_FAKE_SIM

static void *host_angle_sim_thread(void *arg)
{
	struct host_angle_device *dev = (struct host_angle_device *)arg;

	pr_info("Host angle: simulation thread started\n");

	while (dev->sim_running) {
		/* Update current tick based on time within revolution period */
		u64 now_ms = basic_timer_get_ms();
		dev->current_tick = (u32)(now_ms % dev->sim_period_ms);
		host_angle_update_column(dev);

		mdelay(1);
	}

	pr_info("Host angle: simulation thread stopped\n");
	return NULL;
}

static pthread_t g_host_angle_sim_thread;

int host_angle_sim_start(struct host_angle_device *dev, s32 target_rpm)
{
	if (!dev)
		return -EINVAL;

	if (target_rpm <= 0) {
		dev->sim_period_ms = 0;
		dev->measured_rpm = 0;
		return 0;
	}

	dev->sim_period_ms = 60000 / target_rpm;
	dev->measured_rpm = target_rpm;
	dev->sim_running = 1;

	int ret = pthread_create(&g_host_angle_sim_thread, NULL,
		host_angle_sim_thread, dev);
	if (ret != 0) {
		dev->sim_running = 0;
		pr_err("Host angle: failed to create sim thread: %d\n", ret);
		return ret;
	}

	pr_info("Host angle: simulation started at %d RPM (period=%dms)\n",
		target_rpm, dev->sim_period_ms);
	return 0;
}

void host_angle_sim_stop(struct host_angle_device *dev)
{
	if (!dev || !dev->sim_running)
		return;

	dev->sim_running = 0;
	pthread_join(g_host_angle_sim_thread, NULL);
	pr_info("Host angle: simulation stopped\n");
}

#else

int host_angle_sim_start(struct host_angle_device *dev, s32 target_rpm)
{
	/* Not used in hardware mode */
	return -ENOSYS;
}

void host_angle_sim_stop(struct host_angle_device *dev)
{
	/* Not used in hardware mode */
}

#endif /* KINETIS_FAKE_SIM */
