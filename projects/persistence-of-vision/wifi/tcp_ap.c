
#include <linux/string.h>
#include <linux/kernel.h>
#include <linux/printk.h>
#include <linux/slab.h>

#include "lwip/tcp.h"
#include "lwip/ip_addr.h"
#include "lwip/err.h"

#include "tcp_ap.h"

#define LOG_RING_SIZE    4096

struct tcp_ap {
	char ring[LOG_RING_SIZE];
	volatile u32 head;
	volatile u32 tail;
	volatile u8 connected;
	volatile u8 sending;
	struct tcp_pcb *pcb;
	ip4_addr_t server_ip;
	u16 server_port;
};

static void tcp_ap_send(struct tcp_ap *ap)
{
	u32 avail, length;
	err_t ret;

	if (!ap->connected || ap->sending) {
		return;
	}

	if (ap->head >= ap->tail) {
		avail = ap->head - ap->tail;
	} else {
		avail = LOG_RING_SIZE - ap->tail + ap->head;
	}

	if (avail == 0) {
		return;
	}

	length = avail;
	if (length > tcp_sndbuf(ap->pcb)) {
		length = tcp_sndbuf(ap->pcb);
	}
	if (length == 0) {
		return;
	}

	if (ap->tail + length > LOG_RING_SIZE) {
		length = LOG_RING_SIZE - ap->tail;
	}

	ret = tcp_write(ap->pcb, &ap->ring[ap->tail], length, TCP_WRITE_FLAG_COPY);
	if (ret == ERR_OK) {
		ap->sending = 1;
		tcp_output(ap->pcb);
	}
}

static err_t tcp_ap_connected(void *arg, struct tcp_pcb *pcb, err_t err)
{
	struct tcp_ap *ap = (struct tcp_ap *)arg;

	if (err != ERR_OK) {
		tcp_close(pcb);
		ap->pcb = NULL;
		ap->connected = 0;
		return err;
	}

	ap->connected = 1;
	tcp_ap_send(ap);

	return ERR_OK;
}

static err_t tcp_ap_transmit_cb(void *arg, struct tcp_pcb *pcb, u16 length)
{
	struct tcp_ap *ap = (struct tcp_ap *)arg;

	ap->sending = 0;

	ap->tail += length;
	if (ap->tail >= LOG_RING_SIZE) {
		ap->tail -= LOG_RING_SIZE;
	}

	if (ap->head != ap->tail) {
		tcp_ap_send(ap);
	}

	return ERR_OK;
}

static void tcp_ap_error_cb(void *arg, err_t err)
{
	struct tcp_ap *ap = (struct tcp_ap *)arg;

	if (ap->pcb) {
		tcp_close(ap->pcb);
		ap->pcb = NULL;
	}
	ap->connected = 0;
	ap->sending = 0;
}

static err_t tcp_ap_receive_cb(void *arg, struct tcp_pcb *pcb, struct pbuf *p, err_t err)
{
	struct tcp_ap *ap = (struct tcp_ap *)arg;

	if (p) {
		pbuf_free(p);
	}

	if (err != ERR_OK) {
		tcp_ap_error_cb(arg, err);
	}

	return ERR_OK;
}

struct tcp_ap *tcp_ap_alloc(const char *server_ip, u16 server_port)
{
	struct tcp_ap *ap;

	ap = kzalloc(sizeof(struct tcp_ap), GFP_KERNEL);
	if (!ap) {
		return NULL;
	}

	ap->server_port = server_port;
	ip4addr_aton(server_ip, &ap->server_ip);

	ap->pcb = tcp_new();
	if (!ap->pcb) {
		return NULL;
	}

	tcp_arg(ap->pcb, &ap);
	tcp_sent(ap->pcb, tcp_ap_transmit_cb);
	tcp_err(ap->pcb, tcp_ap_error_cb);
	tcp_recv(ap->pcb, tcp_ap_receive_cb);

	if (tcp_connect(ap->pcb, &ap->server_ip, server_port, tcp_ap_connected) != ERR_OK) {
		tcp_close(ap->pcb);
		kfree(ap);
		return NULL;
	}

	return ap;
}

void tcp_ap_free(struct tcp_ap *ap)
{
	if (ap->pcb) {
		tcp_close(ap->pcb);
	}

	kfree(ap);
}

int tcp_ap_send_data(struct tcp_ap *ap, const void *data, u32 length)
{
	u32 space, first;

	if (!ap || length == 0) {
		return -EINVAL;
	}

	if (ap->head >= ap->tail) {
		space = LOG_RING_SIZE - ap->head + ap->tail - 1;
	} else {
		space = ap->tail - ap->head - 1;
	}

	if (length > space) {
		length = space;
	}
	if (length == 0) {
		return -EINVAL;
	}

	if (ap->head + length > LOG_RING_SIZE) {
		first = LOG_RING_SIZE - ap->head;
		memcpy(&ap->ring[ap->head], data, first);
		memcpy(ap->ring, (const u8 *)data + first, length - first);
		ap->head = length - first;
	} else {
		memcpy(&ap->ring[ap->head], data, length);
		ap->head += length;
		if (ap->head >= LOG_RING_SIZE) {
			ap->head = 0;
		}
	}

	if (ap->connected && !ap->sending && ap->pcb) {
		tcp_ap_send(ap);
	}

	return length;
}
