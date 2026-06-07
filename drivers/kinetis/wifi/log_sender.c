/**
 * @file log_sender.c
 * @brief TCP Log Sender implementation
 *
 * Opens a TCP connection to the configured host:port and
 * streams buffered log data.  Automatically reconnects on
 * disconnection with exponential back-off.
 */

#include <linux/kernel.h>
#include <linux/printk.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/errno.h>
#include <linux/string.h>

#include "ap6181_lwip_port.h"
#include "wiced_tcpip.h"
#include "log_sender.h"

/* ── Helpers ──────────────────────────────────────────── */

static u32 _ring_count(struct log_sender *ls)
{
	if (ls->wr_idx >= ls->rd_idx)
		return ls->wr_idx - ls->rd_idx;
	return WIFI_LOG_BUFFER_SIZE - ls->rd_idx + ls->wr_idx;
}

static u32 _ring_space(struct log_sender *ls)
{
	return WIFI_LOG_BUFFER_SIZE - _ring_count(ls) - 1;
}

static u8 _ring_empty(struct log_sender *ls)
{
	return ls->wr_idx == ls->rd_idx;
}

/* ── Init / Deinit ────────────────────────────────────── */

struct log_sender *log_sender_init(const char *host, u16 port)
{
	struct log_sender *ls;
	u32 a, b, c, d;

	ls = kzalloc(sizeof(*ls), GFP_KERNEL);
	if (!ls)
		return NULL;

	ls->enabled    = 0;
	ls->connected  = 0;
	ls->host_port  = port;
	ls->socket_fd  = -1;
	ls->wr_idx     = 0;
	ls->rd_idx     = 0;
	ls->overflow_count = 0;

	/* Parse "a.b.c.d" */
	if (sscanf(host, "%u.%u.%u.%u", &a, &b, &c, &d) == 4) {
		ls->host_ip = ((a & 0xFF) << 24) |
		              ((b & 0xFF) << 16) |
		              ((c & 0xFF) << 8)  |
		              ((d & 0xFF));
	} else {
		pr_err("log_sender: invalid host '%s'\n", host);
		kfree(ls);
		return NULL;
	}

	pr_info("log_sender: target %d.%d.%d.%d:%u\n", a, b, c, d, port);
	return ls;
}

void log_sender_deinit(struct log_sender *ls)
{
	if (!ls)
		return;
	if (ls->socket_fd >= 0)
		wiced_tcp_close(ls->socket_fd);
	kfree(ls);
}

/* ── Enable / Disable ─────────────────────────────────── */

void log_sender_set_enabled(struct log_sender *ls, u8 enabled)
{
	if (!ls)
		return;
	ls->enabled = enabled;
	if (!enabled && ls->socket_fd >= 0) {
		wiced_tcp_close(ls->socket_fd);
		ls->socket_fd  = -1;
		ls->connected  = 0;
	}
}

/* ── Write to ring buffer ─────────────────────────────── */

void log_sender_write(struct log_sender *ls, const char *data, u32 len)
{
	u32 space, i;

	if (!ls || !ls->enabled || !data || len == 0)
		return;

	space = _ring_space(ls);
	if (len > space) {
		ls->overflow_count++;
		if (space == 0)
			return;
		len = space;
	}

	for (i = 0; i < len; i++) {
		ls->ring[ls->wr_idx] = data[i];
		ls->wr_idx = (ls->wr_idx + 1) % WIFI_LOG_BUFFER_SIZE;
	}
}

/* ── Poll — connect and flush ─────────────────────────── */

void log_sender_poll(struct log_sender *ls)
{
	static u32 reconnect_backoff_ms = 1000;
	static u32 last_reconnect_attempt;
	u32 now;
	int ret;

	if (!ls || !ls->enabled)
		return;

	now = (u32)basic_timer_get_ms();

	/* ── Connect ── */
	if (!ls->connected) {
		if (now - last_reconnect_attempt < reconnect_backoff_ms)
			goto skip_connect;

		last_reconnect_attempt = now;

		ls->socket_fd = wiced_tcp_create_socket();
		if (ls->socket_fd < 0) {
			pr_debug("log_sender: socket create failed\n");
			return;
		}

		{
			wiced_ip_address_t addr;
			SET_IPV4_ADDRESS(addr, ls->host_ip);
			ret = wiced_tcp_connect(ls->socket_fd, &addr,
				ls->host_port, 2000);
		}

		if (ret != WICED_SUCCESS) {
			wiced_tcp_close(ls->socket_fd);
			ls->socket_fd = -1;
			reconnect_backoff_ms *= 2;
			if (reconnect_backoff_ms > 30000)
				reconnect_backoff_ms = 30000;
			pr_debug("log_sender: connect failed, "
				"retry in %ums\n", reconnect_backoff_ms);
			return;
		}

		ls->connected    = 1;
		reconnect_backoff_ms = 1000;
		pr_info("log_sender: connected to log host\n");

		/* Flush accumulated ring buffer immediately */
		goto flush;
	}

	/* ── Check connection alive ── */
	if (ls->socket_fd < 0) {
		ls->connected = 0;
		return;
	}

skip_connect:
	/* Don't send on every poll — batch at ~4 Hz */
	if (now % 250 >= WIFI_POLL_INTERVAL_MS)
		return;

flush:
	/* ── Flush ring buffer ── */
	{
		char send_buf[512];
		u32 avail = _ring_count(ls);
		u32 chunk;

		while (avail > 0 && ls->connected) {
			chunk = (avail > sizeof(send_buf)) ?
				sizeof(send_buf) : avail;

			/* Copy from ring to linear buffer */
			for (u32 i = 0; i < chunk; i++) {
				send_buf[i] = ls->ring[ls->rd_idx];
				ls->rd_idx = (ls->rd_idx + 1) %
					WIFI_LOG_BUFFER_SIZE;
			}

			ret = wiced_tcp_send(ls->socket_fd,
				send_buf, chunk);
			if (ret <= 0) {
				/* Connection lost */
				pr_debug("log_sender: send failed, "
					"closing\n");
				wiced_tcp_close(ls->socket_fd);
				ls->socket_fd = -1;
				ls->connected  = 0;
				return;
			}

			avail -= chunk;
		}
	}
}
