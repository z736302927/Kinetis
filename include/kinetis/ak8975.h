#ifndef __AK8975_H
#define __AK8975_H

#ifdef __cplusplus
extern "C" {
#endif

/* The following program is modified by the user according to the hardware device, otherwise the driver cannot run. */

/* Includes ------------------------------------------------------------------*/
#include <linux/types.h>

#include "kinetis/core_common.h"


void ak8975_enter_power_down_mode(void);
void ak8975_enter_single_measurement_mode(void);
void ak8975_enter_continuous_8hz_mode(void);
void ak8975_enter_continuous_10hz_mode(void);
void ak8975_enter_continuous_20hz_mode(void);
void ak8975_enter_continuous_100hz_mode(void);
void ak8975_enter_selftest_mode(void);
void ak8975_enter_fuse_rom_access_mode(void);
void ak8975_who_am_i(u8 *pdata);
void ak8975_device_information(u8 *pdata);
u8 ak8975_data_ready(void);
u8 ak8975_data_overrun(void);
void ak8975_magnetic_measurements(u16 *pdata);
u8 ak8975_magnetic_sensor_overflow(void);
u8 ak8975_output_bit_setting_mirror(void);
void ak8975_operation_mode_setting(u8 tmp);
void ak8975_output_bit_setting(u8 tmp);
void ak8975_soft_reset(u8 tmp);
void ak8975_selftest_control(u8 tmp);
void ak8975_enter_self_test_mode(void);
u8 ak8975_self_test(void);
void ak8975_i2c_disable(void);
void ak8975_i2c_enable(void);
void ak8975_sensitivity_adjustment_values(u8 *pdata);
u8 ak8975_magnetic_adjusted_measurements(u16 *pdata);
void ak8975_init(void);

/* Sleep mode and power management functions */
void ak8975_enter_sleep_mode(void);
void ak8975_wake_up_from_sleep(void);
u8 ak8975_get_power_state(void);

/* Temperature compensation related functions */
s16 ak8975_read_temperature_data(void);
float ak8975_get_temperature_compensation(u16 raw_mag_data, s16 temperature_data);
u16 ak8975_temperature_compensated_measurement(u16 raw_mag_data, s16 temperature_data);
void ak8975_compensated_magnetic_measurements(u16 *pdata);

int ak8975_slave_start();
int ak8975_slave_stop();

/* The above procedure is modified by the user according to the hardware device, otherwise the driver cannot run. */


#ifdef __cplusplus
}
#endif

#endif /* __AK8975_H */
