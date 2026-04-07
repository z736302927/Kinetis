#include <linux/delay.h>
#include <linux/printk.h>
#include <linux/slab.h>

#include "../../fs/fatfs/ff.h"

#include "zmodem.h"

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

int
msz(int argc, char *argv[])
{
	int n_filenames = 0;
	const char **filenames = NULL;
	FILINFO fno;
	FRESULT res;

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

	size_t bytes = zmodem_send(n_filenames, filenames,
			tick_cb,
			complete_cb,
			0,
			RZSZ_FLAGS_NONE);
	pr_err("Sent %zu bytes.\n", bytes);
	for (int i = 0; i < argc; i ++) {
		kfree(filenames[i]);
	}
	kfree(filenames);
	return 0;
}
