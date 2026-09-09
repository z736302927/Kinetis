#include <linux/printk.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/random.h>
#include <linux/string.h>
#include <linux/timekeeping.h>
#include <linux/crc32.h>
#include <fake-mcu/interface.h>

#include <kinetis/tim-task.h>
#include <kinetis/basic-timer.h>
#include <kinetis/serial-port.h>
#include <kinetis/mavlink.h>
#include <kinetis/fatfs-intf.h>
#include <kinetis/bootloader.h>

#include "lrzsz/zmodem.h"
#include "fatfs/ff.h"
#include "fatfs/drivers/fake_ram_diskio.h"

#include "config.h"
#include "test.h"
#include "rotor.h"
#include "stator.h"
#include "interface.h"

#include <pthread.h>
#include <unistd.h>
#include <windows.h>

enum pov_test_scenario {
	POV_TEST_RANDOM = 0,
	POV_TEST_UPLOAD = 1,
	POV_TEST_NEXT = 2,
	POV_TEST_PREVIOUS = 3,
	POV_TEST_REPLAY = 4,
	POV_TEST_PLAY_INDEX = 5,
	POV_TEST_QUERY_MOTOR = 6,
	POV_TEST_BURST = 7,
	POV_TEST_DOWNLOAD = 8,
	POV_TEST_LIST = 9,
	POV_TEST_STARTUP_READY = 10,
	POV_TEST_NEXT_WRAP = 11,
	POV_TEST_PREVIOUS_WRAP = 12,
	POV_TEST_IMAGE_SEQUENCE = 13,
	POV_TEST_LIST_ALL_LAYERS = 14,
	POV_TEST_DOWNLOAD_ALL_LAYERS = 15,
	POV_TEST_UPLOAD_ALL_LAYERS = 16,
	POV_TEST_TRANSFER_ROUND_TRIP = 17,
	POV_TEST_MIXED_WORKFLOW = 18,
	POV_TEST_INVALID_IMAGE = 19,
	POV_TEST_INVALID_TRANSFER = 20,
	POV_TEST_FT_STATE = 21,
	POV_TEST_MOTOR_CONTROL = 22,
	POV_TEST_MAVLINK_DROP = 23,
	POV_TEST_MAVLINK_CORRUPT = 24,
	POV_TEST_MAVLINK_RANDOM_FAULT = 25,
	POV_TEST_STORAGE_READ_FAULT = 26,
	POV_TEST_STORAGE_WRITE_FAULT = 27,
	POV_TEST_LINK_OUTAGE = 28,
	POV_TEST_STORAGE_FULL = 29,
	POV_TEST_STORAGE_READ_ONLY = 30,
	POV_TEST_TRANSFER_CANCEL_ACTIVE = 31,
	POV_TEST_BOOTLOADER_UPDATE = 32,
	POV_TEST_MULTIARM_IMAGE_SYNC = 33,
	POV_TEST_MULTIARM_FILE_CHAIN = 34,
	POV_TEST_MULTIARM_LINK_RECOVERY = 35,
	POV_TEST_MULTIARM_IMAGE_SEQUENCE = 36,
	POV_TEST_MULTIARM_FIRMWARE_UPDATE = 37,
	POV_TEST_MULTIARM_BOOT_REJOIN = 38,
	POV_TEST_MULTIARM_FIRMWARE_CANCEL = 39,
	POV_TEST_MULTIARM_FIRMWARE_STORAGE_FULL = 40,
	POV_TEST_MULTIARM_FIRMWARE_READ_ONLY = 41,
	POV_TEST_MULTIARM_FIRMWARE_CORRUPT = 42,
	POV_TEST_MULTIARM_FIRMWARE_FLASH = 43,
	POV_TEST_MULTIARM_FIRMWARE_RESET = 44,
	POV_TEST_MULTIARM_IMAGE_CANCEL = 45,
	POV_TEST_MULTIARM_IMAGE_STORAGE = 46,
	POV_TEST_MULTIARM_COMMAND_RELIABILITY = 47,
	POV_TEST_MULTIARM_SLAVE_REJOIN = 48,
	POV_TEST_MULTIARM_MASTER_REJOIN = 49,
	POV_TEST_FOUR_ARM_CHAIN = 50,
	POV_TEST_MULTIARM_MIXED_STRESS = 51,
	POV_TEST_SCENARIO_COUNT,
};

struct pov_test_context {
	struct pov_rotor *rotor;
	struct pov_rotor *arms[POV_ARM_COUNT];
	u8 arm_count;
	u32 scenario;
	u32 repeat;
	u32 duration_s;
	u32 interval_ms;
	const char *argument;
	ktime_t deadline;
	volatile u8 running;
	int result;
};

struct phone_context {
	struct pov_test_context *test;
	struct serial_port *port;
	struct mavlink_device *mavlink;
};

struct arm_thread_arg {
	struct pov_rotor *rotor;
	volatile u8 *running;
};

struct controlled_send_arg {
	struct serial_port *port;
	const char **files;
	struct lrzsz_transfer_control *control;
	int result;
};

static volatile u8 stator_thread_switch;

static int phone_drain_link(struct phone_context *phone);

static void *stator_thread_func(void *arg)
{
	struct pov_stator *stator = arg;

	while (stator_thread_switch) {
		pov_stator_fsm(stator);
		tim_task_loop();
		usleep(100);
	}

	return NULL;
}

static bool phone_wait(struct pov_test_context *test, u32 delay_ms)
{
	while (test->running && delay_ms) {
		u32 step_ms = min(delay_ms, 100u);

		if (test->deadline) {
			s64 remaining_ms = ktime_ms_delta(test->deadline, ktime_get());
			if (remaining_ms <= 0) {
				test->running = 0;
				break;
			}
			if (remaining_ms < step_ms)
				step_ms = remaining_ms;
		}
		usleep(step_ms * 1000);
		delay_ms -= step_ms;
	}

	return test->running;
}

static u32 phone_pick_operation(const u32 *weights, u32 n)
{
	u32 total = 0;
	u32 sum = 0;
	u32 r;

	for (u32 i = 0; i < n; i++)
		total += weights[i];

	if (total == 0)
		return POV_TEST_LIST;

	r = get_random_u32() % total;
	for (u32 i = 0; i < n; i++) {
		sum += weights[i];
		if (r < sum)
			return i + POV_TEST_UPLOAD;
	}
	return POV_TEST_LIST;
}

static bool phone_layer_valid(const char *layer)
{
	char expected[16];

	if (!layer)
		return false;
	for (u32 i = 0; i < POV_LAYER_COUNT; i++) {
		snprintf(expected, sizeof(expected), "layer%u", i);
		if (strcmp(layer, expected) == 0)
			return true;
	}
	return false;
}

static int phone_compare_files(const char *source, const char *copy)
{
	FIL source_file;
	FIL copy_file;
	FRESULT res;
	u8 source_buf[512];
	u8 copy_buf[512];
	UINT source_read;
	UINT copy_read;
	u32 offset = 0;
	int ret = 0;

	res = f_open(&source_file, source, FA_READ);
	if (res != FR_OK)
		return -ENOENT;
	res = f_open(&copy_file, copy, FA_READ);
	if (res != FR_OK) {
		f_close(&source_file);
		return -ENOENT;
	}

	while (1) {
		res = f_read(&source_file, source_buf, sizeof(source_buf),
			&source_read);
		if (res != FR_OK) {
			ret = -EIO;
			break;
		}
		res = f_read(&copy_file, copy_buf, sizeof(copy_buf), &copy_read);
		if (res != FR_OK || source_read != copy_read) {
			ret = -EIO;
			break;
		}
		if (memcmp(source_buf, copy_buf, source_read) != 0) {
			for (UINT i = 0; i < source_read; i++) {
				if (source_buf[i] != copy_buf[i]) {
					pr_err("phone: file data differs at offset %u: %02x != %02x (%s, %s)\n",
						offset + i, source_buf[i], copy_buf[i],
						source, copy);
					break;
				}
			}
			ret = -EIO;
			break;
		}
		if (source_read == 0)
			break;
		offset += source_read;
	}

	f_close(&source_file);
	f_close(&copy_file);

	return ret;
}

static int phone_expected_list_count(const char *image_path,
	const char *filter)
{
	DIR dir;
	FILINFO fno;
	FRESULT res;
	char layer[16];
	char path[64];
	int count = 0;

	for (u32 i = 0; i < POV_LAYER_COUNT; i++) {
		snprintf(layer, sizeof(layer), "layer%u", i);
		if (filter && filter[0] && strcmp(filter, layer) != 0)
			continue;

		snprintf(path, sizeof(path), "%s/%s", image_path, layer);
		res = f_opendir(&dir, path);
		if (res != FR_OK)
			continue;
		while (1) {
			res = f_readdir(&dir, &fno);
			if (res != FR_OK || fno.fname[0] == 0)
				break;
			if (!(fno.fattrib & (AM_DIR | AM_SYS)))
				count++;
		}
		f_closedir(&dir);
	}

	return count;
}

static int phone_verify_download(const char *image_path,
	const char *layer, int received_bytes)
{
	DIR dir;
	FILINFO fno;
	FRESULT res;
	char source_dir[64];
	char source[128];
	char copy[128];
	u32 expected_bytes = 0;
	u32 file_count = 0;
	int ret;

	snprintf(source_dir, sizeof(source_dir), "%s/%s",
		image_path, layer);
	res = f_opendir(&dir, source_dir);
	if (res != FR_OK)
		return -ENOENT;

	while (1) {
		res = f_readdir(&dir, &fno);
		if (res != FR_OK || fno.fname[0] == 0)
			break;
		if (fno.fattrib & (AM_DIR | AM_SYS))
			continue;

		snprintf(source, sizeof(source), "%s/%s",
			source_dir, fno.fname);
		snprintf(copy, sizeof(copy), "%s/%s",
			PHONE_DOWNLOAD_DIR, fno.fname);
		ret = phone_compare_files(source, copy);
		if (ret < 0) {
			f_closedir(&dir);
			pr_err("phone: downloaded file mismatch: %s\n", fno.fname);
			return ret;
		}
		expected_bytes += fno.fsize;
		file_count++;
	}
	f_closedir(&dir);

	if (file_count == 0 || expected_bytes != (u32)received_bytes)
		return -EIO;

	pr_info("phone: verified %u downloaded file(s), %u bytes from %s\n",
		file_count, expected_bytes, layer);
	return 0;
}

static int phone_wait_image_index(struct pov_test_context *test,
	struct pov_rotor *rotor, u8 expected, u32 timeout_ms)
{
	u32 elapsed = 0;
	u32 ring_timeout = POV_RING_ACK_TIMEOUT_MS + test->arm_count * 3000;

	if (test->arm_count > 1 && timeout_ms < ring_timeout)
		timeout_ms = ring_timeout;

	while (test->running && elapsed < timeout_ms) {
		if (rotor->image_current == expected &&
			!rotor->image_cmd.pending)
			return 0;
		phone_wait(test, 50);
		elapsed += 50;
	}

	pr_err("phone: image index timeout, expected=%u actual=%u\n",
		expected, rotor->image_current);
	return -ETIMEDOUT;
}

static int phone_wait_image_ack(struct phone_context *phone,
	u8 expected_action, u8 expected_result, u32 timeout_ms)
{
	struct pov_test_context *test = phone->test;
	mavlink_image_ack_t ack;
	u32 elapsed = 0;
	int ret;

	while (test->running && elapsed < timeout_ms) {
		if (serial_port_data_available(phone->port) > 0) {
			ret = mavlink_receive_and_process(phone->mavlink, 100);
			if (ret < 0)
				return ret;
			if (phone->mavlink->rx_msg.msgid == MAVLINK_MSG_ID_IMAGE_ACK) {
				mavlink_msg_image_ack_decode(&phone->mavlink->rx_msg, &ack);
				if (ack.action != expected_action ||
					ack.ack_result != expected_result) {
					pr_err("phone: image ack mismatch action=%u result=%u/%u\n",
						ack.action, expected_action,
						ack.ack_result, expected_result);
					return -EPROTO;
				}
				return 0;
			}
		}
		phone_wait(test, 50);
		elapsed += 50;
	}

	return -ETIMEDOUT;
}

static int phone_test_image_control(struct phone_context *phone,
	u8 action, u8 image_index, u8 expected)
{
	struct pov_test_context *test = phone->test;
	struct pov_rotor *rotor = test->rotor;
	int ret;

	ret = mavlink_send_image_control(phone->mavlink,
			action, image_index);
	if (ret < 0)
		return ret;
	ret = phone_wait_image_ack(phone, action, MAV_ACK_RESULT_OK,
			POV_ARM_COUNT > 1 ? POV_RING_ACK_TIMEOUT_MS +
			phone->test->arm_count * 3000 : 2000);
	if (ret < 0)
		return ret;

	if (action == MAV_IMAGE_ACTION_REPLAY) {
		phone_wait(test, 200);
		if (rotor->image_current != expected)
			return -EIO;
		return 0;
	}

	return phone_wait_image_index(test, rotor, expected, 5000);
}

