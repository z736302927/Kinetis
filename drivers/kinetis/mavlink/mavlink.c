#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/printk.h>
#include <linux/errno.h>
#include <linux/delay.h>
#include <linux/string.h>

#include "kinetis/design_verification.h"
#undef current
#include "kinetis/mavlink.h"

/* Convert plan direction (s8: 0/1/-1) to wire direction (u8: 0/1/2) */
static u8 dir_to_wire(s8 direction)
{
	if (direction > 0) {
		return 1;
	}
	if (direction < 0) {
		return 2;
	}
	return 0;
}

/* Convert wire direction (u8: 0/1/2) to plan direction (s8: 0/1/-1) */
static s8 dir_from_wire(u8 wire)
{
	if (wire == 1) {
		return 1;
	}
	if (wire == 2) {
		return -1;
	}
	return 0;
}

/* Advance motor simulation by one step toward target_speed */
static void mavlink_motor_simulate(struct mavlink_device *dev, u8 motor_id)
{
	struct mavlink_motor_state *motor = &dev->motors[motor_id];

	if (motor->target_speed == 0) {
		if (motor->current_speed > 0) {
			motor->current_speed -= 100;
			if (motor->current_speed < 0) {
				motor->current_speed = 0;
			}
		} else if (motor->current_speed < 0) {
			motor->current_speed += 100;
			if (motor->current_speed > 0) {
				motor->current_speed = 0;
			}
		}

		if (motor->current_speed == 0) {
			motor->direction = MAVLINK_DIRECTION_REVERSE;
			motor->status = MAVLINK_MOTOR_STOPPED;
		}
	} else {
		s8 target_dir = motor->target_speed > 0 ? MAVLINK_DIRECTION_FORWARD : MAVLINK_DIRECTION_REVERSE;
		/* If direction must change, decelerate to zero first */
		if (motor->current_speed != 0 && motor->direction != target_dir) {
			s32 step = (motor->current_speed > 0) ? -200 : 200;
			motor->current_speed += step;
			if (motor->current_speed > -20 && motor->current_speed < 20) {
				motor->current_speed = 0;
				motor->direction = target_dir;
				motor->status = MAVLINK_MOTOR_STOPPED;
			} else {
				motor->direction = target_dir;
				s32 diff = motor->target_speed - motor->current_speed;
				s32 accel = diff / 20;

				if (accel == 0) {
					accel = (diff > 0) ? 1 : -1;
				}
				if (accel < -50) {
					accel = -50;
				}
				if (accel > 50) {
					accel = 50;
				}
				motor->current_speed += accel;
				motor->status = MAVLINK_MOTOR_RUNNING;
			}
		}

	}
}

/* Internal: serialize and transmit a MAVLink message */
static int mavlink_send_raw(struct mavlink_device *dev, const mavlink_message_t *msg)
{
	u8 buffer[MAVLINK_MAX_PACKET_LEN];
	u16 len = mavlink_msg_to_send_buffer(buffer, msg);
	int ret = serial_port_transmit_bytes(dev->serial, buffer, len);
	if (ret >= 0) {
		dev->tx_count++;
	}
	return ret;
}

struct mavlink_device *mavlink_init(struct serial_port *serial, u8 sysid, u8 compid)
{
	struct mavlink_device *dev = kzalloc(sizeof(*dev), GFP_KERNEL);
	if (!dev) {
		return NULL;
	}

	dev->serial = serial;
	dev->sysid = sysid;
	dev->compid = compid;
	dev->rx_status.parse_state = MAVLINK_PARSE_STATE_IDLE;

	return dev;
}

void mavlink_free(struct mavlink_device *dev)
{
	if (dev->thread_running) {
		mavlink_stop_rx_thread(dev);
	}
	kfree(dev);
}

int mavlink_send_motor_control(struct mavlink_device *dev, u32 target_id, s32 speed, s8 direction)
{
	mavlink_motor_control_t payload;
	mavlink_message_t msg;

	payload.target_id = target_id;
	payload.speed = speed;
	payload.direction = dir_to_wire(direction);

	mavlink_msg_motor_control_encode(dev->sysid, dev->compid, &msg, &payload);

	return mavlink_send_raw(dev, &msg);
}

