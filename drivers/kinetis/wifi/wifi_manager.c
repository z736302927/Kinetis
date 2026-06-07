/**
 * @file wifi_manager.c
 * @brief WiFi Manager implementation — AP6181 + WICED + lwIP
 *
 * Orchestrates the WiFi connection lifecycle:
 *   OFF → INIT → SCANNING → CONNECTING → CONNECTED
 *
 * All operations are cooperative (non-blocking from the
 * perspective of the Rotor FSM).  Blocking happens only
 * during init (wiced_init) and join (wiced_join_ap),
 * both of which finish in < 30 seconds.
 */

#include <linux/kernel.h>
#include <linux/printk.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/errno.h>
#include <linux/string.h>

#include <kinetis/basic-timer.h>
#include <kinetis/tim-task.h>

#include "ap6181_wiced_rtos.h"
#include "ap6181_wiced_port.h"
#include "ap6181_lwip_port.h"
#include "wifi_manager.h"

/* WICED API headers */
#include "wiced.h"

/* ── Internal helpers ──────────────────────────────────── */

static void _state_transition(struct wifi_manager_device *dev,
	enum wifi_state new_state)
{
	if (new_state != dev->state) {
		static const char *names[] = {
			"OFF", "INIT", "SCANNING",
			"CONNECTING", "CONNECTED", "ERROR"
		};
		pr_info("wifi: %s -> %s\n",
			names[dev->state], names[new_state]);
	}
	dev->state = new_state;
}

/* ── Init ──────────────────────────────────────────────── */

struct wifi_manager_device *wifi_manager_init(const struct wifi_config *cfg)
{
	struct wifi_manager_device *dev;

	dev = kzalloc(sizeof(*dev), GFP_KERNEL);
	if (!dev)
		return NULL;

	/* Copy config */
	memcpy(&dev->config, cfg, sizeof(*cfg));
	dev->state = WIFI_STATE_OFF;

	/* ── Platform init (SDIO + GPIO) ── */
	if (wiced_platform_init() < 0) {
		pr_err("wifi: platform init failed\n");
		goto err_free;
	}

	/* ── RTOS port init ── */
	wiced_rtos_port_init();

	/* ── WICED framework init ── */
	{
		wiced_result_t ret;
		pr_info("wifi: initializing WICED framework...\n");
		ret = wiced_init();
		if (ret != WICED_SUCCESS) {
			pr_err("wifi: wiced_init() returned %d\n", ret);
			goto err_free;
		}
	}

	pr_info("wifi: manager initialized\n");
	_state_transition(dev, WIFI_STATE_INIT);

	return dev;

err_free:
	kfree(dev);
	return NULL;
}

/* ── Connect ───────────────────────────────────────────── */

