/* SPDX-License-Identifier: GPL-2.0 */
#ifndef __STM32_INTERFACE_H
#define __STM32_INTERFACE_H

#include <linux/types.h>

#include <kinetis/iic_soft.h>
#include <kinetis/spi_soft.h>
#include <kinetis/tim-task.h>

#include "driver_w25qxx.h"

extern struct iic_master iic1_master;
extern struct spi_master spi1_master;
extern struct spi_master spi6_master;
extern struct spi_master spi7_master;
extern struct qspi_ops qspi1_ops;
extern struct rtc_device general_rtc;
extern struct serial_port_ops usart2_serial_port_ops;
extern struct serial_port_ops uart4_serial_port_ops;
extern struct serial_port_ops uart5_serial_port_ops;
extern struct serial_port_ops usart6_serial_port_ops;
extern struct serial_port_ops nrf24l01_serial_port_ops;
extern struct flash_ops general_flash_ops;
extern struct bootloader_ops general_bootloader_ops;
extern struct serial_port *stm32_serial_port[5];
extern struct hall_device *hall_dev;
// extern struct w25qxxx_device *w25qxxx_dev;
extern w25qxx_handle_t *ld_w25qxx_handle;

u32 hall_read_rotated_time(struct hall_device *dev);
void hall_fake_pulse(struct tim_task *task);
void arm_dwt_init(void);
void flash_set_wait_for_completion(u8 enable);
int pov_spi_multi_wait_complete(u32 timeout_us);
u8 pov_read_arm_id(void);

#endif	/* __STM32_INTERFACE_H */
