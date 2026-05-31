/**
 * @file pov_main.c
 * @brief Host (rotating end) main state machine
 * @note Implements the display workflow:
 *   WAIT_SLAVE → SEND_SPEED → DISPLAYING
 *
 * File transfer from phone via ZMODEM (app_port):
 *   Phone sends ZMODEM start → rotor calls lrzsz_rz() → saves .pov files
 *   → rescan images and reload
 */

#include <linux/kernel.h>
#include <linux/printk.h>
#include <linux/errno.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/random.h>

#include <kinetis/basic-timer.h>
#include <kinetis/fatfs-intf.h>
#include <kinetis/mavlink.h>
#include <kinetis/serial-port.h>
#include "../../drivers/kinetis/lrzsz/zmodem.h"
#include "../../drivers/kinetis/bootloader/bootloader.h"

#include "rotor.h"

int pov_spi_multi_transmit(struct pov_rotor *rotor);

static int pov_set_rgb(struct pov_rotor *rotor)
{
	/* Wait for previous transfer to complete */
	int ret;

	ret = pov_spi_multi_transmit(rotor);

	/* Log head/tail of each SPI group for debug */
	// for (int g = 0; g < POV_SPI_GROUPS; g++) {
	// 	const u8 *data = (const u8 *)rotor->rgb_strip[g];
	// 	pr_debug("SPI%u [%02x %02x %02x %02x ... %02x %02x %02x %02x] (%u bytes)\n",
	// 		 g,
	// 		 data[0], data[1], data[2], data[3],
	// 		 data[POV_BYTES_PER_GROUP - 4],
	// 		 data[POV_BYTES_PER_GROUP - 3],
	// 		 data[POV_BYTES_PER_GROUP - 2],
	// 		 data[POV_BYTES_PER_GROUP - 1],
	// 		 POV_BYTES_PER_GROUP);
	// }

	return ret;
}

int pov_create_fake_images(const char *path)
{
	struct pov_image_header header;
	FRESULT res;
	FIL fp;
	UINT bw;
	char fullpath[64];
	u8 *buf = NULL;
	const char *fake_image_names[FAKE_IMAGE_COUNT] = {
		"rainbow.pov",
		"stars.pov",
		"checker.pov",
	};
	int ret = 0;

	res = f_mkdir(path);
	if (res != FR_OK && res != FR_EXIST) {
		pr_err("mkdir '%s' failed: %d\n", path, res);
		return -EACCES;
	}

	buf = kmalloc(POV_BYTES_PER_COL, GFP_KERNEL);
	if (!buf) {
		return -ENOMEM;
	}

	for (int i = 0; i < FAKE_IMAGE_COUNT; i++) {
		snprintf(fullpath, sizeof(fullpath), "%s/%s",
			path, fake_image_names[i]);

		ret = fatfs_find_file((char *)path, (char *)fake_image_names[i]);
		if (ret == 0) {
			pr_info("image already exists, skipping: %s\n", fullpath);
			continue;
		}

		res = f_open(&fp, fullpath, FA_CREATE_ALWAYS | FA_WRITE);
		if (res != FR_OK) {
			pr_err("failed to create '%s': %d\n", fullpath, res);
			ret = -EACCES;
			goto out;
		}

		header.magic = POV_IMG_MAGIC;
		header.version = POV_IMG_VERSION;
		header.columns = POV_DISPLAY_COLS;
		header.leds_per_group = POV_LEDS_PER_GROUP;
		header.spi_groups = POV_SPI_GROUPS;
		memset(header.reserved, 0, sizeof(header.reserved));

		res = f_write(&fp, &header, sizeof(header), &bw);
		if (res != FR_OK || bw != sizeof(header)) {
			pr_err("write header failed: %d\n", res);
			ret = -EACCES;
			f_close(&fp);
			goto out;
		}

		for (int col = 0; col < POV_DISPLAY_COLS; col++) {
			get_random_bytes(buf, POV_BYTES_PER_COL);
			res = f_write(&fp, buf, POV_BYTES_PER_COL, &bw);
			if (res != FR_OK || bw != POV_BYTES_PER_COL) {
				pr_err("write col %d failed: %d\n", col, res);
				ret = -EACCES;
				f_close(&fp);
				goto out;
			}
		}

		f_close(&fp);
		pr_info("created fake image: %s (%d bytes)\n",
			fullpath, (int)(sizeof(header) + POV_DISPLAY_COLS * POV_BYTES_PER_COL));
	}

	pr_info("created %d fake images in %s\n", FAKE_IMAGE_COUNT, path);

out:
	kfree(buf);
	return ret;
}

