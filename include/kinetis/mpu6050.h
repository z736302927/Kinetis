#ifndef __MPU6050_H
#define __MPU6050_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

/* Register addresses */
#define MPU6050_ADDR                    0x00

/* Self-test registers */
#define SELF_TEST_X_GYRO                0x00
#define SELF_TEST_Y_GYRO                0x01
#define SELF_TEST_Z_GYRO                0x02
#define SELF_TEST_X_ACCEL               0x0D
#define SELF_TEST_Y_ACCEL               0x0E
#define SELF_TEST_Z_ACCEL               0x0F

/* Offset registers */
#define XG_OFFSET_H                     0x13
#define XG_OFFSET_L                     0x14
#define YG_OFFSET_H                     0x15
#define YG_OFFSET_L                     0x16
#define ZG_OFFSET_H                     0x17
#define ZG_OFFSET_L                     0x18

/* Configuration registers */
#define SMPLRT_DIV                      0x19
#define CONFIG                          0x1A
#define GYRO_CONFIG                     0x1B
#define ACCEL_CONFIG                    0x1C
#define ACCEL_CONFIG2                   0x1D
#define LP_ACCEL_ODR                    0x1E
#define WOM_THR                         0x1F

/* FIFO and I2C master control */
#define FIFO_EN                         0x23
#define I2C_MST_CTRL                    0x24

/* I2C slave registers */
#define I2C_SLV0_ADDR                   0x25
#define I2C_SLV0_REG                    0x26
#define I2C_SLV0_CTRL                   0x27
#define I2C_SLV1_ADDR                   0x28
#define I2C_SLV1_REG                    0x29
#define I2C_SLV1_CTRL                   0x2A
#define I2C_SLV2_ADDR                   0x2B
#define I2C_SLV2_REG                    0x2C
#define I2C_SLV2_CTRL                   0x2D
#define I2C_SLV3_ADDR                   0x2E
#define I2C_SLV3_REG                    0x2F
#define I2C_SLV3_CTRL                   0x30
#define I2C_SLV4_ADDR                   0x31
#define I2C_SLV4_REG                    0x32
#define I2C_SLV4_DO                     0x33
#define I2C_SLV4_CTRL                   0x34
#define I2C_SLV4_DI                     0x35

/* Status and interrupt registers */
#define I2C_MST_STATUS                  0x36
#define INT_PIN_CFG                     0x37
#define INT_ENABLE                      0x38
#define INT_STATUS                      0x3A

/* Data output registers */
#define ACCEL_XOUT_H                    0x3B
#define ACCEL_XOUT_L                    0x3C
#define ACCEL_YOUT_H                    0x3D
#define ACCEL_YOUT_L                    0x3E
#define ACCEL_ZOUT_H                    0x3F
#define ACCEL_ZOUT_L                    0x40
#define TEMP_OUT_H                      0x41
#define TEMP_OUT_L                      0x42
#define GYRO_XOUT_H                     0x43
#define GYRO_XOUT_L                     0x44
#define GYRO_YOUT_H                     0x45
#define GYRO_YOUT_L                     0x46
#define GYRO_ZOUT_H                     0x47
#define GYRO_ZOUT_L                     0x48

/* External sensor data */
#define EXT_SENS_DATA_00                0x49
#define EXT_SENS_DATA_01                0x4A
#define EXT_SENS_DATA_02                0x4B
#define EXT_SENS_DATA_03                0x4C
#define EXT_SENS_DATA_04                0x4D
#define EXT_SENS_DATA_05                0x4E
#define EXT_SENS_DATA_06                0x4F
#define EXT_SENS_DATA_07                0x50
#define EXT_SENS_DATA_08                0x51
#define EXT_SENS_DATA_09                0x52
#define EXT_SENS_DATA_10                0x53
#define EXT_SENS_DATA_11                0x54
#define EXT_SENS_DATA_12                0x55
#define EXT_SENS_DATA_13                0x56
#define EXT_SENS_DATA_14                0x57
#define EXT_SENS_DATA_15                0x58
#define EXT_SENS_DATA_16                0x59
#define EXT_SENS_DATA_17                0x5A
#define EXT_SENS_DATA_18                0x5B
#define EXT_SENS_DATA_19                0x5C
#define EXT_SENS_DATA_20                0x5D
#define EXT_SENS_DATA_21                0x5E
#define EXT_SENS_DATA_22                0x5F
#define EXT_SENS_DATA_23                0x60

