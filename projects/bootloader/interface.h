/* SPDX-License-Identifier: GPL-2.0 */
#ifndef __BOOTLOADER_INTERFACE_H
#define __BOOTLOADER_INTERFACE_H

#include <linux/types.h>

#include <kinetis/serial-port.h>
#include "kinetis/bootloader.h"

extern struct flash_ops general_flash_ops;
extern struct bootloader_ops general_bootloader_ops;
extern struct serial_port_ops usart2_serial_port_ops;
extern struct serial_port *stm32_serial_port[5];

#if KINETIS_FAKE_SIM
extern u8 *general_flash_addr;
#endif

#endif  /* __BOOTLOADER_INTERFACE_H */
