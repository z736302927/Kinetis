/**
 * @file slave_motor.c
 * @brief Motor control implementation for slave (fixed end)
 * @note PWM drive with hall sensor feedback for RPM measurement
 */

#include <linux/kernel.h>
#include <linux/printk.h>
#include <linux/errno.h>
#include <linux/delay.h>
#include <linux/string.h>

#include "slave_motor.h"

#include <kinetis/basic-timer.h>

#if KINETIS_FAKE_SIM
#include <pthread.h>
#endif

/*********************************************************************
 * Initialization
 *********************************************************************/

int slave_motor_init(struct slave_motor_device *dev)
{
	if (!dev)
		return -EINVAL;

	memset(dev, 0, sizeof(*dev));

#if MCU_PLATFORM_STM32
	/*
	 * STM32F103 motor control configuration:
	 *
	 * PWM Timer (TIM3):
	 * - Channel 1: PA6 (PWM output to motor driver)
	 * - Frequency: 1kHz (SLAVE_PWM_FREQ_HZ)
	 * - Resolution: 0-999 (SLAVE_PWM_RESOLUTION - 1)
	 * - Prescaler: SystemClock / (1000 * 1000) - 1
	 *
	 * Direction GPIO:
	 * - PB0: IN1 on motor driver (forward)
	 * - PB1: IN2 on motor driver (reverse)
	 *
	 * Hall Timer (TIM4):
	 * - Channel 1: PB6 (input capture)
	 * - Measures time between hall pulses
	 */
	pr_info("Slave motor: PWM+Hall configured on STM32F103\n");
#else
	pr_info("Slave motor: initialized in simulation mode\n");
#endif

	dev->initialized = 1;
	return 0;
}

/*********************************************************************
 * PWM Control
 *********************************************************************/

int slave_motor_set_pwm(struct slave_motor_device *dev, float duty)
{
	if (!dev || !dev->initialized)
		return -EINVAL;

	if (duty < 0.0f)
		duty = 0.0f;
	if (duty > 100.0f)
		duty = 100.0f;

	dev->pwm_duty = duty;

#if MCU_PLATFORM_STM32
	/* Convert duty to compare value */
	u32 compare = (u32)((duty / 100.0f) * SLAVE_PWM_RESOLUTION);
	__HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_1, compare);
#endif

	return 0;
}

/*********************************************************************
 * Direction Control
 *********************************************************************/

int slave_motor_set_direction(struct slave_motor_device *dev, u8 forward)
{
	if (!dev || !dev->initialized)
		return -EINVAL;

	dev->direction = forward ? 1 : 0;

#if MCU_PLATFORM_STM32
	if (forward) {
		HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0, GPIO_PIN_SET);    /* IN1=1 */
		HAL_GPIO_WritePin(GPIOB, GPIO_PIN_1, GPIO_PIN_RESET);  /* IN2=0 */
	} else {
		HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0, GPIO_PIN_RESET);  /* IN1=0 */
		HAL_GPIO_WritePin(GPIOB, GPIO_PIN_1, GPIO_PIN_SET);    /* IN2=1 */
	}
#endif

	return 0;
}

/*********************************************************************
 * Stop Motor
 *********************************************************************/

void slave_motor_stop(struct slave_motor_device *dev)
{
	if (!dev)
		return;

	slave_motor_set_pwm(dev, 0.0f);
	slave_motor_set_direction(dev, 0);
	dev->measured_rpm = 0;
}

/*********************************************************************
 * RPM Measurement
 *********************************************************************/

s32 slave_motor_get_rpm(struct slave_motor_device *dev)
{
	if (!dev || !dev->initialized)
		return 0;

#if KINETIS_FAKE_SIM
	/* Simulation: RPM tracks PWM with some dynamics */
	return dev->measured_rpm;
#else
	u64 now = basic_timer_get_ms();

	/* Check for hall timeout (motor stalled) */
	if (dev->hall_last_ms > 0 &&
		(now - dev->hall_last_ms) > SLAVE_HALL_TIMEOUT_MS) {
		dev->measured_rpm = 0;
	}

	return dev->measured_rpm;
#endif
}

