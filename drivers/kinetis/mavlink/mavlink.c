/*
 * Use pr_debug for routine messages so they are compiled out unless
 * DEBUG is defined.  Keep pr_info only for state transitions and errors.
 */
#define pr_fmt(fmt) "MAVLink: " fmt

#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/printk.h>
#include <linux/errno.h>
#include <linux/delay.h>
#include <linux/string.h>

#include "kinetis/design_verification.h"
#include "kinetis/mavlink.h"

/**
 * @brief Convert internal direction (s8: 0/1/-1) to wire direction (u8: 0/1/2).
 */
static u8 dir_to_wire(s8 direction)
{
	if (direction > 0)
		return 1;
	if (direction < 0)
		return 2;
	return 0;
}

/**
 * @brief Convert wire direction (u8: 0/1/2) to internal direction (s8: 0/1/-1).
 */
static s8 dir_from_wire(u8 wire)
{
	if (wire == 1)
		return 1;
	if (wire == 2)
		return -1;
	return 0;
}

#define ACCEL_STEP  100   /* RPM per tick when stopping */
#define REV_STEP    200   /* RPM per tick when reversing */
#define ACCEL_DIV   20    /* target/ACCEL_DIV = acceleration */
#define ACCEL_CLAMP 50    /* max acceleration per tick */
#define NEAR_ZERO   20    /* threshold to snap to zero */

/**
 * @brief Simulate one acceleration/deceleration tick for a motor.
 *
 * Handles three cases:
 *   - target == 0  → decelerate to stop
 *   - direction change → decelerate through zero, then accelerate
 *   - same direction → accelerate/decelerate toward target
 */
static void mavlink_motor_simulate(struct mavlink_device *mav, u8 motor_id)
{
	struct mavlink_motor_state *motor = &mav->motors[motor_id];
	s32 target = motor->target_speed;
	s32 current = motor->current_speed;
	s8 target_dir;
	s32 diff;
	s32 step;

	/* ----- Stopping ----- */
	if (target == 0) {
		if (current > 0)
			current -= ACCEL_STEP;
		else if (current < 0)
			current += ACCEL_STEP;

		if (current > -ACCEL_STEP && current < ACCEL_STEP) {
			current = 0;
			motor->direction = MAVLINK_DIRECTION_REVERSE;
			motor->status = MAVLINK_MOTOR_STOPPED;
		}

		motor->current_speed = current;
		return;
	}

	/* ----- Running ----- */
	target_dir = (target > 0) ? MAVLINK_DIRECTION_FORWARD
	                          : MAVLINK_DIRECTION_REVERSE;

	/* Direction change: decelerate through zero first */
	if (current != 0 && motor->direction != target_dir) {
		current += (current > 0) ? -REV_STEP : REV_STEP;

		if (current > -NEAR_ZERO && current < NEAR_ZERO) {
			current = 0;
			motor->direction = target_dir;
			motor->status = MAVLINK_MOTOR_STOPPED;
		}

		motor->current_speed = current;
		return;
	}

	/* Same direction: accelerate toward target */
	diff = target - current;
	step = diff / ACCEL_DIV;

	if (step == 0)
		step = (diff > 0) ? 1 : -1;
	if (step < -ACCEL_CLAMP)
		step = -ACCEL_CLAMP;
	if (step > ACCEL_CLAMP)
		step = ACCEL_CLAMP;

	current += step;
	motor->direction = target_dir;
	motor->status = MAVLINK_MOTOR_RUNNING;
	motor->current_speed = current;
}

/**
 * @brief Decode a received MAVLink message and log its contents at pr_debug level.
 * @param name: Device name / label for log prefix
 * @param msg: Parsed MAVLink message
 */
