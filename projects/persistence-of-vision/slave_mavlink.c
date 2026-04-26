/**
 * @file slave_mavlink.c
 * @brief Slave MAVLink communication implementation
 * @note Receives MOTOR_CONTROL from host, sends MOTOR_STATUS and
 *       MOTOR_ACK back. Manages heartbeat and status timing.
 */

#include <linux/kernel.h>
#include <linux/printk.h>
#include <linux/errno.h>
#include <linux/string.h>
#include <linux/delay.h>
#include <linux/slab.h>

#include "slave_mavlink.h"
#include "slave_config.h"
#include "mavlink_config.h"

#include <kinetis/basic-timer.h>

/*********************************************************************
 * Internal: Process received MAVLink messages
 *********************************************************************/

static int slave_mavlink_rx_process(struct slave_mavlink_device *dev)
{
	u8 buffer[256];
	int ret;
	int processed = 0;

	if (!dev || !dev->mav)
		return -EINVAL;

	ret = serial_port_receive_bytes(dev->mav->serial,
		(char *)buffer, sizeof(buffer), 5);
	if (ret <= 0)
		return ret;

	for (int i = 0; i < ret; i++) {
		if (!mavlink_parse_char(POV_MAVLINK_CHAN, buffer[i],
			&dev->mav->rx_msg, &dev->mav->rx_status))
			continue;

		dev->mav->rx_count++;
		processed++;

		/* Only accept messages from host */
		if (dev->mav->rx_msg.sysid != POV_HOST_SYSID) {
			pr_debug("Slave MAVLink: ignoring msg from sysid=%d\n",
				dev->mav->rx_msg.sysid);
			continue;
		}

		switch (dev->mav->rx_msg.msgid) {
		case MAVLINK_MSG_ID_MOTOR_CONTROL: {
			mavlink_motor_control_t ctrl;
			mavlink_msg_motor_control_decode(&dev->mav->rx_msg, &ctrl);

			pr_info("Slave MAVLink: MOTOR_CONTROL target=%d speed=%d dir=%d\n",
				ctrl.target_id, ctrl.speed, ctrl.direction);

			/* Extract target RPM */
			if (ctrl.direction == POV_DIR_STOP) {
				dev->target_rpm = 0;
			} else {
				dev->target_rpm = ctrl.speed;
			}

			dev->target_received = 1;

			/* Send ACK back to host */
			mavlink_send_motor_ack(dev->mav,
				ctrl.target_id, MAV_ACK_OK);
			break;
		}

		case MAVLINK_MSG_ID_MOTOR_STATUS: {
			/* We don't expect status from host, but handle gracefully */
			pr_debug("Slave MAVLink: unexpected MOTOR_STATUS from host\n");
			break;
		}

		default:
			pr_debug("Slave MAVLink: unknown msgid=%d\n",
				dev->mav->rx_msg.msgid);
			dev->mav->rx_errors++;
			break;
		}
	}

	return processed;
}

/*********************************************************************
 * Initialization
 *********************************************************************/

int slave_mavlink_init(struct slave_mavlink_device *dev)
{
	int ret;

	if (!dev)
		return -EINVAL;

	memset(dev, 0, sizeof(*dev));

	/* Initialize nRF24L01 with dummy callbacks */
	ret = nrf24l01_init(&dev->nrf,
		(nrf24l01_spi_xfer_cb)0,
		(nrf24l01_ce_cb)0,
		(nrf24l01_csn_cb)0);
	if (ret < 0) {
		pr_err("Slave MAVLink: nRF24L01 init failed: %d\n", ret);
		return ret;
	}

	/* Initialize serial port wrapper */
	ret = nrf24l01_serial_init(&dev->nrf_ser, &dev->nrf);
	if (ret < 0) {
		pr_err("Slave MAVLink: serial wrapper init failed: %d\n", ret);
		return ret;
	}

	/* Allocate serial port */
	dev->serial = serial_port_alloc(nrf24l01_serial_get_ops(&dev->nrf_ser));
	if (!dev->serial) {
		pr_err("Slave MAVLink: serial port alloc failed\n");
		return -ENOMEM;
	}

	/* Initialize MAVLink device */
	dev->mav = mavlink_init(dev->serial, POV_SLAVE_SYSID, POV_SLAVE_COMPID);
	if (!dev->mav) {
		pr_err("Slave MAVLink: mavlink init failed\n");
		serial_port_free(dev->serial);
		return -ENOMEM;
	}

	dev->mav->target_sysid = POV_HOST_SYSID;
	dev->mav->target_compid = POV_HOST_COMPID;
	dev->initialized = 1;

	pr_info("Slave MAVLink: initialized (sysid=%d, target=%d)\n",
		POV_SLAVE_SYSID, POV_HOST_SYSID);
	return 0;
}

/*********************************************************************
 * Send Heartbeat
 *********************************************************************/

int slave_mavlink_send_heartbeat(struct slave_mavlink_device *dev)
{
	if (!dev || !dev->initialized)
		return -EINVAL;

	/* Use MOTOR_STATUS with current state as heartbeat */
	s32 rpm = 0;
	u8 status = POV_STATUS_STOPPED;

	/* The caller should use send_status() for detailed info.
	 * Heartbeat is just a minimal "I'm alive" message. */
	int ret = mavlink_send_motor_status(dev->mav,
		POV_MOTOR_ID, 0, POV_DIR_FORWARD, POV_STATUS_STOPPED);

	if (ret >= 0)
		dev->last_heartbeat_sent = basic_timer_get_ms();

	return ret;
}

/*********************************************************************
 * Send Status
 *********************************************************************/

int slave_mavlink_send_status(struct slave_mavlink_device *dev,
	s32 current_rpm, u8 status)
{
	if (!dev || !dev->initialized)
		return -EINVAL;

	u8 direction;

	if (current_rpm > 0)
		direction = POV_DIR_FORWARD;
	else
		direction = POV_DIR_STOP;

	int ret = mavlink_send_motor_status(dev->mav,
		POV_MOTOR_ID, current_rpm, direction, status);

	if (ret >= 0)
		dev->last_status_sent = basic_timer_get_ms();

	return ret;
}

/*********************************************************************
 * Process Messages
 *********************************************************************/

int slave_mavlink_process(struct slave_mavlink_device *dev, u32 timeout_ms)
{
	if (!dev || !dev->initialized)
		return -EINVAL;

	return slave_mavlink_rx_process(dev);
}

/*********************************************************************
 * Command Query
 *********************************************************************/

int slave_mavlink_has_new_command(struct slave_mavlink_device *dev)
{
	if (!dev)
		return 0;

	if (dev->target_received) {
		dev->target_received = 0;
		return 1;
	}

	return 0;
}

s32 slave_mavlink_get_target_rpm(struct slave_mavlink_device *dev)
{
	if (!dev)
		return 0;

	return dev->target_rpm;
}

/*********************************************************************
 * Deinitialization
 *********************************************************************/

void slave_mavlink_deinit(struct slave_mavlink_device *dev)
{
	if (!dev)
		return;

	if (dev->mav) {
		mavlink_free(dev->mav);
		dev->mav = NULL;
	}

	if (dev->serial) {
		serial_port_free(dev->serial);
		dev->serial = NULL;
	}

	dev->initialized = 0;
	pr_info("Slave MAVLink: deinitialized\n");
}