int pov_scan_images(struct pov_rotor *pov)
{
	DIR dir;
	FILINFO fno;
	FRESULT res;
	int ret = 0;

	pov->image_count = 0;

	res = f_opendir(&dir, POV_IMAGE_PATH);
	if (res != FR_OK) {
		pr_err("failed to open '%s': %d\n", POV_IMAGE_PATH, res);
		return -ENOENT;
	}

	while (1) {
		/* Guard against array overflow */
		if (pov->image_count >= POV_MAX_IMAGE_COUNT) {
			pr_warn("max image count (%d) reached, stopping scan\n",
				POV_MAX_IMAGE_COUNT);
			break;
		}

		res = f_readdir(&dir, &fno);
		if (res != FR_OK || fno.fname[0] == 0) {
			break;    /* End of directory or error */
		}

		/* Skip directories and system files */
		if (fno.fattrib & (AM_DIR | AM_SYS)) {
			continue;
		}

		/* Check if it's an .pov file */
		if (fno.fsize > 0) {
			char *ext = strrchr(fno.fname, '.');
			if (ext && strcmp(ext, ".pov") == 0) {
				pov->image_names[pov->image_count] = kstrdup(fno.fname, GFP_KERNEL);
				if (!pov->image_names[pov->image_count]) {
					pr_err("failed to allocate name for '%s'\n", fno.fname);
					ret = -ENOMEM;
					goto out;
				}
				pr_info("found image: %s\n", pov->image_names[pov->image_count]);
				pov->image_count++;
			}
		}
	}

out:
	f_closedir(&dir);

	/* Release already allocated names on failure */
	if (ret < 0) {
		for (u8 i = 0; i < pov->image_count; i++) {
			kfree(pov->image_names[i]);
			pov->image_names[i] = NULL;
		}
		pov->image_count = 0;
	}

	pr_info("found %d images in %s\n", pov->image_count, POV_IMAGE_PATH);

	return ret;
}

int pov_load_images(struct pov_rotor *pov, const char *filename)
{
	FRESULT res;
	UINT br;
	char file_path[512];
	int ret;

	fatfs_build_path(file_path, sizeof(file_path), POV_IMAGE_PATH, filename);

	res = f_open(&pov->image_file, file_path, FA_READ);
	if (res != FR_OK) {
		pr_err("failed to open '%s': %d\n", filename, res);
		return -EIO;
	}

	res = f_read(&pov->image_file, &pov->image_header, sizeof(pov->image_header), &br);
	if (res != FR_OK) {
		pr_err("failed to read header: %d\n", res);
		ret = -EIO;
		goto file_err;
	}
	pov->pending_column += br;

	if (pov->image_header.magic != POV_IMG_MAGIC) {
		pr_err("invalid magic: 0x%08x\n", pov->image_header.magic);
		ret = -EINVAL;
		goto file_err;
	}

	if (pov->image_header.columns != POV_DISPLAY_COLS ||
		pov->image_header.spi_groups != POV_SPI_GROUPS) {
		pr_err("image format mismatch (cols=%d, groups=%d)\n",
			pov->image_header.columns, pov->image_header.spi_groups);
		ret = -EINVAL;
		goto file_err;
	}

	pov->images_open = 1;
	pr_info("loaded '%s' (%d cols, %d groups)\n",
		filename, pov->image_header.columns, pov->image_header.spi_groups);
	return 0;

file_err:
	f_close(&pov->image_file);
	return ret;
}

int pov_read_next_column(struct pov_rotor *pov, u8 *data)
{
	FRESULT res;
	UINT br;

	res = f_read(&pov->image_file, data, POV_BYTES_PER_COL, &br);
	if (res != FR_OK || br != POV_BYTES_PER_COL) {
		if (res == FR_OK && br == 0) {
			return -ENOENT;    /* End of image */
		}
		pr_err("read column failed: %d (got %u)\n", res, br);
		return -EIO;
	}
	pov->pending_column += br;

	return 0;
}