static void mavlink_print_decoded_log(const char *name, const mavlink_message_t *msg)
{
	switch (msg->msgid) {
	case MAVLINK_MSG_ID_MOTOR_CONTROL: {
		mavlink_motor_control_t p;
		mavlink_msg_motor_control_decode(msg, &p);
		pr_debug("%s: MOTOR_CONTROL  target:%u speed:%d dir:%u\n",
			 name, p.target_id, p.speed, p.direction);
		break;
	}
	case MAVLINK_MSG_ID_MOTOR_STATUS_QUERY: {
		mavlink_motor_status_query_t p;
		mavlink_msg_motor_status_query_decode(msg, &p);
		pr_debug("%s: MOTOR_STATUS_QUERY  motor:%u\n", name, p.motor_id);
		break;
	}
	case MAVLINK_MSG_ID_MOTOR_STATUS: {
		mavlink_motor_status_t p;
		mavlink_msg_motor_status_decode(msg, &p);
		pr_debug("%s: MOTOR_STATUS  id:%u speed:%d dir:%u status:%u\n",
			 name, p.motor_id, p.current_speed, p.direction, p.status);
		break;
	}
	case MAVLINK_MSG_ID_MOTOR_ACK: {
		mavlink_motor_ack_t p;
		mavlink_msg_motor_ack_decode(msg, &p);
		pr_debug("%s: MOTOR_ACK  id:%u result:%u\n",
			 name, p.motor_id, p.ack_result);
		break;
	}
	case MAVLINK_MSG_ID_IMAGE_CONTROL: {
		mavlink_image_control_t p;
		mavlink_msg_image_control_decode(msg, &p);
		pr_debug("%s: IMAGE_CONTROL  action:%u index:%u\n",
			 name, p.action, p.image_index);
		break;
	}
	case MAVLINK_MSG_ID_IMAGE_ACK: {
		mavlink_image_ack_t p;
		mavlink_msg_image_ack_decode(msg, &p);
		pr_debug("%s: IMAGE_ACK  action:%u result:%u\n",
			 name, p.action, p.ack_result);
		break;
	}
	case MAVLINK_MSG_ID_SYSTEM_HEARTBEAT: {
		mavlink_system_heartbeat_t p;
		mavlink_msg_system_heartbeat_decode(msg, &p);
		pr_debug("%s: HEARTBEAT  status:%u uptime:%u errors:%u\n",
			 name, p.status, p.uptime_s, p.error_count);
		break;
	}
	case MAVLINK_MSG_ID_FILE_TRANSFER_START: {
		mavlink_file_transfer_start_t p;
		mavlink_msg_file_transfer_start_decode(msg, &p);
		pr_debug("%s: FILE_TRANSFER_START  dir:%u size:%u name:%s\n",
			 name, p.direction, p.file_size, p.filename);
		break;
	}
	case MAVLINK_MSG_ID_FILE_TRANSFER_ACK: {
		mavlink_file_transfer_ack_t p;
		mavlink_msg_file_transfer_ack_decode(msg, &p);
		pr_debug("%s: FILE_TRANSFER_ACK  dir:%u result:%u size:%u\n",
			 name, p.direction, p.result, p.file_size);
		break;
	}
	case MAVLINK_MSG_ID_FILE_TRANSFER_PROGRESS: {
		mavlink_file_transfer_progress_t p;
		mavlink_msg_file_transfer_progress_decode(msg, &p);
		pr_debug("%s: FT_PROGRESS  state:%u xfer:%u/%u err:%u\n",
			 name, p.state, p.bytes_transferred,
			 p.total_bytes, p.error_code);
		break;
	}
	case MAVLINK_MSG_ID_FILE_TRANSFER_CANCEL: {
		mavlink_file_transfer_cancel_t p;
		mavlink_msg_file_transfer_cancel_decode(msg, &p);
		pr_debug("%s: FT_CANCEL  dir:%u\n", name, p.direction);
		break;
	}
	case MAVLINK_MSG_ID_FILE_TRANSFER_LIST: {
		mavlink_file_transfer_list_t p;
		mavlink_msg_file_transfer_list_decode(msg, &p);
		pr_debug("%s: FT_LIST  [%u/%u] %u bytes %s\n",
			 name, p.list_index, p.file_count,
			 p.file_size, p.filename);
		break;
	}
	case MAVLINK_MSG_ID_BOOTLOADER_UPDATE_CMD: {
		mavlink_bootloader_update_cmd_t p;
		mavlink_msg_bootloader_update_cmd_decode(msg, &p);
		pr_debug("%s: BL_UPDATE_CMD  cmd:%u size:%u crc:0x%08x name:%s\n",
			 name, p.command, p.firmware_size,
			 p.firmware_crc32, p.filename);
		break;
	}
	case MAVLINK_MSG_ID_BOOTLOADER_ACK: {
		mavlink_bootloader_ack_t p;
		mavlink_msg_bootloader_ack_decode(msg, &p);
		pr_debug("%s: BL_ACK  cmd:%u result:%u\n",
			 name, p.command, p.result);
		break;
	}
	default:
		pr_debug("%s: MSG id:%u len:%u seq:%u\n",
			 name, msg->msgid, msg->len, msg->seq);
		break;
	}
}

