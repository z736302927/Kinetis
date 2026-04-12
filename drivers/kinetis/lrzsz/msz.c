#define pr_fmt(fmt) "lsz: " fmt

#include <linux/delay.h>
#include <linux/printk.h>
#include <linux/slab.h>
#include <linux/random.h>

#include "kinetis/fatfs-intf.h"

#include "../../fs/fatfs/ff.h"

#include "zmodem.h"

#define LRZSZ_RZ_DIRRECTORY            "0:/lrzsz/rz"
#define LRZSZ_SZ_DIRRECTORY            "0:/lrzsz/sz"

static
bool tick_cb(const char *fname, long bytes_sent, long bytes_total, long last_bps, int min_left, int sec_left)
{
	static long last_sec_left = 0;
	if (last_sec_left != sec_left && sec_left != 0) {
		pr_err("%s: Bytes Sent:%7ld/%7ld   BPS:%-8ld ETA %02d:%02d\n",
			fname, bytes_sent, bytes_total,
			last_bps, min_left, sec_left);
		last_sec_left = sec_left;
	}
	mdelay(1000);
	return true;
}

static void complete_cb(const char *filename, int result, size_t size, u64 date)
{
	if (result == RZSZ_NO_ERROR) {
		pr_err("'%s (%zu bytes)': successful send\n", filename, size);
	} else {
		pr_err("'%s': failed to send\n", filename);
	}
}

int t_msz_send_specified_file(int argc, char *argv[])
{
	struct serial_port *serial;
	int n_filenames = 0;
	const char **filenames = NULL;
	FILINFO fno;
	FRESULT res;
	int ret = 0;

	filenames = kzalloc(argc * sizeof(char *), GFP_KERNEL);

	for (int i = 1; i < argc; i++) {
		// These should be filenames of regular files
		res = f_stat(argv[i], &fno);
		if (res != FR_OK) {
			pr_err("'%s' does not exist.\n", argv[i]);
		} else {
			if (fno.fattrib & AM_DIR) {
				pr_err("'%s' is a directory.\n", argv[i]);
			} else {
				filenames[n_filenames] = kstrdup(argv[i], GFP_KERNEL);
				n_filenames++;
			}
		}
	}

	if (n_filenames == 0) {
		pr_err("No files to send.\n");
		kfree(filenames);
		return 0;
	}

	serial = serial_port_alloc(&fake_serial_port_ops);
	if (!serial) {
		ret = -ENOMEM;
		goto file_err;
	}

	size_t bytes = zmodem_send(serial, n_filenames, filenames,
			tick_cb,
			complete_cb,
			0,
			RZSZ_FLAGS_NONE);
	pr_err("Sent %zu bytes.\n", bytes);

	serial_port_free(serial);

file_err:
	for (int i = 0; i < n_filenames; i++) {
		kfree(filenames[i]);
	}
	kfree(filenames);

	return 0;
}

static void *lrzsz_rz_sim_thread(void *data)
{
	struct serial_port *serial = (struct serial_port *)data;

	lrzsz_rz(serial, LRZSZ_RZ_DIRRECTORY);

	return NULL;
}

