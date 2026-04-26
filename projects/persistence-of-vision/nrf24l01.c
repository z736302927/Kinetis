/**
 * @file nrf24l01.c
 * @brief nRF24L01+ wireless module driver implementation
 * @note Supports both real hardware (MCU_PLATFORM_STM32) and
 *       simulation mode (KINETIS_FAKE_SIM)
 */

#include <linux/kernel.h>
#include <linux/printk.h>
#include <linux/errno.h>
#include <linux/delay.h>
#include <linux/string.h>

#include "nrf24l01.h"

/*********************************************************************
 * SPI Register Access Helpers
 *********************************************************************/

#if MCU_PLATFORM_STM32

static u8 nrf24l01_read_reg(struct nrf24l01_device *dev, u8 reg)
{
	u8 val;

	dev->csn_ctrl(0);
	dev->spi_xfer(NRF24L01_CMD_R_REGISTER | (reg & 0x1F));
	val = dev->spi_xfer(NRF24L01_CMD_NOP);
	dev->csn_ctrl(1);

	return val;
}

static void nrf24l01_write_reg(struct nrf24l01_device *dev, u8 reg, u8 val)
{
	dev->csn_ctrl(0);
	dev->spi_xfer(NRF24L01_CMD_W_REGISTER | (reg & 0x1F));
	dev->spi_xfer(val);
	dev->csn_ctrl(1);
}

static void nrf24l01_read_buf(struct nrf24l01_device *dev, u8 reg,
	u8 *buf, u8 len)
{
	dev->csn_ctrl(0);
	dev->spi_xfer(NRF24L01_CMD_R_REGISTER | (reg & 0x1F));
	for (u8 i = 0; i < len; i++)
		buf[i] = dev->spi_xfer(NRF24L01_CMD_NOP);
	dev->csn_ctrl(1);
}

static void nrf24l01_write_buf(struct nrf24l01_device *dev, u8 reg,
	const u8 *buf, u8 len)
{
	dev->csn_ctrl(0);
	dev->spi_xfer(NRF24L01_CMD_W_REGISTER | (reg & 0x1F));
	for (u8 i = 0; i < len; i++)
		dev->spi_xfer(buf[i]);
	dev->csn_ctrl(1);
}

#endif /* MCU_PLATFORM_STM32 */

/*********************************************************************
 * Initialization
 *********************************************************************/

int nrf24l01_init(struct nrf24l01_device *dev,
	nrf24l01_spi_xfer_cb spi_xfer,
	nrf24l01_ce_cb ce_ctrl,
	nrf24l01_csn_cb csn_ctrl)
{
	if (!dev || !spi_xfer || !ce_ctrl || !csn_ctrl)
		return -EINVAL;

	memset(dev, 0, sizeof(*dev));

	dev->spi_xfer = spi_xfer;
	dev->ce_ctrl = ce_ctrl;
	dev->csn_ctrl = csn_ctrl;
	dev->channel = 76;            /* Default channel */
	dev->data_rate = NRF24L01_DATA_RATE_1M;
	dev->power = NRF24L01_POWER_0DBM;
	dev->addr_width = NRF24L01_ADDR_WIDTH;
	dev->payload_size = NRF24L01_PAYLOAD_SIZE;
	dev->auto_ack = 1;
	dev->retransmit_count = 5;
	dev->retransmit_delay = 5;

#if KINETIS_FAKE_SIM
	dev->sim_tx_fifo_len = 0;
	dev->sim_rx_fifo_len = 0;
	dev->sim_mode = 0;
	memset(dev->sim_tx_pipe_addr, 0xE7, NRF24L01_ADDR_WIDTH);
	memset(dev->sim_rx_pipe_addr, 0xE7, NRF24L01_ADDR_WIDTH);
	pr_info("nRF24L01: initialized in simulation mode\n");
#else
	/* Hardware initialization sequence */
	dev->csn_ctrl(1);
	dev->ce_ctrl(0);
	mdelay(5);  /* Power-on reset delay */

	/* Configure basic registers */
	nrf24l01_write_reg(dev, NRF24L01_REG_CONFIG, 0x08);  /* Enable CRC */
	nrf24l01_write_reg(dev, NRF24L01_REG_EN_AA, dev->auto_ack ? 0x01 : 0x00);
	nrf24l01_write_reg(dev, NRF24L01_REG_EN_RXADDR, 0x01);
	nrf24l01_write_reg(dev, NRF24L01_REG_SETUP_AW, dev->addr_width - 2);
	nrf24l01_write_reg(dev, NRF24L01_REG_SETUP_RETR,
		(dev->retransmit_delay << 4) | dev->retransmit_count);
	nrf24l01_write_reg(dev, NRF24L01_REG_RF_CH, dev->channel);
	nrf24l01_write_reg(dev, NRF24L01_REG_RF_SETUP,
		dev->data_rate | dev->power);
	nrf24l01_write_reg(dev, NRF24L01_REG_RX_PW_P0, dev->payload_size);
	nrf24l01_write_reg(dev, NRF24L01_REG_DYNPD, 0x00);
	nrf24l01_write_reg(dev, NRF24L01_REG_FEATURE, 0x00);

	pr_info("nRF24L01: initialized on hardware\n");
#endif

	return 0;
}

