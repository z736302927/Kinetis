#include <linux/kernel.h>
#include <linux/printk.h>
#include <linux/errno.h>
#include <linux/delay.h>
#include <linux/string.h>

#include "config.h"
#include "motor.h"

#if KINETIS_FAKE_SIM
#include <pthread.h>
#endif

int motor_set_pwm(struct motor_controller *motor, u16 duty)
{
	if (duty > 10000) {
		pr_warn("motor: pwm duty %u clamped to 10000\n", duty);
		duty = 10000;
	}
	motor->pwm_duty = duty;
	return 0;
}

int motor_set_direction(struct motor_controller *motor, u8 forward)
{
	motor->direction = forward ? 1 : 0;
	return 0;
}

void motor_stop(struct motor_controller *motor)
{
	motor_set_pwm(motor, 0);
	motor_set_direction(motor, 0);
	motor->measured_rpm = 0;
}

#if KINETIS_FAKE_SIM

/*
 * First-order low-pass filter for motor RPM simulation.
 * Uses Q8.8 fixed-point arithmetic: alpha = 0.02 ≈ 5/256.
 */
#define MOTOR_SIM_ALPHA_FIXED    5
#define MOTOR_SIM_SCALE_BITS     8

static void *motor_sim_thread(void *arg)
{
	struct motor_controller *motor = (struct motor_controller *)arg;
	s32 current_rpm_fixed = 0;  /* Q8.8 fixed-point */

	while (motor->thread_running) {
		s32 target_rpm = (motor->pwm_duty * SLAVE_MOTOR_MAX_RPM) / 10000;
		if (!motor->direction) {
			target_rpm = 0;
		}

		/* Low-pass filter: current += alpha * (target - current) */
		current_rpm_fixed += ((target_rpm << MOTOR_SIM_SCALE_BITS)
			- current_rpm_fixed) * MOTOR_SIM_ALPHA_FIXED
			>> MOTOR_SIM_SCALE_BITS;

		if (current_rpm_fixed < (0.5f * (1 << MOTOR_SIM_SCALE_BITS))) {
			current_rpm_fixed = 0;
		}

		motor->measured_rpm = current_rpm_fixed >> MOTOR_SIM_SCALE_BITS;

		/* Update motor status based on actual RPM */
		motor->status = (motor->measured_rpm > 0) ?
			POV_STATUS_RUNNING : POV_STATUS_STOPPED;

		mdelay(SLAVE_PID_PERIOD_MS);
	}
	return NULL;
}

int motor_sim_start(struct motor_controller *motor)
{
	motor->thread_running = 1;
	motor->measured_rpm = 0;
	int ret = pthread_create(&motor->thread, NULL, motor_sim_thread, motor);
	if (ret != 0) {
		motor->thread_running = 0;
		return ret;
	}
	return 0;
}

void motor_sim_stop(struct motor_controller *motor)
{
	if (!motor || !motor->thread_running) {
		return;
	}
	motor->thread_running = 0;
	pthread_join(motor->thread, NULL);
}

#endif
