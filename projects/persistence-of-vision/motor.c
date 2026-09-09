#include <linux/kernel.h>
#include <linux/printk.h>
#include <linux/errno.h>
#include <linux/delay.h>
#include <linux/string.h>

#include "stator.h"
#include "config.h"
#include "motor.h"

#if KINETIS_FAKE_SIM
#include <pthread.h>
#endif

/*
 * First-order low-pass filter for motor RPM simulation.
 * Uses Q8.8 fixed-point arithmetic: alpha = 0.02 ≈ 5/256.
 */
#define MOTOR_SIM_ALPHA_FIXED    5
#define MOTOR_SIM_SCALE_BITS     8

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
	motor->sim_rpm_fixed = 0;
	motor->measured_rpm = 0;
	motor->period_us = 0;
}

static void motor_simulation_step(struct motor_controller *motor)
{
	s32 target_rpm = (motor->pwm_duty * motor->max_rpm) / 10000;

	/* Bidirectional: REVERSE → negative target RPM */
	if (motor->direction == SLAVE_MOTOR_DIR_REVERSE) {
		target_rpm = -target_rpm;
	}

	/* Low-pass filter: current += alpha * (target - current) */
	motor->sim_rpm_fixed += ((target_rpm << MOTOR_SIM_SCALE_BITS) - motor->sim_rpm_fixed)
		* MOTOR_SIM_ALPHA_FIXED >> MOTOR_SIM_SCALE_BITS;

	/* Hysteresis dead-zone: snap small values to zero */
	if (motor->sim_rpm_fixed > -(1 << (MOTOR_SIM_SCALE_BITS - 1)) &&
		motor->sim_rpm_fixed < (1 << (MOTOR_SIM_SCALE_BITS - 1))) {
		motor->sim_rpm_fixed = 0;
	}

	motor->measured_rpm = motor->sim_rpm_fixed >> MOTOR_SIM_SCALE_BITS;

	if (motor->measured_rpm != 0) {
		u32 abs_rpm = motor->measured_rpm > 0
			? (u32)motor->measured_rpm
			: (u32)(-motor->measured_rpm);
		motor->period_us = 60000000 / abs_rpm;
	} else {
		motor->period_us = 0;
	}

	motor->status = (motor->measured_rpm != 0)
		? POV_STATUS_RUNNING : POV_STATUS_STOPPED;

}

void motor_simulation(struct tim_task *task)
{
	struct motor_controller *motor = container_of(task, struct motor_controller, sim_task);
	struct pov_stator *stator = container_of(motor, struct pov_stator, motor);

	motor_simulation_step(motor);

	if (motor->period_us > 0) {
		tim_task_set_interval(&stator->hall->fake_pulse_task, motor->period_us);
		tim_task_resume(&stator->hall->fake_pulse_task);
	} else {
		tim_task_suspend(&stator->hall->fake_pulse_task);
	}
}