static int phone_test_upload(struct phone_context *phone,
	const char *requested_layer)
{
	struct pov_test_context *test = phone->test;
	ktime_t completion_deadline;
	DIR dir;
	FILINFO fno;
	FRESULT res;
	char chosen[64] = { 0 };
	char source_dir[128];
	char full_path[128];
	char uploaded_path[128];
	char layer[16];
	const char *file_list[] = { full_path };
	u32 chosen_size = 0;
	int ret;

	if (requested_layer) {
		if (!phone_layer_valid(requested_layer))
			return -EINVAL;
		snprintf(layer, sizeof(layer), "%s", requested_layer);
	} else {
		snprintf(layer, sizeof(layer), "layer%u",
			get_random_u32() % POV_LAYER_COUNT);
	}

	snprintf(source_dir, sizeof(source_dir), "%s/blend", APP_IMAGE_PATH);
	res = f_opendir(&dir, source_dir);
	if (res != FR_OK)
		return -ENOENT;

	while (1) {
		char *ext;

		res = f_readdir(&dir, &fno);
		if (res != FR_OK || fno.fname[0] == 0)
			break;
		if (fno.fattrib & (AM_DIR | AM_SYS))
			continue;

		ext = strrchr(fno.fname, '.');
		if (ext && strcmp(ext, ".pov") == 0) {
			snprintf(chosen, sizeof(chosen), "%s", fno.fname);
			chosen_size = fno.fsize;
			break;
		}
	}
	f_closedir(&dir);
	if (!chosen[0])
		return -ENOENT;

	snprintf(full_path, sizeof(full_path), "%s/%s",
		source_dir, chosen);
	ret = mavlink_send_ft_start(phone->mavlink,
			MAV_FILE_TRANSFER_DIR_UPLOAD, layer, chosen_size, 3000);
	if (ret < 0)
		return ret;

	pr_info("phone: upload '%s' -> %s (%u B)\n",
		full_path, layer, chosen_size);
	ret = lrzsz_sz(phone->port, 1, file_list, NULL);
	if (ret <= 0)
		return -EIO;

	/* The sender can return after transmitting "00", slightly before the
	 * receiver has consumed it and left ZMODEM mode. Do not send another
	 * MAVLink command until the rotor confirms that upload processing is
	 * completely finished. In a ring, the master then forwards the files to
	 * each downstream arm before it reports completion, so budget one transfer
	 * interval per hop instead of using a fixed poll count. */
	completion_deadline = ktime_add_ms(ktime_get(),
		45000u + max((u32)test->arm_count - 1u, 1u) * 1000u);
	while (test->running &&
		ktime_ms_delta(completion_deadline, ktime_get()) > 0) {
		if (serial_port_data_available(phone->port) > 0) {
			ret = mavlink_receive_and_process(phone->mavlink, 100);
			if (ret < 0)
				return ret;
			if (phone->mavlink->ft.state == MAV_FILE_TRANSFER_STATE_COMPLETED)
				break;
			if (phone->mavlink->ft.state == MAV_FILE_TRANSFER_STATE_ERROR)
				return -EIO;
		}
		if (!phone_wait(test, 50))
			return -ETIMEDOUT;
	}
	if (phone->mavlink->ft.state != MAV_FILE_TRANSFER_STATE_COMPLETED) {
		pr_err("phone: upload completion timeout for %s\n", layer);
		return -ETIMEDOUT;
	}
	phone->mavlink->ft.state = MAV_FILE_TRANSFER_STATE_IDLE;

	snprintf(uploaded_path, sizeof(uploaded_path), "%s/%s/%s",
		test->rotor->image_path, layer, chosen);
	ret = phone_compare_files(full_path, uploaded_path);
	if (ret < 0) {
		pr_err("phone: uploaded file mismatch: %s\n", uploaded_path);
		return -EIO;
	}
	pr_info("phone: verified uploaded file %s (%u bytes)\n",
		chosen, chosen_size);

	return 0;
}

static int phone_wait_motor_speed(struct phone_context *phone,
	s32 expected_speed, s32 tolerance, u32 timeout_ms)
{
	struct pov_test_context *test = phone->test;
	ktime_t deadline = ktime_add_ms(ktime_get(), timeout_ms);
	int ret;
	s32 speed;

	while (test->running && ktime_ms_delta(deadline, ktime_get()) > 0) {
		ret = mavlink_query_motor_status(phone->mavlink, 0, 1000);
		if (ret == 0) {
			speed = phone->mavlink->motors[0].current_speed;
			if (abs(speed - expected_speed) <= tolerance &&
				phone->mavlink->motors[0].direction == POV_DIR_FORWARD)
				return 0;
		}
		phone_wait(test, 200);
	}

	pr_err("phone: motor speed timeout expected=%d actual=%d direction=%d\n",
		expected_speed, phone->mavlink->motors[0].current_speed,
		phone->mavlink->motors[0].direction);
	return -ETIMEDOUT;
}

static int phone_test_query_motor(struct phone_context *phone)
{
	int ret;

	ret = phone_wait_motor_speed(phone, POV_TARGET_RPM, 5, 3000);
	if (ret < 0)
		return ret;
	pr_info("phone: query motor status OK, speed=%d\n",
		phone->mavlink->motors[0].current_speed);
	return 0;
}

static int phone_test_burst(struct phone_context *phone)
{
	struct pov_test_context *test = phone->test;
	struct pov_rotor *rotor = test->rotor;
	u32 burst_count = 2 + (get_random_u32() % 3);
	u8 action;
	u8 target;
	int ret;

	if (rotor->image_count == 0)
		return -ENOENT;

	pr_info("phone: burst start (%u ops)\n", burst_count);
	for (u32 i = 0; i < burst_count && test->running; i++) {
		usleep(500000 + (get_random_u32() % 500000));
		action = get_random_u32() & 1 ? MAV_IMAGE_ACTION_NEXT : MAV_IMAGE_ACTION_PREVIOUS;
		if (action == MAV_IMAGE_ACTION_NEXT)
			target = (rotor->image_current + 1) % rotor->image_count;
		else
			target = rotor->image_current == 0 ?
				rotor->image_count - 1 : rotor->image_current - 1;
		ret = phone_test_image_control(phone, action, 0, target);
		if (ret < 0)
			return ret;

		if (!phone_wait(test, (4 + (get_random_u32() % 6)) * 1000))
			return -ETIMEDOUT;
	}

	pr_info("phone: burst end (%u ops)\n", burst_count);
	return test->running ? 0 : -ETIMEDOUT;
}

static int phone_test_download(struct phone_context *phone,
	const char *layer)
{
	int ret;

	if (!phone_layer_valid(layer))
		return -EINVAL;

	ret = mavlink_send_ft_start(phone->mavlink,
			MAV_FILE_TRANSFER_DIR_DOWNLOAD, layer, 0, 5000);
	if (ret < 0)
		return ret;

	pr_info("phone: rotor ready, receiving %s...\n", layer);
	ret = lrzsz_rz(phone->port, PHONE_DOWNLOAD_DIR);
	if (ret <= 0)
		return -EIO;

	pr_info("phone: downloaded %d bytes from %s\n", ret, layer);
	return phone_verify_download(phone->test->rotor->image_path,
			layer, ret);
}

static int phone_test_list(struct phone_context *phone,
	const char *filter)
{
	struct pov_test_context *test = phone->test;
	mavlink_file_transfer_list_t fl = { 0 };
	bool complete = false;
	int expected_count;
	int ret = -ETIMEDOUT;

	expected_count = phone_expected_list_count(phone->test->rotor->image_path, filter);

	ret = mavlink_send_ft_list(phone->mavlink, 0, 0, 0,
			filter ? filter : "");
	if (ret < 0)
		return ret;
	pr_info("phone: sent FT LIST request, filter=%s\n",
		filter ? filter : "<all>");

	for (int i = 0; i < 50 && test->running; i++) {
		if (serial_port_data_available(phone->mavlink->serial) > 0) {
			ret = mavlink_receive_and_process(phone->mavlink, 100);
			if (ret < 0)
				break;
			if (phone->mavlink->rx_msg.msgid ==MAVLINK_MSG_ID_FILE_TRANSFER_LIST) {
				mavlink_msg_file_transfer_list_decode(&phone->mavlink->rx_msg, &fl);
				if (fl.list_index == fl.file_count) {
					complete = true;
					break;
				}
			}
		}
		phone_wait(test, 50);
	}

	if (!complete)
		return ret < 0 ? ret : -ETIMEDOUT;
	if (fl.file_count != expected_count) {
		pr_err("phone: file list count mismatch, received=%u expected=%d\n",
			fl.file_count, expected_count);
		return -EIO;
	}
	pr_info("phone: received complete file list (%u entries, filter=%s)\n",
		fl.file_count, filter ? filter : "<all>");
	return 0;
}

static int phone_test_startup_ready(struct phone_context *phone)
{
	struct pov_rotor *rotor = phone->test->rotor;

	if (rotor->image_count == 0)
		return -ENOENT;
	if (rotor->state != POV_STATE_DISPLAYING &&
		rotor->state != POV_STATE_CHECK_CMD)
		return -EIO;

	return phone_test_query_motor(phone);
}

static int phone_wait_multiarms_ready(struct phone_context *phone,
	u32 timeout_ms)
{
	struct pov_test_context *test = phone->test;
	u32 elapsed = 0;

	while (test->running && elapsed < timeout_ms) {
		bool ready = true;

		for (u8 i = 0; i < test->arm_count; i++) {
			struct pov_rotor *arm = test->arms[i];

			if (!arm || arm->image_count == 0 ||
				(arm->state != POV_STATE_DISPLAYING &&
                 arm->state != POV_STATE_CHECK_CMD &&
                 arm->state != POV_STATE_CHECK_RING)) {
				ready = false;
				break;
			}
		}
		if (ready)
			return 0;
		phone_wait(test, 100);
		elapsed += 100;
	}

	pr_err("phone: not all %u arms became ready within %u ms\n",
		test->arm_count, timeout_ms);
	for (u8 i = 0; i < test->arm_count; i++)
		pr_err("phone: arm %u readiness state=%u images=%u offline=%u paused=%u\n",
			i, test->arms[i] ? test->arms[i]->state : 0xff,
			test->arms[i] ? test->arms[i]->image_count : 0,
			test->arms[i] ? test->arms[i]->forced_offline : 0,
			test->arms[i] ? test->arms[i]->paused : 0);
	return -ETIMEDOUT;
}

static int phone_test_next_wrap(struct phone_context *phone)
{
	struct pov_rotor *rotor = phone->test->rotor;
	int ret;

	if (rotor->image_count == 0)
		return -ENOENT;
	ret = phone_test_image_control(phone, MAV_IMAGE_ACTION_PLAY_INDEX,
			rotor->image_count - 1, rotor->image_count - 1);
	if (ret < 0)
		return ret;

	return phone_test_image_control(phone, MAV_IMAGE_ACTION_NEXT, 0, 0);
}

static int phone_test_previous_wrap(struct phone_context *phone)
{
	struct pov_rotor *rotor = phone->test->rotor;
	int ret;

	if (rotor->image_count == 0)
		return -ENOENT;
	ret = phone_test_image_control(phone, MAV_IMAGE_ACTION_PLAY_INDEX, 0, 0);
	if (ret < 0)
		return ret;

	return phone_test_image_control(phone, MAV_IMAGE_ACTION_PREVIOUS, 0,
			rotor->image_count - 1);
}

static int phone_test_image_sequence(struct phone_context *phone)
{
	struct pov_rotor *rotor = phone->test->rotor;
	u8 middle;
	int ret;

	if (rotor->image_count == 0)
		return -ENOENT;
	middle = rotor->image_count / 2;

	ret = phone_test_next_wrap(phone);
	if (ret < 0)
		return ret;
	ret = phone_test_previous_wrap(phone);
	if (ret < 0)
		return ret;
	ret = phone_test_image_control(phone, MAV_IMAGE_ACTION_PLAY_INDEX,
			middle, middle);
	if (ret < 0)
		return ret;
	return phone_test_image_control(phone, MAV_IMAGE_ACTION_REPLAY, 0, middle);
}

static int phone_test_all_layers(struct phone_context *phone,
	u32 scenario)
{
	char layer[16];
	int ret;

	for (u32 i = 0; i < POV_LAYER_COUNT; i++) {
		snprintf(layer, sizeof(layer), "layer%u", i);
		switch (scenario) {
		case POV_TEST_LIST_ALL_LAYERS:
			ret = phone_test_list(phone, layer);
			break;
		case POV_TEST_DOWNLOAD_ALL_LAYERS:
			ret = phone_test_download(phone, layer);
			break;
		case POV_TEST_UPLOAD_ALL_LAYERS:
			ret = phone_test_upload(phone, layer);
			break;
		default:
			return -EINVAL;
		}
		if (ret < 0)
			return ret;
	}
	return 0;
}

static int phone_test_round_trip(struct phone_context *phone,
	const char *requested_layer)
{
	const char *layer = requested_layer ? requested_layer : "layer0";
	int ret;

	if (!phone_layer_valid(layer))
		return -EINVAL;

	ret = phone_test_upload(phone, layer);
	if (ret < 0)
		return ret;
	ret = phone_test_list(phone, layer);
	if (ret < 0)
		return ret;
	return phone_test_download(phone, layer);
}

static int phone_test_mixed_workflow(struct phone_context *phone)
{
	int ret;

	ret = phone_test_query_motor(phone);
	if (ret < 0)
		return ret;
	ret = phone_test_image_sequence(phone);
	if (ret < 0)
		return ret;
	ret = phone_test_round_trip(phone, "layer0");
	if (ret < 0)
		return ret;
	return phone_test_list(phone, NULL);
}

