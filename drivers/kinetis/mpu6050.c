#define pr_fmt(fmt) "mpu6050: " fmt

#include <linux/bitops.h>
#include <linux/printk.h>
#include <linux/delay.h>
#include <linux/err.h>
#include <linux/slab.h>
#include <linux/random.h>
#include <linux/limits.h>

#include "kinetis/mpu6050.h"
#include "kinetis/iic_soft.h"
#include "kinetis/spi_soft.h"
#include "kinetis/regmap-user-bus.h"
#include "kinetis/delay.h"
#include "kinetis/idebug.h"
#include "kinetis/design_verification.h"
#include "kinetis/random-gene.h"
#include "kinetis/test-kinetis.h"

#ifdef KINETIS_FAKE_SIM
#include <pthread.h>
#endif
#include <math.h>

#define X_AXIS                          0x00
#define Y_AXIS                          0x01
#define Z_AXIS                          0x02
#define I2C_SLAVE0                      0x01
#define I2C_SLAVE1                      0x02
#define I2C_SLAVE2                      0x03
#define I2C_SLAVE3                      0x04
#define I2C_SLAVE4                      0x05
#define SELF_TEST_X_GYRO                0x00
#define SELF_TEST_Y_GYRO                0x01
#define SELF_TEST_Z_GYRO                0x02
#define SELF_TEST_X_ACCEL               0x0D
#define SELF_TEST_Y_ACCEL               0x0E
#define SELF_TEST_Z_ACCEL               0x0F
#define XG_OFFSET_H                     0x13
#define XG_OFFSET_L                     0x14
#define YG_OFFSET_H                     0x15
#define YG_OFFSET_L                     0x16
#define ZG_OFFSET_H                     0x17
#define ZG_OFFSET_L                     0x18
#define SMPLRT_DIV                      0x19
#define CONFIG                          0x1A
#define GYRO_CONFIG                     0x1B
#define ACCEL_CONFIG                    0x1C
#define ACCEL_CONFIG2                   0x1D
#define LP_ACCEL_ODR                    0x1E
#define WOM_THR                         0x1F
#define FIFO_EN                         0x23
#define I2C_MST_CTRL                    0x24
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
#define I2C_MST_STATUS                  0x36
#define INT_PIN_CFG                     0x37
#define INT_ENABLE                      0x38
#define INT_STATUS                      0x3A
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
#define FIFO_COUNTH                     0x72
#define FIFO_COUNTL                     0x73
#define FIFO_R_W                        0x74
#define WHO_AM_I                        0x75
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

#define MPU6050_DEVID                	0x68
#define MPU6050_IIC_ADDR                0x68

/* Device structure - matches ak8975/max30205 architecture */
struct mpu6050_device {
	struct regmap *regmap;

	/* Scale factors */
	float gyro_scale;
	float accel_scale;
	u16 temp_sensitivity;

	/* Configuration */
	mpu6050_config_t config;

#ifdef KINETIS_FAKE_SIM
	/* Slave support for testing */
	struct iic_slave *iic_slave;
	struct spi_slave *spi_slave;
	u8 *slave_regs;

	/* Randomization thread support */
	bool thread_running;
#endif
};

u8 mpu6050_read_gyro_selftest_reg(struct mpu6050_device *dev, u8 axis)
{
	u32 tmp;

	switch (axis) {
	case X_AXIS:
		regmap_read(dev->regmap, SELF_TEST_X_GYRO, &tmp);
		break;

	case Y_AXIS:
		regmap_read(dev->regmap, SELF_TEST_Y_GYRO, &tmp);
		break;

	case Z_AXIS:
		regmap_read(dev->regmap, SELF_TEST_Z_GYRO, &tmp);
		break;

	default:
		break;
	}
}

void mpu6050_write_gyro_selftest_reg(struct mpu6050_device *dev, u8 axis, u8 tmp)
{
	switch (axis) {
	case X_AXIS:
		regmap_write(dev->regmap, SELF_TEST_X_GYRO, tmp);
		break;

	case Y_AXIS:
		regmap_write(dev->regmap, SELF_TEST_Y_GYRO, tmp);
		break;

	case Z_AXIS:
		regmap_write(dev->regmap, SELF_TEST_Z_GYRO, tmp);
		break;

	default:
		break;
	}
}

u8 mpu6050_read_accel_selftest_reg(struct mpu6050_device *dev, u8 axis)
{
	u32 tmp;

	switch (axis) {
	case X_AXIS:
		regmap_read(dev->regmap, SELF_TEST_X_ACCEL, &tmp);
		break;

	case Y_AXIS:
		regmap_read(dev->regmap, SELF_TEST_Y_ACCEL, &tmp);
		break;

	case Z_AXIS:
		regmap_read(dev->regmap, SELF_TEST_Z_ACCEL, &tmp);
		break;

	default:
		break;
	}
}

void mpu6050_write_accel_selftest_reg(struct mpu6050_device *dev, u8 axis, u8 tmp)
{
	switch (axis) {
	case X_AXIS:
		regmap_write(dev->regmap, SELF_TEST_X_ACCEL, tmp);
		break;

	case Y_AXIS:
		regmap_write(dev->regmap, SELF_TEST_Y_ACCEL, tmp);
		break;

	case Z_AXIS:
		regmap_write(dev->regmap, SELF_TEST_Z_ACCEL, tmp);
		break;

	default:
		break;
	}
}

u16 mpu6050_read_gyro_offset_reg(struct mpu6050_device *dev, u8 axis)
{
	u32 high8 = 0;
	u32 low8 = 0;

	switch (axis) {
	case X_AXIS:
		regmap_read(dev->regmap, XG_OFFSET_H, &high8);
		regmap_read(dev->regmap, XG_OFFSET_L, &low8);
		break;

	case Y_AXIS:
		regmap_read(dev->regmap, YG_OFFSET_H, &high8);
		regmap_read(dev->regmap, YG_OFFSET_L, &low8);
		break;

	case Z_AXIS:
		regmap_read(dev->regmap, ZG_OFFSET_H, &high8);
		regmap_read(dev->regmap, ZG_OFFSET_L, &low8);
		break;

	default:
		break;
	}

	return (high8 << 8) | low8;
}

void mpu6050_write_gyro_offset_reg(struct mpu6050_device *dev, u8 axis, u16 tmp)
{
	u8 high8 = 0;
	u8 low8 = 0;

	high8 = tmp >> 8;
	low8 = tmp & 0xFF;

	switch (axis) {
	case X_AXIS:
		regmap_write(dev->regmap, XG_OFFSET_H, high8);
		regmap_write(dev->regmap, XG_OFFSET_L, low8);
		break;

	case Y_AXIS:
		regmap_write(dev->regmap, YG_OFFSET_H, high8);
		regmap_write(dev->regmap, YG_OFFSET_L, low8);
		break;

	case Z_AXIS:
		regmap_write(dev->regmap, ZG_OFFSET_H, high8);
		regmap_write(dev->regmap, ZG_OFFSET_L, low8);
		break;

	default:
		break;
	}
}

u16 mpu6050_read_accel_offset_reg(struct mpu6050_device *dev, u8 axis)
{
	u32 high8 = 0;
	u32 low8 = 0;

	switch (axis) {
	case X_AXIS:
		regmap_read(dev->regmap, XA_OFFSET_H, &high8);
		regmap_read(dev->regmap, XA_OFFSET_L, &low8);
		break;

	case Y_AXIS:
		regmap_read(dev->regmap, YA_OFFSET_H, &high8);
		regmap_read(dev->regmap, YA_OFFSET_L, &low8);
		break;

	case Z_AXIS:
		regmap_read(dev->regmap, ZA_OFFSET_H, &high8);
		regmap_read(dev->regmap, ZA_OFFSET_L, &low8);
		break;

	default:
		break;
	}

	return (high8 << 8) | low8;
}

void mpu6050_write_accel_offset_reg(struct mpu6050_device *dev, u8 axis, u16 tmp)
{
	u8 high8 = 0;
	u8 low8 = 0;

	high8 = tmp >> 8;
	low8 = tmp & 0xFF;

	switch (axis) {
	case X_AXIS:
		regmap_write(dev->regmap, XA_OFFSET_H, high8);
		regmap_write(dev->regmap, XA_OFFSET_L, low8);
		break;

	case Y_AXIS:
		regmap_write(dev->regmap, YA_OFFSET_H, high8);
		regmap_write(dev->regmap, YA_OFFSET_L, low8);
		break;

	case Z_AXIS:
		regmap_write(dev->regmap, ZA_OFFSET_H, high8);
		regmap_write(dev->regmap, ZA_OFFSET_L, low8);
		break;

	default:
		break;
	}
}

void mpu6050_sample_rate_divider(struct mpu6050_device *dev, u8 tmp)
{
	regmap_write(dev->regmap, SMPLRT_DIV, tmp);
}

void mpu6050_fifo_mode(struct mpu6050_device *dev, u8 tmp)
{
	u32 reg;

	regmap_read(dev->regmap, CONFIG, &reg);

	__assign_bit(6, (unsigned long *)&reg, tmp);

	regmap_write(dev->regmap, CONFIG, reg);
}

void mpu6050_fsync_set(struct mpu6050_device *dev, u8 tmp)
{
	u32 reg;

	regmap_read(dev->regmap, CONFIG, &reg);

	set_mask_bits(&reg, 0x07 << 3, tmp);

	regmap_write(dev->regmap, CONFIG, reg);
}

void mpu6050_config_dlpf(struct mpu6050_device *dev, u8 tmp)
{
	u32 reg;

	regmap_read(dev->regmap, CONFIG, &reg);

	set_mask_bits(&reg, 0x07, tmp);

	regmap_write(dev->regmap, CONFIG, reg);
}

void mpu6050_gyro_selftest(struct mpu6050_device *dev, u8 axis, u8 tmp)
{
	u32 reg;

	regmap_read(dev->regmap, GYRO_CONFIG, &reg);
	tmp &= 0x01;

	switch (axis) {
	case X_AXIS:
		__assign_bit(7, (unsigned long *)&reg, tmp);
		break;

	case Y_AXIS:
		__assign_bit(6, (unsigned long *)&reg, tmp);
		break;

	case Z_AXIS:
		__assign_bit(5, (unsigned long *)&reg, tmp);
		break;

	default:
		break;
	}

	regmap_write(dev->regmap, GYRO_CONFIG, reg);
}

void mpu6050_gyro_full_scale_select(struct mpu6050_device *dev, u8 tmp)
{
	u32 reg;

	regmap_read(dev->regmap, GYRO_CONFIG, &reg);

	set_mask_bits(&reg, 0x03 << 3, tmp << 3);

	regmap_write(dev->regmap, GYRO_CONFIG, reg);

	/* Update scale factor */
	switch (tmp) {
	case MPU6050_GYRO_FS_250:
		dev->config.gyro_scale = 131.0f;
		break;
	case MPU6050_GYRO_FS_500:
		dev->config.gyro_scale = 65.5f;
		break;
	case MPU6050_GYRO_FS_1000:
		dev->config.gyro_scale = 32.8f;
		break;
	case MPU6050_GYRO_FS_2000:
		dev->config.gyro_scale = 16.4f;
		break;
	default:
		dev->config.gyro_scale = 131.0f;
		break;
	}
}

void mpu6050_fchoice_b(struct mpu6050_device *dev, u8 tmp)
{
	u32 reg;

	regmap_read(dev->regmap, GYRO_CONFIG, &reg);

	set_mask_bits(&reg, 0x03 << 0, tmp << 0);

	regmap_write(dev->regmap, GYRO_CONFIG, reg);
}

void mpu6050_accel_selftest(struct mpu6050_device *dev, u8 axis, u8 tmp)
{
	u32 reg;

	regmap_read(dev->regmap, ACCEL_CONFIG, &reg);
	tmp &= 0x01;

	switch (axis) {
	case X_AXIS:
		__assign_bit(7, (unsigned long *)&reg, tmp);
		break;

	case Y_AXIS:
		__assign_bit(6, (unsigned long *)&reg, tmp);
		break;

	case Z_AXIS:
		__assign_bit(5, (unsigned long *)&reg, tmp);
		break;

	default:
		break;
	}

	regmap_write(dev->regmap, ACCEL_CONFIG, reg);
}

