/**
 * @file wifi_manager.h
 * @brief WiFi Manager — AP6181 integration for Kinetis POV Rotor
 *
 * Provides a high-level API for:
 *  - WiFi initialization and connection to a home AP
 *  - DHCP IP address acquisition
 *  - Connection state tracking
 *  - Cooperative polling entry point (call from Rotor FSM)
 *  - HTTP server and log sender lifecycle management
 */

#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <linux/types.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ── State ─────────────────────────────────────────────── */

enum wifi_state {
	WIFI_STATE_OFF = 0,       /* WiFi subsystem not initialized */
	WIFI_STATE_INIT,          /* WICED + lwIP initialized */
	WIFI_STATE_SCANNING,      /* Scanning for AP */
	WIFI_STATE_CONNECTING,    /* Joining AP */
	WIFI_STATE_CONNECTED,     /* Connected, IP acquired */
	WIFI_STATE_ERROR          /* Fatal error */
};

/* ── Configuration ─────────────────────────────────────── */

struct wifi_config {
	const char *ssid;          /* AP SSID */
	const char *password;      /* AP password */
	u16         http_port;     /* HTTP server port (default 80) */
	const char *log_host;      /* Log receiver IP */
	u16         log_port;      /* Log receiver port */
	u8          use_dhcp;      /* 1 = DHCP, 0 = static IP */
	u32         static_ip;     /* Static IP (network byte order) */
	u32         static_gw;
	u32         static_netmask;
};

/* ── Device ────────────────────────────────────────────── */

struct wifi_manager_device {
	struct wifi_config config;
	enum wifi_state    state;
	u8                 ip_addr[4];     /* Current IPv4 address */
	u8                 mac_addr[6];
	u32                link_speed;     /* kbps */
	s32                rssi;           /* dBm */
	u32                connect_time_ms; /* Time of last connect */
	u32                retry_count;
	u64                last_poll_ms;

	/* Sub-components (created after WiFi is up) */
	struct http_server *http_srv;
	struct log_sender  *log_cli;
};

/* ── API ───────────────────────────────────────────────── */

/**
 * @brief Allocate and initialize the WiFi manager.
 *
 * Configures WICED, lwIP, and the platform SDIO/Gpio layer.
 * Does NOT connect to an AP — call wifi_manager_connect() for that.
 *
 * @param cfg  WiFi configuration (SSID, password, etc.)
 * @return     Device pointer on success, NULL on failure
 */
struct wifi_manager_device *wifi_manager_init(const struct wifi_config *cfg);

/**
 * @brief Initiate connection to the configured AP.
 *
 * Non-blocking — the connection proceeds asynchronously.
 * Call wifi_manager_poll() periodically to drive the state
 * machine forward.
 *
 * @param dev  WiFi manager device
 * @return     0 on immediate start, negative on error
 */
int wifi_manager_connect(struct wifi_manager_device *dev);

/**
 * @brief Disconnect from AP and shut down WiFi.
 *
 * @param dev  WiFi manager device
 */
void wifi_manager_disconnect(struct wifi_manager_device *dev);

/**
 * @brief Cooperative poll — call from Rotor FSM loop.
 *
 * Drives the WiFi state machine, WWD SDIO I/O, lwIP timers,
 * and sub-component (HTTP server / log sender) processing.
 *
 * @param dev  WiFi manager device
 */
void wifi_manager_poll(struct wifi_manager_device *dev);

/**
 * @brief Shut down WiFi and free all resources.
 *
 * @param dev  WiFi manager device (NULL-safe)
 */
void wifi_manager_deinit(struct wifi_manager_device *dev);

/**
 * @brief Get current WiFi connection state
 */
enum wifi_state wifi_manager_get_state(struct wifi_manager_device *dev);

/**
 * @brief Get IP address as a dotted-decimal string
 *
 * @param dev   WiFi manager device
 * @param buf   Output buffer (at least 16 bytes)
 * @param size  Buffer size
 * @return      buf on success, NULL on error
 */
const char *wifi_manager_get_ip_str(struct wifi_manager_device *dev,
	char *buf, int size);

#ifdef __cplusplus
}
#endif

#endif /* WIFI_MANAGER_H */
