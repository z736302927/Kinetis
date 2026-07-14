#include <linux/kernel.h>
#include <linux/printk.h>
#include <linux/delay.h>
#include <linux/string.h>
#include <linux/errno.h>
#include <linux/slab.h>

#include <kinetis/basic-timer.h>
#include "kinetis/bootloader.h"
#include "kinetis/nrf24l01_port.h"

#include "fatfs/ff.h"

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
}

static void control_motor(struct tim_task *task)
{
	struct pov_stator *stator = container_of(task, struct pov_stator, pid_task);
	s32 set_point, meas, output;

	if (stator->state != STATOR_TUNING_MOTOR &&
	    stator->state != STATOR_ACTIVE)
		return;

	set_point = stator->mavlink->motors[STATOR_MOTOR_ID].target_speed;
	meas      = stator->motor.measured_rpm;   /* signed — motor supports fwd/rev */

	/*
	 * Within TUNING_MOTOR, the relay_tuner.state discriminates
	 * phase 1 (default PID) from phase 2 (bang-bang relay).
	 */
	if (stator->state == STATOR_TUNING_MOTOR &&
	    stator->relay_tuner.state == RELAY_RUNNING) {
		output = pid_relay_update(&stator->relay_tuner, meas);
	} else {
		output = pid_positional_update(&stator->pid, set_point, meas, SLAVE_PID_PERIOD_MS);
	}

	/* Bidirectional: signed output → PWM magnitude + direction */
	if (output >= 0) {
		motor_set_pwm(&stator->motor, (u16)output);
		motor_set_direction(&stator->motor, SLAVE_MOTOR_DIR_FORWARD);
	} else {
		motor_set_pwm(&stator->motor, (u16)(-output));
		motor_set_direction(&stator->motor, SLAVE_MOTOR_DIR_REVERSE);
	}
}

