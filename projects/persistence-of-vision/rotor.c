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
#include <linux/memblock.h>
#undef LOCK_WRITE

#include <kinetis/basic-timer.h>
#include <kinetis/fatfs-intf.h>
#include <kinetis/mavlink.h>
#include <kinetis/serial-port.h>
#include <kinetis/bootloader.h>
#include <kinetis/mt6701.h>
#include <kinetis/nrf24l01_port.h>
#include <kinetis/w25qxx_port.h>
#include <kinetis/step-debug.h>

#define rotor_info(rotor, fmt, ...) \
	pr_info("arm %d: " fmt, (rotor)->arm_id, ##__VA_ARGS__)
#define rotor_err(rotor, fmt, ...) \
	pr_err("arm %d: " fmt, (rotor)->arm_id, ##__VA_ARGS__)
#define rotor_warn(rotor, fmt, ...) \
	pr_warn("arm %d: " fmt, (rotor)->arm_id, ##__VA_ARGS__)

#include "lrzsz/zmodem.h"

#include "blend.h"
#include "rotor.h"
#include "interface.h"

int pov_spi_multi_transmit(struct pov_rotor *rotor);

#if KINETIS_FAKE_SIM
#include <unistd.h>
#include "fatfs/drivers/fake_ram_diskio.h"

struct pov_fake_firmware_snapshot {
	struct pov_rotor *rotor;
	u8 *flash;
	u32 flash_size;
	u32 installed_size;
	u32 installed_crc32;
	u32 boot_count;
};

static struct pov_fake_firmware_snapshot pov_fake_firmware_snapshots[POV_ARM_COUNT];

static int pov_fake_firmware_snapshot_take(struct pov_rotor *rotor)
{
	struct pov_fake_firmware_snapshot *snapshot = NULL;
	u8 i;

	for (i = 0; i < ARRAY_SIZE(pov_fake_firmware_snapshots); i++) {
		snapshot = &pov_fake_firmware_snapshots[i];
		if (snapshot->rotor == rotor)
			return 0;
		if (snapshot->rotor)
			continue;

		snapshot->rotor = rotor;
		snapshot->flash_size = rotor->sim_flash_size;
		snapshot->installed_size = rotor->installed_firmware_size;
		snapshot->installed_crc32 = rotor->installed_firmware_crc32;
		snapshot->boot_count = rotor->firmware_boot_count;
		if (snapshot->flash_size) {
			snapshot->flash = kmemdup(rotor->sim_flash,
						snapshot->flash_size, GFP_KERNEL);
			if (!snapshot->flash) {
				memset(snapshot, 0, sizeof(*snapshot));
				return -ENOMEM;
			}
		}
		return 0;
	}

	return -ENOSPC;
}

static void pov_fake_firmware_transaction_finish(bool rollback)
{
	u8 i;

	for (i = 0; i < ARRAY_SIZE(pov_fake_firmware_snapshots); i++) {
		struct pov_fake_firmware_snapshot *snapshot = &pov_fake_firmware_snapshots[i];
		struct pov_rotor *rotor = snapshot->rotor;

		if (!rotor)
			continue;
		if (rollback) {
			if (snapshot->flash_size == 0) {
				kfree(rotor->sim_flash);
				rotor->sim_flash = NULL;
				rotor->sim_flash_size = 0;
			} else {
				memcpy(rotor->sim_flash, snapshot->flash,
				       snapshot->flash_size);
				rotor->sim_flash_size = snapshot->flash_size;
			}
			rotor->installed_firmware_size = snapshot->installed_size;
			rotor->installed_firmware_crc32 = snapshot->installed_crc32;
			rotor->firmware_boot_count = snapshot->boot_count;
		}
		kfree(snapshot->flash);
		memset(snapshot, 0, sizeof(*snapshot));
	}
}

void pov_fake_firmware_fault_set(struct pov_rotor *rotor,
				 enum pov_fault_type fault)
{
	if (!rotor)
		return;

	rotor->firmware_fault = fault;
	if (fault != POV_FAULT_NONE)
		rotor->firmware_fault_injected = 0;
}
#endif

static int pov_set_rgb(struct pov_rotor *rotor)
{
	/* Wait for previous transfer to complete */
	int ret;

#if KINETIS_FAKE_SIM
	ret = 0;
#else
	ret = pov_spi_multi_transmit(rotor);
#endif
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

static int create_monochrome_image(const char *path, const char *name, u8 r, u8 g, u8 b)
{
	struct pov_image_header header;
	FIL fp;
	UINT bw;
	FRESULT res;
	char full_path[128];
	u8 *buffer;
	int ret = 0;

	if (fatfs_find_file(path, name) == 0) {
		return 0;
	}

	snprintf(full_path, sizeof(full_path), "%s/%s", path, name);
	res = f_open(&fp, full_path, FA_CREATE_ALWAYS | FA_WRITE);
	if (res != FR_OK) {
		pr_err("failed to create '%s': %d\n", full_path, res);
		return -EACCES;
	}

	pov_image_header_init(&header, POV_PAYLOAD_RAW_RGB, POV_BYTES_PER_COL, POV_DISPLAY_COLS);

	res = f_write(&fp, &header, sizeof(header), &bw);
	if (res != FR_OK || bw != sizeof(header)) {
		ret = -EACCES;
		goto close;
	}

	buffer = kmalloc(POV_BYTES_PER_COL, GFP_KERNEL);
	if (!buffer) {
		ret = -ENOMEM;
		goto close;
	}

	for (u32 px = 0; px < POV_LED_COUNT; px++) {
		buffer[px * 3 + 0] = r;
		buffer[px * 3 + 1] = g;
		buffer[px * 3 + 2] = b;
	}

	for (u32 col = 0; col < POV_DISPLAY_COLS; col++) {
		res = f_write(&fp, buffer, POV_BYTES_PER_COL, &bw);
		if (res != FR_OK || bw != POV_BYTES_PER_COL) {
			ret = -EACCES;
			goto free;
		}
	}

	pr_info("created fake image: %s (%d bytes)\n",
		full_path, sizeof(header) + POV_DISPLAY_COLS * POV_BYTES_PER_COL);

free:
	kfree(buffer);
close:
	f_close(&fp);

	return ret;
}

int pov_create_fake_images(const char *path)
{
	struct pov_image_header header;
	FRESULT res;
	FIL fp;
	UINT bw;
	char full_path[64], file_path[128];
	u8 *buffer = NULL;
	const char *layer_dirs[] = {
		"layer0",
		"layer1",
		"layer2",
		"layer3",
	};
	const char *fake_image_names[] = {
		"rainbow.pov",
		"stars.pov",
		"checker.pov"
	};
	int i;
	int ret = 0;

// 	ret = fatfs_delete_dir(path);
// 	if (ret < 0 && ret != -ENOENT) {
// 		pr_err("failed to clear '%s': %d\n", path, ret);
// 		return ret;
// 	}
// 	ret = 0;

	for (i = 0; i < ARRAY_SIZE(layer_dirs); i++) {
		snprintf(full_path, sizeof(full_path), "%s/%s", path, layer_dirs[i]);
		ret = fatfs_create_dirs(full_path);
		if (ret != 0) {
			pr_err("mkdir '%s' failed\n", path);
			return ret;
		}
	}

	buffer = kmalloc(POV_BYTES_PER_COL, GFP_KERNEL);
	if (!buffer) {
		return -ENOMEM;
	}

	snprintf(full_path, sizeof(full_path), "%s/%s", path, layer_dirs[0]);
	for (i = 0; i < ARRAY_SIZE(fake_image_names); i++) {
		ret = fatfs_find_file(full_path, (char *)fake_image_names[i]);
		if (ret == 0) {
			pr_info("image already exists, skipping: %s/%s\n", full_path, fake_image_names[i]);
			continue;
		}
		ret = 0;

		snprintf(file_path, sizeof(file_path), "%s/%s", full_path, fake_image_names[i]);
		res = f_open(&fp, file_path, FA_CREATE_ALWAYS | FA_WRITE);
		if (res != FR_OK) {
			pr_err("failed to create '%s': %d\n", full_path, res);
			ret = -EACCES;
			goto free;
		}

		pov_image_header_init(&header, POV_PAYLOAD_RAW_RGB, POV_BYTES_PER_COL, POV_DISPLAY_COLS);
		res = f_write(&fp, &header, sizeof(header), &bw);
		if (res != FR_OK || bw != sizeof(header)) {
			pr_err("write header failed: %d\n", res);
			ret = -EACCES;
			f_close(&fp);
			goto free;
		}

		for (int col = 0; col < POV_DISPLAY_COLS; col++) {
			get_random_bytes(buffer, POV_BYTES_PER_COL);
			res = f_write(&fp, buffer, POV_BYTES_PER_COL, &bw);
			if (res != FR_OK || bw != POV_BYTES_PER_COL) {
				pr_err("write col %d failed: %d\n", col, res);
				ret = -EACCES;
				f_close(&fp);
				goto free;
			}
		}

		f_close(&fp);
		pr_info("created fake image: %s (%d bytes)\n",
			file_path, (sizeof(header) + POV_DISPLAY_COLS * POV_BYTES_PER_COL));
	}

	pr_info("created %d fake images in %s\n", ARRAY_SIZE(fake_image_names), path);

	snprintf(full_path, sizeof(full_path), "%s/%s", path, layer_dirs[1]);
	ret = create_monochrome_image(full_path, "red.pov", 180, 30, 30);
	if (ret < 0) {
		goto free;
	}

	snprintf(full_path, sizeof(full_path), "%s/%s", path, layer_dirs[2]);
	ret = create_monochrome_image(full_path, "green.pov", 30, 180, 30);
	if (ret < 0) {
		goto free;
	}

	snprintf(full_path, sizeof(full_path), "%s/%s", path, layer_dirs[3]);
	ret = create_monochrome_image(full_path, "blue.pov", 30, 30, 180);
	if (ret < 0) {
		goto free;
	}

	ret = pov_compose_layers(path);

free:
	kfree(buffer);

	return ret;
}

static void pov_free_image_names(struct pov_rotor *rotor)
{
	for (u8 i = 0; i < rotor->image_count; i++) {
		kfree(rotor->image_names[i]);
		rotor->image_names[i] = NULL;
	}
	rotor->image_count = 0;
}

int pov_scan_images(struct pov_rotor *rotor)
{
	DIR dir;
	FILINFO fno;
	FRESULT res;
	char full_path[128];
	int ret = 0;

	/* Release any names from a previous scan before resetting the count,
	 * otherwise the old kstrdup() pointers are orphaned. */
	pov_free_image_names(rotor);

	snprintf(full_path, sizeof(full_path), "%s/%s", rotor->image_path, "blend");
	res = f_opendir(&dir, full_path);
	if (res != FR_OK) {
		rotor_err(rotor, "failed to open '%s': %d\n", full_path, res);
		return -ENOENT;
	}

	while (1) {
		/* Guard against array overflow */
		if (rotor->image_count >= POV_MAX_IMAGE_COUNT) {
			rotor_warn(rotor, "max image count (%d) reached, stopping scan\n",
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
				rotor->image_names[rotor->image_count] = kstrdup(fno.fname, GFP_KERNEL);
				if (!rotor->image_names[rotor->image_count]) {
					rotor_err(rotor, "failed to allocate name for '%s'\n", fno.fname);
					ret = -ENOMEM;
					goto out;
				}
				rotor_info(rotor, "found image: %s\n", rotor->image_names[rotor->image_count]);
				rotor->image_count++;
			}
		}
	}

out:
	f_closedir(&dir);

	/* Release already allocated names on failure */
	if (ret < 0) {
		for (u8 i = 0; i < rotor->image_count; i++) {
			kfree(rotor->image_names[i]);
			rotor->image_names[i] = NULL;
		}
		rotor->image_count = 0;
	}

	rotor_info(rotor, "found %d images in %s\n", rotor->image_count, full_path);

	return ret;
}

static int pov_validate_display_image(FIL *fp,
	const struct pov_image_header *header,
	const char *filename)
{
	FSIZE_t expected_size;
	FSIZE_t actual_size;

	if (header->magic != POV_IMG_MAGIC) {
		pr_err("invalid magic in %s: 0x%08x\n", filename, header->magic);
		return -EINVAL;
	}

	if (header->version != POV_IMG_VERSION) {
		pr_err("unsupported image version in %s: %u\n", filename, header->version);
		return -EINVAL;
	}

	if (header->columns != POV_DISPLAY_COLS ||
		header->leds_per_group != POV_LEDS_PER_GROUP ||
		header->spi_groups != POV_SPI_GROUPS) {
		pr_err("image geometry mismatch in %s (cols=%u leds/group=%u groups=%u)\n",
			filename, header->columns, header->leds_per_group,
			header->spi_groups);
		return -EINVAL;
	}

	if (header->payload_format != POV_PAYLOAD_LED_DRIVER ||
		header->bytes_per_col != POV_LED_DRIVER_COL_BYTES) {
		pr_err("image payload mismatch in %s (format=%u stride=%u)\n",
			filename, header->payload_format, header->bytes_per_col);
		return -EINVAL;
	}

	expected_size = (FSIZE_t)sizeof(*header) +
		(FSIZE_t)header->columns * header->bytes_per_col;
	actual_size = f_size(fp);
	if (actual_size != expected_size) {
		pr_err("image size mismatch in %s (got=%lu expected=%lu)\n",
			filename, (unsigned long)actual_size,
			(unsigned long)expected_size);
		return -EINVAL;
	}

	return 0;
}

int pov_load_images(struct pov_rotor *rotor, const char *filename)
{
	FRESULT res;
	UINT br;
	char file_path[512];
	int ret;

	snprintf(file_path, sizeof(file_path), "%s/%s/%s", rotor->image_path, "blend", filename);
	res = f_open(&rotor->image_file, file_path, FA_READ);
	if (res != FR_OK) {
		rotor_err(rotor, "failed to open '%s': %d\n", filename, res);
		return -EIO;
	}

	res = f_read(&rotor->image_file, &rotor->image_header, sizeof(rotor->image_header), &br);
	if (res != FR_OK || br != sizeof(rotor->image_header)) {
		rotor_err(rotor, "failed to read header: %d(got %u)\n", res, br);
		ret = -EIO;
		goto file_err;
	}
	rotor->pending_column += br;

	ret = pov_validate_display_image(&rotor->image_file, &rotor->image_header, filename);
	if (ret < 0) {
		goto file_err;
	}

	rotor->images_open = 1;
	rotor_info(rotor, "loaded '%s' (%d cols, %d groups), stride=%u\n",
		filename, rotor->image_header.columns, rotor->image_header.spi_groups, rotor->image_header.bytes_per_col);
	return 0;

file_err:
	f_close(&rotor->image_file);
	return ret;
}

void pov_close_image(struct pov_rotor *rotor)
{
	f_close(&rotor->image_file);
	rotor->images_open = 0;
	rotor->pending_column = 0;

	rotor_info(rotor, "image closed\n");
}

static const char *pov_state_name(enum pov_fsm_state s)
{
	static const char *names[] = {
		"WAIT_MOTOR", "WAIT_PHONE", "SEND_SPEED", "WAIT_SPEED", "DISPLAYING",
		"CHECK_CMD", "CHECK_RING", "STATOR_UPDATE", "ROTOR_UPDATE", "ERROR"
	};
	if ((unsigned)s >= sizeof(names) / sizeof(names[0])) {
		return "?";
	}
	return names[s];
}

static void set_fsm_state(struct pov_rotor *rotor, enum pov_fsm_state new_state)
{
	bool to_error = (new_state == POV_STATE_ERROR);

	if (to_error && rotor->state != POV_STATE_ERROR) {
		rotor->fsm_error_count++;
	}

	if (new_state != rotor->state) {
		bool quiet = (rotor->state == POV_STATE_DISPLAYING && new_state == POV_STATE_CHECK_CMD) ||
					 (rotor->state == POV_STATE_CHECK_CMD && new_state == POV_STATE_DISPLAYING) ||
					 (rotor->state == POV_STATE_DISPLAYING && new_state == POV_STATE_CHECK_RING) ||
					 (rotor->state == POV_STATE_CHECK_RING && new_state == POV_STATE_DISPLAYING);

		if (!quiet) {
			if (to_error) {
				rotor_err(rotor, "state: %s -> %s\n", pov_state_name(rotor->state), pov_state_name(new_state));
			} else {
				rotor_info(rotor, "state: %s -> %s\n", pov_state_name(rotor->state), pov_state_name(new_state));
			}
		}
	}

	rotor->state = new_state;
}

static inline bool is_master_arm(struct pov_rotor *rotor)
{
	return rotor->arm_id == 0;
}

static inline bool is_slave_arm(struct pov_rotor *rotor)
{
	return rotor->arm_id > 0;
}

static inline bool is_last_arm(struct pov_rotor *rotor)
{
	return rotor->arm_id == POV_ARM_COUNT - 1 || POV_ARM_COUNT <= 1;
}

static void pov_send_stator_heartbeat_if_due(struct pov_rotor *rotor)
{
	u64 now;

	if (!is_master_arm(rotor) || !serial_port_tx_idle(rotor->motor_port))
		return;

	now = basic_timer_get_ms();
	if (now - rotor->last_send_stator_heartbeat < (u64)POV_HEARTBEAT_INTERVAL_MS)
		return;

	rotor->last_send_stator_heartbeat = now;
	rotor->downtime_s++;
	mavlink_send_heartbeat(rotor->motor_mavlink, MAV_SYSTEM_STATUS_ACTIVE,
			       rotor->downtime_s, 0);
}

static void pov_image_cmd_set(struct pov_rotor *rotor, u8 action,
	u16 image_index, u8 delay_frames)
{
	rotor->image_cmd.pending = 1;
	rotor->image_cmd.action = action;
	rotor->image_cmd.image_index = image_index;
	rotor->image_cmd_delay_frames = delay_frames;
}

static void pov_ring_payload_image(u8 *payload, u8 action,
	u16 image_index, u8 delay_frames)
{
	payload[0] = action;
	payload[1] = image_index & 0xff;
	payload[2] = image_index >> 8;
	payload[3] = delay_frames;
}

static int pov_ring_decode_image_payload(const u8 *payload, u8 payload_len,
	u8 *action, u16 *image_index,
	u8 *delay_frames)
{
	if (payload_len < 3) {
		return -EINVAL;
	}

	*action = payload[0];
	*image_index = payload[1] | ((u16)payload[2] << 8);
	*delay_frames = payload_len >= 4 ? payload[3] : 0;

	return 0;
}

static bool pov_layer_name_valid(const char *layer)
{
	char expected[8];

	if (!layer) {
		return false;
	}

	for (int i = 0; i < POV_LAYER_COUNT; i++) {
		snprintf(expected, sizeof(expected), "layer%d", i);
		if (strcmp(layer, expected) == 0) {
			return true;
		}
	}

	return false;
}

static int pov_ring_decode_file_payload(const u8 *payload, u8 payload_len,
	char *layer, size_t layer_size)
{
	size_t copy_len;

	if (!payload || payload_len == 0 || !layer || layer_size == 0) {
		return -EINVAL;
	}

	copy_len = payload_len;
	if (copy_len >= layer_size) {
		copy_len = layer_size - 1;
	}

	memcpy(layer, payload, copy_len);
	layer[copy_len] = '\0';
	if (layer[0] == '\0' || !pov_layer_name_valid(layer)) {
		return -EINVAL;
	}

	return 0;
}

static int pov_reload_images_after_update(struct pov_rotor *rotor)
{
	int ret;

	if (rotor->images_open) {
		pov_close_image(rotor);
	}

	pov_free_image_names(rotor);

	ret = pov_compose_layers(rotor->image_path);
	if (ret < 0) {
		return ret;
	}
	ret = pov_scan_images(rotor);
	if (ret < 0) {
		return ret;
	}

	if (rotor->image_count == 0) {
		rotor_warn(rotor, "no .pov images found after update\n");
		return -ENOENT;
	}

	ret = pov_load_images(rotor, rotor->image_names[0]);
	if (ret < 0) {
		return ret;
	}

	rotor->image_current = 0;

	return 0;
}

static bool pov_ring_is_transfer_file(const FILINFO *fno)
{
	char *ext;
	if (!fno || fno->fname[0] == 0) {
		return false;
	}
	if (fno->fattrib & (AM_DIR | AM_SYS)) {
		return false;
	}
	if (fno->fsize == 0) {
		return false;
	}
	ext = strrchr(fno->fname, '.');
	return ext && strcmp(ext, ".pov") == 0;
}

static void pov_ring_free_file_list(const char **files, int count)
{
	if (!files) {
		return;
	}

	for (int i = 0; i < count; i++) {
		kfree(files[i]);
	}
	kfree(files);
}

static int pov_ring_build_layer_file_list(const char *image_path, const char *layer,
	const char ***files_out, int *count_out)
{
	DIR dir;
	FILINFO fno;
	FRESULT res;
	const char **files = NULL;
	char layer_dir[64];
	char file_path[128];
	int count = 0;
	int idx = 0;
	int ret = 0;

	if (!layer || !files_out || !count_out) {
		return -EINVAL;
	}

	*files_out = NULL;
	*count_out = 0;

	snprintf(layer_dir, sizeof(layer_dir), "%s/%s", image_path, layer);
	res = f_opendir(&dir, layer_dir);
	if (res != FR_OK) {
		pr_err("ring: cannot open layer dir %s: %d\n", layer_dir, res);
		return -ENOENT;
	}

	while (1) {
		res = f_readdir(&dir, &fno);
		if (res != FR_OK) {
			ret = -EIO;
			goto close_count;
		}
		if (fno.fname[0] == 0) {
			break;
		}
		if (pov_ring_is_transfer_file(&fno)) {
			count++;
		}
	}

close_count:
	f_closedir(&dir);
	if (ret < 0) {
		return ret;
	}
	if (count == 0) {
		pr_warn("ring: no transfer files in %s\n", layer_dir);
		return -ENOENT;
	}

	files = kcalloc(count, sizeof(*files), GFP_KERNEL);
	if (!files) {
		return -ENOMEM;
	}

	res = f_opendir(&dir, layer_dir);
	if (res != FR_OK) {
		ret = -ENOENT;
		goto free_files;
	}

	while (idx < count) {
		res = f_readdir(&dir, &fno);
		if (res != FR_OK) {
			ret = -EIO;
			break;
		}
		if (fno.fname[0] == 0) {
			break;
		}
		if (!pov_ring_is_transfer_file(&fno)) {
			continue;
		}

		snprintf(file_path, sizeof(file_path), "%s/%s", layer_dir, fno.fname);
		files[idx] = kstrdup(file_path, GFP_KERNEL);
		if (!files[idx]) {
			ret = -ENOMEM;
			break;
		}
		idx++;
	}

	f_closedir(&dir);
	if (ret < 0) {
		goto free_files;
	}
	if (idx != count) {
		ret = -EIO;
		goto free_files;
	}

	*files_out = files;
	*count_out = count;

	return 0;

free_files:
	pov_ring_free_file_list(files, idx);

	return ret;
}

static int pov_ring_send_layer_files(struct pov_rotor *rotor,
	struct mavlink_device *mav,
	const char *image_path, const char *layer)
{
	const char **files;
	FSIZE_t expected_size = 0;
	int count;
	size_t cancel_after;
	int ret;

	if (!mav) {
		return -ENODEV;
	}

	ret = pov_ring_build_layer_file_list(image_path, layer, &files, &count);
	if (ret < 0) {
		return ret;
	}
	for (int i = 0; i < count; i++) {
		FILINFO fno;
		FRESULT res;

		res = f_stat(files[i], &fno);
		if (res != FR_OK) {
			pov_ring_free_file_list(files, count);
			return -ENOENT;
		}
		expected_size += fno.fsize;
	}

	pr_info("ring: sending %d file(s) for %s via %s\n", count, layer, MD_NAME(mav));
#if KINETIS_FAKE_SIM
	cancel_after = rotor->zm_xfer_ctrl.cancel_after;
	memset(&rotor->zm_xfer_ctrl, 0, sizeof(rotor->zm_xfer_ctrl));
	rotor->zm_xfer_ctrl.cancel_after = cancel_after;
	ret = lrzsz_sz(mav->serial, count, files, &rotor->zm_xfer_ctrl);
#else
	ret = lrzsz_sz(mav->serial, count, files, NULL);
#endif

	pov_ring_free_file_list(files, count);

	if (ret < 0)
		return ret;
	return (FSIZE_t)ret == expected_size ? 0 : -EIO;
}

static int pov_ring_receive_layer_files(struct pov_rotor *rotor,
	struct mavlink_device *mav,
	const char *layer)
{
	char dest_dir[64];
	int ret;

	if (!mav) {
		return -ENODEV;
	}

	snprintf(dest_dir, sizeof(dest_dir), "%s/%s", rotor->image_path, layer);
	ret = fatfs_create_dirs(dest_dir);
	if (ret < 0) {
		return ret;
	}

	rotor_info(rotor, "ring: receiving layer files into %s via %s", dest_dir, MD_NAME(mav));
#if KINETIS_FAKE_SIM
	if (rotor->storage_fault == POV_FAULT_STORAGE_WRITE)
		disk_fault_alloc(&fake_ram_disk_driver, DISK_FAULT_WRITE,
			1000, 1, 0x494d4700u + rotor->arm_id, 0);
	else if (rotor->storage_fault == POV_FAULT_STORAGE_READ_ONLY)
		disk_fault_alloc(&fake_ram_disk_driver, DISK_FAULT_NONE,
			0, 0, 0, 1);
#endif
	ret = lrzsz_rz(mav->serial, dest_dir);
#if KINETIS_FAKE_SIM
	if (rotor->storage_fault == POV_FAULT_STORAGE_WRITE) {
		rotor->storage_fault_injected = disk_fault_is_injected(&fake_ram_disk_driver);
		disk_fault_free(&fake_ram_disk_driver);
	} else if (rotor->storage_fault == POV_FAULT_STORAGE_READ_ONLY) {
		rotor->storage_fault_injected = disk_fault_get_ro_checks(&fake_ram_disk_driver);
		disk_fault_free(&fake_ram_disk_driver);
	}
	/*
	 * ZMODEM can finish the session after skipping a file whose write was
	 * rejected. Keep that incomplete layer uncommitted and report the
	 * injected storage failure to the uostream transaction. */
	if (rotor->storage_fault != POV_FAULT_NONE &&
		rotor->storage_fault_injected)
		return -EIO;
#endif
	if (ret <= 0) {
		return -ETIMEDOUT;
	}

	return 0;
}

#if KINETIS_FAKE_SIM
static int pov_simulate_bootloader(struct pov_rotor *rotor,
	struct serial_port *app_port, u32 firmware_size, u32 firmware_crc32)
{
	struct bootloader_context *bl_ctx;
	struct bootloader_mem_layout layout;
	struct serial_port *peer;
	u32 needed_size = BOOTLOADER_SIZE + firmware_size;
	enum flash_fault_type flash_fault = FLASH_FAULT_NONE;
	int ret;

	ret = pov_fake_firmware_snapshot_take(rotor);
	if (ret < 0)
		return ret;

	if (rotor->sim_flash_size < needed_size) {
		u8 *sim_flash = kmalloc(needed_size, GFP_KERNEL);

		if (!sim_flash) {
			pov_fake_firmware_transaction_finish(false);
			return -ENOMEM;
		}
		memset(sim_flash, 0xff, needed_size);
		kfree(rotor->sim_flash);
		rotor->sim_flash = sim_flash;
		rotor->sim_flash_size = needed_size;
	}
	general_flash_addr = rotor->sim_flash;
	layout.rom_base = rotor->sim_flash;
	layout.rom_size = rotor->sim_flash_size;
	layout.firmware_max_size = firmware_size;
	layout.bootloader_size = BOOTLOADER_SIZE;
	layout.sram.start = (u8 *)0x20000000u;
	layout.sram.end   = (u8 *)0x20040000u;

	if (rotor->firmware_fault == POV_FAULT_FLASH_ERASE)
		flash_fault = FLASH_FAULT_ERASE;
	else if (rotor->firmware_fault == POV_FAULT_FLASH_WRITE)
		flash_fault = FLASH_FAULT_WRITE;
	else if (rotor->firmware_fault == POV_FAULT_FLASH_READ)
		flash_fault = FLASH_FAULT_READ;
	flash_fault_set(flash_fault);

	peer = app_port ? app_port->private : NULL;
	if (!peer) {
		flash_fault_set(FLASH_FAULT_NONE);
		pov_fake_firmware_transaction_finish(false);
		return -ENODEV;
	}

	/* A reset tears down the application UART.  Reconnect the same peer to
	 * the bootloader UART, then restore the application UART after it exits. */
	serial_port_stop_thread(app_port);
	serial_port_stop_thread(peer);
	bl_ctx = bootloader_init(&layout, rotor->bl_ops, rotor->flash,
			&usart1_serial_port_ops);
	if (!bl_ctx) {
		ret = -ENOMEM;
		goto restart_app;
	}
	serial_port_start_thread(bl_ctx->app_port, SERIAL_PORT_DF_OTHERS,
		NULL, peer);
	serial_port_start_thread(peer, SERIAL_PORT_DF_OTHERS,
		NULL, bl_ctx->app_port);
	/* Fault scenarios need their synchronous simulated reset to return after
	 * the first failed attempt so the application can report an ACK error. */
	if (rotor->firmware_fault != POV_FAULT_NONE)
		bl_ctx->retry_count = 2;
	ret = bootloader_flow(bl_ctx);
	serial_port_stop_thread(bl_ctx->app_port);
	bootloader_free(bl_ctx);

restart_app:
	serial_port_stop_thread(peer);
	serial_port_start_thread(app_port, SERIAL_PORT_DF_OTHERS, NULL, peer);
	serial_port_start_thread(peer, SERIAL_PORT_DF_OTHERS, NULL, app_port);
	if (flash_fault != FLASH_FAULT_NONE) {
		rotor->firmware_fault_injected = flash_fault_get_injected();
		flash_fault_set(FLASH_FAULT_NONE);
	}
	if (ret < 0)
		return ret;

	rotor->installed_firmware_size = firmware_size;
	rotor->installed_firmware_crc32 = firmware_crc32;
	rotor->firmware_boot_count++;
	rotor_info(rotor,
		"firmware installed and application rejoined, size=%u crc=0x%08x boot=%u\n",
		firmware_size, firmware_crc32, rotor->firmware_boot_count);
	return 0;
}
#endif

static int pov_update_firmware(struct pov_rotor *rotor,
	struct serial_port *app_port, const char *path, u32 firmware_size,
	u32 firmware_crc32)
{
	const char *filename = strrchr(path, '/');
	int ret;

	filename = filename ? filename + 1 : path;

	/* The application only persists a complete staged image and requests a
	 * reset.  Flash programming belongs exclusively to the bootloader. */
	ret = bootloader_write_update_flag(firmware_size, firmware_crc32,
		filename, path);
	if (ret < 0)
		return ret;
#if KINETIS_FAKE_SIM
	if (rotor->firmware_fault == POV_FAULT_RESET_AFTER_FLAG) {
		rotor->firmware_fault_injected++;
		rotor->fake_reboot_count++;
		rotor_info(rotor,
			"simulation: reset after staged flag, bootloader resuming update\n");
	}
	return pov_simulate_bootloader(rotor, app_port, firmware_size,
		firmware_crc32);
#else
	if (!rotor->bl_ops || !rotor->bl_ops->system_reset)
		return -ENOSYS;
	rotor->bl_ops->system_reset();
	return -EIO;
#endif
}

static int pov_ring_receive_firmware(struct pov_rotor *rotor,
	struct mavlink_device *mav, const char *filename,
	u32 firmware_size, char *path, size_t path_size)
{
	int ret;

	if (!mav || !filename || !path || path_size == 0)
		return -EINVAL;

	ret = fatfs_create_dirs(rotor->firmware_path);
	if (ret < 0)
		return ret;
#if KINETIS_FAKE_SIM
	if (rotor->firmware_fault == POV_FAULT_STORAGE_WRITE)
		disk_fault_alloc(&fake_ram_disk_driver, DISK_FAULT_WRITE,
			1000, 1, 0x4053544fu + rotor->arm_id, 0);
	else if (rotor->firmware_fault == POV_FAULT_STORAGE_READ_ONLY)
		disk_fault_alloc(&fake_ram_disk_driver, DISK_FAULT_NONE,
			0, 0, 0, 1);
#endif
	ret = lrzsz_rz(mav->serial, rotor->firmware_path);
#if KINETIS_FAKE_SIM
	if (rotor->firmware_fault == POV_FAULT_STORAGE_WRITE) {
		rotor->firmware_fault_injected = disk_fault_is_injected(&fake_ram_disk_driver);
		disk_fault_free(&fake_ram_disk_driver);
	} else if (rotor->firmware_fault == POV_FAULT_STORAGE_READ_ONLY) {
		rotor->firmware_fault_injected = disk_fault_get_ro_checks(&fake_ram_disk_driver);
		disk_fault_free(&fake_ram_disk_driver);
	}
	if ((rotor->firmware_fault == POV_FAULT_STORAGE_WRITE ||
	     rotor->firmware_fault == POV_FAULT_STORAGE_READ_ONLY) &&
	    rotor->firmware_fault_injected)
		return -EIO;
#endif
	if (ret <= 0)
		return ret == -ECANCELED ? ret : -ETIMEDOUT;
	if (ret != firmware_size)
		return -EPIPE;

	/* Reject a truncated path: it would name a different (or non-existent)
	 * file and we would then install the wrong firmware. */
	if (snprintf(path, path_size, "%s/%s", rotor->firmware_path, filename) >= (int)path_size) {
		rotor_err(rotor, "firmware path too long for '%s'\n", filename);
		return -ENAMETOOLONG;
	}

#if KINETIS_FAKE_SIM
	if (rotor->firmware_fault == POV_FAULT_STAGED_CORRUPT) {
		FIL file;
		UINT written;
		u8 byte = 0x5a;

		if (f_open(&file, path, FA_WRITE) != FR_OK)
			return -EIO;
		ret = f_write(&file, &byte, 1, &written);
		f_close(&file);
		if (ret != FR_OK || written != 1)
			return -EIO;
		rotor->firmware_fault_injected++;
	}
#endif
	return 0;
}

static int pov_ring_send_firmware(struct pov_rotor *rotor,
	struct mavlink_device *mav,
	const char *path, u32 firmware_size)
{
	const char *files[] = { path };
	size_t cancel_after;
	int ret;

	if (!mav || !path)
		return -EINVAL;
#if KINETIS_FAKE_SIM
	cancel_after = rotor->zm_xfer_ctrl.cancel_after;
	memset(&rotor->zm_xfer_ctrl, 0, sizeof(rotor->zm_xfer_ctrl));
	rotor->zm_xfer_ctrl.cancel_after = cancel_after;
	ret = lrzsz_sz(mav->serial, 1, files, &rotor->zm_xfer_ctrl);
#else
	ret = lrzsz_sz(mav->serial, 1, files, NULL);
#endif
	return ret == firmware_size ? 0 : -EIO;
}

static struct mavlink_device *pov_ring_reverse_mavlink(struct pov_rotor *rotor)
{
	return rotor->ring_ccw_mavlink;
}

static struct mavlink_device *pov_ring_forward_mavlink(struct pov_rotor *rotor)
{
	return rotor->ring_cw_mavlink;
}

static void pov_ring_clear_result_state(struct mavlink_device *mav)
{
	if (!mav) {
		return;
	}

	mav->ring_ack.pending = 0;
	mav->ring_error.pending = 0;
}

static int pov_ring_forward_error(struct pov_rotor *rotor, u16 seq,
	u8 error_node, u8 error_link_from,
	u8 error_link_to, u16 error_code)
{
	struct mavlink_device *reverse = pov_ring_reverse_mavlink(rotor);

	if (!reverse) {
		return -ENODEV;
	}

	return mavlink_send_ring_error(reverse, seq, error_node,
			error_link_from, error_link_to,
			error_code);
}

static int pov_ring_send_error_on_link(struct mavlink_device *mav, u16 seq,
	u8 error_node, u8 error_link_from,
	u8 error_link_to, u16 error_code)
{
	if (!mav) {
		return -ENODEV;
	}

	return mavlink_send_ring_error(mav, seq, error_node,
			error_link_from, error_link_to,
			error_code);
}

static u16 pov_ring_error_from_ret(int ret)
{
	switch (ret) {
	case -EINVAL:
		return MAV_RING_ERR_INVALID_PARAM;
	case -EOPNOTSUPP:
		return MAV_RING_ERR_UNSUPPORTED;
	case -ENODEV:
		return MAV_RING_ERR_NEXT_LINK_TIMEOUT;
	case -ETIMEDOUT:
		return MAV_RING_ERR_ZMODEM_TIMEOUT;
	case -EBADMSG:
	case -EPIPE:
		return MAV_RING_ERR_ZMODEM_CRC;
	default:
		return MAV_RING_ERR_FILE_IO;
	}
}

static int pov_ring_wait_link_ack(struct mavlink_device *mav, u16 seq,
	u8 expected_result, u8 expected_ack_node,
	u32 timeout_ms,
	struct mavlink_ring_ack_state *ack_out,
	struct mavlink_ring_error_state *err_out)
{
	u64 deadline = basic_timer_get_ms() + timeout_ms;

	if (!mav) {
		return -ENODEV;
	}

	if (ack_out) {
		memset(ack_out, 0, sizeof(*ack_out));
	}
	if (err_out) {
		memset(err_out, 0, sizeof(*err_out));
	}

	while (basic_timer_get_ms() < deadline) {
		if (serial_port_data_available(mav->serial) > 0) {
			mavlink_receive_and_process(mav, 10);
		}

		if (mav->ring_error.pending && mav->ring_error.seq == seq) {
			if (err_out) {
				*err_out = mav->ring_error;
			}
			pr_warn("ring: task %u failed at node %u code=%u link=%u->%u\n",
				seq, mav->ring_error.error_node, mav->ring_error.error_code,
				mav->ring_error.error_link_from, mav->ring_error.error_link_to);
			mav->ring_error.pending = 0;
			return -EIO;
		}

		if (mav->ring_ack.pending && mav->ring_ack.seq == seq) {
			struct mavlink_ring_ack_state ack = mav->ring_ack;

			mav->ring_ack.pending = 0;
			if (ack_out) {
				*ack_out = ack;
			}
			if (ack.result == expected_result) {
				if (expected_ack_node == POV_ARM_BROADCAST ||
					ack.node == expected_ack_node) {
					return 0;
				}

				pr_debug("ring: task %u ignoring ack from node %u, expected %u\n",
					seq, ack.node, expected_ack_node);
				continue;
			}
			if (ack.result == MAV_RING_RESULT_ERROR) {
				return -EIO;
			}
			pr_debug("ring: task %u ignoring ack result %u, from node %u\n",
				seq, ack.result, ack.node);
		}

		mdelay(5);
	}

	pr_err("ring: task %u timed out waiting for result %u on %s\n",
		seq, expected_result, MD_NAME(mav));
	return -ETIMEDOUT;
}

static int pov_ring_execute_cmd(struct pov_rotor *rotor,
	const struct mavlink_ring_cmd_state *cmd)
{
	u8 action;
	u8 delay_frames;
	u16 image_index;
	int ret;

	if (cmd->dst_arm != POV_ARM_BROADCAST && cmd->dst_arm != rotor->arm_id) {
		return 0;
	}

	switch (cmd->cmd) {
	case MAV_RING_CMD_IMAGE_CONTROL:
		ret = pov_ring_decode_image_payload(cmd->payload, cmd->payload_len,
				&action, &image_index, &delay_frames);
		if (ret < 0) {
			return ret;
		}
		pov_image_cmd_set(rotor, action, image_index, delay_frames);
		return 0;
	case MAV_RING_CMD_FILE_DISTRIBUTE:
		return -EOPNOTSUPP;
	default:
		return -EOPNOTSUPP;
	}
}

static int pov_ring_forward_cmd(struct pov_rotor *rotor,
	const struct mavlink_ring_cmd_state *cmd)
{
	struct mavlink_device *forward = pov_ring_forward_mavlink(rotor);

	if (!forward) {
		return -ENODEV;
	}

	pov_ring_clear_result_state(forward);

	return mavlink_send_ring_cmd(forward, cmd->seq, cmd->cmd,
			cmd->src_arm, cmd->dst_arm, rotor->arm_id,
			MAV_RING_DIRECTION_CW, cmd->payload,
			cmd->payload_len);
}

static int pov_ring_handle_file_cmd(struct pov_rotor *rotor,
	struct mavlink_device *incoming,
	const struct mavlink_ring_cmd_state *cmd)
{
	struct mavlink_device *forward;
	struct mavlink_ring_ack_state ack;
	struct mavlink_ring_error_state err;
	char layer[MAVLINK_RING_PAYLOAD_LEN];
	u8 next_arm = (rotor->arm_id + 1) % POV_ARM_COUNT;
	int ret;

	ret = pov_ring_decode_file_payload(cmd->payload, cmd->payload_len, layer, sizeof(layer));
	if (ret < 0) {
		pov_ring_send_error_on_link(incoming, cmd->seq, rotor->arm_id,
			cmd->current_arm, rotor->arm_id,
			MAV_RING_ERR_INVALID_PARAM);
		return ret;
	}
	ret = mavlink_send_ring_ack(incoming, cmd->seq, rotor->arm_id, MAV_RING_RESULT_READY);
	if (ret < 0) {
		return ret;
	}

	ret = pov_ring_receive_layer_files(rotor, incoming, layer);
	if (ret < 0) {
		pov_ring_send_error_on_link(incoming, cmd->seq, rotor->arm_id,
			cmd->current_arm, rotor->arm_id,
			pov_ring_error_from_ret(ret));
		return ret;
	}

	if (is_last_arm(rotor)) {
		ret = pov_reload_images_after_update(rotor);
		if (ret < 0) {
			pov_ring_send_error_on_link(incoming, cmd->seq, rotor->arm_id,
				cmd->current_arm, rotor->arm_id,
				pov_ring_error_from_ret(ret));
			return ret;
		}
		return mavlink_send_ring_ack(incoming, cmd->seq, rotor->arm_id, MAV_RING_RESULT_OK);
	}

	forward = pov_ring_forward_mavlink(rotor);
	if (!forward) {
		pov_ring_send_error_on_link(incoming, cmd->seq, rotor->arm_id,
			rotor->arm_id, next_arm,
			MAV_RING_ERR_NEXT_LINK_TIMEOUT);
		return -ENODEV;
	}

	ret = pov_ring_forward_cmd(rotor, cmd);
	if (ret < 0) {
		pov_ring_send_error_on_link(incoming, cmd->seq, rotor->arm_id,
			rotor->arm_id, next_arm,
			MAV_RING_ERR_NEXT_LINK_TIMEOUT);
		return ret;
	}

	ret = pov_ring_wait_link_ack(forward, cmd->seq, MAV_RING_RESULT_READY, next_arm, POV_RING_LINK_TIMEOUT_MS, &ack, &err);
	if (ret < 0) {
		if (err.pending) {
			pov_ring_send_error_on_link(incoming, err.seq,
				err.error_node,
				err.error_link_from,
				err.error_link_to,
				err.error_code);
		} else {
			pov_ring_send_error_on_link(incoming, cmd->seq,
				rotor->arm_id,
				rotor->arm_id, next_arm,
				pov_ring_error_from_ret(ret));
		}
		return ret;
	}

	ret = pov_ring_send_layer_files(rotor, forward, rotor->image_path, layer);
	if (ret < 0) {
		pov_ring_send_error_on_link(incoming, cmd->seq, rotor->arm_id,
			rotor->arm_id, next_arm,
			pov_ring_error_from_ret(ret));
		return ret;
	}

	ret = pov_ring_wait_link_ack(forward, cmd->seq, MAV_RING_RESULT_OK, POV_ARM_COUNT - 1,
			POV_RING_FILE_TIMEOUT_MS, &ack, &err);
	if (ret < 0) {
		if (err.pending) {
			pov_ring_send_error_on_link(incoming, err.seq,
				err.error_node,
				err.error_link_from,
				err.error_link_to,
				err.error_code);
		} else {
			pov_ring_send_error_on_link(incoming, cmd->seq,
				rotor->arm_id,
				rotor->arm_id, next_arm,
				pov_ring_error_from_ret(ret));
		}
		return ret;
	}

	ret = pov_reload_images_after_update(rotor);
	if (ret < 0) {
		pov_ring_send_error_on_link(incoming, cmd->seq, rotor->arm_id,
			cmd->current_arm, rotor->arm_id,
			pov_ring_error_from_ret(ret));
		return ret;
	}

	return mavlink_send_ring_ack(incoming, cmd->seq, ack.node, MAV_RING_RESULT_OK);
}

static int pov_ring_handle_bootloader_cmd(struct pov_rotor *rotor,
	struct mavlink_device *incoming)
{
	struct mavlink_bl_cmd cmd = incoming->bl;
	struct mavlink_device *forward;
	char path[96];
	int ret;

	incoming->bl.pending = 0;
	if (cmd.command != MAV_BL_CMD_PREPARE_UPDATE ||
	    cmd.firmware_size == 0 ||
	    cmd.filename[0] == '\0' ||
	    strchr(cmd.filename, '/') ||
	    strchr(cmd.filename, '\\')) {
		ret = -EINVAL;
		goto fail;
	}

	ret = pov_ring_receive_firmware(rotor, incoming, cmd.filename,
		cmd.firmware_size, path, sizeof(path));
	if (ret < 0)
		goto fail;

	if (!is_last_arm(rotor)) {
		forward = pov_ring_forward_mavlink(rotor);
		if (!forward) {
			ret = -ENODEV;
			goto fail;
		}

		ret = mavlink_send_bootloader_update(forward, cmd.command,
			cmd.firmware_size, cmd.firmware_crc32,
			cmd.filename, POV_RING_LINK_TIMEOUT_MS);
		if (ret < 0)
			goto fail;

		ret = pov_ring_send_firmware(rotor, forward, path,
			cmd.firmware_size);
		if (ret < 0)
			goto fail;

		ret = mavlink_wait_bootloader_ack(forward, cmd.command,
			MAV_BL_ACK_COMPLETED, POV_RING_FILE_TIMEOUT_MS);
		if (ret < 0)
			goto fail;
	}

	ret = pov_update_firmware(rotor, incoming->serial, path,
		cmd.firmware_size, cmd.firmware_crc32);
	if (ret < 0)
		goto fail;

#if KINETIS_FAKE_SIM
	/* Success: commit the transaction so the snapshot is released.  Skipping
	 * this leaked the kmemdup()'d flash and left a stale entry behind. */
	pov_fake_firmware_transaction_finish(false);
#endif
	return mavlink_send_bootloader_ack(incoming, cmd.command,
		MAV_BL_ACK_COMPLETED);

fail:
#if KINETIS_FAKE_SIM
	pov_fake_firmware_transaction_finish(true);
#endif
	mavlink_send_bootloader_ack(incoming, cmd.command, MAV_BL_ACK_ERROR);
	return ret;
}

static void pov_ring_handle_cmd(struct pov_rotor *rotor,
	struct mavlink_device *mav)
{
	struct mavlink_ring_cmd_state cmd = mav->ring_cmd;
	struct mavlink_ring_ack_state ack;
	struct mavlink_ring_error_state err;
	struct mavlink_device *forward;
	struct mavlink_device *reverse;
	u8 next_arm = (rotor->arm_id + 1) % POV_ARM_COUNT;
	int ret;

	mav->ring_cmd.pending = 0;

	if (cmd.direction != MAV_RING_DIRECTION_CW) {
		pov_ring_forward_error(rotor, cmd.seq, rotor->arm_id,
			cmd.current_arm, rotor->arm_id,
			MAV_RING_ERR_INVALID_PARAM);
		return;
	}

	if (cmd.cmd == MAV_RING_CMD_FILE_DISTRIBUTE) {
		pov_ring_handle_file_cmd(rotor, mav, &cmd);
		return;
	}

	if (is_last_arm(rotor)) {
		ret = pov_ring_execute_cmd(rotor, &cmd);
		if (ret < 0) {
			pov_ring_forward_error(rotor, cmd.seq, rotor->arm_id,
				cmd.current_arm, rotor->arm_id,
				MAV_RING_ERR_EXEC_FAILED);
			return;
		}
		reverse = pov_ring_reverse_mavlink(rotor);
		if (reverse) {
			mavlink_send_ring_ack(reverse, cmd.seq, rotor->arm_id, MAV_RING_RESULT_OK);
		}
		return;
	}

	forward = pov_ring_forward_mavlink(rotor);
	if (!forward) {
		pov_ring_forward_error(rotor, cmd.seq, rotor->arm_id,
			rotor->arm_id, next_arm,
			MAV_RING_ERR_NEXT_LINK_TIMEOUT);
		return;
	}
	ret = pov_ring_forward_cmd(rotor, &cmd);
	if (ret < 0) {
		pov_ring_forward_error(rotor, cmd.seq, rotor->arm_id,
			rotor->arm_id, next_arm,
			MAV_RING_ERR_NEXT_LINK_TIMEOUT);
		return;
	}

	ret = pov_ring_wait_link_ack(forward, cmd.seq, MAV_RING_RESULT_OK, POV_ARM_COUNT - 1,
		POV_RING_LINK_TIMEOUT_MS, &ack, &err);
	if (ret < 0) {
		pov_ring_send_error_on_link(mav, cmd.seq, rotor->arm_id, rotor->arm_id,
			next_arm, pov_ring_error_from_ret(ret));
		return;
	}
	ret = pov_ring_execute_cmd(rotor, &cmd);
	if (ret < 0) {
		pov_ring_send_error_on_link(mav, cmd.seq, rotor->arm_id, cmd.current_arm,
			rotor->arm_id, MAV_RING_ERR_EXEC_FAILED);
		return;
	}
	mavlink_send_ring_ack(mav, cmd.seq, ack.node, MAV_RING_RESULT_OK);
}

static void pov_ring_handle_ack(struct pov_rotor *rotor,
	struct mavlink_device *mav)
{
	struct mavlink_ring_ack_state ack = mav->ring_ack;
	struct mavlink_device *reverse;

	mav->ring_ack.pending = 0;

	if (ack.result == MAV_RING_RESULT_READY) {
		pr_debug("ring: READY seq=%u node=%u on %s\n", ack.seq, ack.node, MD_NAME(mav));
		return;
	}

	if (is_master_arm(rotor)) {
		rotor_info(rotor, "ring: ACK seq=%u node=%u result=%u\n", ack.seq, ack.node, ack.result);
		return;
	}

	reverse = pov_ring_reverse_mavlink(rotor);
	if (reverse) {
		mavlink_send_ring_ack(reverse, ack.seq, ack.node, ack.result);
	}
}

static void pov_ring_handle_error(struct pov_rotor *rotor,
	struct mavlink_device *mav)
{
	struct mavlink_ring_error_state err = mav->ring_error;
	struct mavlink_device *reverse;

	mav->ring_error.pending = 0;

	if (is_master_arm(rotor)) {
		rotor_err(rotor, "ring: ERROR seq=%u node=%u link=%u code=%u\n",
			err.seq, err.error_node, err.error_link_from, err.error_code);
		return;
	}

	reverse = pov_ring_reverse_mavlink(rotor);
	if (reverse) {
		mavlink_send_ring_error(reverse, err.seq, err.error_node,
			err.error_link_from, err.error_link_to,
			err.error_code);
	}
}

static void pov_ring_poll_mavlink(struct pov_rotor *rotor,
	struct mavlink_device *mav,
	bool allow_blocking)
{
	u32 old_rx;

	if (!mav) {
		return;
	}

	old_rx = mav->rx_count;
	if (!(mav->ring_cmd.pending && mav->ring_cmd.cmd == MAV_RING_CMD_FILE_DISTRIBUTE && !allow_blocking) &&
		!(mav->ring_error.pending && mav->ring_cmd.cmd == MAV_RING_CMD_FILE_DISTRIBUTE && !allow_blocking) &&
		!(mav->ring_cmd.pending && !allow_blocking) &&
		serial_port_data_available(mav->serial) > 0) {
		mavlink_receive_and_process(mav, 10);
	}

	if (mav->rx_count == old_rx &&
		!mav->ring_error.pending &&
		!mav->ring_ack.pending &&
		!mav->ring_cmd.pending &&
		!mav->bl.pending) {
		return;
	}
	if (mav->ring_error.pending) {
		pov_ring_handle_error(rotor, mav);
	}
	if (mav->ring_ack.pending) {
		pov_ring_handle_ack(rotor, mav);
	}
	if (mav->bl.pending && allow_blocking) {
		pov_ring_handle_bootloader_cmd(rotor, mav);
	}
	if (mav->ring_cmd.pending && (allow_blocking || mav->ring_cmd.cmd != MAV_RING_CMD_FILE_DISTRIBUTE)) {
		pov_ring_handle_cmd(rotor, mav);
	}
}

static void pov_ring_poll(struct pov_rotor *rotor, bool allow_blocking)
{
	pov_ring_poll_mavlink(rotor, rotor->ring_cw_mavlink, allow_blocking);
	pov_ring_poll_mavlink(rotor, rotor->ring_ccw_mavlink, allow_blocking);
}

static int pov_ring_wait_result(struct pov_rotor *rotor, u16 seq,
	int expected_ack_node, u32 timeout_ms)
{
	struct mavlink_device *links[] = {
		rotor->ring_cw_mavlink,
		rotor->ring_ccw_mavlink,
	};
	u64 deadline = basic_timer_get_ms() + timeout_ms;

	while (basic_timer_get_ms() < deadline) {
		pov_send_stator_heartbeat_if_due(rotor);

		for (u8 i = 0; i < ARRAY_SIZE(links); i++) {
			struct mavlink_device *mav = links[i];

			if (!mav) {
				continue;
			}

			if (serial_port_data_available(mav->serial) > 0) {
				mavlink_receive_and_process(mav, 10);
			}

			if (mav->ring_error.pending && mav->ring_error.seq == seq) {
				rotor_err(rotor, "ring: task %u failed at node %u code=%u link=%u->%u\n",
					seq, mav->ring_error.error_node, mav->ring_error.error_code,
					mav->ring_error.error_link_from, mav->ring_error.error_link_to);
				mav->ring_error.pending = 0;
				return -EIO;
			}

			if (mav->ring_ack.pending && mav->ring_ack.seq == seq) {
				u8 ack_node = mav->ring_ack.node;
				u8 result = mav->ring_ack.result;

				rotor_info(rotor, "ring: task %u acked by node %u result=%u\n", seq, ack_node, result);
				mav->ring_ack.pending = 0;
				if (result == MAV_RING_RESULT_OK) {
					if (expected_ack_node == POV_ARM_BROADCAST || ack_node == expected_ack_node) {
						return 0;
					}

					pr_debug("ring: task %u ignoring ack from node %u, expected %u\n",
						seq, ack_node, expected_ack_node);
				}
				if (result == MAV_RING_RESULT_ERROR) {
					return -EIO;
				}
			}
		}
		mdelay(5);
	}

	rotor_err(rotor, "ring: task %u timed out waiting for ACK/ERROR\n", seq);
	return -ETIMEDOUT;
}

static int pov_ring_send_cmd_and_wait(struct pov_rotor *rotor, u16 cmd,
	const u8 *payload, u8 payload_len)
{
	u16 seq;
	int ret;

	if (POV_ARM_COUNT <= 1) {
		return 0;
	}

	if (!rotor->ring_cw_mavlink) {
		rotor_warn(rotor, "ring: no clockwise link configured\n");
		return -ENODEV;
	}

	seq = ++rotor->ring_next_seq;
	if (seq == 0) {
		seq = ++rotor->ring_next_seq;
	}

	pov_ring_clear_result_state(rotor->ring_cw_mavlink);
	pov_ring_clear_result_state(rotor->ring_ccw_mavlink);

	ret = mavlink_send_ring_cmd(rotor->ring_cw_mavlink, seq, cmd,
			rotor->arm_id, POV_ARM_BROADCAST, rotor->arm_id,
			MAV_RING_DIRECTION_CW, payload, payload_len);
	if (ret < 0) {
		return ret;
	}

	return pov_ring_wait_result(rotor, seq, POV_ARM_COUNT - 1, POV_RING_ACK_TIMEOUT_MS);
}

static int pov_ring_transfer_files(struct pov_rotor *rotor, const char *layer)
{
	struct mavlink_ring_ack_state ack;
	struct mavlink_ring_error_state err;
	u8 payload[MAVLINK_RING_PAYLOAD_LEN];
	size_t payload_len;
	u16 seq;
	u8 next_arm = (rotor->arm_id + 1) % POV_ARM_COUNT;
	int ret;

	if (POV_ARM_COUNT <= 1) {
		return 0;
	}
	if (!layer || layer[0] == '\0') {
		return -EINVAL;
	}
	if (!pov_layer_name_valid(layer)) {
		return -EINVAL;
	}
	if (!rotor->ring_cw_mavlink) {
		rotor_warn(rotor, "ring: no clockwise link configured\n");
		return -ENODEV;
	}

	memset(payload, 0, sizeof(payload));
	payload_len = strnlen(layer, MAVLINK_RING_PAYLOAD_LEN - 1);
	if (payload_len == 0) {
		return -EINVAL;
	}
	memcpy(payload, layer, payload_len);
	payload[payload_len++] = '\0';

	seq = ++rotor->ring_next_seq;
	if (seq == 0) {
		seq = ++rotor->ring_next_seq;
	}

	pov_ring_clear_result_state(rotor->ring_cw_mavlink);
	pov_ring_clear_result_state(rotor->ring_ccw_mavlink);

	ret = mavlink_send_ring_cmd(rotor->ring_cw_mavlink, seq, MAV_RING_CMD_FILE_DISTRIBUTE,
			rotor->arm_id, POV_ARM_BROADCAST, rotor->arm_id,
			MAV_RING_DIRECTION_CW, payload, payload_len);
	if (ret < 0) {
		return ret;
	}

	ret = pov_ring_wait_link_ack(rotor->ring_cw_mavlink, seq,
			MAV_RING_RESULT_READY, next_arm, POV_RING_LINK_TIMEOUT_MS, &ack, &err);
	if (ret < 0) {
		return ret;
	}

	ret = pov_ring_send_layer_files(rotor, rotor->ring_cw_mavlink, rotor->image_path, layer);
	if (ret < 0) {
		return ret;
	}

	return pov_ring_wait_link_ack(rotor->ring_cw_mavlink, seq,
			MAV_RING_RESULT_OK, POV_ARM_COUNT - 1,
			POV_RING_FILE_TIMEOUT_MS, &ack, &err);
}

static int pov_ring_transfer_firmware(struct pov_rotor *rotor,
	const char *path, const char *filename,
	u32 firmware_size, u32 firmware_crc32)
{
	int ret;

	if (POV_ARM_COUNT <= 1)
		return -EOPNOTSUPP;
	if (!rotor->ring_cw_mavlink)
		return -ENODEV;

	ret = mavlink_send_bootloader_update(rotor->ring_cw_mavlink,
		MAV_BL_CMD_PREPARE_UPDATE, firmware_size, firmware_crc32,
		filename, POV_RING_LINK_TIMEOUT_MS);
	if (ret < 0)
		return ret;

	ret = pov_ring_send_firmware(rotor, rotor->ring_cw_mavlink,
		path, firmware_size);
	if (ret < 0)
		return ret;

	return mavlink_wait_bootloader_ack(rotor->ring_cw_mavlink,
		MAV_BL_CMD_PREPARE_UPDATE, MAV_BL_ACK_COMPLETED,
		POV_RING_FILE_TIMEOUT_MS);
}

static int pov_send_image_files(struct pov_rotor *rotor, const char *layer)
{
	const char **files = NULL;
	int count = 0;
	int ret;

	if (!pov_layer_name_valid(layer)) {
		rotor_warn(rotor, "ZMODEM: invalid layer path '%s'\n",
			layer ? layer : "(null)");
		return -EINVAL;
	}

	ret = pov_ring_build_layer_file_list(rotor->image_path, layer, &files, &count);
	if (ret < 0) {
		rotor_warn(rotor, "ZMODEM: no files to send for %s (%d)\n",
			layer, ret);
		mavlink_send_ft_ack(rotor->app_mavlink, MAV_FILE_TRANSFER_DIR_DOWNLOAD,
			MAV_FT_RESULT_NOT_FOUND, 0);
		return ret;
	}

	mavlink_send_ft_ack(rotor->app_mavlink, MAV_FILE_TRANSFER_DIR_DOWNLOAD,
		MAV_FT_RESULT_READY, 0);

	rotor_info(rotor, "ZMODEM: sending %d file(s) from %s to phone...\n",
		count, layer);

	ret = lrzsz_sz(rotor->app_port, count, files, NULL);
	pov_ring_free_file_list(files, count);
	if (ret <= 0) {
		rotor_warn(rotor, "ZMODEM: send failed or cancelled (%d)\n", ret);
		return -EIO;
	}

	rotor_info(rotor, "ZMODEM: sent %d bytes\n", ret);
	return 0;
}

static int pov_list_image_files(struct pov_rotor *rotor, const char *layer)
{
	DIR dir;
	FILINFO fno;
	FRESULT res;
	char layer_dir[64];
	u16 file_count = 0;
	u16 list_index = 0;
	u16 sent = 0;
	int ret;
	int l;

	/* Count files so every response carries a useful total and a global index. */
	for (l = 0; l < POV_LAYER_COUNT; l++) {
		char name[16];

		snprintf(name, sizeof(name), "layer%d", l);
		if (layer && layer[0] != '\0' && strcmp(layer, name) != 0) {
			continue;
		}

		snprintf(layer_dir, sizeof(layer_dir), "%s/%s",
			rotor->image_path, name);
		res = f_opendir(&dir, layer_dir);
		if (res != FR_OK) {
			continue;
		}

		while (1) {
			res = f_readdir(&dir, &fno);
			if (res != FR_OK) {
				break;
			}
			if (fno.fname[0] == 0) {
				break;
			}
			if (!pov_ring_is_transfer_file(&fno)) {
				continue;
			}
			if (file_count == 0xffffu) {
				f_closedir(&dir);
				return -EOVERFLOW;
			}
			file_count++;
		}
		f_closedir(&dir);
	}

	for (l = 0; l < POV_LAYER_COUNT; l++) {
		char name[16];

		snprintf(name, sizeof(name), "layer%d", l);
		if (layer && layer[0] != '\0' && strcmp(layer, name) != 0) {
			continue;
		}

		snprintf(layer_dir, sizeof(layer_dir), "%s/%s",
			rotor->image_path, name);
		res = f_opendir(&dir, layer_dir);
		if (res != FR_OK) {
			continue;
		}

		while (1) {
			res = f_readdir(&dir, &fno);
			if (res != FR_OK || fno.fname[0] == 0) {
				break;
			}
			if (!pov_ring_is_transfer_file(&fno)) {
				continue;
			}

			ret = mavlink_send_ft_list(rotor->app_mavlink,
				file_count, list_index, (u32)fno.fsize, fno.fname);
			if (ret < 0) {
				f_closedir(&dir);
				return ret;
			}
			list_index++;
			sent++;
		}
		f_closedir(&dir);
	}

	file_count = list_index;
	ret = mavlink_send_ft_list(rotor->app_mavlink, file_count, file_count, 0, NULL);
	if (ret < 0) {
		return ret;
	}

	rotor_info(rotor, "ft list: reported %u file(s) to phone\n", sent);
	return 0;
}

static int pov_receive_image_files(struct pov_rotor *rotor)
{
	char dest_dir[64];
	char *layer = rotor->app_mavlink->ft.path;
	int ret;

	if (!pov_layer_name_valid(layer)) {
		rotor_warn(rotor, "ZMODEM: invalid layer path '%s'\n", layer ? layer : "(null)");
		return -EINVAL;
	}

	snprintf(dest_dir, sizeof(dest_dir), "%s/%s", rotor->image_path, layer);
	ret = fatfs_create_dirs(dest_dir);
	if (ret < 0) {
		return ret;
	}

	rotor_info(rotor, "ZMODEM: waiting for phone to send files on app_port...\n");

	/*
	 * lrzsz_rz() blocks until transfer completes.
	 * Files are saved to rotor's image directory.
	 * Returns number of bytes received, or 0 on error.
	 */
	ret = lrzsz_rz(rotor->app_port, dest_dir);
	if (ret == -ECANCELED)
		return ret;
	else if (ret <= 0) {
		rotor_warn(rotor, "ZMODEM: receive failed or cancelled (%d)\n", ret);
		return -EIO;
	}

	rotor_info(rotor, "ZMODEM: received %d bytes, awaiting ring commit...\n", ret);
	return 0;
}

static int pov_read_next_sector(struct pov_rotor *rotor)
{
	FRESULT res;
	UINT br;
	u32 read_bytes = POV_PREFETCH_COLS * POV_LED_DRIVER_COL_BYTES;

	res = f_read(&rotor->image_file, (u8 *)rotor->led_driver_frame[rotor->prefetch_ping], read_bytes, &br);
	if (res != FR_OK) {
		rotor_err(rotor, "read prefetch batch failed: %d (got %u)\n", res, br);
		return -EIO;
	}
	if (br == 0) {
		return -ENOENT;    /* End of image */
	}
	/* A short read would leave half of the ping buffer stale; those columns
	 * would be clocked out to the LED drivers and desync angle <-> column. */
	if (br != read_bytes) {
		rotor_err(rotor, "short prefetch read: got %u of %u bytes\n",
			br, read_bytes);
		return -EIO;
	}
	rotor->pending_column += br;
	rotor->prefetch_ping ^= 1;
	rotor->prefetch_sent_index = 0;

	return 0;
}

static u32 pov_get_angle(struct pov_rotor *rotor)
{
	u32 angle;

#if POV_USE_MT6701
	mt6701_ssi_frame_t frame;

	mt6701_read_angle_ssi(rotor->angle_sensor, &frame);
	/* Scale 14-bit raw angle [0, 16383] → column [0, POV_DISPLAY_COLS) */
	angle = (u32)frame.raw_angle * POV_DISPLAY_COLS / POV_ANGLE_RESOLUTION;
	if (angle >= POV_DISPLAY_COLS) {
		angle = POV_DISPLAY_COLS - 1;
	}
#else
	struct hall_device *hall = rotor->angle_sensor;

	if (hall->period_ticks == 0) {
		return 0;
	}

	/* Map to column number */
	angle = (hall->read_rotated_time(hall) * hall->angular_resolution) / hall->period_ticks;
	if (angle >= hall->angular_resolution) {
		angle = hall->angular_resolution - 1;
	}
#endif

	if (rotor->arm_id == 0) {
		return angle;
	}

	return (angle + rotor->arm_id * (POV_DISPLAY_COLS / POV_ARM_COUNT)) % POV_DISPLAY_COLS;
}

static s32 pov_get_rpm(struct pov_rotor *rotor)
{
#if POV_USE_MT6701
	return mt6701_read_rpm_block(rotor->angle_sensor, true, 10000);
#else
	struct hall_device *hall = rotor->angle_sensor;

	if (hall->period_ticks == 0) {
		return 0;
	}

	return 60000000 / hall->period_ticks;
#endif
}

static int display_image(struct pov_rotor *rotor)
{
	u32 last_angle = pov_get_angle(rotor);
	u32 stall_us;
	u64 deadline;
	int ret;

	ret = pov_read_next_sector(rotor);
	if (ret < 0) {
		return ret;
	}
#if KINETIS_FAKE_SIM == 0
// 	sd_set_wait_for_completion(0);
	flash_set_wait_for_completion(0);
#endif

	while (1) {
#if KINETIS_FAKE_SIM
		if (rotor->forced_offline) {
			rotor->paused = 1;
			ret = -EAGAIN;
			goto over;
		}
#endif
		if (rotor->prefetch_sent_index == POV_PREFETCH_COLS ||
			rotor->prefetch_sent_index == 0) {
			ret = pov_read_next_sector(rotor);
			if (ret == -ENOENT) {
				ret = 0;
				break;
			}
			if (ret < 0) {
				goto over;
			}
		}

		/* Wait for angle_sensor to advance to the next column,
		 * with timeout to prevent infinite loop on sensor failure */
#if POV_USE_MT6701
		stall_us = 0;
#else
		stall_us = rotor->angle_sensor->period_ticks;
#endif
		deadline = basic_timer_get_us() + (stall_us > 1000 ? stall_us : 1000);
		for (;;) {
			if (pov_get_angle(rotor) != last_angle)
				break;
			if (basic_timer_get_us() > deadline) {
				if (pov_get_angle(rotor) == last_angle) {
					rotor_err(rotor, "display: angle_sensor timeout at column %u\n", last_angle);
					ret = -ETIMEDOUT;
					goto over;
				}
				break;
			}
#if KINETIS_FAKE_SIM
			Sleep(0);
#endif
		}

		ret = pov_set_rgb(rotor);
		if (ret < 0) {
			goto over;
		}
		rotor->prefetch_sent_index++;

		last_angle = pov_get_angle(rotor);
	}

over:
#if KINETIS_FAKE_SIM == 0
// 	sd_set_wait_for_completion(1);
	flash_set_wait_for_completion(1);
#endif
	return ret;
}

static int display_image_benchmark(struct pov_rotor *rotor)
{
	int ret;

	ret = pov_read_next_sector(rotor);
	if (ret < 0) {
		return ret;
	}
#if KINETIS_FAKE_SIM == 0
// 	sd_set_wait_for_completion(0);
	flash_set_wait_for_completion(0);
#endif

	while (1) {
		if (rotor->prefetch_sent_index == POV_PREFETCH_COLS ||
			rotor->prefetch_sent_index == 0) {
			ret = pov_read_next_sector(rotor);
			if (ret == -ENOENT) {
				ret = 0;
				break;
			}
			if (ret < 0) {
				goto over;
			}
		}

		ret = pov_set_rgb(rotor);
		if (ret < 0) {
			goto over;
		}
		rotor->prefetch_sent_index++;
	}

over:
#if KINETIS_FAKE_SIM == 0
// 	sd_set_wait_for_completion(1);
	flash_set_wait_for_completion(1);
#endif
	return ret;
}

static int pov_switch_image(struct pov_rotor *rotor, u8 index)
{
	FIL new_file;
	struct pov_image_header new_header;
	char file_path[128];
	UINT br;
	FRESULT res;
	int ret;

	if (index >= rotor->image_count) {
		rotor_warn(rotor, "image index %u out of range (max %u)\n",
			index, rotor->image_count);
		return -EINVAL;
	}
	if (!rotor->image_names[index]) {
		rotor_warn(rotor, "image index %u has no backing name\n", index);
		return -EINVAL;
	}

	/*
	 * Open the replacement *first*.  Closing the current image before the
	 * new one is known-good would leave the FSM in DISPLAYING with
	 * images_open == 0, i.e. reading from a closed FIL.
	 */
	snprintf(file_path, sizeof(file_path), "%s/%s/%s",
		rotor->image_path, "blend", rotor->image_names[index]);
	res = f_open(&new_file, file_path, FA_READ);
	if (res != FR_OK) {
		rotor_err(rotor, "cannot open '%s': %d\n", file_path, res);
		return -EIO;
	}

	res = f_read(&new_file, &new_header, sizeof(new_header), &br);
	if (res != FR_OK || br != sizeof(new_header)) {
		rotor_err(rotor, "cannot read header of '%s': %d(got %u)\n",
			file_path, res, br);
		f_close(&new_file);
		return -EIO;
	}

	ret = pov_validate_display_image(&new_file, &new_header, rotor->image_names[index]);
	if (ret < 0) {
		f_close(&new_file);
		return ret;
	}

	/* Replacement is good — now it is safe to drop the old handle. */
	if (rotor->images_open) {
		pov_close_image(rotor);
	}
	rotor->image_file = new_file;
	rotor->image_header = new_header;

	rotor->images_open = 1;
	rotor->pending_column = sizeof(rotor->image_header);
	rotor->image_current = index;
	rotor->prefetch_ping = 0;
	rotor->prefetch_sent_index = 0;
	rotor_info(rotor, "switched to image [%d]: %s (%d cols)\n",
		index, rotor->image_names[index],
		rotor->image_header.columns);
	return 0;
}

static int pov_handle_end_of_image(struct pov_rotor *rotor)
{
	struct mavlink_image_cmd *cmd = &rotor->image_cmd;
	u8 action;
	FRESULT res;

	if (cmd->pending && rotor->image_cmd_delay_frames > 0) {
		rotor->image_cmd_delay_frames--;
		action = MAV_IMAGE_ACTION_REPLAY;
		pr_debug("image cmd pending, delay frames: %u\n",
			rotor->image_cmd_delay_frames);
	} else {
		action = cmd->pending ? cmd->action : MAV_IMAGE_ACTION_REPLAY;
		cmd->pending = 0;
		rotor->image_cmd_delay_frames = 0;
	}

	switch (action) {
	case MAV_IMAGE_ACTION_REPLAY:
		res = f_lseek(&rotor->image_file, sizeof(struct pov_image_header));
		if (res != FR_OK) {
			return -EIO;
		}
		rotor->pending_column = sizeof(struct pov_image_header);
		rotor->prefetch_ping = 0;
		rotor->prefetch_sent_index = 0;
// 		pr_info("replaying current image %s\n", rotor->image_names[rotor->image_current]);
		return 0;

	case MAV_IMAGE_ACTION_NEXT: {
		u8 next;

		if (rotor->image_count == 0)
			return -ENOENT;
		next = (rotor->image_current + 1) % rotor->image_count;
		return pov_switch_image(rotor, next);
	}

	case MAV_IMAGE_ACTION_PREVIOUS: {
		u8 prev;

		if (rotor->image_count == 0)
			return -ENOENT;
		prev = (rotor->image_current == 0) ?
			rotor->image_count - 1 : rotor->image_current - 1;
		return pov_switch_image(rotor, prev);
	}

	case MAV_IMAGE_ACTION_PLAY_INDEX:
		if (cmd->image_index >= rotor->image_count)
			return -EINVAL;
		return pov_switch_image(rotor, (u8)cmd->image_index);

	default:
		rotor_warn(rotor, "unknown image action %d, replaying\n", action);
		res = f_lseek(&rotor->image_file, sizeof(struct pov_image_header));
		if (res != FR_OK) {
			return -EIO;
		}
		rotor->pending_column = sizeof(struct pov_image_header);
		rotor->prefetch_ping = 0;
		rotor->prefetch_sent_index = 0;
		return 0;
	}
}

static int waiting_for_motor_connection(struct pov_rotor *rotor)
{
	u32 old_rx;
	u64 now;

	rotor->motor_port->ops->enable();

	while (1) {
		now = basic_timer_get_ms();

		if (now - rotor->last_send_stator_heartbeat >= (u64)POV_HEARTBEAT_INTERVAL_MS) {
			rotor->last_send_stator_heartbeat = now;
			mavlink_send_heartbeat(rotor->motor_mavlink, MAV_SYSTEM_STATUS_ACTIVE, 0, 0);
		}

#if POV_USE_NRF24L01
		nrf24l01_port_poll(rotor->motor_port);
#endif
		if (serial_port_data_available(rotor->motor_mavlink->serial) == 0) {
			/* Yield — a bare continue() busy-spins at 100% CPU and starves
			 * the serial port RX thread on a single-core target. */
#if KINETIS_FAKE_SIM
			usleep(1000);
#else
			mdelay(1);
#endif
			continue;
		}

		old_rx = rotor->motor_mavlink->rx_count;
		mavlink_receive_and_process(rotor->motor_mavlink, 10);

		/* Any message from a stator reporting ACTIVE = handshake success */
		if (rotor->motor_mavlink->rx_count > old_rx) {
			if (rotor->motor_mavlink->sys_status == MAV_SYSTEM_STATUS_ACTIVE) {
				rotor_info(rotor, "motor connected!\n");
				rotor->last_recv_stator_heartbeat = basic_timer_get_ms();
				set_fsm_state(rotor, POV_STATE_SEND_SPEED);
				return 0;
			} else if (rotor->motor_mavlink->sys_status == MAV_SYSTEM_STATUS_FAULT) {
				rotor_err(rotor, "motor in fault state\n");
				return -EPIPE;
			}
		}
	}

	return -ENODEV;
}

static int waiting_for_phone_connection(struct pov_rotor *rotor)
{
	u32 old_rx = rotor->app_mavlink->rx_count;

	if (serial_port_data_available(rotor->app_mavlink->serial) == 0) {
		return -ENODEV;
	}

	mavlink_receive_and_process(rotor->app_mavlink, 10);

	/* Any received MAVLink message confirms the phone is alive. */
	if (rotor->app_mavlink->rx_count > old_rx) {
		rotor_info(rotor, "phone connected!\n");
		set_fsm_state(rotor, POV_STATE_SEND_SPEED);
		return 0;
	}

	return -EAGAIN;
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

	rotor_info(rotor, "sent target RPM=%d to motor\n", rpm);

	while (query_count < POV_SPEED_QUERY_RETRIES) {
		ret = mavlink_query_motor_status(rotor->motor_mavlink, 0, 500);
		if (ret < 0) {
			if (ret != -ETIMEDOUT)
				return ret;
		} else {
			rotor->last_recv_stator_heartbeat = basic_timer_get_ms();

			speed = rotor->motor_mavlink->motors[0].current_speed;
			if (abs(speed - (s32)rpm) < 5 &&
				rotor->motor_mavlink->motors[0].direction == POV_DIR_FORWARD) {
				rotor_info(rotor, "motor reached target RPM: %d (current=%d)\n", rpm, speed);
				break;
			}
		}

		query_count++;
		mdelay(POV_SPEED_QUERY_INTERVAL_MS);
	}

	speed = pov_get_rpm(rotor);
	if (abs(speed - (s32)rpm) > 5) {
		rotor_warn(rotor, "angle_sensor RPM %d out of range (target %d)\n", speed, rpm);
		return -EAGAIN;
	}

	rotor_info(rotor, "speed confirmed, entering display mode\n");
	set_fsm_state(rotor, POV_STATE_DISPLAYING);

	rotor->app_port->ops->enable();

	return 0;
}

static int wait_speed_stable(struct pov_rotor *rotor)
{
	s32 speed = pov_get_rpm(rotor);

	if (speed < 0)
		speed = -speed;

	if (abs(speed - (s32)POV_TARGET_RPM) < 5) {
		if (++rotor->speed_stable_samples >= 10) {
			rotor->speed_stable_samples = 0;
			return 0;
		}
	} else {
		rotor->speed_stable_samples = 0;
	}

	mdelay(POV_SPEED_QUERY_INTERVAL_MS);
	return -EAGAIN;
}

static int check_stator_heartbeat(struct pov_rotor *rotor)
{
	u32 old_rx = rotor->motor_mavlink->rx_count;

	/* Non-blocking poll (0 ms timeout) — only drains bytes already in buffer */
#if POV_USE_NRF24L01
	nrf24l01_port_poll(rotor->motor_port);
#endif
	if (serial_port_data_available(rotor->motor_port) > 0) {
		mavlink_receive_and_process(rotor->motor_mavlink, 0);
	}

	/* If any new message arrived and stator reports ACTIVE,
	 * update heartbeat timestamp. Using sys_status is robust
	 * against batch messages (sys_status persists across all
	 * messages in the batch). */
	if (rotor->motor_mavlink->rx_count > old_rx &&
		rotor->motor_mavlink->sys_status == MAV_SYSTEM_STATUS_ACTIVE) {
		rotor->last_recv_stator_heartbeat = basic_timer_get_ms();
	}

	/* Check timeout */
	if (basic_timer_get_ms() - rotor->last_recv_stator_heartbeat > (u64)POV_HEARTBEAT_INTERVAL_MS * 2) {
		rotor_warn(rotor, "stator heartbeat timeout (%llu ms)\n",
			basic_timer_get_ms() - rotor->last_recv_stator_heartbeat);
		return -ETIMEDOUT;
	}

	return 0;
}

static int display_loop(struct pov_rotor *rotor)
{
	u64 t_start, t_end;
	int ret;

	if (is_master_arm(rotor)) {
		ret = check_stator_heartbeat(rotor);
		if (ret < 0) {
			rotor_err(rotor, "stator disconnected during display\n");
			set_fsm_state(rotor, POV_STATE_ERROR);
			return ret;
		}
	}

	t_start = basic_timer_get_us();

	ret = display_image(rotor);
	if (ret) {
#if KINETIS_FAKE_SIM
		if(ret == -EAGAIN && rotor->forced_offline)
			return 0;
#endif
		goto err;
	}

// 	t_end = basic_timer_get_us() - t_start;
// 	pr_debug("image done: %llu us total, %llu us avg per col (%u cols)\n",
// 		t_end, t_end / POV_DISPLAY_COLS, POV_DISPLAY_COLS);

	if (is_slave_arm(rotor)) {
		set_fsm_state(rotor, POV_STATE_CHECK_RING);
	} else {
		set_fsm_state(rotor, POV_STATE_CHECK_CMD);
	}
	return 0;

err:
	set_fsm_state(rotor, POV_STATE_ERROR);
	return ret;
}

static void error_occurred(struct pov_rotor *rotor)
{
	int ret;
	if (is_master_arm(rotor)) {
		ret = set_motor_speed(rotor, 0);
		if (ret < 0)
			rotor_warn(rotor, "stopping motor failed (%d), retrying...\n", ret);
		set_fsm_state(rotor, POV_STATE_WAIT_MOTOR);
		rotor->motor_port->ops->disable();
		rotor->app_port->ops->disable();
	} else {
		set_fsm_state(rotor, POV_STATE_WAIT_SPEED);
	}
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

#if KINETIS_FAKE_SIM
	kfree(rotor->sim_flash);
#endif

#if POV_USE_MT6701
	if (rotor->angle_sensor) {
		mt6701_exit(rotor->angle_sensor);
	}
#else
	if (rotor->angle_sensor) {
		kfree(rotor->angle_sensor);
	}
#endif

	if (rotor->app_mavlink) {
		mavlink_free(rotor->app_mavlink);
	}

	if (rotor->motor_mavlink) {
		mavlink_free(rotor->motor_mavlink);
	}

	if (rotor->ring_cw_mavlink) {
		mavlink_free(rotor->ring_cw_mavlink);
	}

	if (rotor->ring_ccw_mavlink) {
		mavlink_free(rotor->ring_ccw_mavlink);
	}

	if (rotor->motor_port) {
		serial_port_free(rotor->motor_port);
	}

	if (rotor->app_port) {
		serial_port_free(rotor->app_port);
	}

	if (rotor->ring_cw_port) {
		serial_port_free(rotor->ring_cw_port);
	}

	if (rotor->ring_ccw_port) {
		serial_port_free(rotor->ring_ccw_port);
	}

	if (sizeof(rotor) == 8)
		kfree(rotor);
	else
		memblock_free((phys_addr_t)rotor, sizeof(*rotor));
}

struct pov_rotor *pov_rotor_alloc(struct flash_ops *flash, struct bootloader_ops *bl_ops,
	struct serial_port_ops *motor_serial_ops, struct serial_port_ops *app_serial_ops,
	struct serial_port_ops *ring_cw_serial_ops, struct serial_port_ops *ring_ccw_serial_ops,
	struct spi_master *w25q_spi, struct spi_master *mt6701_spi,
	u32(*read_rotated_time)(struct hall_device *dev))
{
	struct pov_rotor *rotor;
	int ret;

	rotor = memblock_alloc(sizeof(*rotor), 32);
	if (!rotor) {
		return NULL;
	}

	rotor->flash = flash;
	rotor->bl_ops = bl_ops;
#if KINETIS_FAKE_SIM
	rotor->forced_offline = 0;
	pov_fake_firmware_fault_set(rotor, POV_FAULT_NONE);
	rotor->storage_fault = POV_FAULT_NONE;
#endif
	rotor->arm_id = pov_read_arm_id();
	if (rotor->arm_id >= POV_ARM_COUNT) {
		pr_err("pov: invalid arm id %d (max %d)\n", rotor->arm_id, POV_ARM_COUNT - 1);
		goto free_dev;
	}
	pr_info("pov: arm id %d, arm count %d\n", rotor->arm_id, POV_ARM_COUNT);
	snprintf(rotor->image_path, sizeof(rotor->image_path),
		"0:/pov_arm%d", rotor->arm_id);
	snprintf(rotor->firmware_path, sizeof(rotor->firmware_path),
		"0:/pov_arm%d/firmware", rotor->arm_id);

	if (POV_ARM_COUNT > 1) {
		rotor->ring_cw_port = serial_port_alloc(ring_cw_serial_ops, "rotor-ring-cw");
		if (!rotor->ring_cw_port) {
			goto free_dev;
		}

		rotor->ring_cw_mavlink = mavlink_init(rotor->ring_cw_port, 10, 1, "rotor-ring-cw");
		if (!rotor->ring_cw_mavlink) {
			goto free_dev;
		}

		rotor->ring_ccw_port = serial_port_alloc(ring_ccw_serial_ops, "rotor-ring-ccw");
		if (!rotor->ring_ccw_port) {
			goto free_dev;
		}

		rotor->ring_ccw_mavlink = mavlink_init(rotor->ring_ccw_port, 10, 2, "rotor-ring-ccw");
		if (!rotor->ring_ccw_mavlink) {
			goto free_dev;
		}
	}

	if (is_master_arm(rotor)) {
		/* Port to stator (MAVLink motor control) */
		rotor->motor_port = serial_port_alloc(motor_serial_ops, "rotor-motor");
		if (!rotor->motor_port) {
			goto free_dev;
		}

		rotor->motor_mavlink = mavlink_init(rotor->motor_port, 1, 1, "rotor-motor");
		if (!rotor->motor_mavlink) {
			goto free_dev;
		}

		/* Port to phone (MAVLink + ZMODEM file transfer) */
		rotor->app_port = serial_port_alloc(app_serial_ops, "rotor-app");
		if (!rotor->app_port) {
			goto free_dev;
		}

		rotor->app_mavlink = mavlink_init(rotor->app_port, 1, 2, "rotor-app");
		if (!rotor->app_mavlink) {
			goto free_dev;
		}
	}

#if POV_USE_MT6701
	rotor->angle_sensor = mt6701_init(REGMAP_BUS_SPI_SOFT, mt6701_spi);
	if (IS_ERR_OR_NULL(rotor->angle_sensor)) {
		goto free_dev;
	}
#else
	rotor->angle_sensor = hall_alloc_dev(POV_DISPLAY_COLS, read_rotated_time);
	if (!rotor->angle_sensor) {
		goto free_dev;
	}
#endif

#if KINETIS_FAKE_SIM == 0
// 	rotor->w25qxxx = w25qxxx_alloc_device(w25q_spi);
// 	if (!rotor->w25qxxx) {
// 		goto free_dev;
// 	}
// 	w25qxxx_dev = rotor->w25qxxx;

	ret = w25qxx_port_init(&rotor->w25qxxx);
	if (ret) {
		goto free_dev;
	}
	ld_w25qxx_handle = &rotor->w25qxxx;
#endif

	ret = fatfs_init();
	if (ret) {
		goto free_dev;
	}

	/* Init heartbeat tracking: assume stator is alive at startup
	 * (actual connection is established in WAIT_MOTOR handshake) */
	rotor->last_recv_stator_heartbeat = basic_timer_get_ms();
	rotor->last_send_stator_heartbeat = basic_timer_get_ms();
	rotor->ring_next_seq = 0;
	rotor->ready = true;
	rotor->fsm_error_count = 0;
	if (rotor->motor_mavlink) {
		mavlink_send_heartbeat(rotor->motor_mavlink, MAV_SYSTEM_STATUS_ACTIVE, 0, 0);
	}

	return rotor;

free_dev:
	pov_rotor_free(rotor);
	return NULL;
}

static void pov_check_phone_cmd(struct pov_rotor *rotor)
{
	u32 old_rx = rotor->app_mavlink->rx_count;
	u8 ring_payload[MAVLINK_RING_PAYLOAD_LEN];
	int ret;

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
		const char *path = rotor->app_mavlink->ft.path;
		u8 direction = rotor->app_mavlink->ft.direction;
		u8 ack_result;

		rotor_info(rotor, "cmd: file transfer '%s' (%u bytes) %s\n",
			path, rotor->app_mavlink->ft.total_bytes,
			direction == MAV_FILE_TRANSFER_DIR_DOWNLOAD ? "download" : "upload");

		rotor->app_mavlink->ft.state = MAV_FILE_TRANSFER_STATE_IDLE;

		if (direction != MAV_FILE_TRANSFER_DIR_UPLOAD &&
		    direction != MAV_FILE_TRANSFER_DIR_DOWNLOAD) {
			rotor_warn(rotor, "cmd: invalid file transfer direction %u\n",
				direction);
			mavlink_send_ft_ack(rotor->app_mavlink, direction,
				MAV_FT_RESULT_DENIED, 0);
			rotor->app_mavlink->ft.state = MAV_FILE_TRANSFER_STATE_ERROR;
			rotor->app_mavlink->ft.active = 0;
			break;
		}
		if (!pov_layer_name_valid(path)) {
			ack_result = direction == MAV_FILE_TRANSFER_DIR_DOWNLOAD ?
				MAV_FT_RESULT_NOT_FOUND : MAV_FT_RESULT_DENIED;
			rotor_warn(rotor, "cmd: invalid file transfer layer '%s'\n",
				path);
			mavlink_send_ft_ack(rotor->app_mavlink, direction,
				ack_result, 0);
			rotor->app_mavlink->ft.state = MAV_FILE_TRANSFER_STATE_ERROR;
			rotor->app_mavlink->ft.active = 0;
			break;
		}

		if (direction == MAV_FILE_TRANSFER_DIR_DOWNLOAD) {
			mavlink_send_heartbeat(rotor->motor_mavlink,
				MAV_SYSTEM_STATUS_STANDBY, rotor->downtime_s, 0);
			ret = pov_send_image_files(rotor, path);
			mavlink_send_heartbeat(rotor->motor_mavlink,
				MAV_SYSTEM_STATUS_ACTIVE, rotor->downtime_s, 0);
			if (ret) {
				rotor_err(rotor, "cmd: failed to send image files\n");
				set_fsm_state(rotor, POV_STATE_ERROR);
				return;
			}
			rotor->app_mavlink->ft.state = MAV_FILE_TRANSFER_STATE_COMPLETED;
			rotor->app_mavlink->ft.active = 0;
			break;
		}

		ret = mavlink_send_ft_ack(rotor->app_mavlink, MAV_FILE_TRANSFER_DIR_UPLOAD,
			MAV_FT_RESULT_READY, rotor->app_mavlink->ft.total_bytes);
		if (ret < 0) {
			rotor_err(rotor, "cmd: failed to acknowledge upload (%d)\n", ret);
			set_fsm_state(rotor, POV_STATE_ERROR);
			return;
		}

		mavlink_send_heartbeat(rotor->motor_mavlink,
			MAV_SYSTEM_STATUS_STANDBY, rotor->downtime_s, 0);
		ret = pov_receive_image_files(rotor);
		/* Image receive is a blocking zmodem transfer.  Heartbeats queued
		 * while it runs still prove that the stator link is alive. */
		rotor->last_recv_stator_heartbeat = basic_timer_get_ms();
		if (ret == -ECANCELED) {
			mavlink_send_heartbeat(rotor->motor_mavlink,
				MAV_SYSTEM_STATUS_ACTIVE, rotor->downtime_s, 0);
			rotor_info(rotor, "cmd: upload canceled during zmodem transfer\n");
			rotor->app_mavlink->ft.state = MAV_FILE_TRANSFER_STATE_CANCELLED;
			rotor->app_mavlink->ft.active = 0;
			mavlink_send_ft_progress(rotor->app_mavlink,
				MAV_FILE_TRANSFER_STATE_CANCELLED,
				rotor->app_mavlink->ft.transferred,
				rotor->app_mavlink->ft.total_bytes,
				MAV_FT_RESULT_READY);
			goto skip_cmd;
		}
		if (ret) {
			mavlink_send_heartbeat(rotor->motor_mavlink,
				MAV_SYSTEM_STATUS_ACTIVE, rotor->downtime_s, 0);
			rotor_err(rotor, "cmd: failed to receive image files\n");
			mavlink_send_ft_progress(rotor->app_mavlink,
				MAV_FILE_TRANSFER_STATE_ERROR, 0,
				rotor->app_mavlink->ft.total_bytes,
				MAV_FT_RESULT_NO_SPACE);
			set_fsm_state(rotor, POV_STATE_ERROR);
			return;
		}

		ret = pov_ring_transfer_files(rotor, path);
		/* Ring distribution is also blocking and can legitimately
		 * outlast the normal heartbeat deadline, including expected
		 * negative tests. */
		rotor->last_recv_stator_heartbeat = basic_timer_get_ms();
		if (ret) {
			rotor_err(rotor, "cmd: failed to distribute ring files\n");
			mavlink_send_heartbeat(rotor->motor_mavlink,
				MAV_SYSTEM_STATUS_ACTIVE, rotor->downtime_s, 0);
			rotor_warn(rotor, "cmd: ring file distribution rejected (%d)\n",
				ret);
			mavlink_send_ft_progress(rotor->app_mavlink,
				MAV_FILE_TRANSFER_STATE_ERROR, 0,
				rotor->app_mavlink->ft.total_bytes,
				MAV_FT_RESULT_NO_SPACE);
			rotor->app_mavlink->ft.active = 0;
			/* CHECK_CMD is entered after the current image reaches
			 * EOF. A rejected transaction must rewind that image
			 * before display resumes, just like the normal
			 * end-of-image command path. */
			pov_image_cmd_set(rotor, MAV_IMAGE_ACTION_REPLAY, rotor->image_current, 0);
			if (pov_handle_end_of_image(rotor) < 0)
				set_fsm_state(rotor, POV_STATE_ERROR);
			else
				set_fsm_state(rotor, POV_STATE_DISPLAYING);
			return;
		}

		ret = pov_reload_images_after_update(rotor);
		if (ret < 0) {
			mavlink_send_heartbeat(rotor->motor_mavlink,
				MAV_SYSTEM_STATUS_ACTIVE, rotor->downtime_s, 0);
			rotor_err(rotor, "cmd: failed to commit received image files (%d)\n",
				ret);
			mavlink_send_ft_progress(rotor->app_mavlink,
				MAV_FILE_TRANSFER_STATE_ERROR, 0,
				rotor->app_mavlink->ft.total_bytes,
				MAV_FT_RESULT_NO_SPACE);
			set_fsm_state(rotor, POV_STATE_ERROR);
			return;
		}

		set_fsm_state(rotor, POV_STATE_DISPLAYING);
		mavlink_send_heartbeat(rotor->motor_mavlink,
			MAV_SYSTEM_STATUS_ACTIVE, rotor->downtime_s, 0);
		rotor->app_mavlink->ft.state = MAV_FILE_TRANSFER_STATE_COMPLETED;
		rotor->app_mavlink->ft.active = 0;
		mavlink_send_ft_progress(rotor->app_mavlink,
			MAV_FILE_TRANSFER_STATE_COMPLETED,
			rotor->app_mavlink->ft.transferred,
			rotor->app_mavlink->ft.total_bytes,
			MAV_FT_RESULT_READY);
		return;
	}

	case MAVLINK_MSG_ID_FILE_TRANSFER_ACK:
		rotor_info(rotor, "cmd: file transfer ack, state=%u, size=%u\n",
			rotor->app_mavlink->ft.state,
			rotor->app_mavlink->ft.total_bytes);
		break;

	case MAVLINK_MSG_ID_FILE_TRANSFER_PROGRESS:
		pr_debug("cmd: ft progress state=%d\n",
			rotor->app_mavlink->ft.state);
		/* TODO: update progress bar or LED indicator */
		break;

	case MAVLINK_MSG_ID_FILE_TRANSFER_CANCEL:
		mavlink_send_ft_progress(rotor->app_mavlink,
			MAV_FILE_TRANSFER_STATE_CANCELLED,
			rotor->app_mavlink->ft.transferred,
			rotor->app_mavlink->ft.total_bytes,
			MAV_FT_RESULT_READY);
		rotor_info(rotor, "cmd: file transfer cancelled\n");
		break;

	case MAVLINK_MSG_ID_FILE_TRANSFER_LIST: {
		mavlink_file_transfer_list_t fl;
		mavlink_msg_file_transfer_list_decode(
			&rotor->app_mavlink->rx_msg, &fl);
		rotor_info(rotor, "cmd: ft list request (filter '%s')\n",
			fl.filename[0] ? fl.filename : "<all layers>");
		/* The request's filename carries an optional layer filter */
		ret= pov_list_image_files(rotor,
			fl.filename[0] ? fl.filename : NULL);
		if (ret < 0) {
			rotor_err(rotor, "cmd: failed to list image files (%d)\n", ret);
			set_fsm_state(rotor, POV_STATE_ERROR);
			return;
		}
		break;
	}

	/* ─── Image Control ─────────────────────────────────── */
	case MAVLINK_MSG_ID_IMAGE_CONTROL: {
		u8 action = rotor->app_mavlink->image_cmd.action;

		rotor_info(rotor, "cmd: image control\n");
		rotor->app_mavlink->image_cmd.pending = 0;
		if (action > MAV_IMAGE_ACTION_PLAY_INDEX ||
		    (action == MAV_IMAGE_ACTION_PLAY_INDEX &&
		     rotor->app_mavlink->image_cmd.image_index >= rotor->image_count)) {
			rotor_warn(rotor, "cmd: invalid image action=%u index=%u\n",
				action, rotor->app_mavlink->image_cmd.image_index);
			mavlink_send_image_ack(rotor->app_mavlink, action,
				MAV_ACK_RESULT_INVALID_PARAM);
			break;
		}

		memset(ring_payload, 0, sizeof(ring_payload));
		pov_ring_payload_image(ring_payload,
			action,
			rotor->app_mavlink->image_cmd.image_index,
			POV_RING_IMAGE_APPLY_DELAY_FRAMES);
		ret = pov_ring_send_cmd_and_wait(rotor, MAV_RING_CMD_IMAGE_CONTROL,
				ring_payload, 4);
		if (ret < 0) {
			rotor_warn(rotor, "cmd: ring image control rejected (%d)\n", ret);
			rotor->app_mavlink->image_cmd.pending = 0;
			mavlink_send_image_ack(rotor->app_mavlink, action,
				MAV_ACK_RESULT_ERROR);
			break;
		}
		pov_image_cmd_set(rotor, action,
			rotor->app_mavlink->image_cmd.image_index,
			POV_RING_IMAGE_APPLY_DELAY_FRAMES);
		rotor->app_mavlink->image_cmd.pending = 0;
		mavlink_send_image_ack(rotor->app_mavlink, action, MAV_ACK_RESULT_OK);
		break;
	}

	case MAVLINK_MSG_ID_IMAGE_ACK:
		rotor_info(rotor, "cmd: image ack\n");
		/* TODO: confirm image switch completed */
		break;

	/* ─── Motor Control (relayed from stator) ──────────── */
	case MAVLINK_MSG_ID_MOTOR_CONTROL: {
		mavlink_motor_control_t mc;
		s8 direction;

		mavlink_msg_motor_control_decode(&rotor->app_mavlink->rx_msg, &mc);
		rotor_info(rotor, "cmd: motor control target=%u speed=%d\n",
			mc.target_id, mc.speed);
		if ((mc.target_id >= MAVLINK_MOTOR_COUNT && mc.target_id != 255) ||
		    mc.direction > MAV_MOTOR_DIRECTION_REVERSE) {
			mavlink_send_motor_ack(rotor->app_mavlink,
				mc.target_id, MAV_ACK_RESULT_INVALID_PARAM);
			break;
		}

		if (mc.direction == MAV_MOTOR_DIRECTION_REVERSE)
			direction = MAVLINK_DIRECTION_REVERSE;
		else if (mc.direction == MAV_MOTOR_DIRECTION_FORWARD)
			direction = MAVLINK_DIRECTION_FORWARD;
		else
			direction = MAVLINK_DIRECTION_STOP;

		ret = mavlink_send_motor_control(rotor->motor_mavlink,
			mc.target_id, mc.speed, direction, 1000);
		if (ret < 0) {
			rotor_err(rotor, "cmd: motor control relay failed (%d)\n", ret);
			mavlink_send_motor_ack(rotor->app_mavlink,
				mc.target_id, MAV_ACK_RESULT_ERROR);
			break;
		}

		rotor->last_recv_stator_heartbeat = basic_timer_get_ms();
		mavlink_send_motor_ack(rotor->app_mavlink,
			mc.target_id == 255 ? 0 : mc.target_id, MAV_ACK_RESULT_OK);
		break;
	}

	case MAVLINK_MSG_ID_MOTOR_STATUS_QUERY: {
		ret = mavlink_query_motor_status(rotor->motor_mavlink, 0, 500);
		if (ret < 0) {
			return;
		}
		rotor->last_recv_stator_heartbeat = basic_timer_get_ms();

		mavlink_send_motor_status(rotor->app_mavlink, 0,
			rotor->motor_mavlink->motors[0].current_speed,
			rotor->motor_mavlink->motors[0].direction,
			rotor->motor_mavlink->motors[0].status);
		break;
	}

	case MAVLINK_MSG_ID_MOTOR_STATUS: {
		mavlink_motor_status_t ms;
		mavlink_msg_motor_status_decode(
			&rotor->app_mavlink->rx_msg, &ms);
		rotor_info(rotor, "cmd: motor status id=%u speed=%d\n",
			ms.motor_id, ms.current_speed);
		break;
	}

	case MAVLINK_MSG_ID_MOTOR_ACK:
		pr_debug("cmd: motor ack\n");
		break;

	/* ─── Bootloader ──────────────────────────────────── */
	case MAVLINK_MSG_ID_BOOTLOADER_UPDATE_CMD: {
		rotor_info(rotor, "cmd: bootloader update - cmd=%d size=%u name=%s\n",
			rotor->app_mavlink->bl.command,
			rotor->app_mavlink->bl.firmware_size,
			rotor->app_mavlink->bl.filename);

		if (rotor->app_mavlink->bl.command == MAV_BL_CMD_PREPARE_STATOR_UPDATE) {
			set_fsm_state(rotor, POV_STATE_STATOR_UPDATE);
			return;
		}

		if (rotor->app_mavlink->bl.command != MAV_BL_CMD_PREPARE_UPDATE)
			break;

		set_fsm_state(rotor, POV_STATE_ROTOR_UPDATE);
		return;
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

static void pov_check_ring_cmd(struct pov_rotor *rotor)
{
	pov_ring_poll(rotor, true);

	if (pov_handle_end_of_image(rotor) < 0) {
		set_fsm_state(rotor, POV_STATE_ERROR);
	} else {
		set_fsm_state(rotor, POV_STATE_DISPLAYING);
	}
}

static void pov_bootloader_update_flow(struct pov_rotor *rotor)
{
	char path[96];
	const char *filename = rotor->app_mavlink->bl.filename;
	u32 firmware_size = rotor->app_mavlink->bl.firmware_size;
	u32 firmware_crc32 = rotor->app_mavlink->bl.firmware_crc32;
	bool stator_heartbeat_paused = false;
	int ret;

	if (filename[0] == '\0' ||
	    firmware_size == 0 ||
	    strchr(filename, '/') ||
	    strchr(filename, '\\')) {
		ret = -EINVAL;
		goto done;
	}

	ret = fatfs_create_dirs(rotor->firmware_path);
	if (ret < 0)
		goto done;

	/* The parser already queued the acceptance ACK.  Drain it before raw
	 * ZMODEM bytes arrive on the same serial link. */
	for (u32 i = 0; rotor->app_port->transmitted_size > 0 && i < 1000; i++)
		udelay(1000);
	if (rotor->app_port->transmitted_size > 0) {
		ret = -ETIMEDOUT;
		goto done;
	}

	rotor_info(rotor, "firmware: receiving %s for all arms\n", filename);
	mavlink_send_heartbeat(rotor->motor_mavlink, MAV_SYSTEM_STATUS_STANDBY,
		rotor->downtime_s, 0);
	stator_heartbeat_paused = true;
	ret = lrzsz_rz(rotor->app_port, rotor->firmware_path);
	if (ret <= 0) {
		ret = ret == -ECANCELED ? ret : -ETIMEDOUT;
		goto done;
	}
	if (ret != firmware_size) {
		ret = -EPIPE;
		goto done;
	}
	if (snprintf(path, sizeof(path), "%s/%s", rotor->firmware_path, filename) >= (int)sizeof(path)) {
		rotor_err(rotor, "firmware path too long for '%s'\n", filename);
		ret = -ENAMETOOLONG;
		goto done;
	}
	if (POV_ARM_COUNT > 1) {
		ret = pov_ring_transfer_firmware(rotor, path, filename,
			firmware_size, firmware_crc32);
		if (ret < 0)
			goto done;
	}

	/* The master remains available until every downstream arm has installed
	 * the image and returned its post-upgrade OK.  Upgrade it last. */
	ret = pov_update_firmware(rotor, rotor->app_port, path,
		firmware_size, firmware_crc32);

done:
	if (stator_heartbeat_paused)
		mavlink_send_heartbeat(rotor->motor_mavlink,
			MAV_SYSTEM_STATUS_ACTIVE, rotor->downtime_s, 0);
	if (ret < 0) {
#if KINETIS_FAKE_SIM
		pov_fake_firmware_transaction_finish(true);
#endif
		rotor_warn(rotor, "firmware update rejected (%d)\n", ret);
		mavlink_send_bootloader_ack(rotor->app_mavlink,
			MAV_BL_CMD_PREPARE_UPDATE, MAV_BL_ACK_ERROR);
	} else {
#if KINETIS_FAKE_SIM
		pov_fake_firmware_transaction_finish(false);
#endif
		mavlink_send_bootloader_ack(rotor->app_mavlink,
			MAV_BL_CMD_PREPARE_UPDATE, MAV_BL_ACK_COMPLETED);
		rotor_info(rotor, "firmware update completed across %u arms\n", POV_ARM_COUNT);
	}

	/* Firmware transfer is intentionally blocking. Motor heartbeat queued
	 * during ZMODEM still prove the stator link is alive; do not interrupt
	 * the service interval itself as a disconnection. */
	rotor->last_recv_stator_heartbeat = basic_timer_get_ms();
	pov_image_cmd_set(rotor, MAV_IMAGE_ACTION_REPLAY, rotor->image_current, 0);
	if (pov_handle_end_of_image(rotor) < 0)
		set_fsm_state(rotor, POV_STATE_ERROR);
	else
		set_fsm_state(rotor, POV_STATE_DISPLAYING);
}

static void pov_stator_update_flow(struct pov_rotor *rotor)
{
	char fw_full_path[128];
	const char *fw_paths[1];
	const char *filename;
	int ret;

	ret = fatfs_create_dirs(STATOR_UPDATE_DIR);
	if (ret) {
		rotor_err(rotor, "Failed to create '%s' (ret=%d)\n", STATOR_UPDATE_DIR, ret);
		goto err;
	}

	rotor_info(rotor, "ZMODEM: waiting for PC to send stator firmware on app_port...\n");
	ret = lrzsz_rz(rotor->app_port, STATOR_UPDATE_DIR);
	if (ret <= 0) {
		rotor_err(rotor, "ZMODEM receive stator firmware failed (%d)\n", ret);
		goto err;
	}
	rotor_info(rotor, "Stator firmware received: %d bytes\n", ret);

	filename = rotor->app_mavlink->bl.filename;

	if (filename[0] == '\0' ||
	    strchr(filename, '/') ||
	    strchr(filename, '\\')) {
		rotor_err(rotor, "invalid stator firmware name '%s'\n", filename);
		goto err;
	}
	if (snprintf(fw_full_path, sizeof(fw_full_path), "%s/%s", STATOR_UPDATE_DIR, filename) >= (int)sizeof(fw_full_path)) {
		rotor_err(rotor, "stator firmware path too long for '%s'\n", filename);
		goto err;
	}

	ret = mavlink_send_bootloader_update(rotor->motor_mavlink,
			MAV_BL_CMD_PREPARE_UPDATE,
			rotor->app_mavlink->bl.firmware_size,
			rotor->app_mavlink->bl.firmware_crc32,
			filename,
			3000);
	if (ret < 0) {
		rotor_err(rotor, "Failed to send bootloader cmd to stator: %d\n", ret);
		goto err;
	}
	rotor_info(rotor, "Sent BOOTLOADER_UPDATE_CMD to stator, waiting for reset...\n");

	/* Wait for stator to reset and bootloader to start
	 * The stator will: write update_flag on FatFS → system_reset → bootloader init
	 * Give it time for full reset cycle + bootloader init + FatFS mount */
	mdelay(3000);

	rotor_info(rotor, "ZMODEM: sending '%s' to stator on motor_port...\n", fw_full_path);
	fw_paths[0] = fw_full_path;
	ret = lrzsz_sz(rotor->motor_port, 1, fw_paths, NULL);
	if (ret <= 0) {
		rotor_err(rotor, "ZMODEM send to stator failed (%d)\n", ret);
		goto err;
	}

	set_fsm_state(rotor, POV_STATE_WAIT_MOTOR);
	return;

err:
	set_fsm_state(rotor, POV_STATE_ERROR);
}

static int pov_speed_test(struct pov_rotor *rotor)
{
	u8 *buffer = NULL;
	ktime_t k0, k1;
	u64 t0, elapsed;
	FIL fp;
	FRESULT res;
	UINT br;
	int ret = 0;
	u32 total_bytes, test_loops, total_columns, total_rows;
	u32 period_us, column_budget_us, required_kbps;
	u64 max_us = 0, min_us = 0;

	rotor_info(rotor, "---------------------- pov speed test ----------------------\n");

	period_us = 60000000u / POV_TARGET_RPM;
	column_budget_us = period_us / POV_DISPLAY_COLS;

	rotor_info(rotor, "config:\n");
	rotor_info(rotor, "  rpm:                    %u\n", POV_TARGET_RPM);
	rotor_info(rotor, "  revolution period:      %u us\n", period_us);
	rotor_info(rotor, "  columns per revolution: %u\n", POV_DISPLAY_COLS);
	rotor_info(rotor, "  column budget:          %u us\n", column_budget_us);
	rotor_info(rotor, "  spi groups:             %u\n", POV_SPI_GROUPS);
	rotor_info(rotor, "  leds per group:         %u\n", POV_LEDS_PER_GROUP);
	rotor_info(rotor, "  led driver group bytes: %u\n", POV_LED_DRIVER_GROUP_BYTES);
	rotor_info(rotor, "  led driver bytes/column:%u\n", POV_LED_DRIVER_COL_BYTES);
	rotor_info(rotor, "  prefetch columns:       %u\n", POV_PREFETCH_COLS);
	rotor_info(rotor, "  prefetch led drv bytes: %u\n", POV_PREFETCH_BYTES);
	rotor_info(rotor, "  spi clock:              %u hz (%u kb/s per channel)\n",
		POV_SPI_CLOCK_HZ, POV_SPI_CLOCK_HZ / 8000);

	rotor_info(rotor, "budget: revolution %u us = %u columns * %u us/column\n",
		period_us, POV_DISPLAY_COLS, column_budget_us);
	rotor_info(rotor, "  spi needs ~%u us/column -> leaves %u us for read\n",
		POV_SPI_SEND_TIME_US,
		column_budget_us > POV_SPI_SEND_TIME_US ?
		column_budget_us - POV_SPI_SEND_TIME_US : 0);
	rotor_info(rotor, "  minimum read speed required: %u kb/s\n",
		POV_LED_DRIVER_COL_BYTES * 1000u / column_budget_us);

	rotor_info(rotor, "--- 1. fatfs sequential read ---\n");

	buffer = kmalloc(POV_PREFETCH_BYTES, GFP_KERNEL);
	if (!buffer) {
		return -ENOMEM;
	}

	res = f_lseek(&rotor->image_file, sizeof(struct pov_image_header));
	if (res != FR_OK) {
		rotor_err(rotor, "  f_lseek failed: %d\n", res);
		ret = -EIO;
		goto out;
	}

	total_columns = rotor->image_header.columns;
	total_rows = total_columns / POV_PREFETCH_COLS;
	total_bytes = total_columns * POV_LED_DRIVER_COL_BYTES;
	rotor_info(rotor, "  image: %u columns * %u bytes = %u bytes\n",
		total_columns, POV_LED_DRIVER_COL_BYTES, total_bytes);

	k0 = ktime_get();
	for (u32 row = 0; row < total_rows; row++) {
		res = f_read(&rotor->image_file, buffer, POV_PREFETCH_BYTES, &br);
		if (res != FR_OK || br == 0) {
			rotor_err(rotor, "  f_read failed at row %u: %d (br=%u)\n",
				row, res, br);
			ret = -EIO;
			goto out;
		}
	}
	k1 = ktime_get();
	elapsed = ktime_us_delta(k1, k0);
	if (elapsed == 0) {
		elapsed = 1;
	}
	rotor_info(rotor, "  read %u kb in %llu us, %llu us/column, throughput: %u kb/s\n",
		total_bytes / 1024, elapsed, elapsed / total_columns,
		(unsigned)((u64)total_bytes * 1000000u / elapsed / 1024));

	/* prefetch latency (worst case) */
	res = f_lseek(&rotor->image_file, sizeof(struct pov_image_header));
	max_us = 0;
	if (res == FR_OK) {
		for (u32 i = 0; i < total_rows && i < 200; i++) {
			k0 = ktime_get();
			f_read(&rotor->image_file, buffer, POV_PREFETCH_BYTES, &br);
			k1 = ktime_get();
			t0 = ktime_us_delta(k1, k0);
			if (t0 > max_us) {
				max_us = t0;
			}
		}
	}
	rotor_info(rotor, "  prefetch latency (worst): %llu us (budget %u us for %u columns)\n",
		max_us, POV_PREFETCH_COLS * column_budget_us, POV_PREFETCH_COLS);
	if (max_us > (u64)POV_PREFETCH_COLS * column_budget_us) {
		rotor_warn(rotor, "   read too slow, prefetch may starve display \n");
	}

	rotor_info(rotor, "--- 2. spi dma send (4x parallel) ---\n");

	get_random_bytes(rotor->led_driver_frame, sizeof(rotor->led_driver_frame));

	test_loops = POV_DISPLAY_COLS;
	k0 = ktime_get();
	for (u32 i = 0; i < test_loops; i++) {
		rotor->prefetch_ping = 0;
		rotor->prefetch_sent_index = i % POV_PREFETCH_COLS;
		ret = pov_set_rgb(rotor);
		if (ret < 0) {
			break;
		}
	}
	k1 = ktime_get();
	elapsed = ktime_us_delta(k1, k0);
	if (elapsed == 0) {
		elapsed = 1;
	}
	rotor_info(rotor, "  burst %u columns: %llu us (average %llu us/column, %u kb/s)\n",
		test_loops, elapsed, elapsed / test_loops,
		(unsigned)((u64)test_loops * POV_LED_DRIVER_COL_BYTES * 1000000u / elapsed / 1024));
	if (elapsed / test_loops > column_budget_us)
		rotor_warn(rotor, "  spi too slow, average %llu us > budget %u us\n",
			elapsed / test_loops, column_budget_us);
	else
		rotor_info(rotor, "  spi ok: average %llu us < %u us (margin %lld us)\n",
			elapsed / test_loops, column_budget_us,
			(s64)column_budget_us - (s64)(elapsed / test_loops));

	rotor_info(rotor, "--- 3. combined pipeline (read to send) ---\n");

	res = f_lseek(&rotor->image_file, sizeof(struct pov_image_header));
	if (res != FR_OK) {
		rotor_err(rotor, "  f_lseek failed: %d\n", res);
		ret = -EACCES;
		goto out;
	}
	rotor->pending_column = sizeof(struct pov_image_header);
	rotor->prefetch_ping = 0;
	rotor->prefetch_sent_index = 0;

	k0 = ktime_get();
	ret = display_image_benchmark(rotor);
	k1 = ktime_get();
	if (ret < 0) {
		goto out;
	}
	elapsed = ktime_us_delta(k1, k0);
	rotor_info(rotor, "  %u columns in %llu us (average %llu us/column, %u kb/s)\n",
		total_columns, elapsed, elapsed / total_columns,
		(unsigned)((u64)total_columns * POV_LED_DRIVER_COL_BYTES * 1000000u /
		elapsed / 1024));
	if (elapsed > (u64)total_columns * column_budget_us)
		rotor_warn(rotor, "  pipeline can't keep up, %llu us > %u us budget\n",
			elapsed / total_columns, column_budget_us);
	else
		rotor_info(rotor, "  pipeline ok: %llu us < %u us (margin %lld us)\n",
			elapsed / total_columns, column_budget_us,
			(s64)column_budget_us - (s64)(elapsed / total_columns));

	res = f_lseek(&rotor->image_file, sizeof(struct pov_image_header));
	if (res != FR_OK) {
		rotor_err(rotor, "f_lseek back to start failed: %d\n", res);
		ret = -EACCES;
		goto out;
	}
	rotor->pending_column = sizeof(struct pov_image_header);
	rotor->prefetch_ping = 0;
	rotor->prefetch_sent_index = 0;

	rotor_info(rotor, "--- 4. angle sensor read latency ---\n");
	max_us = 0;
	min_us = ~0ULL;
	for (u32 i = 0; i < 100; i++) {
		k0 = ktime_get();
		pov_get_angle(rotor);
		k1 = ktime_get();
		t0 = ktime_us_delta(k1, k0);
		if (t0 > max_us) {
			max_us = t0;
		}
		if (t0 < min_us) {
			min_us = t0;
		}
	}
	rotor_info(rotor, "  get_angle: minimum %llu us, maximum %llu us (100 samples)\n", min_us, max_us);

	required_kbps = POV_LED_DRIVER_COL_BYTES * 1000u / column_budget_us;
	rotor_info(rotor, "--- 5. summary ---\n");
	rotor_info(rotor, "  item                              | required                     \n");
	rotor_info(rotor, "  ----------------------------------|------------------------------\n");
	rotor_info(rotor, "  read speed                        | >= %u kb/s\n", required_kbps);
	rotor_info(rotor, "  spi send                          | <= %u us/column\n", column_budget_us);
	rotor_info(rotor, "  pipeline                          | <= %u us/column\n", column_budget_us);
	rotor_info(rotor, "  angle sensor                      | < 10 us\n");
	rotor_info(rotor, "  at %u rpm (2x): budget = %u us/column (spi alone needs %u us - %s)\n",
		POV_TARGET_RPM * 2,
		60000000u / (POV_TARGET_RPM * 2) / POV_DISPLAY_COLS,
		POV_SPI_SEND_TIME_US,
		60000000u / (POV_TARGET_RPM * 2) / POV_DISPLAY_COLS >= POV_SPI_SEND_TIME_US ?
		"ok" : "impossible");

	rotor_info(rotor, "---------------------- speed test done ----------------------\n");
out:
	kfree(buffer);
	return ret;
}

int pov_rotor_display(struct pov_rotor *rotor, volatile u8 *running)
{
	u64 now;
	int ret;

#if KINETIS_FAKE_SIM
	ret = pov_create_fake_images(rotor->image_path);
	if (ret < 0) {
		goto out;
	}
#endif

	ret = pov_scan_images(rotor);
	if (ret < 0) {
		goto out;
	}

	if (rotor->image_count == 0) {
		rotor_err(rotor, "no .pov images found in '%s'\n", rotor->image_path);
		ret = -ENOENT;
		goto out;
	}

	ret = pov_load_images(rotor, rotor->image_names[0]);
	if (ret < 0) {
		goto out;
	}

	if (is_master_arm(rotor)) {
		ret = pov_speed_test(rotor);
		if (ret < 0) {
			goto out;
		}
		rotor_info(rotor, "waiting for motor connection\n");
	} else {
		rotor_info(rotor, "arm %u/%u waiting for speed stable\n", rotor->arm_id, POV_ARM_COUNT);
		set_fsm_state(rotor, POV_STATE_WAIT_SPEED);
	}

	while (!running || *running) {
#if KINETIS_FAKE_SIM
		if (rotor->forced_offline) {
			rotor->paused = 1;
			usleep(1000);
			continue;
		}
		rotor->paused = 0;
#endif
		switch (rotor->state) {
		case POV_STATE_WAIT_MOTOR:
			if (is_master_arm(rotor)) {
				waiting_for_motor_connection(rotor);
			} else {
				set_fsm_state(rotor, POV_STATE_WAIT_SPEED);
			}
			break;

		case POV_STATE_WAIT_PHONE:
			if (is_master_arm(rotor)) {
				waiting_for_phone_connection(rotor);
			} else {
				set_fsm_state(rotor, POV_STATE_WAIT_SPEED);
			}
			break;

		case POV_STATE_SEND_SPEED:
			if (is_master_arm(rotor)) {
				ret = set_motor_speed(rotor, POV_TARGET_RPM);
				if (ret == -EAGAIN) {
					rotor_warn(rotor, "motor speed not stable yet, retrying...\n");
				} else if (ret < 0) {
					rotor_err(rotor, "set_motor_speed failed (%d), retrying...\n", ret);
				}
			} else {
				set_fsm_state(rotor, POV_STATE_WAIT_SPEED);
			}
			break;

		case POV_STATE_DISPLAYING:
			display_loop(rotor);
			break;

		case POV_STATE_CHECK_CMD:
			if (is_master_arm(rotor)) {
				pov_check_phone_cmd(rotor);
			} else {
				pov_check_ring_cmd(rotor);
			}
			break;

		case POV_STATE_WAIT_SPEED:
			if (wait_speed_stable(rotor) == 0) {
				set_fsm_state(rotor, POV_STATE_DISPLAYING);
			}
			break;

		case POV_STATE_CHECK_RING:
			pov_check_ring_cmd(rotor);
			break;

		case POV_STATE_STATOR_UPDATE:
			if (is_master_arm(rotor)) {
				pov_stator_update_flow(rotor);
			} else {
				set_fsm_state(rotor, POV_STATE_ERROR);
			}
			break;
		case POV_STATE_ROTOR_UPDATE:
			if (is_master_arm(rotor)) {
				pov_bootloader_update_flow(rotor);
			} else {
				set_fsm_state(rotor, POV_STATE_ERROR);
			}
			break;

		case POV_STATE_ERROR:
			error_occurred(rotor);
			break;

		default:
			if (is_slave_arm(rotor)) {
				set_fsm_state(rotor, POV_STATE_DISPLAYING);
			} else {
				set_fsm_state(rotor, POV_STATE_WAIT_MOTOR);
			}
			break;
		}

		if (is_master_arm(rotor)) {
#if POV_USE_NRF24L01
			nrf24l01_port_poll(rotor->motor_port);
#endif
		}

		pov_send_stator_heartbeat_if_due(rotor);
	}

out:
	return ret;
}