/**
 * @brief Encode a MAVLink message into a buffer and transmit over serial.
 * @return Number of bytes sent on success, negative on error
 */
static int mavlink_send_encoded_data(struct mavlink_device *mav,
				     const mavlink_message_t *msg)
{
	u8 buffer[MAVLINK_MAX_PACKET_LEN];
	u16 len = mavlink_msg_to_send_buffer(buffer, msg);
	int ret;

	mavlink_print_decoded_log(MD_NAME(mav), msg);

	ret = serial_port_transmit_bytes(mav->serial, buffer, len);
	if (ret >= 0)
		mav->tx_count++;

	return ret;
}

struct mavlink_device *mavlink_init(struct serial_port *serial,
				    u8 sysid, u8 compid, const char *name)
{
	struct mavlink_device *mav = kzalloc(sizeof(*mav), GFP_KERNEL);
	if (!mav)
		return NULL;

	mav->serial = serial;
	mav->sysid = sysid;
	mav->compid = compid;

	if (name)
		mav->name = kstrdup(name, GFP_KERNEL);

	mav->rx_status.parse_state = MAVLINK_PARSE_STATE_IDLE;
	mav->ft.state = MAV_FT_IDLE;
	mav->sys_status = MAV_SYS_STANDBY;

	return mav;
}

void mavlink_free(struct mavlink_device *mav)
{
	if (!mav)
		return;

	if (mav->thread_running)
		mavlink_stop_rx_thread(mav);

	kfree(mav->name);
	kfree(mav);
}

