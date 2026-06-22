
#define pr_fmt(fmt) "nrf24l01-port: " fmt

#include <linux/printk.h>
#include <linux/delay.h>
#include <linux/errno.h>
#include <linux/slab.h>
#include <linux/string.h>

#include "kinetis/serial-port.h"
#include "kinetis/spi_soft.h"
#include "kinetis/nrf24l01_port.h"
#include <kinetis/mavlink.h>

#include "driver_nrf24l01.h"
#include "driver_nrf24l01_interface.h"

/* Constants used directly from mavlink_types.h / mavlink_dialect/mavlink.h:
 *   MAVLINK_STX        (253, 0xFD)        — v2 start-of-message
 *   MAVLINK_STX_MAVLINK1 (0xFE)           — v1 STX fallback
 *   MAVLINK_NUM_NON_PAYLOAD_BYTES (12)    — v2: STX+header(9)+CK(2)
 *   MAVLINK_IFLAG_SIGNED (0x01)           — signature flag in incompat_flags
 *   MAVLINK_SIGNATURE_BLOCK_LEN (13)      — signature block bytes
 */
#define MAV_V1_OVERHEAD  (1 + MAVLINK_CORE_HEADER_MAVLINK1_LEN + MAVLINK_NUM_CHECKSUM_BYTES)
/* 1+5+2 = 8 */

/* Reassembly cache: MAVLINK_MAX_PACKET_LEN = 255+12+13 = 280 bytes max.
 * Use 512 to leave headroom and avoid cache overflow during burst RX. */
#define NRF24L01_RX_CACHE_SIZE 512

#define NRF24L01_MAX_PAYLOAD       32

struct nrf24l01_port {
	struct serial_port *serial;
	nrf24l01_handle_t nrf_handle;

	u8  rx_cache[NRF24L01_RX_CACHE_SIZE];
	u16 rx_cache_len;
};

static struct nrf24l01_port *nrf24l01_active_port;

static void nrf24l01_port_write_ring(struct nrf24l01_port *port,
	const u8 *data, u16 len)
{
	u16 i, index;

	index = port->serial->producer;
	for (i = 0; i < len; i++) {
		port->serial->rx_buffer[index] = data[i];
		index = (index + 1) % SERIAL_PORT_BUFFER_SIZE;
	}
	port->serial->producer = index;
}

static void nrf24l01_port_flush_complete(struct nrf24l01_port *port)
{
	u8 stx, len, incompat;
	u16 pos = 0, total, sign, remain;

	while (pos + 2 <= port->rx_cache_len) {
		stx = port->rx_cache[pos];

		if (stx == MAVLINK_STX) {
			/* v2: need STX + LEN + INCOMPAT = 3 bytes minimum */
			if (pos + 3 > port->rx_cache_len)
				break;

			len      = port->rx_cache[pos + 1];
			incompat = port->rx_cache[pos + 2];
			sign     = (incompat & MAVLINK_IFLAG_SIGNED) ? MAVLINK_SIGNATURE_BLOCK_LEN : 0;

			total = MAVLINK_NUM_NON_PAYLOAD_BYTES + len + sign;
		} else if (stx == MAVLINK_STX_MAVLINK1) {
			/* v1: need STX + LEN = 2 bytes minimum */
			if (pos + 2 > port->rx_cache_len)
				break;

			len   = port->rx_cache[pos + 1];
			total = MAV_V1_OVERHEAD + len;
		} else {
			pos++;
			continue;
		}

		/* Incomplete? Wait for more NRF24L01 chunks */
		if (pos + total > port->rx_cache_len)
			break;

		nrf24l01_port_write_ring(port, port->rx_cache + pos, total);

		pr_debug("rx %u bytes (v%c LEN=%u)\n", total,
			stx == MAVLINK_STX ? '2' : '1',
			port->rx_cache[pos + 1]);

		pos += total;
	}

	/* Remove consumed bytes (including skipped garbage before first STX) */
	if (pos > 0 && pos < port->rx_cache_len) {
		remain = port->rx_cache_len - pos;
		memmove(port->rx_cache, port->rx_cache + pos, remain);
		port->rx_cache_len = remain;
	} else if (pos >= port->rx_cache_len) {
		port->rx_cache_len = 0;
	}
	/* else pos == 0: nothing consumed, cache untouched */
}

