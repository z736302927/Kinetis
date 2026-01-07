#ifndef __MAX30205_H
#define __MAX30205_H

#ifdef __cplusplus
extern "C" {
#endif

/* The following program is modified by the user according to the hardware device, otherwise the driver cannot run. */

/* Includes ------------------------------------------------------------------*/
#include <linux/types.h>
#include "kinetis/core_common.h"

/* Register addresses */
#define MAX30205_REG_TEMP               0x00
#define MAX30205_REG_CONFIG             0x01
#define MAX30205_REG_THYST              0x02
#define MAX30205_REG_TOS                0x03

/* Configuration register bits */
#define MAX30205_CONFIG_SHUTDOWN_BIT    0x01
#define MAX30205_CONFIG_MODE_BIT        0x02
#define MAX30205_CONFIG_OS_POLARITY_BIT 0x04
#define MAX30205_CONFIG_FAULT_QUEUE_MASK 0x18
#define MAX30205_CONFIG_DATA_FORMAT_BIT 0x20
#define MAX30205_CONFIG_TIMEOUT_BIT     0x40
#define MAX30205_CONFIG_ONE_SHOT_BIT    0x80

/* Fault queue values */
#define MAX30205_FAULT_QUEUE_1          0x00
#define MAX30205_FAULT_QUEUE_2          0x08
#define MAX30205_FAULT_QUEUE_4          0x10
#define MAX30205_FAULT_QUEUE_6          0x18

/* Operating modes */
#define MAX30205_MODE_COMPARATOR        0x00
#define MAX30205_MODE_INTERRUPT         0x01

/* Temperature conversion macros */
#define MAX30205_RAW_TO_CELSIUS(raw)    ((float)(raw) * 0.00390625f)
#define MAX30205_CELSIUS_TO_RAW(celsius) ((u16)((celsius) / 0.00390625f))

/* Device address (7-bit, 0x48 is typical for MAX30205) */
#define MAX30205_I2C_ADDR               0x48

/* Function declarations */
void max30205_init(void);
void max30205_get_temperature(float *ptemperature);
u16 max30205_get_raw_temperature(void);
void max30205_set_shutdown_mode(u8 enable);
void max30205_set_operating_mode(u8 mode);
void max30205_set_os_polarity(u8 polarity);
void max30205_set_fault_queue(u8 fault_count);
void max30205_set_data_format(u8 format);
void max30205_enable_timeout(u8 enable);
void max30205_trigger_one_shot(void);
void max30205_set_threshold_high(u16 threshold_raw);
void max30205_set_threshold_low(u16 threshold_raw);
u16 max30205_get_threshold_high(void);
u16 max30205_get_threshold_low(void);
u8 max30205_check_os_flag(void);
void max30205_clear_os_flag(void);
void max30205_calibrate_offset(float offset_celsius);
float max30205_get_temperature_with_calibration(void);
u8 max30205_is_device_present(void);
void max30205_set_temperature_limits(float min_temp, float max_temp);
u8 max30205_check_temperature_limits(float temperature);
void max30205_register_high_temp_callback(void (*callback)(float temperature));
void max30205_register_low_temp_callback(void (*callback)(float temperature));
void max30205_register_temp_normal_callback(void (*callback)(void));
void max30205_process_temperature_alert(float temperature);

/* Legacy function compatibility */
void max30205_ShutDown(u8 Data);
void max30205_EnterComparatorMode(void);
void max30205_EnterInterruptMode(void);
void max30205_OSPolarity(u8 Data);
void max30205_ConfigFaultQueue(u8 Data);
void max30205_DataFormat(u8 Data);
void max30205_EnableTimeout(u8 Data);
void max30205_OneShot(u8 Data);
void max30205_ReadTHYST(u16 *pdata);
void max30205_WriteTHYST(u16 Data);
void max30205_ReadTOS(u16 *pdata);
void max30205_WriteTOS(u16 Data);

/* The above procedure is modified by the user according to the hardware device, otherwise the driver cannot run. */

#ifdef __cplusplus
}
#endif

#endif /* __MAX30205_H */