void mpu6050_accel_full_scale_select(struct mpu6050_device *dev, u8 tmp)
{
	u32 reg;

	regmap_read(dev->regmap, ACCEL_CONFIG, &reg);

	set_mask_bits(&reg, 0x03 << 3, tmp << 3);

	regmap_write(dev->regmap, ACCEL_CONFIG, reg);

	/* Update scale factor */
	switch (tmp) {
	case MPU6050_ACCEL_FS_2:
		dev->config.accel_scale = 16384.0f;
		break;
	case MPU6050_ACCEL_FS_4:
		dev->config.accel_scale = 8192.0f;
		break;
	case MPU6050_ACCEL_FS_8:
		dev->config.accel_scale = 4096.0f;
		break;
	case MPU6050_ACCEL_FS_16:
		dev->config.accel_scale = 2048.0f;
		break;
	default:
		dev->config.accel_scale = 16384.0f;
		break;
	}
}

void mpu6050_accel_fchoice_b(struct mpu6050_device *dev, u8 tmp)
{
	u32 reg;

	regmap_read(dev->regmap, ACCEL_CONFIG2, &reg);

	set_mask_bits(&reg, 0x01 << 3, tmp << 3);

	regmap_write(dev->regmap, ACCEL_CONFIG2, reg);
}

void mpu6050_config_accel_dlpf(struct mpu6050_device *dev, u8 tmp)
{
	u32 reg;

	regmap_read(dev->regmap, ACCEL_CONFIG2, &reg);

	set_mask_bits(&reg, 0x07 << 0, tmp << 0);

	regmap_write(dev->regmap, ACCEL_CONFIG2, reg);
}

void mpu6050_low_power_accel_odr_control(struct mpu6050_device *dev, u8 tmp)
{
	u32 reg;

	regmap_read(dev->regmap, LP_ACCEL_ODR, &reg);

	set_mask_bits(&reg, 0x0F << 0, tmp << 0);

	regmap_write(dev->regmap, LP_ACCEL_ODR, reg);
}

void mpu6050_wake_on_motion_threshold(struct mpu6050_device *dev, u8 tmp)
{
	regmap_write(dev->regmap, WOM_THR, tmp);
}

void mpu6050_fifo_enable_with_temp(struct mpu6050_device *dev, u8 tmp)
{
	u32 reg;

	regmap_read(dev->regmap, FIFO_EN, &reg);

	set_mask_bits(&reg, 0x01 << 7, tmp << 7);

	regmap_write(dev->regmap, FIFO_EN, reg);
}

void mpu6050_fifo_enable_with_gyro(struct mpu6050_device *dev, u8 axis, u8 tmp)
{
	u32 reg;

	regmap_read(dev->regmap, FIFO_EN, &reg);
	tmp &= 0x01;

	switch (axis) {
	case X_AXIS:
		__assign_bit(6, (unsigned long *)&reg, tmp);
		break;

	case Y_AXIS:
		__assign_bit(5, (unsigned long *)&reg, tmp);
		break;

	case Z_AXIS:
		__assign_bit(4, (unsigned long *)&reg, tmp);
		break;

	default:
		break;
	}

	regmap_write(dev->regmap, FIFO_EN, reg);
}

void mpu6050_fifo_enable_with_accel(struct mpu6050_device *dev, u8 tmp)
{
	u32 reg;

	regmap_read(dev->regmap, FIFO_EN, &reg);

	set_mask_bits(&reg, 0x01 << 3, tmp << 3);

	regmap_write(dev->regmap, FIFO_EN, reg);
}

void mpu6050_fifo_enable_with_ext_sensor(struct mpu6050_device *dev, u8 slave, u8 tmp)
{
	u32 reg;

	regmap_read(dev->regmap, FIFO_EN, &reg);
	tmp &= 0x01;

	switch (slave) {
	case I2C_SLAVE2:
		__assign_bit(2, (unsigned long *)&reg, tmp);
		break;

	case I2C_SLAVE1:
		__assign_bit(1, (unsigned long *)&reg, tmp);
		break;

	case I2C_SLAVE0:
		__assign_bit(0, (unsigned long *)&reg, tmp);
		break;

	default:
		break;
	}

	regmap_write(dev->regmap, FIFO_EN, reg);
}

void mpu6050_enable_multi_master(struct mpu6050_device *dev, u8 tmp)
{
	u32 reg;

	regmap_read(dev->regmap, I2C_MST_CTRL, &reg);

	set_mask_bits(&reg, 0x01 << 7, tmp << 7);

	regmap_write(dev->regmap, I2C_MST_CTRL, reg);
}

void mpu6050_wait_for_ext_sensor(struct mpu6050_device *dev, u8 tmp)
{
	u32 reg;

	regmap_read(dev->regmap, I2C_MST_CTRL, &reg);

	set_mask_bits(&reg, 0x01 << 6, tmp << 6);

	regmap_write(dev->regmap, I2C_MST_CTRL, reg);
}

void mpu6050_enable_slave3_fifo(struct mpu6050_device *dev, u8 tmp)
{
	u32 reg;

	regmap_read(dev->regmap, I2C_MST_CTRL, &reg);

	set_mask_bits(&reg, 0x01 << 5, tmp << 5);

	regmap_write(dev->regmap, I2C_MST_CTRL, reg);
}

void mpu6050_i2c_signal_between_read(struct mpu6050_device *dev, u8 tmp)
{
	u32 reg;

	regmap_read(dev->regmap, I2C_MST_CTRL, &reg);

	set_mask_bits(&reg, 0x01 << 4, tmp << 4);

	regmap_write(dev->regmap, I2C_MST_CTRL, reg);
}

void mpu6050_i2c_master_clock(struct mpu6050_device *dev, u8 tmp)
{
	u32 reg;

	regmap_read(dev->regmap, I2C_MST_CTRL, &reg);

	set_mask_bits(&reg, 0x0F << 0, tmp << 0);

	regmap_write(dev->regmap, I2C_MST_CTRL, reg);
}

void mpu6050_i2c_slave_addr(struct mpu6050_device *dev, u8 slave, u8 dir, u8 addr)
{
	u8 reg;

	reg |= (dir << 7);
	reg |= (addr << 0);

	switch (slave) {
	case I2C_SLAVE0:
		regmap_write(dev->regmap, I2C_SLV0_ADDR, reg);
		break;

	case I2C_SLAVE1:
		regmap_write(dev->regmap, I2C_SLV1_ADDR, reg);
		break;

	case I2C_SLAVE2:
		regmap_write(dev->regmap, I2C_SLV2_ADDR, reg);
		break;

	case I2C_SLAVE3:
		regmap_write(dev->regmap, I2C_SLV3_ADDR, reg);
		break;

	case I2C_SLAVE4:
		regmap_write(dev->regmap, I2C_SLV4_ADDR, reg);
		break;

	default:
		break;
	}
}

void mpu6050_i2c_slave_reg(struct mpu6050_device *dev, u8 slave, u8 reg)
{
	switch (slave) {
	case I2C_SLAVE0:
		regmap_write(dev->regmap, I2C_SLV0_REG, reg);
		break;

	case I2C_SLAVE1:
		regmap_write(dev->regmap, I2C_SLV1_REG, reg);
		break;

	case I2C_SLAVE2:
		regmap_write(dev->regmap, I2C_SLV2_REG, reg);
		break;

	case I2C_SLAVE3:
		regmap_write(dev->regmap, I2C_SLV3_REG, reg);
		break;

	case I2C_SLAVE4:
		regmap_write(dev->regmap, I2C_SLV4_REG, reg);
		break;

	default:
		break;
	}
}

void mpu6050_i2c_slave_enable(struct mpu6050_device *dev, u8 slave, u8 tmp)
{
	u32 reg;

	tmp &= 0x01;

	switch (slave) {
	case I2C_SLAVE0:
		regmap_read(dev->regmap, I2C_SLV0_CTRL, &reg);
		__assign_bit(7, (unsigned long *)&reg, tmp);
		regmap_write(dev->regmap, I2C_SLV0_CTRL, reg);
		break;

	case I2C_SLAVE1:
		regmap_read(dev->regmap, I2C_SLV1_CTRL, &reg);
		__assign_bit(7, (unsigned long *)&reg, tmp);
		regmap_write(dev->regmap, I2C_SLV1_CTRL, reg);
		break;

	case I2C_SLAVE2:
		regmap_read(dev->regmap, I2C_SLV2_CTRL, &reg);
		__assign_bit(7, (unsigned long *)&reg, tmp);
		regmap_write(dev->regmap, I2C_SLV2_CTRL, reg);
		break;

	case I2C_SLAVE3:
		regmap_read(dev->regmap, I2C_SLV3_CTRL, &reg);
		__assign_bit(7, (unsigned long *)&reg, tmp);
		regmap_write(dev->regmap, I2C_SLV3_CTRL, reg);
		break;

	case I2C_SLAVE4:
		regmap_read(dev->regmap, I2C_SLV4_CTRL, &reg);
		__assign_bit(7, (unsigned long *)&reg, tmp);
		regmap_write(dev->regmap, I2C_SLV4_CTRL, reg);
		break;

	default:
		break;
	}
}

void mpu6050_i2c_slave_swap_bytes(struct mpu6050_device *dev, u8 slave, u8 tmp)
{
	u32 reg;

	tmp &= 0x01;

	switch (slave) {
	case I2C_SLAVE0:
		regmap_read(dev->regmap, I2C_SLV0_CTRL, &reg);
		__assign_bit(6, (unsigned long *)&reg, tmp);
		regmap_write(dev->regmap, I2C_SLV0_CTRL, reg);
		break;

	case I2C_SLAVE1:
		regmap_read(dev->regmap, I2C_SLV1_CTRL, &reg);
		__assign_bit(6, (unsigned long *)&reg, tmp);
		regmap_write(dev->regmap, I2C_SLV1_CTRL, reg);
		break;

	case I2C_SLAVE2:
		regmap_read(dev->regmap, I2C_SLV2_CTRL, &reg);
		__assign_bit(6, (unsigned long *)&reg, tmp);
		regmap_write(dev->regmap, I2C_SLV2_CTRL, reg);
		break;

	case I2C_SLAVE3:
		regmap_read(dev->regmap, I2C_SLV3_CTRL, &reg);
		__assign_bit(6, (unsigned long *)&reg, tmp);
		regmap_write(dev->regmap, I2C_SLV3_CTRL, reg);
		break;

	default:
		break;
	}
}

void mpu6050_i2c_slave_dis_reg(struct mpu6050_device *dev, u8 slave, u8 tmp)
{
	u32 reg;

	tmp &= 0x01;

	switch (slave) {
	case I2C_SLAVE0:
		regmap_read(dev->regmap, I2C_SLV0_CTRL, &reg);
		__assign_bit(5, (unsigned long *)&reg, tmp);
		regmap_write(dev->regmap, I2C_SLV0_CTRL, reg);
		break;

	case I2C_SLAVE1:
		regmap_read(dev->regmap, I2C_SLV1_CTRL, &reg);
		__assign_bit(5, (unsigned long *)&reg, tmp);
		regmap_write(dev->regmap, I2C_SLV1_CTRL, reg);
		break;

	case I2C_SLAVE2:
		regmap_read(dev->regmap, I2C_SLV2_CTRL, &reg);
		__assign_bit(5, (unsigned long *)&reg, tmp);
		regmap_write(dev->regmap, I2C_SLV2_CTRL, reg);
		break;

	case I2C_SLAVE3:
		regmap_read(dev->regmap, I2C_SLV3_CTRL, &reg);
		__assign_bit(5, (unsigned long *)&reg, tmp);
		regmap_write(dev->regmap, I2C_SLV3_CTRL, reg);
		break;

	case I2C_SLAVE4:
		regmap_read(dev->regmap, I2C_SLV4_CTRL, &reg);
		__assign_bit(5, (unsigned long *)&reg, tmp);
		regmap_write(dev->regmap, I2C_SLV4_CTRL, reg);
		break;

	default:
		break;
	}
}