static int phone_test_invalid_image(struct phone_context *phone)
{
	struct pov_rotor *rotor = phone->test->rotor;
	u8 initial_index = rotor->image_current;
	int ret;

	ret = mavlink_send_image_control(phone->mavlink,
			MAV_IMAGE_ACTION_PLAY_INDEX, rotor->image_count);
	if (ret < 0)
		return ret;
	ret = phone_wait_image_ack(phone, MAV_IMAGE_ACTION_PLAY_INDEX,
			MAV_ACK_RESULT_INVALID_PARAM, 2000);
	if (ret < 0)
		return ret;

	ret = mavlink_send_image_control(phone->mavlink, 0xff, 0);
	if (ret < 0)
		return ret;
	ret = phone_wait_image_ack(phone, 0xff,
			MAV_ACK_RESULT_INVALID_PARAM, 2000);
	if (ret < 0)
		return ret;

	phone_wait(phone->test, 200);
	if (rotor->image_current != initial_index ||
		rotor->app_mavlink->image_cmd.pending)
		return -EIO;

	pr_info("phone: invalid image commands rejected without state change\n");
	return 0;
}

static int phone_test_invalid_transfer(struct phone_context *phone)
{
	struct pov_rotor *rotor = phone->test->rotor;
	int ret;

	ret = mavlink_send_ft_start(phone->mavlink,
			MAV_FILE_TRANSFER_DIR_UPLOAD, "missing", 1, 2000);
	if (ret != -EACCES)
		return ret < 0 ? -EPROTO : -EIO;

	ret = mavlink_send_ft_start(phone->mavlink,
			MAV_FILE_TRANSFER_DIR_DOWNLOAD, "missing", 0, 2000);
	if (ret != -ENOENT)
		return ret < 0 ? -EPROTO : -EIO;

	ret = mavlink_send_ft_start(phone->mavlink,
			0xff, "layer0", 0, 2000);
	if (ret != -EACCES)
		return ret < 0 ? -EPROTO : -EIO;

	if (rotor->app_mavlink->ft.active ||
		rotor->app_mavlink->ft.state != MAV_FILE_TRANSFER_STATE_ERROR)
		return -EIO;

	pr_info("phone: invalid transfer requests rejected without FSM transition\n");
	return 0;
}

static int phone_test_ft_state(struct phone_context *phone)
{
	struct pov_test_context *test = phone->test;
	struct mavlink_ft_state *rotor_ft = &test->rotor->app_mavlink->ft;
	u32 elapsed;
	int ret;

	ret = mavlink_send_ft_ack(phone->mavlink, MAV_FILE_TRANSFER_DIR_UPLOAD,
			MAV_FT_RESULT_READY, 1234);
	if (ret < 0)
		return ret;
	for (elapsed = 0; test->running && elapsed < 2000; elapsed += 50) {
		if (rotor_ft->state == MAV_FILE_TRANSFER_STATE_TRANSFERRING &&
			rotor_ft->direction == MAV_FILE_TRANSFER_DIR_UPLOAD &&
			rotor_ft->total_bytes == 1234)
			break;
		phone_wait(test, 50);
	}
	if (elapsed >= 2000)
		return -ETIMEDOUT;

	ret = mavlink_send_ft_progress(phone->mavlink,
			MAV_FILE_TRANSFER_STATE_TRANSFERRING, 512, 1234, MAV_FT_ERR_NONE);
	if (ret < 0)
		return ret;
	for (elapsed = 0; test->running && elapsed < 2000; elapsed += 50) {
		if (rotor_ft->state == MAV_FILE_TRANSFER_STATE_TRANSFERRING &&
			rotor_ft->transferred == 512 &&
			rotor_ft->total_bytes == 1234)
			break;
		phone_wait(test, 50);
	}
	if (elapsed >= 2000)
		return -ETIMEDOUT;

	ret = mavlink_send_ft_cancel(phone->mavlink, MAV_FILE_TRANSFER_DIR_UPLOAD);
	if (ret < 0)
		return ret;
	for (elapsed = 0; test->running && elapsed < 2000; elapsed += 50) {
		if (serial_port_data_available(phone->port) > 0) {
			ret = mavlink_receive_and_process(phone->mavlink, 100);
			if (ret < 0)
				return ret;
			if (phone->mavlink->ft.state == MAV_FILE_TRANSFER_STATE_CANCELLED &&
				rotor_ft->state == MAV_FILE_TRANSFER_STATE_CANCELLED)
				break;
		}
		phone_wait(test, 50);
	}
	if (elapsed >= 2000)
		return -ETIMEDOUT;

	pr_info("phone: FT ACK/PROGRESS/CANCEL state transitions verified\n");
	return 0;
}

static void *phone_controlled_send_thread(void *arg)
{
	struct controlled_send_arg *send = arg;

	send->result = lrzsz_sz(send->port, 1, send->files,
			send->control);
	return NULL;
}

static int phone_test_transfer_cancel_active(struct phone_context *phone)
{
	static const char source[] = APP_IMAGE_PATH "/blend/rainbow.pov";
	static const char destination[] = POV_IMAGE_PATH "/layer0/rainbow.pov";
	struct pov_test_context *test = phone->test;
	struct lrzsz_transfer_control control = { 0 };
	struct controlled_send_arg send;
	const char *files[] = { source };
	FILINFO fno;
	FRESULT res;
	pthread_t thread;
	u32 elapsed;
	int ret;

	res = f_stat(source, &fno);
	if (res != FR_OK || fno.fsize < 4096)
		return -ENOENT;

	ret = mavlink_send_ft_start(phone->mavlink, MAV_FILE_TRANSFER_DIR_UPLOAD,
			"layer0", fno.fsize, 3000);
	if (ret < 0)
		return ret;

	send.port = phone->port;
	send.files = files;
	send.control = &control;
	send.result = -EINPROGRESS;
	ret = pthread_create(&thread, NULL, phone_controlled_send_thread,
			&send);
	if (ret)
		return ret;

	for (elapsed = 0; test->running && elapsed < 5000; elapsed++) {
		if (control.transferred > 0)
			break;
		if (!control.active && send.result != -EINPROGRESS)
			break;
		usleep(1000);
	}

	if (control.transferred == 0 || control.transferred > fno.fsize) {
		control.cancel_requested = 1;
		pthread_join(thread, NULL);
		return -EIO;
	}

	control.cancel_requested = 1;
	pthread_join(thread, NULL);
	if (send.result != -ECANCELED || !control.cancelled ||
		control.transferred == 0 || control.transferred > fno.fsize)
		return -EIO;

	for (elapsed = 0; test->running && elapsed < 5000; elapsed += 50) {
		if (serial_port_data_available(phone->port) > 0) {
			ret = mavlink_receive_and_process(phone->mavlink, 100);
			if (ret < 0)
				return ret;
		}
        if (phone->mavlink->ft.state == MAV_FILE_TRANSFER_STATE_CANCELLED &&
            test->rotor->app_mavlink->ft.state == MAV_FILE_TRANSFER_STATE_CANCELLED &&
            !test->rotor->app_mavlink->ft.active)
            break;
		phone_wait(test, 50);
	}
	if (elapsed >= 5000)
		return -ETIMEDOUT;

	if (f_stat(destination, &fno) == FR_OK)
		return -EIO;

	ret = phone_drain_link(phone);
	if (ret < 0)
		return ret;
	ret = phone_test_upload(phone, "layer0");
	if (ret < 0)
		return ret;

	pr_info("phone: active upload cancelled at %zu bytes, partial file removed, retry succeeded\n",
		control.transferred);
	return 0;
}

static int phone_send_bootloader_update_wait_ack(struct phone_context *phone,
	u8 command, u32 firmware_size, u32 firmware_crc32,
	const char *filename, u32 timeout_ms)
{
	mavlink_bootloader_ack_t ack;
	u32 old_rx = phone->mavlink->rx_count;
	u32 elapsed = 0;
	int ret;

	ret = mavlink_send_bootloader_update(phone->mavlink, command,
			firmware_size, firmware_crc32, filename, timeout_ms);
	if (ret < 0)
		return ret;
	if (phone->mavlink->rx_count > old_rx &&
		phone->mavlink->rx_msg.msgid == MAVLINK_MSG_ID_BOOTLOADER_ACK) {
		mavlink_msg_bootloader_ack_decode(&phone->mavlink->rx_msg, &ack);
		if (ack.command != command)
			return -EPROTO;
		return ack.result == MAV_BL_ACK_ACCEPTED ? 0 : -EACCES;
	}

	while (phone->test->running && elapsed < timeout_ms) {
		if (serial_port_data_available(phone->port) > 0) {
			ret = mavlink_receive_and_process(phone->mavlink, 100);
			if (ret < 0)
				return ret;
			if (phone->mavlink->rx_count > old_rx &&
				phone->mavlink->rx_msg.msgid == MAVLINK_MSG_ID_BOOTLOADER_ACK) {
				mavlink_msg_bootloader_ack_decode(
					&phone->mavlink->rx_msg, &ack);
				if (ack.command != command)
					return -EPROTO;
				return ack.result == MAV_BL_ACK_ACCEPTED ? 0 : -EACCES;
			}
		}
		phone_wait(phone->test, 50);
		elapsed += 50;
	}

	return -ETIMEDOUT;
}

static int phone_wait_bootloader_ack(struct phone_context *phone,
	u8 command, u8 expected_result, u32 old_rx, u32 timeout_ms)
{
	mavlink_bootloader_ack_t ack;
	u32 elapsed = 0;
	int ret;

	while (phone->test->running && elapsed < timeout_ms) {
		if (serial_port_data_available(phone->port) > 0) {
			ret = mavlink_receive_and_process(phone->mavlink, 100);
			if (ret < 0)
				return ret;
			if (phone->mavlink->rx_count > old_rx &&
				phone->mavlink->rx_msg.msgid == MAVLINK_MSG_ID_BOOTLOADER_ACK) {
				mavlink_msg_bootloader_ack_decode( &phone->mavlink->rx_msg, &ack);
				if (ack.command != command)
					return -EPROTO;
				return ack.result == expected_result ? 0 : -EIO;
			}
		}
		phone_wait(phone->test, 50);
		elapsed += 50;
	}

	return -ETIMEDOUT;
}

static int phone_test_bootloader_update(struct phone_context *phone)
{
	static const char firmware_name[] = BL_FIRMWARE_NAME;
	static const char computer_path[] = BL_COMPUTER_PATH "/" BL_FIRMWARE_NAME;
	static const char backup_path[] = BL_BACKUP_PATH "/" BL_FIRMWARE_NAME;
	static const char installed_path[] = BL_FIRMWARE_PATH "/" BL_FIRMWARE_NAME;
	const u32 firmware_size = 128 * 1024;
	const char *files[] = { computer_path };
	u8 *firmware = NULL;
	FIL file;
	FILINFO fno;
	FRESULT res;
	UINT written = 0;
	u32 firmware_crc32;
	u32 elapsed;
	u32 attempt;
	uintptr_t entry;
	bool opened = false;
	bool update_complete = false;
	int ret = -EIO;

	if (!general_flash_addr) {
		general_flash_addr = kzalloc(ROM_TOTAL_SIZE, GFP_KERNEL);
		if (!general_flash_addr)
			return -ENOMEM;
	}

	memset(general_flash_addr, 0xff, ROM_TOTAL_SIZE);

	firmware = kmalloc(firmware_size, GFP_KERNEL);
	if (!firmware)
		return -ENOMEM;
	for (u32 i = 0; i < firmware_size; i++)
		firmware[i] = (u8)(i * 37u + 0x5au);
	*(u32 *)&firmware[0] = 0x20002000u;

	/* The vector-table slot is only 32 bits wide while the simulated flash
	 * lives at a host (possibly 64-bit) address.  Catch the truncation
	 * instead of silently writing a bogus entry point. */
	entry = (uintptr_t)(general_flash_addr + BOOTLOADER_SIZE + 0x100);
	if (entry > 0xffffffffu) {
		pr_err("phone: simulated entry point %p does not fit in 32 bits\n",
			(void *)entry);
		goto out;
	}
	*(u32 *)&firmware[4] = (u32)entry;

	firmware_crc32 = crc32(~0, firmware, firmware_size) ^ ~0;

	if (fatfs_create_dirs(BL_COMPUTER_PATH) < 0)
		goto out;
	res = f_open(&file, computer_path, FA_CREATE_ALWAYS | FA_WRITE);
	if (res != FR_OK)
		goto out;
	opened = true;
	res = f_write(&file, firmware, firmware_size, &written);
	if (res != FR_OK || written != firmware_size)
		goto out;
	res = f_close(&file);
	opened = false;
	if (res != FR_OK)
		goto out;

	/* Application accepts the request, writes update.flag and switches the
	 * same physical link over to the simulated bootloader. */
	ret = phone_send_bootloader_update_wait_ack(phone,
			MAV_BL_CMD_PREPARE_UPDATE, firmware_size, firmware_crc32,
			firmware_name, 3000);
	if (ret < 0)
		goto out;
	phone_wait(phone->test, 500);

	/* The bootloader independently accepts the metadata before entering
	 * raw ZMODEM receive mode. A rejected/truncated transfer returns it to
	 * WAIT_CMD; exercise the bootloader's documented three-attempt retry
	 * contract instead of abandoning it in that state. */
	for (attempt = 1; attempt <= 3 && phone->test->running; attempt++) {
		ret = phone_send_bootloader_update_wait_ack(phone,
				MAV_BL_CMD_PREPARE_UPDATE, firmware_size, firmware_crc32,
				firmware_name, 3000);
		if (ret < 0) {
			pr_warn("phone: bootloader command attempt %u failed (%d)\n",
				attempt, ret);
			phone_wait(phone->test, 500);
			continue;
		}

		ret = lrzsz_sz(phone->port, 1, files, NULL);
		if (ret != firmware_size)
			pr_warn("phone: bootloader transfer attempt %u sent %d/%u bytes\n",
				attempt, ret, firmware_size);

		/* Success is committed by deleting update.flag. This is stronger
		 * than trusting the sender-side byte counter. */
		for (elapsed = 0; phone->test->running && elapsed < 5000; elapsed += 100) {
			if (f_stat(BL_UPDATE_FLAG_PATH, &fno) == FR_NO_FILE) {
				update_complete = true;
				break;
			}
			phone_wait(phone->test, 100);
		}
		if (update_complete)
			break;
		phone_wait(phone->test, 500);
	}

	/* Bootloader must verify the incoming file, program flash, verify flash
	 * again and delete update flag before the application link is restored. */
	if (!update_complete ||
		memcmp(general_flash_addr + BOOTLOADER_SIZE,
			firmware, firmware_size) != 0) {
		ret = -EIO;
		goto out;
	}

	if (phone_compare_files(computer_path, backup_path) < 0 ||
		phone_compare_files(computer_path, installed_path) < 0) {
		ret = -EIO;
		goto out;
	}

	ret = phone_drain_link(phone);
	if (ret < 0)
		goto out;
	for (u32 attempt = 0; attempt < 3; attempt++) {
		ret = mavlink_query_motor_status(phone->mavlink, 0, 1500);
		if (ret == 0)
			break;
	}
	if (ret < 0)
		goto out;

	pr_info("phone: bootloader full update verified, size=%u crc=0x%08x, application link resto\n",
		firmware_size, firmware_crc32);
	ret = 0;

out:
	if (opened)
		f_close(&file);
	f_unlink(computer_path);
	f_unlink(backup_path);
	f_unlink(installed_path);
	if (ret == 0)
		f_unlink(BL_UPDATE_FLAG_PATH);
	kfree(firmware);
	return ret;
}

