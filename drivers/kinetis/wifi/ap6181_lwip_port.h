/**
 * @file ap6181_lwip_port.h
 * @brief lwIP platform configuration for Kinetis bare-metal
 *
 * Combines cc.h, lwipopts.h, perf.h, and cpu.h into a single
 * header for the Kinetis cooperative port.
 */

#ifndef AP6181_LWIP_PORT_H
#define AP6181_LWIP_PORT_H

#ifdef __cplusplus
extern "C" {
#endif

#include <linux/printk.h>
#include <linux/random.h>

/* ── compiler abstraction (cc.h) ──────────────────────── */

typedef int sys_prot_t;

#define U16_F  "hu"
#define S16_F  "hd"
#define X16_F  "hx"
#define U32_F  "lu"
#define S32_F  "ld"
#define X32_F  "lx"
#define SZT_F  "u"

/* GCC packed struct support */
#define PACK_STRUCT_BEGIN
#define PACK_STRUCT_STRUCT  __attribute__((__packed__))
#define PACK_STRUCT_END
#define PACK_STRUCT_FIELD(x)  x

/* Diagnostic output */
#define LWIP_PLATFORM_DIAG(x)    {printk x;}

/* Assert — empty in release */
#define LWIP_PLATFORM_ASSERT(x)

/* ── CPU (cpu.h) ──────────────────────────────────────── */

#ifndef BYTE_ORDER
#define BYTE_ORDER  LITTLE_ENDIAN
#endif

/* ── Performance stubs (perf.h) ───────────────────────── */

#define PERF_START
#define PERF_STOP(x)

/* ── lwIP configuration (lwipopts.h) ──────────────────── */

#define NO_SYS                          0   /* Use OS mode for socket API */

/* Memory */
#define MEM_LIBC_MALLOC                 1
#define MEMP_MEM_MALLOC                 1
#define MEM_ALIGNMENT                   4
#define MEM_SIZE                        (32 * 1024)

/* Memory pool sizes */
#define MEMP_NUM_PBUF                   16
#define MEMP_NUM_UDP_PCB                8
#define MEMP_NUM_TCP_PCB                8
#define MEMP_NUM_TCP_PCB_LISTEN         4
#define MEMP_NUM_TCP_SEG                32
#define MEMP_NUM_NETBUF                 8
#define MEMP_NUM_NETCONN                12
#define MEMP_NUM_TCPIP_MSG_API          16
#define MEMP_NUM_TCPIP_MSG_INPKT        16

/* Protocol options */
#define LWIP_RAW                        1
#define LWIP_NETCONN                    1
#define LWIP_SOCKET                     1
#define LWIP_COMPAT_MUTEX               1
#define LWIP_SO_RCVTIMEO                1
#define LWIP_SO_SNDTIMEO                1

/* IPv4 only */
#define LWIP_IPV4                       1
#define LWIP_IPV6                       0

/* DHCP / DNS */
#define LWIP_DHCP                       1
#define LWIP_DNS                        1
#define LWIP_DNS_MAX_SERVERS            2

/* Network interface */
#define LWIP_NETIF_HOSTNAME             1
#define LWIP_NETIF_STATUS_CALLBACK      1
#define LWIP_NETIF_LINK_CALLBACK        1
#define LWIP_NETIF_TX_SINGLE_PBUF       1

/* TCP */
#define LWIP_TCP                        1
#define TCP_MSS                         1460
#define TCP_WND                         (8 * TCP_MSS)
#define TCP_SND_BUF                     (8 * TCP_MSS)
#define TCP_SND_QUEUELEN                (4 * (TCP_SND_BUF / TCP_MSS))
#define TCP_QUEUE_OOSEQ                 1
#define LWIP_TCP_KEEPALIVE              1

/* UDP */
#define LWIP_UDP                        1

/* ICMP */
#define LWIP_ICMP                       1

/* Checksum — hardware offload */
#define CHECKSUM_GEN_IP                 0
#define CHECKSUM_GEN_UDP                0
#define CHECKSUM_GEN_TCP                0
#define CHECKSUM_GEN_ICMP               0
#define CHECKSUM_CHECK_IP               0
#define CHECKSUM_CHECK_UDP              0
#define CHECKSUM_CHECK_TCP              0
#define CHECKSUM_CHECK_ICMP             0

/* Statistics */
#define LWIP_STATS                      0
#define LWIP_STATS_DISPLAY              0
#define LINK_STATS                      0

/* Debug */
#define LWIP_DEBUG                      0
#define TCPIP_DEBUG                     LWIP_DBG_OFF
#define TCP_DEBUG                       LWIP_DBG_OFF
#define DHCP_DEBUG                      LWIP_DBG_OFF

/* TCPIP thread config */
#define TCPIP_THREAD_NAME               "tcpip"
#define TCPIP_THREAD_STACKSIZE          4096
#define TCPIP_THREAD_PRIO               2
#define TCPIP_MBOX_SIZE                 32
#define DEFAULT_TCP_RECVMBOX_SIZE       8
#define DEFAULT_UDP_RECVMBOX_SIZE       8
#define DEFAULT_ACCEPTMBOX_SIZE         4
#define DEFAULT_RAW_RECVMBOX_SIZE       4

/* PBUF pools */
#define PBUF_POOL_SIZE                  8
#define PBUF_POOL_BUFSIZE               1536

/* lwIP provides errno */
#define LWIP_PROVIDE_ERRNO              1

/* Random */
#define LWIP_RAND()                     ((u32_t)get_random_u32())

/* Timing */
#define LWIP_TCPIP_TIMEOUT              1

#ifdef __cplusplus
}
#endif

#endif /* AP6181_LWIP_PORT_H */