int mavlink_receive_and_process(struct mavlink_device *mav, u32 timeout_ms)
{
	u8 buffer[MAVLINK_RX_BUF_SIZE];
	int ret;

	ret = serial_port_receive_bytes(mav->serial, (char *)buffer,
					sizeof(buffer), timeout_ms);
	if (ret < 0)
		return ret;

	for (int i = 0; i < ret; i++) {
		if (!mavlink_parse_char(MAVLINK_COMM_0, buffer[i],
					&mav->rx_msg, &mav->rx_status))
			continue;

		mav->rx_count++;

		switch (mav->rx_msg.msgid) {
		case MAVLINK_MSG_ID_MOTOR_CONTROL: {
			mavlink_motor_control_t ctrl;

			mavlink_msg_motor_control_decode(&mav->rx_msg, &ctrl);
			pr_debug("MOTOR CONTROL target=%d speed=%d direction=%d\n",
				 ctrl.target_id, ctrl.speed, ctrl.direction);

			if (ctrl.target_id >= MAVLINK_MOTOR_COUNT &&
			    ctrl.target_id != 255) {
				mavlink_send_motor_ack(mav, ctrl.target_id,
						       MAV_ACK_INVALID_PARAM);
				break;
			}

			u8 start = (ctrl.target_id == 255) ? 0 : ctrl.target_id;
			u8 end = (ctrl.target_id == 255)
				     ? MAVLINK_MOTOR_COUNT
				     : ctrl.target_id + 1;

			for (u8 idx = start; idx < end; idx++) {
				mav->motors[idx].target_speed = ctrl.speed;
				mav->motors[idx].direction =
					dir_from_wire(ctrl.direction);
				mavlink_send_motor_ack(mav, idx, MAV_ACK_OK);
			}
			break;
		}

		case MAVLINK_MSG_ID_MOTOR_STATUS_QUERY: {
			mavlink_motor_status_query_t query;

			mavlink_msg_motor_status_query_decode(&mav->rx_msg, &query);
			pr_debug("MOTOR STATUS QUERY motor_id=%d\n",
				 query.motor_id);

			if (query.motor_id >= MAVLINK_MOTOR_COUNT &&
			    query.motor_id != 255)
				break;

			u8 start = (query.motor_id == 255) ? 0 : query.motor_id;
			u8 end = (query.motor_id == 255)
				     ? MAVLINK_MOTOR_COUNT
				     : query.motor_id + 1;

			for (u8 idx = start; idx < end; idx++)
				mavlink_send_motor_status(mav, idx,
					mav->motors[idx].current_speed,
					mav->motors[idx].direction,
					mav->motors[idx].status);
			break;
		}

		case MAVLINK_MSG_ID_MOTOR_STATUS: {
			mavlink_motor_status_t st;

			mavlink_msg_motor_status_decode(&mav->rx_msg, &st);
			if (st.motor_id < MAVLINK_MOTOR_COUNT) {
				mav->motors[st.motor_id].current_speed =
					st.current_speed;
				mav->motors[st.motor_id].direction =
					st.direction;
				mav->motors[st.motor_id].status = st.status;
			}
			pr_debug("MOTOR STATUS id=%d speed=%d dir=%d status=%d\n",
				 st.motor_id, st.current_speed,
				 st.direction, st.status);
			break;
		}

		case MAVLINK_MSG_ID_MOTOR_ACK: {
			mavlink_motor_ack_t ack;

			mavlink_msg_motor_ack_decode(&mav->rx_msg, &ack);
			pr_debug("MOTOR ACK id=%d result=%d\n",
				 ack.motor_id, ack.ack_result);
			break;
		}

		case MAVLINK_MSG_ID_IMAGE_CONTROL: {
			mavlink_image_control_t ctrl;

			mavlink_msg_image_control_decode(&mav->rx_msg, &ctrl);
			pr_info("IMAGE CONTROL action=%d index=%d\n",
				ctrl.action, ctrl.image_index);
			mav->image_cmd.pending = 1;
			mav->image_cmd.action = ctrl.action;
			mav->image_cmd.image_index = ctrl.image_index;
			break;
		}

		case MAVLINK_MSG_ID_SYSTEM_HEARTBEAT: {
			mavlink_system_heartbeat_t hb;

			mavlink_msg_system_heartbeat_decode(&mav->rx_msg, &hb);
			mav->sys_status = hb.status;
			mav->uptime_s = hb.uptime_s;
			mav->error_count = hb.error_count;
			pr_debug("HEARTBEAT status=%d uptime=%u errors=%u\n",
				 hb.status, hb.uptime_s, hb.error_count);
			break;
		}

		case MAVLINK_MSG_ID_FILE_TRANSFER_START: {
			mavlink_file_transfer_start_t ft;
			size_t len;

			mavlink_msg_file_transfer_start_decode(&mav->rx_msg, &ft);
			pr_info("FT START dir=%d size=%u name=%s\n",
				ft.direction, ft.file_size, ft.filename);

			mav->ft.direction = ft.direction;
			mav->ft.total_bytes = ft.file_size;
			mav->ft.transferred = 0;
			mav->ft.error_code = MAV_FT_ERR_NONE;
			len = strnlen(ft.filename, sizeof(ft.filename));
			memcpy(mav->ft.filename, ft.filename, len);
			mav->ft.filename[len] = '\0';
			mav->ft.state = MAV_FT_TRANSFERRING;
			mav->ft.active = 1;
			break;
		}

		case MAVLINK_MSG_ID_FILE_TRANSFER_ACK: {
			mavlink_file_transfer_ack_t ack;

			mavlink_msg_file_transfer_ack_decode(&mav->rx_msg, &ack);
			pr_debug("FT ACK dir=%d result=%d size=%u\n",
				 ack.direction, ack.result, ack.file_size);
			break;
		}

		case MAVLINK_MSG_ID_FILE_TRANSFER_PROGRESS: {
			mavlink_file_transfer_progress_t prog;

			mavlink_msg_file_transfer_progress_decode(&mav->rx_msg, &prog);
			mav->ft.state = prog.state;
			mav->ft.transferred = prog.bytes_transferred;
			mav->ft.total_bytes = prog.total_bytes;
			mav->ft.error_code = prog.error_code;

			if (prog.state == MAV_FT_COMPLETED ||
			    prog.state == MAV_FT_CANCELLED ||
			    prog.state == MAV_FT_ERROR)
				mav->ft.active = 0;

			pr_debug("FT PROGRESS state=%d xfer=%u/%u err=%d\n",
				 prog.state, prog.bytes_transferred,
				 prog.total_bytes, prog.error_code);
			break;
		}

		case MAVLINK_MSG_ID_FILE_TRANSFER_CANCEL: {
			mavlink_file_transfer_cancel_t cancel;

			mavlink_msg_file_transfer_cancel_decode(&mav->rx_msg, &cancel);
			pr_info("FT CANCEL dir=%d\n", cancel.direction);
			mav->ft.state = MAV_FT_CANCELLED;
			mav->ft.active = 0;
			break;
		}

		case MAVLINK_MSG_ID_FILE_TRANSFER_LIST: {
			mavlink_file_transfer_list_t fl;

			mavlink_msg_file_transfer_list_decode(&mav->rx_msg, &fl);
			pr_debug("FT LIST idx=%d/%d size=%u name=%s\n",
				 fl.list_index, fl.file_count,
				 fl.file_size, fl.filename);
			break;
		}

		case MAVLINK_MSG_ID_BOOTLOADER_UPDATE_CMD: {
			mavlink_bootloader_update_cmd_t cmd;

			mavlink_msg_bootloader_update_cmd_decode(&mav->rx_msg, &cmd);
			pr_info("BL UPDATE_CMD cmd=%d size=%u crc=0x%08x name=%s\n",
				cmd.command, cmd.firmware_size,
				cmd.firmware_crc32, cmd.filename);

			mav->bl.pending = 1;
			mav->bl.command = cmd.command;
			mav->bl.firmware_size = cmd.firmware_size;
			mav->bl.firmware_crc32 = cmd.firmware_crc32;
			mav->bl.filename[sizeof(mav->bl.filename) - 1] = '\0';
			strncpy(mav->bl.filename, cmd.filename,
				sizeof(mav->bl.filename) - 1);

			mavlink_send_bootloader_ack(mav, cmd.command,
						    MAV_BL_ACK_ACCEPTED);
			break;
		}

		default:
			pr_debug("Unknown message id=%d\n", mav->rx_msg.msgid);
			mav->rx_errors++;
			break;
		}
	}

	return 0;
}

