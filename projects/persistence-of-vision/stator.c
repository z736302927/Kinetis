#include <linux/kernel.h>
#include <linux/printk.h>
#include <linux/delay.h>
#include <linux/string.h>
#include <linux/errno.h>
#include <linux/slab.h>

#include <kinetis/basic-timer.h>

#include "config.h"
#include "stator.h"
#include "motor.h"
#include "interface.h"

static void set_stator_state(struct pov_stator *stator,
			     enum pov_stator_state new_state)
{
	if (new_state != stator->state) {
		pr_info("state %s -> %s\n",
			stator_state_name(stator->state),
			stator_state_name(new_state));
	}
	stator->state = new_state;
}

static void stator_heartbeat_task(struct tim_task *task)
{
	struct pov_stator *stator = container_of(task, struct pov_stator, heartbeat_task);

	stator->uptime_s++;

	/* Always send heartbeat so rotor can detect us */
	mavlink_send_heartbeat(stator->mavlink, MAV_SYS_ACTIVE,
		 stator->uptime_s, stator->mavlink->error_count);

	switch (stator->state) {
	case STATOR_INIT:
		/* First heartbeat task run: start handshake, motor off */
		motor_set_pwm(&stator->motor, 0);
		set_stator_state(stator, STATOR_WAIT_ROTOR);
		break;

	case STATOR_ACTIVE: {
		/* Check if rotor heartbeat has timed out */
		u64 now = basic_timer_get_ms();

		if (now - stator->last_rotor_heartbeat_ms > (u64)POV_HEARTBEAT_INTERVAL_MS) {
			pr_warn("rotor heartbeat timeout (%llu ms), safe stop (PWM=0, coast by inertia)\n",
				now - stator->last_rotor_heartbeat_ms);
			motor_set_pwm(&stator->motor, 0);
			pid_reset(&stator->pid);
			stator->mavlink->motors[STATOR_MOTOR_ID].target_speed = 0;
			set_stator_state(stator, STATOR_SAFE_STOP);
		}
		break;
	}

	case STATOR_SAFE_STOP:
		/* Still sending heartbeats, waiting for rotor to reconnect.
		 * Motor is already stopped. When we detect rotor heartbeat
		 * again (in process_cmd_task), we go back to WAIT_ROTOR. */
		break;

	case STATOR_WAIT_ROTOR:
		/* Already sending heartbeats, motor stopped.
		 * Transition to ACTIVE happens in process_cmd_task
		 * when rotor heartbeat is first received. */
		break;
	}
}

static void control_motor(struct tim_task *task)
{
	struct pov_stator *stator = container_of(task, struct pov_stator, pid_task);
	s32 output;

	if (stator->state != STATOR_ACTIVE)
		return;

	output = pid_update(&stator->pid, stator->mavlink->motors[STATOR_MOTOR_ID].target_speed - hall_get_rpm(stator->hall));
	motor_set_pwm(&stator->motor, output);
	motor_set_direction(&stator->motor, stator->mavlink->motors[STATOR_MOTOR_ID].direction);
}