/*********************************************************************
 * Configuration
 *********************************************************************/

int nrf24l01_set_channel(struct nrf24l01_device *dev, u8 channel)
{
	if (channel > NRF24L01_MAX_CHANNEL)
		return -EINVAL;

	dev->channel = channel;

#if MCU_PLATFORM_STM32
	nrf24l01_write_reg(dev, NRF24L01_REG_RF_CH, channel);
#endif

	return 0;
}

int nrf24l01_set_data_rate(struct nrf24l01_device *dev, u8 rate)
{
	if (rate != NRF24L01_DATA_RATE_250K &&
		rate != NRF24L01_DATA_RATE_1M &&
		rate != NRF24L01_DATA_RATE_2M)
		return -EINVAL;

	dev->data_rate = rate;

#if MCU_PLATFORM_STM32
	u8 setup = nrf24l01_read_reg(dev, NRF24L01_REG_RF_SETUP);
	setup &= ~(0x28);  /* Clear data rate bits */
	setup |= rate;
	nrf24l01_write_reg(dev, NRF24L01_REG_RF_SETUP, setup);
#endif

	return 0;
}

int nrf24l01_set_address(struct nrf24l01_device *dev, const u8 *addr, u8 width)
{
	if (!addr || width < 3 || width > 5)
		return -EINVAL;

	dev->addr_width = width;
	memcpy(dev->tx_pipe_addr, addr, width);

#if MCU_PLATFORM_STM32
	nrf24l01_write_buf(dev, NRF24L01_REG_TX_ADDR, addr, width);
	nrf24l01_write_buf(dev, NRF24L01_REG_RX_ADDR_P0, addr, width);
	nrf24l01_write_reg(dev, NRF24L01_REG_SETUP_AW, width - 2);
#else
	memcpy(dev->sim_tx_pipe_addr, addr, width);
	memcpy(dev->sim_rx_pipe_addr, addr, width);
#endif

	return 0;
}

/*********************************************************************
 * TX/RX Mode Control
 *********************************************************************/

int nrf24l01_tx_mode(struct nrf24l01_device *dev)
{
#if MCU_PLATFORM_STM32
	u8 cfg = nrf24l01_read_reg(dev, NRF24L01_REG_CONFIG);
	cfg &= ~(0x01);       /* Clear PRIM_RX */
	cfg |= (1 << 3);      /* PWR_UP */
	nrf24l01_write_reg(dev, NRF24L01_REG_CONFIG, cfg);
	dev->ce_ctrl(0);
#else
	dev->sim_mode = 1;
#endif

	return 0;
}

int nrf24l01_rx_mode(struct nrf24l01_device *dev)
{
#if MCU_PLATFORM_STM32
	u8 cfg = nrf24l01_read_reg(dev, NRF24L01_REG_CONFIG);
	cfg |= (1 << 0);      /* PRIM_RX = 1 */
	cfg |= (1 << 3);      /* PWR_UP */
	nrf24l01_write_reg(dev, NRF24L01_REG_CONFIG, cfg);
	dev->ce_ctrl(1);       /* CE high for RX */
#else
	dev->sim_mode = 2;
#endif

	return 0;
}

/*********************************************************************
 * Data Transfer
 *********************************************************************/

