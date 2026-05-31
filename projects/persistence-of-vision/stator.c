#include <linux/kernel.h>
#include <linux/printk.h>
#include <linux/delay.h>
#include <linux/string.h>
#include <linux/errno.h>
#include <linux/slab.h>

#include "config.h"
#include "stator.h"
#include "motor.h"
#include "interface.h"

static void report_motor_status(struct tim_task *task)
{
	struct pov_stator *stator = container_of(task, struct pov_stator, report_task);

	stator->uptime_s++;

	mavlink_send_heartbeat(stator->mavlink, MAV_SYS_ACTIVE,
		stator->uptime_s, stator->mavlink->error_count);
}

static void control_motor(struct tim_task *task)
{
	struct pov_stator *stator = container_of(task, struct pov_stator, pid_task);
	s32 output;

	output = pid_update(&stator->pid, stator->mavlink->motors[STATOR_MOTOR_ID].target_speed - hall_get_rpm(stator->hall));
	motor_set_pwm(&stator->motor, output);
	motor_set_direction(&stator->motor, stator->mavlink->motors[STATOR_MOTOR_ID].direction);
}

static void process_motor_commands(struct tim_task *task)
{
	struct pov_stator *stator = container_of(task, struct pov_stator, process_cmd_task);
	u32 old_rx = stator->mavlink->rx_count;

	/* Process pending MAVLink messages (motor control commands from rotor) */
	if (serial_port_data_available(stator->motor_port) > 0) {
		mavlink_receive_and_process(stator->mavlink, 10);
	}

	/* Only dispatch if a new message was received */
	if (stator->mavlink->rx_count == old_rx) {
		return;
	}

	switch (stator->mavlink->rx_msg.msgid) {
	case MAVLINK_MSG_ID_MOTOR_CONTROL: {
		break;
	}

	case MAVLINK_MSG_ID_MOTOR_STATUS_QUERY: {
		mavlink_motor_status_query_t query;

		mavlink_msg_motor_status_query_decode(&stator->mavlink->rx_msg, &query);
		if (query.motor_id >= MAVLINK_MOTOR_COUNT &&
			query.motor_id != 255) {
			pr_err("stator: invalid motor_id=%u\n", query.motor_id);
			break;
		}

		mavlink_send_motor_status(stator->mavlink, query.motor_id,
			hall_get_rpm(stator->hall), stator->motor.direction, stator->motor.status);
		break;
	}

	default:
		pr_debug("stator: unhandled msgid=%u\n",
			stator->mavlink->rx_msg.msgid);
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

	tim_task_add(&stator->report_task, "report_task motor status",
		10000, true, false, report_motor_status);

	tim_task_add(&stator->pid_task, "pid_task",
		10, true, false, control_motor);

	tim_task_add(&stator->process_cmd_task, "process_cmd_task",
		100, true, false, process_motor_commands);

#if KINETIS_FAKE_SIM
	motor_sim_start(&stator->motor);
#endif

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

#if KINETIS_FAKE_SIM
	motor_sim_stop(&stator->motor);
#endif

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