int mavlink_send_motor_control(struct mavlink_device *mav, u8 target_id,
			       s32 speed, s8 direction, u32 timeout_ms)
{
	mavlink_motor_control_t payload;
	mavlink_message_t msg;
	int ret;

	payload.target_id = target_id;
	payload.speed = speed;
	payload.direction = dir_to_wire(direction);

	mavlink_msg_motor_control_encode(mav->sysid, mav->compid, &msg, &payload);

	ret = mavlink_send_encoded_data(mav, &msg);
	if (ret < 0)
		return ret;

	return mavlink_receive_and_process(mav, timeout_ms);
}

int mavlink_send_motor_status(struct mavlink_device *mav, u16 motor_id,
			      s32 current_speed, s8 direction, u8 status)
{
	mavlink_motor_status_t payload;
	mavlink_message_t msg;

	payload.motor_id = motor_id;
	payload.current_speed = current_speed;
	payload.direction = dir_to_wire(direction);
	payload.status = status;

	mavlink_msg_motor_status_encode(mav->sysid, mav->compid, &msg, &payload);

	return mavlink_send_encoded_data(mav, &msg);
}

int mavlink_send_motor_ack(struct mavlink_device *mav, u8 motor_id,
			   u8 ack_result)
{
	mavlink_motor_ack_t payload;
	mavlink_message_t msg;

	payload.motor_id = motor_id;
	payload.ack_result = ack_result;

	mavlink_msg_motor_ack_encode(mav->sysid, mav->compid, &msg, &payload);

	return mavlink_send_encoded_data(mav, &msg);
}

