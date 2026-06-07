/**
 * @file http_server.h
 * @brief Lightweight HTTP File Server for Kinetis POV
 *
 * Provides HTTP GET (download), POST (upload), and directory
 * listing for files on the board's SD card (FATFS).
 * Accessible from any browser or curl on the same WiFi network.
 *
 * Usage:
 *   GET  /                 → directory listing
 *   GET  /filename         → download file (with Content-Type guessing)
 *   POST /upload?name=f    → upload file to SD card
 */

#ifndef HTTP_SERVER_H
#define HTTP_SERVER_H

#include <linux/types.h>

#ifdef __cplusplus
extern "C" {
#endif

struct http_server;

/* ── API ───────────────────────────────────────────────── */

/**
 * @brief Create and start the HTTP server on the given port.
 *
 * @param port       TCP port (usually 80)
 * @param root_path  FATFS root path (e.g. "0:/" or "0:/pov_images")
 * @return           Server handle, or NULL on failure
 */
struct http_server *http_server_init(u16 port, const char *root_path);

/**
 * @brief Poll the HTTP server — accept new connections and
 *        process pending requests.
 *
 * Call periodically from wifi_manager_poll().
 * Each call processes at most one new connection + one
 * pending request to keep latency low.
 *
 * @param srv  Server handle
 */
void http_server_poll(struct http_server *srv);

/**
 * @brief Shut down and free the HTTP server.
 */
void http_server_deinit(struct http_server *srv);

#ifdef __cplusplus
}
#endif

#endif /* HTTP_SERVER_H */