void mpu6050_i2c_slave_group_type(struct mpu6050_device *dev, u8 slave, u8 tmp)
{
	u32 reg;

	tmp &= 0x01;

	switch (slave) {
	case I2C_SLAVE0:
		regmap_read(dev->regmap, I2C_SLV0_CTRL, &reg);
		__assign_bit(4, (unsigned long *)&reg, tmp);
		regmap_write(dev->regmap, I2C_SLV0_CTRL, reg);
		break;

	case I2C_SLAVE1:
		regmap_read(dev->regmap, I2C_SLV1_CTRL, &reg);
		__assign_bit(4, (unsigned long *)&reg, tmp);
		regmap_write(dev->regmap, I2C_SLV1_CTRL, reg);
		break;

	case I2C_SLAVE2:
		regmap_read(dev->regmap, I2C_SLV2_CTRL, &reg);
		__assign_bit(4, (unsigned long *)&reg, tmp);
		regmap_write(dev->regmap, I2C_SLV2_CTRL, reg);
		break;

	case I2C_SLAVE3:
		regmap_read(dev->regmap, I2C_SLV3_CTRL, &reg);
		__assign_bit(4, (unsigned long *)&reg, tmp);
		regmap_write(dev->regmap, I2C_SLV3_CTRL, reg);
		break;

	default:
		break;
	}
}

void mpu6050_i2c_slave_number_of_read_bytes(struct mpu6050_device *dev, u8 slave, u8 tmp)
{
	u32 reg;

	tmp &= 0x0F;

	switch (slave) {
	case I2C_SLAVE0:
		regmap_read(dev->regmap, I2C_SLV0_CTRL, &reg);
		set_mask_bits(&reg, 0x0F << 0, tmp << 0);
		regmap_write(dev->regmap, I2C_SLV0_CTRL, reg);
		break;

	case I2C_SLAVE1:
		regmap_read(dev->regmap, I2C_SLV1_CTRL, &reg);
		set_mask_bits(&reg, 0x0F << 0, tmp << 0);
		regmap_write(dev->regmap, I2C_SLV1_CTRL, reg);
		break;

	case I2C_SLAVE2:
		regmap_read(dev->regmap, I2C_SLV2_CTRL, &reg);
		set_mask_bits(&reg, 0x0F << 0, tmp << 0);
		regmap_write(dev->regmap, I2C_SLV2_CTRL, reg);
		break;

	case I2C_SLAVE3:
		regmap_read(dev->regmap, I2C_SLV3_CTRL, &reg);
		set_mask_bits(&reg, 0x0F << 0, tmp << 0);
		regmap_write(dev->regmap, I2C_SLV3_CTRL, reg);
		break;

	case I2C_SLAVE4:
		regmap_read(dev->regmap, I2C_SLV4_CTRL, &reg);
		set_mask_bits(&reg, 0x0F << 0, tmp << 0);
		regmap_write(dev->regmap, I2C_SLV4_CTRL, reg);
		break;

	default:
		break;
	}
}

void mpu6050_i2c_slave4_do(struct mpu6050_device *dev, u8 slave, u8 tmp)
{
	switch (slave) {
	case I2C_SLAVE0:
		regmap_write(dev->regmap, I2C_SLV0_DO, tmp);
		break;

	case I2C_SLAVE1:
		regmap_write(dev->regmap, I2C_SLV1_DO, tmp);
		break;

	case I2C_SLAVE2:
		regmap_write(dev->regmap, I2C_SLV2_DO, tmp);
		break;

	case I2C_SLAVE3:
		regmap_write(dev->regmap, I2C_SLV3_DO, tmp);
		break;

	case I2C_SLAVE4:
		regmap_write(dev->regmap, I2C_SLV4_DO, tmp);
		break;

	default:
		break;
	}
}

void mpu6050_i2c_slave_enable_int(struct mpu6050_device *dev, u8 tmp)
{
	u32 reg;

	regmap_read(dev->regmap, I2C_SLV4_CTRL, &reg);

	set_mask_bits(&reg, 0x01 << 6, tmp << 6);

	regmap_write(dev->regmap, I2C_SLV4_CTRL, reg);
}

void mpu6050_i2c_slave_master_delay(struct mpu6050_device *dev, u8 tmp)
{
	u32 reg;

	regmap_read(dev->regmap, I2C_SLV4_CTRL, &reg);

	set_mask_bits(&reg, 0x1F << 0, tmp << 0);

	regmap_write(dev->regmap, I2C_SLV4_CTRL, reg);
}

u8 mpu6050_i2c_slave4_di(struct mpu6050_device *dev, u8 *pdata)
{
	u32 reg;

	regmap_read(dev->regmap, I2C_SLV4_DI, &reg);

	return reg;
}

u8 mpu6050_status_of_fsync_int(struct mpu6050_device *dev)
{
	u32 reg;

	regmap_read(dev->regmap, I2C_MST_STATUS, &reg);

	return test_bit(7, (unsigned long *)&reg);
}

u8 mpu6050_slave4_transfer_done(struct mpu6050_device *dev)
{
	u32 reg;

	regmap_read(dev->regmap, I2C_MST_STATUS, &reg);

	return test_bit(6, (unsigned long *)&reg);

}

u8 mpu6050_slave_looses_arbitration(struct mpu6050_device *dev)
{
	u32 reg;

	regmap_read(dev->regmap, I2C_MST_STATUS, &reg);

	return test_bit(5, (unsigned long *)&reg);

}

u8 mpu6050_slave_receives_nack(struct mpu6050_device *dev, u8 slave)
{
	u32 reg;
	u8 val = 0;

	regmap_read(dev->regmap, I2C_MST_STATUS, &reg);

	switch (slave) {
	case I2C_SLAVE4:
		val = test_bit(4, (unsigned long *)&reg);

		break;

	case I2C_SLAVE3:
		val = test_bit(3, (unsigned long *)&reg);

		break;

	case I2C_SLAVE2:
		val = test_bit(2, (unsigned long *)&reg);

		break;

	case I2C_SLAVE1:
		val = test_bit(1, (unsigned long *)&reg);

		break;

	case I2C_SLAVE0:
		val = test_bit(0, (unsigned long *)&reg);

		break;

	default:
		break;
	}

	return val;
}

void mpu6050_logic_level_for_int(struct mpu6050_device *dev, u8 tmp)
{
	u32 reg;

	regmap_read(dev->regmap, INT_PIN_CFG, &reg);

	set_mask_bits(&reg, 0x01 << 7, tmp << 7);

	regmap_write(dev->regmap, INT_PIN_CFG, reg);
}

void mpu6050_enable_pull_up(struct mpu6050_device *dev, u8 tmp)
{
	u32 reg;

	regmap_read(dev->regmap, INT_PIN_CFG, &reg);

	set_mask_bits(&reg, 0x01 << 6, tmp << 6);

	regmap_write(dev->regmap, INT_PIN_CFG, reg);
}

void mpu6050_latch_int_pin(struct mpu6050_device *dev, u8 tmp)
{
	u32 reg;

	regmap_read(dev->regmap, INT_PIN_CFG, &reg);

	set_mask_bits(&reg, 0x01 << 5, tmp << 5);

	regmap_write(dev->regmap, INT_PIN_CFG, reg);
}

void mpu6050_int_anyrd2_clear(struct mpu6050_device *dev, u8 tmp)
{
	u32 reg;

	regmap_read(dev->regmap, INT_PIN_CFG, &reg);

	set_mask_bits(&reg, 0x01 << 4, tmp << 4);

	regmap_write(dev->regmap, INT_PIN_CFG, reg);
}

void mpu6050_logic_level_for_fsync(struct mpu6050_device *dev, u8 tmp)
{
	u32 reg;

	regmap_read(dev->regmap, INT_PIN_CFG, &reg);

	set_mask_bits(&reg, 0x01 << 3, tmp << 3);

	regmap_write(dev->regmap, INT_PIN_CFG, reg);
}

void mpu6050_fsync_int_mode(struct mpu6050_device *dev, u8 tmp)
{
	u32 reg;

	regmap_read(dev->regmap, INT_PIN_CFG, &reg);

	set_mask_bits(&reg, 0x01 << 2, tmp << 2);

	regmap_write(dev->regmap, INT_PIN_CFG, reg);
}

void mpu6050_bypass_mode(struct mpu6050_device *dev, u8 tmp)
{
	u32 reg;

	regmap_read(dev->regmap, INT_PIN_CFG, &reg);

	set_mask_bits(&reg, 0x01 << 1, tmp << 1);

	regmap_write(dev->regmap, INT_PIN_CFG, reg);
}

void mpu6050_int_for_wake_on_motion(struct mpu6050_device *dev, u8 tmp)
{
	u32 reg;

	regmap_read(dev->regmap, INT_ENABLE, &reg);

	set_mask_bits(&reg, 0x01 << 6, tmp << 6);

	regmap_write(dev->regmap, INT_ENABLE, reg);
}

void mpu6050_int_for_fifo_overflow(struct mpu6050_device *dev, u8 tmp)
{
	u32 reg;

	regmap_read(dev->regmap, INT_ENABLE, &reg);

	set_mask_bits(&reg, 0x01 << 4, tmp << 4);

	regmap_write(dev->regmap, INT_ENABLE, reg);
}

void mpu6050_int_for_fsync(struct mpu6050_device *dev, u8 tmp)
{
	u32 reg;

	regmap_read(dev->regmap, INT_ENABLE, &reg);

	set_mask_bits(&reg, 0x01 << 3, tmp << 3);

	regmap_write(dev->regmap, INT_ENABLE, reg);
}

void mpu6050_int_for_raw_sensor_tmp_ready(struct mpu6050_device *dev, u8 tmp)
{
	u32 reg;

	regmap_read(dev->regmap, INT_ENABLE, &reg);

	set_mask_bits(&reg, 0x01 << 0, tmp << 0);

	regmap_write(dev->regmap, INT_ENABLE, reg);
}

u8 mpu6050_int_status(struct mpu6050_device *dev, u8 *pdata)
{
	u32 reg;

	regmap_read(dev->regmap, INT_STATUS, &reg);

	return reg;
}

void mpu6050_accel_measurements(struct mpu6050_device *dev, u16 *pdata)
{
	u8 val[6];

	regmap_bulk_read(dev->regmap, ACCEL_XOUT_H, val, 6);

	pdata[0] = (val[0] << 8) | val[1];
	pdata[1] = (val[2] << 8) | val[3];
	pdata[2] = (val[4] << 8) | val[5];
}

void mpu6050_temp_measurement(struct mpu6050_device *dev, u16 *pdata)
{
	u8 val[2];

	regmap_bulk_read(dev->regmap, TEMP_OUT_H, val, 2);

	pdata[0] = (val[0] << 8) | val[1];
}

float mpu6050_get_temperature(struct mpu6050_device *dev)
{
	u16 val;

	mpu6050_temp_measurement(dev, &val);

	return ((val - 0) / dev->temp_sensitivity) + 21;
}

void mpu6050_gyro_measurements(struct mpu6050_device *dev, u16 *pdata)
{
	u8 val[6];

	regmap_bulk_read(dev->regmap, GYRO_XOUT_H, val, 6);

	pdata[0] = (val[0] << 8) | val[1];
	pdata[1] = (val[2] << 8) | val[3];
	pdata[2] = (val[4] << 8) | val[5];
}

void mpu6050_get_accel_and_gyro(struct mpu6050_device *dev, u16 *pdata)
{
	u8 val[14];

	regmap_bulk_read(dev->regmap, ACCEL_XOUT_H, val, 14);

	pdata[0] = (val[0] << 8) | val[1];
	pdata[1] = (val[2] << 8) | val[3];
	pdata[2] = (val[4] << 8) | val[5];

	pdata[3] = (val[8] << 8) | val[9];
	pdata[4] = (val[10] << 8) | val[11];
	pdata[5] = (val[12] << 8) | val[13];
}

