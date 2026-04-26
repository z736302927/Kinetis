/**
 * @file slave_pid.h
 * @brief PID controller for slave motor speed regulation
 * @note Implements position-form PID with anti-windup and
 *       configurable output limits
 */

#ifndef POV_SLAVE_PID_H
#define POV_SLAVE_PID_H

#include <linux/types.h>

/*********************************************************************
 * PID Device Structure
 *********************************************************************/

struct slave_pid_device {
	float kp;              /* Proportional gain */
	float ki;              /* Integral gain */
	float kd;              /* Derivative gain */
	float integral;        /* Current integral accumulator */
	float integral_max;    /* Anti-windup integral limit */
	float last_error;      /* Previous error for derivative */
	float output;          /* Current PID output (0~100%) */
	float output_max;      /* Maximum output */
	float output_min;      /* Minimum output */
	u8 initialized;
};

/*********************************************************************
 * Public API
 *********************************************************************/

/**
 * @brief Initialize PID controller
 * @param dev: Device pointer
 * @param kp: Proportional gain
 * @param ki: Integral gain
 * @param kd: Derivative gain
 * @return 0 on success, negative error code on failure
 */
int slave_pid_init(struct slave_pid_device *dev,
	float kp, float ki, float kd);

/**
 * @brief Reset PID state (clear integral and last error)
 * @param dev: Device pointer
 * @note Call this when target speed changes to avoid integral windup
 */
void slave_pid_reset(struct slave_pid_device *dev);

/**
 * @brief Calculate PID output
 * @param dev: Device pointer
 * @param setpoint: Target value (RPM)
 * @param feedback: Current measured value (RPM)
 * @return PID output (0~100% duty cycle)
 */
float slave_pid_calculate(struct slave_pid_device *dev,
	float setpoint, float feedback);

/**
 * @brief Set output limits
 * @param dev: Device pointer
 * @param min: Minimum output
 * @param max: Maximum output
 */
void slave_pid_set_limits(struct slave_pid_device *dev,
	float min, float max);

/**
 * @brief Set integral anti-windup limit
 * @param dev: Device pointer
 * @param max: Maximum integral value
 */
void slave_pid_set_integral_limit(struct slave_pid_device *dev, float max);

/**
 * @brief Get current PID output
 * @param dev: Device pointer
 * @return Current output value
 */
float slave_pid_get_output(struct slave_pid_device *dev);

#endif /* POV_SLAVE_PID_H */