int mavlink_send_motor_status(struct mavlink_device *dev, u16 motor_id,
	s32 current_speed, s8 direction, u8 status)
{
	mavlink_motor_status_t payload;
	mavlink_message_t msg;

	payload.motor_id = motor_id;
	payload.current_speed = current_speed;
	payload.direction = dir_to_wire(direction);
	payload.status = status;

	mavlink_msg_motor_status_encode(dev->sysid, dev->compid, &msg, &payload);

	return mavlink_send_raw(dev, &msg);
}

int mavlink_send_motor_ack(struct mavlink_device *dev, u8 motor_id, u8 ack_result)
{
	mavlink_motor_ack_t payload;
	mavlink_message_t msg;

	payload.motor_id = motor_id;
	payload.ack_result = ack_result;

	mavlink_msg_motor_ack_encode(dev->sysid, dev->compid, &msg, &payload);

	return mavlink_send_raw(dev, &msg);
}

int mavlink_receive_and_process(struct mavlink_device *dev, u32 timeout_ms)
{
	u8 buffer[256];
	int ret;
	int processed = 0;

	ret = serial_port_receive_bytes(dev->serial, (char *)buffer,
		sizeof(buffer), timeout_ms);
	if (ret < 0) {
		return ret;
	}


	for (int i = 0; i < ret; i++) {
		if (!mavlink_parse_char(MAVLINK_COMM_0, buffer[i], &dev->rx_msg, &dev->rx_status)) {
			continue;
		}

		dev->rx_count++;
		processed++;

		switch (dev->rx_msg.msgid) {
		case MAVLINK_MSG_ID_MOTOR_CONTROL: {
			mavlink_motor_control_t ctrl;

			mavlink_msg_motor_control_decode(&dev->rx_msg, &ctrl);
			pr_info("MAVLink: MOTOR CONTROL target=%d speed=%d direction=%d\n",
				ctrl.target_id, ctrl.speed, ctrl.direction);

			if (ctrl.target_id >= MAVLINK_MOTOR_COUNT && ctrl.target_id != 255) {
				mavlink_send_motor_ack(dev, ctrl.target_id, MAV_ACK_INVALID_PARAM);
				break;
			}

			u8 start = (ctrl.target_id == 255) ? 0 : ctrl.target_id;
			u8 end = (ctrl.target_id == 255) ? MAVLINK_MOTOR_COUNT : ctrl.target_id + 1;

			for (u8 idx = start; idx < end; idx++) {
				dev->motors[idx].target_speed = ctrl.speed;
				dev->motors[idx].direction = dir_from_wire(ctrl.direction);

				mavlink_motor_simulate(dev, idx);

				mavlink_send_motor_status(dev, idx,
					dev->motors[idx].current_speed,
					dev->motors[idx].direction,
					dev->motors[idx].status);

				mavlink_send_motor_ack(dev, idx, MAV_ACK_OK);
			}
			break;
		}

		case MAVLINK_MSG_ID_MOTOR_STATUS: {
			mavlink_motor_status_t st;
			mavlink_msg_motor_status_decode(&dev->rx_msg, &st);
			pr_info("MAVLink: MOTOR STATUS id=%d speed=%d dir=%d status=%d\n",
				st.motor_id, st.current_speed, st.direction, st.status);
			break;
		}

		case MAVLINK_MSG_ID_MOTOR_ACK: {
			mavlink_motor_ack_t ack;
			mavlink_msg_motor_ack_decode(&dev->rx_msg, &ack);
			pr_info("MAVLink: MOTOR ACK id=%d result=%d\n",
				ack.motor_id, ack.ack_result);
			break;
		}

		default:
			pr_info("MAVLink: Unknown message id=%d\n", dev->rx_msg.msgid);
            dev->rx_errors++;
			break;
		}
	}

	return processed;
}

static void *mavlink_rx_thread_func(void *arg)
{
	struct mavlink_device *dev = (struct mavlink_device *)arg;

	pr_info("MAVLink rx thread started\n");

	while (dev->thread_running) {
		if (serial_port_data_available(dev->serial) > 0) {
			mavlink_receive_and_process(dev, 10);
		} else {
			mdelay(5);
		}
	}

	pr_info("MAVLink rx thread stopped\n");
	return NULL;
}

int mavlink_start_rx_thread(struct mavlink_device *dev)
{
	dev->thread_running = 1;
	int ret = pthread_create(&dev->rx_thread, NULL, mavlink_rx_thread_func, dev);

	if (ret != 0) {
		dev->thread_running = 0;
		pr_err("MAVLink: failed to create rx thread: %d\n", ret);
		return ret;
	}

	return 0;
}