void mpu6050_delay_shadow_of_ext_sensor(struct mpu6050_device *dev, u8 tmp)
{
	u32 reg;

	regmap_read(dev->regmap, I2C_MST_DELAY_CTRL, &reg);

	set_mask_bits(&reg, 0x01 << 7, tmp << 7);

	regmap_write(dev->regmap, I2C_MST_DELAY_CTRL, reg);
}

void mpu6050_i2c_slave_delay_enable(struct mpu6050_device *dev, u8 slave, u8 tmp)
{
	u32 reg;

	tmp &= 0x01;

	switch (slave) {
	case I2C_SLAVE0:
		regmap_read(dev->regmap, I2C_MST_DELAY_CTRL, &reg);
		set_mask_bits(&reg, 0x01 << 0, tmp << 0);
		regmap_write(dev->regmap, I2C_MST_DELAY_CTRL, reg);
		break;

	case I2C_SLAVE1:
		regmap_read(dev->regmap, I2C_MST_DELAY_CTRL, &reg);
		set_mask_bits(&reg, 0x01 << 1, tmp << 1);
		regmap_write(dev->regmap, I2C_MST_DELAY_CTRL, reg);
		break;

	case I2C_SLAVE2:
		regmap_read(dev->regmap, I2C_MST_DELAY_CTRL, &reg);
		set_mask_bits(&reg, 0x01 << 2, tmp << 2);
		regmap_write(dev->regmap, I2C_MST_DELAY_CTRL, reg);
		break;

	case I2C_SLAVE3:
		regmap_read(dev->regmap, I2C_MST_DELAY_CTRL, &reg);
		set_mask_bits(&reg, 0x01 << 3, tmp << 3);
		regmap_write(dev->regmap, I2C_MST_DELAY_CTRL, reg);
		break;

	case I2C_SLAVE4:
		regmap_read(dev->regmap, I2C_MST_DELAY_CTRL, &reg);
		set_mask_bits(&reg, 0x01 << 4, tmp << 4);
		regmap_write(dev->regmap, I2C_MST_DELAY_CTRL, reg);
		break;

	default:
		break;
	}
}

void mpu6050_gyro_signal_path_reset(struct mpu6050_device *dev, u8 tmp)
{
	u32 reg;

	regmap_read(dev->regmap, SIGNAL_PATH_RESET, &reg);

	set_mask_bits(&reg, 0x01 << 2, tmp << 2);

	regmap_write(dev->regmap, SIGNAL_PATH_RESET, reg);
}

void mpu6050_accel_signal_path_reset(struct mpu6050_device *dev, u8 tmp)
{
	u32 reg;

	regmap_read(dev->regmap, SIGNAL_PATH_RESET, &reg);

	set_mask_bits(&reg, 0x01 << 1, tmp << 1);

	regmap_write(dev->regmap, SIGNAL_PATH_RESET, reg);
}

void mpu6050_temp_signal_path_reset(struct mpu6050_device *dev, u8 tmp)
{
	u32 reg;

	regmap_read(dev->regmap, SIGNAL_PATH_RESET, &reg);

	set_mask_bits(&reg, 0x01 << 0, tmp << 0);

	regmap_write(dev->regmap, SIGNAL_PATH_RESET, reg);
}

void mpu6050_enable_wake_on_motion(struct mpu6050_device *dev, u8 tmp)
{
	u32 reg;

	regmap_read(dev->regmap, MOT_DETECT_CTRL, &reg);

	set_mask_bits(&reg, 0x01 << 7, tmp << 7);

	regmap_write(dev->regmap, MOT_DETECT_CTRL, reg);
}

void mpu6050_accel_int_mode(struct mpu6050_device *dev, u8 tmp)
{
	u32 reg;

	regmap_read(dev->regmap, MOT_DETECT_CTRL, &reg);

	set_mask_bits(&reg, 0x01 << 6, tmp << 6);

	regmap_write(dev->regmap, MOT_DETECT_CTRL, reg);
}

void mpu6050_enable_fifo_raw(struct mpu6050_device *dev, u8 tmp)
{
	u32 reg;

	regmap_read(dev->regmap, USER_CTRL, &reg);

	set_mask_bits(&reg, 0x01 << 6, tmp << 6);

	regmap_write(dev->regmap, USER_CTRL, reg);
}

void mpu6050_enable_i2c_master(struct mpu6050_device *dev, u8 tmp)
{
	u32 reg;

	regmap_read(dev->regmap, USER_CTRL, &reg);

	set_mask_bits(&reg, 0x01 << 5, tmp << 5);

	regmap_write(dev->regmap, USER_CTRL, reg);
}

void mpu6050_disable_i2c_slave(struct mpu6050_device *dev, u8 tmp)
{
	u32 reg;

	regmap_read(dev->regmap, USER_CTRL, &reg);

	set_mask_bits(&reg, 0x01 << 4, tmp << 4);

	regmap_write(dev->regmap, USER_CTRL, reg);
}

void mpu6050_reset_fifo_raw(struct mpu6050_device *dev, u8 tmp)
{
	u32 reg;

	regmap_read(dev->regmap, USER_CTRL, &reg);

	set_mask_bits(&reg, 0x01 << 2, tmp << 2);

	regmap_write(dev->regmap, USER_CTRL, reg);
}

void mpu6050_reset_i2c_master(struct mpu6050_device *dev, u8 tmp)
{
	u32 reg;

	regmap_read(dev->regmap, USER_CTRL, &reg);

	set_mask_bits(&reg, 0x01 << 1, tmp << 1);

	regmap_write(dev->regmap, USER_CTRL, reg);
}

void mpu6050_reset_signal_path(struct mpu6050_device *dev, u8 tmp)
{
	u32 reg;

	regmap_read(dev->regmap, USER_CTRL, &reg);

	set_mask_bits(&reg, 0x01 << 0, tmp << 0);

	regmap_write(dev->regmap, USER_CTRL, reg);
}

void mpu6050_soft_reset(struct mpu6050_device *dev, u8 tmp)
{
	u32 reg;

	regmap_read(dev->regmap, PWR_MGMT_1, &reg);

	set_mask_bits(&reg, 0x01 << 7, tmp << 7);

	regmap_write(dev->regmap, PWR_MGMT_1, reg);
}

void mpu6050_enter_sleep(struct mpu6050_device *dev, u8 tmp)
{
	u32 reg;

	regmap_read(dev->regmap, PWR_MGMT_1, &reg);

	set_mask_bits(&reg, 0x01 << 6, tmp << 6);

	regmap_write(dev->regmap, PWR_MGMT_1, reg);
}

void mpu6050_cycle_sample(struct mpu6050_device *dev, u8 tmp)
{
	u32 reg;

	regmap_read(dev->regmap, PWR_MGMT_1, &reg);

	set_mask_bits(&reg, 0x01 << 5, tmp << 5);

	regmap_write(dev->regmap, PWR_MGMT_1, reg);
}

void mpu6050_gyro_standby(struct mpu6050_device *dev, u8 tmp)
{
	u32 reg;

	regmap_read(dev->regmap, PWR_MGMT_1, &reg);

	set_mask_bits(&reg, 0x01 << 4, tmp << 4);

	regmap_write(dev->regmap, PWR_MGMT_1, reg);
}

void mpu6050_power_down_ptat(struct mpu6050_device *dev, u8 tmp)
{
	u32 reg;

	regmap_read(dev->regmap, PWR_MGMT_1, &reg);

	set_mask_bits(&reg, 0x01 << 3, tmp << 3);

	regmap_write(dev->regmap, PWR_MGMT_1, reg);
}

void mpu6050_clock_source_select(struct mpu6050_device *dev, u8 tmp)
{
	u32 reg;

	regmap_read(dev->regmap, PWR_MGMT_1, &reg);

	set_mask_bits(&reg, 0x07 << 0, tmp << 0);

	regmap_write(dev->regmap, PWR_MGMT_1, reg);
}

void mpu6050_accel_disabled(struct mpu6050_device *dev, u8 axis, u8 tmp)
{
	u32 reg;

	regmap_read(dev->regmap, PWR_MGMT_2, &reg);
	tmp &= 0x01;

	switch (axis) {
	case X_AXIS:
		__assign_bit(5, (unsigned long *)&reg, tmp);
		break;

	case Y_AXIS:
		__assign_bit(4, (unsigned long *)&reg, tmp);
		break;

	case Z_AXIS:
		__assign_bit(3, (unsigned long *)&reg, tmp);
		break;

	default:
		break;
	}

	regmap_write(dev->regmap, PWR_MGMT_2, reg);
}

void mpu6050_gyro_disabled(struct mpu6050_device *dev, u8 axis, u8 tmp)
{
	u32 reg;

	regmap_read(dev->regmap, PWR_MGMT_2, &reg);
	tmp &= 0x01;

	switch (axis) {
	case X_AXIS:
		__assign_bit(2, (unsigned long *)&reg, tmp);
		break;

	case Y_AXIS:
		__assign_bit(1, (unsigned long *)&reg, tmp);
		break;

	case Z_AXIS:
		__assign_bit(0, (unsigned long *)&reg, tmp);
		break;

	default:
		break;
	}

	regmap_write(dev->regmap, PWR_MGMT_2, reg);
}

void mpu6050_fifo_count(struct mpu6050_device *dev, u16 *pdata)
{
	u8 reg[2];

	regmap_bulk_read(dev->regmap, FIFO_COUNTH, reg, 2);

	reg[0] &= 0x1F;
	pdata[0] = (reg[0] << 8) | reg[1];
}

u8 mpu6050_fifo_read(struct mpu6050_device *dev)
{
	u32 reg;

	regmap_read(dev->regmap, FIFO_R_W, &reg);

	return reg;
}

void mpu6050_fifo_write(struct mpu6050_device *dev, u8 tmp)
{
	regmap_write(dev->regmap, FIFO_R_W, tmp);
}

u8 mpu6050_who_am_i(struct mpu6050_device *dev)
{
	u32 reg;

	regmap_read(dev->regmap, WHO_AM_I, &reg);

	return reg;
}

u16 mpu9250_read_accel_offset_reg(struct mpu6050_device *dev, u8 axis)
{
	u32 high8 = 0;
	u32 low8 = 0;

	switch (axis) {
	case X_AXIS:
		regmap_read(dev->regmap, XA_OFFSET_H, &high8);
		regmap_read(dev->regmap, XA_OFFSET_L, &low8);
		break;

	case Y_AXIS:
		regmap_read(dev->regmap, YA_OFFSET_H, &high8);
		regmap_read(dev->regmap, YA_OFFSET_L, &low8);
		break;

	case Z_AXIS:
		regmap_read(dev->regmap, ZA_OFFSET_H, &high8);
		regmap_read(dev->regmap, ZA_OFFSET_L, &low8);
		break;

	default:
		break;
	}

	return (high8 << 7) | (low8 >> 1);
}

void mpu9250_write_accel_offset_reg(struct mpu6050_device *dev, u8 axis, u16 tmp)
{
	u8 high8 = 0;
	u8 low8 = 0;

	high8 = tmp >> 7;
	low8 = (tmp & 0xFE) >> 1;

	switch (axis) {
	case X_AXIS:
		regmap_write(dev->regmap, XA_OFFSET_H, high8);
		regmap_write(dev->regmap, XA_OFFSET_L, low8);
		break;

	case Y_AXIS:
		regmap_write(dev->regmap, YA_OFFSET_H, high8);
		regmap_write(dev->regmap, YA_OFFSET_L, low8);
		break;

	case Z_AXIS:
		regmap_write(dev->regmap, ZA_OFFSET_H, high8);
		regmap_write(dev->regmap, ZA_OFFSET_L, low8);
		break;

	default:
		break;
	}
}

u8 mpu6050_is_device_present(struct mpu6050_device *dev)
{
	/* mpu6050 should return 0x68 */
	if (mpu6050_who_am_i(dev) == 0x68) {
		return 1;
	}

	return 0;
}

void mpu6050_set_sample_rate(struct mpu6050_device *dev, u16 rate)
{
	u8 divider;

	if (rate > 1000) {
		rate = 1000;
	}
	if (rate < 1) {
		rate = 1;
	}

	/* Calculate divider: sample_rate = 1000 / (divider + 1) */
	divider = (1000 / rate) - 1;

	mpu6050_sample_rate_divider(dev, divider);
}