static int phone_wait_all_arms_image(struct phone_context *phone,
	u8 expected, u32 timeout_ms)
{
	struct pov_test_context *test = phone->test;
	u32 elapsed = 0;
	u32 ring_timeout = POV_RING_ACK_TIMEOUT_MS + test->arm_count * 3000;

	if (test->arm_count > 1 && timeout_ms < ring_timeout)
		timeout_ms = ring_timeout;

	while (test->running && elapsed < timeout_ms) {
		bool complete = true;

		for (u8 i = 0; i < test->arm_count; i++) {
			struct pov_rotor *arm = test->arms[i];

			if (!arm || arm->image_current != expected ||
				arm->image_cmd.pending) {
				complete = false;
				break;
			}
		}
		if (complete)
			return 0;
		phone_wait(test, 50);
		elapsed += 50;
	}

	return -ETIMEDOUT;
}

static int phone_test_multitarm_image_sync(struct phone_context *phone)
{
	struct pov_test_context *test = phone->test;
	struct pov_rotor *master = test->rotor;
	u32 errors[POV_ARM_COUNT];
	u8 target;
	int ret;

	if (test->arm_count < 2)
		return -EOPNOTSUPP;
	if (master->image_count == 0)
		return -ENOENT;
	for (u8 i = 0; i < test->arm_count; i++)
		errors[i] = test->arms[i]->fsm_error_count;

	target = (master->image_current + 1) % master->image_count;
	ret = phone_test_image_control(phone, MAV_IMAGE_ACTION_NEXT, 0, target);
	if (ret < 0)
		return ret;
	ret = phone_wait_all_arms_image(phone, target, 10000);
	if (ret < 0)
		return ret;

	for (u8 i = 0; i < test->arm_count; i++) {
		if (test->arms[i]->arm_id != i ||
			test->arms[i]->fsm_error_count != errors[i])
			return -EIO;
	}

    pr_info("phone: multi-arm image synchronous across %u arms, target=%u\n",
        test->arm_count, target);
    
    return 0;
}

static int phone_test_multitarm_file_chain(struct phone_context *phone)
{
	struct pov_test_context *test = phone->test;
	static const char filename[] = "rainbow.pov";
	u8 marker[32];
	char source[128];
	char copy[128];
	FIL file;
	FRESULT res;
	UINT written = 0;
	u32 errors[POV_ARM_COUNT];
	int ret;

	if (test->arm_count < 2)
		return -EOPNOTSUPP;
	for (u8 i = 0; i < test->arm_count; i++)
		errors[i] = test->arms[i]->fsm_error_count;

	/* Make the phone's copy observably different from every arm's initial
	 * image. This proves that each hop received data instead of merely
	 * finding a pre-existing file with the same name. */
	for (u8 i = 0; i < sizeof(marker); i++)
		marker[i] = 0xa0u + i;
	snprintf(source, sizeof(source), "%s/blend/%s",
		APP_IMAGE_PATH, filename);
	res = f_open(&file, source, FA_WRITE);
	if (res != FR_OK)
		return -EIO;
	res = f_lseek(&file, 2048);
	if (res == FR_OK)
		res = f_write(&file, marker, sizeof(marker), &written);
	f_close(&file);
	if (res != FR_OK || written != sizeof(marker))
		return -EIO;

	ret = phone_test_upload(phone, "layer0");
	if (ret < 0)
		return ret;
	for (u8 i = 0; i < test->arm_count; i++) {
		snprintf(copy, sizeof(copy), "%s/layer0/%s",
			test->arms[i]->image_path, filename);
		if (phone_compare_files(source, copy) < 0 ||
			test->arms[i]->fsm_error_count != errors[i])
			return -EIO;
	}

	pr_info("phone: multi-arm file transfer completed across %u arms\n",
		test->arm_count);
	return 0;
}

static int phone_test_multitarm_link_recovery(struct phone_context *phone)
{
	struct pov_test_context *test = phone->test;
	struct pov_rotor *master = test->rotor;
	u32 errors[POV_ARM_COUNT];
	u32 injected;
	u8 payload[4];
	u8 target;
	u16 seq;
	int ret;

	if (test->arm_count < 2 || !master->ring_cw_port)
		return -EOPNOTSUPP;
	if (master->image_count == 0)
		return -ENOENT;
	for (u8 i = 0; i < test->arm_count; i++)
		errors[i] = test->arms[i]->fsm_error_count;
	target = (master->image_current + 1) % master->image_count;
	payload[0] = MAV_IMAGE_ACTION_NEXT;
	payload[1] = 0;
	payload[2] = 0;
	payload[3] = POV_RING_IMAGE_APPLY_DELAY_FRAMES;
	seq = ++master->ring_next_seq;
	master->ring_cw_mavlink->ring_ack.pending = 0;
	master->ring_cw_mavlink->ring_error.pending = 0;

	ret = communication_fault_alloc(master->ring_cw_port,
		COMMUNICATION_FAULT_DROP, 1000, 1, 0x31415926u);
	if (ret < 0)
		return ret;
	ret = mavlink_send_ring_cmd(master->ring_cw_mavlink,
		seq, MAV_RING_CMD_IMAGE_CONTROL, master->arm_id,
		POV_ARM_BROADCAST, master->arm_id, MAV_RING_DIRECTION_CW,
		payload, sizeof(payload));
	phone_wait(test, 500);
	injected = communication_fault_injected(master->ring_cw_port);
	communication_fault_free(master->ring_cw_port);
	if (ret < 0 || injected != 1 ||
		master->ring_cw_mavlink->ring_ack.pending ||
		master->ring_cw_mavlink->ring_error.pending)
		return -EIO;

	ret = phone_test_image_control(phone, MAV_IMAGE_ACTION_NEXT, 0, target);
	if (ret < 0)
		return ret;
	ret = phone_wait_all_arms_image(phone, target, 10000);
	if (ret < 0)
		return ret;

	for (u8 i = 0; i < test->arm_count; i++) {
		if (test->arms[i]->fsm_error_count != errors[i])
			return -EIO;
	}

	pr_info("phone: multi-arm ring drop recovered, dropped=%u target=%u\n",
		injected, target);
	return 0;
}

static int phone_test_multitarm_image_sequence(struct phone_context *phone)
{
	struct pov_test_context *test = phone->test;
	struct pov_rotor *master = test->rotor;
	u32 errors[POV_ARM_COUNT];
	u8 cur;
	u8 target;
	int ret;

	if (test->arm_count < 2)
		return -EOPNOTSUPP;
	if (master->image_count < 2)
		return -ENOENT;
	if (test->arm_count > POV_ARM_COUNT)
		return -EOVERFLOW;
	for (u8 i = 0; i < test->arm_count; i++)
		errors[i] = test->arms[i]->fsm_error_count;

	/* PREVIOUS must land one image *back* — the old code paired PREVIOUS with
	 * (current + 1) % count, which only happens to be right when there are
	 * exactly two images. */
	cur = master->image_current;
	target = (cur == 0) ? master->image_count - 1 : cur - 1;
	ret = phone_test_image_control(phone, MAV_IMAGE_ACTION_PREVIOUS, 0, target);
	if (ret < 0 || phone_wait_all_arms_image(phone, target, 5000) < 0)
		return ret < 0 ? ret : -ETIMEDOUT;

	/* NEXT from there returns to where we started. */
	target = cur;
	ret = phone_test_image_control(phone, MAV_IMAGE_ACTION_NEXT, 0, target);
	if (ret < 0 || phone_wait_all_arms_image(phone, target, 5000) < 0)
		return ret < 0 ? ret : -ETIMEDOUT;

	target = master->image_count - 1;
	ret = phone_test_image_control(phone, MAV_IMAGE_ACTION_PLAY_INDEX,
			target, target);
	if (ret < 0 || phone_wait_all_arms_image(phone, target, 5000) < 0)
		return ret < 0 ? ret : -ETIMEDOUT;

	ret = phone_test_image_control(phone, MAV_IMAGE_ACTION_REPLAY, 0, target);
	if (ret < 0 || phone_wait_all_arms_image(phone, target, 5000) < 0)
		return ret < 0 ? ret : -ETIMEDOUT;

	for (u8 i = 0; i < test->arm_count; i++) {
		if (test->arms[i]->fsm_error_count != errors[i])
			return -EIO;
	}

	pr_info("phone: multi-arm NEXT/PREVIOUS/PLAY_INDEX/REPLAY synchronized across %u arms\n",
		test->arm_count);
	return 0;
}

static int phone_run_multitarm_firmware_update(struct phone_context *phone,
	u8 pattern_seed)
{
	struct pov_test_context *test = phone->test;
	static const char filename[] = "ring-firmware.bin";
	static const char source[] = BL_COMPUTER_PATH "/ring-firmware.bin";
	const u32 firmware_size = 64 * 1024;
	const char *files[] = { source };
	u32 old_boot_count[POV_ARM_COUNT];
	u32 old_errors[POV_ARM_COUNT];
	u8 *firmware = NULL;
	FIL file;
	FRESULT res;
	UINT written = 0;
	u32 firmware_crc32;
	u32 old_rx;
	u8 target;
	bool opened = false;
	int ret = -EIO;

	if (test->arm_count < 2)
		return -EOPNOTSUPP;

	firmware = kmalloc(firmware_size, GFP_KERNEL);
	if (!firmware)
		return -ENOMEM;
	for (u32 i = 0; i < firmware_size; i++)
		firmware[i] = (u8)((i * 29u + pattern_seed));
	*(u32 *)&firmware[0] = 0x20002000u;
	*(u32 *)&firmware[4] = 0x08010101u;
	firmware_crc32 = crc32(~0, firmware, firmware_size) ^ ~0;

	if (fatfs_create_dirs(BL_COMPUTER_PATH) < 0)
		goto out;
	res = f_open(&file, source, FA_CREATE_ALWAYS | FA_WRITE);
	if (res != FR_OK)
		goto out;
	opened = true;
	res = f_write(&file, firmware, firmware_size, &written);
	if (res != FR_OK || written != firmware_size)
		goto out;
	res = f_close(&file);
	opened = false;
	if (res != FR_OK)
		goto out;

	for (u8 i = 0; i < test->arm_count; i++) {
		old_boot_count[i] = test->arms[i]->firmware_boot_count;
		old_errors[i] = test->arms[i]->fsm_error_count;
	}

	ret = phone_send_bootloader_update_wait_ack(phone,
			MAV_BL_CMD_PREPARE_UPDATE, firmware_size,
			firmware_crc32, filename, 3000);
	if (ret < 0)
		goto out;
	old_rx = phone->mavlink->rx_count;
	ret = lrzsz_sz(phone->port, 1, files, NULL);
	if (ret != firmware_size) {
		ret = -EIO;
		goto out;
	}
	ret = phone_wait_bootloader_ack(phone,
			MAV_BL_CMD_PREPARE_UPDATE, MAV_BL_ACK_COMPLETED,
			old_rx, 60000);
	if (ret < 0)
		goto out;

	for (u8 i = 0; i < test->arm_count; i++) {
		struct pov_rotor *arm = test->arms[i];

		if (arm->installed_firmware_size != firmware_size ||
			arm->installed_firmware_crc32 != firmware_crc32 ||
			arm->firmware_boot_count != old_boot_count[i] + 1 ||
			arm->fsm_error_count != old_errors[i] ||
			!arm->sim_flash ||
			memcmp(arm->sim_flash + BOOTLOADER_SIZE,
				firmware, firmware_size) != 0) {
			ret = -EIO;
			goto out;
		}
	}

	/* A normal command after all installations proves that every arm has
	 * returned to the ring instead of only completing its flash write. */
	if (test->rotor->image_count == 0) {
		ret = -ENOENT;
		goto out;
	}
	target = (test->rotor->image_current + 1) % test->rotor->image_count;
	ret = phone_test_image_control(phone, MAV_IMAGE_ACTION_NEXT, 0, target);
	if (ret < 0 || phone_wait_all_arms_image(phone, target, 10000) < 0) {
		ret = ret < 0 ? ret : -ETIMEDOUT;
		goto out;
	}

	pr_info("phone: multi-arm firmware update verified across %u arms, size=%u crc=0x%08x, rejo\n",
		test->arm_count, firmware_size, firmware_crc32);
	ret = 0;
out:
	if (opened)
		f_close(&file);
	f_unlink(source);
	for (u8 i = 0; i < test->arm_count; i++) {
		char path[96];

		snprintf(path, sizeof(path), "%s/%s",
			test->arms[i]->firmware_path, filename);
		f_unlink(path);
	}
	kfree(firmware);
	return ret;
}

