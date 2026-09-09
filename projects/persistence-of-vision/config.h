#ifndef POV_CONFIG_H
#define POV_CONFIG_H

#include <kinetis/mavlink.h>

// #include "wiced_tcpip.h"

#define POV_IMAGE_PATH                       "0:/pov_images"
#define POV_LAYER_COUNT                      4
#define POV_FIRMWARE_PATH                    "0:/firmware"
#define APP_IMAGE_PATH                       "0:/app_images"
#define STATOR_UPDATE_DIR                    "0:/computer"
#define PHONE_DOWNLOAD_DIR                   "0:/phone_download"

#define POV_USE_NRF24L01                     0
#define POV_USE_MT6701                       0
#define POV_ANGLE_RESOLUTION                 16384
#define POV_DISPLAY_COLS                     720
#define POV_LED_COUNT                        256
#define POV_SPI_GROUPS                       4
#define POV_LEDS_PER_GROUP                   (POV_LED_COUNT / POV_SPI_GROUPS)
#define POV_BYTES_PER_GROUP                  (POV_LEDS_PER_GROUP * 3)
#define POV_BYTES_PER_COL                    (POV_BYTES_PER_GROUP * POV_SPI_GROUPS)

#define POV_TARGET_RPM                       900
#define POV_COL_TIME_US                      (60000000u / POV_TARGET_RPM / POV_DISPLAY_COLS)

#define POV_SPI_CLOCK_HZ                     20000000
#define POV_LED_DRIVER_LEDS_PER_IC           4
#define POV_LED_DRIVER_IC_PER_GROUP          (POV_LEDS_PER_GROUP / POV_LED_DRIVER_LEDS_PER_IC)
#define POV_LED_DRIVER_PACKET_BYTES          28
#define POV_LED_DRIVER_GROUP_BYTES           (POV_LED_DRIVER_IC_PER_GROUP * POV_LED_DRIVER_PACKET_BYTES)
#define POV_LED_DRIVER_COL_BYTES             (POV_LED_DRIVER_GROUP_BYTES * POV_SPI_GROUPS)
#define POV_LED_DRIVER_ICO_NEAR_MCU          1
#define POV_SPI_SEND_TIME_US                 (((POV_LED_DRIVER_GROUP_BYTES * 8ULL * 1000000ULL) + \
	POV_SPI_CLOCK_HZ - 1) / POV_SPI_CLOCK_HZ)

#if POV_SPI_SEND_TIME_US > POV_COL_TIME_US
#warning "POV: SPI bandwidth below column budget - raise POV_SPI_CLOCK_HZ, or lower POV_LED_COUNT / POV_DISPLAY_COLS"
#endif

#define POV_PREFETCH_COLS                    4
#define POV_PREFETCH_BYTES                   (POV_PREFETCH_COLS * POV_LED_DRIVER_COL_BYTES)

#define POV_IMAGE_FILENAME                   "pov.img"
#define POV_IMAGE_HEADER_SIZE                sizeof(struct pov_image_header)

#define POV_TIM_CLOCK_HZ                     200000000
#define POV_TIM_PRESCALER                    199

#define POV_DEFAULT_ARM_ID                   0
#define POV_ARM_COUNT                        2
#define POV_ARM_BROADCAST                    255
#define POV_RING_ACK_TIMEOUT_MS              10000
#define POV_RING_FILE_TIMEOUT_MS             600000
#define POV_RING_LINK_TIMEOUT_MS             2000
#define POV_RING_IMAGE_APPLY_DELAY_FRAMES    2

#define POV_HOST_SYSID                       1
#define POV_HOST_COMPID                      0
#define POV_SLAVE_SYSID                      2
#define POV_SLAVE_COMPID                     0

#define POV_MAVLINK_CHAN                     MAVLINK_COMM_0

#define POV_HEARTBEAT_INTERVAL_MS            5000
#define POV_STATUS_INTERVAL_MS               100
#define POV_CMD_RETRY_INTERVAL_MS            500
#define POV_CMD_MAX_RETRIES                  3

#define POV_SPEED_QUERY_RETRIES              10
#define POV_SPEED_QUERY_INTERVAL_MS          500

#define POV_MOTOR_ID                         STATOR_MOTOR_ID
#define POV_MOTOR_TARGET_ID                  0

