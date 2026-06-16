
#include <linux/printk.h>
#include <linux/delay.h>
#include <linux/iopoll.h>

#include <kinetis/iic_soft.h>
#include <kinetis/spi_soft.h>
#include <kinetis/real-time-clock.h>
#include <kinetis/basic-timer.h>
#include <kinetis/serial-port.h>
#include "kinetis/bootloader.h"

static int usart1_serial_transmit_bytes(const u8 *data, u16 size)
{
// 	pr_debug("usart1 tx: %u bytes", size);
	return 0;
}

static int usart1_serial_receive_bytes(u8 *data, u16 size, u32 timeout_ms)
{
// 	pr_debug("usart1 rx: request %u bytes, timeout %u ms", size, timeout_ms);
	return 0;
}

static void usart1_serial_update_producer(struct serial_port *serial)
{
	serial->producer = (serial->producer + serial->transmited_size) % SERIAL_PORT_BUFFER_SIZE;
	pr_debug("usart1 update producer: prod=%d cons=%d",
		serial->producer, serial->consumer);
}

static int usart1_serial_config(struct serial_port *serial, u32 baud_rate,
	u8 parity, u8 data_bits, u8 flow_control)
{
	pr_info("usart1 serial config: baud=%u parity=%u data_bits=%u flow=%u",
		baud_rate, parity, data_bits, flow_control);
	return 0;
}

static void usart1_serial_irq_disable(void)
{
	pr_debug("usart1 irq disable");
}

static void usart1_serial_irq_enable(void)
{
	pr_debug("usart1 irq enable");
}

static void usart1_serial_set_tx(u8 state)
{
	pr_debug("usart1 set tx: %s", state ? "high" : "low");
}

struct serial_port_ops usart1_serial_port_ops = {
	.transmit_bytes  = usart1_serial_transmit_bytes,
	.receive_bytes   = usart1_serial_receive_bytes,
	.update_producer = usart1_serial_update_producer,
	.config          = usart1_serial_config,
	.irq_disable     = usart1_serial_irq_disable,
	.irq_enable      = usart1_serial_irq_enable,
	.set_tx          = usart1_serial_set_tx,
};

static int usart2_serial_transmit_bytes(const u8 *data, u16 size)
{
	pr_debug("usart2 tx: %u bytes", size);
	return 0;
}

static int usart2_serial_receive_bytes(u8 *data, u16 size, u32 timeout_ms)
{
	pr_debug("usart2 rx: request %u bytes, timeout %u ms", size, timeout_ms);
	return 0;
}

static void usart2_serial_update_producer(struct serial_port *serial)
{
	serial->producer = (serial->producer + serial->transmited_size) % SERIAL_PORT_BUFFER_SIZE;
	pr_debug("usart2 update producer: prod=%d cons=%d",
		serial->producer, serial->consumer);
}

static int usart2_serial_config(struct serial_port *serial, u32 baud_rate,
	u8 parity, u8 data_bits, u8 flow_control)
{
	pr_info("usart2 serial config: baud=%u parity=%u data_bits=%u flow=%u",
		baud_rate, parity, data_bits, flow_control);
	return 0;
}

static void usart2_serial_irq_disable(void)
{
	pr_debug("usart2 irq disable");
}

static void usart2_serial_irq_enable(void)
{
	pr_debug("usart2 irq enable");
}

static void usart2_serial_set_tx(u8 state)
{
	pr_debug("usart2 set tx: %s", state ? "high" : "low");
}

struct serial_port_ops usart2_serial_port_ops = {
	.transmit_bytes  = usart2_serial_transmit_bytes,
	.receive_bytes   = usart2_serial_receive_bytes,
	.update_producer = usart2_serial_update_producer,
	.config          = usart2_serial_config,
	.irq_disable     = usart2_serial_irq_disable,
	.irq_enable      = usart2_serial_irq_enable,
	.set_tx          = usart2_serial_set_tx,
};

u8 *general_flash_addr;

static int general_flash_erase(u64 addr, u32 size)
{
	if (addr + size > (u64)general_flash_addr + ROM_TOTAL_SIZE) {
		pr_err("general_erase: addr 0x%x+%u > %u\n", addr, size, ROM_TOTAL_SIZE);
		return -EINVAL;
	}
	memset((u8 *)addr, 0xFF, size);

	return 0;
}

static int general_flash_write(u64 addr, const u8 *data, u32 size)
{
	if (addr + size > (u64)general_flash_addr + ROM_TOTAL_SIZE) {
		pr_err("general_write: addr 0x%x+%u > %u\n", addr, size, ROM_TOTAL_SIZE);
		return -EINVAL;
	}
	memcpy((u8 *)addr, data, size);

	return 0;
}

static int general_flash_read(u64 addr, u8 *buf, u32 size)
{
	if (addr + size > (u64)general_flash_addr + ROM_TOTAL_SIZE) {
		pr_err("general_read: addr 0x%x+%u > %u\n", addr, size, ROM_TOTAL_SIZE);
		return -EINVAL;
	}
	memcpy(buf, (u8 *)addr, size);

	return 0;
}

static void general_disable_irq(void)
{
	pr_info("general disable irq called (ignored in sim)\n");
}
static void general_cleanup_hw_resource(void)
{
	pr_info("general cleanup hw resource called (ignored in sim)\n");
}
static void general_disable_cache(void)
{
	pr_info("general disable cache called (ignored in sim)\n");
}
static void general_set_stack_pointer(u64 ptr)
{
	pr_info("general set stack pointer called (ptr=0x%08llx, ignored in sim)\n", ptr);
}
static void general_redirect_isr_table(u64 addr)
{
	pr_info("general redirect isr table called (addr=0x%08llx, ignored in sim)\n", addr);
}
static void general_system_reset(void)
{
	pr_info("general system reset called (ignored in sim)\n");
}
static void general_clear_pending_irqs(void)
{
	pr_info("general clear pending irqs called (ignored in sim)\n");
}
static void general_enable_irq(void)
{
	pr_info("general enable irq called (ignored in sim)\n");
}

struct flash_ops general_flash_ops = {
	.erase                = general_flash_erase,
	.write                = general_flash_write,
	.read                 = general_flash_read,
};

struct bootloader_ops general_bootloader_ops = {
	.disable_irq          = general_disable_irq,
	.cleanup_hw_resource  = general_cleanup_hw_resource,
	.disable_cache        = general_disable_cache,
	.set_stack_pointer    = general_set_stack_pointer,
	.redirect_isr_table   = general_redirect_isr_table,
	.system_reset         = general_system_reset,
	.clear_pending_irqs   = general_clear_pending_irqs,
	.enable_irq           = general_enable_irq,
};