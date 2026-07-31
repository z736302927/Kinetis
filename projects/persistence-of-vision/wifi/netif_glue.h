
#ifndef __NETIF_GLUE_H__
#define __NETIF_GLUE_H__

#include "lwip/netif.h"
#include "wwd_constants.h"
#include "network/wwd_network_interface.h"
#include "network/wwd_buffer_interface.h"

#ifdef __cplusplus
extern "C" {
#endif

err_t ethernetif_init(struct netif *netif);

void host_network_process_ethernet_data(wiced_buffer_t buffer, wwd_interface_t interface);

void host_network_set_ethertype_filter(u16 ethertype, wwd_interface_t interface,
	wwd_network_filter_ethernet_packet_t ethernet_packet_callback, void *userdata);

#ifdef __cplusplus
}
#endif

#endif /* __NETIF_GLUE_H__ */
