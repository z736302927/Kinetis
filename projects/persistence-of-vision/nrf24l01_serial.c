/**
 * @file nrf24l01_serial.c
 * @brief nRF24L01 serial_port_ops implementation for MAVLink transport
 * @note Handles fragmentation of MAVLink messages to fit nRF24L01's
 *       32-byte payload, and reassembly of received fragments
 */

#include <linux/kernel.h>
#include <linux/printk.h>
#include <linux/errno.h>
#include <linux/string.h>
#include <linux/delay.h>

#include "nrf24l01_serial.h"

/*********************************************************************
 * Internal: Fragment and send data over nRF24L01
 *********************************************************************/

static int nrf24l01_serial_transmit(const u8 *data, u16 size)
{
	/* This is called via serial_port_ops with our device context
	 * embedded in the serial_port structure. We use the global
	 * device pointer since serial_port_ops doesn't pass context. */
	extern struct nrf24l01_serial_device *g_nrf24l01_serial;

	struct nrf24l01_serial_device *dev = g_nrf24l01_serial;
	u16 offset = 0;
	u8 frag_index = 0;

	if (!dev || !dev->nrf)
		return -EINVAL;

	if (size == 0)
		return 0;

	while (offset < size) {
		u8 pkt[NRF24L01_PAYLOAD_SIZE];
		u16 remaining = size - offset;
		u8 frag_size;

		if (remaining <= NRF24L01_FRAG_DATA_SIZE) {
			/* Last fragment */
			pkt[0] = NRF24L01_FRAG_LAST_BIT | frag_index;
			frag_size = remaining;
		} else {
			/* More fragments follow */
			pkt[0] = frag_index;
			frag_size = NRF24L01_FRAG_DATA_SIZE;
		}

		memcpy(&pkt[1], &data[offset], frag_size);
		offset += frag_size;

		int ret = nrf24l01_send(dev->nrf, pkt, frag_size + 1);
		if (ret < 0) {
			pr_err("nRF24L01 serial: TX fragment %d failed: %d\n",
				frag_index, ret);
			return ret;
		}

		frag_index++;

		/* Small delay between fragments for receiver to process */
		if (offset < size)
			udelay(200);
	}

	pr_debug("nRF24L01 serial: sent %d bytes in %d fragments\n",
		size, frag_index);
	return size;
}

/*********************************************************************
 * Internal: Receive and reassemble data from nRF24L01
 *********************************************************************/

static int nrf24l01_serial_receive(u8 *data, u16 size, u32 timeout_ms)
{
	extern struct nrf24l01_serial_device *g_nrf24l01_serial;

	struct nrf24l01_serial_device *dev = g_nrf24l01_serial;
	u32 start_ms = 0;
	u16 copied = 0;

	if (!dev || !dev->nrf)
		return -EINVAL;

	/* Try to receive fragments and reassemble */
	u32 deadline = timeout_ms;

	while (deadline > 0 && copied < size) {
		/* Check if we have data in reassembly buffer */
		if (dev->rx_len > dev->rx_read) {
			u16 avail = dev->rx_len - dev->rx_read;
			u16 to_copy = (avail < (size - copied)) ? avail : (size - copied);

			memcpy(&data[copied], &dev->rx_buf[dev->rx_read], to_copy);
			dev->rx_read += to_copy;
			copied += to_copy;

			/* Reset buffer if fully consumed */
			if (dev->rx_read >= dev->rx_len) {
				dev->rx_len = 0;
				dev->rx_read = 0;
			}

			if (copied >= size)
				break;
		}

		/* Try to receive a new fragment */
		if (nrf24l01_data_available(dev->nrf)) {
			u8 pkt[NRF24L01_PAYLOAD_SIZE];
			u8 pkt_len = 0;

			int ret = nrf24l01_receive(dev->nrf, pkt, &pkt_len);
			if (ret > 0 && pkt_len > 1) {
				u8 frag_header = pkt[0];
				u8 frag_data_len = pkt_len - 1;
				u8 is_last = frag_header & NRF24L01_FRAG_LAST_BIT;

				if (dev->rx_len + frag_data_len <= sizeof(dev->rx_buf)) {
					memcpy(&dev->rx_buf[dev->rx_len], &pkt[1], frag_data_len);
					dev->rx_len += frag_data_len;
				}

				if (is_last) {
					dev->rx_read = 0;
					/* Continue to copy from buffer */
					continue;
				}
			}
		} else {
			mdelay(1);
			if (deadline > 0)
				deadline--;
		}
	}

	return copied > 0 ? (int)copied : -EAGAIN;
}

