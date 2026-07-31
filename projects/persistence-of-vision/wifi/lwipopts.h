
#ifndef __LWIPOPTS_H__
#define __LWIPOPTS_H__

#include "network/wwd_network_constants.h"

#ifdef __cplusplus
extern "C" {
#endif

#define NO_SYS                                1   /* Bare-metal: no OS */
#define LWIP_NETCONN                          0   /* Disable netconn API */
#define LWIP_SOCKET                           0   /* Disable socket API */
#define LWIP_RAW                              1   /* Keep raw API */
#define LWIP_NETIF_API                        0   /* Disable netif API */
#define LWIP_TCPIP_TIMEOUT                    0   /* No tcpip thread */

#define MEM_ALIGNMENT                         4
#define MEM_LIBC_MALLOC                       0
#define MEMP_OVERFLOW_CHECK                   0
#define MEMP_SANITY_CHECK                     0

#define LWIP_TCP                              1
#define TCP_LISTEN_BACKLOG                    1
#define TCP_MSS                               (WICED_PAYLOAD_MTU - 20 - 20)
#define TCP_SND_BUF                           (6 * TCP_MSS)
#define TCP_WND                               (4 * TCP_MSS)
#define TCP_SND_QUEUELEN                      (4 * TCP_SND_BUF / TCP_MSS)
#define TCP_QUEUE_OOSEQ                       1
#define LWIP_TCP_KEEPALIVE                    1

#define LWIP_UDP                              1
#define MEMP_NUM_UDP_PCB                      8

#define MEMP_NUM_TCP_SEG                      (TCP_SND_QUEUELEN + 1)

#define PBUF_POOL_TX_SIZE                     10
#define PBUF_POOL_RX_SIZE                     7
#define PBUF_POOL_BUFSIZE                     (LWIP_MEM_ALIGN_SIZE(WICED_LINK_MTU) + LWIP_MEM_ALIGN_SIZE(sizeof(struct pbuf)) + 1)
#define PBUF_LINK_HLEN                        (WICED_PHYSICAL_HEADER)

#define MEMP_NUM_NETBUF                       (PBUF_POOL_TX_SIZE + PBUF_POOL_RX_SIZE)

#define LWIP_IPV4                             1
#define LWIP_IPV6                             0
#define IP_REASSEMBLY                         1
#define IP_REASS_MAX_PBUFS                    (PBUF_POOL_TX_SIZE - 2)
#define MEMP_NUM_REASSDATA                    (IP_REASS_MAX_PBUFS - 1)

#define LWIP_DHCP                             1
#define DHCP_DOES_ARP_CHECK                   0

#define LWIP_DNS                              1
#define DNS_TABLE_SIZE                        4

#define LWIP_ICMP                             1

#define LWIP_IGMP                             0

#define ARP_QUEUEING                          1
#define MEMP_NUM_ARP_QUEUE                    5

#define LWIP_STATS                            0
#define LWIP_STATS_DISPLAY                    0
#define LINK_STATS                            0

#define CHECKSUM_GEN_IP                       1
#define CHECKSUM_GEN_UDP                      1
#define CHECKSUM_GEN_TCP                      1
#define CHECKSUM_GEN_ICMP                     1
#define CHECKSUM_CHECK_IP                     1
#define CHECKSUM_CHECK_UDP                    1
#define CHECKSUM_CHECK_TCP                    1
#define CHECKSUM_CHECK_ICMP                   1

#define LWIP_NETIF_STATUS_CALLBACK            1
#define LWIP_NETIF_LINK_CALLBACK              1
#define LWIP_NETIF_HOSTNAME                   1
#define LWIP_NETIF_TX_SINGLE_PBUF             1

#define LWIP_COMPAT_MUTEX                     0
#define SYS_LIGHTWEIGHT_PROT                  0   /* No OS protection */

#define SO_REUSE                              1

#define LWIP_PROVIDE_ERRNO                    1

#define LWIP_TIMERS                           1
#define LWIP_TIMERS_CUSTOM                    0

#define LWIP_RANDOMIZE_INITIAL_LOCAL_PORTS    1

#define LWIP_HOOK_FILENAME                    "lwip_hooks.h"

#ifdef __cplusplus
}
#endif

#endif /* __LWIPOPTS_H__ */
