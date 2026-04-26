/**
 * @file nrf24l01_serial.h
 * @brief nRF24L01 serial_port_ops wrapper for MAVLink transport
 * @note Bridges nRF24L01 radio to the serial_port abstraction,
 *       handling packet fragmentation for MAVLink messages > 32 bytes
 */

#ifndef POV_NRF24L01_SERIAL_H
#define POV_NRF24L01_SERIAL_H

#include <kinetis/serial-port.h>
#include "nrf24l01.h"

/*********************************************************************
 * Fragmentation Protocol
 *********************************************************************/

/*
 * nRF24L01 has a 32-byte payload limit. MAVLink messages can be larger.
 * We use a simple fragmentation scheme:
 *
 * Byte 0:   Fragment header
 *   Bits [7]:   1 = last fragment, 0 = more fragments
 *   Bits [6:0]: Fragment index (0-based)
 * Bytes 1-31:  Fragment data (up to 31 bytes)
 *
 * Total MAVLink message is reassembled across fragments.
 */

#define NRF24L01_FRAG_LAST_BIT    0x80
#define NRF24L01_FRAG_INDEX_MASK  0x7F
#define NRF24L01_FRAG_DATA_SIZE   31  /* 32 - 1 byte header */

/*********************************************************************
 * nRF24L01 Serial Device Structure
 *********************************************************************/

struct nrf24l01_serial_device {
	struct nrf24l01_device *nrf;
	struct serial_port_ops ops;

	/* TX fragmentation state */
	u8 tx_buf[MAVLINK_MAX_PACKET_LEN + 32];  /* TX reassembly buffer */
	u16 tx_len;      /* Total bytes to send */
	u16 tx_sent;     /* Bytes sent so far */

	/* RX reassembly state */
	u8 rx_buf[MAVLINK_MAX_PACKET_LEN + 32];  /* RX reassembly buffer */
	u16 rx_len;      /* Total bytes received */
	u16 rx_read;     /* Bytes read by consumer */

	u8 rx_fragments;  /* Expected fragment count */
	u8 rx_got;        /* Fragments received */
};

/*********************************************************************
 * Public API
 *********************************************************************/

/**
 * @brief Initialize nRF24L01 serial port wrapper
 * @param dev: Serial device to initialize
 * @param nrf: Underlying nRF24L01 device
 * @return 0 on success, negative error code on failure
 */
int nrf24l01_serial_init(struct nrf24l01_serial_device *dev,
	struct nrf24l01_device *nrf);

/**
 * @brief Get the serial_port_ops for this device
 * @param dev: Serial device
 * @return Pointer to serial_port_ops
 */
struct serial_port_ops *nrf24l01_serial_get_ops(struct nrf24l01_serial_device *dev);

/**
 * @brief Inject received data into RX buffer (for simulation)
 * @param dev: Serial device
 * @param data: Data to inject
 * @param len: Data length
 * @return 0 on success, negative error code on failure
 * @note Only used in KINETIS_FAKE_SIM mode
 */
int nrf24l01_serial_inject_rx(struct nrf24l01_serial_device *dev,
	const u8 *data, u16 len);

#endif /* POV_NRF24L01_SERIAL_H */
