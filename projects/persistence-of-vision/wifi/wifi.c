#include <linux/string.h>
#include <linux/kernel.h>
#include <linux/printk.h>
#include <linux/slab.h>

#include "wwd_wifi.h"
#include "wwd_constants.h"
#include "wwd_management.h"

#include "wwd_poll.h"
#include "RTOS/wwd_rtos_interface.h"

#include "lwip/netif.h"
#include "lwip/dhcp.h"
#include "lwip/init.h"
#include "lwip/timeouts.h"
#include "netif/etharp.h"

#include "network/wwd_network_constants.h"
#include "wwd_assert.h"

#include "netif_glue.h"
#include "wifi.h"

#include "../config.h"

static void wifi_status(struct wifi_manager *wifi)
{
	char ip_str[16];

	wifi_get_ip_str(wifi, ip_str);

	pr_info("── WiFi Status ──\n");
	pr_info("  State: ");
	switch (wifi->state) {
	case WIFI_STATE_OFF:
		pr_info("OFF\n");
		break;
	case WIFI_STATE_INIT:
		pr_info("INIT\n");
		break;
	case WIFI_STATE_JOINING:
		pr_info("JOINING\n");
		break;
	case WIFI_STATE_DHCP:
		pr_info("DHCP\n");
		break;
	case WIFI_STATE_CONNECTED:
		pr_info("CONNECTED\n");
		break;
	case WIFI_STATE_ERROR:
		pr_info("ERROR\n");
		break;
	default:
		pr_info("UNKNOWN\n");
		break;
	}

	if (wifi->state == WIFI_STATE_CONNECTED) {
		pr_info("  IP:      %s\n", ip_str);
	}
}

void wifi_scan(void)
{
	wiced_scan_result_t results[10];
	u32 count = 10;
	wwd_result_t ret;
	u32 i;

	pr_info("Scanning WiFi networks...\n");

	ret = wwd_wifi_scan(WICED_SCAN_TYPE_ACTIVE, WICED_BSS_TYPE_ANY,
			NULL, NULL, NULL, NULL, results, &count);
	if (ret != WWD_SUCCESS) {
		pr_info("  Scan failed (err=%d)\n", ret);
		return;
	}

	pr_info("  Found %lu APs:\n", count);
	for (i = 0; i < count; i++) {
		pr_info("    %2lu. %-32.32s  RSSI:%3d  CH:%2u  SEC:%s\n",
			i + 1,
			(const char *)results[i].SSID,
			results[i].signal_strength,
			results[i].channel,
			results[i].security == WICED_SECURITY_OPEN          ? "Open"  :
			results[i].security == WICED_SECURITY_WPA2_AES_PSK  ? "WPA2"  :
			"Other");
	}
}

void wifi_schedule()
{
	wwd_thread_poll_all();
	sys_check_timeouts();
}

static int wifi_wait_dhcp(struct wifi_manager *wifi, u32 timeout_ms)
{
	u32 elapsed = 0;

	while (!wifi->dhcp_bound && elapsed < timeout_ms) {
		wifi_schedule();

		mdelay(10);
		elapsed += 10;
	}

	return wifi->dhcp_bound ? 0 : -ETIMEDOUT;
}

static void wifi_netif_status_cb(struct wifi_manager *wifi)
{
	if (netif_is_up(&wifi->netif) && !ip4_addr_isany_val(*netif_ip4_addr(&wifi->netif))) {
		wifi->dhcp_bound = 1;
		wifi->state = WIFI_STATE_CONNECTED;
	}
}

