#include <linux/kernel.h>
#include <linux/printk.h>
#include <linux/delay.h>
#include <linux/string.h>
#include <linux/errno.h>
#include <linux/slab.h>

#include "config.h"
#include "stator.h"
#include "motor.h"

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

	output = pid_update(&stator->pid, SLAVE_TARGET_RPM - stator->motor.measured_rpm);
	motor_set_pwm(&stator->motor, output);
	motor_set_direction(&stator->motor, SLAVE_MOTOR_DIR_FORWARD);
}

static void process_motor_commands(struct tim_task *task)
{
	struct pov_stator *stator = container_of(task, struct pov_stator, process_cmd_task);

	/* Sync mavlink motor state with actual motor for status query responses */
	stator->mavlink->motors[STATOR_MOTOR_ID].current_speed = stator->motor.measured_rpm;
	stator->mavlink->motors[STATOR_MOTOR_ID].direction = stator->motor.direction;
	stator->mavlink->motors[STATOR_MOTOR_ID].status = stator->motor.status;

	if (serial_port_data_available(stator->motor_port) > 0) {
		mavlink_receive_and_process(stator->mavlink, 10);
	}
}

struct pov_stator *pov_stator_alloc(void)
{
	struct pov_stator *stator;
	int ret;

	stator = kzalloc(sizeof(*stator), GFP_KERNEL);
	if (!stator) {
		return NULL;
	}

	stator->motor_port = serial_port_alloc(&fake_serial_port_ops, "stator-motor");
	if (!stator->motor_port) {
		goto err;
	}

	stator->mavlink = mavlink_init(stator->motor_port, 1, 1, "stator-mavlink");
	if (!stator->mavlink) {
		goto err_free_port;
	}

	stator->hall = hall_alloc_dev(POV_DISPLAY_COLS, hall_read_rotated_time);
	if (!stator->hall) {
		goto err_free_mavlink;
	}

	pid_init(&stator->pid, SLAVE_PID_KP, SLAVE_PID_KI, SLAVE_PID_KD);

	tim_task_add(&stator->report_task, "report_task motor status",
		1000, true, false, report_motor_status);

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
