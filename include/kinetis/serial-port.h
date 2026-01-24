#ifndef __K_SERIALPORT_H
#define __K_SERIALPORT_H

#ifdef __cplusplus
extern "C" {
#endif

/* The following program is modified by the user according to the hardware device, otherwise the driver cannot run. */

/* Includes ------------------------------------------------------------------*/
#include <linux/types.h>
#include <linux/list.h>

#include "kinetis/core_common.h"

#include <pthread.h>

#define SERIAL_PORT_BUFFER_SIZE 256

struct virtual_at_command {
	const char *request;
	const char *response;
};

struct serial_port {
	u8 rx_buffer[SERIAL_PORT_BUFFER_SIZE];
	u8 tx_buffer[SERIAL_PORT_BUFFER_SIZE];
	u16 received_size;
	u16 transmited_size;
	u32 rx_scan_interval;
	u16 producer;
	u16 consumer;
	bool rx_complete;
	bool tx_complete;

	struct list_head list;

	int (*transmit_bytes)(const u8 *data, u16 size);

	pthread_t thread;
	u8 thread_switch;

	struct virtual_at_command *at_cmd_set;
};

struct serial_port *serial_port_alloc(struct virtual_at_command *cmd);
void serial_port_free(struct serial_port *serial);
int serial_port_get_data(struct serial_port *serial_port, char *buffer, int size, u32 timeout_ms);
int serial_port_transmit_bytes(struct serial_port *serial, const u8 *data, u16 size);

/* The above procedure is modified by the user according to the hardware device, otherwise the driver cannot run. */

#ifdef __cplusplus
}
#endif

#endif /* __K_SERIALPORT_H */