/* Additional external sensor data definitions for consistency */
#define EXT_SENS_DATA_0C                0x55
#define EXT_SENS_DATA_0D                0x56
#define EXT_SENS_DATA_0E                0x57
#define EXT_SENS_DATA_0F                0x58

/* I2C slave data out registers */
#define I2C_SLV0_DO                     0x63
#define I2C_SLV1_DO                     0x64
#define I2C_SLV2_DO                     0x65
#define I2C_SLV3_DO                     0x66
#define I2C_MST_DELAY_CTRL              0x67
#define SIGNAL_PATH_RESET               0x68
#define MOT_DETECT_CTRL                 0x69
#define USER_CTRL                       0x6A
#define PWR_MGMT_1                      0x6B
#define PWR_MGMT_2                      0x6C

/* FIFO registers */
#define FIFO_COUNTH                     0x72
#define FIFO_COUNTL                     0x73
#define FIFO_R_W                        0x74

/* Device identification */
#define WHO_AM_I                        0x75

/* Accelerometer offset registers */
#define XA_OFFSET_H                     0x77
#define XA_OFFSET_L                     0x78
#define YA_OFFSET_H                     0x7A
#define YA_OFFSET_L                     0x7B
#define ZA_OFFSET_H                     0x7D
#define ZA_OFFSET_L                     0x7E

/* Configuration bit masks */
#define MPU6050_GYRO_FS_250             0x00
#define MPU6050_GYRO_FS_500             0x01
#define MPU6050_GYRO_FS_1000            0x02
#define MPU6050_GYRO_FS_2000            0x03

#define MPU6050_ACCEL_FS_2              0x00
#define MPU6050_ACCEL_FS_4              0x01
#define MPU6050_ACCEL_FS_8              0x02
#define MPU6050_ACCEL_FS_16             0x03

#define MPU6050_CLOCK_INTERNAL          0x00
#define MPU6050_CLOCK_PLL_X             0x01
#define MPU6050_CLOCK_PLL_Y             0x02
#define MPU6050_CLOCK_PLL_Z             0x03
#define MPU6050_CLOCK_EXT_32K           0x04
#define MPU6050_CLOCK_EXT_19M           0x05

#define MPU6050_DLPF_256HZ              0x00
#define MPU6050_DLPF_188HZ              0x01
#define MPU6050_DLPF_98HZ               0x02
#define MPU6050_DLPF_42HZ               0x03
#define MPU6050_DLPF_20HZ               0x04
#define MPU6050_DLPF_10HZ               0x05
#define MPU6050_DLPF_5HZ                0x06

/* Type definitions */
typedef struct {
    int16_t x;
    int16_t y;
    int16_t z;
} mpu6050_raw_data_t;

typedef struct {
    float x;
    float y;
    float z;
} mpu6050_data_t;

typedef struct {
    uint8_t device_present;
    uint8_t gyro_sensitivity;
    uint8_t accel_sensitivity;
    float gyro_scale;
    float accel_scale;
    uint8_t initialized;
} mpu6050_config_t;

/* Self-test functions */
void mpu6050_read_gyro_selftest_reg(uint8_t axis, uint8_t *pdata);
void mpu6050_write_gyro_selftest_reg(uint8_t axis, uint8_t tmp);
void mpu6050_read_accel_selftest_reg(uint8_t axis, uint8_t *pdata);
void mpu6050_write_accel_selftest_reg(uint8_t axis, uint8_t tmp);

/* Offset functions */
void mpu6050_read_gyro_offset_reg(uint8_t axis, uint16_t *pdata);
void mpu6050_write_gyro_offset_reg(uint8_t axis, uint16_t tmp);
void mpu6050_read_accel_offset_reg(uint8_t axis, uint16_t *pdata);
void mpu6050_write_accel_offset_reg(uint8_t axis, uint16_t tmp);

/* Configuration functions */
void mpu6050_sample_rate_divider(uint8_t tmp);
void mpu6050_fifo_mode(uint8_t tmp);
void mpu6050_fsync_set(uint8_t tmp);
void mpu6050_config_dlpf(uint8_t tmp);
void mpu6050_gyro_selftest(uint8_t axis, uint8_t tmp);
void mpu6050_gyro_full_scale_select(uint8_t tmp);
void mpu6050_fchoice_b(uint8_t tmp);
void mpu6050_accel_selftest(uint8_t axis, uint8_t tmp);
void mpu6050_accel_full_scale_select(uint8_t tmp);
void mpu6050_accel_fchoice_b(uint8_t tmp);
void mpu6050_config_accel_dlpf(uint8_t tmp);
void mpu6050_low_power_accel_odr_control(uint8_t tmp);
void mpu6050_wake_on_motion_threshold(uint8_t tmp);

