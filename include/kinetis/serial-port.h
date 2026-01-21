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

typedef enum {
	AT_CMD_TYPE_TEST,            /* Test command (AT+XXX?) - Query current value */
	AT_CMD_TYPE_READ,            /* Read command (AT+XXX?) - Read parameter */
	AT_CMD_TYPE_WRITE,           /* Write command (AT+XXX=param) - Set parameter */
	AT_CMD_TYPE_EXECUTE,         /* Execute command (AT+XXX) - Execute action */
	AT_CMD_TYPE_BASIC            /* Basic command (AT) - Simple operation */
} at_cmd_type_t;

struct at_command {
	const char *command;         /* Command string */
	at_cmd_type_t type;        /* Command type */
	const char *description;     /* Command description */
	const char *params;          /* Parameters description */
	const char *default_value;   /* Default value */
	const char *response;        /* Expected response */
	const char *error_response;  /* Error response if any */
};

#define SERIAL_PORT_BUFFER_SIZE 256

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

	struct at_command *at_cmd_set;
};

const struct at_command *at_command_find(struct at_command *array, const char *command_name);

void serial_port_init(struct serial_port *serial, struct at_command *at_cmd_set);
void serial_port_deinit(struct serial_port *serial);
int serial_port_get_data(struct serial_port *serial_port, char *buffer, int size, u32 timeout_ms);
int serial_port_transmit_bytes(struct serial_port *serial, const u8 *data, u16 size);

/* The above procedure is modified by the user according to the hardware device, otherwise the driver cannot run. */

#ifdef __cplusplus
}
#endif

#endif /* __K_SERIALPORT_H */