/*********************************************************************
 * Hall ISR
 *********************************************************************/

void slave_motor_hall_isr(struct slave_motor_device *dev)
{
	if (!dev)
		return;

#if MCU_PLATFORM_STM32
	u32 capture = HAL_TIM_ReadCapturedValue(&htim4, TIM_CHANNEL_1);
	u64 now = basic_timer_get_ms();

	if (dev->hall_last_tick != 0 && dev->hall_last_ms != 0) {
		dev->hall_period_ticks = capture - dev->hall_last_tick;

		/* Calculate RPM from period */
		if (dev->hall_period_ticks > 0) {
			u32 timer_freq = SystemCoreClock / (htim4.Init.Prescaler + 1);
			u32 rpm = (60 * timer_freq) /
				(dev->hall_period_ticks * SLAVE_HALL_PULSES_PER_REV);
			dev->measured_rpm = (s32)rpm;
		}
	}

	dev->hall_last_tick = capture;
	dev->hall_last_ms = now;
	dev->hall_count++;
#endif
}

/*********************************************************************
 * Status Check
 *********************************************************************/

int slave_motor_is_running(struct slave_motor_device *dev)
{
	if (!dev)
		return 0;

#if KINETIS_FAKE_SIM
	return dev->measured_rpm > 0 ? 1 : 0;
#else
	u64 now = basic_timer_get_ms();
	return (dev->hall_last_ms > 0 &&
		(now - dev->hall_last_ms) < SLAVE_HALL_TIMEOUT_MS) ? 1 : 0;
#endif
}

/*********************************************************************
 * Simulation
 *********************************************************************/

#if KINETIS_FAKE_SIM

static void *slave_motor_sim_thread(void *arg)
{
	struct slave_motor_device *dev = (struct slave_motor_device *)arg;

	pr_info("Slave motor: simulation thread started\n");

	while (dev->sim_running) {
		/* Simulate RPM based on PWM duty with first-order response */
		float target_rpm = (dev->pwm_duty / 100.0f) * SLAVE_MOTOR_MAX_RPM;
		if (!dev->direction)
			target_rpm = 0;

		/* First-order lag: tau = 0.5s, dt = 10ms */
		float alpha = 0.02f;
		dev->sim_current_rpm += alpha * (target_rpm - dev->sim_current_rpm);

		if (dev->sim_current_rpm < 0.5f)
			dev->sim_current_rpm = 0;

		dev->measured_rpm = (s32)dev->sim_current_rpm;

		mdelay(SLAVE_PID_PERIOD_MS);
	}

	pr_info("Slave motor: simulation thread stopped\n");
	return NULL;
}

/* Thread handle - exposed for slave_main to start */
static pthread_t g_slave_motor_sim_thread;

int slave_motor_sim_start(struct slave_motor_device *dev)
{
	if (!dev)
		return -EINVAL;

	dev->sim_running = 1;
	dev->sim_current_rpm = 0;
	dev->measured_rpm = 0;

	int ret = pthread_create(&g_slave_motor_sim_thread, NULL,
		slave_motor_sim_thread, dev);
	if (ret != 0) {
		dev->sim_running = 0;
		pr_err("Slave motor: sim thread create failed: %d\n", ret);
		return ret;
	}

	return 0;
}

void slave_motor_sim_stop(struct slave_motor_device *dev)
{
	if (!dev || !dev->sim_running)
		return;

	dev->sim_running = 0;
	pthread_join(g_slave_motor_sim_thread, NULL);
}
#endif /* KINETIS_FAKE_SIM */

/*********************************************************************
 * Deinitialization
 *********************************************************************/

void slave_motor_deinit(struct slave_motor_device *dev)
{
	if (!dev)
		return;

#if KINETIS_FAKE_SIM
	if (dev->sim_running)
		slave_motor_sim_stop(dev);
#endif

	slave_motor_stop(dev);
	dev->initialized = 0;
	pr_info("Slave motor: deinitialized\n");
}
