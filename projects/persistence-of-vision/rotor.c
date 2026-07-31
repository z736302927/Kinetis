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

#include "lrzsz/zmodem.h"

#include "blend.h"
#include "rotor.h"
#include "interface.h"

int pov_spi_multi_transmit(struct pov_rotor *rotor);

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

int pov_scan_images(struct pov_rotor *pov)
{
	DIR dir;
	FILINFO fno;
	FRESULT res;
	char full_path[128];
	int ret = 0;

	pov->image_count = 0;

	snprintf(full_path, sizeof(full_path), "%s/%s", POV_IMAGE_PATH, "blend");
	res = f_opendir(&dir, full_path);
	if (res != FR_OK) {
		pr_err("failed to open '%s': %d\n", full_path, res);
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

	pr_info("found %d images in %s\n", pov->image_count, full_path);

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

int pov_load_images(struct pov_rotor *pov, const char *filename)
{
	FRESULT res;
	UINT br;
	char file_path[512];
	int ret;

	snprintf(file_path, sizeof(file_path), "%s/%s/%s", POV_IMAGE_PATH, "blend", filename);
	res = f_open(&pov->image_file, file_path, FA_READ);
	if (res != FR_OK) {
		pr_err("failed to open '%s': %d\n", filename, res);
		return -EIO;
	}

	res = f_read(&pov->image_file, &pov->image_header, sizeof(pov->image_header), &br);
	if (res != FR_OK || br != sizeof(pov->image_header)) {
		pr_err("failed to read header: %d(got %u)\n", res, br);
		ret = -EIO;
		goto file_err;
	}
	pov->pending_column += br;

	ret = pov_validate_display_image(&pov->image_file, &pov->image_header, filename);
	if (ret < 0) {
		goto file_err;
	}

	pov->images_open = 1;
	pr_info("loaded '%s' (%d cols, %d groups), stride=%u\n",
		filename, pov->image_header.columns, pov->image_header.spi_groups, pov->image_header.bytes_per_col);
	return 0;

file_err:
	f_close(&pov->image_file);
	return ret;
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
		"WAIT_MOTOR", "WAIT_PHONE", "SEND_SPEED", "DISPLAYING", "CHECK_CMD",
		"STATOR_UPDATE", "ERROR"
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

static void pov_free_image_names(struct pov_rotor *rotor)
{
	for (u8 i = 0; i < rotor->image_count; i++) {
		kfree(rotor->image_names[i]);
		rotor->image_names[i] = NULL;
	}
	rotor->image_count = 0;
}

static int pov_reload_images_after_update(struct pov_rotor *rotor)
{
	int ret;

	if (rotor->images_open) {
		pov_close_image(rotor);
	}

	pov_free_image_names(rotor);

	ret = pov_compose_layers(POV_IMAGE_PATH);
	if (ret < 0) {
		return ret;
	}
	ret = pov_scan_images(rotor);
	if (ret < 0) {
		return ret;
	}

	if (rotor->image_count == 0) {
		pr_warn("no .pov images found after update\n");
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

static int pov_ring_build_layer_file_list(const char *layer,
	const char ***files_out,
	int *count_out)
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

	snprintf(layer_dir, sizeof(layer_dir), "%s/%s", POV_IMAGE_PATH, layer);
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

static int pov_ring_send_layer_files(struct mavlink_device *mav,
	const char *layer)
{
	const char **files;
	int count;
	int ret;

	if (!mav) {
		return -ENODEV;
	}

	ret = pov_ring_build_layer_file_list(layer, &files, &count);
	if (ret < 0) {
		return ret;
	}

	pr_info("ring: sending %d file(s) for %s via %s\n", count, layer, MD_NAME(mav));
	ret = lrzsz_sz(mav->serial, count, files);

	pov_ring_free_file_list(files, count);

	return ret > 0 ? 0 : -ETIMEDOUT;
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

	snprintf(dest_dir, sizeof(dest_dir), "%s/%s", POV_IMAGE_PATH, layer);
	ret = fatfs_create_dirs(dest_dir);
	if (ret < 0) {
		return ret;
	}

	pr_info("ring: receiving layer files into %s via %s", dest_dir, MD_NAME(mav));
	ret = lrzsz_rz(mav->serial, dest_dir);
	if (ret <= 0) {
		return -ETIMEDOUT;
	}

	return pov_reload_images_after_update(rotor);
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
		return MAV_RING_ERR_BAD_PARAM;
	case -EOPNOTSUPP:
		return MAV_RING_ERR_NOT_SUPPORTED;
	case -ENODEV:
		return MAV_RING_ERR_LINK_TIMEOUT;
	case -ETIMEDOUT:
		return MAV_RING_ERR_ZM_TIMEOUT;
	default:
		return MAV_RING_ERR_FILE;
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
			pr_err("ring: task %u failed at node %u code=%u link=%u->%u\n",
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
			if (ack.result == MAV_RING_ERROR_RESULT) {
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
	case MAV_RING_CMD_IMAGE:
		ret = pov_ring_decode_image_payload(cmd->payload, cmd->payload_len,
				&action, &image_index, &delay_frames);
		if (ret < 0) {
			return ret;
		}
		pov_image_cmd_set(rotor, action, image_index, delay_frames);
		return 0;
	case MAV_RING_CMD_FILE:
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
			MAV_RING_DIR_CW, cmd->payload,
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
			MAV_RING_ERR_BAD_PARAM);
		return ret;
	}
	ret = mavlink_send_ring_ack(incoming, cmd->seq, rotor->arm_id, MAV_RING_READY);
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
		return mavlink_send_ring_ack(incoming, cmd->seq, rotor->arm_id, MAV_RING_OK);
	}

	forward = pov_ring_forward_mavlink(rotor);
	if (!forward) {
		pov_ring_send_error_on_link(incoming, cmd->seq, rotor->arm_id,
			rotor->arm_id, next_arm,
			MAV_RING_ERR_LINK_TIMEOUT);
		return -ENODEV;
	}

	ret = pov_ring_forward_cmd(rotor, cmd);
	if (ret < 0) {
		pov_ring_send_error_on_link(incoming, cmd->seq, rotor->arm_id,
			rotor->arm_id, next_arm,
			MAV_RING_ERR_LINK_TIMEOUT);
		return ret;
	}

	ret = pov_ring_wait_link_ack(forward, cmd->seq, MAV_RING_READY, next_arm, POV_RING_LINK_TIMEOUT_MS, &ack, &err);
	if (ret < 0) {
		if (err.pending) {
			pov_ring_send_error_on_link(incoming, err.seq,
				err.error_code,
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

	ret = pov_ring_send_layer_files(forward, layer);
	if (ret < 0) {
		pov_ring_send_error_on_link(incoming, cmd->seq, rotor->arm_id,
			rotor->arm_id, next_arm,
			pov_ring_error_from_ret(ret));
		return ret;
	}

	ret = pov_ring_wait_link_ack(forward, cmd->seq, MAV_RING_OK, POV_ARM_COUNT - 1,
			POV_RING_FILE_TIMEOUT_MS, &ack, &err);
	if (ret < 0) {
		if (err.pending) {
			pov_ring_send_error_on_link(incoming, err.seq,
				err.error_code,
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

	return mavlink_send_ring_ack(incoming, cmd->seq, ack.node, MAV_RING_OK);
}

static void pov_ring_handle_cmd(struct pov_rotor *rotor,
	struct mavlink_device *mav)
{
	struct mavlink_ring_cmd_state cmd = mav->ring_cmd;
	struct mavlink_device *reverse;
	u8 next_arm = (rotor->arm_id + 1) % POV_ARM_COUNT;
	int ret;

	mav->ring_cmd.pending = 0;

	if (cmd.direction != MAV_RING_DIR_CW) {
		pov_ring_forward_error(rotor, cmd.seq, rotor->arm_id,
			cmd.current_arm, rotor->arm_id,
			MAV_RING_ERR_BAD_PARAM);
		return;
	}

	if (cmd.cmd == MAV_RING_CMD_FILE) {
		pov_ring_handle_file_cmd(rotor, mav, &cmd);
		return;
	}

	ret = pov_ring_execute_cmd(rotor, &cmd);
	if (ret < 0) {
		pov_ring_forward_error(rotor, cmd.seq, rotor->arm_id,
			cmd.current_arm, rotor->arm_id,
			MAV_RING_ERR_EXEC);
		return;
	}

	if (is_last_arm(rotor)) {
		reverse = pov_ring_reverse_mavlink(rotor);
		if (reverse) {
			mavlink_send_ring_ack(reverse, cmd.seq, rotor->arm_id, MAV_RING_OK);
		}
		return;
	}

	ret = pov_ring_forward_cmd(rotor, &cmd);
	if (ret < 0) {
		pov_ring_forward_error(rotor, cmd.seq, rotor->arm_id,
			rotor->arm_id, next_arm,
			MAV_RING_ERR_LINK_TIMEOUT);
	}
}

static void pov_ring_handle_ack(struct pov_rotor *rotor,
	struct mavlink_device *mav)
{
	struct mavlink_ring_ack_state ack = mav->ring_ack;
	struct mavlink_device *reverse;

	mav->ring_ack.pending = 0;

	if (ack.result == MAV_RING_READY) {
		pr_debug("ring: READY seq=%u node=%u on %s\n", ack.seq, ack.node, MD_NAME(mav));
		return;
	}

	if (is_master_arm(rotor)) {
		pr_info("ring: ACK seq=%u node=%u result=%u\n", ack.seq, ack.node, ack.result);
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
		pr_err("ring: ERROR seq=%u node=%u link=%u code=%u\n",
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
	if (!(mav->ring_cmd.pending && mav->ring_cmd.cmd != MAV_RING_CMD_FILE && !allow_blocking) &&
		serial_port_data_available(mav->serial) > 0) {
		mavlink_receive_and_process(mav, 10);
	}

	if (mav->rx_count == old_rx &&
		!mav->ring_error.pending && !mav->ring_ack.pending && !mav->ring_cmd.pending) {
		return;
	}
	if (mav->ring_error.pending) {
		pov_ring_handle_error(rotor, mav);
	}
	if (mav->ring_ack.pending) {
		pov_ring_handle_ack(rotor, mav);
	}
	if (mav->ring_cmd.pending && (allow_blocking || mav->ring_cmd.cmd != MAV_RING_CMD_FILE)) {
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
		for (u8 i = 0; i < ARRAY_SIZE(links); i++) {
			struct mavlink_device *mav = links[i];

			if (!mav) {
				continue;
			}

			if (serial_port_data_available(mav->serial) > 0) {
				mavlink_receive_and_process(mav, 10);
			}

			if (mav->ring_error.pending && mav->ring_error.seq == seq) {
				pr_err("ring: task %u failed at node %u code=%u link=%u->%u\n",
					seq, mav->ring_error.error_node, mav->ring_error.error_code,
					mav->ring_error.error_link_from, mav->ring_error.error_link_to);
				mav->ring_error.pending = 0;
				return -EIO;
			}

			if (mav->ring_ack.pending && mav->ring_ack.seq == seq) {
				u8 ack_node = mav->ring_ack.node;
				u8 result = mav->ring_ack.result;

				pr_info("ring: task %u acked by node %u result=%u\n", seq, ack_node, result);
				mav->ring_ack.pending = 0;
				if (result == MAV_RING_OK) {
					if (expected_ack_node == POV_ARM_BROADCAST || ack_node == expected_ack_node) {
						return 0;
					}

					pr_debug("ring: task %u ignoring ack from node %u, expected %u\n",
						seq, ack_node, expected_ack_node);
				}
				if (result == MAV_RING_ERROR_RESULT) {
					return -EIO;
				}
			}
		}
		mdelay(5);
	}

	pr_err("ring: task %u timed out waiting for ACK/ERROR\n", seq);
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
		pr_warn("ring: no clockwise link configured\n");
		return -ENODEV;
	}

	seq = ++rotor->ring_next_seq;
	if (seq == 0) {
		seq = ++rotor->ring_next_seq;
	}

	pov_ring_clear_result_state(rotor->ring_cw_mavlink);

	ret = mavlink_send_ring_cmd(rotor->ring_cw_mavlink, seq, cmd,
			rotor->arm_id, POV_ARM_BROADCAST, rotor->arm_id,
			MAV_RING_DIR_CW, payload, payload_len);
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
		pr_warn("ring: no clockwise link configured\n");
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

	ret = mavlink_send_ring_cmd(rotor->ring_cw_mavlink, seq, MAV_RING_CMD_FILE,
			rotor->arm_id, POV_ARM_BROADCAST, rotor->arm_id,
			MAV_RING_DIR_CW, payload, payload_len);
	if (ret < 0) {
		return ret;
	}

	ret = pov_ring_wait_link_ack(rotor->ring_cw_mavlink, seq,
			MAV_RING_READY, next_arm, POV_RING_LINK_TIMEOUT_MS, &ack, &err);
	if (ret < 0) {
		return ret;
	}

	ret = pov_ring_send_layer_files(rotor->ring_cw_mavlink, layer);
	if (ret < 0) {
		return ret;
	}

	return pov_ring_wait_link_ack(rotor->ring_cw_mavlink, seq,
			MAV_RING_OK, POV_ARM_COUNT - 1,
			POV_RING_FILE_TIMEOUT_MS, &ack, &err);
}

static int pov_receive_image_files(struct pov_rotor *rotor)
{
	char dest_dir[64];
	char *layer = rotor->app_mavlink->ft.path;
	int ret;

	if (!pov_layer_name_valid(layer)) {
		pr_warn("ZMODEM: invalid layer path '%s'\n", layer ? layer : "(null)");
		return -EINVAL;
	}

	snprintf(dest_dir, sizeof(dest_dir), "%s/%s", POV_IMAGE_PATH, layer);
	ret = fatfs_create_dirs(dest_dir);
	if (ret < 0) {
		return ret;
	}

	pr_info("ZMODEM: waiting for phone to send files on app_port...\n");

	/*
	 * lrzsz_rz() blocks until transfer completes.
	 * Files are saved to POV_IMAGE_PATH directory.
	 * Returns number of bytes received, or 0 on error.
	 */
	ret = lrzsz_rz(rotor->app_port, dest_dir);
	if (ret <= 0) {
		pr_warn("ZMODEM: receive failed or cancelled (%d)\n", ret);
		return -EIO;
	}

	pr_info("ZMODEM: received %d bytes, rescanning images...\n", ret);

	ret = pov_reload_images_after_update(rotor);
	if (ret < 0) {
		return ret;
	}

	/* If currently displaying, keep showing; else start */
	if (rotor->state == POV_STATE_DISPLAYING || rotor->state == POV_STATE_SEND_SPEED) {
		return 0;
	}

	set_fsm_state(rotor, POV_STATE_WAIT_MOTOR);
	return 0;
}

static int pov_read_next_sector(struct pov_rotor *pov)
{
	FRESULT res;
	UINT br;
	u32 read_bytes = POV_PREFETCH_COLS * POV_LED_DRIVER_COL_BYTES;

	res = f_read(&pov->image_file, (u8 *)pov->led_driver_frame[pov->prefetch_ping], read_bytes, &br);
	if (res != FR_OK || br == 0) {
		if (res == FR_OK && br == 0) {
			return -ENOENT;    /* End of image */
		}
		pr_err("read prefetch batch failed: %d (got %u)\n", res, br);
		return -EIO;
	}
	pov->pending_column += br;
	pov->prefetch_ping ^= 1;
	pov->prefetch_sent_index = 0;

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
	u32 spin_count;
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
		if (rotor->prefetch_sent_index == POV_PREFETCH_COLS || rotor->prefetch_sent_index == 0) {
			ret = pov_read_next_sector(rotor);
			if (ret == -ENOENT) {
				ret = 0;
				break;
			} else if (ret == -EIO) {
				goto over;
			}
		}

		/* Wait for angle_sensor to advance to the next column,
		 * with timeout to prevent infinite loop on sensor failure */
		spin_count = 0;
		while (pov_get_angle(rotor) == last_angle) {
			if (++spin_count > POV_DISPLAY_COLS * POV_ANGLE_TIMEOUT_FACTOR) {
				pr_err("display: angle_sensor timeout at column %u\n", last_angle);
				ret = -ETIMEDOUT;
				goto over;
			}
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
			} else if (ret == -EIO) {
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
	char file_path[128];
	UINT br;
	FRESULT res;
	int ret;

	if (index >= rotor->image_count) {
		return -EINVAL;
	}

	if (rotor->images_open) {
		pov_close_image(rotor);
	}

	snprintf(file_path, sizeof(file_path), "%s/%s/%s", POV_IMAGE_PATH, "blend", rotor->image_names[index]);
	res = f_open(&rotor->image_file, file_path, FA_READ);
	if (res != FR_OK) {
		return -EIO;
	}

	res = f_read(&rotor->image_file, &rotor->image_header,
			sizeof(rotor->image_header), &br);
	if (res != FR_OK || br != sizeof(rotor->image_header)) {
		goto err_close;
	}

	ret = pov_validate_display_image(&rotor->image_file, &rotor->image_header, rotor->image_names[index]);
	if (ret < 0) {
		goto err_close;
	}

	rotor->images_open = 1;
	rotor->pending_column = sizeof(rotor->image_header);
	rotor->image_current = index;
	rotor->prefetch_ping = 0;
	rotor->prefetch_sent_index = 0;
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
	struct mavlink_image_cmd *cmd = &rotor->image_cmd;
	u8 action;
	FRESULT res;

	if (cmd->pending && rotor->image_cmd_delay_frames > 0) {
		rotor->image_cmd_delay_frames--;
		action = MAV_IMAGE_REPLAY;
		pr_debug("image cmd pending, delay frames: %u\n",
			rotor->image_cmd_delay_frames);
	} else {
		action = cmd->pending ? cmd->action : MAV_IMAGE_REPLAY;
		cmd->pending = 0;
		rotor->image_cmd_delay_frames = 0;
	}

	switch (action) {
	case MAV_IMAGE_REPLAY:
		res = f_lseek(&rotor->image_file, sizeof(struct pov_image_header));
		if (res != FR_OK) {
			return -EIO;
		}
		rotor->pending_column = sizeof(struct pov_image_header);
		rotor->prefetch_ping = 0;
		rotor->prefetch_sent_index = 0;
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
		rotor->prefetch_ping = 0;
		rotor->prefetch_sent_index = 0;
		return 0;
	}

	return 0;
}

static int waiting_for_motor_connection(struct pov_rotor *rotor)
{
	u32 old_rx;
	u64 now;

	rotor->motor_port->ops->enable();

	while (1) {
		now = basic_timer_get_ms();

		if (now - rotor->last_send_heartbeat_ms >= (u64)POV_HEARTBEAT_INTERVAL_MS) {
			rotor->last_send_heartbeat_ms = now;
			mavlink_send_heartbeat(rotor->motor_mavlink, MAV_SYS_ACTIVE, 0, 0);
		}

#if POV_USE_NRF24L01
		nrf24l01_port_poll(rotor->motor_port);
#endif
		if (serial_port_data_available(rotor->motor_mavlink->serial) == 0) {
			continue;
		}

		old_rx = rotor->motor_mavlink->rx_count;
		mavlink_receive_and_process(rotor->motor_mavlink, 10);

		/* Any message from a stator reporting ACTIVE = handshake success */
		if (rotor->motor_mavlink->rx_count > old_rx) {
			if (rotor->motor_mavlink->sys_status == MAV_SYS_ACTIVE) {
				pr_info("motor connected!\n");
				rotor->last_stator_heartbeat_ms = basic_timer_get_ms();
				set_fsm_state(rotor, POV_STATE_SEND_SPEED);
				return 0;
			} else if (rotor->motor_mavlink->sys_status == MAV_SYS_FAULT) {
				pr_err("motor in fault state\n");
				return -EPIPE;
			}
		}
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

	speed = pov_get_rpm(rotor);
	if (abs(speed - (s32)rpm) > 50) {
		pr_warn("angle_sensor RPM %d out of range (target %d)\n", speed, rpm);
		return -EAGAIN;
	}

	pr_info("speed confirmed, entering display mode\n");
	set_fsm_state(rotor, POV_STATE_DISPLAYING);

	rotor->app_port->ops->enable();

	return 0;
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
		rotor->motor_mavlink->sys_status == MAV_SYS_ACTIVE) {
		rotor->last_stator_heartbeat_ms = basic_timer_get_ms();
	}

	/* Check timeout */
	if (basic_timer_get_ms() - rotor->last_stator_heartbeat_ms > (u64)POV_HEARTBEAT_INTERVAL_MS * 2) {
		pr_warn("stator heartbeat timeout (%llu ms)\n",
			basic_timer_get_ms() - rotor->last_stator_heartbeat_ms);
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
			pr_err("stator disconnected during display\n");
			set_fsm_state(rotor, POV_STATE_ERROR);
			return ret;
		}
	}

	t_start = basic_timer_get_us();

	ret = display_image(rotor);
	if (ret) {
		goto err;
	}

	t_end = basic_timer_get_us() - t_start;
	pr_debug("image done: %llu us total, %llu us avg per col (%u cols)\n",
		t_end, t_end / POV_DISPLAY_COLS, POV_DISPLAY_COLS);

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
	if (is_master_arm(rotor)) {
		set_motor_speed(rotor, 0);
		set_fsm_state(rotor, POV_STATE_WAIT_MOTOR);
		rotor->motor_port->ops->disable();
		rotor->app_port->ops->disable();
	} else {
		set_fsm_state(rotor, POV_STATE_DISPLAYING);
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
	rotor->arm_id = pov_read_arm_id();
	if (rotor->arm_id >= POV_ARM_COUNT) {
		pr_err("pov: invalid arm id %d (max %d)\n", rotor->arm_id, POV_ARM_COUNT - 1);
		goto free_dev;
	}
	pr_info("pov: arm id %d, arm count %d\n", rotor->arm_id, POV_ARM_COUNT);

	if (POV_ARM_COUNT > 1) {
		rotor->ring_cw_port = serial_port_alloc(ring_cw_serial_ops, "ring-cw");
		if (!rotor->ring_cw_port) {
			goto free_dev;
		}

		rotor->ring_cw_mavlink = mavlink_init(rotor->ring_cw_port, 10, 1, "ring-cw-mavlink");
		if (!rotor->ring_cw_mavlink) {
			goto free_dev;
		}

		rotor->ring_ccw_port = serial_port_alloc(ring_ccw_serial_ops, "ring-ccw");
		if (!rotor->ring_ccw_port) {
			goto free_dev;
		}

		rotor->ring_ccw_mavlink = mavlink_init(rotor->ring_ccw_port, 10, 2, "ring-ccw-mavlink");
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
	rotor->last_stator_heartbeat_ms = basic_timer_get_ms();
	rotor->last_send_heartbeat_ms = basic_timer_get_ms();
	rotor->ring_next_seq = 0;
	if (rotor->motor_mavlink) {
		mavlink_send_heartbeat(rotor->motor_mavlink, MAV_SYS_ACTIVE, 0, 0);
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
		pr_info("cmd: file transfer '%s' (%u bytes)\n",
			rotor->app_mavlink->ft.path,
			rotor->app_mavlink->ft.total_bytes);

		rotor->app_mavlink->ft.state = MAV_FT_IDLE;
		ret = pov_receive_image_files(rotor);
		if (ret) {
			pr_err("cmd: failed to receive image files\n");
			set_fsm_state(rotor, POV_STATE_ERROR);
			return;
		}

		ret = pov_ring_transfer_files(rotor, rotor->app_mavlink->ft.path);
		if (ret) {
			pr_err("cmd: failed to distribute ring files\n");
			set_fsm_state(rotor, POV_STATE_ERROR);
			return;
		}

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

		memset(ring_payload, 0, sizeof(ring_payload));
		pov_ring_payload_image(ring_payload,
			rotor->app_mavlink->image_cmd.action,
			rotor->app_mavlink->image_cmd.image_index,
			POV_RING_IMAGE_APPLY_DELAY_FRAMES);
		ret = pov_ring_send_cmd_and_wait(rotor, MAV_RING_CMD_IMAGE,
				ring_payload, 4);
		if (ret < 0) {
			pr_err("cmd: ring image control failed (%d)\n", ret);
			set_fsm_state(rotor, POV_STATE_ERROR);
			return;
		}
		pov_image_cmd_set(rotor, rotor->app_mavlink->image_cmd.action,
			rotor->app_mavlink->image_cmd.image_index,
			POV_RING_IMAGE_APPLY_DELAY_FRAMES);
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
		int ret;

		pr_info("cmd: bootloader update - cmd=%d size=%u name=%s\n",
			rotor->app_mavlink->bl.command,
			rotor->app_mavlink->bl.firmware_size,
			rotor->app_mavlink->bl.filename);

		if (rotor->app_mavlink->bl.command == MAV_BL_CMD_PREPARE_STATOR_UPDATE) {
			set_fsm_state(rotor, POV_STATE_STATOR_UPDATE);
			return;
		}

		ret = bootloader_write_update_flag(rotor->app_mavlink->bl.firmware_size,
						   rotor->app_mavlink->bl.firmware_crc32,
						   rotor->app_mavlink->bl.filename);
		if (ret < 0)
			break;

		if (rotor->bl_ops && rotor->bl_ops->system_reset) {
			rotor->bl_ops->system_reset();
		} else {
			pr_err("No system_reset available, cannot enter bootloader!\n");
		}
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

static void pov_check_ring_cmd(struct pov_rotor *rotor)
{
	pov_ring_poll(rotor, true);

	if (pov_handle_end_of_image(rotor) < 0) {
		set_fsm_state(rotor, POV_STATE_ERROR);
	} else {
		set_fsm_state(rotor, POV_STATE_DISPLAYING);
	}
}

static void pov_stator_update_flow(struct pov_rotor *rotor)
{
	char fw_full_path[128];
	const char *fw_paths[1];
	const char *filename;
	int ret;

	ret = fatfs_create_dirs(STATOR_UPDATE_DIR);
	if (ret) {
		pr_err("Failed to create '%s' (ret=%d)\n", STATOR_UPDATE_DIR, ret);
		goto err;
	}

	pr_info("ZMODEM: waiting for PC to send stator firmware on app_port...\n");
	ret = lrzsz_rz(rotor->app_port, STATOR_UPDATE_DIR);
	if (ret <= 0) {
		pr_err("ZMODEM receive stator firmware failed (%d)\n", ret);
		goto err;
	}
	pr_info("Stator firmware received: %d bytes\n", ret);

	filename = rotor->app_mavlink->bl.filename;
	snprintf(fw_full_path, sizeof(fw_full_path), "%s/%s",
		STATOR_UPDATE_DIR, filename);

	ret = mavlink_send_bootloader_update(rotor->motor_mavlink,
			MAV_BL_CMD_PREPARE_UPDATE,
			rotor->app_mavlink->bl.firmware_size,
			rotor->app_mavlink->bl.firmware_crc32,
			filename,
			3000);
	if (ret < 0) {
		pr_err("Failed to send bootloader cmd to stator: %d\n", ret);
		goto err;
	}
	pr_info("Sent BOOTLOADER_UPDATE_CMD to stator, waiting for reset...\n");

	/* Wait for stator to reset and bootloader to start
	 * The stator will: write update_flag on FatFS → system_reset → bootloader init
	 * Give it time for full reset cycle + bootloader init + FatFS mount */
	mdelay(3000);

	pr_info("ZMODEM: sending '%s' to stator on motor_port...\n", fw_full_path);
	fw_paths[0] = fw_full_path;
	ret = lrzsz_sz(rotor->motor_port, 1, fw_paths);
	if (ret <= 0) {
		pr_err("ZMODEM send to stator failed (%d)\n", ret);
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
	u64 max_us = 0, min_us = 0, open_max_us = 0;
	u32 open_ok = 0;

	pr_info("---------------------- pov speed test ----------------------\n");

	period_us = 60000000u / POV_TARGET_RPM;
	column_budget_us = period_us / POV_DISPLAY_COLS;

	pr_info("config:\n");
	pr_info("  rpm:                    %u\n", POV_TARGET_RPM);
	pr_info("  revolution period:      %u us\n", period_us);
	pr_info("  columns per revolution: %u\n", POV_DISPLAY_COLS);
	pr_info("  column budget:          %u us\n", column_budget_us);
	pr_info("  spi groups:             %u\n", POV_SPI_GROUPS);
	pr_info("  leds per group:         %u\n", POV_LEDS_PER_GROUP);
	pr_info("  led driver group bytes: %u\n", POV_LED_DRIVER_GROUP_BYTES);
	pr_info("  led driver bytes/column:%u\n", POV_LED_DRIVER_COL_BYTES);
	pr_info("  prefetch columns:       %u\n", POV_PREFETCH_COLS);
	pr_info("  prefetch led drv bytes: %u\n", POV_PREFETCH_BYTES);
	pr_info("  spi clock:              %u hz (%u kb/s per channel)\n",
		POV_SPI_CLOCK_HZ, POV_SPI_CLOCK_HZ / 8000);

	pr_info("budget: revolution %u us = %u columns * %u us/column\n",
		period_us, POV_DISPLAY_COLS, column_budget_us);
	pr_info("  spi needs ~%u us/column -> leaves %u us for read\n",
		POV_SPI_SEND_TIME_US,
		column_budget_us > POV_SPI_SEND_TIME_US ?
		column_budget_us - POV_SPI_SEND_TIME_US : 0);
	pr_info("  minimum read speed required: %u kb/s\n",
		POV_LED_DRIVER_COL_BYTES * 1000u / column_budget_us);

	pr_info("--- 1. fatfs sequential read ---\n");

	buffer = kmalloc(POV_PREFETCH_BYTES, GFP_KERNEL);
	if (!buffer) {
		return -ENOMEM;
	}

	res = f_lseek(&rotor->image_file, sizeof(struct pov_image_header));
	if (res != FR_OK) {
		pr_err("  f_lseek failed: %d\n", res);
		ret = -EIO;
		goto out;
	}

	total_columns = rotor->image_header.columns;
	total_rows = total_columns / POV_PREFETCH_COLS;
	total_bytes = total_columns * POV_LED_DRIVER_COL_BYTES;
	pr_info("  image: %u columns * %u bytes = %u bytes\n",
		total_columns, POV_LED_DRIVER_COL_BYTES, total_bytes);

	k0 = ktime_get();
	for (u32 row = 0; row < total_rows; row++) {
		res = f_read(&rotor->image_file, buffer, POV_PREFETCH_BYTES, &br);
		if (res != FR_OK || br == 0) {
			pr_err("  f_read failed at row %u: %d (br=%u)\n",
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
	pr_info("  read %u kb in %llu us, %llu us/column, throughput: %u kb/s\n",
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
	pr_info("  prefetch latency (worst): %llu us (budget %u us for %u columns)\n",
		max_us, POV_PREFETCH_COLS * column_budget_us, POV_PREFETCH_COLS);
	if (max_us > (u64)POV_PREFETCH_COLS * column_budget_us) {
		pr_warn("   read too slow, prefetch may starve display \n");
	}

	pr_info("--- 2. spi dma send (4x parallel) ---\n");

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
	pr_info("  burst %u columns: %llu us (average %llu us/column, %u kb/s)\n",
		test_loops, elapsed, elapsed / test_loops,
		(unsigned)((u64)test_loops * POV_LED_DRIVER_COL_BYTES * 1000000u / elapsed / 1024));
	if (elapsed / test_loops > column_budget_us)
		pr_warn("  spi too slow, average %llu us > budget %u us\n",
			elapsed / test_loops, column_budget_us);
	else
		pr_info("  spi ok: average %llu us < %u us (margin %lld us)\n",
			elapsed / test_loops, column_budget_us,
			(s64)column_budget_us - (s64)(elapsed / test_loops));

	pr_info("--- 3. combined pipeline (read to send) ---\n");

	res = f_lseek(&rotor->image_file, sizeof(struct pov_image_header));
	if (res != FR_OK) {
		pr_err("  f_lseek failed: %d\n", res);
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
	pr_info("  %u columns in %llu us (average %llu us/column, %u kb/s)\n",
		total_columns, elapsed, elapsed / total_columns,
		(unsigned)((u64)total_columns * POV_LED_DRIVER_COL_BYTES * 1000000u /
		elapsed / 1024));
	if (elapsed > (u64)total_columns * column_budget_us)
		pr_warn("  pipeline can't keep up, %llu us > %u us budget\n",
			elapsed / total_columns, column_budget_us);
	else
		pr_info("  pipeline ok: %llu us < %u us (margin %lld us)\n",
			elapsed / total_columns, column_budget_us,
			(s64)column_budget_us - (s64)(elapsed / total_columns));

	res = f_lseek(&rotor->image_file, sizeof(struct pov_image_header));
	if (res != FR_OK) {
		pr_err("f_lseek back to start failed: %d\n", res);
		ret = -EACCES;
		goto out;
	}
	rotor->pending_column = sizeof(struct pov_image_header);
	rotor->prefetch_ping = 0;
	rotor->prefetch_sent_index = 0;

	pr_info("--- 4. angle sensor read latency ---\n");
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
	pr_info("  get_angle: minimum %llu us, maximum %llu us (100 samples)\n", min_us, max_us);

	required_kbps = POV_LED_DRIVER_COL_BYTES * 1000u / column_budget_us;
	pr_info("--- 5. summary ---\n");
	pr_info("  item                              | required                     \n");
	pr_info("  ----------------------------------|------------------------------\n");
	pr_info("  read speed                        | >= %u kb/s\n", required_kbps);
	pr_info("  spi send                          | <= %u us/column\n", column_budget_us);
	pr_info("  pipeline                          | <= %u us/column\n", column_budget_us);
	pr_info("  angle sensor                      | < 10 us\n");
	pr_info("  at %u rpm (2x): budget = %u us/column (spi alone needs %u us - %s)\n",
		POV_TARGET_RPM * 2,
		60000000u / (POV_TARGET_RPM * 2) / POV_DISPLAY_COLS,
		POV_SPI_SEND_TIME_US,
		60000000u / (POV_TARGET_RPM * 2) / POV_DISPLAY_COLS >= POV_SPI_SEND_TIME_US ?
		"ok" : "impossible");

	pr_info("---------------------- speed test done ----------------------\n");
out:
	kfree(buffer);
	return ret;
}

int pov_rotor_display(struct pov_rotor *rotor)
{
	u64 now;
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

	ret = pov_speed_test(rotor);
	if (ret < 0) {
		goto out;
	}

	if (is_master_arm(rotor)) {
		pr_info("waiting for motor connection\n");
	} else {
		pr_info("arm %u/%u running as display slave rotor\n", rotor->arm_id, POV_ARM_COUNT);
		set_fsm_state(rotor, POV_STATE_DISPLAYING);
	}

	while (1) {
		switch (rotor->state) {
		case POV_STATE_WAIT_MOTOR:
			if (is_master_arm(rotor)) {
				waiting_for_motor_connection(rotor);
			} else {
				set_fsm_state(rotor, POV_STATE_DISPLAYING);
			}
			break;

		case POV_STATE_WAIT_PHONE:
			if (is_master_arm(rotor)) {
				waiting_for_phone_connection(rotor);
			} else {
				set_fsm_state(rotor, POV_STATE_DISPLAYING);
			}
			break;

		case POV_STATE_SEND_SPEED:
			if (is_master_arm(rotor)) {
				ret = set_motor_speed(rotor, POV_TARGET_RPM);
				if (ret < 0) {
					pr_warn("set_motor_speed failed (%d), retrying...\n", ret);
				}
			} else {
				set_fsm_state(rotor, POV_STATE_DISPLAYING);
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
			now = basic_timer_get_ms();

			if (now - rotor->last_send_heartbeat_ms >= (u64)POV_HEARTBEAT_INTERVAL_MS) {
				rotor->last_send_heartbeat_ms = now;
				rotor->downtime_s++;
				mavlink_send_heartbeat(rotor->motor_mavlink, MAV_SYS_ACTIVE, rotor->downtime_s, 0);
			}
		}
	}

out:
	return ret;
}
