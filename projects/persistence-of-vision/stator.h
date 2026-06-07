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

enum pov_stator_state {
	STATOR_INIT = 0,       /* Hardware initialized, waiting for first heartbeat task run */
	STATOR_WAIT_ROTOR,     /* Waiting for rotor heartbeat, motor stopped */
	STATOR_ACTIVE,         /* Normal operation, PID active */
	STATOR_SAFE_STOP,      /* Rotor heartbeat lost, motor coasts to stop (PWM=0) */
};

static inline const char *stator_state_name(enum pov_stator_state s)
{
	static const char *names[] = {
		"INIT", "WAIT_ROTOR", "ACTIVE", "SAFE_STOP"
	};
	if ((unsigned)s >= sizeof(names) / sizeof(names[0]))
		return "?";
	return names[s];
}

struct pov_stator {
	struct serial_port *motor_port;
	struct mavlink_device *mavlink;
	struct hall_device *hall;
	struct motor_controller motor;
	struct pid_controller pid;

	u32 uptime_s;

	enum pov_stator_state state;
	u64 last_rotor_heartbeat_ms;    /* Last time rotor heartbeat received */

	struct tim_task heartbeat_task; /* Periodic heartbeat send + check (1s) */
	struct tim_task process_cmd_task;
	struct tim_task pid_task;
};

/**
 * @brief Main entry point for stator program
 *
 * Initialization sequence:
 *   1. Serial port init
 *   2. MAVLink init
 *   3. Motor + simulation thread start
 *   4. Hall sensor init
 *   5. PI controller init
 *   6. State machine starts at STATOR_INIT → STATOR_WAIT_ROTOR
 *
 * State machine:
 *   INIT → WAIT_ROTOR → ACTIVE → SAFE_STOP → WAIT_ROTOR
 *   PID only runs in ACTIVE; motor stopped in all other states.
 *
 * Periodic tasks:
 *   - heartbeat_task (1s): send heartbeat + check rotor heartbeat timeout
 *   - process_cmd_task (100ms): receive MAVLink commands
 *   - pid_task (10ms): PI control (ACTIVE only)
 *
 * @return Pointer to allocated stator instance, or NULL on error
 */
struct pov_stator *pov_stator_alloc(struct serial_port_ops *motor_serial_ops,
	u32(*read_rotated_time)(struct hall_device *dev));

/**
 * @brief Free stator instance and all owned resources
 * @param stator: Stator instance to free (NULL-safe)
 */
void pov_stator_free(struct pov_stator *stator);

#endif /* POV_STATOR_H */
