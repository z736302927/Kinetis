/**
 * @file mavlink_config.h
 * @brief MAVLink channel and ID configuration for POV project
 * @note Defines system IDs, component IDs, channel assignments,
 *       and timing parameters for host-slave MAVLink communication
 */

#ifndef POV_MAVLINK_CONFIG_H
#define POV_MAVLINK_CONFIG_H

/*********************************************************************
 * System and Component IDs
 *********************************************************************/

#define POV_HOST_SYSID          1       /* Host (rotating) system ID */
#define POV_HOST_COMPID         0       /* Host component ID */
#define POV_SLAVE_SYSID         2       /* Slave (fixed) system ID */
#define POV_SLAVE_COMPID        0       /* Slave component ID */

/*********************************************************************
 * MAVLink Channel
 *********************************************************************/

#define POV_MAVLINK_CHAN        MAVLINK_COMM_0

/*********************************************************************
 * Communication Timing (ms)
 *********************************************************************/

#define POV_HEARTBEAT_INTERVAL_MS   1000    /* Heartbeat send interval */
#define POV_STATUS_INTERVAL_MS      100     /* Status report interval */
#define POV_SLAVE_TIMEOUT_MS        3000    /* Slave online timeout */
#define POV_CMD_RETRY_INTERVAL_MS   500     /* Command retry interval */
#define POV_CMD_MAX_RETRIES         3       /* Max command retries */

/*********************************************************************
 * POV-specific Motor ID Mapping
 *********************************************************************/

/* Reuse motor_id=0 as the single motor in POV system */
#define POV_MOTOR_ID              0
/* target_id=0 in MOTOR_CONTROL means POV motor */
#define POV_MOTOR_TARGET_ID       0

/*********************************************************************
 * POV Status Codes (mapped to motor status field)
 *********************************************************************/

#define POV_STATUS_STOPPED        0
#define POV_STATUS_RUNNING        1
#define POV_STATUS_FAULT          2

/*********************************************************************
 * POV Direction Codes (mapped to motor direction field)
 *********************************************************************/

#define POV_DIR_STOP             0
#define POV_DIR_FORWARD          1
#define POV_DIR_REVERSE          2

#endif /* POV_MAVLINK_CONFIG_H */
