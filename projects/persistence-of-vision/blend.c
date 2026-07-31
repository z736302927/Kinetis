#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/printk.h>
#include <linux/string.h>

#include <kinetis/fatfs-intf.h>
#include <kinetis/tlc5971.h>

#include "config.h"
#include "rotor.h"
#include "blend.h"

static int validate_raw_layer_file(FIL *fp,
	const struct pov_image_header *header,
	const char *path,
	int require_display_cols)
{
	FSIZE_t body_size;
	FSIZE_t expected_size;

	if (header->magic != POV_IMG_MAGIC) {
		pr_warn("blend: bad magic in %s\n", path);
		return -EINVAL;
	}

	if (header->leds_per_group != POV_LEDS_PER_GROUP ||
		header->spi_groups != POV_SPI_GROUPS ||
		(require_display_cols && header->columns != POV_DISPLAY_COLS)) {
		pr_warn("blend: geometry mismatch in %s (cols=%u leds/group=%u groups=%u)\n",
			path, header->columns, header->leds_per_group,
			header->spi_groups);
		return -EINVAL;
	}

	if (header->version != POV_IMG_VERSION && header->version != 1) {
		pr_warn("blend: unsupported raw layer version in %s: %u\n",
			path, header->version);
		return -EINVAL;
	}

	if (header->version == POV_IMG_VERSION &&
		(header->payload_format != POV_PAYLOAD_RAW_RGB ||
			header->bytes_per_col != POV_BYTES_PER_COL)) {
		pr_warn("blend: %s is not raw RGB (format=%u stride=%u)\n",
			path, header->payload_format, header->bytes_per_col);
		return -EINVAL;
	}

	if (f_size(fp) < sizeof(*header)) {
		pr_warn("blend: %s is smaller than header\n", path);
		return -EINVAL;
	}

	body_size = f_size(fp) - sizeof(*header);
	expected_size = (FSIZE_t)header->columns * header->bytes_per_col;
	if (body_size != expected_size) {
		pr_warn("blend: %s has unexpected size: %llu vs %llu\n",
			path, (unsigned long long)body_size,
			(unsigned long long)expected_size);
		return -EINVAL;
	}

	return 0;
}

static FRESULT open_background_file(const char *layer_path, FIL *fp, char *full_path, u32 path_len)
{
	FRESULT res, rd;
	FILINFO fno;
	DIR dir;
	char *ext;

	full_path[0] = '\0';

	res = f_opendir(&dir, layer_path);
	if (res != FR_OK) {
		return FR_NO_FILE;
	}

	res = FR_NO_FILE;
	while (1) {
		memset(&fno, 0, sizeof(fno));
		rd = f_readdir(&dir, &fno);
		if (rd != FR_OK || fno.fname[0] == 0) {
			break;
		}
		if (fno.fattrib & (AM_DIR | AM_SYS)) {
			continue;
		}
		ext = strrchr(fno.fname, '.');
		if (!ext || strcmp(ext, ".pov") != 0) {
			continue;
		}

		snprintf(full_path, path_len, "%s/%s", layer_path, fno.fname);
		res = f_open(fp, full_path, FA_READ);
		break;
	}
	f_closedir(&dir);

	return res;
}

static void rgb_to_rgba_col(const u8 *rgb, struct rgba_pixel *rgba)
{
	u32 px;

	for (px = 0; px < POV_LED_COUNT; px++) {
		rgba[px].red   = rgb[px * 3 + 0];
		rgba[px].green = rgb[px * 3 + 1];
		rgba[px].blue  = rgb[px * 3 + 2];
		rgba[px].alpha = 255;
	}
}

static void rgb_to_rgb_col(const struct rgba_pixel *rgba, u8 *rgb)
{
	u32 px;

	for (px = 0; px < POV_LED_COUNT; px++) {
		rgb[px * 3 + 0] = rgba[px].red;
		rgb[px * 3 + 1] = rgba[px].green;
		rgb[px * 3 + 2] = rgba[px].blue;
	}
}

static void rgba_to_led_driver_col(const struct rgba_pixel *rgba, u8 *out)
{
	u32 group, ic;

	for (group = 0; group < POV_SPI_GROUPS; group++) {
		for (ic = 0; ic < POV_LED_DRIVER_IC_PER_GROUP; ic++) {
			u32 base = group * POV_LEDS_PER_GROUP +
				ic * POV_LED_DRIVER_LEDS_PER_IC;
			u32 wire_ic = POV_LED_DRIVER_ICO_NEAR_MCU ?
				(POV_LED_DRIVER_IC_PER_GROUP - 1 - ic) : ic;
			u16 r[POV_LED_DRIVER_LEDS_PER_IC];
			u16 g[POV_LED_DRIVER_LEDS_PER_IC];
			u16 b[POV_LED_DRIVER_LEDS_PER_IC];
			u32 l;

			for (l = 0; l < POV_LED_DRIVER_LEDS_PER_IC; l++) {
				r[l] = tlc5971_rgb8_to_gs(rgba[base + l].red);
				g[l] = tlc5971_rgb8_to_gs(rgba[base + l].green);
				b[l] = tlc5971_rgb8_to_gs(rgba[base + l].blue);
			}

			u8 *ic_out = out + (group * POV_LED_DRIVER_GROUP_BYTES \
					+ wire_ic * POV_LED_DRIVER_PACKET_BYTES);
			tlc5971_pack_chip(r, g, b, ic_out);
		}
	}
}

