/**
 * @file host_mavlink.c
 * @brief Host MAVLink communication implementation
 * @note Manages nRF24L01 radio, processes incoming MOTOR_STATUS
 *       messages from slave, and sends MOTOR_CONTROL commands
 */

#include <linux/kernel.h>
#include <linux/printk.h>
#include <linux/errno.h>
#include <linux/string.h>
#include <linux/delay.h>
#include <linux/slab.h>

#include "host_mavlink.h"
#include "mavlink_config.h"

#include <kinetis/basic-timer.h>

/*********************************************************************
 * Internal: MAVLink message receive callback
 *********************************************************************/

static int host_mavlink_process_messages(struct host_mavlink_device *dev)
{
	int processed = 0;
	mavlink_message_t msg;
	mavlink_status_t status;

	if (!dev || !dev->mav)
		return -EINVAL;

	/* Use mavlink_receive_and_process which handles parsing */
	int ret = mavlink_receive_and_process(dev->mav, 10);
	if (ret < 0)
		return ret;

	return ret;
}

/*********************************************************************
 * Internal: Custom MAVLink receive handler
 *********************************************************************/

/* We override the default message processing by using a custom
 * receive loop that intercepts MOTOR_STATUS messages */

static int host_mavlink_rx_process(struct host_mavlink_device *dev)
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

		switch (dev->mav->rx_msg.msgid) {
		case MAVLINK_MSG_ID_MOTOR_STATUS: {
			mavlink_motor_status_t st;
			mavlink_msg_motor_status_decode(&dev->mav->rx_msg, &st);

			/* Only accept messages from slave */
			if (dev->mav->rx_msg.sysid == POV_SLAVE_SYSID) {
				dev->slave_rpm = st.current_speed;
				dev->slave_status = st.status;
				dev->last_heartbeat_ms = basic_timer_get_ms();
				dev->slave_online = 1;
			}

			pr_debug("Host MAVLink: MOTOR_STATUS rpm=%d status=%d\n",
				st.current_speed, st.status);
			break;
		}

		case MAVLINK_MSG_ID_MOTOR_ACK: {
			mavlink_motor_ack_t ack;
			mavlink_msg_motor_ack_decode(&dev->mav->rx_msg, &ack);

			if (dev->mav->rx_msg.sysid == POV_SLAVE_SYSID) {
				dev->last_heartbeat_ms = basic_timer_get_ms();
				dev->slave_online = 1;

				if (ack.ack_result != MAV_ACK_OK)
					pr_warn("Host MAVLink: motor ACK error=%d\n",
						ack.ack_result);
			}
			break;
		}

		default:
			pr_debug("Host MAVLink: unknown msgid=%d\n",
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

int host_mavlink_init(struct host_mavlink_device *dev)
{
	int ret;

	if (!dev)
		return -EINVAL;

	memset(dev, 0, sizeof(*dev));

	/* Initialize nRF24L01 with dummy callbacks (set later for HW) */
	ret = nrf24l01_init(&dev->nrf,
		(nrf24l01_spi_xfer_cb)0,  /* Set by board_init for HW */
		(nrf24l01_ce_cb)0,
		(nrf24l01_csn_cb)0);
	if (ret < 0) {
		pr_err("Host MAVLink: nRF24L01 init failed: %d\n", ret);
		return ret;
	}

	/* Initialize nRF24L01 serial port wrapper */
	ret = nrf24l01_serial_init(&dev->nrf_ser, &dev->nrf);
	if (ret < 0) {
		pr_err("Host MAVLink: serial wrapper init failed: %d\n", ret);
		return ret;
	}

	/* Allocate serial port */
	dev->serial = serial_port_alloc(nrf24l01_serial_get_ops(&dev->nrf_ser));
	if (!dev->serial) {
		pr_err("Host MAVLink: serial port alloc failed\n");
		return -ENOMEM;
	}

	/* Initialize MAVLink device */
	dev->mav = mavlink_init(dev->serial, POV_HOST_SYSID, POV_HOST_COMPID);
	if (!dev->mav) {
		pr_err("Host MAVLink: mavlink init failed\n");
		serial_port_free(dev->serial);
		return -ENOMEM;
	}

	dev->mav->target_sysid = POV_SLAVE_SYSID;
	dev->mav->target_compid = POV_SLAVE_COMPID;
	dev->initialized = 1;

	pr_info("Host MAVLink: initialized (sysid=%d, target=%d)\n",
		POV_HOST_SYSID, POV_SLAVE_SYSID);
	return 0;
}

/*********************************************************************
 * Send Speed Command
 *********************************************************************/

int host_mavlink_send_speed_cmd(struct host_mavlink_device *dev, s32 rpm)
{
	if (!dev || !dev->initialized)
		return -EINVAL;

	s8 direction;

	if (rpm > 0) {
		direction = MAVLINK_DIRECTION_FORWARD;
	} else if (rpm < 0) {
		direction = MAVLINK_DIRECTION_REVERSE;
		rpm = -rpm;
	} else {
		direction = MAVLINK_DIRECTION_REVERSE;  /* 0 = stop */
	}

	int ret = mavlink_send_motor_control(dev->mav,
		POV_MOTOR_TARGET_ID, rpm, direction);
	if (ret < 0) {
		pr_err("Host MAVLink: send speed cmd failed: %d\n", ret);
		return ret;
	}

	dev->target_rpm = rpm;
	pr_debug("Host MAVLink: sent speed cmd rpm=%d\n", rpm);
	return 0;
}

/*********************************************************************
 * Process Messages
 *********************************************************************/

int host_mavlink_process(struct host_mavlink_device *dev, u32 timeout_ms)
{
	if (!dev || !dev->initialized)
		return -EINVAL;

	return host_mavlink_rx_process(dev);
}

/*********************************************************************
 * Status Query
 *********************************************************************/

int host_mavlink_is_slave_online(struct host_mavlink_device *dev)
{
	if (!dev || !dev->initialized)
		return 0;

	u64 now = basic_timer_get_ms();

	if (dev->slave_online &&
		(now - dev->last_heartbeat_ms) < POV_SLAVE_TIMEOUT_MS)
		return 1;

	/* Timeout - mark offline */
	dev->slave_online = 0;
	return 0;
}

s32 host_mavlink_get_slave_rpm(struct host_mavlink_device *dev)
{
	if (!dev)
		return 0;

	return dev->slave_rpm;
}

u8 host_mavlink_get_slave_status(struct host_mavlink_device *dev)
{
	if (!dev)
		return POV_STATUS_STOPPED;

	return dev->slave_status;
}

/*********************************************************************
 * Deinitialization
 *********************************************************************/

void host_mavlink_deinit(struct host_mavlink_device *dev)
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
	pr_info("Host MAVLink: deinitialized\n");
}
