/**
 * @file nrf24l01.h
 * @brief nRF24L01+ wireless module driver
 * @note Provides SPI-based communication with nRF24L01 radio,
 *       with fake simulation support when KINETIS_FAKE_SIM=1
 */

#ifndef POV_NRF24L01_H
#define POV_NRF24L01_H

#include <linux/types.h>

/*********************************************************************
 * nRF24L01 Register Definitions
 *********************************************************************/

#define NRF24L01_REG_CONFIG      0x00
#define NRF24L01_REG_EN_AA       0x01
#define NRF24L01_REG_EN_RXADDR   0x02
#define NRF24L01_REG_SETUP_AW    0x03
#define NRF24L01_REG_SETUP_RETR  0x04
#define NRF24L01_REG_RF_CH       0x05
#define NRF24L01_REG_RF_SETUP    0x06
#define NRF24L01_REG_STATUS      0x07
#define NRF24L01_REG_OBSERVE_TX  0x08
#define NRF24L01_REG_RPD         0x09
#define NRF24L01_REG_RX_ADDR_P0  0x0A
#define NRF24L01_REG_RX_ADDR_P1  0x0B
#define NRF24L01_REG_RX_ADDR_P2  0x0C
#define NRF24L01_REG_RX_ADDR_P3  0x0D
#define NRF24L01_REG_RX_ADDR_P4  0x0E
#define NRF24L01_REG_RX_ADDR_P5  0x0F
#define NRF24L01_REG_TX_ADDR     0x10
#define NRF24L01_REG_RX_PW_P0    0x11
#define NRF24L01_REG_RX_PW_P1    0x12
#define NRF24L01_REG_RX_PW_P2    0x13
#define NRF24L01_REG_RX_PW_P3    0x14
#define NRF24L01_REG_RX_PW_P4    0x15
#define NRF24L01_REG_RX_PW_P5    0x16
#define NRF24L01_REG_FIFO_STATUS 0x17
#define NRF24L01_REG_DYNPD       0x1C
#define NRF24L01_REG_FEATURE     0x1D

/*********************************************************************
 * nRF24L01 Command Definitions
 *********************************************************************/

#define NRF24L01_CMD_R_REGISTER    0x00
#define NRF24L01_CMD_W_REGISTER    0x20
#define NRF24L01_CMD_R_RX_PAYLOAD  0x61
#define NRF24L01_CMD_W_TX_PAYLOAD  0xA0
#define NRF24L01_CMD_FLUSH_TX      0xE1
#define NRF24L01_CMD_FLUSH_RX      0xE2
#define NRF24L01_CMD_REUSE_TX_PL   0xE3
#define NRF24L01_CMD_NOP           0xFF

/*********************************************************************
 * nRF24L01 Constants
 *********************************************************************/

#define NRF24L01_PAYLOAD_SIZE      32
#define NRF24L01_ADDR_WIDTH        5
#define NRF24L01_MAX_CHANNEL       125
#define NRF24L01_MAX_RETRIES       15
#define NRF24L01_MAX_RETRY_DELAY   15  /* (value+1)*250us */

/*********************************************************************
 * Data Rate Definitions
 *********************************************************************/

#define NRF24L01_DATA_RATE_250K    0x20
#define NRF24L01_DATA_RATE_1M      0x00
#define NRF24L01_DATA_RATE_2M      0x08

/*********************************************************************
 * Power Definitions
 *********************************************************************/

#define NRF24L01_POWER_0DBM       0x06
#define NRF24L01_POWER_MINUS6DBM  0x04
#define NRF24L01_POWER_MINUS12DBM 0x02
#define NRF24L01_POWER_MINUS18DBM 0x00

/*********************************************************************
 * SPI Callback Types (hardware abstraction)
 *********************************************************************/

/**
 * @brief SPI transfer callback - sends and receives one byte
 * @param tx_data: Byte to transmit
 * @return Received byte
 */
typedef u8 (*nrf24l01_spi_xfer_cb)(u8 tx_data);

/**
 * @brief GPIO control callback for CE pin
 * @param state: 0=low, 1=high
 */
typedef void (*nrf24l01_ce_cb)(u8 state);

/**
 * @brief GPIO control callback for CSN pin
 * @param state: 0=low (selected), 1=high (deselected)
 */
typedef void (*nrf24l01_csn_cb)(u8 state);