/* FIFO functions */
void mpu6050_fifo_enable_with_temp(uint8_t tmp);
void mpu6050_fifo_enable_with_gyro(uint8_t axis, uint8_t tmp);
void mpu6050_fifo_enable_with_accel(uint8_t tmp);
void mpu6050_fifo_enable_with_ext_sensor(uint8_t slave, uint8_t tmp);
void mpu6050_fifo_count(uint16_t *pdata);
void mpu6050_fifo_read(uint8_t *pdata);
void mpu6050_fifo_write(uint8_t tmp);

/* I2C master functions */
void mpu6050_enable_multi_master(uint8_t tmp);
void mpu6050_wait_for_ext_sensor(uint8_t tmp);
void mpu6050_enable_slave3_fifo(uint8_t tmp);
void mpu6050_i2c_signal_between_read(uint8_t tmp);
void mpu6050_i2c_master_clock(uint8_t tmp);
void mpu6050_i2c_slave_addr(uint8_t slave, uint8_t dir, uint8_t addr);
void mpu6050_i2c_slave_reg(uint8_t slave, uint8_t reg);
void mpu6050_i2c_slave_enable(uint8_t slave, uint8_t tmp);
void mpu6050_i2c_slave_swap_bytes(uint8_t slave, uint8_t tmp);
void mpu6050_i2c_slave_dis_reg(uint8_t slave, uint8_t tmp);
void mpu6050_i2c_slave_group_type(uint8_t slave, uint8_t tmp);
void mpu6050_i2c_slave_number_of_read_bytes(uint8_t slave, uint8_t tmp);
void mpu6050_i2c_slave4_do(uint8_t slave, uint8_t tmp);
void mpu6050_i2c_slave_enable_int(uint8_t tmp);
void mpu6050_i2c_slave_master_delay(uint8_t tmp);
void mpu6050_i2c_slave4_di(uint8_t *pdata);

/* Status functions */
uint8_t mpu6050_status_of_fsync_int(void);
uint8_t mpu6050_slave4_transfer_done(void);
uint8_t mpu6050_slave_looses_arbitration(void);
uint8_t mpu6050_slave_receives_nack(uint8_t slave);
void mpu6050_who_am_i(uint8_t *pdata);

/* Interrupt and pin configuration */
void mpu6050_logic_level_for_int(uint8_t tmp);
void mpu6050_enable_pull_up(uint8_t tmp);
void mpu6050_latch_int_pin(uint8_t tmp);
void mpu6050_int_anyrd2_clear(uint8_t tmp);
void mpu6050_logic_level_for_fsync(uint8_t tmp);
void mpu6050_fsync_int_mode(uint8_t tmp);
void mpu6050_bypass_mode(uint8_t tmp);
void mpu6050_int_for_wake_on_motion(uint8_t tmp);
void mpu6050_int_for_fifo_overflow(uint8_t tmp);
void mpu6050_int_for_fsync(uint8_t tmp);
void mpu6050_int_for_raw_sensor_tmp_ready(uint8_t tmp);
void mpu6050_int_status(uint8_t *pdata);

/* Data acquisition functions */
void mpu6050_accel_measurements(uint16_t *pdata);
void mpu6050_temp_measurement(uint16_t *pdata);
void mpu6050_get_temperature(float *pdata);
void mpu6050_gyro_measurements(uint16_t *pdata);
void mpu6050_get_accel_and_gyro(uint16_t *pdata);
void mpu6050_ExternalSensortmp(uint8_t reg, uint8_t *pdata);

/* Signal path and motion detection */
void mpu6050_delay_shadow_of_ext_sensor(uint8_t tmp);
void mpu6050_i2c_slave_delay_enable(uint8_t slave, uint8_t tmp);
void mpu6050_gyro_signal_path_reset(uint8_t tmp);
void mpu6050_accel_signal_path_reset(uint8_t tmp);
void mpu6050_temp_signal_path_reset(uint8_t tmp);
void mpu6050_enable_wake_on_motion(uint8_t tmp);
void mpu6050_accel_int_mode(uint8_t tmp);

/* User control functions */

void mpu6050_enable_i2c_master(uint8_t tmp);
void mpu6050_disable_i2c_slave(uint8_t tmp);