int mavlink_send_image_ack(struct mavlink_device *mav, u8 action,
			   u8 ack_result)
{
	mavlink_image_ack_t payload;
	mavlink_message_t msg;

	payload.action = action;
	payload.ack_result = ack_result;

	mavlink_msg_image_ack_encode(mav->sysid, mav->compid, &msg, &payload);

	return mavlink_send_encoded_data(mav, &msg);
}

int mavlink_query_motor_status(struct mavlink_device *mav, u8 motor_id,
			       u32 timeout_ms)
{
	mavlink_motor_status_query_t payload;
	mavlink_message_t msg;
	int ret;

	payload.motor_id = motor_id;

	mavlink_msg_motor_status_query_encode(mav->sysid, mav->compid, &msg, &payload);

	ret = mavlink_send_encoded_data(mav, &msg);
	if (ret < 0)
		return ret;

	return mavlink_receive_and_process(mav, timeout_ms);
}

int mavlink_send_ft_start(struct mavlink_device *mav, u8 direction,
			  const char *filename, u32 file_size, u32 timeout_ms)
{
	mavlink_file_transfer_start_t payload;
	mavlink_message_t msg;
	size_t len;
	int ret;

	payload.direction = direction;
	payload.file_size = file_size;
	len = strnlen(filename, sizeof(payload.filename) - 1);
	memcpy(payload.filename, filename, len);
	payload.filename[len] = '\0';

	mavlink_msg_file_transfer_start_encode(mav->sysid, mav->compid,
					       &msg, &payload);

	ret = mavlink_send_encoded_data(mav, &msg);
	if (ret < 0)
		return ret;

	return mavlink_receive_and_process(mav, timeout_ms);
}

int mavlink_send_ft_progress(struct mavlink_device *mav, u8 state,
			     u32 transferred, u32 total_bytes, u8 error_code)
{
	mavlink_file_transfer_progress_t payload;
	mavlink_message_t msg;

	payload.state = state;
	payload.bytes_transferred = transferred;
	payload.total_bytes = total_bytes;
	payload.error_code = error_code;

	mavlink_msg_file_transfer_progress_encode(mav->sysid, mav->compid,
						  &msg, &payload);

	return mavlink_send_encoded_data(mav, &msg);
}

int mavlink_send_ft_cancel(struct mavlink_device *mav, u8 direction)
{
	mavlink_file_transfer_cancel_t payload;
	mavlink_message_t msg;

	payload.direction = direction;

	mavlink_msg_file_transfer_cancel_encode(mav->sysid, mav->compid,
						&msg, &payload);

	return mavlink_send_encoded_data(mav, &msg);
}

int mavlink_send_ft_list(struct mavlink_device *mav, u8 file_count,
			 u8 list_index, u32 file_size, const char *filename)
{
	mavlink_file_transfer_list_t payload;
	mavlink_message_t msg;
	size_t len;

	payload.file_count = file_count;
	payload.list_index = list_index;
	payload.file_size = file_size;
	len = strnlen(filename, sizeof(payload.filename) - 1);
	memcpy(payload.filename, filename, len);
	payload.filename[len] = '\0';

	mavlink_msg_file_transfer_list_encode(mav->sysid, mav->compid,
					      &msg, &payload);

	return mavlink_send_encoded_data(mav, &msg);
}

int mavlink_send_ft_ack(struct mavlink_device *mav, u8 direction, u8 result, u32 file_size)
{
	mavlink_file_transfer_ack_t payload;
	mavlink_message_t msg;

	payload.direction = direction;
	payload.result = result;
	payload.file_size = file_size;

	mavlink_msg_file_transfer_ack_encode(mav->sysid, mav->compid,
					     &msg, &payload);

	return mavlink_send_encoded_data(mav, &msg);
}