/*********************************************************************
 * nRF24L01 Device Structure
 *********************************************************************/

struct nrf24l01_device {
	nrf24l01_spi_xfer_cb spi_xfer;
	nrf24l01_ce_cb ce_ctrl;
	nrf24l01_csn_cb csn_ctrl;
	u8 channel;
	u8 data_rate;
	u8 power;
	u8 addr_width;
	u8 payload_size;
	u8 auto_ack;
	u8 retransmit_count;
	u8 retransmit_delay;
	u8 tx_pipe_addr[NRF24L01_ADDR_WIDTH];

#if KINETIS_FAKE_SIM
	/* Simulation buffers */
	u8 sim_tx_fifo[NRF24L01_PAYLOAD_SIZE];
	u8 sim_rx_fifo[NRF24L01_PAYLOAD_SIZE];
	u8 sim_tx_fifo_len;
	u8 sim_rx_fifo_len;
	u8 sim_tx_pipe_addr[NRF24L01_ADDR_WIDTH];
	u8 sim_rx_pipe_addr[NRF24L01_ADDR_WIDTH];
	u8 sim_mode;  /* 0=unset, 1=TX, 2=RX */
#endif
};

/*********************************************************************
 * Public API
 *********************************************************************/

/**
 * @brief Initialize nRF24L01 device
 * @param dev: Device pointer
 * @param spi_xfer: SPI transfer callback
 * @param ce_ctrl: CE pin control callback
 * @param csn_ctrl: CSN pin control callback
 * @return 0 on success, negative error code on failure
 */
int nrf24l01_init(struct nrf24l01_device *dev,
	nrf24l01_spi_xfer_cb spi_xfer,
	nrf24l01_ce_cb ce_ctrl,
	nrf24l01_csn_cb csn_ctrl);

/**
 * @brief Set RF channel
 * @param dev: Device pointer
 * @param channel: Channel number (0-125)
 * @return 0 on success, negative error code on failure
 */
int nrf24l01_set_channel(struct nrf24l01_device *dev, u8 channel);

/**
 * @brief Set data rate
 * @param dev: Device pointer
 * @param rate: NRF24L01_DATA_RATE_xxx
 * @return 0 on success, negative error code on failure
 */
int nrf24l01_set_data_rate(struct nrf24l01_device *dev, u8 rate);

/**
 * @brief Set TX/RX address for pipe 0
 * @param dev: Device pointer
 * @param addr: Address bytes (5 bytes default)
 * @param width: Address width (3-5)
 * @return 0 on success, negative error code on failure
 */
int nrf24l01_set_address(struct nrf24l01_device *dev, const u8 *addr, u8 width);

/**
 * @brief Switch to TX mode
 * @param dev: Device pointer
 * @return 0 on success, negative error code on failure
 */
int nrf24l01_tx_mode(struct nrf24l01_device *dev);

/**
 * @brief Switch to RX mode
 * @param dev: Device pointer
 * @return 0 on success, negative error code on failure
 */
int nrf24l01_rx_mode(struct nrf24l01_device *dev);

/**
 * @brief Send data packet
 * @param dev: Device pointer
 * @param data: Data to send (max NRF24L01_PAYLOAD_SIZE bytes)
 * @param len: Data length
 * @return 0 on success, negative error code on failure
 * @note Blocks until transmission completes or fails
 */
int nrf24l01_send(struct nrf24l01_device *dev, const u8 *data, u8 len);

/**
 * @brief Receive data packet
 * @param dev: Device pointer
 * @param data: Buffer for received data
 * @param len: Buffer size, updated with actual received length
 * @return Number of bytes received, or negative error code
 */
int nrf24l01_receive(struct nrf24l01_device *dev, u8 *data, u8 *len);

/**
 * @brief Check if data is available in RX FIFO
 * @param dev: Device pointer
 * @return 1 if data available, 0 if not
 */
int nrf24l01_data_available(struct nrf24l01_device *dev);

/**
 * @brief Flush TX FIFO
 * @param dev: Device pointer
 */
void nrf24l01_flush_tx(struct nrf24l01_device *dev);

/**
 * @brief Flush RX FIFO
 * @param dev: Device pointer
 */
void nrf24l01_flush_rx(struct nrf24l01_device *dev);

#endif /* POV_NRF24L01_H */