struct wifi_manager *wifi_init(char *ssid, char *password,
	u8 use_dhcp,
	u32 ip, u32 gateway, u32 subnet_mask)
{
	struct wifi_manager *wifi;
	wwd_result_t result;
	ip4_addr_t ipaddr, netmask, gw;
	char ip[16];
	int ret;

	wifi = kzalloc(sizeof(*wifi), GFP_KERNEL);
	if (!wifi) {
		return NULL;
	}

	wifi->state = WIFI_STATE_INIT;
	wifi->static_ip = ip;
	wifi->static_gateway = gateway;
	wifi->static_subnet_mask = subnet_mask;
	wifi->use_dhcp = use_dhcp;

	lwip_init();

	result = wwd_management_wifi_on(WICED_COUNTRY_CHINA);
	if (result != WWD_SUCCESS) {
		goto err_free;
	}

	if (use_dhcp) {
		ip4_addr_set_zero(&ipaddr);
		ip4_addr_set_zero(&netmask);
		ip4_addr_set_zero(&gw);
	} else {
		IP4_ADDR(&ipaddr,
			(ip >> 24) & 0xFF,
			(ip >> 16) & 0xFF,
			(ip >> 8) & 0xFF,
			ip & 0xFF);
		IP4_ADDR(&netmask,
			(subnet_mask >> 24) & 0xFF,
			(subnet_mask >> 16) & 0xFF,
			(subnet_mask >> 8) & 0xFF,
			subnet_mask & 0xFF);
		IP4_ADDR(&gw,
			(gateway >> 24) & 0xFF,
			(gateway >> 16) & 0xFF,
			(gateway >> 8) & 0xFF,
			gateway & 0xFF);
	}

	wifi->netif.state = WWD_STA_INTERFACE;

	if (!netif_add(&wifi->netif, &ipaddr, &netmask, &gw,
		WWD_STA_INTERFACE,
		ethernetif_init, netif_input)) {
		goto err_free;
	}

	netif_set_default(&wifi->netif);
	netif_set_status_callback(&wifi->netif,
		(netif_status_callback_fn)wifi_netif_status_cb);

	if (use_dhcp) {
		wifi->dhcp_bound = 0;
		dhcp_start(&wifi->netif);
	} else {
		netif_set_up(&wifi->netif);
		wifi->dhcp_bound = 1;
	}

	wifi->state = WIFI_STATE_JOINING;

	wifi->ssid_length = strlen(ssid);
	if (wifi->ssid_length > 32) {
		wifi->ssid_length = 32;
	}
	strncpy(wifi->ssid, ssid, wifi->ssid_length);

	wifi->password_length = strlen(password);
	if (wifi->password_length > 64) {
		wifi->password_length = 64;
	}
	strncpy(wifi->password, password, wifi->password_length);

	result = wwd_wifi_join(wifi->ssid, WICED_SECURITY_WPA2_AES_PSK,
			wifi->password, wifi->password_length, NULL, WWD_STA_INTERFACE);
	if (result != WWD_SUCCESS) {
		goto err_netif_remove;
	}

	if (use_dhcp) {
		ip4_addr_t fb_ip, fb_mask, fb_gw;

		wifi->state = WIFI_STATE_DHCP;
		netif_set_link_up(&wifi->netif);

		if (wifi_wait_dhcp(wifi, 15000) != 0) {
			IP4_ADDR(&fb_ip,
				(ip >> 24) & 0xFF,
				(ip >> 16) & 0xFF,
				(ip >> 8) & 0xFF,
				ip & 0xFF);
			IP4_ADDR(&fb_mask,
				(subnet_mask >> 24) & 0xFF,
				(subnet_mask >> 16) & 0xFF,
				(subnet_mask >> 8) & 0xFF,
				subnet_mask & 0xFF);
			IP4_ADDR(&fb_gw,
				(gateway >> 24) & 0xFF,
				(gateway >> 16) & 0xFF,
				(gateway >> 8) & 0xFF,
				gateway & 0xFF);

			netif_set_ipaddr(&wifi->netif, &fb_ip);
			netif_set_netmask(&wifi->netif, &fb_mask);
			netif_set_gw(&wifi->netif, &fb_gw);
			netif_set_up(&wifi->netif);

			wifi->state = WIFI_STATE_CONNECTED;
		}
	} else {
		netif_set_link_up(&wifi->netif);
	}

	wifi_status(wifi);

	wifi_get_ip_str(wifi, ip);
	pr_info("pov: WiFi connected, IP: %s\n", ip);

	wifi->ap = tcp_ap_alloc(WIFI_LOG_HOST, WIFI_LOG_PORT);
	if (!wifi->ap) {
		goto err_netif_remove;
	}

	ret = http_server_init();
	if (ret != 0) {
		goto err_netif_remove;
	}

	return wifi;

err_netif_remove:
	netif_remove(&wifi->netif);
err_free:
	kfree(wifi);
	return NULL;
}

void wifi_get_ip_str(struct wifi_manager *wifi, char *buf)
{
	const ip4_addr_t *ip;

	if (wifi->state != WIFI_STATE_CONNECTED) {
		buf[0] = '\0';
		return;
	}

	ip = netif_ip4_addr(&wifi->netif);
	snprintf(buf, 16, "%u.%u.%u.%u", ip4_addr1(ip), ip4_addr2(ip), ip4_addr3(ip), ip4_addr4(ip));
}