/*********************************************************************
 * Internal: Check data availability
 *********************************************************************/

static void nrf24l01_serial_update_producer(struct serial_port *serial)
{
	/* Poll for incoming data and buffer it */
	extern struct nrf24l01_serial_device *g_nrf24l01_serial;
	struct nrf24l01_serial_device *dev = g_nrf24l01_serial;

	if (!dev || !dev->nrf)
		return;

	/* Try to receive any pending fragments */
	if (nrf24l01_data_available(dev->nrf)) {
		u8 pkt[NRF24L01_PAYLOAD_SIZE];
		u8 pkt_len = 0;

		int ret = nrf24l01_receive(dev->nrf, pkt, &pkt_len);
		if (ret > 0 && pkt_len > 1) {
			u8 frag_header = pkt[0];
			u8 frag_data_len = pkt_len - 1;
			u8 is_last = frag_header & NRF24L01_FRAG_LAST_BIT;

			if (dev->rx_len + frag_data_len <= sizeof(dev->rx_buf)) {
				memcpy(&dev->rx_buf[dev->rx_len], &pkt[1], frag_data_len);
				dev->rx_len += frag_data_len;
			}

			if (is_last)
				dev->rx_read = 0;
		}
	}
}

/*********************************************************************
 * Initialization
 *********************************************************************/

/* Global pointer for serial_port_ops callbacks (no context parameter) */
struct nrf24l01_serial_device *g_nrf24l01_serial;

int nrf24l01_serial_init(struct nrf24l01_serial_device *dev,
	struct nrf24l01_device *nrf)
{
	if (!dev || !nrf)
		return -EINVAL;

	memset(dev, 0, sizeof(*dev));
	dev->nrf = nrf;

	dev->ops.transmit_bytes = nrf24l01_serial_transmit;
	dev->ops.receive_bytes = nrf24l01_serial_receive;
	dev->ops.update_producer = nrf24l01_serial_update_producer;
	dev->ops.config = NULL;
	dev->ops.irq_disable = NULL;
	dev->ops.irq_enable = NULL;
	dev->ops.set_tx = NULL;

	g_nrf24l01_serial = dev;

	pr_info("nRF24L01 serial: initialized\n");
	return 0;
}

struct serial_port_ops *nrf24l01_serial_get_ops(struct nrf24l01_serial_device *dev)
{
	if (!dev)
		return NULL;
	return &dev->ops;
}

int nrf24l01_serial_inject_rx(struct nrf24l01_serial_device *dev,
	const u8 *data, u16 len)
{
#if KINETIS_FAKE_SIM
	if (!dev || !data || len == 0)
		return -EINVAL;

	/* Fragment the data and inject into nRF24L01 sim RX FIFO */
	u16 offset = 0;
	u8 frag_index = 0;

	while (offset < len) {
		u8 pkt[NRF24L01_PAYLOAD_SIZE];
		u16 remaining = len - offset;
		u8 frag_size;

		if (remaining <= NRF24L01_FRAG_DATA_SIZE) {
			pkt[0] = NRF24L01_FRAG_LAST_BIT | frag_index;
			frag_size = remaining;
		} else {
			pkt[0] = frag_index;
			frag_size = NRF24L01_FRAG_DATA_SIZE;
		}

		memcpy(&pkt[1], &data[offset], frag_size);

		/* Inject into nRF24L01 sim RX FIFO */
		memcpy(dev->nrf->sim_rx_fifo, pkt, frag_size + 1);
		dev->nrf->sim_rx_fifo_len = frag_size + 1;

		/* Process this fragment immediately */
		u8 frag_header = pkt[0];
		u8 frag_data_len = frag_size;
		u8 is_last = frag_header & NRF24L01_FRAG_LAST_BIT;

		if (dev->rx_len + frag_data_len <= sizeof(dev->rx_buf)) {
			memcpy(&dev->rx_buf[dev->rx_len], &pkt[1], frag_data_len);
			dev->rx_len += frag_data_len;
		}

		if (is_last)
			dev->rx_read = 0;

		offset += frag_size;
		frag_index++;
	}

	return 0;
#else
	return -ENOSYS;
#endif
}