void pov_close_image(struct pov_rotor *pov)
{
	f_close(&pov->image_file);
	pov->images_open = 0;
	pov->pending_column = 0;

	pr_info("image closed\n");
}

static const char *pov_state_name(enum pov_fsm_state s)
{
	static const char *names[] = {
		"WAIT_MOTOR", "WAIT_PHONE", "SEND_SPEED", "DISPLAYING", "CHECK_CMD", "ERROR"
	};
	if ((unsigned)s >= sizeof(names) / sizeof(names[0])) {
		return "?";
	}
	return names[s];
}

static void set_fsm_state(struct pov_rotor *rotor, enum pov_fsm_state new_state)
{
	if (new_state != rotor->state) {
		pr_info("state: %s -> %s\n", pov_state_name(rotor->state), pov_state_name(new_state));
	}

	rotor->state = new_state;
}

static int pov_receive_image_files(struct pov_rotor *rotor)
{
	int ret;

	pr_info("ZMODEM: waiting for phone to send files on app_port...\n");

	/*
	 * lrzsz_rz() blocks until transfer completes.
	 * Files are saved to POV_IMAGE_PATH directory.
	 * Returns number of bytes received, or 0 on error.
	 */
	ret = lrzsz_rz(rotor->app_port, POV_IMAGE_PATH);
	if (ret <= 0) {
		pr_warn("ZMODEM: receive failed or cancelled (%d)\n", ret);
		return -EIO;
	}

	pr_info("ZMODEM: received %d bytes, rescanning images...\n", ret);

	/* Close currently playing image if any */
	if (rotor->images_open) {
		pov_close_image(rotor);
	}

	/* Free old image names and rescan */
	for (u8 i = 0; i < rotor->image_count; i++) {
		kfree(rotor->image_names[i]);
		rotor->image_names[i] = NULL;
	}

	ret = pov_scan_images(rotor);
	if (ret < 0) {
		return ret;
	}

	/* Reload first image */
	if (rotor->image_count > 0) {
		ret = pov_load_images(rotor, rotor->image_names[0]);
		if (ret < 0) {
			return ret;
		}
		rotor->image_current = 0;
	} else {
		pr_warn("ZMODEM: no .pov images found after transfer\n");
	}

	/* If currently displaying, keep showing; else start */
	if (rotor->state == POV_STATE_DISPLAYING || rotor->state == POV_STATE_SEND_SPEED) {
		return 0;
	}

	set_fsm_state(rotor, POV_STATE_WAIT_MOTOR);
	return 0;
}

static int display_image(struct pov_rotor *rotor)
{
	u32 last_angle = hall_get_angle(rotor->hall);
	u32 spin_count;
	int ret;

	while (1) {
		/* Wait for hall to advance to the next column,
		 * with timeout to prevent infinite loop on sensor failure */
		spin_count = 0;
		while (hall_get_angle(rotor->hall) == last_angle) {
			if (++spin_count > POV_DISPLAY_COLS * 100) {
				pr_err("display: hall timeout at column %u\n", last_angle);
				return -ETIMEDOUT;
			}
		}

		last_angle = hall_get_angle(rotor->hall);

		ret = pov_read_next_column(rotor, (u8 *)rotor->rgb_strip);
		if (ret == -ENOENT) {
			break;
		} else if (ret == -EIO) {
			return ret;
		}

		ret = pov_set_rgb(rotor);
		if (ret < 0) {
			return ret;
		}
	}

	return 0;
}

