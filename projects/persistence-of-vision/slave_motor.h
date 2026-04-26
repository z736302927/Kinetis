/**
 * @file slave_motor.h
 * @brief Motor control interface for slave (fixed end)
 * @note Manages PWM output, direction GPIO, and Hall sensor RPM measurement
 */

#ifndef POV_SLAVE_MOTOR_H
#define POV_SLAVE_MOTOR_H

#include <linux/types.h>
#include "slave_config.h"

/*********************************************************************
 * Slave Motor Device Structure
 *********************************************************************/

struct slave_motor_device {
	u8 initialized;
	float pwm_duty;            /* Current PWM duty cycle (0~100%) */
	u8 direction;              /* 0=reverse/stop, 1=forward */
	s32 measured_rpm;          /* Measured RPM from hall sensor */
	s32 target_rpm;            /* Target RPM */

	/* Hall sensor state */
	u32 hall_last_tick;        /* Last hall pulse timer value */
	u32 hall_period_ticks;     /* Ticks between hall pulses */
	u32 hall_count;            /* Total hall pulse count */
	u64 hall_last_ms;          /* Last hall pulse timestamp (ms) */

#if KINETIS_FAKE_SIM
	/* Simulation state */
	u8 sim_running;
	float sim_current_rpm;     /* Simulated RPM that tracks PWM */
	pthread_t sim_thread;
#endif
};

/*********************************************************************
 * Public API
 *********************************************************************/

/**
 * @brief Initialize motor control
 * @param dev: Device pointer
 * @return 0 on success, negative error code on failure
 * @note Configures PWM timer, direction GPIO, and hall input capture
 */
int slave_motor_init(struct slave_motor_device *dev);

/**
 * @brief Set motor PWM duty cycle
 * @param dev: Device pointer
 * @param duty: Duty cycle 0.0~100.0 (%)
 * @return 0 on success, negative error code on failure
 */
int slave_motor_set_pwm(struct slave_motor_device *dev, float duty);

/**
 * @brief Set motor direction
 * @param dev: Device pointer
 * @param forward: 1=forward, 0=reverse/stop
 * @return 0 on success
 */
int slave_motor_set_direction(struct slave_motor_device *dev, u8 forward);

/**
 * @brief Stop motor (duty=0, direction=stop)
 * @param dev: Device pointer
 */
void slave_motor_stop(struct slave_motor_device *dev);

/**
 * @brief Get measured RPM from hall sensor
 * @param dev: Device pointer
 * @return Measured RPM, or 0 if no hall pulse detected
 */
s32 slave_motor_get_rpm(struct slave_motor_device *dev);

/**
 * @brief Hall sensor ISR callback
 * @param dev: Device pointer
 * @note Called from timer input capture ISR on hall pulse
 */
void slave_motor_hall_isr(struct slave_motor_device *dev);

/**
 * @brief Check if motor is running (hall pulses detected recently)
 * @param dev: Device pointer
 * @return 1 if running, 0 if stopped/stalled
 */
int slave_motor_is_running(struct slave_motor_device *dev);

/**
 * @brief Deinitialize motor control
 * @param dev: Device pointer
 */
void slave_motor_deinit(struct slave_motor_device *dev);

#endif /* POV_SLAVE_MOTOR_H */