int mavlink_ft_enter_passthrough(struct mavlink_device *mav)
{
	if (mav->ft.active) {
		pr_warn("FT passthrough already active\n");
		return -EBUSY;
	}

	mav->ft.active = 1;
	mav->ft.state = MAV_FT_TRANSFERRING;
	pr_info("FT entering ZMODEM passthrough mode\n");
	return 0;
}

void mavlink_ft_exit_passthrough(struct mavlink_device *mav)
{
	mav->ft.active = 0;
	mav->ft.state = MAV_FT_IDLE;
	pr_info("FT exited ZMODEM passthrough mode\n");
}

int mavlink_send_image_control(struct mavlink_device *mav, u8 action, u16 image_index)
{
	mavlink_image_control_t payload;
	mavlink_message_t msg;

	payload.action = action;
	payload.image_index = image_index;

	mavlink_msg_image_control_encode(mav->sysid, mav->compid, &msg, &payload);

	return mavlink_send_encoded_data(mav, &msg);
}

int mavlink_send_heartbeat(struct mavlink_device *mav, u8 status,
			   u32 uptime_s, u16 error_count)
{
	mavlink_system_heartbeat_t payload;
	mavlink_message_t msg;

	payload.status = status;
	payload.uptime_s = uptime_s;
	payload.error_count = error_count;

	mavlink_msg_system_heartbeat_encode(mav->sysid, mav->compid,
					    &msg, &payload);

	return mavlink_send_encoded_data(mav, &msg);
}

int mavlink_send_bootloader_update(struct mavlink_device *mav, u8 command,
				   u32 firmware_size, u32 firmware_crc32,
				   const char *filename, u32 timeout_ms)
{
	mavlink_bootloader_update_cmd_t payload;
	mavlink_message_t msg;
	size_t len;
	int ret;

	payload.command = command;
	payload.firmware_size = firmware_size;
	payload.firmware_crc32 = firmware_crc32;
	len = strnlen(filename, sizeof(payload.filename) - 1);
	memcpy(payload.filename, filename, len);
	payload.filename[len] = '\0';

	mavlink_msg_bootloader_update_cmd_encode(mav->sysid, mav->compid,
						 &msg, &payload);

	ret = mavlink_send_encoded_data(mav, &msg);
	if (ret < 0)
		return ret;

	return mavlink_receive_and_process(mav, timeout_ms);
}

int mavlink_send_bootloader_ack(struct mavlink_device *mav, u8 command,
				u8 result)
{
	mavlink_bootloader_ack_t payload;
	mavlink_message_t msg;

	payload.command = command;
	payload.result = result;

	mavlink_msg_bootloader_ack_encode(mav->sysid, mav->compid,
					  &msg, &payload);

	return mavlink_send_encoded_data(mav, &msg);
}

/**
 * @brief Background RX thread: continuously poll serial and process messages.
 *
 * The thread exits when mav->thread_running is cleared.
 */
static void *mavlink_rx_thread_func(void *arg)
{
	struct mavlink_device *mav = (struct mavlink_device *)arg;

	pr_info("RX thread started\n");

	while (mav->thread_running) {
		if (serial_port_data_available(mav->serial) > 0)
			mavlink_receive_and_process(mav, 10);
		else
			mdelay(5);
	}

	pr_info("RX thread stopped\n");
	return NULL;
}

int mavlink_start_rx_thread(struct mavlink_device *mav)
{
	mav->thread_running = 1;

	int ret = pthread_create(&mav->rx_thread, NULL,
				 mavlink_rx_thread_func, mav);
	if (ret != 0) {
		mav->thread_running = 0;
		pr_err("Failed to create RX thread: %d\n", ret);
		return ret;
	}

	return 0;
}

void mavlink_stop_rx_thread(struct mavlink_device *mav)
{
	mav->thread_running = 0;
	pthread_join(mav->rx_thread, NULL);
}