static int phone_test_multitarm_firmware_update(struct phone_context *phone)
{
	return phone_run_multitarm_firmware_update(phone, 0x63u);
}

static int phone_test_multitarm_boot_rejoin(struct phone_context *phone)
{
	FILINFO fno;
	u32 boot_count[POV_ARM_COUNT];
	int ret;

	for (u8 i = 0; i < phone->test->arm_count; i++)
		boot_count[i] = phone->test->arms[i]->firmware_boot_count;
	ret = phone_run_multitarm_firmware_update(phone, 0x31u);
	if (ret < 0)
		return ret;
	if (f_stat(BL_UPDATE_FLAG_PATH, &fno) == FR_OK)
		return -EIO;

	ret = phone_run_multitarm_firmware_update(phone, 0xb7u);
	if (ret < 0)
		return ret;
	if (f_stat(BL_UPDATE_FLAG_PATH, &fno) == FR_OK)
		return -EIO;
	for (u8 i = 0; i < phone->test->arm_count; i++) {
		if (phone->test->arms[i]->firmware_boot_count !=
			boot_count[i] + 2)
			return -EIO;
	}

	pr_info("phone: two staged boot cycles completed and every arm rejoined\n");
	return 0;
}

static int phone_test_multitarm_firmware_cancel(struct phone_context *phone)
{
	struct lrzsz_transfer_control *control = &phone->test->rotor->zm_xfer_ctrl;
	size_t cancelled_at;
	int ret;

	control->cancel_after = 4096;
	ret = phone_run_multitarm_firmware_update(phone, 0x52u);
	control->cancel_after = 0;
	if (ret >= 0 || !control->cancelled || control->transferred < 4096)
		return -EIO;
	cancelled_at = control->transferred;

	phone_wait(phone->test, 300);
	while (serial_port_data_available(phone->test->rotor->ring_cw_mavlink->serial) > 0) {
		ret = mavlink_receive_and_process(phone->test->rotor->ring_cw_mavlink, 100);
		if (ret < 0)
			return ret;
	}

	ret = phone_drain_link(phone);
	if (ret < 0)
		return ret;
	ret = phone_run_multitarm_firmware_update(phone, 0x52u);
	if (ret < 0)
		return ret;

	pr_info("phone: inter-arm firmware transfer cancelled at %zu bytes and retry completed\n",
		cancelled_at);
	return 0;
}

static int phone_drain_ring_results(struct phone_context *phone)
{
	int ret;

	phone_wait(phone->test, 300);
	for (u8 i = 0; i < phone->test->arm_count; i++) {
		struct mavlink_device *mav = phone->test->arms[i]->ring_cw_mavlink;
		if (!mav)
			continue;
		while (serial_port_data_available(mav->serial) > 0) {
			ret = mavlink_receive_and_process(mav, 100);
			if (ret < 0)
				return ret;
		}
	}
	return 0;
}

static int phone_drain_ring_bootloader_results(struct phone_context *phone)
{
	int ret = phone_drain_ring_results(phone);

	return ret < 0 ? ret : phone_drain_link(phone);
}

static int phone_test_multitarm_firmware_fault_once(struct phone_context *phone, u8 fault, u8 pattern_seed)
{
	struct pov_test_context *test = phone->test;
	/* Target an intermediate arm when possible: this verifies that a failure
	 * after a downstream arm has committed rolls the whole transaction back. */
	struct pov_rotor *target;
	u32 boot_count[POV_ARM_COUNT];
	int ret;

	if (test->arm_count < 2 || test->arm_count > POV_ARM_COUNT)
		return -EOPNOTSUPP;
	/* Target an intermediate arm when possible: this verifies that a failure
	 * after a downstream arm has committed rolls the whole transaction back. */
	target = test->arms[test->arm_count > 2 ? test->arm_count - 2 : test->arm_count - 1];

	for (u8 i = 0; i < test->arm_count; i++)
		boot_count[i] = test->arms[i]->firmware_boot_count;
	pov_fake_firmware_fault_set(target, fault);
	ret = phone_run_multitarm_firmware_update(phone, pattern_seed);
	pov_fake_firmware_fault_set(target, POV_FAULT_NONE);

	if (target->firmware_fault_injected == 0) {
		ret = -EIO;
		return ret;
	}

	if (ret < 0) {
		for (u8 i = 0; i < test->arm_count; i++) {
			if (test->arms[i]->firmware_boot_count != boot_count[i])
				return -EIO;
		}
		ret = phone_drain_ring_bootloader_results(phone);
		if (ret < 0)
			return ret;
		ret = phone_run_multitarm_firmware_update(phone, pattern_seed);
		if (ret < 0)
			return ret;
	}

	for (u8 i = 0; i < test->arm_count; i++) {
		if (test->arms[i]->firmware_boot_count != boot_count[i] + 1)
			return -EIO;
	}

	pr_info("phone: firmware fault=%u rejected without partial commit; retry completed\n",
		fault);
	return 0;
}

static int phone_test_multitarm_firmware_storage_full(struct phone_context *phone)
{
	return phone_test_multitarm_firmware_fault_once(phone,
			POV_FAULT_STORAGE_WRITE, 0x40u);
}

static int phone_test_multitarm_firmware_read_only(struct phone_context *phone)
{
	return phone_test_multitarm_firmware_fault_once(phone,
			POV_FAULT_STORAGE_READ_ONLY, 0x41u);
}

static int phone_test_multitarm_firmware_corrupt(struct phone_context *phone)
{
	return phone_test_multitarm_firmware_fault_once(phone,
			POV_FAULT_STAGED_CORRUPT, 0x42u);
}

static int phone_test_multitarm_firmware_flash(struct phone_context *phone)
{
	static const u8 faults[] = {
		POV_FAULT_FLASH_ERASE,
		POV_FAULT_FLASH_WRITE,
		POV_FAULT_FLASH_READ,
	};
	int ret;

	for (u8 i = 0; i < ARRAY_SIZE(faults); i++) {
		ret = phone_test_multitarm_firmware_fault_once(phone,
				faults[i], 0x50u + i);
		if (ret < 0)
			return ret;
	}
	pr_info("phone: bootloader erase/write/read rejection and recovery verified\n");
	return 0;
}

static int phone_test_multitarm_firmware_reset(struct phone_context *phone)
{
	struct pov_rotor *target;
	u32 reboot_count;
	int ret;

	if (phone->test->arm_count < 2)
		return -EOPNOTSUPP;
	target = phone->test->arms[phone->test->arm_count - 1];
	reboot_count = target->fake_reboot_count;

	pov_fake_firmware_fault_set(target, POV_FAULT_RESET_AFTER_FLAG);
	ret = phone_run_multitarm_firmware_update(phone, 0x44u);
	pov_fake_firmware_fault_set(target, POV_FAULT_NONE);
	if (ret < 0 || target->firmware_fault_injected != 1 ||
		target->fake_reboot_count != reboot_count + 1)
		return -EIO;
	pr_info("phone: staged update resumed across simulated reset and ring rejoined\n");
	return 0;
}

static int phone_test_multitarm_image_cancel(struct phone_context *phone)
{
	struct lrzsz_transfer_control *control = &phone->test->rotor->zm_xfer_ctrl;
	size_t cancelled_at;
	int ret;

	control->cancel_after = 4096;
	ret = phone_test_upload(phone, "layer0");
	control->cancel_after = 0;
	if (ret >= 0 || !control->cancelled || control->transferred < 4096)
		return -EIO;
	cancelled_at = control->transferred;

	ret = phone_drain_ring_results(phone);
	if (ret < 0)
		return ret;
	ret = phone_test_upload(phone, "layer0");
	if (ret < 0)
		return ret;

	pr_info("phone: inter-arm image transfer cancelled at %zu bytes and retry completed\n",
		cancelled_at);
	return 0;
}

static int phone_test_multitarm_image_storage_once(struct phone_context *phone, u8 fault)
{
	struct pov_rotor *target;
	int ret;

	if (phone->test->arm_count < 2)
		return -EOPNOTSUPP;
	target = phone->test->arms[phone->test->arm_count - 1];

	target->storage_fault = fault;
	target->storage_fault_injected = 0;
	ret = phone_test_upload(phone, "layer1");
	target->storage_fault = POV_FAULT_NONE;
	pr_info("phone: image storage injection fault=%u result=%d injected=%u\n",
		fault, ret, target->storage_fault_injected);
	if (ret >= 0 || target->storage_fault_injected == 0)
		return -EIO;
	ret = phone_drain_ring_results(phone);
	if (ret < 0)
		return ret;
	ret = phone_test_upload(phone, "layer1");
	if (ret < 0)
		return ret;

	pr_info("phone: image storage fault=%u rejected and retry completed\n",
		fault);
	return 0;
}

static int phone_test_multitarm_image_storage(struct phone_context *phone)
{
	int ret;

	ret = phone_test_multitarm_image_storage_once(phone,
			POV_FAULT_STORAGE_WRITE);
	if (ret < 0)
		return ret;
	return phone_test_multitarm_image_storage_once(phone,
			POV_FAULT_STORAGE_READ_ONLY);
}

static int phone_test_multitarm_command_reliability(struct phone_context *phone)
{
	struct pov_test_context *test = phone->test;
	struct pov_rotor *master = test->rotor;
	u8 expected;
	u8 unchanged = 0;
	int ret;

	for (u8 link = 0; link + 1 < test->arm_count; link++) {
		unchanged = master->image_current;
		u32 injected;

		ret = communication_fault_alloc(test->arms[link]->ring_cw_port,
				COMMUNICATION_FAULT_DROP, 1000, 1, 0x43444400u + link);
		if (ret < 0)
			return ret;
		ret = mavlink_send_image_control(phone->mavlink,
				MAV_IMAGE_ACTION_NEXT, 0);
		if (ret < 0)
			return ret;
		ret = phone_wait_image_ack(phone, MAV_IMAGE_ACTION_NEXT,
				MAV_ACK_RESULT_ERROR, POV_RING_ACK_TIMEOUT_MS +
				test->arm_count * 3000);
		injected = communication_fault_injected(test->arms[link]->ring_cw_port);
		communication_fault_free(test->arms[link]->ring_cw_port);
		if (ret < 0 || injected != 1) {
			pr_err("phone: command fault link=%u ack_ret=%d injected=%u\n",
				link, ret, injected);
			return -EIO;
		}
		phone_wait(test, 200);
		for (u8 arm = 0; arm < test->arm_count; arm++) {
			if (test->arms[arm]->image_current != unchanged) {
				pr_err("phone: command fault link=%u changed arm=%u image=%u expected=%u\n",
					link, arm, test->arms[arm]->image_current,
					unchanged);
				return -EIO;
			}
		}
	}
	phone_wait(test, POV_RING_ACK_TIMEOUT_MS + 5000);
	ret = phone_drain_ring_results(phone);
	if (ret < 0)
		return ret;

	expected = (unchanged + 1) % master->image_count;
	ret = phone_test_image_control(phone, MAV_IMAGE_ACTION_NEXT, 0,
			expected);
	if (ret < 0 ||
		phone_wait_all_arms_image(phone, expected, 5000) < 0)
		return ret < 0 ? ret : -ETIMEDOUT;
	for (u8 i = 0; i < 81; i++) {
		u8 action = i & 1 ? MAV_IMAGE_ACTION_PREVIOUS : MAV_IMAGE_ACTION_NEXT;

		if (action == MAV_IMAGE_ACTION_NEXT)
			expected = (master->image_current + 1) % master->image_count;
		else
			expected = master->image_current == 0 ?
				master->image_count - 1 : master->image_current - 1;
		ret = phone_test_image_control(phone, action, 0, expected);
		if (ret < 0 ||
			phone_wait_all_arms_image(phone, expected, 5000) < 0)
			return ret < 0 ? ret : -ETIMEDOUT;
	}

	pr_info("phone: each inter-arm command link rejected atomically, retries and burst stay\n");
	return 0;
}