static int pov_switch_image(struct pov_rotor *rotor, u8 index)
{
	FRESULT res;

	if (index >= rotor->image_count) {
		return -EINVAL;
	}

	if (rotor->images_open) {
		pov_close_image(rotor);
	}

	res = f_open(&rotor->image_file, rotor->image_names[index], FA_READ);
	if (res != FR_OK) {
		return -EIO;
	}

	res = f_read(&rotor->image_file, &rotor->image_header,
			sizeof(rotor->image_header), NULL);
	if (res != FR_OK) {
		goto err_close;
	}

	if (rotor->image_header.magic != POV_IMG_MAGIC) {
		goto err_close;
	}

	if (rotor->image_header.columns != POV_DISPLAY_COLS ||
		rotor->image_header.spi_groups != POV_SPI_GROUPS) {
		goto err_close;
	}

	rotor->images_open = 1;
	rotor->pending_column = sizeof(rotor->image_header);
	rotor->image_current = index;
	pr_info("switched to image [%d]: %s (%d cols)\n",
		index, rotor->image_names[index],
		rotor->image_header.columns);
	return 0;

err_close:
	f_close(&rotor->image_file);
	return -EIO;
}

static int pov_handle_end_of_image(struct pov_rotor *rotor)
{
	struct mavlink_image_cmd *cmd = &rotor->motor_mavlink->image_cmd;
	u8 action = cmd->pending ? cmd->action : MAV_IMAGE_REPLAY;
	FRESULT res;

	cmd->pending = 0;

	switch (action) {
	case MAV_IMAGE_REPLAY:
		res = f_lseek(&rotor->image_file, sizeof(struct pov_image_header));
		if (res != FR_OK) {
			return -EIO;
		}
		rotor->pending_column = sizeof(struct pov_image_header);
		pr_info("replaying current image %s\n", rotor->image_names[rotor->image_current]);
		return 0;

	case MAV_IMAGE_NEXT: {
		u8 next = (rotor->image_current + 1) % rotor->image_count;
		return pov_switch_image(rotor, next);
	}

	case MAV_IMAGE_PREVIOUS: {
		u8 prev = (rotor->image_current == 0) ?
			rotor->image_count - 1 : rotor->image_current - 1;
		return pov_switch_image(rotor, prev);
	}

	case MAV_IMAGE_PLAY_INDEX:
		return pov_switch_image(rotor, cmd->image_index);

	default:
		pr_warn("unknown image action %d, replaying\n", action);
		res = f_lseek(&rotor->image_file, sizeof(struct pov_image_header));
		if (res != FR_OK) {
			return -EIO;
		}
		rotor->pending_column = sizeof(struct pov_image_header);
		return 0;
	}
}

static int waiting_for_motor_connection(struct pov_rotor *rotor)
{
	mavlink_system_heartbeat_t payload;

	if (serial_port_data_available(rotor->motor_mavlink->serial) == 0) {
		return -ENODEV;
	}

	mavlink_receive_and_process(rotor->motor_mavlink, 10);
	mavlink_msg_system_heartbeat_decode(&rotor->motor_mavlink->rx_msg, &payload);

	if (payload.status == MAV_SYS_ACTIVE) {
		pr_info("motor connected!\n");
		set_fsm_state(rotor, POV_STATE_SEND_SPEED);
		return mavlink_send_heartbeat(rotor->motor_mavlink, MAV_SYS_ACTIVE, 0, 0);
	}

	if (payload.status == MAV_SYS_FAULT) {
		pr_err("motor in fault state\n");
		return -EIO;
	}

	return -ENODEV;
}

static int waiting_for_phone_connection(struct pov_rotor *rotor)
{
	mavlink_system_heartbeat_t payload;

	if (serial_port_data_available(rotor->app_mavlink->serial) == 0) {
		return -ENODEV;
	}

	mavlink_receive_and_process(rotor->app_mavlink, 10);
	mavlink_msg_system_heartbeat_decode(&rotor->app_mavlink->rx_msg, &payload);

	/* Phone sends HEARTBEAT during handshake — any received
	 * MAVLink message confirms the phone is alive. */
	if (payload.status == MAV_SYS_ACTIVE) {
		pr_info("phone connected!\n");
		set_fsm_state(rotor, POV_STATE_SEND_SPEED);
		return mavlink_send_heartbeat(rotor->app_mavlink, MAV_SYS_ACTIVE, 0, 0);
	}

	return -ENODEV;
}

