#ifndef __WIFI_H__
#define __WIFI_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <kinetis/tim-task.h>

#include "wwd_constants.h"

#include "lwip/netif.h"

#include "tcp_ap.h"

enum wifi_state {
    WIFI_STATE_OFF = 0,
    WIFI_STATE_INIT,
    WIFI_STATE_JOINING,
    WIFI_STATE_DHCP,
    WIFI_STATE_CONNECTED,
    WIFI_STATE_ERROR,
};

struct wifi_manager {
	struct netif netif;
	enum wifi_state state;
	volatile int dhcp_bound;

    char ssid[33];
    char password[65];
	u8 ssid_length;
	u8 password_length;
    u8 use_dhcp;
    u32 static_ip;
    u32 static_gateway;
    u32 static_subnet_mask;    

    struct tcp_ap *ap;

	struct tim_task schedule_task;
};

void wifi_schedule();

struct wifi_manager *wifi_init(char *ssid, char *password,
    u8 use_dhcp,
    u32 ip, u32 gateway, u32 subnet_mask);

void wifi_get_ip_str(struct wifi_manager *wifi, char *buf);

#ifdef __cplusplus
}
#endif

#endif /* __WIFI_H__ */