static void phone_fake_reboot_arm(struct pov_rotor *arm)
{
	/* These links are optional (they are only created when the ring is
	 * wired up), so they must be probed before being dereferenced. */
	if (arm->ring_cw_mavlink) {
		arm->ring_cw_mavlink->ring_ack.pending = 0;
		arm->ring_cw_mavlink->ring_error.pending = 0;
		arm->ring_cw_mavlink->ring_cmd.pending = 0;
		arm->ring_cw_mavlink->bl.pending = 0;
	}
	if (arm->ring_ccw_mavlink) {
		arm->ring_ccw_mavlink->ring_ack.pending = 0;
		arm->ring_ccw_mavlink->ring_error.pending = 0;
		arm->ring_ccw_mavlink->ring_cmd.pending = 0;
		arm->ring_ccw_mavlink->bl.pending = 0;
	}
	arm->image_cmd.pending = 0;
	arm->speed_stable_samples = 0;
	arm->image_current = 0;
	if (arm->arm_id == 0) {
		arm->last_recv_stator_heartbeat = basic_timer_get_ms();
		arm->last_send_stator_heartbeat = basic_timer_get_ms();
		if (arm->app_mavlink) {
			arm->app_mavlink->image_cmd.pending = 0;
			arm->app_mavlink->ft.active = 0;
		}
		arm->state = POV_STATE_DISPLAYING;
	} else {
		/* A firmware reboot does not stop the physical rotor. Resume the
		 * application on the already spinning ring, as the staged Bootloader
		 * path does, instead of re-running motor spin-up. */
		arm->state = POV_STATE_DISPLAYING;
	}
	arm->fake_reboot_count++;
	arm->forced_offline = 0;
}

static int phone_test_multitarm_slave_rejoin(struct phone_context *phone)
{
	struct pov_test_context *test = phone->test;
	struct pov_rotor *slave = test->arms[test->arm_count - 1];
	u32 reboot_count = slave->fake_reboot_count;
	u8 expected;
	int ret;

	slave->forced_offline = 1;
	for (u32 elapsed = 0; test->running && !slave->paused &&
		elapsed < 5000; elapsed += 50)
		phone_wait(test, 50);
	if (!slave->paused)
		return -ETIMEDOUT;
	ret = mavlink_send_image_control(phone->mavlink, MAV_IMAGE_ACTION_NEXT, 0);
	if (ret < 0)
		return ret;
	ret = phone_wait_image_ack(phone, MAV_IMAGE_ACTION_NEXT,
			MAV_ACK_RESULT_ERROR, POV_RING_ACK_TIMEOUT_MS +
			test->arm_count * 3000);
	if (ret < 0)
		return ret;

	phone_fake_reboot_arm(slave);
	if (slave->fake_reboot_count != reboot_count + 1)
		return -EIO;
	ret = phone_wait_multiarms_ready(phone, 30000);
	if (ret < 0)
		return ret;

	/* Let the upstream waiter close the timed-out sequence and discard its
	 * late ACK before sending the first post-reboot command. */
	phone_wait(test, POV_RING_ACK_TIMEOUT_MS + 5000);
	ret = phone_drain_ring_results(phone);
	if (ret < 0)
		return ret;
	expected = test->rotor->image_count > 1 ? 1 : 0;
	ret = phone_test_image_control(phone, MAV_IMAGE_ACTION_PLAY_INDEX,
			expected, expected);
	if (ret < 0 || phone_wait_all_arms_image(phone, expected, 8000) < 0)
		return ret < 0 ? ret : -ETIMEDOUT;
	pr_info("phone: slave outage, reboot, ring rejoin and image resync verified\n");
	return 0;
}

static int phone_test_multitarm_master_rejoin(struct phone_context *phone)
{
	struct pov_test_context *test = phone->test;
	struct pov_rotor *master = test->rotor;
	u32 reboot_count = master->fake_reboot_count;
	u8 expected;
	int ret;

	/* A periodic motor status may already be queued before the outage. Drain
	 * it so the deliberately dropped query cannot consume stale data as its
	 * response. */
	ret = phone_drain_link(phone);
	if (ret < 0)
		return ret;
	ret = communication_fault_alloc(phone->port,
			COMMUNICATION_FAULT_DROP, 1000, 100, 0x4d415354u);
	if (ret < 0)
		return ret;
	ret = communication_fault_alloc(master->app_port,
			COMMUNICATION_FAULT_DROP, 1000, 100, 0x52454254u);
	if (ret < 0) {
		communication_fault_free(phone->port);
		return ret;
	}
	ret = mavlink_query_motor_status(phone->mavlink, 0, 500);
	communication_fault_free(phone->port);
	communication_fault_free(master->app_port);
	if (ret != -ETIMEDOUT)
		return -EIO;
	master->app_mavlink->image_cmd.pending = 0;
	master->app_mavlink->ft.active = 0;
	master->last_recv_stator_heartbeat = basic_timer_get_ms();
	master->fake_reboot_count++;
	if (master->fake_reboot_count != reboot_count + 1)
		return -EIO;

	for (u8 attempt = 0; attempt < 20; attempt++) {
		ret = mavlink_query_motor_status(phone->mavlink, 0, 1000);
		if (ret == 0)
			break;
		phone_wait(test, 250);
	}
	if (ret < 0)
		return ret;
	expected = master->image_count > 2 ? 2 : 0;
	ret = phone_test_image_control(phone, MAV_IMAGE_ACTION_PLAY_INDEX,
			expected, expected);
	if (ret < 0 || phone_wait_all_arms_image(phone, expected, 8000) < 0)
		return ret < 0 ? ret : -ETIMEDOUT;
	pr_info("phone: master outage, motor reconnect and ring resync verified\n");
	return 0;
}

static int phone_test_four_arm_chain(struct phone_context *phone)
{
	int ret;

	if (phone->test->arm_count < 4)
		return -EOPNOTSUPP;
	ret = phone_test_multitarm_image_sequence(phone);
	if (ret < 0)
		return ret;
	ret = phone_test_multitarm_file_chain(phone);
	if (ret < 0)
		return ret;
	ret = phone_drain_ring_results(phone);
	if (ret < 0)
		return ret;
	ret = phone_test_multitarm_command_reliability(phone);
	if (ret < 0)
		return ret;
	ret = phone_run_multitarm_firmware_update(phone, 0x50u);
	if (ret < 0)
		return ret;
	pr_info("phone: four-arm command, image, file and firmware chains verified\n");
	return 0;
}

static int phone_test_multitarm_mixed_stress(struct phone_context *phone)
{
	struct pov_test_context *test = phone->test;
	struct pov_rotor *master = test->rotor;
	u32 duration_s = test->argument ? simple_strtoul(test->argument, NULL, 10) : 20;
	ktime_t deadline = ktime_add_ms(ktime_get(), (u64)duration_s * 1000);
	u32 rounds = 0;
	int ret;

	while (test->running && ktime_ms_delta(deadline, ktime_get()) > 0) {
		u8 expected;

		switch (rounds % 6) {
		case 0:
			expected = (master->image_current + 1) % master->image_count;
			ret = phone_test_image_control(phone, MAV_IMAGE_ACTION_NEXT,
					0, expected);
			if (ret >= 0)
				ret = phone_wait_all_arms_image(phone, expected, 5000);
			break;
		case 1:
			ret = phone_test_query_motor(phone);
			break;
		case 2:
			ret = phone_test_list(phone, "layer0");
			break;
		case 3:
			expected = master->image_current == 0 ?
				master->image_count - 1 : master->image_current - 1;
			ret = phone_test_image_control(phone, MAV_IMAGE_ACTION_PREVIOUS,
					0, expected);
			if (ret >= 0)
				ret = phone_wait_all_arms_image(phone, expected, 5000);
			break;
		case 4:
			ret = phone_test_image_control(phone, MAV_IMAGE_ACTION_REPLAY,
					0, master->image_current);
			break;
		default:
			ret = phone_test_upload(phone, "layer2");
			break;
		}
		if (ret < 0)
			return ret;
		rounds++;
		phone_wait(test, 100);
	}
	if (rounds < 6)
		return -ETIMEDOUT;
	pr_info("phone: multi-arm mixed stress completed, duration=%u s rounds=%u\n",
		duration_s, rounds);
	return 0;
}

static int phone_test_motor_control(struct phone_context *phone)
{
	int restore_ret;
	int ret;

	ret = mavlink_send_motor_control(phone->mavlink, 0, 700,
			MAVLINK_DIRECTION_FORWARD, 2000);
	if (ret < 0)
		return ret;
	ret = phone_wait_motor_speed(phone, 700, 20, 8000);

	restore_ret = mavlink_send_motor_control(phone->mavlink, 0,
			POV_TARGET_RPM, MAVLINK_DIRECTION_FORWARD, 2000);
	if (restore_ret < 0)
		return restore_ret;
	restore_ret = phone_wait_motor_speed(phone, POV_TARGET_RPM, 20, 8000);
	if (ret < 0)
		return ret;
	if (restore_ret < 0)
		return restore_ret;

	pr_info("phone: motor control relay 900 -> 700 -> 900 RPM verified\n");
	return 0;
}

static int phone_drain_link(struct phone_context *phone)
{
	u32 quiet_ms = 0;
	u32 total_ms = 0;
	int ret;

	while (phone->test->running && quiet_ms < 200 && total_ms < 3000) {
		if (serial_port_data_available(phone->port) > 0) {
			ret = mavlink_receive_and_process(phone->mavlink, 100);
			if (ret < 0)
				return ret;
			quiet_ms = 0;
		} else {
			phone_wait(phone->test, 50);
			quiet_ms += 50;
		}
		total_ms += 50;
	}

	return quiet_ms >= 200 ? 0 : -ETIMEDOUT;
}

static int phone_expect_link_fault(struct phone_context *phone,
	struct serial_port *fault_port,
	enum communication_fault_mode mode, u16 count, u32 seed,
	const char *direction)
{
	u32 injected;
	u32 recovery_attempt;
	int ret;

	ret = phone_drain_link(phone);
	if (ret < 0)
		return ret;
	ret = communication_fault_alloc(fault_port, mode, 1000, count, seed);
	if (ret < 0)
		return ret;
	ret = mavlink_query_motor_status(phone->mavlink, 0, 400);
	injected = communication_fault_injected(fault_port);
	communication_fault_free(fault_port);
	if (ret != -ETIMEDOUT || injected == 0)
		return -EIO;

	/* A damaged length/header can consume part of the next frame before the
	 * parser sees a new STX. Require bounded automatic resynchronization. */
	for (recovery_attempt = 1; recovery_attempt <= 3; recovery_attempt++) {
		ret = mavlink_query_motor_status(phone->mavlink, 0, 1000);
		if (ret == 0)
			break;
	}
	if (ret < 0)
		return ret;

	pr_info("phone: fake link fault recovered, direction=%s mode=%u seed=%u injected=%u attempt=%u\n",
		direction, mode, seed, injected, recovery_attempt);
	return 0;
}

static int phone_test_mavlink_fault(struct phone_context *phone,
	enum communication_fault_mode mode)
{
	struct pov_rotor *rotor = phone->test->rotor;
	u32 seed = phone->test->argument ?
		simple_strtoul(phone->test->argument, NULL, 0) : 0x13579bdu;
	int ret;

	ret = phone_expect_link_fault(phone, phone->port, mode, 1,
			seed, "phone-to-rotor");
	if (ret < 0)
		return ret;

	/* The rotor can send heartbeat traffic, so keep injection enabled for
	 * enough packets to guarantee that the query response is also damaged. */
	return phone_expect_link_fault(phone, rotor->app_port, mode, 32,
			seed ^ 0xa5a5a5a5u, "rotor-to-phone");
}

static int phone_test_mavlink_random_fault(struct phone_context *phone)
{
	u32 seed = phone->test->argument ?
		simple_strtoul(phone->test->argument, NULL, 0) : 0x2468ace1u;
	u32 before;
	u32 after;
	u32 injected;
	int ret;

	ret = phone_drain_link(phone);
	if (ret < 0)
		return ret;
	ret = communication_fault_alloc(phone->port,
			COMMUNICATION_FAULT_RANDOM, 350, 6, seed);
	if (ret < 0)
		return ret;
	for (u32 i = 0; i < 30 && phone->test->running; i++) {
		before = communication_fault_injected(phone->port);
		ret = mavlink_query_motor_status(phone->mavlink, 0, 400);
		after = communication_fault_injected(phone->port);
		if (after != before) {
			if (ret != -ETIMEDOUT) {
				communication_fault_free(phone->port);
				return -EIO;
			}
		} else if (ret == 0) {
			communication_fault_free(phone->port);
			return ret;
		}
		if (after > 6)
			break;
	}
	injected = communication_fault_injected(phone->port);
	communication_fault_free(phone->port);
	if (injected == 0)
		return -EIO;

	ret = mavlink_query_motor_status(phone->mavlink, 0, 2000);
	if (ret < 0)
		return ret;
	pr_info("phone: randomized fake link faults recovered, seed=%u injected=%u\n",
		seed, injected);
	return 0;
}

static int phone_test_storage_fault(struct phone_context *phone,
	enum disk_fault_operation operation)
{
	static const char path[] = "0:/pov_fault_probe.bin";
	FIL file;
	FRESULT res;
	u8 expected[512];
	u8 actual[512];
	UINT transferred = 0;
	u32 injected = 0;
	u32 seed = phone->test->argument ?
		simple_strtoul(phone->test->argument, NULL, 0) : 0x10203040u;
	bool opened = false;
	bool fault_active = true;
	int ret = 0;

	for (u32 i = 0; i < sizeof(expected); i++)
		expected[i] = (u8)(i ^ 0x5a);

	f_unlink(path);
	res = f_open(&file, path, FA_CREATE_ALWAYS | FA_WRITE);
	if (res != FR_OK)
		return -ENOMEM;
	opened = true;
	res = f_write(&file, expected, sizeof(expected), &transferred);
	if (res != FR_OK || transferred != sizeof(expected)) {
		ret = -EIO;
		goto out;
	}
	res = f_close(&file);
	opened = false;
	if (res != FR_OK) {
		ret = -EIO;
		goto out;
	}