#if KINETIS_FAKE_SIM == 0
void nrf24l01_interface_receive_callback(u8 type, u8 num,
	u8 *buf, u8 len)
{
	struct nrf24l01_port *port = nrf24l01_active_port;

	if (!port)
		return;

	switch (type) {
	case NRF24L01_INTERRUPT_RX_DR:
		if (port->rx_cache_len + len > NRF24L01_RX_CACHE_SIZE) {
			pr_warn("rx_cache overflow (%u + %u), dropping\n", port->rx_cache_len, len);
			port->rx_cache_len = 0;
			return;
		}
		memcpy(port->rx_cache + port->rx_cache_len, buf, len);
		port->rx_cache_len += len;

		nrf24l01_port_flush_complete(port);
		break;

	case NRF24L01_INTERRUPT_TX_DS:
		pr_debug("tx ok (pipe %u, %u bytes)\n", num, len);
		break;

	case NRF24L01_INTERRUPT_MAX_RT:
		pr_warn("tx max retry reached\n");
		break;

	case NRF24L01_INTERRUPT_TX_FULL:
		pr_warn("tx fifo full\n");
		break;

	default:
		break;
	}
}
#endif

static void nrf24l01_port_link(nrf24l01_handle_t *handle)
{
	DRIVER_NRF24L01_LINK_INIT(handle, nrf24l01_handle_t);
	DRIVER_NRF24L01_LINK_SPI_INIT(handle, nrf24l01_interface_spi_init);
	DRIVER_NRF24L01_LINK_SPI_DEINIT(handle, nrf24l01_interface_spi_deinit);
	DRIVER_NRF24L01_LINK_SPI_READ(handle, nrf24l01_interface_spi_read);
	DRIVER_NRF24L01_LINK_SPI_WRITE(handle, nrf24l01_interface_spi_write);
	DRIVER_NRF24L01_LINK_GPIO_INIT(handle, nrf24l01_interface_gpio_init);
	DRIVER_NRF24L01_LINK_GPIO_DEINIT(handle, nrf24l01_interface_gpio_deinit);
	DRIVER_NRF24L01_LINK_GPIO_WRITE(handle, nrf24l01_interface_gpio_write);
	DRIVER_NRF24L01_LINK_DELAY_MS(handle, nrf24l01_interface_delay_ms);
	DRIVER_NRF24L01_LINK_DEBUG_PRINT(handle, nrf24l01_interface_debug_print);
	DRIVER_NRF24L01_LINK_RECEIVE_CALLBACK(handle, nrf24l01_interface_receive_callback);
}

static void nrf24l01_port_configure(nrf24l01_handle_t *handle)
{
	u8 addr0[5] = {0xE1, 0xF0, 0x01, 0x01, 0x01};
	u8 addr1[5] = {0xC2, 0xF0, 0x01, 0x01, 0x01};
	u8 rd;

	/* Power-up and CRC */
	nrf24l01_set_config(handle, NRF24L01_CONFIG_PWR_UP, NRF24L01_BOOL_TRUE);
	nrf24l01_set_config(handle, NRF24L01_CONFIG_CRCO, NRF24L01_BOOL_TRUE);
	nrf24l01_set_config(handle, NRF24L01_CONFIG_EN_CRC, NRF24L01_BOOL_TRUE);
	nrf24l01_set_config(handle, NRF24L01_CONFIG_MASK_MAX_RT, NRF24L01_BOOL_FALSE);
	nrf24l01_set_config(handle, NRF24L01_CONFIG_MASK_TX_DS, NRF24L01_BOOL_FALSE);
	nrf24l01_set_config(handle, NRF24L01_CONFIG_MASK_RX_DR, NRF24L01_BOOL_FALSE);

	/* Both sides start in RX; transmit switches temporarily to TX */
	nrf24l01_set_mode(handle, NRF24L01_MODE_RX);

	/* Pipes 0–1: auto-ACK + RX enabled */
	nrf24l01_set_auto_acknowledgment(handle, NRF24L01_PIPE_0, NRF24L01_BOOL_TRUE);
	nrf24l01_set_auto_acknowledgment(handle, NRF24L01_PIPE_1, NRF24L01_BOOL_TRUE);
	nrf24l01_set_rx_pipe(handle, NRF24L01_PIPE_0, NRF24L01_BOOL_TRUE);
	nrf24l01_set_rx_pipe(handle, NRF24L01_PIPE_1, NRF24L01_BOOL_TRUE);

	/* 5-byte addresses */
	nrf24l01_set_address_width(handle, NRF24L01_ADDRESS_WIDTH_5_BYTES);

	/* Retransmit: 750 us delay, up to 10 retries */
	nrf24l01_auto_retransmit_delay_convert_to_register(handle, 750, &rd);
	nrf24l01_set_auto_retransmit_delay(handle, rd);
	nrf24l01_set_auto_retransmit_count(handle, 10);

	/* RF: ch 20 (2420 MHz), 2 Mbps, 0 dBm */
	nrf24l01_set_channel_frequency(handle, 20);
	nrf24l01_set_data_rate(handle, NRF24L01_DATA_RATE_2M);
	nrf24l01_set_output_power(handle, NRF24L01_OUTPUT_POWER_0_DBM);
	nrf24l01_set_continuous_carrier_transmit(handle, NRF24L01_BOOL_FALSE);
	nrf24l01_set_force_pll_lock_signal(handle, NRF24L01_BOOL_FALSE);

	/* Fixed 32-byte payload (no dynamic payload) */
	nrf24l01_set_dynamic_payload(handle, NRF24L01_BOOL_FALSE);
	nrf24l01_set_pipe_dynamic_payload(handle, NRF24L01_PIPE_0, NRF24L01_BOOL_FALSE);
	nrf24l01_set_pipe_dynamic_payload(handle, NRF24L01_PIPE_1, NRF24L01_BOOL_FALSE);
	nrf24l01_set_pipe_0_payload_number(handle, NRF24L01_MAX_PAYLOAD);
	nrf24l01_set_pipe_1_payload_number(handle, NRF24L01_MAX_PAYLOAD);

	/* Addresses: pipe0 = rotor→stator, pipe1 = stator→rotor */
	nrf24l01_set_tx_address(handle, addr0, 5);
	nrf24l01_set_rx_pipe_0_address(handle, addr1, 5);
	nrf24l01_set_rx_pipe_1_address(handle, addr1, 5);

	/* Clear interrupts, flush FIFOs, power up */
	nrf24l01_clear_interrupt(handle, NRF24L01_INTERRUPT_RX_DR);
	nrf24l01_clear_interrupt(handle, NRF24L01_INTERRUPT_TX_DS);
	nrf24l01_clear_interrupt(handle, NRF24L01_INTERRUPT_MAX_RT);
	nrf24l01_flush_tx(handle);
	nrf24l01_flush_rx(handle);
	nrf24l01_set_active(handle, NRF24L01_BOOL_TRUE);
}