void mavlink_stop_rx_thread(struct mavlink_device *dev)
{
	dev->thread_running = 0;
	pthread_cancel(dev->rx_thread);
	pthread_join(dev->rx_thread, NULL);
}

#ifdef DESIGN_VERIFICATION_MAVLINK

int t_mavlink_master_slave_sim(int argc, char *argv[])
{
    struct serial_port *master_port, *slave_port;
    struct mavlink_device *master, *slave;
    int ret = 0;

    pr_info("Starting MAVLink master-slave simulation test\n");

    master_port = serial_port_alloc(&fake_serial_port_ops);
    if (!master_port) {
        pr_err("Failed to allocate master serial port\n");
        return -ENOMEM;
    }

    slave_port = serial_port_alloc(&fake_serial_port_ops);
    if (!slave_port) {
        pr_err("Failed to allocate slave serial port\n");
        ret = -ENOMEM;
        goto cleanup_master;
    }

    serial_port_start_thread(master_port, SERIAL_PORT_DF_OTHERS, NULL, slave_port);
    serial_port_start_thread(slave_port, SERIAL_PORT_DF_OTHERS, NULL, master_port);

    master = mavlink_init(master_port, 1, 0);
    slave = mavlink_init(slave_port, 2, 0);

    if (!master || !slave) {
        pr_err("Failed to init MAVLink devices\n");
        ret = -ENOMEM;
        goto cleanup;
    }

    ret = mavlink_start_rx_thread(slave);
    if (ret < 0) {
        pr_err("Failed to start slave rx thread: %d\n", ret);
        goto cleanup;
    }

    mdelay(100);

    for (u8 motor_id = 0; motor_id < MAVLINK_MOTOR_COUNT; motor_id++) {
        s32 speed = 1000 + motor_id * 500;
        
        pr_info("MAVLink master: sending MOTOR_CONTROL motor=%u speed=%d\n", 
               motor_id, speed);
        ret = mavlink_send_motor_control(master, motor_id, speed, MAVLINK_DIRECTION_FORWARD);
        if (ret < 0) {
            pr_err("Failed to send MOTOR_CONTROL: %d\n", ret);
            continue;
        }

        mdelay(50);

        int timeout = 100;
        int responses = 0;
        while (timeout-- > 0 && responses < 2) {
            int r = mavlink_receive_and_process(master, 10);
            if (r > 0)
                responses += r;
            else
                mdelay(1);
        }
        
        if (responses == 0)
            pr_warn("MAVLink master: timeout waiting for motor %u response\n", motor_id);

        mdelay(200);
    }

    mavlink_stop_rx_thread(slave);

cleanup:
    mavlink_free(master);
    mavlink_free(slave);
    serial_port_stop_thread(master_port);
    serial_port_stop_thread(slave_port);
    serial_port_free(master_port);
    serial_port_free(slave_port);

cleanup_master:
    serial_port_free(master_port);
    serial_port_free(slave_port);

    pr_info("MAVLink master-slave simulation test completed with ret=%d\n", ret);
    return ret;
}

int t_mavlink_msg_pack_parse(int argc, char *argv[])
{
    struct serial_port *port;
    struct mavlink_device *dev;
    int ret = 0;

    pr_info("Starting MAVLink message packing/parsing test\n");

    port = serial_port_alloc(&fake_serial_port_ops);
    if (!port) {
        pr_err("Failed to allocate serial port\n");
        return -ENOMEM;
    }

    serial_port_start_thread(port, SERIAL_PORT_DF_OTHERS, NULL, port);

    dev = mavlink_init(port, 1, 0);
    if (!dev) {
        pr_err("Failed to init MAVLink device\n");
        ret = -ENOMEM;
        goto cleanup;
    }

    ret = mavlink_send_motor_control(dev, 0, 1500, MAVLINK_DIRECTION_FORWARD);
    if (ret < 0) {
        pr_err("Failed to send MOTOR_CONTROL: %d\n", ret);
        goto cleanup;
    }

    pr_info("MOTOR_CONTROL message sent successfully (tx_count=%u)\n", dev->tx_count);

    ret = mavlink_receive_and_process(dev, 50);
    pr_info("Receive result: %d\n", ret);
    ret = 0;

cleanup:
    mavlink_free(dev);
    serial_port_stop_thread(port);
    serial_port_free(port);

    pr_info("MAVLink message packing/parsing test completed with ret=%d\n", ret);
    return ret;
}

#endif /* DESIGN_VERIFICATION_MAVLINK */