int wifi_manager_connect(struct wifi_manager_device *dev)
{
	wiced_ip_address_t ip_addr;
	wiced_result_t     ret;

	if (dev->state < WIFI_STATE_INIT) {
		pr_err("wifi: not initialized\n");
		return -EINVAL;
	}

	_state_transition(dev, WIFI_STATE_CONNECTING);

	pr_info("wifi: connecting to '%s'...\n", dev->config.ssid);

	/* ── Bring up STA interface ── */
	ret = wiced_network_up(WICED_STA_INTERFACE,
		WICED_USE_EXTERNAL_DHCP_SERVER, NULL);
	if (ret != WICED_SUCCESS) {
		pr_err("wifi: network_up failed (%d)\n", ret);
		goto err;
	}

	/* ── Join AP ── */
	{
		wiced_ssid_t ssid;
		u8 security = WICED_SECURITY_WPA2_AES_PSK;

		ssid.length = strlen(dev->config.ssid);
		memcpy(ssid.value, dev->config.ssid, ssid.length);

		ret = wiced_wifi_join_ap(&ssid, security,
			(const u8 *)dev->config.password,
			strlen(dev->config.password), NULL);
		if (ret != WICED_SUCCESS) {
			pr_err("wifi: join AP failed (%d)\n", ret);
			goto err_down;
		}
	}

	pr_info("wifi: joined AP, waiting for IP...\n");

	/* ── Bring up IP layer ── */
	if (dev->config.use_dhcp) {
		pr_info("wifi: starting DHCP...\n");
		ret = wiced_ip_up(WICED_STA_INTERFACE,
			WICED_USE_EXTERNAL_DHCP_SERVER, &ip_addr);
	} else {
		wiced_ip_setting_t ip_settings;
		SET_IPV4_ADDRESS(ip_settings.ip_address,
			dev->config.static_ip);
		SET_IPV4_ADDRESS(ip_settings.gateway,
			dev->config.static_gw);
		SET_IPV4_ADDRESS(ip_settings.netmask,
			dev->config.static_netmask);

		ret = wiced_ip_up(WICED_STA_INTERFACE,
			WICED_USE_STATIC_IP, &ip_settings);
	}

	if (ret != WICED_SUCCESS && ret != WICED_PENDING) {
		pr_err("wifi: IP up failed (%d)\n", ret);
		goto err_down;
	}

	/* Extract IP */
	if (dev->config.use_dhcp) {
		u32 ip_raw = GET_IPV4_ADDRESS(ip_addr);
		dev->ip_addr[0] = (ip_raw >> 24) & 0xFF;
		dev->ip_addr[1] = (ip_raw >> 16) & 0xFF;
		dev->ip_addr[2] = (ip_raw >> 8)  & 0xFF;
		dev->ip_addr[3] = (ip_raw)       & 0xFF;
	} else {
		u32 ip_raw = dev->config.static_ip;
		dev->ip_addr[0] = (ip_raw >> 24) & 0xFF;
		dev->ip_addr[1] = (ip_raw >> 16) & 0xFF;
		dev->ip_addr[2] = (ip_raw >> 8)  & 0xFF;
		dev->ip_addr[3] = (ip_raw)       & 0xFF;
	}

	dev->connect_time_ms = (u32)basic_timer_get_ms();
	dev->retry_count = 0;

	pr_info("wifi: CONNECTED  IP=%d.%d.%d.%d\n",
		dev->ip_addr[0], dev->ip_addr[1],
		dev->ip_addr[2], dev->ip_addr[3]);

	_state_transition(dev, WIFI_STATE_CONNECTED);
	return 0;

err_down:
	wiced_network_down(WICED_STA_INTERFACE);
err:
	dev->retry_count++;
	_state_transition(dev, WIFI_STATE_ERROR);
	return -EIO;
}

/* ── Disconnect ────────────────────────────────────────── */

void wifi_manager_disconnect(struct wifi_manager_device *dev)
{
	if (!dev || dev->state < WIFI_STATE_INIT)
		return;

	pr_info("wifi: disconnecting...\n");
	wiced_network_down(WICED_STA_INTERFACE);
	_state_transition(dev, WIFI_STATE_INIT);
}

/* ── Cooperative poll ──────────────────────────────────── */

void wifi_manager_poll(struct wifi_manager_device *dev)
{
	u64 now;

	if (!dev)
		return;

	now = basic_timer_get_ms();

	/* Rate-limit polling (target: 20 Hz) */
	if (now - dev->last_poll_ms < WIFI_POLL_INTERVAL_MS)
		return;
	dev->last_poll_ms = now;

	/* ── Drive WICED RTOS & lwIP ── */
	wiced_rtos_poll();

	/* ── Sub-component processing (after WiFi connected) ── */
	if (dev->state == WIFI_STATE_CONNECTED) {
		/* HTTP server and log sender are polled here */
		(void)dev; /* Placeholder — wired up in rotor integration */
	}
}

/* ── Deinit ────────────────────────────────────────────── */

void wifi_manager_deinit(struct wifi_manager_device *dev)
{
	if (!dev)
		return;

	if (dev->state >= WIFI_STATE_INIT)
		wiced_network_down(WICED_STA_INTERFACE);

	wiced_platform_deinit();
	kfree(dev);
	pr_info("wifi: manager deinitialized\n");
}

/* ── Status queries ────────────────────────────────────── */

enum wifi_state wifi_manager_get_state(struct wifi_manager_device *dev)
{
	return dev ? dev->state : WIFI_STATE_OFF;
}

const char *wifi_manager_get_ip_str(struct wifi_manager_device *dev,
	char *buf, int size)
{
	if (!dev || !buf || size < 16)
		return NULL;

	snprintf(buf, size, "%d.%d.%d.%d",
		dev->ip_addr[0], dev->ip_addr[1],
		dev->ip_addr[2], dev->ip_addr[3]);
	return buf;
}
