#define pr_fmt(fmt) "lrz: " fmt

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

int lrzsz_rz(struct serial_port *serial, const char *directory)
{
	size_t bytes = zmodem_receive(serial, directory, /* use specified directory */
			approver_cb, /* receive everything */
			tick_cb,
			complete_cb,
			0,
			RZSZ_FLAGS_NONE);
	pr_err("Received %zu bytes.\n", bytes);
	return 0;
}

int
t_mrz(int argc, char *argv[])
{
	struct serial_port *serial;

	serial = serial_port_alloc(&fake_serial_port_ops);
	if (!serial) {
		return -ENOMEM;
	}
	serial_port_start_thread(serial, SERIAL_PORT_DF_OTHERS, NULL, NULL);

	size_t bytes = zmodem_receive(serial, NULL, /* use current directory */
			approver_cb, /* receive everything */
			tick_cb,
			complete_cb,
			0,
			RZSZ_FLAGS_NONE);
	pr_err("Received %zu bytes.\n", bytes);
	return 0;
}