void mpu6050_get_raw_data(struct mpu6050_device *dev, mpu6050_raw_data_t *accel, mpu6050_raw_data_t *gyro, s16 *temperature)
{
	u16 raw_data[7]; /* accel[3], temp[1], gyro[3] */

	/* Read all sensor data in one transaction */
	mpu6050_get_accel_and_gyro(dev, raw_data);

	/* Extract accelerometer data */
	accel->x = (s16)raw_data[0];
	accel->y = (s16)raw_data[1];
	accel->z = (s16)raw_data[2];

	/* Extract temperature data */
	*temperature = (s16)raw_data[3];

	/* Extract gyroscope data */
	gyro->x = (s16)raw_data[4];
	gyro->y = (s16)raw_data[5];
	gyro->z = (s16)raw_data[6];
}

void mpu6050_get_scaled_data(struct mpu6050_device *dev, mpu6050_data_t *accel, mpu6050_data_t *gyro, float *temperature)
{
	mpu6050_raw_data_t raw_accel, raw_gyro;
	s16 raw_temp;

	/* Get raw data */
	mpu6050_get_raw_data(dev, &raw_accel, &raw_gyro, &raw_temp);

	/* Convert to scaled data */
	accel->x = (float)raw_accel.x / dev->config.accel_scale;
	accel->y = (float)raw_accel.y / dev->config.accel_scale;
	accel->z = (float)raw_accel.z / dev->config.accel_scale;

	gyro->x = (float)raw_gyro.x / dev->config.gyro_scale;
	gyro->y = (float)raw_gyro.y / dev->config.gyro_scale;
	gyro->z = (float)raw_gyro.z / dev->config.gyro_scale;

	/* Convert temperature to Celsius: Temp = (Temp_OUT - 521) / 340 + 35.0 */
	*temperature = ((float)raw_temp - 521.0f) / 340.0f + 35.0f;
}

void mpu6050_calibrate_gyro(struct mpu6050_device *dev)
{
	mpu6050_raw_data_t accel, gyro;
	s16 temperature;
	int32_t gyro_x_sum = 0, gyro_y_sum = 0, gyro_z_sum = 0;
	u16 samples = 1000;
	u16 i;
	s16 offset_x, offset_y, offset_z;

	pr_info("starting gyroscope calibration...");
	pr_info("please keep device stationary for 10 seconds");

	/* Collect samples */
	for (i = 0; i < samples; i++) {
		mpu6050_get_raw_data(dev, &accel, &gyro, &temperature);
		gyro_x_sum += gyro.x;
		gyro_y_sum += gyro.y;
		gyro_z_sum += gyro.z;

		mdelay(10);
	}

	/* Calculate average offsets */
	offset_x = -(gyro_x_sum / samples);
	offset_y = -(gyro_y_sum / samples);
	offset_z = -(gyro_z_sum / samples);

	/* Store offsets in device */
	mpu6050_write_gyro_offset_reg(dev, X_AXIS, (u16)offset_x);
	mpu6050_write_gyro_offset_reg(dev, Y_AXIS, (u16)offset_y);
	mpu6050_write_gyro_offset_reg(dev, Z_AXIS, (u16)offset_z);

	pr_info("gyroscope calibration completed");
	pr_info("offsets - x: %d, y: %d, z: %d", offset_x, offset_y, offset_z);
}

void mpu6050_calibrate_accel(struct mpu6050_device *dev)
{
	mpu6050_raw_data_t accel, gyro;
	s16 temperature;
	int32_t accel_x_sum = 0, accel_y_sum = 0, accel_z_sum = 0;
	u16 samples = 100;
	u16 i;
	s16 offset_x, offset_y, offset_z;

	pr_info("starting accelerometer calibration...");
	pr_info("place device in 6 different orientations and call this function");

	/* Collect samples */
	for (i = 0; i < samples; i++) {
		mpu6050_get_raw_data(dev, &accel, &gyro, &temperature);
		accel_x_sum += accel.x;
		accel_y_sum += accel.y;
		accel_z_sum += accel.z;

		mdelay(10);
	}

	/* Calculate average offsets (assuming device is level) */
	offset_x = -(accel_x_sum / samples);
	offset_y = -(accel_y_sum / samples);
	offset_z = -(accel_z_sum / samples - 16384); /* 1g = 16384 LSB for ±2g range */

	/* Store offsets in device */
	mpu9250_write_accel_offset_reg(dev, X_AXIS, (u16)offset_x);
	mpu9250_write_accel_offset_reg(dev, Y_AXIS, (u16)offset_y);
	mpu9250_write_accel_offset_reg(dev, Z_AXIS, (u16)offset_z);

	pr_info("accelerometer calibration completed");
	pr_info("offsets - x: %d, y: %d, z: %d", offset_x, offset_y, offset_z);
}

void mpu6050_enable_interrupt(struct mpu6050_device *dev, u8 interrupt)
{
	mpu6050_int_for_raw_sensor_tmp_ready(dev, interrupt & 0x01);
	mpu6050_int_for_fsync(dev, (interrupt >> 3) & 0x01);
	mpu6050_int_for_fifo_overflow(dev, (interrupt >> 4) & 0x01);
	mpu6050_int_for_wake_on_motion(dev, (interrupt >> 6) & 0x01);
}

void mpu6050_disable_interrupt(struct mpu6050_device *dev, u8 interrupt)
{
	mpu6050_int_for_raw_sensor_tmp_ready(dev, !(interrupt & 0x01));
	mpu6050_int_for_fsync(dev, !((interrupt >> 3) & 0x01));
	mpu6050_int_for_fifo_overflow(dev, !((interrupt >> 4) & 0x01));
	mpu6050_int_for_wake_on_motion(dev, !((interrupt >> 6) & 0x01));
}

void mpu6050_clear_interrupt_flags(struct mpu6050_device *dev)
{
	/* Read status register to clear flags */
	u8 status;
	mpu6050_int_status(dev, &status);
}

u8 mpu6050_check_interrupt_flags(struct mpu6050_device *dev)
{
	u8 status;
	mpu6050_int_status(dev, &status);
	return status;
}

void mpu6050_enter_cycle_mode(struct mpu6050_device *dev)
{
	mpu6050_cycle_sample(dev, 1);
	mpu6050_gyro_standby(dev, 1);
}

void mpu6050_enable_wake_motion_interrupt(struct mpu6050_device *dev, u8 enable)
{
	mpu6050_enable_wake_on_motion(dev, enable);
	mpu6050_int_for_wake_on_motion(dev, enable);
}

u8 mpu6050_gyro_self_test(struct mpu6050_device *dev)
{
	u8 gyro_x_orig, gyro_y_orig, gyro_z_orig;
	u8 gyro_x_test, gyro_y_test, gyro_z_test;
	u16 axis_offset[3];
	int32_t test_values[3];
	u8 i;

	pr_info("testing gyroscope...");

	/* Save original offset values */
	axis_offset[0] = mpu6050_read_gyro_offset_reg(dev, X_AXIS);
	axis_offset[1] = mpu6050_read_gyro_offset_reg(dev, Y_AXIS);
	axis_offset[2] = mpu6050_read_gyro_offset_reg(dev, Z_AXIS);

	/* Enable self-test for all axes */
	mpu6050_gyro_selftest(dev, X_AXIS, 1);
	mpu6050_gyro_selftest(dev, Y_AXIS, 1);
	mpu6050_gyro_selftest(dev, Z_AXIS, 1);

	mdelay(100);

	/* Read self-test values */
	gyro_x_test = mpu6050_read_gyro_selftest_reg(dev, X_AXIS);
	gyro_y_test = mpu6050_read_gyro_selftest_reg(dev, Y_AXIS);
	gyro_z_test = mpu6050_read_gyro_selftest_reg(dev, Z_AXIS);

	/* Disable self-test */
	mpu6050_gyro_selftest(dev, X_AXIS, 0);
	mpu6050_gyro_selftest(dev, Y_AXIS, 0);
	mpu6050_gyro_selftest(dev, Z_AXIS, 0);

	mdelay(100);

	/* Read normal operation values */
	gyro_x_orig = mpu6050_read_gyro_selftest_reg(dev, X_AXIS);
	gyro_y_orig = mpu6050_read_gyro_selftest_reg(dev, Y_AXIS);
	gyro_z_orig = mpu6050_read_gyro_selftest_reg(dev, Z_AXIS);

	/* Restore original offsets */
	mpu6050_write_gyro_offset_reg(dev, X_AXIS, axis_offset[0]);
	mpu6050_write_gyro_offset_reg(dev, Y_AXIS, axis_offset[1]);
	mpu6050_write_gyro_offset_reg(dev, Z_AXIS, axis_offset[2]);

	/* Calculate test results (difference between test and normal) */
	test_values[0] = abs((int8_t)gyro_x_test - (int8_t)gyro_x_orig);
	test_values[1] = abs((int8_t)gyro_y_test - (int8_t)gyro_y_orig);
	test_values[2] = abs((int8_t)gyro_z_test - (int8_t)gyro_z_orig);

	/* Check if values are within acceptable range (14-196 LSB) */
	for (i = 0; i < 3; i++) {
		if (test_values[i] < 14 || test_values[i] > 196) {
			pr_info("gyroscope self-test failed - axis %d: %d", i, test_values[i]);
			return 0;
		}
	}

	pr_info("gyroscope self-test passed");
	pr_info("test values - x: %d, y: %d, z: %d",
		test_values[0], test_values[1], test_values[2]);
	return 1;
}

u8 mpu6050_accel_self_test(struct mpu6050_device *dev)
{
	u8 accel_x_orig, accel_y_orig, accel_z_orig;
	u8 accel_x_test, accel_y_test, accel_z_test;
	u16 axis_offset[3];
	int32_t test_values[3];
	u8 i;

	pr_info("testing accelerometer...");

	/* Save original offset values */
	axis_offset[0] = mpu9250_read_accel_offset_reg(dev, X_AXIS);
	axis_offset[1] = mpu9250_read_accel_offset_reg(dev, Y_AXIS);
	axis_offset[2] = mpu9250_read_accel_offset_reg(dev, Z_AXIS);

	/* Enable self-test for all axes */
	mpu6050_accel_selftest(dev, X_AXIS, 1);
	mpu6050_accel_selftest(dev, Y_AXIS, 1);
	mpu6050_accel_selftest(dev, Z_AXIS, 1);

	mdelay(100);

	/* Read self-test values */
	accel_x_test = mpu6050_read_accel_selftest_reg(dev, X_AXIS);
	accel_y_test = mpu6050_read_accel_selftest_reg(dev, Y_AXIS);
	accel_z_test = mpu6050_read_accel_selftest_reg(dev, Z_AXIS);

	/* Disable self-test */
	mpu6050_accel_selftest(dev, X_AXIS, 0);
	mpu6050_accel_selftest(dev, Y_AXIS, 0);
	mpu6050_accel_selftest(dev, Z_AXIS, 0);

	mdelay(100);

	/* Read normal operation values */
	accel_x_orig = mpu6050_read_accel_selftest_reg(dev, X_AXIS);
	accel_y_orig = mpu6050_read_accel_selftest_reg(dev, Y_AXIS);
	accel_z_orig = mpu6050_read_accel_selftest_reg(dev, Z_AXIS);

	/* Restore original offsets */
	mpu9250_write_accel_offset_reg(dev, X_AXIS, axis_offset[0]);
	mpu9250_write_accel_offset_reg(dev, Y_AXIS, axis_offset[1]);
	mpu9250_write_accel_offset_reg(dev, Z_AXIS, axis_offset[2]);

	/* Calculate test results (difference between test and normal) */
	test_values[0] = abs((int8_t)accel_x_test - (int8_t)accel_x_orig);
	test_values[1] = abs((int8_t)accel_y_test - (int8_t)accel_y_orig);
	test_values[2] = abs((int8_t)accel_z_test - (int8_t)accel_z_orig);

	/* Check if values are within acceptable range (14-196 LSB) */
	for (i = 0; i < 3; i++) {
		if (test_values[i] < 14 || test_values[i] > 196) {
			pr_info("accelerometer self-test failed - axis %d: %d", i, test_values[i]);
			return 0;
		}
	}

	pr_info("accelerometer self-test passed");
	pr_info("test values - x: %d, y: %d, z: %d",
		test_values[0], test_values[1], test_values[2]);
	return 1;
}

