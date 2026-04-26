/**
 * @file slave_mavlink.h
 * @brief Slave MAVLink communication interface
 * @note Manages MAVLink connection to host via nRF24L01,
 *       receiving speed commands and sending heartbeat/status
 */

#ifndef POV_SLAVE_MAVLINK_H
#define POV_SLAVE_MAVLINK_H

#include <linux/types.h>
#include <kinetis/mavlink.h>
#include "nrf24l01.h"
#include "nrf24l01_serial.h"
#include "mavlink_config.h"

/*********************************************************************
 * Slave MAVLink Device Structure
 *********************************************************************/

struct slave_mavlink_device {
	struct nrf24l01_device nrf;             /* nRF24L01 radio */
	struct nrf24l01_serial_device nrf_ser;  /* Serial port wrapper */
	struct serial_port *serial;             /* Serial port instance */
	struct mavlink_device *mav;             /* MAVLink device */

	s32 target_rpm;            /* Target RPM from host command */
	u8 target_received;        /* 1 if speed command received */
	u64 last_heartbeat_sent;   /* Last heartbeat send timestamp */
	u64 last_status_sent;      /* Last status send timestamp */
	u8 initialized;
};

/*********************************************************************
 * Public API
 *********************************************************************/

/**
 * @brief Initialize slave MAVLink communication
 * @param dev: Device pointer
 * @return 0 on success, negative error code on failure
 */
int slave_mavlink_init(struct slave_mavlink_device *dev);

/**
 * @brief Send heartbeat to host
 * @param dev: Device pointer
 * @return 0 on success, negative error code on failure
 * @note Should be called at SLAVE_HEARTBEAT_MS intervals
 */
int slave_mavlink_send_heartbeat(struct slave_mavlink_device *dev);

/**
 * @brief Send motor status to host
 * @param dev: Device pointer
 * @param current_rpm: Current measured RPM
 * @param status: Motor status (POV_STATUS_xxx)
 * @return 0 on success, negative error code on failure
 * @note Should be called at SLAVE_STATUS_MS intervals
 */
int slave_mavlink_send_status(struct slave_mavlink_device *dev,
	s32 current_rpm, u8 status);

/**
 * @brief Process incoming MAVLink messages
 * @param dev: Device pointer
 * @param timeout_ms: Receive timeout in milliseconds
 * @return Number of messages processed, or negative error code
 */
int slave_mavlink_process(struct slave_mavlink_device *dev, u32 timeout_ms);

/**
 * @brief Check if a new speed command was received
 * @param dev: Device pointer
 * @return 1 if new command received, 0 otherwise
 * @note Calling this clears the flag
 */
int slave_mavlink_has_new_command(struct slave_mavlink_device *dev);

/**
 * @brief Get the target RPM from the last received command
 * @param dev: Device pointer
 * @return Target RPM, or 0 if no command received
 */
s32 slave_mavlink_get_target_rpm(struct slave_mavlink_device *dev);

/**
 * @brief Deinitialize slave MAVLink
 * @param dev: Device pointer
 */
void slave_mavlink_deinit(struct slave_mavlink_device *dev);

#endif /* POV_SLAVE_MAVLINK_H */
