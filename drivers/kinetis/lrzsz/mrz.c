#include <linux/delay.h>
#include <linux/printk.h>
#include <linux/ctype.h>

#include "zmodem.h"

static bool
approver_cb(const char *filename, size_t size, u64 date)
{
	pr_err("Sender requests to send %s: %zu bytes\n", filename, size);
	return true;
}

static
bool tick_cb(const char *fname, long bytes_sent, long bytes_total, long last_bps, int min_left, int sec_left)
{
	static long last_sec_left = 0;
	if (last_sec_left != sec_left && sec_left != 0) {
		pr_err("%s: Bytes Received:%7ld/%7ld   BPS:%-8ld ETA %02d:%02d\n",
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
		pr_err("'%s': received\n", filename);
	} else {
		pr_err("'%s': failed to receive\n", filename);
	}
}

int
mrz(int argc, char *argv[])
{
	size_t bytes = zmodem_receive(NULL, /* use current directory */
			approver_cb, /* receive everything */
			tick_cb,
			complete_cb,
			0,
			RZSZ_FLAGS_NONE);
	pr_err("Received %zu bytes.\n", bytes);
	return 0;
}