int nrf24l01_send(struct nrf24l01_device *dev, const u8 *data, u8 len)
{
	if (!data || len == 0 || len > NRF24L01_PAYLOAD_SIZE)
		return -EINVAL;

#if KINETIS_FAKE_SIM
	/* Simulation: store in TX FIFO */
	memcpy(dev->sim_tx_fifo, data, len);
	dev->sim_tx_fifo_len = len;
	pr_debug("nRF24L01 SIM: TX %d bytes\n", len);
	return 0;
#else
	u8 status;

	/* Flush TX FIFO */
	nrf24l01_write_reg(dev, NRF24L01_REG_FLUSH_TX, 0xFF);  /* Flush TX */

	/* Write payload */
	dev->csn_ctrl(0);
	dev->spi_xfer(NRF24L01_CMD_W_TX_PAYLOAD);
	for (u8 i = 0; i < len; i++)
		dev->spi_xfer(data[i]);
	dev->csn_ctrl(1);

	/* Pulse CE to start transmission */
	dev->ce_ctrl(1);
	udelay(15);  /* Minimum CE pulse width */
	dev->ce_ctrl(0);

	/* Wait for TX_DS or MAX_RT */
	u32 timeout = 1000;  /* 1ms max wait */
	while (timeout--) {
		status = nrf24l01_read_reg(dev, NRF24L01_REG_STATUS);
		if (status & ((1 << 5) | (1 << 4)))  /* TX_DS or MAX_RT */
			break;
		udelay(1);
	}

	/* Clear flags */
	nrf24l01_write_reg(dev, NRF24L01_REG_STATUS,
		(1 << 5) | (1 << 4) | (1 << 6));

	if (status & (1 << 4)) {
		/* MAX_RT - max retries reached */
		nrf24l01_flush_tx(dev);
		pr_warn("nRF24L01: TX failed (max retries)\n");
		return -EIO;
	}

	if (!(status & (1 << 5))) {
		pr_warn("nRF24L01: TX timeout\n");
		return -ETIMEDOUT;
	}

	return 0;
#endif
}

int nrf24l01_receive(struct nrf24l01_device *dev, u8 *data, u8 *len)
{
	if (!data || !len)
		return -EINVAL;

#if KINETIS_FAKE_SIM
	/* Simulation: read from RX FIFO (populated externally for testing) */
	if (dev->sim_rx_fifo_len == 0)
		return -EAGAIN;

	u8 rx_len = dev->sim_rx_fifo_len;
	memcpy(data, dev->sim_rx_fifo, rx_len);
	*len = rx_len;
	dev->sim_rx_fifo_len = 0;
	pr_debug("nRF24L01 SIM: RX %d bytes\n", rx_len);
	return rx_len;
#else
	u8 status = nrf24l01_read_reg(dev, NRF24L01_REG_STATUS);

	if (!(status & (1 << 6)))  /* RX_DR not set */
		return -EAGAIN;

	/* Read payload */
	dev->csn_ctrl(0);
	dev->spi_xfer(NRF24L01_CMD_R_RX_PAYLOAD);
	for (u8 i = 0; i < dev->payload_size; i++)
		data[i] = dev->spi_xfer(NRF24L01_CMD_NOP);
	dev->csn_ctrl(1);

	*len = dev->payload_size;

	/* Clear RX_DR flag */
	nrf24l01_write_reg(dev, NRF24L01_REG_STATUS, (1 << 6));

	return *len;
#endif
}

int nrf24l01_data_available(struct nrf24l01_device *dev)
{
#if KINETIS_FAKE_SIM
	return dev->sim_rx_fifo_len > 0 ? 1 : 0;
#else
	u8 fifo = nrf24l01_read_reg(dev, NRF24L01_REG_FIFO_STATUS);
	return !(fifo & (1 << 0));  /* RX_EMPTY bit */
#endif
}

void nrf24l01_flush_tx(struct nrf24l01_device *dev)
{
#if MCU_PLATFORM_STM32
	dev->csn_ctrl(0);
	dev->spi_xfer(NRF24L01_CMD_FLUSH_TX);
	dev->csn_ctrl(1);
#else
	dev->sim_tx_fifo_len = 0;
#endif
}

void nrf24l01_flush_rx(struct nrf24l01_device *dev)
{
#if MCU_PLATFORM_STM32
	dev->csn_ctrl(0);
	dev->spi_xfer(NRF24L01_CMD_FLUSH_RX);
	dev->csn_ctrl(1);
#else
	dev->sim_rx_fifo_len = 0;
#endif
}
