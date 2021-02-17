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

struct serial_port {
    u16 tmp_buffer_size;
    u16 *tmp_buffer;
    u16 rx_buffer_size;
    char *rx_buffer;
    u32 rx_scan_interval;
    u16 rx_head;
    u16 rx_tail;
    u8 *tx_buffer;
    u16 tx_buffer_size;
    u8 port_nbr;
    u8 *end_char;
    u8 end_char_size;
    u8 *current_buffer;
    struct list_head list;
};

/* The above procedure is modified by the user according to the hardware device, otherwise the driver cannot run. */

u8 serial_port_open(struct serial_port *Instance);
void serial_port_close(struct serial_port *Instance);
void serial_port_send(struct serial_port *Instance);
u8 serial_port_receive(struct serial_port *Instance);



#ifdef __cplusplus
}
#endif

#endif /* __K_SERIALPORT_H */