#ifdef DESIGN_VERIFICATION_MAVLINK

int t_mavlink_master_slave_sim(int argc, char *argv[])
{
	struct serial_port *master_port = NULL;
	struct serial_port *slave_port = NULL;
	struct mavlink_device *master = NULL;
	struct mavlink_device *slave = NULL;
	int ret = 0;

	pr_info("Starting MAVLink master-slave simulation test\n");

	master_port = serial_port_alloc(&fake_serial_port_ops, NULL);
	if (!master_port) {
		pr_err("Failed to allocate master serial port\n");
		return -ENOMEM;
	}

	slave_port = serial_port_alloc(&fake_serial_port_ops, NULL);
	if (!slave_port) {
		pr_err("Failed to allocate slave serial port\n");
		ret = -ENOMEM;
		goto out;
	}

	serial_port_start_thread(master_port, SERIAL_PORT_DF_OTHERS,
				 NULL, slave_port);
	serial_port_start_thread(slave_port, SERIAL_PORT_DF_OTHERS,
				 NULL, master_port);

	master = mavlink_init(master_port, 1, 0, NULL);
	slave = mavlink_init(slave_port, 2, 0, NULL);

	if (!master || !slave) {
		pr_err("Failed to init MAVLink devices\n");
		ret = -ENOMEM;
		goto out;
	}

	ret = mavlink_start_rx_thread(slave);
	if (ret < 0) {
		pr_err("Failed to start slave RX thread: %d\n", ret);
		goto out;
	}

	mdelay(100);

	for (u8 motor_id = 0; motor_id < MAVLINK_MOTOR_COUNT; motor_id++) {
		s32 speed = 1000 + motor_id * 500;

		pr_info("Sending MOTOR_CONTROL motor=%u speed=%d\n",
			motor_id, speed);
		ret = mavlink_send_motor_control(master, motor_id, speed,
						 MAVLINK_DIRECTION_FORWARD, 100);
		if (ret < 0) {
			pr_err("Failed to send MOTOR_CONTROL: %d\n", ret);
			continue;
		} else if (ret == 0) {
			pr_warn("Timeout waiting for motor %u response\n",
				motor_id);
		}

		mdelay(200);
	}

	mavlink_stop_rx_thread(slave);

out:
	mavlink_free(master);
	mavlink_free(slave);

	if (slave_port) {
		serial_port_stop_thread(slave_port);
		serial_port_free(slave_port);
	}
	if (master_port) {
		serial_port_stop_thread(master_port);
		serial_port_free(master_port);
	}

	pr_info("MAVLink master-slave simulation test completed with ret=%d\n",
		ret);
	return ret;
}

int t_mavlink_msg_pack_parse(int argc, char *argv[])
{
	struct serial_port *port = NULL;
	struct mavlink_device *mav = NULL;
	int ret = 0;

	pr_info("Starting MAVLink message packing/parsing test\n");

	port = serial_port_alloc(&fake_serial_port_ops, NULL);
	if (!port) {
		pr_err("Failed to allocate serial port\n");
		return -ENOMEM;
	}

	serial_port_start_thread(port, SERIAL_PORT_DF_OTHERS, NULL, port);

	mav = mavlink_init(port, 1, 0, NULL);
	if (!mav) {
		pr_err("Failed to init MAVLink device\n");
		ret = -ENOMEM;
		goto out;
	}

	ret = mavlink_send_motor_control(mav, 0, 1500,
					 MAVLINK_DIRECTION_FORWARD, 50);
	if (ret < 0)
		pr_err("Failed to send MOTOR_CONTROL: %d\n", ret);
	else
		pr_info("MOTOR_CONTROL sent OK (tx=%u, processed=%d)\n",
			mav->tx_count, ret);

	ret = 0;

out:
	mavlink_free(mav);
	if (port) {
		serial_port_stop_thread(port);
		serial_port_free(port);
	}

	pr_info("MAVLink message packing/parsing test completed\n");
	return ret;
}

#endif /* DESIGN_VERIFICATION_MAVLINK */
