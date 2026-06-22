#include <linux/string.h>

#include "lwip/debug.h"
#include "lwip/netif.h"
#include "lwip/igmp.h"
#include "lwip/ethip6.h"
#include "netif/etharp.h"

#include "network/wwd_network_interface.h"
#include "network/wwd_buffer_interface.h"
#include "network/wwd_network_constants.h"

#include "wwd_wifi.h"
#include "wwd_eapol.h"
#include "wwd_assert.h"
#include "wiced_constants.h"
#include "wiced_low_power.h"
#include "wiced_management.h"

#include "netif_glue.h"

#ifdef ADD_LWIP_EAPOL_SUPPORT
#define ETHTYPE_EAPOL 0x888E
#endif

#define IFNAME0 'w'
#define IFNAME1 'l'

#define MULTICAST_IP_TO_MAC(ip) {(u8)0x01,             \
		(u8)0x00,             \
		(u8)0x5e,             \
		(u8)((ip)[1] & 0x7F), \
		(u8)(ip)[2],          \
		(u8)(ip)[3]}

#if LWIP_IGMP
static err_t lwip_igmp_mac_filter(struct netif *netif, const ip4_addr_t *group, enum netif_mac_filter_action action)
{
	wiced_mac_t mac = {MULTICAST_IP_TO_MAC((u8 *)group)};

	switch (action) {
	case IGMP_ADD_MAC_FILTER:
		if (wwd_wifi_register_multicast_address(&mac) != WWD_SUCCESS)
			return ERR_VAL;
		break;
	case IGMP_DEL_MAC_FILTER:
		if (wwd_wifi_unregister_multicast_address(&mac) != WWD_SUCCESS)
			return ERR_VAL;
		break;
	default:
		return ERR_VAL;
	}
	return ERR_OK;
}
#endif

static u16 filter_ethertype;
static wwd_interface_t filter_interface;
static wwd_network_filter_ethernet_packet_t filter_ethernet_packet_cb;
static void *filter_userdata;

static void low_level_init(struct netif *netif)
{
	netif->hwaddr_len = ETHARP_HWADDR_LEN;

	if (wwd_wifi_get_mac_address((wiced_mac_t *)(netif->hwaddr), netif->state) != WWD_SUCCESS)
		return;

	netif->mtu = WICED_PAYLOAD_MTU;
	netif->flags = NETIF_FLAG_BROADCAST | NETIF_FLAG_ETHARP | NETIF_FLAG_LINK_UP;
#if LWIP_IGMP
	netif->flags |= NETIF_FLAG_IGMP;
	netif_set_igmp_mac_filter(netif, lwip_igmp_mac_filter);
#endif
}

static err_t low_level_output(struct netif *netif, struct pbuf *p)
{
	if (netif->state == WICED_ETHERNET_INTERFACE || wwd_wifi_is_ready_to_transceive(netif->state) == WWD_SUCCESS) {
		pbuf_ref(p);
		wwd_network_send_ethernet_data(p, netif->state);
		LINK_STATS_INC(link.xmit);
		return ERR_OK;
	} else {
		return ERR_INPROGRESS;
	}
}

void host_network_process_ethernet_data(wiced_buffer_t buffer,
	wwd_interface_t interface)
{
	struct eth_hdr *eth_hdr;
	struct netif *tmp_netif;
	u8 result;
	u8 tmp[12];
	u16 ethertype;

	if (buffer == NULL)
		return;

	eth_hdr = (struct eth_hdr *)buffer->payload;
	ethertype = htons(eth_hdr->type);

	if (filter_ethernet_packet_cb != NULL &&
		filter_ethertype == ethertype &&
		filter_interface == interface)
		filter_ethernet_packet_cb(buffer->payload, filter_userdata);

	if (ethertype == WICED_ETHERTYPE_8021Q) {
		memcpy(tmp, buffer->payload, 12);
		memcpy(((u8 *)buffer->payload) + 4, tmp, 12);

		buffer->payload = ((u8 *)buffer->payload) + 4;
		buffer->len = (u16)(buffer->len - 4);

		eth_hdr = (struct eth_hdr *)buffer->payload;
		ethertype = htons(eth_hdr->type);
	}

	if (WICED_DEEP_SLEEP_IS_ENABLED() && WICED_DEEP_SLEEP_SAVE_PACKETS_NUM != 0) {
		if (wiced_deep_sleep_save_packet(buffer, interface))
			return;
	}

	switch (ethertype) {
	case WICED_ETHERTYPE_IPv4:
	case WICED_ETHERTYPE_ARP:
		for (tmp_netif = netif_list; tmp_netif != NULL && tmp_netif->state != interface; tmp_netif = tmp_netif->next)
			;

		if (tmp_netif == NULL) {
			result = pbuf_free(buffer);
			LWIP_ASSERT("Failed to release packet buffer", (result != (u8)0));
			buffer = NULL;
			return;
		}

		/*
		 * NO_SYS: netif->input() replaces tcpip_input(), processing
		 * the packet inline without requiring a tcpip_thread.
		 */
		if (tmp_netif->input(buffer, tmp_netif) != ERR_OK) {
			result = pbuf_free(buffer);
			LWIP_ASSERT("Failed to release packet buffer", (result != (u8)0));
			buffer = NULL;
		}
		break;

#ifdef ADD_LWIP_EAPOL_SUPPORT
	case WICED_ETHERTYPE_EAPOL:
		wwd_eapol_receive_eapol_packet(buffer, interface);
		break;
#endif
	default:
		result = pbuf_free(buffer);
		LWIP_ASSERT("Failed to release packet buffer", (result != (u8)0));
		buffer = NULL;
		break;
	}
}

void host_network_set_ethertype_filter(u16 ethertype,
	wwd_interface_t interface,
	wwd_network_filter_ethernet_packet_t cb,
	void *userdata)
{
	filter_ethertype = ethertype;
	filter_interface = interface;
	filter_ethernet_packet_cb = cb;
	filter_userdata = userdata;
}

err_t ethernetif_init(struct netif *netif)
{
	LWIP_ASSERT("netif != NULL", (netif != NULL));

	if (netif->state > WWD_ETHERNET_INTERFACE)
		return ERR_ARG;

	NETIF_INIT_SNMP(netif, snmp_ifType_ethernet_csmacd, 100000000);

	netif->name[0] = IFNAME0;
	netif->name[1] = IFNAME1;

	netif->output = etharp_output;
#if LWIP_IPV6
	netif->output_ip6 = ethip6_output;
#endif
	netif->linkoutput = low_level_output;

	low_level_init(netif);

	return ERR_OK;
}