static int set_motor_speed(struct pov_rotor *rotor, u16 rpm)
{
	u8 query_count = 0;
	int ret;
	s32 speed;

	ret = mavlink_send_motor_control(rotor->motor_mavlink, 0, rpm, POV_DIR_FORWARD, 500);
	if (ret < 0) {
		return ret;
	}

	pr_info("sent target RPM=%d to motor\n", rpm);

	while (query_count < POV_SPEED_QUERY_RETRIES) {
		ret = mavlink_query_motor_status(rotor->motor_mavlink, 0, 500);
		if (ret < 0) {
			return ret;
		}

		speed = rotor->motor_mavlink->motors[0].current_speed;
		if (abs(speed - (s32)rpm) < 50 &&
			rotor->motor_mavlink->motors[0].direction == POV_DIR_FORWARD) {
			pr_info("motor reached target RPM: %d (current=%d)\n", rpm, speed);
			break;
		}

		query_count++;
		mdelay(POV_SPEED_QUERY_INTERVAL_MS);
	}

	speed = hall_get_rpm(rotor->hall);
	if (abs(speed - (s32)rpm) > 50) {
		pr_warn("hall RPM %d out of range (target %d)\n", speed, rpm);
		return -EAGAIN;
	}

	pr_info("speed confirmed, entering display mode\n");
	set_fsm_state(rotor, POV_STATE_DISPLAYING);

	return 0;
}

static int display_loop(struct pov_rotor *rotor)
{
	u64 t_start, t_end;
	int ret;

	t_start = basic_timer_get_us();

	ret = display_image(rotor);
	if (ret) {
		goto err;
	}

	t_end = basic_timer_get_us() - t_start;
	pr_debug("image done: %llu us total, %llu us avg per col (%u cols)\n",
		t_end, t_end / POV_DISPLAY_COLS, POV_DISPLAY_COLS);

	set_fsm_state(rotor, POV_STATE_CHECK_CMD);
	return 0;

err:
	set_fsm_state(rotor, POV_STATE_ERROR);
	return ret;
}

static void error_occurred(struct pov_rotor *rotor)
{
	set_motor_speed(rotor, 0);
	set_fsm_state(rotor, POV_STATE_WAIT_MOTOR);
}

void pov_rotor_free(struct pov_rotor *rotor)
{
	if (!rotor) {
		return;
	}

	if (rotor->images_open) {
		pov_close_image(rotor);
	}

	for (u8 i = 0; i < rotor->image_count; i++) {
		kfree(rotor->image_names[i]);
	}

	if (rotor->hall) {
		kfree(rotor->hall);
	}

	if (rotor->app_mavlink) {
		mavlink_free(rotor->app_mavlink);
	}

	if (rotor->motor_mavlink) {
		mavlink_free(rotor->motor_mavlink);
	}

	if (rotor->motor_port) {
		serial_port_free(rotor->motor_port);
	}

	if (rotor->app_port) {
		serial_port_free(rotor->app_port);
	}

	kfree(rotor);
}

struct pov_rotor *pov_rotor_alloc(struct flash_ops *flash, struct serial_port_ops *motor_serial_ops, struct serial_port_ops *app_serial_ops,
	u32(*read_rotated_time)(struct hall_device *dev))
{
	struct pov_rotor *rotor;
	int ret;

	ret = fatfs_init();
	if (ret) {
		return NULL;
	}

	rotor = kzalloc(sizeof(*rotor), GFP_KERNEL);
	if (!rotor) {
		return NULL;
	}

	rotor->flash = flash;

	/* Port to stator (MAVLink motor control) */
	rotor->motor_port = serial_port_alloc(motor_serial_ops, "rotor-motor");
	if (!rotor->motor_port) {
		goto free_dev;
	}

	rotor->motor_mavlink = mavlink_init(rotor->motor_port, 1, 1, "rotor-motor_mavlink");
	if (!rotor->motor_mavlink) {
		goto free_dev;
	}

	/* Port to phone (MAVLink + ZMODEM file transfer) */
	rotor->app_port = serial_port_alloc(app_serial_ops, "rotor-app");
	if (!rotor->app_port) {
		goto free_dev;
	}

	rotor->app_mavlink = mavlink_init(rotor->app_port, 1, 2, "rotor-app-mavlink");
	if (!rotor->app_mavlink) {
		goto free_dev;
	}

	rotor->hall = hall_alloc_dev(POV_DISPLAY_COLS, read_rotated_time);
	if (!rotor->hall) {
		goto free_dev;
	}

	return rotor;

free_dev:
	pov_rotor_free(rotor);
	return NULL;
}

