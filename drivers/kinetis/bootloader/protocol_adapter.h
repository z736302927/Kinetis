/**
 * @file protocol_adapter.h
 * @brief Protocol adapter layer for Bootloader
 *
 * Provides callback function templates for MAVLink/Zmodem integration.
 * The user fills in the actual protocol implementation in protocol_adapter.c.
 */

#ifndef BOOTLOADER_PROTOCOL_ADAPTER_H
#define BOOTLOADER_PROTOCOL_ADAPTER_H

#include <linux/types.h>

/*********************************************************************
 * Protocol Event Types
 *********************************************************************/

/** Protocol connection status */
enum proto_conn_status {
	PROTO_CONN_DISCONNECTED = 0,
	PROTO_CONN_CONNECTED,
	PROTO_CONN_ERROR,
};

/** Protocol data receive result */
enum proto_recv_result {
	PROTO_RECV_OK       = 0,   /**< Data received successfully */
	PROTO_RECV_TIMEOUT  = -1,  /**< Receive timeout */
	PROTO_RECV_CRC_ERR  = -2,  /**< CRC check failed */
	PROTO_RECV_ERROR    = -3,  /**< General error */
	PROTO_RECV_COMPLETE = 1,   /**< Transfer complete (end flag received) */
};

/** Protocol event callback types */
enum proto_event {
	PROTO_EVENT_CONNECTED,       /**< Connection established */
	PROTO_EVENT_DISCONNECTED,    /**< Connection lost */
	PROTO_EVENT_DATA_RECEIVED,   /**< Data block received */
	PROTO_EVENT_TRANSFER_COMPLETE, /**< All data transferred */
	PROTO_EVENT_ERROR,           /**< Protocol error */
};

/*********************************************************************
 * Protocol Adapter Interface
 *********************************************************************/

/**
 * @brief Initialize the protocol adapter
 * @return 0 on success, negative on error
 * @note User should initialize UART/DMA/MAVLink/Zmodem here
 */
int protocol_adapter_init(void);

/**
 * @brief Deinitialize the protocol adapter
 * @note Release resources, stop reception
 */
void protocol_adapter_deinit(void);

/**
 * @brief Wait for protocol connection to be established
 * @param timeout_ms: Timeout in milliseconds (0 = non-blocking, -1 = forever)
 * @return 0 on connected, -1 on timeout, other negative on error
 */
int protocol_adapter_wait_connection(int timeout_ms);

/**
 * @brief Receive a data block from the protocol layer
 * @param buf: Output buffer for received data
 * @param buf_size: Buffer size in bytes
 * @param received_len: Output actual received length
 * @param timeout_ms: Timeout in milliseconds (-1 = forever)
 * @return proto_recv_result value (PROTO_RECV_OK, PROTO_RECV_COMPLETE, etc.)
 * @note This function should handle protocol-level CRC checking internally.
 *       Only pass up verified data. Return PROTO_RECV_CRC_ERR if CRC fails
 *       and the protocol layer has already requested retransmission.
 */
int protocol_adapter_recv_block(u8 *buf, u32 buf_size,
				u32 *received_len, int timeout_ms);

/**
 * @brief Send an ACK/confirmation message
 * @return 0 on success, negative on error
 */
int protocol_adapter_send_ack(void);

/**
 * @brief Send a NACK/retransmit request
 * @return 0 on success, negative on error
 */
int protocol_adapter_send_nack(void);

/**
 * @brief Report update progress
 * @param percent: Progress percentage (0-100)
 * @param current: Bytes written so far
 * @param total: Total bytes to write
 * @return 0 on success, negative on error
 */
int protocol_adapter_report_progress(u8 percent, u32 current, u32 total);

/**
 * @brief Report update result
 * @param success: 1 if update succeeded, 0 if failed
 * @param crc_expected: Expected CRC32 value (if verification failed)
 * @param crc_actual: Actual CRC32 value (if verification failed)
 * @return 0 on success, negative on error
 */
int protocol_adapter_report_result(u8 success, u32 crc_expected, u32 crc_actual);

/**
 * @brief Send firmware version request or response
 * @return 0 on success, negative on error
 */
int protocol_adapter_send_version(void);

/**
 * @brief Check if protocol connection is active
 * @return 1 if connected, 0 if disconnected
 */
int protocol_adapter_is_connected(void);

#endif /* BOOTLOADER_PROTOCOL_ADAPTER_H */
