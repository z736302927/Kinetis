#ifndef POV_STATOR_H
#define POV_STATOR_H

#include <linux/types.h>

#include "kinetis/tim-task.h"
#include <kinetis/mavlink.h>

#include "motor.h"
#include "pid.h"
#include "hall.h"

/**
 * @file stator.h
 * @brief POV Stator (Fixed End) main entry
 *
 * Stator runs on STM32F103, receives speed commands from rotor via
 * MAVLink, and controls motor via PI controller.
 */

#define STATOR_MOTOR_ID      0       /* Single motor ID */
#define STATOR_HALL_INDEX    0       /* Single hall sensor */

struct pov_stator {
	struct serial_port *motor_port;
	struct mavlink_device *mavlink;
	struct hall_device *hall;
	struct motor_controller motor;
	struct pid_controller pid;

	u32 uptime_s;

	struct tim_task report_task;
	struct tim_task process_cmd_task;
	struct tim_task pid_task;
};

/**
 * @brief Main entry point for stator program
 *
 * Initialization sequence:
 *   1. Serial port init
 *   2. MAVLink init + RX thread start
 *   3. Motor + simulation thread start
 *   4. Hall sensor init
 *   5. PI controller init
 *
 * Control loop (10ms period):
 *   - Read target_rpm from mavlink dev->motors[0].target_speed
 *   - Read measured_rpm from motor->measured_rpm
 *   - PI update -> PWM duty
 *   - motor_set_pwm()
 *   - Check stability
 *   - Send motor_status periodically
 *   - Send heartbeat
 *
 * @return Pointer to allocated stator instance, or NULL on error
 */
struct pov_stator *pov_stator_alloc(void);

/**
 * @brief Free stator instance and all owned resources
 * @param stator: Stator instance to free (NULL-safe)
 */
void pov_stator_free(struct pov_stator *stator);

#endif /* POV_STATOR_H */
