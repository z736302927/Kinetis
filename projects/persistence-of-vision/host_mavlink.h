/**
 * @file host_mavlink.h
 * @brief Host MAVLink communication interface
 * @note Manages MAVLink connection to slave via nRF24L01,
 *       sending speed commands and receiving slave status/heartbeat
 */

#ifndef POV_HOST_MAVLINK_H
#define POV_HOST_MAVLINK_H

#include <linux/types.h>
#include <kinetis/mavlink.h>
#include "nrf24l01.h"
#include "nrf24l01_serial.h"
#include "mavlink_config.h"

/*********************************************************************
 * Host MAVLink Device Structure
 *********************************************************************/

struct host_mavlink_device {
	struct nrf24l01_device nrf;             /* nRF24L01 radio */
	struct nrf24l01_serial_device nrf_ser;  /* Serial port wrapper */
	struct serial_port *serial;             /* Serial port instance */
	struct mavlink_device *mav;             /* MAVLink device */

	u8 slave_online;           /* 1 if slave heartbeat received */
	u64 last_heartbeat_ms;     /* Last slave heartbeat timestamp */
	s32 slave_rpm;             /* Latest reported slave RPM */
	u8 slave_status;           /* Latest reported slave status */
	s32 target_rpm;            /* Target RPM sent to slave */
	u8 initialized;
};

/*********************************************************************
 * Public API
 *********************************************************************/

/**
 * @brief Initialize host MAVLink communication
 * @param dev: Device pointer
 * @return 0 on success, negative error code on failure
 * @note Sets up nRF24L01, serial port wrapper, and MAVLink device
 */
int host_mavlink_init(struct host_mavlink_device *dev);

/**
 * @brief Send speed command to slave
 * @param dev: Device pointer
 * @param rpm: Target RPM (0 to stop)
 * @return 0 on success, negative error code on failure
 */
int host_mavlink_send_speed_cmd(struct host_mavlink_device *dev, s32 rpm);

/**
 * @brief Process incoming MAVLink messages
 * @param dev: Device pointer
 * @param timeout_ms: Receive timeout in milliseconds
 * @return Number of messages processed, or negative error code
 */
int host_mavlink_process(struct host_mavlink_device *dev, u32 timeout_ms);

/**
 * @brief Check if slave is online
 * @param dev: Device pointer
 * @return 1 if online, 0 if offline
 * @note Slave is considered offline if no heartbeat for POV_SLAVE_TIMEOUT_MS
 */
int host_mavlink_is_slave_online(struct host_mavlink_device *dev);

/**
 * @brief Get slave's current RPM
 * @param dev: Device pointer
 * @return Current RPM, or 0 if no status received
 */
s32 host_mavlink_get_slave_rpm(struct host_mavlink_device *dev);

/**
 * @brief Get slave's current status
 * @param dev: Device pointer
 * @return Status code (POV_STATUS_STOPPED/RUNNING/FAULT)
 */
u8 host_mavlink_get_slave_status(struct host_mavlink_device *dev);

/**
 * @brief Deinitialize host MAVLink
 * @param dev: Device pointer
 */
void host_mavlink_deinit(struct host_mavlink_device *dev);

#endif /* POV_HOST_MAVLINK_H */
