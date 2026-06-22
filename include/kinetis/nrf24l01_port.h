/* SPDX-License-Identifier: GPL-2.0 */
#ifndef __NRF24L01_PORT_H
#define __NRF24L01_PORT_H

#include <linux/types.h>

#include <kinetis/serial-port.h>

void nrf24l01_port_poll(struct serial_port *serial);

#endif /* __NRF24L01_PORT_H */