/* Status: use mavlink aliases MAVLINK_MOTOR_STOPPED / RUNNING / FAULT */
#define POV_STATUS_STOPPED                   MAVLINK_MOTOR_STOPPED
#define POV_STATUS_RUNNING                   MAVLINK_MOTOR_RUNNING
#define POV_STATUS_FAULT                     MAVLINK_MOTOR_FAULT

/* Direction: use mavlink aliases MAV_MOTOR_DIRECTION_* */
#define POV_DIR_STOP                         MAV_MOTOR_DIRECTION_STOP
#define POV_DIR_FORWARD                      MAV_MOTOR_DIRECTION_FORWARD
#define POV_DIR_REVERSE                      MAV_MOTOR_DIRECTION_REVERSE

#define SLAVE_PID_KP                         768     /* Kp=3.0   — 略提升，快速响应 */
#define SLAVE_PID_KI                         38      /* Ki=0.15  — 缓积，防过冲 */
#define SLAVE_PID_KD                         26      /* Kd=0.1   — 轻柔阻尼，不再暴力 */
#define SLAVE_PID_OUTPUT_MAX                 10000   /* 对应 motor_set_pwm 上限 */
#define SLAVE_PID_OUTPUT_MIN                 (-10000) /* 支持双向执行机构（符号→方向映射） */
#define SLAVE_PID_INTEGRAL_MAX               5000    /* 稳态 bias ~3000, 留余量 */
#define SLAVE_PID_INTEGRAL_MIN               (-5000) /* 与 INTEGRAL_MAX 对称，支持双向 */
#define SLAVE_PID_D_ALPHA                    192     /* D 项低通系数 (Q8.8, 192≈0.75, 0=冻结, 256=无滤波) */
#define SLAVE_PID_DEADBAND                   3       /* 死区 RPM, |err|<=5 时冻结积分并保持输出 */
#define SLAVE_PID_SLEW_RATE                  200     /* 输出变化率限制 (Q8.8, 0=禁用), 200≈0.78/拍 */
#define SLAVE_PID_RELAY_AMP                  2000    /* 继电器自整定 bang-bang 幅值 */
#define SLAVE_PID_RELAY_CYCLES               5       /* 自整定最少振荡周期数 */
#define SLAVE_PID_RELAY_MIN_HALF_TICKS       20      /* 最短半周期（拍），防止继电切换过快（电机来不及响应） */
#define SLAVE_TUNING_STABLE_THRESH           10      /* 转速误差 < 10 RPM 认为稳定，可触发整定 */
#define SLAVE_TUNING_STABLE_TICKS            50      /* 连续稳定拍数（50×10ms=500ms）确认转速稳定 */

#define SLAVE_TARGET_RPM                     900
#define SLAVE_PWM_FREQ_HZ                    1000
#define SLAVE_PWM_RESOLUTION                 1000
#define SLAVE_MOTOR_MAX_RPM                  3000
#define SLAVE_MOTOR_DIR_FORWARD              1
#define SLAVE_MOTOR_DIR_REVERSE              0

#define SLAVE_HALL_PULSES_PER_REV            1
#define SLAVE_HALL_TIMEOUT_MS                500

#define SLAVE_PID_PERIOD_MS                  10
#define SLAVE_STATUS_MS                      100

#define SLAVE_STABLE_RPM_THRESH              2
#define SLAVE_STABLE_COUNT_REQ               100

#define SLAVE_SYSID                          2
#define SLAVE_COMPID                         0

// #define WIFI_SSID                     "YOUR_HOME_WIFI"       /* Home WiFi SSID */
// #define WIFI_PASSWORD                 "YOUR_PASSWORD"        /* Home WiFi password */
// #define WIFI_HTTP_PORT                80
// #define WIFI_LOG_HOST                 "192.168.1.100"        /* PC IP for log streaming */
// #define WIFI_LOG_PORT                 8888
// #define WIFI_LOG_BUFFER_SIZE          2048
// #define WIFI_POLL_INTERVAL_MS         1
// #define WIFI_STATIC_IP                MAKE_IPV4_ADDRESS(192, 168, 100, 128)
// #define WIFI_STATIC_GW                MAKE_IPV4_ADDRESS(192, 168, 100, 1)
// #define WIFI_STATIC_NETMASK           MAKE_IPV4_ADDRESS(255, 255, 255, 0)

#endif
