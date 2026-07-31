#ifndef POV_STATOR_H
#define POV_STATOR_H

#include <linux/types.h>

#include <kinetis/tim-task.h>
#include <kinetis/mavlink.h>

#include "motor.h"
#include "pid.h"
#include "hall.h"

struct flash_ops;
struct bootloader_ops;

#define STATOR_MOTOR_ID      0       /* Single motor ID */
#define STATOR_HALL_INDEX    0       /* Single hall sensor */

enum pov_stator_state {
	STATOR_INIT = 0,
	STATOR_TUNING_MOTOR,         /* Phase 1: default PID → Phase 2: relay auto-tuning */
	STATOR_WAIT_ROTOR,
	STATOR_ACTIVE,
	STATOR_SAFE_STOP,
	STATOR_ENTER_BOOTLOADER,
};

static inline const char *stator_state_name(enum pov_stator_state s)
{
	static const char *names[] = {
		"INIT", "TUNING_MOTOR", "WAIT_ROTOR", "ACTIVE", "SAFE_STOP",
		"ENTER_BOOTLOADER"
	};
	if ((unsigned)s >= sizeof(names) / sizeof(names[0])) {
		return "?";
	}
	return names[s];
}

struct pov_stator {
	struct serial_port *motor_port;
	struct mavlink_device *mavlink;
	struct hall_device *hall;
	struct motor_controller motor;
	struct pid_positional pid;
	struct pid_relay_tuner relay_tuner;

	u32 uptime_s;

	enum pov_stator_state state;
	u64 last_rotor_heartbeat_ms;    /* Last time rotor heartbeat received */

	/* ── TUNING sub-state tracking ─────────────────── */
	u32 tuning_stable_ticks;        /* Consecutive ticks where |err| < threshold */

	struct flash_ops *flash;
	struct bootloader_ops *bl_ops;

	struct tim_task heartbeat_task; /* Periodic heartbeat send (5s intervals via POV_HEARTBEAT_INTERVAL_MS) */
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
 *   5. PID controller init (default gains from config.h)
 *   6. Relay tuner init
 *   7. State machine starts at STATOR_INIT → STATOR_TUNING_MOTOR
 *
 * State machine:
 *   INIT → TUNING_MOTOR → WAIT_ROTOR → ACTIVE → SAFE_STOP
 *           (PID + relay auto-tuning, two phases inside single state)
 *
 *   TUNING_MOTOR (phase 1): Default-gain PID brings motor to target RPM.
 *                           Once |err| < threshold for N consecutive ticks,
 *                           relay tuner starts.
 *   TUNING_MOTOR (phase 2): Bang-bang oscillation, collect Ku/Tu,
 *                           compute Z-N gains, write to PID, reset PID,
 *                           stop motor → WAIT_ROTOR.
 *   ACTIVE:                 PID runs with tuned gains (or defaults if tuning skipped).
 *
 * Periodic tasks:
 *   - heartbeat_task (5s): send heartbeat
 *   - process_cmd_task (100ms): receive MAVLink commands
 *   - pid_task (10ms): PID/relay control (TUNING_MOTOR, ACTIVE)
 *
 * @return Pointer to allocated stator instance, or NULL on error
 */
struct pov_stator *pov_stator_alloc(struct serial_port_ops *motor_serial_ops,
	struct flash_ops *flash, struct bootloader_ops *bl_ops,
	u32(*read_rotated_time)(struct hall_device *dev));

void pov_stator_free(struct pov_stator *stator);

void pov_stator_fsm(struct pov_stator *stator);

#endif /* POV_STATOR_H */
