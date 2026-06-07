/* SPDX-License-Identifier: GPL-2.0 */
#ifndef __STM32_INTERFACE_H
#define __STM32_INTERFACE_H

#include <linux/types.h>

#include <kinetis/iic_soft.h>
#include <kinetis/spi_soft.h>
#include <kinetis/tim-task.h>

extern struct iic_master general_iic_master;
extern struct spi_master general_spi_master;
extern struct rtc_device general_rtc;
extern struct serial_port_ops usart2_serial_port_ops;
extern struct serial_port_ops usart6_serial_port_ops;
extern struct flash_ops general_flash_ops;
extern struct bootloader_ops general_bl_ops;
extern struct serial_port *stm32_serial_port[5];
extern struct hall_device *hall_dev;

int wiced_platform_init(void);
void wiced_platform_deinit(void);
void wiced_platform_reset_wifi(void);
void wiced_platform_power_wifi(u8 on);
int wiced_platform_oob_irq_pending(void);
void wiced_platform_oob_irq_clear(void);

int  wiced_sdio_send_command(u8 cmd, u32 arg, u32 *resp);
int  wiced_sdio_read_data(u8 *buf, u32 len);
int  wiced_sdio_write_data(const u8 *buf, u32 len);
int  wiced_sdio_set_block_size(u16 blk_size);
int  wiced_sdio_set_bus_width(u8 width);
int  wiced_sdio_set_clock(u32 freq_hz);
void wiced_sdio_enable_irq(int enable);

u32 hall_read_rotated_time(struct hall_device *dev);
void hall_fake_pulse(struct tim_task *task);

#endif	/* __STM32_INTERFACE_H */