	res = f_open(&file, path, operation == DISK_FAULT_READ ? FA_READ : FA_WRITE);
	if (res != FR_OK) {
		ret = -EIO;
		goto out;
	}
	opened = true;
	if (disk_fault_alloc(&fake_ram_disk_driver, operation,
		1000, 1, seed, 0) < 0) {
		ret = -ENOMEM;
		goto out;
	}
	if (operation == DISK_FAULT_READ) {
		res = f_read(&file, actual, sizeof(actual), &transferred);
	} else
		res = f_write(&file, expected, sizeof(expected), &transferred);

	injected = disk_fault_is_injected(&fake_ram_disk_driver);
	disk_fault_free(&fake_ram_disk_driver);
	fault_active = false;
	f_close(&file);
	opened = false;
	if (res == FR_OK || injected != 1) {
		ret = -EIO;
		goto out;
	}

	/* Reopen after the injected disk failure: FatFs latches an I/O failure
	 * in the current FIL object, so recovery must use a fresh object. */
	if (operation == DISK_FAULT_WRITE) {
		for (u32 i = 0; i < sizeof(expected); i++)
			expected[i] ^= 0xff;
		res = f_open(&file, path, FA_WRITE);
		if (res != FR_OK) {
			ret = -EIO;
			goto out;
		}
		opened = true;
		res = f_write(&file, expected, sizeof(expected), &transferred);
		if (res != FR_OK || transferred != sizeof(expected)) {
			ret = -EIO;
			goto out;
		}
		res = f_close(&file);
		opened = false;
		if (res != FR_OK) {
			ret = -EIO;
			goto out;
		}
	}

	res = f_open(&file, path, FA_READ);
	if (res != FR_OK) {
		ret = -EIO;
		goto out;
	}
	opened = true;
	res = f_read(&file, actual, sizeof(actual), &transferred);
	if (res != FR_OK || transferred != sizeof(actual) ||
		memcmp(actual, expected, sizeof(actual)) != 0) {
		ret = -EIO;
		goto out;
	}

out:
	/* Only free if the explicit release above was not reached — a second
	 * disk_fault_free() on an already-released driver is a redundant free. */
	if (fault_active) {
		disk_fault_free(&fake_ram_disk_driver);
		fault_active = false;
	}
	if (opened)
		f_close(&file);
	f_unlink(path);
	if (ret == 0)
		pr_info("phone: fake storage fault recovered, operation=%u seed=%u injected=%u\n",
			operation, seed, injected);
	return ret;
}

static int phone_verify_storage_write(const char *path)
{
	static const u8 expected[] = {
		0x5d, 0x4f, 0x56, 0x2d, 0x53, 0x54, 0x4f, 0x52,
		0x4d, 0x4f, 0x45, 0x2d, 0x4f, 0x4b,
	};
	u8 actual[sizeof(expected)];
	FIL file;
	FRESULT res;
	UINT transferred;
	bool opened = false;
	int ret = -EIO;

	res = f_open(&file, path, FA_CREATE_ALWAYS | FA_WRITE);
	if (res != FR_OK)
		goto out;
	opened = true;
	res = f_write(&file, expected, sizeof(expected), &transferred);
	if (res != FR_OK || transferred != sizeof(expected)) {
		ret = -EIO;
		goto out;
	}
	res = f_close(&file);
	opened = false;
	if (res != FR_OK)
		goto out;

	res = f_open(&file, path, FA_READ);
	if (res != FR_OK)
		goto out;
	opened = true;
	res = f_read(&file, actual, sizeof(actual), &transferred);
	if (res != FR_OK || transferred != sizeof(actual) ||
		memcmp(actual, expected, sizeof(actual)) != 0)
		goto out;

	ret = 0;
out:
	if (opened)
		f_close(&file);
	f_unlink(path);
	return ret;
}

static int phone_test_storage_full(struct phone_context *phone)
{
	static const char filler_path[] = "0:/pov_full_filler.bin";
	static const char probe_path[] = "0:/pov_full_probe.bin";
	FATFS *fatfs;
	FIL filler;
	FIL probe;
	FRESULT res;
	DWORD free_before;
	DWORD free_full;
	DWORD free_after;
	FSIZE_t fill_size;
	UINT written = 0;
	bool filler_open = false;
	bool probe_open = false;
	int ret = -EIO;

	f_unlink(probe_path);
	f_unlink(filler_path);
	res = f_open(&filler, filler_path, FA_CREATE_ALWAYS | FA_WRITE);
	if (res != FR_OK)
		goto out;
	filler_open = true;
	res = f_getfree("0:", &free_before, &fatfs);
	if (res != FR_OK || free_before == 0)
		goto out;

	fill_size = (FSIZE_t)free_before * fatfs->csize * FATFS_SECTOR_SIZE;
	res = f_expand(&filler, fill_size, 1);
	if (res != FR_OK)
		goto out;
	res = f_close(&filler);
	filler_open = false;
	if (res != FR_OK)
		goto out;
	res = f_getfree("0:", &free_full, &fatfs);
	if (res != FR_OK || free_full != 0)
		goto out;

	res = f_open(&probe, probe_path, FA_CREATE_ALWAYS | FA_WRITE);
	if (res == FR_OK) {
		u8 value = 0xa5;

		probe_open = true;
		res = f_write(&probe, &value, sizeof(value), &written);
		if (res != FR_OK) {
			pr_err("phone: storage-full probe write failed: %d\n", res);
			goto out;
		}
		/* FatFs signals a full volume by completing fewer bytes than asked
		 * (f_open can still create a zero-length entry when only the data
		 * area is exhausted). */
		if (written != sizeof(value)) {
			res = FR_DENIED;
		} else {
			pr_err("phone: probe write succeeded although volume was full\n");
			goto out;
		}
	} else if (res != FR_DENIED) {
		pr_err("phone: storage-full probe open failed: %d\n", res);
		goto out;
	}

	if (probe_open) {
		f_close(&probe);
		probe_open = false;
	}
	f_unlink(probe_path);
	res = f_unlink(filler_path);
	if (res != FR_OK)
		goto out;
	res = f_getfree("0:", &free_after, &fatfs);
	if (res != FR_OK || free_after == 0)
		goto out;
	ret = phone_verify_storage_write(probe_path);
	if (ret == 0)
		pr_info("phone: storage-full handling recovered, free=%u->%u clusters\n",
			free_before, free_after);
	return ret;

out:
	if (probe_open)
		f_close(&probe);
	if (filler_open)
		f_close(&filler);
	f_unlink(probe_path);
	f_unlink(filler_path);
	return ret;
}

static int phone_test_storage_read_only(struct phone_context *phone)
{
	static const char path[] = "0:/pov_read_only_probe.bin";
	FIL file;
	FRESULT res;
	u32 checks;
	int ret;

	f_unlink(path);
	if (disk_fault_alloc(&fake_ram_disk_driver, DISK_FAULT_NONE, 0, 0, 0, 1) < 0)
		return -ENOMEM;
	res = f_open(&file, path, FA_CREATE_ALWAYS | FA_WRITE);
	checks = disk_fault_get_ro_checks(&fake_ram_disk_driver);
	disk_fault_free(&fake_ram_disk_driver);
	if (res == FR_OK) {
		f_close(&file);
		f_unlink(path);
		return -EIO;
	}
	if (res != FR_WRITE_PROTECTED || checks == 0)
		return -EIO;

	ret = phone_verify_storage_write(path);
	if (ret == 0)
		pr_info("phone: read-only storage rejected writes and recovered, checks=%u\n",
			checks);
	return ret;
}

static int phone_test_link_outage(struct phone_context *phone)
{
	struct pov_rotor *rotor = phone->test->rotor;
	u32 seed = phone->test->argument ?
		simple_strtoul(phone->test->argument, NULL, 0) : 0xabcde01u;
	u32 phone_injected;
	u32 rotor_injected;
	int ret;

	ret = phone_drain_link(phone);
	if (ret < 0)
		return ret;
	ret = communication_fault_alloc(phone->port,
			COMMUNICATION_FAULT_DROP, 1000, 100, seed);
	if (ret < 0)
		return ret;
	ret = communication_fault_alloc(rotor->app_port,
			COMMUNICATION_FAULT_DROP, 1000, 100, seed ^ 0x5a5a5a5au);
	if (ret < 0) {
		communication_fault_free(phone->port);
		return ret;
	}
	for (u32 i = 0; i < 3; i++) {
		ret = mavlink_query_motor_status(phone->mavlink, 0, 400);
		if (ret != -ETIMEDOUT) {
			communication_fault_free(phone->port);
			communication_fault_free(rotor->app_port);
			return -EIO;
		}
	}
	phone_injected = communication_fault_injected(phone->port);
	communication_fault_free(phone->port);
	ret = mavlink_query_motor_status(phone->mavlink, 0, 400);
	if (ret != -ETIMEDOUT) {
		communication_fault_free(rotor->app_port);
		return -EIO;
	}
	rotor_injected = communication_fault_injected(rotor->app_port);
	communication_fault_free(rotor->app_port);
	if (phone_injected < 1 || rotor_injected == 0)
		return -EIO;

	ret = mavlink_query_motor_status(phone->mavlink, 0, 2000);
	if (ret < 0)
		return ret;

	pr_info("phone: fake link outage recovered, seed=%u dropped=%u/%u\n",
		seed, phone_injected, rotor_injected);
	return 0;
}

static int phone_run_operation(struct phone_context *phone, u32 operation)
{
	struct pov_test_context *test = phone->test;
	struct pov_rotor *rotor = test->rotor;
	u8 target;

	switch (operation) {
	case POV_TEST_UPLOAD:
		return phone_test_upload(phone, test->argument);

	case POV_TEST_NEXT:
		if (rotor->image_count == 0)
			return -ENOENT;
		target = (rotor->image_current + 1) % rotor->image_count;
		return phone_test_image_control(phone, MAV_IMAGE_ACTION_NEXT, 0, target);

	case POV_TEST_PREVIOUS:
		if (rotor->image_count == 0)
			return -ENOENT;
		target = rotor->image_current == 0 ?
			rotor->image_count - 1 : rotor->image_current - 1;
		return phone_test_image_control(phone,
				MAV_IMAGE_ACTION_PREVIOUS, 0, target);

	case POV_TEST_REPLAY:
		return phone_test_image_control(phone,
				MAV_IMAGE_ACTION_REPLAY, 0, rotor->image_current);

	case POV_TEST_PLAY_INDEX:
		if (rotor->image_count == 0)
			return -ENOENT;
		if (!test->argument || test->argument[0] == '\0')
			target = (rotor->image_current + 1) % rotor->image_count;
		else if (strcmp(test->argument, "first") == 0)
			target = 0;
		else if (strcmp(test->argument, "middle") == 0)
			target = rotor->image_count / 2;
		else if (strcmp(test->argument, "last") == 0)
			target = rotor->image_count - 1;
		else
			target = simple_strtoul(test->argument, NULL, 10);
		if (target >= rotor->image_count)
			return -EINVAL;
		return phone_test_image_control(phone,
				MAV_IMAGE_ACTION_PLAY_INDEX, target, target);

	case POV_TEST_QUERY_MOTOR:
		return phone_test_query_motor(phone);

	case POV_TEST_BURST:
		return phone_test_burst(phone);

	case POV_TEST_DOWNLOAD:
		return phone_test_download(phone,
				test->argument ? test->argument : "layer0");

	case POV_TEST_LIST:
		return phone_test_list(phone, test->argument);

	case POV_TEST_STARTUP_READY:
		return phone_test_startup_ready(phone);

	case POV_TEST_NEXT_WRAP:
		return phone_test_next_wrap(phone);

	case POV_TEST_PREVIOUS_WRAP:
		return phone_test_previous_wrap(phone);

	case POV_TEST_IMAGE_SEQUENCE:
		return phone_test_image_sequence(phone);

	case POV_TEST_LIST_ALL_LAYERS:
	case POV_TEST_DOWNLOAD_ALL_LAYERS:
	case POV_TEST_UPLOAD_ALL_LAYERS:
		return phone_test_all_layers(phone, operation);

	case POV_TEST_TRANSFER_ROUND_TRIP:
		return phone_test_round_trip(phone, test->argument);

	case POV_TEST_MIXED_WORKFLOW:
		return phone_test_mixed_workflow(phone);

	case POV_TEST_INVALID_IMAGE:
		return phone_test_invalid_image(phone);

	case POV_TEST_INVALID_TRANSFER:
		return phone_test_invalid_transfer(phone);

	case POV_TEST_FT_STATE:
		return phone_test_ft_state(phone);

	case POV_TEST_MOTOR_CONTROL:
		return phone_test_motor_control(phone);

	case POV_TEST_MAVLINK_DROP:
		return phone_test_mavlink_fault(phone, COMMUNICATION_FAULT_DROP);

	case POV_TEST_MAVLINK_CORRUPT:
		return phone_test_mavlink_fault(phone, COMMUNICATION_FAULT_CORRUPT);

	case POV_TEST_MAVLINK_RANDOM_FAULT:
		return phone_test_mavlink_random_fault(phone);

	case POV_TEST_STORAGE_READ_FAULT:
		return phone_test_storage_fault(phone, DISK_FAULT_READ);

	case POV_TEST_STORAGE_WRITE_FAULT:
		return phone_test_storage_fault(phone, DISK_FAULT_WRITE);

	case POV_TEST_LINK_OUTAGE:
		return phone_test_link_outage(phone);

	case POV_TEST_STORAGE_FULL:
		return phone_test_storage_full(phone);

	case POV_TEST_STORAGE_READ_ONLY:
		return phone_test_storage_read_only(phone);

	case POV_TEST_TRANSFER_CANCEL_ACTIVE:
		return phone_test_transfer_cancel_active(phone);

	case POV_TEST_BOOTLOADER_UPDATE:
		if (test->arm_count > 1)
			return phone_run_multitarm_firmware_update(phone, 0x32u);
		return phone_test_bootloader_update(phone);

	case POV_TEST_MULTIARM_IMAGE_SYNC:
		return phone_test_multitarm_image_sync(phone);

	case POV_TEST_MULTIARM_FILE_CHAIN:
		return phone_test_multitarm_file_chain(phone);

	case POV_TEST_MULTIARM_LINK_RECOVERY:
		return phone_test_multitarm_link_recovery(phone);

	case POV_TEST_MULTIARM_IMAGE_SEQUENCE:
		return phone_test_multitarm_image_sequence(phone);

	case POV_TEST_MULTIARM_FIRMWARE_UPDATE:
		return phone_test_multitarm_firmware_update(phone);

	case POV_TEST_MULTIARM_BOOT_REJOIN:
		return phone_test_multitarm_boot_rejoin(phone);

	case POV_TEST_MULTIARM_FIRMWARE_CANCEL:
		return phone_test_multitarm_firmware_cancel(phone);

	case POV_TEST_MULTIARM_FIRMWARE_STORAGE_FULL:
		return phone_test_multitarm_firmware_storage_full(phone);

	case POV_TEST_MULTIARM_FIRMWARE_READ_ONLY:
		return phone_test_multitarm_firmware_read_only(phone);

	case POV_TEST_MULTIARM_FIRMWARE_CORRUPT:
		return phone_test_multitarm_firmware_corrupt(phone);

	case POV_TEST_MULTIARM_FIRMWARE_FLASH:
		return phone_test_multitarm_firmware_flash(phone);

	case POV_TEST_MULTIARM_FIRMWARE_RESET:
		return phone_test_multitarm_firmware_reset(phone);

	case POV_TEST_MULTIARM_IMAGE_CANCEL:
		return phone_test_multitarm_image_cancel(phone);

	case POV_TEST_MULTIARM_IMAGE_STORAGE:
		return phone_test_multitarm_image_storage(phone);

	case POV_TEST_MULTIARM_COMMAND_RELIABILITY:
		return phone_test_multitarm_command_reliability(phone);

	case POV_TEST_MULTIARM_SLAVE_REJOIN:
		return phone_test_multitarm_slave_rejoin(phone);

	case POV_TEST_MULTIARM_MASTER_REJOIN:
		return phone_test_multitarm_master_rejoin(phone);

	case POV_TEST_FOUR_ARM_CHAIN:
		return phone_test_four_arm_chain(phone);

	case POV_TEST_MULTIARM_MIXED_STRESS:
		return phone_test_multitarm_mixed_stress(phone);

	default:
		return -EINVAL;
	}
}

