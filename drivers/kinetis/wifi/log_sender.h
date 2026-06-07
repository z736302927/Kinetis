/**
 * @file log_sender.h
 * @brief TCP Log Sender — streams board printf/pr_info logs over WiFi
 *
 * Hooks into the Kinetis printk subsystem to capture all logs
 * into a ring buffer, then sends them over a TCP connection
 * to a PC running `nc -l <port>` or similar tool.
 */

#ifndef LOG_SENDER_H
#define LOG_SENDER_H

#include <linux/types.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ── Configuration constants (override in config.h) ────── */

#ifndef WIFI_LOG_BUFFER_SIZE
#define WIFI_LOG_BUFFER_SIZE      2048
#endif

/* ── Device ────────────────────────────────────────────── */

struct log_sender {
	u8   enabled;
	u8   connected;
	u32  host_ip;           /* Network byte order */
	u16  host_port;
	int  socket_fd;

	/* Ring buffer */
	char  ring[WIFI_LOG_BUFFER_SIZE];
	u32   wr_idx;
	u32   rd_idx;
	u32   overflow_count;
};

/* ── API ───────────────────────────────────────────────── */

/**
 * @brief Initialize log sender with target host:port.
 *
 * @param host     Target IP string (e.g. "192.168.1.100")
 * @param port     Target TCP port
 * @return         Device pointer, or NULL on failure
 */
struct log_sender *log_sender_init(const char *host, u16 port);

/**
 * @brief Enable / disable log streaming
 */
void log_sender_set_enabled(struct log_sender *ls, u8 enabled);

/**
 * @brief Write a log line into the ring buffer.
 *
 * Called from the printk hook — ISR-safe (no blocking).
 * If not connected, data accumulates in the ring buffer
 * and will be flushed when the connection is re-established.
 *
 * @param ls    Log sender device
 * @param data  Log string (null-terminated or with length)
 * @param len   Length of data
 */
void log_sender_write(struct log_sender *ls, const char *data, u32 len);

/**
 * @brief Poll — maintain TCP connection and flush ring buffer.
 *
 * Call periodically from wifi_manager_poll().
 * If not connected, attempts to reconnect.
 *
 * @param ls  Log sender device
 */
void log_sender_poll(struct log_sender *ls);

/**
 * @brief Shut down and free log sender.
 */
void log_sender_deinit(struct log_sender *ls);

#ifdef __cplusplus
}
#endif

#endif /* LOG_SENDER_H */
