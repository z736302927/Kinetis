/* SPDX-License-Identifier: GPL-2.0 */
#ifndef __FAKE_INTERFACE_H
#define __FAKE_INTERFACE_H

#include <linux/types.h>

#include <kinetis/iic_soft.h>
#include <kinetis/spi_soft.h>
#include <kinetis/tim-task.h>

extern struct serial_port_ops usart2_serial_port_ops;
extern struct serial_port_ops uart3_serial_port_ops;
extern struct serial_port_ops uart4_serial_port_ops;
extern struct serial_port_ops uart5_serial_port_ops;
extern struct serial_port_ops usart6_serial_port_ops;
extern struct flash_ops general_flash_ops;
extern struct bootloader_ops general_bootloader_ops;

extern u8 *general_flash_addr;

#endif	/* __FAKE_INTERFACE_H */