int t_msz_send_random_file(int argc, char *argv[])
{
	struct serial_port *serial_sz, *serial_rz;
	pthread_t thread;
	int ret;
	int n_filenames = 0;
	const char **filenames_sz, **filenames_rz;
	char file_path[64];
	char file_name[32];
	u8 *file_data;
	size_t file_size;
	int num_files = 3;
	int pass_count = 0;
	int fail_count = 0;

	if (argc > 1) {
		num_files = simple_strtol(argv[1], NULL, 10);
		if (num_files < 1 || num_files > 10) {
			return -EINVAL;
		}
	}

	filenames_sz = kcalloc(num_files, sizeof(char *), GFP_KERNEL);
	if (!filenames_sz) {
		return -ENOMEM;
	}
	filenames_rz = kcalloc(num_files, sizeof(char *), GFP_KERNEL);
	if (!filenames_rz) {
		kfree(filenames_sz);
		return -ENOMEM;
	}

	file_data = kzalloc(1024, GFP_KERNEL);
	if (!file_data) {
		kfree(filenames_sz);
		kfree(filenames_rz);
		return -ENOMEM;
	}

	for (int i = 0; i < num_files; i++) {
		snprintf(file_name, sizeof(file_name), "test_file_%d.txt", i + 1);
		snprintf(file_path, sizeof(file_path), "%s/%s", LRZSZ_SZ_DIRRECTORY, file_name);

		ret = fatfs_create_file(LRZSZ_SZ_DIRRECTORY, file_name);
		if (ret) {
			goto file_err;
		}

		file_size = get_random_range(1024, 1024 * 1024);
		for (size_t j = 0; j < file_size; j++) {
			file_data[j] = get_random_u32() & 0xFF;
		}

		ret = fatfs_write_store_info(LRZSZ_SZ_DIRRECTORY, file_name, 0, file_data, file_size);
		if (ret) {
			goto file_err;
		}

		filenames_sz[i] = kstrdup(file_path, GFP_KERNEL);
		if (filenames_sz[i]) {
			n_filenames++;
			pr_info("created test file: %s (%zu bytes)\n", file_path, file_size);
		}

		snprintf(file_path, sizeof(file_path), "%s/%s", LRZSZ_RZ_DIRRECTORY, file_name);
		filenames_rz[i] = kstrdup(file_path, GFP_KERNEL);
	}

	kfree(file_data);
	file_data = NULL;

	serial_sz = serial_port_alloc(&fake_serial_port_ops);
	serial_rz = serial_port_alloc(&fake_serial_port_ops);
	serial_port_start_thread(serial_sz, SERIAL_PORT_DF_OTHERS, NULL, serial_rz);
	serial_port_start_thread(serial_rz, SERIAL_PORT_DF_OTHERS, NULL, serial_sz);

	/* Start rz receiver thread */
	ret = pthread_create(&thread, NULL, lrzsz_rz_sim_thread, serial_rz);
	if (ret != 0) {
		pr_err("failed to create rz thread: %d\n", ret);
		goto thread_err;
	}

	/* Send files */
	size_t bytes = zmodem_send(serial_sz, n_filenames, filenames_sz,
			tick_cb,
			complete_cb,
			0,
			RZSZ_FLAGS_NONE);
	pr_info("Sent %zu bytes in %d files.\n", bytes, n_filenames);

	pthread_join(thread, NULL);

	pr_info("\n=== file verification ===\n");
	for (int i = 0; i < n_filenames; i++) {
		FIL fp_src, fp_dst;
		FRESULT res;
		u8 buf_src[256], buf_dst[256];
		UINT br_src, br_dst;
		FSIZE_t size_src, size_dst;
		bool match = true;

		res = f_open(&fp_src, filenames_sz[i], FA_READ);
		if (res != FR_OK) {
			pr_err("[%d/%d] failed: cannot open %s (res=%d)\n", i + 1, n_filenames, filenames_sz[i], res);
			fail_count++;
			continue;
		}

		res = f_open(&fp_dst, filenames_rz[i], FA_READ);
		if (res != FR_OK) {
			pr_err("[%d/%d] failed: cannot open %s (res=%d)\n", i + 1, n_filenames, filenames_rz[i], res);
			f_close(&fp_src);
			fail_count++;
			continue;
		}

		/* Compare file sizes first */
		size_src = f_size(&fp_src);
		size_dst = f_size(&fp_dst);
		if (size_src != size_dst) {
			pr_err("[%d/%d] failed: %s - size mismatch (%llu vs %llu)\n",
				i + 1, n_filenames, file_name, size_src, size_dst);
			f_close(&fp_src);
			f_close(&fp_dst);
			fail_count++;
			continue;
		}

		/* Compare file content */
		while (1) {
			f_read(&fp_src, buf_src, sizeof(buf_src), &br_src);
			f_read(&fp_dst, buf_dst, sizeof(buf_dst), &br_dst);

			if (br_src != br_dst) {
				pr_err("[%d/%d] failed: %s - read size mismatch (%u vs %u)\n",
					i + 1, n_filenames, file_name, br_src, br_dst);
				match = false;
				break;
			}

			if (br_src == 0) {
				break;
			}

			if (memcmp(buf_src, buf_dst, br_src) != 0) {
				pr_err("[%d/%d] failed: %s - data mismatch\n", i + 1, n_filenames, file_name);
				match = false;
				break;
			}
		}

		f_close(&fp_src);
		f_close(&fp_dst);

		if (match) {
			pr_info("[%d/%d] PASS: %s (%llu bytes)\n", i + 1, n_filenames, file_name, size_src);
			pass_count++;
		} else {
			fail_count++;
		}
	}

	pr_info("\n=== Summary: %d passed, %d failed ===\n", pass_count, fail_count);
	ret = (fail_count > 0) ? -1 : 0;

thread_err:
	serial_port_free(serial_sz);
	serial_port_free(serial_rz);
file_err:
	for (int i = 0; i < n_filenames; i++) {
		fatfs_delete_file(NULL, filenames_sz[i]);
		fatfs_delete_file(NULL, filenames_rz[i]);
		kfree((void *)filenames_sz[i]);
		kfree((void *)filenames_rz[i]);
	}
	kfree(filenames_sz);
	kfree(filenames_rz);
	kfree(file_data);

	return ret;
}
