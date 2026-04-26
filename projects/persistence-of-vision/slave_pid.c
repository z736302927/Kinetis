/**
 * @file slave_pid.c
 * @brief PID controller implementation for motor speed regulation
 * @note Position-form PID with anti-windup and output clamping
 */

#include <linux/kernel.h>
#include <linux/printk.h>
#include <linux/errno.h>
#include <linux/string.h>

#include "slave_pid.h"
#include "slave_config.h"

/*********************************************************************
 * Initialization
 *********************************************************************/

int slave_pid_init(struct slave_pid_device *dev,
	float kp, float ki, float kd)
{
	if (!dev)
		return -EINVAL;

	memset(dev, 0, sizeof(*dev));

	dev->kp = kp;
	dev->ki = ki;
	dev->kd = kd;
	dev->integral = 0.0f;
	dev->integral_max = SLAVE_PID_INTEGRAL_MAX;
	dev->last_error = 0.0f;
	dev->output = 0.0f;
	dev->output_max = SLAVE_PID_OUTPUT_MAX;
	dev->output_min = SLAVE_PID_OUTPUT_MIN;
	dev->initialized = 1;

	pr_info("Slave PID: initialized (Kp=%.2f, Ki=%.2f, Kd=%.2f)\n",
		kp, ki, kd);
	return 0;
}

/*********************************************************************
 * Reset
 *********************************************************************/

void slave_pid_reset(struct slave_pid_device *dev)
{
	if (!dev)
		return;

	dev->integral = 0.0f;
	dev->last_error = 0.0f;
	dev->output = 0.0f;

	pr_debug("Slave PID: reset\n");
}

/*********************************************************************
 * PID Calculation
 *********************************************************************/

float slave_pid_calculate(struct slave_pid_device *dev,
	float setpoint, float feedback)
{
	if (!dev || !dev->initialized)
		return 0.0f;

	float error = setpoint - feedback;

	/* Proportional term */
	float p_term = dev->kp * error;

	/* Integral term with anti-windup */
	dev->integral += error;

	/* Clamp integral */
	if (dev->integral > dev->integral_max)
		dev->integral = dev->integral_max;
	else if (dev->integral < -dev->integral_max)
		dev->integral = -dev->integral_max;

	float i_term = dev->ki * dev->integral;

	/* Derivative term (on error) */
	float d_term = dev->kd * (error - dev->last_error);
	dev->last_error = error;

	/* Calculate output */
	dev->output = p_term + i_term + d_term;

	/* Clamp output */
	if (dev->output > dev->output_max)
		dev->output = dev->output_max;
	else if (dev->output < dev->output_min)
		dev->output = dev->output_min;

	return dev->output;
}

/*********************************************************************
 * Configuration
 *********************************************************************/

void slave_pid_set_limits(struct slave_pid_device *dev,
	float min, float max)
{
	if (!dev)
		return;

	dev->output_min = min;
	dev->output_max = max;
}

void slave_pid_set_integral_limit(struct slave_pid_device *dev, float max)
{
	if (!dev)
		return;

	dev->integral_max = max;
}

float slave_pid_get_output(struct slave_pid_device *dev)
{
	if (!dev)
		return 0.0f;

	return dev->output;
}