static void *phone_thread_func(void *arg)
{
	struct phone_context *phone = arg;
	struct pov_test_context *test = phone->test;
	struct pov_rotor *rotor = test->rotor;
	u32 round = 0;
	u32 initial_fsm_errors = 0;
	u32 operation_fsm_errors;
	u32 operation;
	u8 rotor_ready = 0;
	static const u32 weights[] = { 15, 15, 12, 8, 8, 10, 10, 5, 7 };
	int ret = 0;

	if (!phone_wait(test, 10000))
		goto done;

	phone->port = serial_port_alloc(&usart1_serial_port_ops, "phone-app");
	if (!phone->port) {
		ret = -ENOMEM;
		goto done;
	}

	phone->mavlink = mavlink_init(phone->port, 3, 0, "phone-app");
	if (!phone->mavlink) {
		ret = -ENOMEM;
		goto cleanup;
	}

	serial_port_start_thread(phone->port, SERIAL_PORT_DF_OTHERS,
		NULL, rotor->app_port);
	serial_port_start_thread(rotor->app_port, SERIAL_PORT_DF_OTHERS,
		NULL, phone->port);

	phone_wait(test, 5000);
	while (test->running) {
		u32 old_rx = phone->mavlink->rx_count;

		mavlink_query_motor_status(phone->mavlink, 0, 500);
		if (phone->mavlink->rx_count > old_rx)
			break;
		phone_wait(test, 1000);
	}
	if (!test->running)
		goto cleanup;

	rotor_ready = 1;
	if (test->arm_count > 1) {
		ret = phone_wait_multiarms_ready(phone, 30000);
		if (ret < 0)
			goto cleanup;
	}

	initial_fsm_errors = rotor->fsm_error_count;
	if (test->scenario == POV_TEST_RANDOM) {
		test->deadline = ktime_add_ms(ktime_get(), (u64)test->duration_s * 1000);
		pr_info("phone: random stress test started, duration=%u s\n",
			test->duration_s);
	}

	while (test->running) {
		if (test->scenario == POV_TEST_RANDOM) {
			if (!phone_wait(test, (1 + (get_random_u32() % 4)) * 1000))
				break;
			operation = phone_pick_operation(weights, ARRAY_SIZE(weights));
		} else
			operation = test->scenario;

		operation_fsm_errors = rotor->fsm_error_count;
		ret = phone_run_operation(phone, operation);
		if (ret >= 0 && operation != POV_TEST_MULTIARM_MASTER_REJOIN &&
			rotor->fsm_error_count != operation_fsm_errors)
			ret = -EIO;

		if (test->scenario != POV_TEST_RANDOM) {
			round++;
			pr_info("phone: scenario=%u round=%u/%u result=%d\n",
				test->scenario, round, test->repeat, ret);
			if (ret < 0 || round >= test->repeat) {
				test->running = 0;
				break;
			}
			phone_wait(test, test->interval_ms);
		} else if (ret < 0) {
			test->running = 0;
			break;
		}
	}

cleanup:
	if (rotor_ready && ret >= 0 &&
		test->scenario != POV_TEST_MULTIARM_MASTER_REJOIN &&
		rotor->fsm_error_count != initial_fsm_errors)
		ret = -EIO;
	if (test->scenario == POV_TEST_RANDOM &&
		test->deadline && !test->running)
		pr_info("phone: random stress test finished\n");
	pr_info("phone: thread stopping\n");

	if (phone->port)
		serial_port_stop_thread(phone->port);
	if (rotor->app_port)
		serial_port_stop_thread(rotor->app_port);
	if (phone->mavlink)
		mavlink_free(phone->mavlink);
	if (phone->port)
		serial_port_free(phone->port);

done:
	test->result = ret;
	return NULL;
}

static void *arm_thread_func(void *arg)
{
	struct arm_thread_arg *thread_arg = arg;

	pov_rotor_display(thread_arg->rotor, thread_arg->running);
	return NULL;
}

static struct pov_rotor *create_arm(struct pov_rotor *last,
	pthread_t *arm_thread, struct arm_thread_arg *thread_arg,
	volatile u8 *running)
{
	struct pov_rotor *arm;
	int ret;

	arm = pov_rotor_alloc(&general_flash_ops, &general_bootloader_ops,
			&usart1_serial_port_ops, &usart2_serial_port_ops,
			&uart4_serial_port_ops, &uart5_serial_port_ops,
			&spi1_master, &spi1_master, hall_read_rotated_time);
	if (!arm)
		return NULL;

	serial_port_start_thread(arm->ring_ccw_port,
		SERIAL_PORT_DF_OTHERS, NULL, last->ring_cw_port);
	serial_port_start_thread(last->ring_cw_port,
		SERIAL_PORT_DF_OTHERS, NULL, arm->ring_ccw_port);

	thread_arg->rotor = arm;
	thread_arg->running = running;
	ret = pthread_create(arm_thread, NULL, arm_thread_func, thread_arg);
	if (ret != 0) {
		pov_rotor_free(arm);
		return NULL;
	}

	return arm;
}

int pov_test_run(int argc, char **argv)
{
	struct pov_rotor *first = NULL;
	struct pov_rotor *last;
	struct pov_rotor *prev;
	struct pov_stator *stator = NULL;
	pthread_t stator_thread;
	pthread_t phone_thread;
	pthread_t arm_threads[POV_ARM_COUNT];
	struct arm_thread_arg arm_thread_args[POV_ARM_COUNT];
	struct pov_test_context test = { 0 };
	struct phone_context phone = { 0 };
	u32 scenario = 0;
	u32 arg2 = 0;
	u32 arg3 = 0;
	char *arg_str = NULL;
	u8 arm_count = 0;
	u8 phone_started = 0;
	int ret = 0;

	if (argc > 1)
		scenario = simple_strtoul(argv[1], NULL, 10);
	if (argc > 2)
		arg2 = simple_strtoul(argv[2], NULL, 10);
	if (argc > 3)
		arg3 = simple_strtoul(argv[3], NULL, 10);
	if (argc > 4)
		arg_str = argv[4];

	if (scenario >= POV_TEST_SCENARIO_COUNT) {
		pr_err("pov: unsupported scenario=%u (expected 0..%u)\n",
			scenario, POV_TEST_SCENARIO_COUNT - 1);
		return -EINVAL;
	}

	if (scenario >= POV_TEST_FOUR_ARM_CHAIN && POV_ARM_COUNT < 4) {
		pr_warn("pov: scenario=%u requires four arms; set "
				"POV_ARM_COUNT to 4 in config.h and rebuild\n", scenario);
		return 0;
	}

	if (scenario >= POV_TEST_MULTIARM_IMAGE_SYNC && POV_ARM_COUNT < 2) {
		pr_warn("pov: scenario=%d requires multiple arms; set "
				"POV_ARM_COUNT to 3 in config.h and rebuild\n", scenario);
		return 0;
	}

	test.scenario = scenario;
	test.repeat = scenario == POV_TEST_RANDOM ? 0 : (arg2 ? arg2 : 1);
	test.duration_s = scenario == POV_TEST_RANDOM ? (arg2 ? arg2 : 60) : 0;
	test.interval_ms = arg3 ? arg3 : 200;
	test.argument = arg_str;
	test.running = 1;
	if (scenario == POV_TEST_RANDOM)
		pr_info("pov: scenario=%d stress duration=%u\n",
			test.duration_s);
	else
		pr_info("pov: scenario=%u repeat=%u interval_ms=%u argument='%s'\n",
			test.scenario, test.repeat, test.interval_ms,
			test.argument ? test.argument : "(null)");

	first = pov_rotor_alloc(&general_flash_ops, &general_bootloader_ops,
			&usart1_serial_port_ops, &usart2_serial_port_ops,
			&uart4_serial_port_ops, &uart5_serial_port_ops,
			&spi1_master, &spi1_master, hall_read_rotated_time);
	if (!first)
		return -ENOMEM;
	test.rotor = first;
	test.arms[0] = first;
	test.arm_count = 1;
	phone.test = &test;

	stator = pov_stator_alloc(&usart1_serial_port_ops,
			&general_flash_ops, &general_bootloader_ops,
			hall_read_rotated_time);
	if (!stator) {
		pov_rotor_free(first);
		return -ENOMEM;
	}
	stator->hall->priv = first->angle_sensor;

	serial_port_start_thread(first->motor_port, SERIAL_PORT_DF_OTHERS,
		NULL, stator->motor_port);
	serial_port_start_thread(stator->motor_port, SERIAL_PORT_DF_OTHERS,
		NULL, first->motor_port);

	stator_thread_switch = 1;
	ret = pthread_create(&stator_thread, NULL,
			stator_thread_func, stator);
	if (ret != 0) {
		stator_thread_switch = 0;
		goto out;
	}

	prev = first;
	for (int i = 1; i < POV_ARM_COUNT; i++) {
		last = create_arm(prev, &arm_threads[arm_count],
				&arm_thread_args[arm_count], &test.running);
		if (!last) {
			ret = -ENOMEM;
			goto out;
		}
		arm_count++;
		test.arms[test.arm_count++] = last;
		prev->angle_sensor->priv = last->angle_sensor;
		prev = last;
	}

	pov_create_fake_images(APP_IMAGE_PATH);
	fatfs_create_dirs(PHONE_DOWNLOAD_DIR);
	ret = pthread_create(&phone_thread, NULL, phone_thread_func, &phone);
	if (ret != 0) {
		goto out;
	}
	phone_started = 1;
	sleep(2);

	ret = pov_rotor_display(first, &test.running);

out:
	test.running = 0;
	if (phone_started) {
		pthread_join(phone_thread, NULL);
		if (scenario != POV_TEST_RANDOM && ret == 0)
			ret = test.result;
	}
	if (stator_thread_switch) {
		stator_thread_switch = 0;
		pthread_join(stator_thread, NULL);
	}
	for (int i = 0; i < arm_count; i++)
		pthread_join(arm_threads[i], NULL);

	for (u8 i = 0; i < test.arm_count; i++) {
		if (test.arms[i] && test.arms[i]->ring_cw_port)
			serial_port_stop_thread(test.arms[i]->ring_cw_port);
		if (test.arms[i] && test.arms[i]->ring_ccw_port)
			serial_port_stop_thread(test.arms[i]->ring_ccw_port);
	}
	for (u8 i = test.arm_count; i > 0; i--)
		pov_rotor_free(test.arms[i - 1]);
	pov_stator_free(stator);
	return ret;
}