static int read_column(FIL *fp, u8 *buf, FSIZE_t first_frame)
{
	FRESULT res;
	UINT br;

	res = f_read(fp, buf, POV_BYTES_PER_COL, &br);
	if (res != FR_OK) {
		return -EIO;
	}
	if (br == 0) {
		f_lseek(fp, first_frame);
		res = f_read(fp, buf, POV_BYTES_PER_COL, &br);
		if (res != FR_OK || br != POV_BYTES_PER_COL) {
			return -EIO;
		}
	} else if (br != POV_BYTES_PER_COL) {
		return -EIO;
	}

	return 0;
}

int pov_compose_layers(const char *path)
{
	FRESULT res;
	FIL layer_fp[4];
	FIL blend_fp;
	DIR dir0;
	FILINFO fno0;
	UINT br, bw;
	char layer_dir_path[4][64];
	char layer_file_path[4][128];
	char blend_dir_path[64];
	char blend_file_path[128];
	struct pov_image_header header;
	struct pov_image_header blend_header;
	u8 *rgb_buf;
	u8 *led_driver_buf;
	struct rgba_pixel *rgba_dst, *rgba_src;
	u32 total_cols, col, px;
	int i, ret = 0;

	memset(layer_fp, 0, sizeof(FIL) * 4);
	memset(&blend_fp, 0, sizeof(FIL));

	for (i = 0; i < 4; i++) {
		snprintf(layer_dir_path[i], sizeof(layer_dir_path[i]), "%s/layer%d", path, i);
	}
	snprintf(blend_dir_path, sizeof(blend_dir_path), "%s/blend", path);

	rgb_buf = kmalloc(POV_BYTES_PER_COL, GFP_KERNEL);
	led_driver_buf = kmalloc(POV_LED_DRIVER_GROUP_BYTES * POV_SPI_GROUPS, GFP_KERNEL);
	rgba_dst = kmalloc(POV_LED_COUNT * sizeof(struct rgba_pixel), GFP_KERNEL);
	rgba_src = kmalloc(POV_LED_COUNT * sizeof(struct rgba_pixel), GFP_KERNEL);
	if (!rgb_buf || !led_driver_buf || !rgba_dst || !rgba_src) {
		ret = -ENOMEM;
		goto out_free;
	}

	pr_info("blend: starting layer compositing in %s\n", path);

	ret = fatfs_create_dirs(blend_dir_path);
	if (ret != 0) {
		pr_err("mkdir '%s' failed\n", blend_dir_path);
		goto out_free;
	}

	/* Open each background layer 1-3, at most one will be opened. */
	for (i = 1; i < 4; i++) {
		res = open_background_file(layer_dir_path[i], &layer_fp[i], layer_file_path[i], sizeof(layer_file_path[i]));
		if (res != FR_OK) {
			continue;
		}
		res = f_read(&layer_fp[i], &header, sizeof(header), &bw);
		if (res != FR_OK || bw != sizeof(header)) {
			pr_err("blend: cannot read header in %s: %d\n",
				layer_file_path[i], res);
			f_close(&layer_fp[i]);
			continue;
		}

		if (validate_raw_layer_file(&layer_fp[i], &header, layer_file_path[i], 0) != 0) {
			f_close(&layer_fp[i]);
			continue;
		}
		pr_info("blend: layer%d background = %s\n", i, layer_file_path[i]);
	}

	res = f_opendir(&dir0, layer_dir_path[0]);
	if (res != FR_OK) {
		pr_err("blend: cannot open %s: %d\n", layer_dir_path[0], res);
		ret = -ENOENT;
		goto out_bg;
	}

	ret = 0;
	while (1) {
		char *ext;
		int layer0_opened = 0;
		int blend_opened = 0;
		int display_blended = 0;

		if (f_readdir(&dir0, &fno0) != FR_OK || fno0.fname[0] == 0) {
			break;
		}
		if (fno0.fattrib & (AM_DIR | AM_SYS)) {
			continue;
		}
		ext = strrchr(fno0.fname, '.');
		if (ext == NULL || strcmp(ext, ".pov") != 0) {
			continue;
		}

		snprintf(layer_file_path[0], sizeof(layer_file_path[0]), "%s/%s", layer_dir_path[0], fno0.fname);
		snprintf(blend_file_path, sizeof(blend_file_path), "%s/%s", blend_dir_path, fno0.fname);

		if (fatfs_find_file(blend_dir_path, fno0.fname) == 0) {
			pr_info("blend: skip existing %s\n", fno0.fname);
			continue;
		}

		res = f_open(&layer_fp[0], layer_file_path[0], FA_READ);
		if (res != FR_OK) {
			pr_err("blend: cannot open %s: %d\n", layer_file_path[0], res);
			continue;
		}

		layer0_opened = 1;
		res = f_read(&layer_fp[0], &header, sizeof(header), &bw);
		if (res != FR_OK || bw != sizeof(header)) {
			pr_err("blend: cannot read header in %s: %d(got %u)\n",
				layer_file_path[0], res, bw);
			f_close(&layer_fp[0]);
			continue;
		}

		if (validate_raw_layer_file(&layer_fp[0], &header, layer_file_path[0], 1) != 0) {
			goto close_file;
		}

		res = f_open(&blend_fp, blend_file_path, FA_CREATE_ALWAYS | FA_WRITE);
		if (res != FR_OK) {
			pr_err("blend: cannot create %s: %d\n", blend_file_path, res);
			goto close_file;
		}

		blend_opened = 1;
		display_blended = 1;
		pov_image_header_init(&blend_header, POV_PAYLOAD_LED_DRIVER, POV_LED_DRIVER_COL_BYTES, header.columns);
		res = f_write(&blend_fp, &blend_header, sizeof(blend_header), &bw);
		if (res != FR_OK || bw != sizeof(blend_header)) {
			pr_err("blend: cannot write header in %s: %d(got %u)\n", blend_file_path, res, bw);
			goto close_file;
		}

		for (i = 1; i < 4; i++) {
			if (layer_fp[i].obj.fs) {
				f_lseek(&layer_fp[i], sizeof(header));
			}
		}

		// total_cols = (f_size(&layer_fp[0]) - sizeof(struct pov_image_header)) / POV_BYTES_PER_COL;
		total_cols = header.columns;
		if (total_cols == 0 || total_cols > POV_DISPLAY_COLS * 2) {
			pr_err("blend: bogus column count %u in %s, skipping\n",
				total_cols, fno0.fname);
			goto close_file;
		}
		pr_info("blend: compositing %s (%u cols)...\n",
			fno0.fname, total_cols);
		for (col = 0; col < total_cols; col++) {
			if ((col % 360) == 0 || col == total_cols - 1) {
				pr_info("blend: %s %u/%u\n", fno0.fname, col, total_cols);
			}
			if (read_column(&layer_fp[0], rgb_buf, sizeof(header)) < 0) {
				pr_warn("blend: read_column failed at col %u in %s\n", col, layer_file_path[0]);
				goto close_file;
			}

			rgb_to_rgba_col(rgb_buf, rgba_dst);

			for (i = 1; i < 4; i++) {
				if (!layer_fp[i].obj.fs) {
					continue;
				}
				if (read_column(&layer_fp[i], rgb_buf, sizeof(header)) < 0) {
					continue;
				}
				rgb_to_rgba_col(rgb_buf, rgba_src);
				for (px = 0; px < POV_LED_COUNT; px++) {
					porter_duff_composite_pixel(&rgba_dst[px], &rgba_src[px], POV_PD_SRC_OVER);
				}
			}

			rgba_to_led_driver_col(rgba_dst, led_driver_buf);
			res = f_write(&blend_fp, led_driver_buf, POV_LED_DRIVER_COL_BYTES, &bw);
			if (res != FR_OK || bw != POV_LED_DRIVER_COL_BYTES) {
				pr_err("blend: write error at column %u in %s: %d\n", col, blend_file_path, res);
				ret = -EACCES;
				goto close_file;
			}
		}

		display_blended = 0;
		pr_info("blend: wrote %s, (%u bytes)\n", blend_file_path, f_size(&blend_fp));

close_file:
		if (layer0_opened) {
			f_close(&layer_fp[0]);
		}
		if (blend_opened) {
			f_close(&blend_fp);
		}
		if (display_blended) {
			f_unlink(blend_file_path);
		}
	}

	f_closedir(&dir0);

out_bg:
	for (i = 1; i < 4; i++) {
		if (layer_fp[i].obj.fs) {
			f_close(&layer_fp[i]);
		}
	}

out_free:
	kfree(rgb_buf);
	kfree(led_driver_buf);
	kfree(rgba_dst);
	kfree(rgba_src);

	return ret;
}