static int nrf24l01_port_init(struct serial_port *serial)
{
	struct nrf24l01_port *port;
	u8 res;

	port = kzalloc(sizeof(*port), GFP_KERNEL);
	if (!port)
		return -ENOMEM;

	port->serial = serial;
	serial->private_data = port;

	nrf24l01_port_link(&port->nrf_handle);

	res = nrf24l01_init(&port->nrf_handle);
	if (res != 0) {
		kfree(port);
		return -EINVAL;
	}

	nrf24l01_set_active(&port->nrf_handle, NRF24L01_BOOL_FALSE);
	nrf24l01_port_configure(&port->nrf_handle);

	nrf24l01_active_port = port;

	pr_info("port '%s' ready (RX-primary, 2Mbps, ch20)\n", SP_NAME(serial));
	return 0;
}

static int nrf24l01_port_exit()
{
	struct nrf24l01_port *port = nrf24l01_active_port;

	if (!port)
		return 0;

	nrf24l01_deinit(&port->nrf_handle);

	nrf24l01_active_port = NULL;

	kfree(port);

	return 0;
}

static int nrf24l01_port_enable()
{
	return nrf24l01_set_active(&nrf24l01_active_port->nrf_handle, NRF24L01_BOOL_TRUE);
}

static int nrf24l01_port_disable()
{
	return nrf24l01_set_active(&nrf24l01_active_port->nrf_handle, NRF24L01_BOOL_FALSE);
}

static int nrf24l01_port_transmit_bytes(const u8 *data, u16 size)
{
	struct nrf24l01_port *port = nrf24l01_active_port;
	nrf24l01_handle_t *handle;
	u16 offset;
	u8 ret;

	handle = &port->nrf_handle;

	nrf24l01_set_mode(handle, NRF24L01_MODE_TX);

	for (offset = 0; offset < size; offset += NRF24L01_MAX_PAYLOAD) {
		u16 chunk = min_t(u16, size - offset, NRF24L01_MAX_PAYLOAD);

		ret = nrf24l01_send(handle, (u8 *)(data + offset), chunk);
		if (ret != 0) {
			pr_err("send chunk %u/%u failed: %u\n", offset / NRF24L01_MAX_PAYLOAD + 1,
				(size + NRF24L01_MAX_PAYLOAD - 1) / NRF24L01_MAX_PAYLOAD,
				ret);
			nrf24l01_flush_tx(handle);
			nrf24l01_set_mode(handle, NRF24L01_MODE_RX);
			return -EPIPE;
		}
	}

	pr_debug("tx %u bytes in %u chunks\n", size,
		(size + NRF24L01_MAX_PAYLOAD - 1) / NRF24L01_MAX_PAYLOAD);

	nrf24l01_set_mode(handle, NRF24L01_MODE_RX);
	return 0;
}

struct serial_port_ops nrf24l01_serial_port_ops = {
	.init            = nrf24l01_port_init,
	.exit            = nrf24l01_port_exit,
	.enable          = nrf24l01_port_enable,
	.disable         = nrf24l01_port_disable,
	.transmit_bytes  = nrf24l01_port_transmit_bytes,
};

void nrf24l01_port_poll(struct serial_port *serial)
{
	struct nrf24l01_port *port = nrf24l01_active_port;

	if (!port)
		return;

	nrf24l01_irq_handler(&port->nrf_handle);
}