void mpu6050_reset_i2c_master(uint8_t tmp);
void mpu6050_reset_signal_path(uint8_t tmp);

/* Power management functions */
void mpu6050_soft_reset(uint8_t tmp);
void mpu6050_enter_sleep(uint8_t tmp);
void mpu6050_cycle_sample(uint8_t tmp);
void mpu6050_gyro_standby(uint8_t tmp);
void mpu6050_power_down_ptat(uint8_t tmp);
void mpu6050_clock_source_select(uint8_t tmp);
void mpu6050_accel_disabled(uint8_t axis, uint8_t tmp);
void mpu6050_gyro_disabled(uint8_t axis, uint8_t tmp);

/* Enhanced driver functions */
void mpu6050_init(void);
uint8_t mpu6050_is_device_present(void);
void mpu6050_reset(void);
void mpu6050_set_clock_source(uint8_t clock_source);
void mpu6050_set_gyro_full_scale(uint8_t fs);
void mpu6050_set_accel_full_scale(uint8_t fs);
void mpu6050_set_sample_rate(uint16_t rate);
void mpu6050_set_dlpf_bandwidth(uint8_t bandwidth);
void mpu6050_calibrate_gyro(void);
void mpu6050_calibrate_accel(void);
void mpu6050_get_raw_data(mpu6050_raw_data_t *accel, mpu6050_raw_data_t *gyro, int16_t *temperature);
void mpu6050_get_scaled_data(mpu6050_data_t *accel, mpu6050_data_t *gyro, float *temperature);
void mpu6050_enable_interrupt(uint8_t interrupt);
void mpu6050_disable_interrupt(uint8_t interrupt);
void mpu6050_clear_interrupt_flags(void);
uint8_t mpu6050_check_interrupt_flags(void);
void mpu6050_enter_sleep_mode(void);
void mpu6050_enter_wake_mode(void);
void mpu6050_enter_cycle_mode(void);
void mpu6050_set_wake_motion_threshold(uint8_t threshold);
void mpu6050_enable_wake_motion_interrupt(uint8_t enable);

/* Advanced configuration functions */
void mpu6050_configure_advanced_filters(void);
void mpu6050_set_high_pass_filter(uint8_t enable, uint8_t frequency);
void mpu6050_set_motion_detection_threshold(uint8_t threshold);
void mpu6050_set_zero_motion_detection_threshold(uint8_t threshold);
void mpu6050_set_zero_motion_detection_duration(uint8_t duration);
void mpu6050_enable_free_fall_detection(uint8_t enable);
void mpu6050_set_free_fall_threshold(uint8_t threshold);
void mpu6050_set_free_fall_duration(uint8_t duration);

/* I2C Master mode functions */
uint8_t mpu6050_enable_i2c_master_mode(uint8_t enable);
uint8_t mpu6050_configure_i2c_slave(uint8_t slave_num, uint8_t dev_addr, uint8_t reg_addr, uint8_t rw_flag, uint8_t len);
uint8_t mpu6050_read_i2c_slave_data(uint8_t slave_num, uint8_t *data, uint8_t len);
uint8_t mpu6050_write_i2c_slave_data(uint8_t slave_num, uint8_t *data, uint8_t len);
uint8_t mpu6050_get_i2c_master_status(void);

/* External sensor data functions */
void mpu6050_read_external_sensor_data(uint8_t slave_num, uint8_t reg_addr, uint8_t *data, uint8_t len);
uint8_t mpu6050_check_external_sensor_data_ready(uint8_t slave_num);

/* Advanced motion detection */
void mpu6050_configure_motion_detection(void);
void mpu6050_set_accel_artifact_removal(uint8_t enable);
void mpu6050_set_gyro_threshold(uint8_t axis, uint16_t threshold);

/* Self-test and diagnostic functions */
uint8_t mpu6050_self_test(void);
uint8_t mpu6050_gyro_self_test(void);
uint8_t mpu6050_accel_self_test(void);
uint8_t mpu6050_check_fifo_overflow(void);
void mpu6050_reset_fifo(void);
void mpu6050_enable_fifo(uint8_t enable);
void mpu6050_set_fifo_mode(uint8_t mode);
uint16_t mpu6050_get_fifo_count(void);
uint8_t mpu6050_read_fifo_data(uint8_t *data, uint16_t length);

/* Test function */
void mpu6050_Test(void);

#ifdef __cplusplus
}
#endif

#endif /* __MPU6050_H */