static void pov_check_phone_cmd(struct pov_rotor *rotor)
{
	u32 old_rx = rotor->app_mavlink->rx_count;

	/* Process pending MAVLink messages (phone commands via app_port) */
	if (serial_port_data_available(rotor->app_port) > 0) {
		mavlink_receive_and_process(rotor->app_mavlink, 10);
	}

	/* Only dispatch if a new message was received */
	if (rotor->app_mavlink->rx_count == old_rx) {
		goto skip_cmd;
	}

	switch (rotor->app_mavlink->rx_msg.msgid) {
	/* ─── File Transfer ─────────────────────────────────── */
	case MAVLINK_MSG_ID_FILE_TRANSFER_START: {
		pr_info("cmd: file transfer — '%s' (%u bytes)\n",
			rotor->app_mavlink->ft.filename,
			rotor->app_mavlink->ft.total_bytes);

		rotor->app_mavlink->ft.state = MAV_FT_IDLE;
		pov_receive_image_files(rotor);
		set_fsm_state(rotor, POV_STATE_DISPLAYING);
		return;
	}

	case MAVLINK_MSG_ID_FILE_TRANSFER_ACK:
		pr_info("cmd: file transfer ack\n");
		/* TODO: handle transfer ACK from phone */
		break;

	case MAVLINK_MSG_ID_FILE_TRANSFER_PROGRESS:
		pr_debug("cmd: ft progress state=%d\n",
			rotor->app_mavlink->ft.state);
		/* TODO: update progress bar or LED indicator */
		break;

	case MAVLINK_MSG_ID_FILE_TRANSFER_CANCEL:
		pr_info("cmd: file transfer cancelled\n");
		/* TODO: abort ZMODEM transfer in progress */
		break;

	case MAVLINK_MSG_ID_FILE_TRANSFER_LIST: {
		mavlink_file_transfer_list_t fl;
		mavlink_msg_file_transfer_list_decode(
			&rotor->app_mavlink->rx_msg, &fl);
		pr_info("cmd: ft list [%u/%u] %s (%u bytes)\n",
			fl.list_index, fl.file_count,
			fl.filename, fl.file_size);
		/* TODO: display file list to user */
		break;
	}

	/* ─── Image Control ─────────────────────────────────── */
	case MAVLINK_MSG_ID_IMAGE_CONTROL:
		pr_info("cmd: image control\n");
		/* Relay IMAGE_CONTROL from phone to main motor_mavlink so that
		 * pov_handle_end_of_image() picks it up from rotor->motor_mavlink. */
		rotor->motor_mavlink->image_cmd.pending = rotor->app_mavlink->image_cmd.pending;
		rotor->motor_mavlink->image_cmd.action = rotor->app_mavlink->image_cmd.action;
		rotor->motor_mavlink->image_cmd.image_index = rotor->app_mavlink->image_cmd.image_index;
		rotor->app_mavlink->image_cmd.pending = 0;
		break;

	case MAVLINK_MSG_ID_IMAGE_ACK:
		pr_info("cmd: image ack\n");
		/* TODO: confirm image switch completed */
		break;

	/* ─── Motor Control (relayed from stator) ──────────── */
	case MAVLINK_MSG_ID_MOTOR_CONTROL: {
		mavlink_motor_control_t mc;
		mavlink_msg_motor_control_decode(
			&rotor->app_mavlink->rx_msg, &mc);
		pr_info("cmd: motor control target=%u speed=%d\n",
			mc.target_id, mc.speed);
		/* TODO: relay to stator */
		break;
	}

	case MAVLINK_MSG_ID_MOTOR_STATUS_QUERY:
		pr_debug("cmd: motor status query\n");
		/* TODO: query stator and respond */
		break;

	case MAVLINK_MSG_ID_MOTOR_STATUS: {
		mavlink_motor_status_t ms;
		mavlink_msg_motor_status_decode(
			&rotor->app_mavlink->rx_msg, &ms);
		pr_info("cmd: motor status id=%u speed=%d\n",
			ms.motor_id, ms.current_speed);
		break;
	}

	case MAVLINK_MSG_ID_MOTOR_ACK:
		pr_debug("cmd: motor ack\n");
		break;

	/* ─── System ───────────────────────────────────────── */
	case MAVLINK_MSG_ID_SYSTEM_HEARTBEAT:
		pr_debug("cmd: heartbeat\n");
		break;

	/* ─── Bootloader ──────────────────────────────────── */
	case MAVLINK_MSG_ID_BOOTLOADER_UPDATE_CMD: {
		struct bootloader_config bl_cfg;

		pr_info("cmd: bootloader update — cmd=%d size=%u name=%s\n",
			rotor->app_mavlink->bl.command,
			rotor->app_mavlink->bl.firmware_size,
			rotor->app_mavlink->bl.filename);

		/* Build config to persist across reset */
		memset(&bl_cfg, 0, sizeof(bl_cfg));
		bl_cfg.magic = BOOTLOADER_MAGIC;
		bl_cfg.version = 1;
		bl_cfg.mode = BL_MODE_UPDATE;
		bl_cfg.firmware_size = rotor->app_mavlink->bl.firmware_size;
		bl_cfg.firmware_crc32 = rotor->app_mavlink->bl.firmware_crc32;

		if (!rotor->flash) {
			pr_err("No flash ops configured, cannot enter update mode\n");
			break;
		}

		if (rotor->flash->erase(CONFIG_BASE_ADDR, sizeof(bl_cfg)) < 0 ||
		    rotor->flash->write(CONFIG_BASE_ADDR, (const u8 *)&bl_cfg, sizeof(bl_cfg)) < 0) {
			pr_err("Failed to write boot config to flash\n");
			break;
		}

		pr_info("Boot flag set to UPDATE mode, resetting...\n");

		/* In production: board-level system_reset would be called here.
		 * In simulation we just return (effective reset via main loop). */
		break;
	}

	default:
		pr_debug("cmd: unhandled msgid=%u\n",
			rotor->app_mavlink->rx_msg.msgid);
		break;
	}

skip_cmd:
	/* IMAGE_CONTROL / replay: pov_handle_end_of_image reads
	 * rotor->app_mavlink->image_cmd internally and dispatches. */
	if (pov_handle_end_of_image(rotor) < 0) {
		set_fsm_state(rotor, POV_STATE_ERROR);
	} else {
		set_fsm_state(rotor, POV_STATE_DISPLAYING);
	}
}