u8 mpu6050_self_test(struct mpu6050_device *dev)
{
	u8 gyro_result, accel_result;

	pr_info("starting mpu6050 self-test...");

	/* Test gyroscope */
	gyro_result = mpu6050_gyro_self_test(dev);

	/* Test accelerometer */
	accel_result = mpu6050_accel_self_test(dev);

	/* Return overall result */
	if (gyro_result && accel_result) {
		pr_info("mpu6050 self-test passed");
		return 1;
	} else {
		pr_info("mpu6050 self-test failed");
		return 0;
	}
}

u8 mpu6050_check_fifo_overflow(struct mpu6050_device *dev)
{
	u8 int_status;

	mpu6050_int_status(dev, &int_status);

	/* Check bit 4 for FIFO overflow */
	return (int_status & 0x10) ? 1 : 0;
}

void mpu6050_reset_fifo(struct mpu6050_device *dev)
{
	/* Reset FIFO */
	mpu6050_reset_fifo_raw(dev, 1);
	mdelay(10);
	mpu6050_reset_fifo_raw(dev, 0);
}

void mpu6050_enable_fifo(struct mpu6050_device *dev, u8 enable)
{
	/* Call the low-level function */
	mpu6050_enable_fifo_raw(dev, enable);

	if (enable) {
		pr_info("fifo enabled");
	} else {
		pr_info("fifo disabled");
	}
}

void mpu6050_set_fifo_mode(struct mpu6050_device *dev, u8 mode)
{
	/* Call the existing function with correct parameter type */
	mpu6050_fifo_mode(dev, mode);

	if (mode) {
		pr_info("fifo mode enabled");
	} else {
		pr_info("fifo mode disabled");
	}
}

u16 mpu6050_get_fifo_count(struct mpu6050_device *dev)
{
	u16 count;

	mpu6050_fifo_count(dev, &count);
	return count;
}

u8 mpu6050_read_fifo_data(struct mpu6050_device *dev, u8 *data, u16 length)
{
	u16 fifo_count = mpu6050_get_fifo_count(dev);
	u16 i;

	if (fifo_count == 0) {
		return 0;
	}

	if (length > fifo_count) {
		length = fifo_count;
	}

	/* Read data byte by byte */
	for (i = 0; i < length; i++) {
		data[i] = mpu6050_fifo_read(dev);
	}

	return length;
}

void mpu6050_set_high_pass_filter(struct mpu6050_device *dev, u8 enable, u8 frequency)
{
	u32 reg;

	regmap_read(dev->regmap, CONFIG, &reg);

	/* Clear HPF bits and set new values */
	reg &= ~0x07;
	reg |= (frequency & 0x07);

	if (enable) {
		reg |= 0x08; /* Set HPF_EN bit */
	}

	regmap_write(dev->regmap, CONFIG, reg);

	pr_info("high-pass filter %s, frequency: 0x%02x",
		enable ? "enabled" : "disabled", frequency);
}

void mpu6050_configure_advanced_filters(struct mpu6050_device *dev)
{
	pr_info("configuring advanced filters...");

	/* Configure DLPF with optimal settings for motion detection */
	mpu6050_config_dlpf(dev, MPU6050_DLPF_42HZ);

	/* Enable high-pass filter for motion detection */
	mpu6050_set_high_pass_filter(dev, 1, 0x03);

	pr_info("advanced filters configured");
}

void mpu6050_set_motion_detection_threshold(struct mpu6050_device *dev, u8 threshold)
{
	regmap_write(dev->regmap, WOM_THR, threshold);
	pr_info("motion detection threshold set to: %d", threshold);
}

void mpu6050_set_zero_motion_detection_threshold(struct mpu6050_device *dev, u8 threshold)
{
	/* Write to ZRMOT_THR register (same as WOM_THR for this implementation) */
	regmap_write(dev->regmap, WOM_THR, threshold);

	pr_info("zero motion detection threshold set to: %d", threshold);
}

void mpu6050_set_zero_motion_detection_duration(struct mpu6050_device *dev, u8 duration)
{
	/* Configure motion detection control register */
	u32 reg;

	regmap_read(dev->regmap, MOT_DETECT_CTRL, &reg);

	/* Set duration bits (bits 0-5) */
	reg &= ~0x3F;
	reg |= (duration & 0x3F);

	regmap_write(dev->regmap, MOT_DETECT_CTRL, reg);

	pr_info("zero motion detection duration set to: %d", duration);
}

void mpu6050_enable_free_fall_detection(struct mpu6050_device *dev, u8 enable)
{
	u32 reg;

	regmap_read(dev->regmap, MOT_DETECT_CTRL, &reg);

	if (enable) {
		reg |= 0x40; /* Set free fall enable bit */
	} else {
		reg &= ~0x40; /* Clear free fall enable bit */
	}

	regmap_write(dev->regmap, MOT_DETECT_CTRL, reg);

	pr_info("free fall detection %s", enable ? "enabled" : "disabled");
}

void mpu6050_set_free_fall_threshold(struct mpu6050_device *dev, u8 threshold)
{
	/* For this implementation, use WOM_THR register */
	regmap_write(dev->regmap, WOM_THR, threshold);
	pr_info("free fall threshold set to: %d", threshold);
}

void mpu6050_set_free_fall_duration(struct mpu6050_device *dev, u8 duration)
{
	u32 reg;

	regmap_read(dev->regmap, MOT_DETECT_CTRL, &reg);

	/* Set free fall duration bits (bits 6-7) */
	reg &= ~0xC0;
	reg |= ((duration & 0x03) << 6);

	regmap_write(dev->regmap, MOT_DETECT_CTRL, reg);

	pr_info("free fall duration set to: %d", duration);
}

u8 mpu6050_enable_i2c_master_mode(struct mpu6050_device *dev, u8 enable)
{
	u32 reg;

	regmap_read(dev->regmap, USER_CTRL, &reg);

	if (enable) {
		reg |= 0x20; /* Set I2C_MST_EN bit */
	} else {
		reg &= ~0x20; /* Clear I2C_MST_EN bit */
	}

	regmap_write(dev->regmap, USER_CTRL, reg);

	pr_info("i2c master mode %s", enable ? "enabled" : "disabled");

	return 1;
}

u8 mpu6050_configure_i2c_slave(struct mpu6050_device *dev, u8 slave_num, u8 dev_addr, u8 reg_addr, u8 rw_flag, u8 len)
{
	if (slave_num > 3) {
		return 0;
	}

	/* Configure slave address */
	mpu6050_i2c_slave_addr(dev, slave_num, rw_flag, dev_addr);

	/* Configure register address */
	mpu6050_i2c_slave_reg(dev, slave_num, reg_addr);

	/* Configure number of bytes */
	mpu6050_i2c_slave_number_of_read_bytes(dev, slave_num, len);

	/* Enable slave */
	mpu6050_i2c_slave_enable(dev, slave_num, 1);

	pr_info("i2c slave %d configured: addr=0x%02x, reg=0x%02x, len=%d",
		slave_num, dev_addr, reg_addr, len);

	return 1;
}

u8 mpu6050_read_i2c_slave_data(struct mpu6050_device *dev, u8 slave_num, u8 *data, u8 len)
{
	if (slave_num > 3) {
		return 0;
	}

	/* Read from external sensor data registers */
	if (slave_num == 0) {
		regmap_bulk_read(dev->regmap, EXT_SENS_DATA_00, data, len);
	} else if (slave_num == 1) {
		regmap_bulk_read(dev->regmap, EXT_SENS_DATA_06, data, len);
	} else if (slave_num == 2) {
		regmap_bulk_read(dev->regmap, EXT_SENS_DATA_12, data, len);
	} else if (slave_num == 3) {
		regmap_bulk_read(dev->regmap, EXT_SENS_DATA_18, data, len);
	}

	return len;
}

u8 mpu6050_write_i2c_slave_data(struct mpu6050_device *dev, u8 slave_num, u8 *data, u8 len)
{
	u8 i;

	if (slave_num > 3) {
		return 0;
	}

	/* Write to slave data out registers */
	for (i = 0; i < len; i++) {
		if (slave_num == 0) {
			regmap_write(dev->regmap, I2C_SLV0_DO + i, data[i]);
		} else if (slave_num == 1) {
			regmap_write(dev->regmap, I2C_SLV1_DO + i, data[i]);
		} else if (slave_num == 2) {
			regmap_write(dev->regmap, I2C_SLV2_DO + i, data[i]);
		} else if (slave_num == 3) {
			regmap_write(dev->regmap, I2C_SLV3_DO + i, data[i]);
		}
	}

	return 1;
}

u8 mpu6050_get_i2c_master_status(struct mpu6050_device *dev)
{
	u32 status;

	regmap_read(dev->regmap, I2C_MST_STATUS, &status);

	return status;
}

void mpu6050_read_external_sensor_data(struct mpu6050_device *dev, u8 slave_num, u8 reg_addr, u8 *data, u8 len)
{
	if (slave_num > 3 || reg_addr + len > 24) {
		return;
	}

	/* Read from external sensor data registers */
	regmap_bulk_read(dev->regmap, EXT_SENS_DATA_00 + reg_addr, data, len);
}

u8 mpu6050_check_external_sensor_data_ready(struct mpu6050_device *dev, u8 slave_num)
{
	u8 status = mpu6050_get_i2c_master_status(dev);

	/* Check slave data ready bit for the specified slave */
	if (slave_num < 4) {
		return (status & (1 << slave_num)) ? 1 : 0;
	}

	return 0;
}

void mpu6050_configure_motion_detection(struct mpu6050_device *dev)
{
	pr_info("configuring motion detection...");

	/* Set motion detection threshold */
	mpu6050_set_motion_detection_threshold(dev, 10);

	/* Set zero motion detection */
	mpu6050_set_zero_motion_detection_threshold(dev, 5);
	mpu6050_set_zero_motion_detection_duration(dev, 50);

	/* Enable wake on motion interrupt */
	mpu6050_enable_wake_motion_interrupt(dev, 1);

	pr_info("motion detection configured");
}

void mpu6050_set_accel_artifact_removal(struct mpu6050_device *dev, u8 enable)
{
	u32 reg;

	regmap_read(dev->regmap, MOT_DETECT_CTRL, &reg);

	if (enable) {
		reg |= 0x80; /* Set accel artifact removal bit */
	} else {
		reg &= ~0x80; /* Clear accel artifact removal bit */
	}

	regmap_write(dev->regmap, MOT_DETECT_CTRL, reg);

	pr_info("accelerometer artifact removal %s", enable ? "enabled" : "disabled");
}

void mpu6050_set_gyro_threshold(struct mpu6050_device *dev, u8 axis, u16 threshold)
{
	/* Write to appropriate offset register as threshold */
	mpu6050_write_gyro_offset_reg(dev, axis, threshold);

	pr_info("gyroscope %c-axis threshold set to: %d",
		axis == 0 ? 'x' : axis == 1 ? 'y' : 'z', threshold);
}

static const struct regmap_range mpu6050_volatile_ranges[] = {
	regmap_reg_range(INT_STATUS, INT_STATUS),  /* 0x3A */
	regmap_reg_range(ACCEL_XOUT_H, GYRO_ZOUT_L),  /* 0x3B-0x48 */
	regmap_reg_range(FIFO_COUNTH, FIFO_R_W),  /* 0x72-0x74 */
};

static const struct regmap_access_table mpu6050_volatile_table = {
	.yes_ranges = mpu6050_volatile_ranges,
	.n_yes_ranges = ARRAY_SIZE(mpu6050_volatile_ranges),
};

static const struct regmap_config mpu6050_regmap_config = {
	.reg_bits = 8,
	.val_bits = 8,
	.max_register = 0x7E,  /* ZA_OFFSET_L */
	.volatile_table = &mpu6050_volatile_table,
	.cache_type = REGCACHE_NONE,
};