static void process_motor_commands(struct tim_task *task)
{
	struct pov_stator *stator = container_of(task, struct pov_stator, process_cmd_task);
	u32 old_rx = stator->mavlink->rx_count;

	/* Process pending MAVLink messages (motor control + heartbeat from rotor) */
#if POV_USE_NRF24L01
	nrf24l01_port_poll(stator->motor_port);
#endif
	if (serial_port_data_available(stator->motor_port) > 0) {
		mavlink_receive_and_process(stator->mavlink, 10);
	}

	if (stator->mavlink->rx_count > old_rx) {
		/* Any MAVLink packet from rotor indicates the link is alive.
		 * No need to wait specifically for HEARTBEAT. */
		stator->last_rotor_heartbeat_ms = basic_timer_get_ms();

		/* Rotor came back: transition out of SAFE_STOP or
		 * first handshake: transition out of WAIT_ROTOR */
		if (stator->state == STATOR_SAFE_STOP ||
			stator->state == STATOR_WAIT_ROTOR) {
			pr_info("rotor link detected, entering WAIT_ROTOR handshake\n");
			set_stator_state(stator, STATOR_WAIT_ROTOR);
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

	case MAVLINK_MSG_ID_BOOTLOADER_UPDATE_CMD: {
		struct bootloader_update_flag flag;
		FIL fp;
		UINT bw;
		FRESULT res;

		pr_info("cmd: bootloader update - cmd=%d size=%u crc=0x%08x name=%s\n",
			stator->mavlink->bl.command,
			stator->mavlink->bl.firmware_size,
			stator->mavlink->bl.firmware_crc32,
			stator->mavlink->bl.filename);

		/* Write update flag file on FatFS so bootloader picks it up after reset */
		memset(&flag, 0, sizeof(flag));
		flag.magic = BL_UPDATE_FLAG_MAGIC;
		flag.firmware_size = stator->mavlink->bl.firmware_size;
		flag.firmware_crc32 = stator->mavlink->bl.firmware_crc32;
		if (stator->mavlink->bl.filename[0])
			strncpy(flag.firmware_filename, stator->mavlink->bl.filename,
				sizeof(flag.firmware_filename) - 1);

		if (f_open(&fp, BL_UPDATE_FLAG_PATH, FA_CREATE_ALWAYS | FA_WRITE) != FR_OK) {
			pr_err("Failed to create update flag file\n");
			break;
		}
		res = f_write(&fp, &flag, sizeof(flag), &bw);
		if (res != FR_OK || bw != sizeof(flag)) {
			pr_err("Failed to write update flag file\n");
			f_close(&fp);
			break;
		}
		f_close(&fp);

		set_stator_state(stator, STATOR_ENTER_BOOTLOADER);
		break;
	}

	default:
		pr_debug("unhandled msgid=%u\n", stator->mavlink->rx_msg.msgid);
		break;
	}
}

struct pov_stator *pov_stator_alloc(struct serial_port_ops *motor_serial_ops,
	struct flash_ops *flash, struct bootloader_ops *bl_ops,
	u32(*read_rotated_time)(struct hall_device *dev))
{
	struct pov_stator *stator;
	int ret;

	stator = kzalloc(sizeof(*stator), GFP_KERNEL);
	if (!stator) {
		return NULL;
	}

	stator->flash = flash;
	stator->bl_ops = bl_ops;

	stator->motor_port = serial_port_alloc(motor_serial_ops, "stator-motor");
	if (!stator->motor_port) {
		goto err;
	}

	stator->mavlink = mavlink_init(stator->motor_port, 2, 1, "stator-mavlink");
	if (!stator->mavlink) {
		goto err_free_port;
	}

	stator->hall = hall_alloc_dev(POV_DISPLAY_COLS, read_rotated_time);
	if (!stator->hall) {
		goto err_free_mavlink;
	}

	stator->motor.max_rpm = SLAVE_MOTOR_MAX_RPM;
	stator->state = STATOR_INIT;
	stator->last_rotor_heartbeat_ms = basic_timer_get_ms();

	pid_positional_init(&stator->pid, SLAVE_PID_KP, SLAVE_PID_KI, SLAVE_PID_KD);
	pid_relay_init(&stator->relay_tuner, &stator->pid,
		       SLAVE_PID_RELAY_AMP, 0, POV_TARGET_RPM, SLAVE_PID_RELAY_CYCLES);

	stator->tuning_stable_ticks = 0;

	tim_task_add(&stator->hall->fake_pulse_task, "fake_pulse_task",
		66667, true, false, hall_fake_pulse);

	tim_task_add(&stator->motor.sim_task, "motor_sim_task",
		SLAVE_PID_PERIOD_MS * 1000, true, false, motor_simulation);

	tim_task_add(&stator->pid_task, "pid_task",
		SLAVE_PID_PERIOD_MS * 1000, true, false, control_motor);

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

void pov_stator_fsm(struct pov_stator *stator)
{
	s32 err;

	switch (stator->state) {
	case STATOR_INIT:
		motor_set_pwm(&stator->motor, 0);
		motor_set_direction(&stator->motor, SLAVE_MOTOR_DIR_FORWARD);
		stator->mavlink->motors[STATOR_MOTOR_ID].target_speed = POV_TARGET_RPM;
		set_stator_state(stator, STATOR_TUNING_MOTOR);
		break;

	case STATOR_TUNING_MOTOR: {
		static u64 last_print;
		u64 now = basic_timer_get_ms();
		s32 rpm = stator->motor.measured_rpm;  /* signed, supports fwd/rev */

		stator->mavlink->motors[STATOR_MOTOR_ID].target_speed = POV_TARGET_RPM;

		if (now - last_print >= 1000) {
			pr_info("current rpm: %d\n", rpm);
			last_print = now;
		}

		/*
		 * Phase 1 — stabilise with default PID, then trigger relay.
		 * Phase 2 — wait for relay tuner to finish (control_motor()
		 *           feeds pid_relay_update() each tick; tuner writes
		 *           Kp/Ki/Kd and sets state=RELAY_DONE/RELAY_FAILED
		 *           autonomously).
		 */
		if (stator->relay_tuner.state == RELAY_IDLE) {
			err = POV_TARGET_RPM - rpm;
			if (err >= -SLAVE_TUNING_STABLE_THRESH &&
			    err <= SLAVE_TUNING_STABLE_THRESH) {
				stator->tuning_stable_ticks++;
				if (stator->tuning_stable_ticks >= SLAVE_TUNING_STABLE_TICKS) {
					pr_info("motor stable at err=%d, starting relay tuning\n", err);
					pid_relay_start(&stator->relay_tuner);
				}
			} else {
				stator->tuning_stable_ticks = 0;
			}
		} else if (stator->relay_tuner.state == RELAY_RUNNING) {
			/* Still running, wait for completion */
		} else {
			/* RELAY_DONE or RELAY_FAILED */
			if (stator->relay_tuner.state == RELAY_DONE) {
				pr_info("relay tuning done, gains written. Stopping motor.\n");
			} else {
				pr_warn("relay tuning failed, keeping default gains. Stopping motor.\n");
			}
			motor_set_pwm(&stator->motor, 0);
			pid_positional_reset(&stator->pid);
			stator->mavlink->motors[STATOR_MOTOR_ID].target_speed = 0;

			stator->motor_port->ops->enable();

			tim_task_add(&stator->heartbeat_task, "stator heartbeat",
				POV_HEARTBEAT_INTERVAL_MS * 1000, true, false, stator_heartbeat_task);

			tim_task_add(&stator->process_cmd_task, "process_cmd_task",
				100000, true, false, process_motor_commands);

			set_stator_state(stator, STATOR_WAIT_ROTOR);
		}
		break;
	}

	case STATOR_ACTIVE: {
		/* Check if rotor heartbeat has timed out */
		u64 now = basic_timer_get_ms();

		if (now - stator->last_rotor_heartbeat_ms > (u64)POV_HEARTBEAT_INTERVAL_MS * 2) {
			pr_warn("rotor heartbeat timeout (%llu ms), safe stop (PWM=0, coast by inertia)\n",
				now - stator->last_rotor_heartbeat_ms);
			motor_set_pwm(&stator->motor, 0);
			pid_positional_reset(&stator->pid);
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

	case STATOR_ENTER_BOOTLOADER:
		motor_set_pwm(&stator->motor, 0);

		if (stator->bl_ops && stator->bl_ops->system_reset) {
			stator->bl_ops->system_reset();
		} else {
			pr_err("No system_reset available, cannot enter bootloader!\n");
		}
		break;
	}
}
