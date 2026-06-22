
#ifndef __TCP_AP_H__
#define __TCP_AP_H__

#include <linux/types.h>

#ifdef __cplusplus
extern "C" {
#endif

struct tcp_ap *tcp_ap_alloc(const char *server_ip, unsigned short server_port);
void tcp_ap_free(struct tcp_ap *ap);
int tcp_ap_send_data(struct tcp_ap *ap, const void *data, unsigned int length);

#ifdef __cplusplus
}
#endif

#endif /* __TCP_AP_H__ */