static void process_motor_commands(struct tim_task *task)
{
	struct pov_stator *stator = container_of(task, struct pov_stator, process_cmd_task);
	u32 old_rx = stator->mavlink->rx_count;

	/* Process pending MAVLink messages (motor control + heartbeat from rotor) */
	if (serial_port_data_available(stator->motor_port) > 0) {
		mavlink_receive_and_process(stator->mavlink, 10);
	}

	if (stator->mavlink->rx_count > old_rx) {
		/* If any new message arrived and peer reports ACTIVE status,
		* the peer is alive. sys_status is updated by
		* mavlink_receive_and_process() when it decodes a HEARTBEAT.
		* Using sys_status is robust against batch messages where
		* the last rx_msg.msgid may not be HEARTBEAT. */
		if (stator->mavlink->sys_status == MAV_SYS_ACTIVE) {
			stator->last_rotor_heartbeat_ms = basic_timer_get_ms();

			/* Rotor came back: transition out of SAFE_STOP or
			* first handshake: transition out of WAIT_ROTOR */
			if (stator->state == STATOR_SAFE_STOP ||
				stator->state == STATOR_WAIT_ROTOR) {
				pr_info("rotor heartbeat detected, entering WAIT_ROTOR handshake\n");
				set_stator_state(stator, STATOR_WAIT_ROTOR);
			}
		}

		/* ── In WAIT_ROTOR: first heartbeat triggers ACTIVE ───── */
		/* (transition happens after TWO heartbeats: first sets
		* last_rotor_heartbeat_ms and moves to WAIT_ROTOR,
		* second confirms rotor is stable and moves to ACTIVE) */
		if (stator->state == STATOR_WAIT_ROTOR) {
			u64 now = basic_timer_get_ms();

			/* Only transition to ACTIVE if we recently received a rotor
			* heartbeat (within the last heartbeat interval) */
			if (now - stator->last_rotor_heartbeat_ms < (u64)POV_HEARTBEAT_INTERVAL_MS) {
				pr_info("handshake complete, entering ACTIVE\n");
				set_stator_state(stator, STATOR_ACTIVE);
			}
		}
	} else if (stator->mavlink->rx_count == old_rx) {
		return;
	}

	switch (stator->mavlink->rx_msg.msgid) {
	case MAVLINK_MSG_ID_SYSTEM_HEARTBEAT:
		break;

	case MAVLINK_MSG_ID_MOTOR_CONTROL:
		break;

	case MAVLINK_MSG_ID_MOTOR_STATUS_QUERY: {
		mavlink_motor_status_query_t query;

		mavlink_msg_motor_status_query_decode(&stator->mavlink->rx_msg, &query);
		if (query.motor_id >= MAVLINK_MOTOR_COUNT &&
		    query.motor_id != 255) {
			pr_err("invalid motor_id=%u\n", query.motor_id);
			break;
		}

		mavlink_send_motor_status(stator->mavlink, query.motor_id,
			hall_get_rpm(stator->hall), stator->motor.direction, stator->motor.status);
		break;
	}

	default:
		pr_debug("unhandled msgid=%u\n", stator->mavlink->rx_msg.msgid);
		break;
	}
}

struct pov_stator *pov_stator_alloc(struct serial_port_ops *motor_serial_ops,
	u32(*read_rotated_time)(struct hall_device *dev))
{
	struct pov_stator *stator;
	int ret;

	stator = kzalloc(sizeof(*stator), GFP_KERNEL);
	if (!stator) {
		return NULL;
	}

	stator->motor_port = serial_port_alloc(motor_serial_ops, "stator-motor");
	if (!stator->motor_port) {
		goto err;
	}

	stator->mavlink = mavlink_init(stator->motor_port, 1, 1, "stator-mavlink");
	if (!stator->mavlink) {
		goto err_free_port;
	}

	stator->hall = hall_alloc_dev(POV_DISPLAY_COLS, read_rotated_time);
	if (!stator->hall) {
		goto err_free_mavlink;
	}

	tim_task_add(&stator->hall->fake_pulse_task, "fake_pulse_task",
		67, true, false, hall_fake_pulse);

	tim_task_add(&stator->motor.sim_task, "motor_sim_task",
		SLAVE_PID_PERIOD_MS, true, false, motor_simulation);

	pid_init(&stator->pid, SLAVE_PID_KP, SLAVE_PID_KI, SLAVE_PID_KD);

	stator->motor.max_rpm = SLAVE_MOTOR_MAX_RPM;

	stator->state = STATOR_INIT;
	stator->last_rotor_heartbeat_ms = basic_timer_get_ms();

	tim_task_add(&stator->heartbeat_task, "stator heartbeat",
		POV_HEARTBEAT_INTERVAL_MS, true, false, stator_heartbeat_task);

	tim_task_add(&stator->pid_task, "pid_task",
		10, true, false, control_motor);

	tim_task_add(&stator->process_cmd_task, "process_cmd_task",
		100, true, false, process_motor_commands);

	stator_heartbeat_task(&stator->heartbeat_task);

	return stator;

err_free_mavlink:
	mavlink_free(stator->mavlink);
err_free_port:
	serial_port_free(stator->motor_port);
err:
	kfree(stator);

	return NULL;
}

void pov_stator_free(struct pov_stator *stator)
{
	if (!stator) {
		return;
	}

	if (stator->mavlink) {
		mavlink_free(stator->mavlink);
	}

	if (stator->hall) {
		kfree(stator->hall);
	}

	if (stator->motor_port) {
		serial_port_free(stator->motor_port);
	}

	kfree(stator);
}