#ifdef KINETIS_FAKE_SIM
/* Box-Muller transform for Gaussian noise */
static float mpu6050_gaussian_noise(float mean, float std_dev)
{
	static u32 use_last = 0;
	static float z2, z1;
	float u1, u2, z0;

	if (!use_last) {
		u1 = (float)get_random_u32() / (float)U32_MAX;
		u2 = (float)get_random_u32() / (float)U32_MAX;

		/* Ensure u1 is not zero */
		if (u1 == 0.0f)
			u1 = 0.0001f;

		z0 = sqrtf(-2.0f * logf(u1)) * sinf(2.0f * M_PI * u2);
		z2 = sqrtf(-2.0f * logf(u1)) * cosf(2.0f * M_PI * u2);

		z1 = z0;
		use_last = 1;
	} else {
		z1 = z2;
		use_last = 0;
	}

	return z1 * std_dev + mean;
}

static void *mpu6050_reg_random_thread(void *arg)
{
	struct mpu6050_device *dev = arg;
	s16 accel_raw[3], gyro_raw[3], temp_raw;
	u8 val[2];
	u8 pwr_mgmt_1, smplrt_div;
	u32 update_period_ms;

	/* Initialize fixed registers at thread startup */
	dev->slave_regs[WHO_AM_I] = MPU6050_DEVID;
	dev->slave_regs[PWR_MGMT_1] = 0x00;        /* Wake up */
	dev->slave_regs[PWR_MGMT_2] = 0x00;
	dev->slave_regs[SMPLRT_DIV] = 0x09;        /* 100Hz sample rate */
	dev->slave_regs[CONFIG] = 0x00;           /* DLPF disabled */
	dev->slave_regs[GYRO_CONFIG] = 0x00;       /* ±250°/s */
	dev->slave_regs[ACCEL_CONFIG] = 0x00;      /* ±2g */
	dev->slave_regs[INT_PIN_CFG] = 0x00;
	dev->slave_regs[INT_ENABLE] = 0x00;
	dev->slave_regs[USER_CTRL] = 0x00;
	dev->slave_regs[SIGNAL_PATH_RESET] = 0x00;

	while (dev->thread_running) {
		/* Read PWR_MGMT_1 to check if device is in sleep mode */
		pwr_mgmt_1 = dev->slave_regs[PWR_MGMT_1];

		/* Check if device is awake (SLEEP bit = 0) and not in standby */
		if ((pwr_mgmt_1 & 0x40) == 0x00 && (pwr_mgmt_1 & 0x38) == 0x00) {
			/* Read SMPLRT_DIV to calculate sample rate */
			smplrt_div = dev->slave_regs[SMPLRT_DIV];

			/* Calculate update period: Sample Rate = 1kHz / (1 + SMPLRT_DIV) */
			/* Range: 4Hz (SMPLRT_DIV=0xFF) to 1000Hz (SMPLRT_DIV=0x00) */
			update_period_ms = 1000 / (1 + smplrt_div);

			/* Clamp to valid range (1ms to 250ms) */
			if (update_period_ms < 1)
				update_period_ms = 1;
			if (update_period_ms > 250)
				update_period_ms = 250;

			/* Generate accelerometer data: ~1g on Z-axis with noise */
			accel_raw[0] = (s16)mpu6050_gaussian_noise(0.0f, 163.84f);     /* X: 0g ± 0.01g */
			accel_raw[1] = (s16)mpu6050_gaussian_noise(0.0f, 163.84f);     /* Y: 0g ± 0.01g */
			accel_raw[2] = (s16)mpu6050_gaussian_noise(16384.0f, 163.84f); /* Z: 1g ± 0.01g */

			/* Write accelerometer data to slave registers */
			val[0] = (accel_raw[0] >> 8) & 0xFF;
			val[1] = accel_raw[0] & 0xFF;
			dev->slave_regs[ACCEL_XOUT_H] = val[0];
			dev->slave_regs[ACCEL_XOUT_L] = val[1];

			val[0] = (accel_raw[1] >> 8) & 0xFF;
			val[1] = accel_raw[1] & 0xFF;
			dev->slave_regs[ACCEL_YOUT_H] = val[0];
			dev->slave_regs[ACCEL_YOUT_L] = val[1];

			val[0] = (accel_raw[2] >> 8) & 0xFF;
			val[1] = accel_raw[2] & 0xFF;
			dev->slave_regs[ACCEL_ZOUT_H] = val[0];
			dev->slave_regs[ACCEL_ZOUT_L] = val[1];

			/* Generate gyroscope data: ~0°/s with noise */
			gyro_raw[0] = (s16)mpu6050_gaussian_noise(0.0f, 131.0f);   /* X: 0°/s ± 1°/s */
			gyro_raw[1] = (s16)mpu6050_gaussian_noise(0.0f, 131.0f);   /* Y: 0°/s ± 1°/s */
			gyro_raw[2] = (s16)mpu6050_gaussian_noise(0.0f, 131.0f);   /* Z: 0°/s ± 1°/s */

			/* Write gyroscope data to slave registers */
			val[0] = (gyro_raw[0] >> 8) & 0xFF;
			val[1] = gyro_raw[0] & 0xFF;
			dev->slave_regs[GYRO_XOUT_H] = val[0];
			dev->slave_regs[GYRO_XOUT_L] = val[1];

			val[0] = (gyro_raw[1] >> 8) & 0xFF;
			val[1] = gyro_raw[1] & 0xFF;
			dev->slave_regs[GYRO_YOUT_H] = val[0];
			dev->slave_regs[GYRO_YOUT_L] = val[1];

			val[0] = (gyro_raw[2] >> 8) & 0xFF;
			val[1] = gyro_raw[2] & 0xFF;
			dev->slave_regs[GYRO_ZOUT_H] = val[0];
			dev->slave_regs[GYRO_ZOUT_L] = val[1];

			/* Generate temperature data: ~25°C with noise */
			temp_raw = (s16)mpu6050_gaussian_noise(521.0f, 170.0f); /* 25°C ± 0.5°C */

			/* Write temperature data to slave registers */
			val[0] = (temp_raw >> 8) & 0xFF;
			val[1] = temp_raw & 0xFF;
			dev->slave_regs[TEMP_OUT_H] = val[0];
			dev->slave_regs[TEMP_OUT_L] = val[1];
		}

		/* Sleep based on calculated sample rate (default 10ms = 100Hz) */
		msleep(update_period_ms);
	}

	return 0;
}

static pthread_t reg_thread;

static int mpu6050_start_reg_random(struct mpu6050_device *dev)
{
	int ret;

	dev->thread_running = true;

	ret = pthread_create(&reg_thread, NULL,
			mpu6050_reg_random_thread, dev);
	if (ret) {
		return -ret;
	}

	msleep(100);

	pr_info("mpu6050: Randomization thread started successfully\n");
	return 0;
}

static void mpu6050_stop_reg_random(struct mpu6050_device *dev)
{
	dev->thread_running = false;
	pthread_join(reg_thread, NULL);
}
#endif

struct mpu6050_device *mpu6050_init(enum regmap_user_bus_type bus_type, void *bus_master)
{
	struct mpu6050_device *dev;
	int ret;

	dev = kzalloc(sizeof(*dev), GFP_KERNEL);
	if (!dev) {
		return ERR_PTR(-ENOMEM);
	}

	/* Initialize regmap based on bus type */
	if (bus_type == REGMAP_BUS_IIC_SOFT) {
		dev->regmap = regmap_init_iic_soft(bus_master, MPU6050_IIC_ADDR, &mpu6050_regmap_config);
	} else if (bus_type == REGMAP_BUS_SPI_SOFT) {
		/* Default SPI attributes: CPOL=0, CPHA=0, MSB first */
		dev->regmap = regmap_init_spi_soft(bus_master,
						   0, 0, SPI_BIT_ORDER_MSB, 2, &mpu6050_regmap_config);
	} else {
		pr_err("Invalid bus type specified for mpu6050 initialization");
		return ERR_PTR(-EINVAL);
	}
	if (IS_ERR(dev->regmap)) {
		kfree(dev);
		return NULL;
	}


#ifdef KINETIS_FAKE_SIM
	/* Allocate register space for slave simulation */
	dev->slave_regs = kmalloc(mpu6050_regmap_config.max_register + 1, GFP_KERNEL);
	if (!dev->slave_regs) {
		kfree(dev);
		return ERR_PTR(-ENOMEM);
	}

	/* Initialize simulated registers with default values */
	memset(dev->slave_regs, 0, mpu6050_regmap_config.max_register + 1);
	dev->slave_regs[WHO_AM_I] = MPU6050_DEVID;  /* Device ID */
	dev->slave_regs[PWR_MGMT_1] = 0x40;        /* Sleep mode initially */
	dev->slave_regs[TEMP_OUT_H] = 0x0D;
	dev->slave_regs[TEMP_OUT_L] = 0x48;         /* ~25°C */
	dev->slave_regs[ACCEL_ZOUT_L] = 0x01;       /* Small gravity value */

	/* Initialize slave for testing (MUST be done before thread starts) */
	if (bus_type == REGMAP_BUS_SPI_SOFT) {
		dev->spi_slave = spi_slave_soft_init("mpu6050", 0, 0, SPI_BIT_ORDER_MSB,
				dev->slave_regs, mpu6050_regmap_config.max_register + 1);
		if (IS_ERR(dev->spi_slave)) {
			pr_err("mpu6050: Failed to initialize SPI slave");
			kfree(dev->slave_regs);
			kfree(dev);
			return NULL;
		}
	} else {
		dev->iic_slave = iic_slave_soft_init("mpu6050", MPU6050_IIC_ADDR,
				dev->slave_regs, mpu6050_regmap_config.max_register + 1);
		if (IS_ERR(dev->iic_slave)) {
			pr_err("mpu6050: Failed to initialize I2C slave");
			kfree(dev->slave_regs);
			kfree(dev);
			return NULL;
		}
	}

	/* Start randomization thread (MUST be after slave is initialized) */
	mpu6050_start_reg_random(dev);
#endif

	/* Initialize default configuration */
	dev->gyro_scale = 131.0f;   /* LSB/°/s for ±250°/s */
	dev->accel_scale = 16384.0f; /* LSB/g for ±2g */
	dev->temp_sensitivity = 340;
	dev->config.device_present = 0;
	dev->config.gyro_sensitivity = MPU6050_GYRO_FS_250;
	dev->config.accel_sensitivity = MPU6050_ACCEL_FS_2;
	dev->config.initialized = 1;

	/* Perform device detection */
	if (mpu6050_who_am_i(dev) == MPU6050_DEVID) {
		dev->config.device_present = 1;
		pr_info("mpu6050: Device detected successfully");
	} else {
		pr_warn("mpu6050: Device detection failed, continuing anyway");
		dev->config.device_present = 0;
	}

	/* Set the device reset bit */
	mpu6050_soft_reset(dev, 1);
	/* Wait for reset to complete */
	mdelay(100);

	/* Set clock source to PLL with X axis gyroscope reference */
	mpu6050_clock_source_select(dev, MPU6050_CLOCK_PLL_X);

	/* Configure gyroscope: ±250°/s full scale */
	mpu6050_gyro_full_scale_select(dev, MPU6050_GYRO_FS_250);

	/* Configure accelerometer: ±2g full scale */
	mpu6050_accel_full_scale_select(dev, MPU6050_ACCEL_FS_2);

	/* Set sample rate to 100Hz */
	mpu6050_set_sample_rate(dev, 100);

	/* Configure DLPF bandwidth to 42Hz */
	mpu6050_config_dlpf(dev, MPU6050_DLPF_42HZ);

	/* Enable necessary interrupts */
	mpu6050_enable_interrupt(dev, 0x01); /* Data ready interrupt */

	pr_info("mpu6050 initialized successfully");
	pr_info("Gyro scale: %.1f LSB/°/s", dev->gyro_scale);
	pr_info("Accel scale: %.1f LSB/g", dev->accel_scale);

	return dev;
}