int pov_rotor_display(struct pov_rotor *rotor)
{
	int ret;

// #if KINETIS_FAKE_SIM
	ret = pov_create_fake_images(POV_IMAGE_PATH);
	if (ret < 0) {
		goto out;
	}
// #endif

	ret = pov_scan_images(rotor);
	if (ret < 0) {
		goto out;
	}

	if (rotor->image_count == 0) {
		pr_err("no .pov images found in '%s'\n", POV_IMAGE_PATH);
		ret = -ENOENT;
		goto out;
	}

	ret = pov_load_images(rotor, rotor->image_names[0]);
	if (ret < 0) {
		goto out;
	}

	pr_info("waiting for motor connection\n");

	while (1) {
		switch (rotor->state) {
		case POV_STATE_WAIT_MOTOR:
			waiting_for_motor_connection(rotor);
			break;

		case POV_STATE_WAIT_PHONE:
			waiting_for_phone_connection(rotor);
			break;

		case POV_STATE_SEND_SPEED:
			ret = set_motor_speed(rotor, POV_TARGET_RPM);
			if (ret < 0) {
				pr_warn("set_motor_speed failed (%d), retrying...\n", ret);
			}
			break;

		case POV_STATE_DISPLAYING:
			display_loop(rotor);
			break;

		case POV_STATE_CHECK_CMD:
			pov_check_phone_cmd(rotor);
			break;

		case POV_STATE_ERROR:
			error_occurred(rotor);
			goto out;

		default:
			rotor->state = POV_STATE_WAIT_MOTOR;
			break;
		}
	}

out:
	return ret;
}
