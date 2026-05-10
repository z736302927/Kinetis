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

struct serial_port;

struct serial_port_ops {
	int (*transmit_bytes)(const u8 *data, u16 size);
	int (*receive_bytes)(u8 *data, u16 size, u32 timeout_ms);
	void (*update_producer)(struct serial_port *serial);
	int (*config)(struct serial_port *serial, u32 baud_rate, u8 parity, u8 data_bits, u8 flow_control);
	void (*irq_disable)(void);
	void (*irq_enable)(void);
	void (*set_tx)(u8 state);
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

	u32(*sim_callback)(char *request, char *response, void *context);
	void *private;

	u8 thread_switch;
	pthread_t thread;

	u32 baud_rate;		/* Current baud rate: 1200, 2400, 4800, 9600, 19200, 38400, 57600, 115200, etc */
	u8 parity;		/* Current parity: 0=None, 1=Odd, 2=Even */
	u8 data_bits;		/* Current data bits: 7 or 8 */
	u8 flow_control;	/* Current flow control: 0=None, 1=XON/XOFF, 2=RTS/CTS */

	struct serial_port_ops *ops;

	char *name;

};

#define SP_NAME(sp)  ((sp)->name ? (sp)->name : "?")

#define SERIAL_PORT_DF_NONE 	1
#define SERIAL_PORT_DF_SELF 	2
#define SERIAL_PORT_DF_OTHERS 	3

void serial_port_start_thread(struct serial_port *serial, u8 oneself,
	u32(*sim_callback)(char *request, char *response, void *context),
	void *private);
void serial_port_stop_thread(struct serial_port *serial);
struct serial_port *serial_port_alloc(struct serial_port_ops *ops, const char *name);
void serial_port_free(struct serial_port *serial);
void serial_port_clear_rx(struct serial_port *serial);
int serial_port_receive_bytes(struct serial_port *serial_port, char *buffer, int size, u32 timeout_ms);
int serial_port_transmit_bytes(struct serial_port *serial, const u8 *data, u16 size);
int serial_port_transmit_byte(struct serial_port *serial, u8 byte);
int serial_port_data_available(struct serial_port *serial);
int serial_port_config(struct serial_port *serial, u32 baud_rate, u8 parity, u8 data_bits, u8 flow_control);
void serial_port_send_break(struct serial_port *serial);

extern struct serial_port_ops fake_serial_port_ops;

/* The above procedure is modified by the user according to the hardware device, otherwise the driver cannot run. */

#ifdef __cplusplus
}
#endif

#endif /* __K_SERIALPORT_H */