void mpu6050_exit(struct mpu6050_device *dev)
{
#ifdef KINETIS_FAKE_SIM
	/* Stop randomization thread before cleanup */
	mpu6050_stop_reg_random(dev);

	if (dev->iic_slave)
		iic_slave_soft_exit(dev->iic_slave);
	if (dev->spi_slave)
		spi_slave_soft_exit(dev->spi_slave);
	kfree(dev->slave_regs);
#endif
	regmap_exit(dev->regmap);
	kfree(dev);
}

#ifdef DESIGN_VERIFICATION_MPU6050
#include "kinetis/test-kinetis.h"

static struct mpu6050_device *mpu6050_dev;

int t_mpu6050_initialize(int argc, char **argv)
{
	bool on_off = true;
	enum regmap_user_bus_type bus_type = REGMAP_BUS_IIC_SOFT;

	if (argc > 1) {
		if (!strcmp(argv[1], "on")) {
			on_off = true;
		} else if (!strcmp(argv[1], "off")) {
			on_off = false;
		} else {
			return -EINVAL;
		}
	}

	if (argc > 2) {
		if (!strcmp(argv[2], "spi")) {
			bus_type = REGMAP_BUS_SPI_SOFT;
		} else if (!strcmp(argv[2], "i2c")) {
			bus_type = REGMAP_BUS_IIC_SOFT;
		} else {
			pr_err("Invalid bus type: %s (use 'i2c' or 'spi')", argv[2]);
			return -EINVAL;
		}
	}

	if (on_off) {
		pr_info("starting mpu6050 slave with %s mode", bus_type ? "spi" : "i2c");
		mpu6050_dev = mpu6050_init(bus_type, bus_type == REGMAP_BUS_IIC_SOFT ? (void *)&general_iic_master : (void *)&general_spi_master);
		if (IS_ERR_OR_NULL(mpu6050_dev)) {
			return -EINVAL;
		}
		return 0;
	}

	mpu6050_exit(mpu6050_dev);
	return 0;
}

int t_mpu6050_sensor_data(int argc, char **argv)
{
	struct mpu6050_device *dev = mpu6050_dev;
	mpu6050_raw_data_t accel, gyro;
	s16 temperature;
	u16 readings = 100;
	u16 i;

	if (argc > 1) {
		readings = simple_strtoul(argv[1], &argv[1], 10);
		if (readings > 1000) {
			readings = 1000;
		}
	}

	for (i = 0; i < readings; i++) {
		/* Read raw sensor data */
		mpu6050_get_raw_data(dev, &accel, &gyro, &temperature);

		/* Show first and last readings at info level */
		pr_info("reading %d/%d: accel[x=%6d,y=%6d,z=%6d] gyro[x=%6d,y=%6d,z=%6d] temp=%6d",
			i + 1, readings, accel.x, accel.y, accel.z,
			gyro.x, gyro.y, gyro.z, temperature);

		mdelay(10);
	}

	pr_info("Completed %d sensor data readings", readings);
	return 0;
}

int t_mpu6050_selftest(int argc, char **argv)
{
	struct mpu6050_device *dev = mpu6050_dev;
	u8 gyro_result, accel_result;

	/* Test gyroscope */
	gyro_result = mpu6050_gyro_self_test(dev);

	/* Test accelerometer */
	accel_result = mpu6050_accel_self_test(dev);

	/* Return overall result */
	if (gyro_result && accel_result) {
		pr_info("mpu6050 self-test completed successfully");
		return 0;
	} else {
		pr_err("fail: mpu6050 self-test failed");
		if (!gyro_result) {
			pr_err("  - gyroscope self-test failed");
		}
		if (!accel_result) {
			pr_err("  - accelerometer self-test failed");
		}
		return FAIL;
	}
}

int t_mpu6050_gyro_calibration(int argc, char **argv)
{
	struct mpu6050_device *dev = mpu6050_dev;
	u16 offset_x, offset_y, offset_z;

	pr_info("Note: Please keep device stationary during calibration");

	/* Read initial offsets */
	offset_x = mpu6050_read_gyro_offset_reg(dev, X_AXIS);
	offset_y = mpu6050_read_gyro_offset_reg(dev, Y_AXIS);
	offset_z = mpu6050_read_gyro_offset_reg(dev, Z_AXIS);

	pr_info("initial gyro offsets - x: %d, y: %d, z: %d", offset_x, offset_y, offset_z);

	/* Perform calibration */
	mpu6050_calibrate_gyro(dev);

	/* Read calibrated offsets */
	offset_x = mpu6050_read_gyro_offset_reg(dev, X_AXIS);
	offset_y = mpu6050_read_gyro_offset_reg(dev, Y_AXIS);
	offset_z = mpu6050_read_gyro_offset_reg(dev, Z_AXIS);

	pr_info("calibrated gyro offsets - x: %d, y: %d, z: %d", offset_x, offset_y, offset_z);

	pr_info("gyroscope calibration completed");
	return 0;
}

int t_mpu6050_accel_calibration(int argc, char **argv)
{
	struct mpu6050_device *dev = mpu6050_dev;
	u16 offset_x, offset_y, offset_z;

	pr_info("Note: Place device in 6 different orientations and call this function");

	/* Read initial offsets */
	offset_x = mpu9250_read_accel_offset_reg(dev, X_AXIS);
	offset_y = mpu9250_read_accel_offset_reg(dev, Y_AXIS);
	offset_z = mpu9250_read_accel_offset_reg(dev, Z_AXIS);

	pr_info("initial accel offsets - x: %d, y: %d, z: %d", offset_x, offset_y, offset_z);

	/* Perform calibration */
	mpu6050_calibrate_accel(dev);

	/* Read calibrated offsets */
	offset_x = mpu9250_read_accel_offset_reg(dev, X_AXIS);
	offset_y = mpu9250_read_accel_offset_reg(dev, Y_AXIS);
	offset_z = mpu9250_read_accel_offset_reg(dev, Z_AXIS);

	pr_info("calibrated accel offsets - x: %d, y: %d, z: %d", offset_x, offset_y, offset_z);

	pr_info("accelerometer calibration completed");
	return 0;
}

int t_mpu6050_fifo_test(int argc, char **argv)
{
	struct mpu6050_device *dev = mpu6050_dev;
	u16 fifo_count;
	u8 fifo_data[32];
	u8 bytes_read;

	/* Reset FIFO */
	mpu6050_reset_fifo(dev);
	mdelay(10);

	/* Enable FIFO */
	mpu6050_enable_fifo(dev, 1);
	mpu6050_set_fifo_mode(dev, 1);
	mdelay(10);

	/* Check FIFO count */
	fifo_count = mpu6050_get_fifo_count(dev);
	pr_info("fifo count after enable: %d bytes", fifo_count);

	/* Read FIFO data (if any) */
	if (fifo_count > 0) {
		bytes_read = mpu6050_read_fifo_data(dev, fifo_data, (fifo_count < 32) ? fifo_count : 32);
		pr_info("read %d bytes from fifo", bytes_read);
	} else {
		pr_info("fifo is empty (expected)");
	}

	/* Disable FIFO */
	mpu6050_enable_fifo(dev, 0);

	return 0;
}

int t_mpu6050_interrupt_test(int argc, char **argv)
{
	struct mpu6050_device *dev = mpu6050_dev;
	u8 int_status;

	/* Enable data ready interrupt */
	mpu6050_enable_interrupt(dev, 0x01);

	/* Read interrupt status */
	int_status = mpu6050_check_interrupt_flags(dev);
	pr_info("Interrupt status: 0x%02X", int_status);

	/* Clear interrupt flags */
	mpu6050_clear_interrupt_flags(dev);

	/* Disable interrupt */
	mpu6050_disable_interrupt(dev, 0x01);

	return 0;
}

int t_mpu6050_power_test(int argc, char **argv)
{
	struct mpu6050_device *dev = mpu6050_dev;

	/* Enter sleep mode */
	mpu6050_enter_sleep(dev, 1);
	pr_info("Entered sleep mode");
	mdelay(100);

	/* Enter wake mode */
	mpu6050_enter_sleep(dev, 0);
	pr_info("Entered wake mode");
	mdelay(100);

	/* Enter cycle mode (low power) */
	mpu6050_enter_cycle_mode(dev);
	pr_info("Entered cycle mode");
	mdelay(100);

	/* Return to normal mode */
	mpu6050_enter_sleep(dev, 0);
	pr_info("Returned to normal mode");

	return 0;
}

int t_mpu6050_fullscale_test(int argc, char **argv)
{
	struct mpu6050_device *dev = mpu6050_dev;

	/* Test gyroscope full scale ranges */
	pr_info("Testing gyroscope full scale ranges:");
	mpu6050_gyro_full_scale_select(dev, MPU6050_GYRO_FS_250);
	pr_info("  - Gyro ±250°/s: scale=%.1f LSB/°/s", dev->config.gyro_scale);
	mdelay(10);

	mpu6050_gyro_full_scale_select(dev, MPU6050_GYRO_FS_500);
	pr_info("  - Gyro ±500°/s: scale=%.1f LSB/°/s", dev->config.gyro_scale);
	mdelay(10);

	mpu6050_gyro_full_scale_select(dev, MPU6050_GYRO_FS_1000);
	pr_info("  - Gyro ±1000°/s: scale=%.1f LSB/°/s", dev->config.gyro_scale);
	mdelay(10);

	mpu6050_gyro_full_scale_select(dev, MPU6050_GYRO_FS_2000);
	pr_info("  - Gyro ±2000°/s: scale=%.1f LSB/°/s", dev->config.gyro_scale);
	mdelay(10);

	/* Test accelerometer full scale ranges */
	pr_info("Testing accelerometer full scale ranges:");
	mpu6050_accel_full_scale_select(dev, MPU6050_ACCEL_FS_2);
	pr_info("  - Accel ±2g: scale=%.1f LSB/g", dev->config.accel_scale);
	mdelay(10);

	mpu6050_accel_full_scale_select(dev, MPU6050_ACCEL_FS_4);
	pr_info("  - Accel ±4g: scale=%.1f LSB/g", dev->config.accel_scale);
	mdelay(10);

	mpu6050_accel_full_scale_select(dev, MPU6050_ACCEL_FS_8);
	pr_info("  - Accel ±8g: scale=%.1f LSB/g", dev->config.accel_scale);
	mdelay(10);

	mpu6050_accel_full_scale_select(dev, MPU6050_ACCEL_FS_16);
	pr_info("  - Accel ±16g: scale=%.1f LSB/g", dev->config.accel_scale);
	mdelay(10);

	/* Restore default configuration */
	mpu6050_gyro_full_scale_select(dev, MPU6050_GYRO_FS_250);
	mpu6050_accel_full_scale_select(dev, MPU6050_ACCEL_FS_2);

	pr_info("Full scale range test completed");
	return 0;
}

int t_mpu6050_dlpf_test(int argc, char **argv)
{
	struct mpu6050_device *dev = mpu6050_dev;

	/* Test various DLPF bandwidths */
	mpu6050_config_dlpf(dev, MPU6050_DLPF_256HZ);
	pr_info("DLPF bandwidth: 256Hz");
	mdelay(10);

	mpu6050_config_dlpf(dev, MPU6050_DLPF_188HZ);
	pr_info("DLPF bandwidth: 188Hz");
	mdelay(10);

	mpu6050_config_dlpf(dev, MPU6050_DLPF_98HZ);
	pr_info("DLPF bandwidth: 98Hz");
	mdelay(10);

	mpu6050_config_dlpf(dev, MPU6050_DLPF_42HZ);
	pr_info("DLPF bandwidth: 42Hz");
	mdelay(10);

	mpu6050_config_dlpf(dev, MPU6050_DLPF_20HZ);
	pr_info("DLPF bandwidth: 20Hz");
	mdelay(10);

	mpu6050_config_dlpf(dev, MPU6050_DLPF_10HZ);
	pr_info("DLPF bandwidth: 10Hz");
	mdelay(10);

	mpu6050_config_dlpf(dev, MPU6050_DLPF_5HZ);
	pr_info("DLPF bandwidth: 5Hz");
	mdelay(10);

	pr_info("DLPF bandwidth test completed");
	return 0;
}

#endif
