/**
 * @file slave_main.c
 * @brief Slave (fixed end) main program
 * @note Implements PID-based motor speed control with MAVLink communication.
 *       Runs at 10ms PID period, sends heartbeat at 1Hz and status at 10Hz.
 */

#include <linux/kernel.h>
#include <linux/printk.h>
#include <linux/errno.h>
#include <linux/delay.h>

#include "slave_config.h"
#include "slave_mavlink.h"
#include "slave_pid.h"
#include "slave_motor.h"
#include "mavlink_config.h"

#include <kinetis/basic-timer.h>

/*********************************************************************
 * Slave Device Structure
 *********************************************************************/

struct slave_device {
	struct slave_mavlink_device mavlink;
	struct slave_pid_device pid;
	struct slave_motor_device motor;

	s32 target_rpm;            /* Current target RPM */
	u8 motor_enabled;          /* 1 if motor should be running */
	u16 stable_count;          /* Consecutive stable PID cycles */
	u8 is_stable;              /* 1 if RPM is stable */
	u64 last_pid_ms;           /* Last PID execution time */
	u64 last_heartbeat_ms;     /* Last heartbeat send time */
	u64 last_status_ms;        /* Last status send time */
};

static struct slave_device g_slave;

/*********************************************************************
 * Internal: Check and update stability
 *********************************************************************/

static void slave_update_stability(struct slave_device *dev)
{
	s32 rpm = slave_motor_get_rpm(&dev->motor);
	s32 diff = rpm - dev->target_rpm;
	if (diff < 0)
		diff = -diff;

	if (diff <= SLAVE_STABLE_RPM_THRESH && dev->target_rpm > 0) {
		dev->stable_count++;
		if (dev->stable_count >= SLAVE_STABLE_COUNT_REQ)
			dev->is_stable = 1;
	} else {
		dev->stable_count = 0;
		dev->is_stable = 0;
	}
}

/*********************************************************************
 * Internal: PID control cycle
 *********************************************************************/

static void slave_pid_cycle(struct slave_device *dev)
{
	s32 current_rpm = slave_motor_get_rpm(&dev->motor);
	float duty;

	if (dev->target_rpm <= 0) {
		/* Target is zero or negative - stop motor */
		duty = 0.0f;
		slave_motor_stop(&dev->motor);
		dev->stable_count = 0;
		dev->is_stable = 0;
		return;
	}

	/* Run PID calculation */
	duty = slave_pid_calculate(&dev->pid,
		(float)dev->target_rpm, (float)current_rpm);

	/* Apply to motor */
	slave_motor_set_pwm(&dev->motor, duty);

	if (!slave_motor_is_running(&dev->motor) && duty > 0) {
		/* Motor not running but should be - ensure direction is set */
		slave_motor_set_direction(&dev->motor, SLAVE_MOTOR_DIR_FORWARD);
	}
}

/*********************************************************************
 * Internal: MAVLink communication cycle
 *********************************************************************/

static void slave_mavlink_cycle(struct slave_device *dev)
{
	u64 now = basic_timer_get_ms();

	/* Process incoming messages (non-blocking) */
	slave_mavlink_process(&dev->mavlink, 1);

	/* Check for new speed command */
	if (slave_mavlink_has_new_command(&dev->mavlink)) {
		s32 new_rpm = slave_mavlink_get_target_rpm(&dev->mavlink);

		pr_info("Slave: new target RPM=%d (was %d)\n",
			new_rpm, dev->target_rpm);

		/* If target changed, reset PID */
		if (new_rpm != dev->target_rpm) {
			slave_pid_reset(&dev->pid);
			dev->stable_count = 0;
			dev->is_stable = 0;
		}

		dev->target_rpm = new_rpm;
		dev->motor_enabled = (new_rpm > 0) ? 1 : 0;
	}

	/* Send heartbeat at configured interval */
	if ((now - dev->last_heartbeat_ms) >= SLAVE_HEARTBEAT_MS) {
		slave_mavlink_send_heartbeat(&dev->mavlink);
		dev->last_heartbeat_ms = now;
	}

	/* Send status at configured interval */
	if ((now - dev->last_status_ms) >= SLAVE_STATUS_MS) {
		s32 rpm = slave_motor_get_rpm(&dev->motor);
		u8 status;

		if (!dev->motor_enabled || dev->target_rpm == 0)
			status = POV_STATUS_STOPPED;
		else if (dev->is_stable)
			status = POV_STATUS_RUNNING;
		else
			status = POV_STATUS_RUNNING;  /* Running but not yet stable */

		slave_mavlink_send_status(&dev->mavlink, rpm, status);
		dev->last_status_ms = now;
	}
}

/*********************************************************************
 * Slave Main Entry
 *********************************************************************/

/**
 * @brief Slave main function - runs the motor control loop
 * @return 0 on normal exit, negative error code on failure
 */
int slave_main(void)
{
	struct slave_device *dev = &g_slave;
	int ret;

	pr_info("==== POV Slave Starting ====\n");

	/* Initialize subsystems */
	ret = slave_mavlink_init(&dev->mavlink);
	if (ret < 0) {
		pr_err("Slave: MAVLink init failed: %d\n", ret);
		return ret;
	}

	ret = slave_pid_init(&dev->pid, SLAVE_PID_KP, SLAVE_PID_KI, SLAVE_PID_KD);
	if (ret < 0) {
		pr_err("Slave: PID init failed: %d\n", ret);
		return ret;
	}

	ret = slave_motor_init(&dev->motor);
	if (ret < 0) {
		pr_err("Slave: motor init failed: %d\n", ret);
		return ret;
	}

#if KINETIS_FAKE_SIM
	/* In simulation, use loopback serial port */
	serial_port_start_thread(dev->mavlink.serial,
		SERIAL_PORT_DF_SELF, NULL, dev->mavlink.serial);

	/* Start motor simulation thread */
	extern int slave_motor_sim_start(struct slave_motor_device *dev);
	slave_motor_sim_start(&dev->motor);
#endif

	/* Initialize timing */
	dev->target_rpm = 0;
	dev->motor_enabled = 0;
	dev->stable_count = 0;
	dev->is_stable = 0;
	dev->last_pid_ms = basic_timer_get_ms();
	dev->last_heartbeat_ms = basic_timer_get_ms();
	dev->last_status_ms = basic_timer_get_ms();

	pr_info("Slave: entering main loop (PID period=%dms)\n",
		SLAVE_PID_PERIOD_MS);

	/* Main loop */
	while (1) {
		u64 now = basic_timer_get_ms();

		/* Run PID at fixed interval */
		if ((now - dev->last_pid_ms) >= SLAVE_PID_PERIOD_MS) {
			slave_pid_cycle(dev);
			slave_update_stability(dev);
			dev->last_pid_ms = now;
		}

		/* MAVLink communication */
		slave_mavlink_cycle(dev);

		/* Small delay to prevent busy-waiting */
		mdelay(1);
	}

	return 0;
}
