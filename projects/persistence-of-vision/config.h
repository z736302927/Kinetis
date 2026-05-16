#ifndef POV_CONFIG_H
#define POV_CONFIG_H

#include <kinetis/mavlink.h>

#define POV_IMAGE_PATH                 "0:/pov_images"
#define POV_FIRMWARE_PATH              "0:/firmware"
#define APP_IMAGE_PATH                 "0:/app_images"

#if KINETIS_FAKE_SIM
#define FAKE_IMAGE_COUNT               3
#endif

#define POV_DISPLAY_COLS               720
#define POV_LED_COUNT                  256
#define POV_SPI_GROUPS                 4
#define POV_LEDS_PER_GROUP             (POV_LED_COUNT / POV_SPI_GROUPS)
#define POV_BYTES_PER_GROUP            (POV_LEDS_PER_GROUP * 3)
#define POV_BYTES_PER_COL              (POV_BYTES_PER_GROUP * POV_SPI_GROUPS)

#define POV_TARGET_RPM                 900
#define POV_RPS                        (POV_TARGET_RPM / 60)
#define POV_COL_TIME_US                92

#define POV_SPI_CLOCK_HZ               20000000
#define POV_SPI_SEND_TIME_US           76

#define POV_STABLE_THRESHOLD_RPM       2
#define POV_STABLE_DURATION_MS         1000

#define POV_IMAGE_FILENAME             "pov.img"
#define POV_IMAGE_HEADER_SIZE          sizeof(struct pov_image_header)

#define POV_HALL_PULSES_PER_REV        1
#define POV_TIM_CLOCK_HZ               200000000
#define POV_TIM_PRESCALER              199

#define POV_QSPI_BASE_ADDR             0x90000000

#define POV_HOST_SYSID                 1
#define POV_HOST_COMPID                0
#define POV_SLAVE_SYSID                2
#define POV_SLAVE_COMPID               0

#define POV_MAVLINK_CHAN               MAVLINK_COMM_0

#define POV_HEARTBEAT_INTERVAL_MS      1000
#define POV_STATUS_INTERVAL_MS         100
#define POV_SLAVE_TIMEOUT_MS           3000
#define POV_CMD_RETRY_INTERVAL_MS      500
#define POV_CMD_MAX_RETRIES            3

#define POV_SPEED_QUERY_RETRIES        10
#define POV_SPEED_QUERY_INTERVAL_MS    100

#define POV_MOTOR_ID                   STATOR_MOTOR_ID
#define POV_MOTOR_TARGET_ID            0

/* Status: use mavlink aliases MAVLINK_MOTOR_STOPPED / RUNNING / FAULT */
#define POV_STATUS_STOPPED             MAVLINK_MOTOR_STOPPED
#define POV_STATUS_RUNNING             MAVLINK_MOTOR_RUNNING
#define POV_STATUS_FAULT               MAVLINK_MOTOR_FAULT

/* Direction: use mavlink aliases MAV_MOTOR_DIRECTION_* */
#define POV_DIR_STOP                   MAV_MOTOR_DIRECTION_STOP
#define POV_DIR_FORWARD                MAV_MOTOR_DIRECTION_FORWARD
#define POV_DIR_REVERSE                MAV_MOTOR_DIRECTION_REVERSE

/*********************************************************************
 * PI Controller Parameters (Q8.8 fixed-point)
 *********************************************************************/

#define SLAVE_PID_KP                   512     /* Kp=2.0 */
#define SLAVE_PID_KI                   128     /* Ki=0.5, already includes Ts=10ms */
#define SLAVE_PID_KD                   2560    /* Kd=10.0, already includes 1/Ts */
#define SLAVE_PID_OUTPUT_MAX           25600   /* 100% */
#define SLAVE_PID_OUTPUT_MIN           0
#define SLAVE_PID_INTEGRAL_MAX         12800   /* 50% */
#define SLAVE_PID_INTEGRAL_MIN         (-12800)

#define SLAVE_TARGET_RPM               900
#define SLAVE_PWM_FREQ_HZ              1000
#define SLAVE_PWM_RESOLUTION           1000
#define SLAVE_MOTOR_MAX_RPM            3000
#define SLAVE_MOTOR_DIR_FORWARD        1
#define SLAVE_MOTOR_DIR_REVERSE        0

#define SLAVE_HALL_PULSES_PER_REV      1
#define SLAVE_HALL_TIMEOUT_MS          500

#define SLAVE_PID_PERIOD_MS            10
#define SLAVE_HEARTBEAT_MS             1000
#define SLAVE_STATUS_MS                100

#define SLAVE_STABLE_RPM_THRESH        2
#define SLAVE_STABLE_COUNT_REQ         100

#define SLAVE_SYSID                    2
#define SLAVE_COMPID                   0

#endif
