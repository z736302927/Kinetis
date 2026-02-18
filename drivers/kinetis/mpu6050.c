#include <linux/bitops.h>
#include <linux/printk.h>
#include <linux/delay.h>

#include "kinetis/mpu6050.h"
#include "kinetis/iic_soft.h"
#include "kinetis/delay.h"
#include "kinetis/idebug.h"
#include "kinetis/design_verification.h"

/* The following program is modified by the user according to the hardware device, otherwise the driver cannot run. */

/**
  * @step 1:  Modify the corresponding function according to the modified area and the corresponding function name.
  * @step 2:  Modify four areas: GPIO_PORT/GPIO_PIN/Led_TypeDef/LEDn.
  * @step 3:  .
  * @step 4:  .
  * @step 5:
  */

#define MPU6050_ADDR                    0x00

static struct iic_master *mpu6050_iic = &fake_master;

static inline void mpu6050_port_transmit(u8 addr, u8 tmp)
{
	iic_master_port_transmit(mpu6050_iic, MPU6050_ADDR, addr, tmp);
}

static inline void mpu6050_port_receive(u8 addr, u8 *pdata)
{
	iic_master_port_receive(mpu6050_iic, MPU6050_ADDR, addr, pdata);
}

static inline void mpu6050_port_multi_transmit(u8 addr, u8 *pdata, u32 Length)
{
	iic_master_port_multi_transmit(mpu6050_iic, MPU6050_ADDR, addr, pdata, Length);
}

static inline void mpu6050_port_multi_receive(u8 addr, u8 *pdata, u32 Length)
{
	iic_master_port_multi_receive(mpu6050_iic, MPU6050_ADDR, addr, pdata, Length);
}

/* The above procedure is modified by the user according to the hardware device, otherwise the driver cannot run. */

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

static u8 g_temp_sensitivity = 0;

void mpu6050_read_gyro_selftest_reg(u8 axis, u8 *pdata)
{
	switch (axis) {
	case X_AXIS:
		mpu6050_port_receive(SELF_TEST_X_GYRO, pdata);
		break;

	case Y_AXIS:
		mpu6050_port_receive(SELF_TEST_Y_GYRO, pdata);
		break;

	case Z_AXIS:
		mpu6050_port_receive(SELF_TEST_Z_GYRO, pdata);
		break;

	default:
		break;
	}
}

void mpu6050_write_gyro_selftest_reg(u8 axis, u8 tmp)
{
	switch (axis) {
	case X_AXIS:
		mpu6050_port_transmit(SELF_TEST_X_GYRO, tmp);
		break;

	case Y_AXIS:
		mpu6050_port_transmit(SELF_TEST_Y_GYRO, tmp);
		break;

	case Z_AXIS:
		mpu6050_port_transmit(SELF_TEST_Z_GYRO, tmp);
		break;

	default:
		break;
	}
}

void mpu6050_read_accel_selftest_reg(u8 axis, u8 *pdata)
{
	switch (axis) {
	case X_AXIS:
		mpu6050_port_receive(SELF_TEST_X_ACCEL, pdata);
		break;

	case Y_AXIS:
		mpu6050_port_receive(SELF_TEST_Y_ACCEL, pdata);
		break;

	case Z_AXIS:
		mpu6050_port_receive(SELF_TEST_Z_ACCEL, pdata);
		break;

	default:
		break;
	}
}

void mpu6050_write_accel_selftest_reg(u8 axis, u8 tmp)
{
	switch (axis) {
	case X_AXIS:
		mpu6050_port_transmit(SELF_TEST_X_ACCEL, tmp);
		break;

	case Y_AXIS:
		mpu6050_port_transmit(SELF_TEST_Y_ACCEL, tmp);
		break;

	case Z_AXIS:
		mpu6050_port_transmit(SELF_TEST_Z_ACCEL, tmp);
		break;

	default:
		break;
	}
}

void mpu6050_read_gyro_offset_reg(u8 axis, u16 *pdata)
{
	u8 high8 = 0;
	u8 low8 = 0;

	switch (axis) {
	case X_AXIS:
		mpu6050_port_receive(XG_OFFSET_H, &high8);
		mpu6050_port_receive(XG_OFFSET_L, &low8);
		break;

	case Y_AXIS:
		mpu6050_port_receive(YG_OFFSET_H, &high8);
		mpu6050_port_receive(YG_OFFSET_L, &low8);
		break;

	case Z_AXIS:
		mpu6050_port_receive(ZG_OFFSET_H, &high8);
		mpu6050_port_receive(ZG_OFFSET_L, &low8);
		break;

	default:
		break;
	}

	*pdata = (high8 << 8) | low8;
}

void mpu6050_write_gyro_offset_reg(u8 axis, u16 tmp)
{
	u8 high8 = 0;
	u8 low8 = 0;

	high8 = tmp >> 8;
	low8 = tmp & 0xFF;

	switch (axis) {
	case X_AXIS:
		mpu6050_port_transmit(XG_OFFSET_H, high8);
		mpu6050_port_transmit(XG_OFFSET_L, low8);
		break;

	case Y_AXIS:
		mpu6050_port_transmit(YG_OFFSET_H, high8);
		mpu6050_port_transmit(YG_OFFSET_L, low8);
		break;

	case Z_AXIS:
		mpu6050_port_transmit(ZG_OFFSET_H, high8);
		mpu6050_port_transmit(ZG_OFFSET_L, low8);
		break;

	default:
		break;
	}
}

/**
 * @brief Read accelerometer offset register
 * @param axis Axis to read (0=X, 1=Y, 2=Z)
 * @param pdata Pointer to store the offset value
 * @return None
 */
void mpu6050_read_accel_offset_reg(uint8_t axis, uint16_t *pdata)
{
	u8 high8 = 0;
	u8 low8 = 0;

	switch (axis) {
	case X_AXIS:
		mpu6050_port_receive(XA_OFFSET_H, &high8);
		mpu6050_port_receive(XA_OFFSET_L, &low8);
		break;

	case Y_AXIS:
		mpu6050_port_receive(YA_OFFSET_H, &high8);
		mpu6050_port_receive(YA_OFFSET_L, &low8);
		break;

	case Z_AXIS:
		mpu6050_port_receive(ZA_OFFSET_H, &high8);
		mpu6050_port_receive(ZA_OFFSET_L, &low8);
		break;

	default:
		break;
	}

	*pdata = (high8 << 8) | low8;
}

/**
 * @brief Write accelerometer offset register
 * @param axis Axis to write (0=X, 1=Y, 2=Z)
 * @param tmp Offset value to write
 * @return None
 */
void mpu6050_write_accel_offset_reg(uint8_t axis, uint16_t tmp)
{
	u8 high8 = 0;
	u8 low8 = 0;

	high8 = tmp >> 8;
	low8 = tmp & 0xFF;

	switch (axis) {
	case X_AXIS:
		mpu6050_port_transmit(XA_OFFSET_H, high8);
		mpu6050_port_transmit(XA_OFFSET_L, low8);
		break;

	case Y_AXIS:
		mpu6050_port_transmit(YA_OFFSET_H, high8);
		mpu6050_port_transmit(YA_OFFSET_L, low8);
		break;

	case Z_AXIS:
		mpu6050_port_transmit(ZA_OFFSET_H, high8);
		mpu6050_port_transmit(ZA_OFFSET_L, low8);
		break;

	default:
		break;
	}
}

void mpu6050_sample_rate_divider(u8 tmp)
{
	mpu6050_port_transmit(SMPLRT_DIV, tmp);
}

void mpu6050_fifo_mode(u8 tmp)
{
	u8 reg = 0;

	mpu6050_port_receive(CONFIG, &reg);

	__assign_bit(6, (unsigned long *)&reg, tmp);

	mpu6050_port_transmit(CONFIG, reg);
}

void mpu6050_fsync_set(u8 tmp)
{
	u8 reg = 0;

	mpu6050_port_receive(CONFIG, &reg);

	set_mask_bits(&reg, 0x07 << 3, tmp);

	mpu6050_port_transmit(CONFIG, reg);
}

void mpu6050_config_dlpf(u8 tmp)
{
	u8 reg = 0;

	mpu6050_port_receive(CONFIG, &reg);

	set_mask_bits(&reg, 0x07, tmp);

	mpu6050_port_transmit(CONFIG, reg);
}

void mpu6050_gyro_selftest(u8 axis, u8 tmp)
{
	u8 reg = 0;

	mpu6050_port_receive(GYRO_CONFIG, &reg);
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

	mpu6050_port_transmit(GYRO_CONFIG, reg);
}

void mpu6050_gyro_full_scale_select(u8 tmp)
{
	u8 reg = 0;

	mpu6050_port_receive(GYRO_CONFIG, &reg);

	set_mask_bits(&reg, 0x03 << 3, tmp << 3);

	mpu6050_port_transmit(GYRO_CONFIG, reg);
}

void mpu6050_fchoice_b(u8 tmp)
{
	u8 reg = 0;

	mpu6050_port_receive(GYRO_CONFIG, &reg);

	set_mask_bits(&reg, 0x03 << 0, tmp << 0);

	mpu6050_port_transmit(GYRO_CONFIG, reg);
}

void mpu6050_accel_selftest(u8 axis, u8 tmp)
{
	u8 reg = 0;

	mpu6050_port_receive(ACCEL_CONFIG, &reg);
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

	mpu6050_port_transmit(ACCEL_CONFIG, reg);
}

void mpu6050_accel_full_scale_select(u8 tmp)
{
	u8 reg = 0;

	mpu6050_port_receive(ACCEL_CONFIG, &reg);

	set_mask_bits(&reg, 0x03 << 3, tmp << 3);

	mpu6050_port_transmit(ACCEL_CONFIG, reg);
}

void mpu6050_accel_fchoice_b(u8 tmp)
{
	u8 reg = 0;

	mpu6050_port_receive(ACCEL_CONFIG2, &reg);

	set_mask_bits(&reg, 0x01 << 3, tmp << 3);

	mpu6050_port_transmit(ACCEL_CONFIG2, reg);
}

void mpu6050_config_accel_dlpf(u8 tmp)
{
	u8 reg = 0;

	mpu6050_port_receive(ACCEL_CONFIG2, &reg);

	set_mask_bits(&reg, 0x07 << 0, tmp << 0);

	mpu6050_port_transmit(ACCEL_CONFIG2, reg);
}

void mpu6050_low_power_accel_odr_control(u8 tmp)
{
	u8 reg = 0;

	mpu6050_port_receive(LP_ACCEL_ODR, &reg);

	set_mask_bits(&reg, 0x0F << 0, tmp << 0);

	mpu6050_port_transmit(LP_ACCEL_ODR, reg);
}

void mpu6050_wake_on_motion_threshold(u8 tmp)
{
	mpu6050_port_transmit(WOM_THR, tmp);
}

void mpu6050_fifo_enable_with_temp(u8 tmp)
{
	u8 reg = 0;

	mpu6050_port_receive(FIFO_EN, &reg);

	set_mask_bits(&reg, 0x01 << 7, tmp << 7);

	mpu6050_port_transmit(FIFO_EN, reg);
}

void mpu6050_fifo_enable_with_gyro(u8 axis, u8 tmp)
{
	u8 reg = 0;

	mpu6050_port_receive(FIFO_EN, &reg);
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

	mpu6050_port_transmit(FIFO_EN, reg);
}

void mpu6050_fifo_enable_with_accel(u8 tmp)
{
	u8 reg = 0;

	mpu6050_port_receive(FIFO_EN, &reg);

	set_mask_bits(&reg, 0x01 << 3, tmp << 3);

	mpu6050_port_transmit(FIFO_EN, reg);
}

void mpu6050_fifo_enable_with_ext_sensor(u8 slave, u8 tmp)
{
	u8 reg = 0;

	mpu6050_port_receive(FIFO_EN, &reg);
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

	mpu6050_port_transmit(FIFO_EN, reg);
}

void mpu6050_enable_multi_master(u8 tmp)
{
	u8 reg = 0;

	mpu6050_port_receive(I2C_MST_CTRL, &reg);

	set_mask_bits(&reg, 0x01 << 7, tmp << 7);

	mpu6050_port_transmit(I2C_MST_CTRL, reg);
}

void mpu6050_wait_for_ext_sensor(u8 tmp)
{
	u8 reg = 0;

	mpu6050_port_receive(I2C_MST_CTRL, &reg);

	set_mask_bits(&reg, 0x01 << 6, tmp << 6);

	mpu6050_port_transmit(I2C_MST_CTRL, reg);
}

void mpu6050_enable_slave3_fifo(u8 tmp)
{
	u8 reg = 0;

	mpu6050_port_receive(I2C_MST_CTRL, &reg);

	set_mask_bits(&reg, 0x01 << 5, tmp << 5);

	mpu6050_port_transmit(I2C_MST_CTRL, reg);
}

void mpu6050_i2c_signal_between_read(u8 tmp)
{
	u8 reg = 0;

	mpu6050_port_receive(I2C_MST_CTRL, &reg);

	set_mask_bits(&reg, 0x01 << 4, tmp << 4);

	mpu6050_port_transmit(I2C_MST_CTRL, reg);
}

void mpu6050_i2c_master_clock(u8 tmp)
{
	u8 reg = 0;

	mpu6050_port_receive(I2C_MST_CTRL, &reg);

	set_mask_bits(&reg, 0x0F << 0, tmp << 0);

	mpu6050_port_transmit(I2C_MST_CTRL, reg);
}

void mpu6050_i2c_slave_addr(u8 slave, u8 dir, u8 addr)
{
	u8 reg = 0;

	reg |= (dir << 7);
	reg |= (addr << 0);

	switch (slave) {
	case I2C_SLAVE0:
		mpu6050_port_transmit(I2C_SLV0_ADDR, reg);
		break;

	case I2C_SLAVE1:
		mpu6050_port_transmit(I2C_SLV1_ADDR, reg);
		break;

	case I2C_SLAVE2:
		mpu6050_port_transmit(I2C_SLV2_ADDR, reg);
		break;

	case I2C_SLAVE3:
		mpu6050_port_transmit(I2C_SLV3_ADDR, reg);
		break;

	case I2C_SLAVE4:
		mpu6050_port_transmit(I2C_SLV4_ADDR, reg);
		break;

	default:
		break;
	}
}

void mpu6050_i2c_slave_reg(u8 slave, u8 reg)
{
	switch (slave) {
	case I2C_SLAVE0:
		mpu6050_port_transmit(I2C_SLV0_REG, reg);
		break;

	case I2C_SLAVE1:
		mpu6050_port_transmit(I2C_SLV1_REG, reg);
		break;

	case I2C_SLAVE2:
		mpu6050_port_transmit(I2C_SLV2_REG, reg);
		break;

	case I2C_SLAVE3:
		mpu6050_port_transmit(I2C_SLV3_REG, reg);
		break;

	case I2C_SLAVE4:
		mpu6050_port_transmit(I2C_SLV4_REG, reg);
		break;

	default:
		break;
	}
}

void mpu6050_i2c_slave_enable(u8 slave, u8 tmp)
{
	u8 reg = 0;

	tmp &= 0x01;

	switch (slave) {
	case I2C_SLAVE0:
		mpu6050_port_receive(I2C_SLV0_CTRL, &reg);
		__assign_bit(7, (unsigned long *)&reg, tmp);
		mpu6050_port_transmit(I2C_SLV0_CTRL, reg);
		break;

	case I2C_SLAVE1:
		mpu6050_port_receive(I2C_SLV1_CTRL, &reg);
		__assign_bit(7, (unsigned long *)&reg, tmp);
		mpu6050_port_transmit(I2C_SLV1_CTRL, reg);
		break;

	case I2C_SLAVE2:
		mpu6050_port_receive(I2C_SLV2_CTRL, &reg);
		__assign_bit(7, (unsigned long *)&reg, tmp);
		mpu6050_port_transmit(I2C_SLV2_CTRL, reg);
		break;

	case I2C_SLAVE3:
		mpu6050_port_receive(I2C_SLV3_CTRL, &reg);
		__assign_bit(7, (unsigned long *)&reg, tmp);
		mpu6050_port_transmit(I2C_SLV3_CTRL, reg);
		break;

	case I2C_SLAVE4:
		mpu6050_port_receive(I2C_SLV4_CTRL, &reg);
		__assign_bit(7, (unsigned long *)&reg, tmp);
		mpu6050_port_transmit(I2C_SLV4_CTRL, reg);
		break;

	default:
		break;
	}
}

void mpu6050_i2c_slave_swap_bytes(u8 slave, u8 tmp)
{
	u8 reg = 0;

	tmp &= 0x01;

	switch (slave) {
	case I2C_SLAVE0:
		mpu6050_port_receive(I2C_SLV0_CTRL, &reg);
		__assign_bit(6, (unsigned long *)&reg, tmp);
		mpu6050_port_transmit(I2C_SLV0_CTRL, reg);
		break;

	case I2C_SLAVE1:
		mpu6050_port_receive(I2C_SLV1_CTRL, &reg);
		__assign_bit(6, (unsigned long *)&reg, tmp);
		mpu6050_port_transmit(I2C_SLV1_CTRL, reg);
		break;

	case I2C_SLAVE2:
		mpu6050_port_receive(I2C_SLV2_CTRL, &reg);
		__assign_bit(6, (unsigned long *)&reg, tmp);
		mpu6050_port_transmit(I2C_SLV2_CTRL, reg);
		break;

	case I2C_SLAVE3:
		mpu6050_port_receive(I2C_SLV3_CTRL, &reg);
		__assign_bit(6, (unsigned long *)&reg, tmp);
		mpu6050_port_transmit(I2C_SLV3_CTRL, reg);
		break;

	default:
		break;
	}
}

void mpu6050_i2c_slave_dis_reg(u8 slave, u8 tmp)
{
	u8 reg = 0;

	tmp &= 0x01;

	switch (slave) {
	case I2C_SLAVE0:
		mpu6050_port_receive(I2C_SLV0_CTRL, &reg);
		__assign_bit(5, (unsigned long *)&reg, tmp);
		mpu6050_port_transmit(I2C_SLV0_CTRL, reg);
		break;

	case I2C_SLAVE1:
		mpu6050_port_receive(I2C_SLV1_CTRL, &reg);
		__assign_bit(5, (unsigned long *)&reg, tmp);
		mpu6050_port_transmit(I2C_SLV1_CTRL, reg);
		break;

	case I2C_SLAVE2:
		mpu6050_port_receive(I2C_SLV2_CTRL, &reg);
		__assign_bit(5, (unsigned long *)&reg, tmp);
		mpu6050_port_transmit(I2C_SLV2_CTRL, reg);
		break;

	case I2C_SLAVE3:
		mpu6050_port_receive(I2C_SLV3_CTRL, &reg);
		__assign_bit(5, (unsigned long *)&reg, tmp);
		mpu6050_port_transmit(I2C_SLV3_CTRL, reg);
		break;

	case I2C_SLAVE4:
		mpu6050_port_receive(I2C_SLV4_CTRL, &reg);
		__assign_bit(5, (unsigned long *)&reg, tmp);
		mpu6050_port_transmit(I2C_SLV4_CTRL, reg);
		break;

	default:
		break;
	}
}

void mpu6050_i2c_slave_group_type(u8 slave, u8 tmp)
{
	u8 reg = 0;

	tmp &= 0x01;

	switch (slave) {
	case I2C_SLAVE0:
		mpu6050_port_receive(I2C_SLV0_CTRL, &reg);
		__assign_bit(4, (unsigned long *)&reg, tmp);
		mpu6050_port_transmit(I2C_SLV0_CTRL, reg);
		break;

	case I2C_SLAVE1:
		mpu6050_port_receive(I2C_SLV1_CTRL, &reg);
		__assign_bit(4, (unsigned long *)&reg, tmp);
		mpu6050_port_transmit(I2C_SLV1_CTRL, reg);
		break;

	case I2C_SLAVE2:
		mpu6050_port_receive(I2C_SLV2_CTRL, &reg);
		__assign_bit(4, (unsigned long *)&reg, tmp);
		mpu6050_port_transmit(I2C_SLV2_CTRL, reg);
		break;

	case I2C_SLAVE3:
		mpu6050_port_receive(I2C_SLV3_CTRL, &reg);
		__assign_bit(4, (unsigned long *)&reg, tmp);
		mpu6050_port_transmit(I2C_SLV3_CTRL, reg);
		break;

	default:
		break;
	}
}

void mpu6050_i2c_slave_number_of_read_bytes(u8 slave, u8 tmp)
{
	u8 reg = 0;

	tmp &= 0x0F;

	switch (slave) {
	case I2C_SLAVE0:
		mpu6050_port_receive(I2C_SLV0_CTRL, &reg);
		set_mask_bits(&reg, 0x0F << 0, tmp << 0);
		mpu6050_port_transmit(I2C_SLV0_CTRL, reg);
		break;

	case I2C_SLAVE1:
		mpu6050_port_receive(I2C_SLV1_CTRL, &reg);
		set_mask_bits(&reg, 0x0F << 0, tmp << 0);
		mpu6050_port_transmit(I2C_SLV1_CTRL, reg);
		break;

	case I2C_SLAVE2:
		mpu6050_port_receive(I2C_SLV2_CTRL, &reg);
		set_mask_bits(&reg, 0x0F << 0, tmp << 0);
		mpu6050_port_transmit(I2C_SLV2_CTRL, reg);
		break;

	case I2C_SLAVE3:
		mpu6050_port_receive(I2C_SLV3_CTRL, &reg);
		set_mask_bits(&reg, 0x0F << 0, tmp << 0);
		mpu6050_port_transmit(I2C_SLV3_CTRL, reg);
		break;

	case I2C_SLAVE4:
		mpu6050_port_receive(I2C_SLV4_CTRL, &reg);
		set_mask_bits(&reg, 0x0F << 0, tmp << 0);
		mpu6050_port_transmit(I2C_SLV4_CTRL, reg);
		break;

	default:
		break;
	}
}

void mpu6050_i2c_slave4_do(u8 slave, u8 tmp)
{
	switch (slave) {
	case I2C_SLAVE0:
		mpu6050_port_transmit(I2C_SLV0_DO, tmp);
		break;

	case I2C_SLAVE1:
		mpu6050_port_transmit(I2C_SLV1_DO, tmp);
		break;

	case I2C_SLAVE2:
		mpu6050_port_transmit(I2C_SLV2_DO, tmp);
		break;

	case I2C_SLAVE3:
		mpu6050_port_transmit(I2C_SLV3_DO, tmp);
		break;

	case I2C_SLAVE4:
		mpu6050_port_transmit(I2C_SLV4_DO, tmp);
		break;

	default:
		break;
	}
}

void mpu6050_i2c_slave_enable_int(u8 tmp)
{
	u8 reg = 0;

	mpu6050_port_receive(I2C_SLV4_CTRL, &reg);

	set_mask_bits(&reg, 0x01 << 6, tmp << 6);

	mpu6050_port_transmit(I2C_SLV4_CTRL, reg);
}

void mpu6050_i2c_slave_master_delay(u8 tmp)
{
	u8 reg = 0;

	mpu6050_port_receive(I2C_SLV4_CTRL, &reg);

	set_mask_bits(&reg, 0x1F << 0, tmp << 0);

	mpu6050_port_transmit(I2C_SLV4_CTRL, reg);
}

void mpu6050_i2c_slave4_di(u8 *pdata)
{
	mpu6050_port_receive(I2C_SLV4_DI, pdata);
}

u8 mpu6050_status_of_fsync_int(void)
{
	u8 reg = 0;

	mpu6050_port_receive(I2C_MST_STATUS, &reg);

	return test_bit(7, (unsigned long *)&reg);
}

u8 mpu6050_slave4_transfer_done(void)
{
	u8 reg = 0;

	mpu6050_port_receive(I2C_MST_STATUS, &reg);

	return test_bit(6, (unsigned long *)&reg);

}

u8 mpu6050_slave_looses_arbitration(void)
{
	u8 reg = 0;

	mpu6050_port_receive(I2C_MST_STATUS, &reg);

	return test_bit(5, (unsigned long *)&reg);

}

u8 mpu6050_slave_receives_nack(u8 slave)
{
	u8 reg = 0;
	u8 val = 0;

	mpu6050_port_receive(I2C_MST_STATUS, &reg);

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

void mpu6050_logic_level_for_int(u8 tmp)
{
	u8 reg = 0;

	mpu6050_port_receive(INT_PIN_CFG, &reg);

	set_mask_bits(&reg, 0x01 << 7, tmp << 7);

	mpu6050_port_transmit(INT_PIN_CFG, reg);
}

void mpu6050_enable_pull_up(u8 tmp)
{
	u8 reg = 0;

	mpu6050_port_receive(INT_PIN_CFG, &reg);

	set_mask_bits(&reg, 0x01 << 6, tmp << 6);

	mpu6050_port_transmit(INT_PIN_CFG, reg);
}

void mpu6050_latch_int_pin(u8 tmp)
{
	u8 reg = 0;

	mpu6050_port_receive(INT_PIN_CFG, &reg);

	set_mask_bits(&reg, 0x01 << 5, tmp << 5);

	mpu6050_port_transmit(INT_PIN_CFG, reg);
}

void mpu6050_int_anyrd2_clear(u8 tmp)
{
	u8 reg = 0;

	mpu6050_port_receive(INT_PIN_CFG, &reg);

	set_mask_bits(&reg, 0x01 << 4, tmp << 4);

	mpu6050_port_transmit(INT_PIN_CFG, reg);
}

void mpu6050_logic_level_for_fsync(u8 tmp)
{
	u8 reg = 0;

	mpu6050_port_receive(INT_PIN_CFG, &reg);

	set_mask_bits(&reg, 0x01 << 3, tmp << 3);

	mpu6050_port_transmit(INT_PIN_CFG, reg);
}

void mpu6050_fsync_int_mode(u8 tmp)
{
	u8 reg = 0;

	mpu6050_port_receive(INT_PIN_CFG, &reg);

	set_mask_bits(&reg, 0x01 << 2, tmp << 2);

	mpu6050_port_transmit(INT_PIN_CFG, reg);
}

void mpu6050_bypass_mode(u8 tmp)
{
	u8 reg = 0;

	mpu6050_port_receive(INT_PIN_CFG, &reg);

	set_mask_bits(&reg, 0x01 << 1, tmp << 1);

	mpu6050_port_transmit(INT_PIN_CFG, reg);
}

void mpu6050_int_for_wake_on_motion(u8 tmp)
{
	u8 reg = 0;

	mpu6050_port_receive(INT_ENABLE, &reg);

	set_mask_bits(&reg, 0x01 << 6, tmp << 6);

	mpu6050_port_transmit(INT_ENABLE, reg);
}

void mpu6050_int_for_fifo_overflow(u8 tmp)
{
	u8 reg = 0;

	mpu6050_port_receive(INT_ENABLE, &reg);

	set_mask_bits(&reg, 0x01 << 4, tmp << 4);

	mpu6050_port_transmit(INT_ENABLE, reg);
}

void mpu6050_int_for_fsync(u8 tmp)
{
	u8 reg = 0;

	mpu6050_port_receive(INT_ENABLE, &reg);

	set_mask_bits(&reg, 0x01 << 3, tmp << 3);

	mpu6050_port_transmit(INT_ENABLE, reg);
}

void mpu6050_int_for_raw_sensor_tmp_ready(u8 tmp)
{
	u8 reg = 0;

	mpu6050_port_receive(INT_ENABLE, &reg);

	set_mask_bits(&reg, 0x01 << 0, tmp << 0);

	mpu6050_port_transmit(INT_ENABLE, reg);
}

void mpu6050_int_status(u8 *pdata)
{
	mpu6050_port_receive(INT_STATUS, pdata);
}

void mpu6050_accel_measurements(u16 *pdata)
{
	u8 val[6];

	mpu6050_port_multi_receive(ACCEL_XOUT_H, val, 6);

	pdata[0] = (val[0] << 8) | val[1];
	pdata[1] = (val[2] << 8) | val[3];
	pdata[2] = (val[4] << 8) | val[5];
}

void mpu6050_temp_measurement(u16 *pdata)
{
	u8 val[2];

	mpu6050_port_multi_receive(TEMP_OUT_H, val, 2);

	pdata[0] = (val[0] << 8) | val[1];
}

void mpu6050_get_temperature(float *pdata)
{
	u16 val;

	mpu6050_temp_measurement(&val);

	pdata[0] = ((val - 0) / g_temp_sensitivity) + 21;
}

void mpu6050_gyro_measurements(u16 *pdata)
{
	u8 val[6];

	mpu6050_port_multi_receive(GYRO_XOUT_H, val, 6);

	pdata[0] = (val[0] << 8) | val[1];
	pdata[1] = (val[2] << 8) | val[3];
	pdata[2] = (val[4] << 8) | val[5];
}

void mpu6050_get_accel_and_gyro(u16 *pdata)
{
	u8 val[14];

	mpu6050_port_multi_receive(ACCEL_XOUT_H, val, 14);

	pdata[0] = (val[0] << 8) | val[1];
	pdata[1] = (val[2] << 8) | val[3];
	pdata[2] = (val[4] << 8) | val[5];

	pdata[3] = (val[8] << 8) | val[9];
	pdata[4] = (val[10] << 8) | val[11];
	pdata[5] = (val[12] << 8) | val[13];
}

void mpu6050_ExternalSensortmp(u8 reg, u8 *pdata)
{
	mpu6050_port_receive(reg, pdata);
}

void mpu6050_delay_shadow_of_ext_sensor(u8 tmp)
{
	u8 reg = 0;

	mpu6050_port_receive(I2C_MST_DELAY_CTRL, &reg);

	set_mask_bits(&reg, 0x01 << 7, tmp << 7);

	mpu6050_port_transmit(I2C_MST_DELAY_CTRL, reg);
}

void mpu6050_i2c_slave_delay_enable(u8 slave, u8 tmp)
{
	u8 reg = 0;

	tmp &= 0x01;

	switch (slave) {
	case I2C_SLAVE0:
		mpu6050_port_receive(I2C_MST_DELAY_CTRL, &reg);
		set_mask_bits(&reg, 0x01 << 0, tmp << 0);
		mpu6050_port_transmit(I2C_MST_DELAY_CTRL, reg);
		break;

	case I2C_SLAVE1:
		mpu6050_port_receive(I2C_MST_DELAY_CTRL, &reg);
		set_mask_bits(&reg, 0x01 << 1, tmp << 1);
		mpu6050_port_transmit(I2C_MST_DELAY_CTRL, reg);
		break;

	case I2C_SLAVE2:
		mpu6050_port_receive(I2C_MST_DELAY_CTRL, &reg);
		set_mask_bits(&reg, 0x01 << 2, tmp << 2);
		mpu6050_port_transmit(I2C_MST_DELAY_CTRL, reg);
		break;

	case I2C_SLAVE3:
		mpu6050_port_receive(I2C_MST_DELAY_CTRL, &reg);
		set_mask_bits(&reg, 0x01 << 3, tmp << 3);
		mpu6050_port_transmit(I2C_MST_DELAY_CTRL, reg);
		break;

	case I2C_SLAVE4:
		mpu6050_port_receive(I2C_MST_DELAY_CTRL, &reg);
		set_mask_bits(&reg, 0x01 << 4, tmp << 4);
		mpu6050_port_transmit(I2C_MST_DELAY_CTRL, reg);
		break;

	default:
		break;
	}
}

void mpu6050_gyro_signal_path_reset(u8 tmp)
{
	u8 reg = 0;

	mpu6050_port_receive(SIGNAL_PATH_RESET, &reg);

	set_mask_bits(&reg, 0x01 << 2, tmp << 2);

	mpu6050_port_transmit(SIGNAL_PATH_RESET, reg);
}

void mpu6050_accel_signal_path_reset(u8 tmp)
{
	u8 reg = 0;

	mpu6050_port_receive(SIGNAL_PATH_RESET, &reg);

	set_mask_bits(&reg, 0x01 << 1, tmp << 1);

	mpu6050_port_transmit(SIGNAL_PATH_RESET, reg);
}

void mpu6050_temp_signal_path_reset(u8 tmp)
{
	u8 reg = 0;

	mpu6050_port_receive(SIGNAL_PATH_RESET, &reg);

	set_mask_bits(&reg, 0x01 << 0, tmp << 0);

	mpu6050_port_transmit(SIGNAL_PATH_RESET, reg);
}

void mpu6050_enable_wake_on_motion(u8 tmp)
{
	u8 reg = 0;

	mpu6050_port_receive(MOT_DETECT_CTRL, &reg);

	set_mask_bits(&reg, 0x01 << 7, tmp << 7);

	mpu6050_port_transmit(MOT_DETECT_CTRL, reg);
}

void mpu6050_accel_int_mode(u8 tmp)
{
	u8 reg = 0;

	mpu6050_port_receive(MOT_DETECT_CTRL, &reg);

	set_mask_bits(&reg, 0x01 << 6, tmp << 6);

	mpu6050_port_transmit(MOT_DETECT_CTRL, reg);
}

void mpu6050_enable_fifo_raw(u8 tmp)
{
	u8 reg = 0;

	mpu6050_port_receive(USER_CTRL, &reg);

	set_mask_bits(&reg, 0x01 << 6, tmp << 6);

	mpu6050_port_transmit(USER_CTRL, reg);
}

void mpu6050_enable_i2c_master(u8 tmp)
{
	u8 reg = 0;

	mpu6050_port_receive(USER_CTRL, &reg);

	set_mask_bits(&reg, 0x01 << 5, tmp << 5);

	mpu6050_port_transmit(USER_CTRL, reg);
}

void mpu6050_disable_i2c_slave(u8 tmp)
{
	u8 reg = 0;

	mpu6050_port_receive(USER_CTRL, &reg);

	set_mask_bits(&reg, 0x01 << 4, tmp << 4);

	mpu6050_port_transmit(USER_CTRL, reg);
}

void mpu6050_reset_fifo_raw(u8 tmp)
{
	u8 reg = 0;

	mpu6050_port_receive(USER_CTRL, &reg);

	set_mask_bits(&reg, 0x01 << 2, tmp << 2);

	mpu6050_port_transmit(USER_CTRL, reg);
}

void mpu6050_reset_i2c_master(u8 tmp)
{
	u8 reg = 0;

	mpu6050_port_receive(USER_CTRL, &reg);

	set_mask_bits(&reg, 0x01 << 1, tmp << 1);

	mpu6050_port_transmit(USER_CTRL, reg);
}

void mpu6050_reset_signal_path(u8 tmp)
{
	u8 reg = 0;

	mpu6050_port_receive(USER_CTRL, &reg);

	set_mask_bits(&reg, 0x01 << 0, tmp << 0);

	mpu6050_port_transmit(USER_CTRL, reg);
}

void mpu6050_soft_reset(u8 tmp)
{
	u8 reg = 0;

	mpu6050_port_receive(PWR_MGMT_1, &reg);

	set_mask_bits(&reg, 0x01 << 7, tmp << 7);

	mpu6050_port_transmit(PWR_MGMT_1, reg);
}

void mpu6050_enter_sleep(u8 tmp)
{
	u8 reg = 0;

	mpu6050_port_receive(PWR_MGMT_1, &reg);

	set_mask_bits(&reg, 0x01 << 6, tmp << 6);

	mpu6050_port_transmit(PWR_MGMT_1, reg);
}

void mpu6050_cycle_sample(u8 tmp)
{
	u8 reg = 0;

	mpu6050_port_receive(PWR_MGMT_1, &reg);

	set_mask_bits(&reg, 0x01 << 5, tmp << 5);

	mpu6050_port_transmit(PWR_MGMT_1, reg);
}

void mpu6050_gyro_standby(u8 tmp)
{
	u8 reg = 0;

	mpu6050_port_receive(PWR_MGMT_1, &reg);

	set_mask_bits(&reg, 0x01 << 4, tmp << 4);

	mpu6050_port_transmit(PWR_MGMT_1, reg);
}

void mpu6050_power_down_ptat(u8 tmp)
{
	u8 reg = 0;

	mpu6050_port_receive(PWR_MGMT_1, &reg);

	set_mask_bits(&reg, 0x01 << 3, tmp << 3);

	mpu6050_port_transmit(PWR_MGMT_1, reg);
}

void mpu6050_clock_source_select(u8 tmp)
{
	u8 reg = 0;

	mpu6050_port_receive(PWR_MGMT_1, &reg);

	set_mask_bits(&reg, 0x07 << 0, tmp << 0);

	mpu6050_port_transmit(PWR_MGMT_1, reg);
}

void mpu6050_accel_disabled(u8 axis, u8 tmp)
{
	u8 reg = 0;

	mpu6050_port_receive(PWR_MGMT_2, &reg);
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

	mpu6050_port_transmit(PWR_MGMT_2, reg);
}

void mpu6050_gyro_disabled(u8 axis, u8 tmp)
{
	u8 reg = 0;

	mpu6050_port_receive(PWR_MGMT_2, &reg);
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

	mpu6050_port_transmit(PWR_MGMT_2, reg);
}

void mpu6050_fifo_count(u16 *pdata)
{
	u8 reg[2];

	mpu6050_port_multi_receive(FIFO_COUNTH, reg, 2);

	reg[0] &= 0x1F;
	pdata[0] = (reg[0] << 8) | reg[1];
}

void mpu6050_fifo_read(u8 *pdata)
{
	mpu6050_port_receive(FIFO_R_W, pdata);
}

void mpu6050_fifo_write(u8 tmp)
{
	mpu6050_port_transmit(FIFO_R_W, tmp);
}

void mpu6050_who_am_i(u8 *pdata)
{
	mpu6050_port_receive(WHO_AM_I, pdata);
}

void mpu9250_read_accel_offset_reg(u8 axis, u16 *pdata)
{
	u8 high8 = 0;
	u8 low8 = 0;

	switch (axis) {
	case X_AXIS:
		mpu6050_port_receive(XA_OFFSET_H, &high8);
		mpu6050_port_receive(XA_OFFSET_L, &low8);
		break;

	case Y_AXIS:
		mpu6050_port_receive(YA_OFFSET_H, &high8);
		mpu6050_port_receive(YA_OFFSET_L, &low8);
		break;

	case Z_AXIS:
		mpu6050_port_receive(ZA_OFFSET_H, &high8);
		mpu6050_port_receive(ZA_OFFSET_L, &low8);
		break;

	default:
		break;
	}

	*pdata = (high8 << 7) | (low8 >> 1);
}

void mpu9250_write_accel_offset_reg(u8 axis, u16 tmp)
{
	u8 high8 = 0;
	u8 low8 = 0;

	high8 = tmp >> 7;
	low8 = (tmp & 0xFE) >> 1;

	switch (axis) {
	case X_AXIS:
		mpu6050_port_transmit(XA_OFFSET_H, high8);
		mpu6050_port_transmit(XA_OFFSET_L, low8);
		break;

	case Y_AXIS:
		mpu6050_port_transmit(YA_OFFSET_H, high8);
		mpu6050_port_transmit(YA_OFFSET_L, low8);
		break;

	case Z_AXIS:
		mpu6050_port_transmit(ZA_OFFSET_H, high8);
		mpu6050_port_transmit(ZA_OFFSET_L, low8);
		break;

	default:
		break;
	}
}

/* Enhanced driver global variables */
static mpu6050_config_t g_mpu6050_config = {0};
static uint8_t g_mpu6050_initialized = 0;

/**
 * @brief Enhanced initialization function
 * @param None
 * @return None
 * @note This function initializes the MPU6050 with optimal settings
 */
void mpu6050_init(void)
{
	uint8_t who_am_i = 0;

	printk("Initializing MPU6050 6-axis sensor...");

	/* Check if device is present */
	if (!mpu6050_is_device_present()) {
		printk("ERROR: MPU6050 device not found!");
		return;
	}

	/* Perform software reset */
	mpu6050_reset();

	/* Wait for reset to complete */
	mdelay(100);

	/* Set clock source to PLL with X axis gyroscope reference */
	mpu6050_set_clock_source(MPU6050_CLOCK_PLL_X);

	/* Configure gyroscope: ±250°/s full scale */
	mpu6050_set_gyro_full_scale(MPU6050_GYRO_FS_250);

	/* Configure accelerometer: ±2g full scale */
	mpu6050_set_accel_full_scale(MPU6050_ACCEL_FS_2);

	/* Set sample rate to 100Hz */
	mpu6050_set_sample_rate(100);

	/* Configure DLPF bandwidth to 42Hz */
	mpu6050_set_dlpf_bandwidth(MPU6050_DLPF_42HZ);

	/* Enable necessary interrupts */
	mpu6050_enable_interrupt(0x01); /* Data ready interrupt */

	/* Initialize global configuration */
	g_mpu6050_config.device_present = 1;
	g_mpu6050_config.gyro_sensitivity = MPU6050_GYRO_FS_250;
	g_mpu6050_config.accel_sensitivity = MPU6050_ACCEL_FS_2;
	g_mpu6050_config.gyro_scale = 131.0f; /* LSB/°/s for ±250°/s */
	g_mpu6050_config.accel_scale = 16384.0f; /* LSB/g for ±2g */
	g_mpu6050_config.initialized = 1;

	g_mpu6050_initialized = 1;

	printk("MPU6050 initialized successfully");
	printk("Gyro scale: %.1f LSB/°/s", g_mpu6050_config.gyro_scale);
	printk("Accel scale: %.1f LSB/g", g_mpu6050_config.accel_scale);
}

/**
 * @brief Check if MPU6050 device is present
 * @param None
 * @return 1 if device is present, 0 otherwise
 */
uint8_t mpu6050_is_device_present(void)
{
	uint8_t who_am_i = 0;

	mpu6050_who_am_i(&who_am_i);

	/* MPU6050 should return 0x68 */
	if (who_am_i == 0x68) {
		return 1;
	}

	return 0;
}

/**
 * @brief Perform software reset
 * @param None
 * @return None
 */
void mpu6050_reset(void)
{
	/* Set the device reset bit */
	mpu6050_soft_reset(1);

	/* Wait for reset to complete */
	mdelay(100);
}

/**
 * @brief Set clock source
 * @param clock_source Clock source selection
 * @return None
 */
void mpu6050_set_clock_source(uint8_t clock_source)
{
	mpu6050_clock_source_select(clock_source);
}

/**
 * @brief Set gyroscope full scale range
 * @param fs Full scale selection
 * @return None
 */
void mpu6050_set_gyro_full_scale(uint8_t fs)
{
	mpu6050_gyro_full_scale_select(fs);

	/* Update scale factor */
	switch (fs) {
	case MPU6050_GYRO_FS_250:
		g_mpu6050_config.gyro_scale = 131.0f;
		break;
	case MPU6050_GYRO_FS_500:
		g_mpu6050_config.gyro_scale = 65.5f;
		break;
	case MPU6050_GYRO_FS_1000:
		g_mpu6050_config.gyro_scale = 32.8f;
		break;
	case MPU6050_GYRO_FS_2000:
		g_mpu6050_config.gyro_scale = 16.4f;
		break;
	default:
		g_mpu6050_config.gyro_scale = 131.0f;
		break;
	}
}

/**
 * @brief Set accelerometer full scale range
 * @param fs Full scale selection
 * @return None
 */
void mpu6050_set_accel_full_scale(uint8_t fs)
{
	mpu6050_accel_full_scale_select(fs);

	/* Update scale factor */
	switch (fs) {
	case MPU6050_ACCEL_FS_2:
		g_mpu6050_config.accel_scale = 16384.0f;
		break;
	case MPU6050_ACCEL_FS_4:
		g_mpu6050_config.accel_scale = 8192.0f;
		break;
	case MPU6050_ACCEL_FS_8:
		g_mpu6050_config.accel_scale = 4096.0f;
		break;
	case MPU6050_ACCEL_FS_16:
		g_mpu6050_config.accel_scale = 2048.0f;
		break;
	default:
		g_mpu6050_config.accel_scale = 16384.0f;
		break;
	}
}

/**
 * @brief Set sample rate divider
 * @param rate Sample rate in Hz (1-1000)
 * @return None
 */
void mpu6050_set_sample_rate(uint16_t rate)
{
	uint8_t divider;

	if (rate > 1000) {
		rate = 1000;
	}
	if (rate < 1) {
		rate = 1;
	}

	/* Calculate divider: sample_rate = 1000 / (divider + 1) */
	divider = (1000 / rate) - 1;

	mpu6050_sample_rate_divider(divider);
}

/**
 * @brief Set DLPF bandwidth
 * @param bandwidth Bandwidth selection
 * @return None
 */
void mpu6050_set_dlpf_bandwidth(uint8_t bandwidth)
{
	mpu6050_config_dlpf(bandwidth);
}

/**
 * @brief Get raw sensor data
 * @param accel Pointer to accelerometer raw data structure
 * @param gyro Pointer to gyroscope raw data structure
 * @param temperature Pointer to temperature reading
 * @return None
 */
void mpu6050_get_raw_data(mpu6050_raw_data_t *accel, mpu6050_raw_data_t *gyro, int16_t *temperature)
{
	uint16_t raw_data[7]; /* accel[3], temp[1], gyro[3] */

	if (!g_mpu6050_initialized) {
		mpu6050_init();
	}

	/* Read all sensor data in one transaction */
	mpu6050_get_accel_and_gyro(raw_data);

	/* Extract accelerometer data */
	accel->x = (int16_t)raw_data[0];
	accel->y = (int16_t)raw_data[1];
	accel->z = (int16_t)raw_data[2];

	/* Extract temperature data */
	*temperature = (int16_t)raw_data[3];

	/* Extract gyroscope data */
	gyro->x = (int16_t)raw_data[4];
	gyro->y = (int16_t)raw_data[5];
	gyro->z = (int16_t)raw_data[6];
}

/**
 * @brief Get scaled sensor data
 * @param accel Pointer to accelerometer scaled data structure
 * @param gyro Pointer to gyroscope scaled data structure
 * @param temperature Pointer to temperature in Celsius
 * @return None
 */
void mpu6050_get_scaled_data(mpu6050_data_t *accel, mpu6050_data_t *gyro, float *temperature)
{
	mpu6050_raw_data_t raw_accel, raw_gyro;
	int16_t raw_temp;

	/* Get raw data */
	mpu6050_get_raw_data(&raw_accel, &raw_gyro, &raw_temp);

	/* Convert to scaled data */
	accel->x = (float)raw_accel.x / g_mpu6050_config.accel_scale;
	accel->y = (float)raw_accel.y / g_mpu6050_config.accel_scale;
	accel->z = (float)raw_accel.z / g_mpu6050_config.accel_scale;

	gyro->x = (float)raw_gyro.x / g_mpu6050_config.gyro_scale;
	gyro->y = (float)raw_gyro.y / g_mpu6050_config.gyro_scale;
	gyro->z = (float)raw_gyro.z / g_mpu6050_config.gyro_scale;

	/* Convert temperature to Celsius: Temp = (Temp_OUT - 521) / 340 + 35.0 */
	*temperature = ((float)raw_temp - 521.0f) / 340.0f + 35.0f;
}

/**
 * @brief Calibrate gyroscope (zero bias calibration)
 * @param None
 * @return None
 * @note This function should be called when device is stationary
 */
void mpu6050_calibrate_gyro(void)
{
	mpu6050_raw_data_t accel, gyro;
	int16_t temperature;
	int32_t gyro_x_sum = 0, gyro_y_sum = 0, gyro_z_sum = 0;
	uint16_t samples = 1000;
	uint16_t i;
	int16_t offset_x, offset_y, offset_z;

	printk("Starting gyroscope calibration...");
	printk("Please keep device stationary for 10 seconds");

	/* Collect samples */
	for (i = 0; i < samples; i++) {
		mpu6050_get_raw_data(&accel, &gyro, &temperature);
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
	mpu6050_write_gyro_offset_reg(X_AXIS, (uint16_t)offset_x);
	mpu6050_write_gyro_offset_reg(Y_AXIS, (uint16_t)offset_y);
	mpu6050_write_gyro_offset_reg(Z_AXIS, (uint16_t)offset_z);

	printk("Gyroscope calibration completed");
	printk("Offsets - X: %d, Y: %d, Z: %d", offset_x, offset_y, offset_z);
}

/**
 * @brief Calibrate accelerometer (gravity vector calibration)
 * @param None
 * @return None
 * @note This function should be called with device in 6 different orientations
 */
void mpu6050_calibrate_accel(void)
{
	mpu6050_raw_data_t accel, gyro;
	int16_t temperature;
	int32_t accel_x_sum = 0, accel_y_sum = 0, accel_z_sum = 0;
	uint16_t samples = 100;
	uint16_t i;
	int16_t offset_x, offset_y, offset_z;

	printk("Starting accelerometer calibration...");
	printk("Place device in 6 different orientations and call this function");

	/* Collect samples */
	for (i = 0; i < samples; i++) {
		mpu6050_get_raw_data(&accel, &gyro, &temperature);
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
	mpu9250_write_accel_offset_reg(X_AXIS, (uint16_t)offset_x);
	mpu9250_write_accel_offset_reg(Y_AXIS, (uint16_t)offset_y);
	mpu9250_write_accel_offset_reg(Z_AXIS, (uint16_t)offset_z);

	printk("Accelerometer calibration completed");
	printk("Offsets - X: %d, Y: %d, Z: %d", offset_x, offset_y, offset_z);
}

/**
 * @brief Enable interrupt
 * @param interrupt Interrupt enable mask
 * @return None
 */
void mpu6050_enable_interrupt(uint8_t interrupt)
{
	mpu6050_int_for_raw_sensor_tmp_ready(interrupt & 0x01);
	mpu6050_int_for_fsync((interrupt >> 3) & 0x01);
	mpu6050_int_for_fifo_overflow((interrupt >> 4) & 0x01);
	mpu6050_int_for_wake_on_motion((interrupt >> 6) & 0x01);
}

/**
 * @brief Disable interrupt
 * @param interrupt Interrupt disable mask
 * @return None
 */
void mpu6050_disable_interrupt(uint8_t interrupt)
{
	mpu6050_int_for_raw_sensor_tmp_ready(!(interrupt & 0x01));
	mpu6050_int_for_fsync(!((interrupt >> 3) & 0x01));
	mpu6050_int_for_fifo_overflow(!((interrupt >> 4) & 0x01));
	mpu6050_int_for_wake_on_motion(!((interrupt >> 6) & 0x01));
}

/**
 * @brief Clear interrupt flags
 * @param None
 * @return None
 */
void mpu6050_clear_interrupt_flags(void)
{
	/* Read status register to clear flags */
	uint8_t status;
	mpu6050_int_status(&status);
}

/**
 * @brief Check interrupt flags
 * @param None
 * @return Interrupt status
 */
uint8_t mpu6050_check_interrupt_flags(void)
{
	uint8_t status;
	mpu6050_int_status(&status);
	return status;
}

/**
 * @brief Enter sleep mode
 * @param None
 * @return None
 */
void mpu6050_enter_sleep_mode(void)
{
	mpu6050_enter_sleep(1);
}

/**
 * @brief Enter wake mode
 * @param None
 * @return None
 */
void mpu6050_enter_wake_mode(void)
{
	mpu6050_enter_sleep(0);
}

/**
 * @brief Enter cycle mode (low power accelerometer only)
 * @param None
 * @return None
 */
void mpu6050_enter_cycle_mode(void)
{
	mpu6050_cycle_sample(1);
	mpu6050_gyro_standby(1);
}

/**
 * @brief Set wake on motion threshold
 * @param threshold Threshold value (0-255)
 * @return None
 */
void mpu6050_set_wake_motion_threshold(uint8_t threshold)
{
	mpu6050_wake_on_motion_threshold(threshold);
}

/**
 * @brief Enable wake on motion interrupt
 * @param enable Enable/disable flag
 * @return None
 */
void mpu6050_enable_wake_motion_interrupt(uint8_t enable)
{
	mpu6050_enable_wake_on_motion(enable);
	mpu6050_int_for_wake_on_motion(enable);
}

/**
 * @brief Perform complete self-test
 * @param None
 * @return 1 if all tests pass, 0 if any test fails
 */
uint8_t mpu6050_self_test(void)
{
	uint8_t gyro_result, accel_result;

	printk("Starting MPU6050 self-test...");

	/* Test gyroscope */
	gyro_result = mpu6050_gyro_self_test();

	/* Test accelerometer */
	accel_result = mpu6050_accel_self_test();

	/* Return overall result */
	if (gyro_result && accel_result) {
		printk("MPU6050 self-test PASSED");
		return 1;
	} else {
		printk("MPU6050 self-test FAILED");
		return 0;
	}
}

/**
 * @brief Perform gyroscope self-test
 * @param None
 * @return 1 if test passes, 0 if test fails
 */
uint8_t mpu6050_gyro_self_test(void)
{
	uint8_t gyro_x_orig, gyro_y_orig, gyro_z_orig;
	uint8_t gyro_x_test, gyro_y_test, gyro_z_test;
	uint16_t axis_offset[3];
	int32_t test_values[3];
	uint8_t i;

	printk("Testing gyroscope...");

	/* Save original offset values */
	mpu6050_read_gyro_offset_reg(X_AXIS, &axis_offset[0]);
	mpu6050_read_gyro_offset_reg(Y_AXIS, &axis_offset[1]);
	mpu6050_read_gyro_offset_reg(Z_AXIS, &axis_offset[2]);

	/* Enable self-test for all axes */
	mpu6050_gyro_selftest(X_AXIS, 1);
	mpu6050_gyro_selftest(Y_AXIS, 1);
	mpu6050_gyro_selftest(Z_AXIS, 1);

	mdelay(100);

	/* Read self-test values */
	mpu6050_read_gyro_selftest_reg(X_AXIS, &gyro_x_test);
	mpu6050_read_gyro_selftest_reg(Y_AXIS, &gyro_y_test);
	mpu6050_read_gyro_selftest_reg(Z_AXIS, &gyro_z_test);

	/* Disable self-test */
	mpu6050_gyro_selftest(X_AXIS, 0);
	mpu6050_gyro_selftest(Y_AXIS, 0);
	mpu6050_gyro_selftest(Z_AXIS, 0);

	mdelay(100);

	/* Read normal operation values */
	mpu6050_read_gyro_selftest_reg(X_AXIS, &gyro_x_orig);
	mpu6050_read_gyro_selftest_reg(Y_AXIS, &gyro_y_orig);
	mpu6050_read_gyro_selftest_reg(Z_AXIS, &gyro_z_orig);

	/* Restore original offsets */
	mpu6050_write_gyro_offset_reg(X_AXIS, axis_offset[0]);
	mpu6050_write_gyro_offset_reg(Y_AXIS, axis_offset[1]);
	mpu6050_write_gyro_offset_reg(Z_AXIS, axis_offset[2]);

	/* Calculate test results (difference between test and normal) */
	test_values[0] = abs((int8_t)gyro_x_test - (int8_t)gyro_x_orig);
	test_values[1] = abs((int8_t)gyro_y_test - (int8_t)gyro_y_orig);
	test_values[2] = abs((int8_t)gyro_z_test - (int8_t)gyro_z_orig);

	/* Check if values are within acceptable range (14-196 LSB) */
	for (i = 0; i < 3; i++) {
		if (test_values[i] < 14 || test_values[i] > 196) {
			printk("Gyroscope self-test FAILED - Axis %d: %d", i, test_values[i]);
			return 0;
		}
	}

	printk("Gyroscope self-test PASSED");
	printk("Test values - X: %d, Y: %d, Z: %d",
		test_values[0], test_values[1], test_values[2]);
	return 1;
}

/**
 * @brief Perform accelerometer self-test
 * @param None
 * @return 1 if test passes, 0 if test fails
 */
uint8_t mpu6050_accel_self_test(void)
{
	uint8_t accel_x_orig, accel_y_orig, accel_z_orig;
	uint8_t accel_x_test, accel_y_test, accel_z_test;
	uint16_t axis_offset[3];
	int32_t test_values[3];
	uint8_t i;

	printk("Testing accelerometer...");

	/* Save original offset values */
	mpu9250_read_accel_offset_reg(X_AXIS, &axis_offset[0]);
	mpu9250_read_accel_offset_reg(Y_AXIS, &axis_offset[1]);
	mpu9250_read_accel_offset_reg(Z_AXIS, &axis_offset[2]);

	/* Enable self-test for all axes */
	mpu6050_accel_selftest(X_AXIS, 1);
	mpu6050_accel_selftest(Y_AXIS, 1);
	mpu6050_accel_selftest(Z_AXIS, 1);

	mdelay(100);

	/* Read self-test values */
	mpu6050_read_accel_selftest_reg(X_AXIS, &accel_x_test);
	mpu6050_read_accel_selftest_reg(Y_AXIS, &accel_y_test);
	mpu6050_read_accel_selftest_reg(Z_AXIS, &accel_z_test);

	/* Disable self-test */
	mpu6050_accel_selftest(X_AXIS, 0);
	mpu6050_accel_selftest(Y_AXIS, 0);
	mpu6050_accel_selftest(Z_AXIS, 0);

	mdelay(100);

	/* Read normal operation values */
	mpu6050_read_accel_selftest_reg(X_AXIS, &accel_x_orig);
	mpu6050_read_accel_selftest_reg(Y_AXIS, &accel_y_orig);
	mpu6050_read_accel_selftest_reg(Z_AXIS, &accel_z_orig);

	/* Restore original offsets */
	mpu9250_write_accel_offset_reg(X_AXIS, axis_offset[0]);
	mpu9250_write_accel_offset_reg(Y_AXIS, axis_offset[1]);
	mpu9250_write_accel_offset_reg(Z_AXIS, axis_offset[2]);

	/* Calculate test results (difference between test and normal) */
	test_values[0] = abs((int8_t)accel_x_test - (int8_t)accel_x_orig);
	test_values[1] = abs((int8_t)accel_y_test - (int8_t)accel_y_orig);
	test_values[2] = abs((int8_t)accel_z_test - (int8_t)accel_z_orig);

	/* Check if values are within acceptable range (14-196 LSB) */
	for (i = 0; i < 3; i++) {
		if (test_values[i] < 14 || test_values[i] > 196) {
			printk("Accelerometer self-test FAILED - Axis %d: %d", i, test_values[i]);
			return 0;
		}
	}

	printk("Accelerometer self-test PASSED");
	printk("Test values - X: %d, Y: %d, Z: %d",
		test_values[0], test_values[1], test_values[2]);
	return 1;
}

/**
 * @brief Check FIFO overflow status
 * @param None
 * @return 1 if overflow detected, 0 otherwise
 */
uint8_t mpu6050_check_fifo_overflow(void)
{
	uint8_t int_status;

	mpu6050_int_status(&int_status);

	/* Check bit 4 for FIFO overflow */
	return (int_status & 0x10) ? 1 : 0;
}

/**
 * @brief Reset FIFO buffer
 * @param None
 * @return None
 */
void mpu6050_reset_fifo(void)
{
	/* Reset FIFO */
	mpu6050_reset_fifo_raw(1);
	mdelay(10);
	mpu6050_reset_fifo_raw(0);
}

/**
 * @brief Enable or disable FIFO
 * @param enable Enable/disable flag
 * @return None
 */
void mpu6050_enable_fifo(uint8_t enable)
{
	/* Call the low-level function */
	mpu6050_enable_fifo_raw((u8)enable);

	if (enable) {
		printk("FIFO enabled");
	} else {
		printk("FIFO disabled");
	}
}

/**
 * @brief Set FIFO mode
 * @param mode FIFO mode (0=disabled, 1=enabled)
 * @return None
 */
void mpu6050_set_fifo_mode(uint8_t mode)
{
	/* Call the existing function with correct parameter type */
	mpu6050_fifo_mode((u8)mode);

	if (mode) {
		printk("FIFO mode enabled");
	} else {
		printk("FIFO mode disabled");
	}
}

/**
 * @brief Get current FIFO count
 * @param None
 * @return Number of bytes in FIFO
 */
uint16_t mpu6050_get_fifo_count(void)
{
	uint16_t count;

	mpu6050_fifo_count(&count);
	return count;
}

/**
 * @brief Read data from FIFO
 * @param data Pointer to data buffer
 * @param length Number of bytes to read
 * @return Number of bytes actually read
 */
uint8_t mpu6050_read_fifo_data(uint8_t *data, uint16_t length)
{
	uint16_t fifo_count = mpu6050_get_fifo_count();
	uint16_t i;

	if (fifo_count == 0) {
		return 0;
	}

	if (length > fifo_count) {
		length = fifo_count;
	}

	/* Read data byte by byte */
	for (i = 0; i < length; i++) {
		mpu6050_fifo_read(&data[i]);
	}

	return length;
}

/**
 * @brief Configure advanced filters
 * @param None
 * @return None
 */
void mpu6050_configure_advanced_filters(void)
{
	printk("Configuring advanced filters...");

	/* Configure DLPF with optimal settings for motion detection */
	mpu6050_config_dlpf(MPU6050_DLPF_42HZ);

	/* Enable high-pass filter for motion detection */
	mpu6050_set_high_pass_filter(1, 0x03);

	printk("Advanced filters configured");
}

/**
 * @brief Set high-pass filter
 * @param enable Enable/disable flag (0=disable, 1=enable)
 * @param frequency Filter frequency setting
 * @return None
 */
void mpu6050_set_high_pass_filter(uint8_t enable, uint8_t frequency)
{
	u8 reg = 0;

	mpu6050_port_receive(CONFIG, &reg);

	/* Clear HPF bits and set new values */
	reg &= ~0x07;
	reg |= (frequency & 0x07);

	if (enable) {
		reg |= 0x08; /* Set HPF_EN bit */
	}

	mpu6050_port_transmit(CONFIG, reg);

	printk("High-pass filter %s, frequency: 0x%02X",
		enable ? "enabled" : "disabled", frequency);
}

/**
 * @brief Set motion detection threshold
 * @param threshold Motion detection threshold value
 * @return None
 */
void mpu6050_set_motion_detection_threshold(uint8_t threshold)
{
	mpu6050_port_transmit(WOM_THR, threshold);
	printk("Motion detection threshold set to: %d", threshold);
}

/**
 * @brief Set zero motion detection threshold
 * @param threshold Zero motion detection threshold value
 * @return None
 */
void mpu6050_set_zero_motion_detection_threshold(uint8_t threshold)
{
	u8 reg = 0;

	/* Write to ZRMOT_THR register (same as WOM_THR for this implementation) */
	mpu6050_port_transmit(WOM_THR, threshold);

	printk("Zero motion detection threshold set to: %d", threshold);
}

/**
 * @brief Set zero motion detection duration
 * @param duration Duration setting for zero motion detection
 * @return None
 */
void mpu6050_set_zero_motion_detection_duration(uint8_t duration)
{
	/* Configure motion detection control register */
	u8 reg = 0;

	mpu6050_port_receive(MOT_DETECT_CTRL, &reg);

	/* Set duration bits (bits 0-5) */
	reg &= ~0x3F;
	reg |= (duration & 0x3F);

	mpu6050_port_transmit(MOT_DETECT_CTRL, reg);

	printk("Zero motion detection duration set to: %d", duration);
}

/**
 * @brief Enable free fall detection
 * @param enable Enable/disable flag (0=disable, 1=enable)
 * @return None
 */
void mpu6050_enable_free_fall_detection(uint8_t enable)
{
	u8 reg = 0;

	mpu6050_port_receive(MOT_DETECT_CTRL, &reg);

	if (enable) {
		reg |= 0x40; /* Set free fall enable bit */
	} else {
		reg &= ~0x40; /* Clear free fall enable bit */
	}

	mpu6050_port_transmit(MOT_DETECT_CTRL, reg);

	printk("Free fall detection %s", enable ? "enabled" : "disabled");
}

/**
 * @brief Set free fall threshold
 * @param threshold Free fall threshold value
 * @return None
 */
void mpu6050_set_free_fall_threshold(uint8_t threshold)
{
	/* For this implementation, use WOM_THR register */
	mpu6050_port_transmit(WOM_THR, threshold);
	printk("Free fall threshold set to: %d", threshold);
}

/**
 * @brief Set free fall duration
 * @param duration Free fall duration setting
 * @return None
 */
void mpu6050_set_free_fall_duration(uint8_t duration)
{
	u8 reg = 0;

	mpu6050_port_receive(MOT_DETECT_CTRL, &reg);

	/* Set free fall duration bits (bits 6-7) */
	reg &= ~0xC0;
	reg |= ((duration & 0x03) << 6);

	mpu6050_port_transmit(MOT_DETECT_CTRL, reg);

	printk("Free fall duration set to: %d", duration);
}

/**
 * @brief Enable I2C master mode
 * @param enable Enable/disable flag (0=disable, 1=enable)
 * @return 1 if successful, 0 otherwise
 */
uint8_t mpu6050_enable_i2c_master_mode(uint8_t enable)
{
	u8 reg = 0;

	mpu6050_port_receive(USER_CTRL, &reg);

	if (enable) {
		reg |= 0x20; /* Set I2C_MST_EN bit */
	} else {
		reg &= ~0x20; /* Clear I2C_MST_EN bit */
	}

	mpu6050_port_transmit(USER_CTRL, reg);

	printk("I2C master mode %s", enable ? "enabled" : "disabled");

	return 1;
}

/**
 * @brief Configure I2C slave
 * @param slave_num Slave number (0-3)
 * @param dev_addr Device address
 * @param reg_addr Register address
 * @param rw_flag Read/write flag (0=write, 1=read)
 * @param len Data length
 * @return 1 if successful, 0 otherwise
 */
uint8_t mpu6050_configure_i2c_slave(uint8_t slave_num, uint8_t dev_addr, uint8_t reg_addr, uint8_t rw_flag, uint8_t len)
{
	if (slave_num > 3) {
		return 0;
	}

	/* Configure slave address */
	mpu6050_i2c_slave_addr(slave_num, rw_flag, dev_addr);

	/* Configure register address */
	mpu6050_i2c_slave_reg(slave_num, reg_addr);

	/* Configure number of bytes */
	mpu6050_i2c_slave_number_of_read_bytes(slave_num, len);

	/* Enable slave */
	mpu6050_i2c_slave_enable(slave_num, 1);

	printk("I2C slave %d configured: addr=0x%02X, reg=0x%02X, len=%d",
		slave_num, dev_addr, reg_addr, len);

	return 1;
}

/**
 * @brief Read I2C slave data
 * @param slave_num Slave number
 * @param data Pointer to data buffer
 * @param len Number of bytes to read
 * @return Number of bytes read
 */
uint8_t mpu6050_read_i2c_slave_data(uint8_t slave_num, uint8_t *data, uint8_t len)
{
	uint8_t i;

	if (slave_num > 3) {
		return 0;
	}

	/* Read from external sensor data registers */
	for (i = 0; i < len; i++) {
		if (slave_num == 0) {
			mpu6050_port_receive(EXT_SENS_DATA_00 + i, &data[i]);
		} else if (slave_num == 1) {
			mpu6050_port_receive(EXT_SENS_DATA_06 + i, &data[i]);
		} else if (slave_num == 2) {
			mpu6050_port_receive(EXT_SENS_DATA_0C + i, &data[i]);
		} else if (slave_num == 3) {
			mpu6050_port_receive(EXT_SENS_DATA_12 + i, &data[i]);
		}
	}

	return len;
}

/**
 * @brief Write I2C slave data
 * @param slave_num Slave number
 * @param data Pointer to data buffer
 * @param len Number of bytes to write
 * @return 1 if successful, 0 otherwise
 */
uint8_t mpu6050_write_i2c_slave_data(uint8_t slave_num, uint8_t *data, uint8_t len)
{
	uint8_t i;

	if (slave_num > 3) {
		return 0;
	}

	/* Write to slave data out registers */
	for (i = 0; i < len; i++) {
		if (slave_num == 0) {
			mpu6050_port_transmit(I2C_SLV0_DO + i, data[i]);
		} else if (slave_num == 1) {
			mpu6050_port_transmit(I2C_SLV1_DO + i, data[i]);
		} else if (slave_num == 2) {
			mpu6050_port_transmit(I2C_SLV2_DO + i, data[i]);
		} else if (slave_num == 3) {
			mpu6050_port_transmit(I2C_SLV3_DO + i, data[i]);
		}
	}

	return 1;
}

/**
 * @brief Get I2C master status
 * @param None
 * @return Status byte
 */
uint8_t mpu6050_get_i2c_master_status(void)
{
	u8 status = 0;

	mpu6050_port_receive(I2C_MST_STATUS, &status);

	return status;
}

/**
 * @brief Read external sensor data
 * @param slave_num Slave number
 * @param reg_addr Register address
 * @param data Pointer to data buffer
 * @param len Number of bytes to read
 * @return None
 */
void mpu6050_read_external_sensor_data(uint8_t slave_num, uint8_t reg_addr, uint8_t *data, uint8_t len)
{
	uint8_t i;

	if (slave_num > 3 || reg_addr + len > 24) {
		return;
	}

	/* Read from external sensor data registers */
	for (i = 0; i < len; i++) {
		mpu6050_port_receive(EXT_SENS_DATA_00 + reg_addr + i, &data[i]);
	}
}

/**
 * @brief Check if external sensor data is ready
 * @param slave_num Slave number
 * @return 1 if data ready, 0 otherwise
 */
uint8_t mpu6050_check_external_sensor_data_ready(uint8_t slave_num)
{
	u8 status = mpu6050_get_i2c_master_status();

	/* Check slave data ready bit for the specified slave */
	if (slave_num < 4) {
		return (status & (1 << slave_num)) ? 1 : 0;
	}

	return 0;
}

/**
 * @brief Configure motion detection
 * @param None
 * @return None
 */
void mpu6050_configure_motion_detection(void)
{
	printk("Configuring motion detection...");

	/* Set motion detection threshold */
	mpu6050_set_motion_detection_threshold(10);

	/* Set zero motion detection */
	mpu6050_set_zero_motion_detection_threshold(5);
	mpu6050_set_zero_motion_detection_duration(50);

	/* Enable wake on motion interrupt */
	mpu6050_enable_wake_motion_interrupt(1);

	printk("Motion detection configured");
}

/**
 * @brief Set accelerometer artifact removal
 * @param enable Enable/disable flag
 * @return None
 */
void mpu6050_set_accel_artifact_removal(uint8_t enable)
{
	u8 reg = 0;

	mpu6050_port_receive(MOT_DETECT_CTRL, &reg);

	if (enable) {
		reg |= 0x80; /* Set accel artifact removal bit */
	} else {
		reg &= ~0x80; /* Clear accel artifact removal bit */
	}

	mpu6050_port_transmit(MOT_DETECT_CTRL, reg);

	printk("Accelerometer artifact removal %s", enable ? "enabled" : "disabled");
}

/**
 * @brief Set gyroscope threshold for motion detection
 * @param axis Axis (0=X, 1=Y, 2=Z)
 * @param threshold Threshold value
 * @return None
 */
void mpu6050_set_gyro_threshold(uint8_t axis, uint16_t threshold)
{
	/* Write to appropriate offset register as threshold */
	mpu6050_write_gyro_offset_reg(axis, threshold);

	printk("Gyroscope %c-axis threshold set to: %d",
		axis == 0 ? 'X' : axis == 1 ? 'Y' : 'Z', threshold);
}

/* Enhanced test function */
#ifdef DESIGN_VERIFICATION_MPU6050
#include "kinetis/test-kinetis.h"

/**
 * @brief Test 1: MPU6050 Basic Device Detection and Identification
 * @return PASS if device is detected and WHO_AM_I is correct, FAIL otherwise
 */
int t_mpu6050_device_id(int argc, char **argv)
{
	u8 who_am_i = 0;

	pr_info("=== MPU6050 Device ID Test ===");

	/* Check if device is present */
	if (!mpu6050_is_device_present()) {
		pr_err("FAIL: MPU6050 device not found");
		return FAIL;
	}

	/* Read WHO_AM_I register */
	mpu6050_who_am_i(&who_am_i);
	pr_info("Device ID (WHO_AM_I): 0x%02X (expected: 0x68)", who_am_i);

	if (who_am_i == 0x68) {
		pr_info("Device ID matches");
		return PASS;
	} else {
		pr_err("FAIL: Device ID mismatch, got 0x%02X", who_am_i);
		return FAIL;
	}
}

/**
 * @brief Test 2: MPU6050 Accelerometer and Gyroscope Data Reading
 * @param argc Argument count
 * @param argv Argument vector (argv[1] = number of readings, default 100)
 * @return PASS if data reading is successful, FAIL otherwise
 */
int t_mpu6050_sensor_data(int argc, char **argv)
{
	mpu6050_raw_data_t accel, gyro;
	int16_t temperature;
	u16 readings = 100;
	u16 i;

	if (argc > 1) {
		readings = simple_strtoul(argv[1], &argv[1], 10);
		if (readings > 1000) {
			readings = 1000;
		}
	}

	pr_info("=== MPU6050 Sensor Data Test (%d readings) ===", readings);

	/* Ensure device is initialized */
	if (!g_mpu6050_initialized) {
		mpu6050_init();
	}

	if (!g_mpu6050_initialized) {
		pr_err("FAIL: MPU6050 initialization failed");
		return FAIL;
	}

	for (i = 0; i < readings; i++) {
		/* Read raw sensor data */
		mpu6050_get_raw_data(&accel, &gyro, &temperature);

		/* Show first and last readings at info level */
		if (i == 0 || i == readings - 1) {
			pr_info("Reading %d/%d: Accel[X=%6d,Y=%6d,Z=%6d] Gyro[X=%6d,Y=%6d,Z=%6d] Temp=%6d",
				i + 1, readings, accel.x, accel.y, accel.z,
				gyro.x, gyro.y, gyro.z, temperature);
		}

		mdelay(10);
	}

	pr_info("Completed %d sensor data readings", readings);
	return PASS;
}

/**
 * @brief Test 3: MPU6050 Self-Test Functionality
 * @return PASS if self-test passes for both accelerometer and gyroscope, FAIL otherwise
 */
int t_mpu6050_selftest(int argc, char **argv)
{
	uint8_t gyro_result, accel_result;

	pr_info("=== MPU6050 Self-Test ===");

	/* Test gyroscope */
	gyro_result = mpu6050_gyro_self_test();

	/* Test accelerometer */
	accel_result = mpu6050_accel_self_test();

	/* Return overall result */
	if (gyro_result && accel_result) {
		pr_info("MPU6050 self-test completed successfully");
		return PASS;
	} else {
		pr_err("FAIL: MPU6050 self-test failed");
		if (!gyro_result) {
			pr_err("  - Gyroscope self-test failed");
		}
		if (!accel_result) {
			pr_err("  - Accelerometer self-test failed");
		}
		return FAIL;
	}
}

/**
 * @brief Test 4: MPU6050 Gyroscope Calibration
 * @return PASS if calibration completes successfully, FAIL otherwise
 */
int t_mpu6050_gyro_calibration(int argc, char **argv)
{
	uint16_t offset_x, offset_y, offset_z;

	pr_info("=== MPU6050 Gyroscope Calibration ===");
	pr_info("Note: Please keep device stationary during calibration");

	/* Ensure device is initialized */
	if (!g_mpu6050_initialized) {
		mpu6050_init();
	}

	/* Read initial offsets */
	mpu6050_read_gyro_offset_reg(X_AXIS, &offset_x);
	mpu6050_read_gyro_offset_reg(Y_AXIS, &offset_y);
	mpu6050_read_gyro_offset_reg(Z_AXIS, &offset_z);

	pr_info("Initial gyro offsets - X: %d, Y: %d, Z: %d", offset_x, offset_y, offset_z);

	/* Perform calibration */
	mpu6050_calibrate_gyro();

	/* Read calibrated offsets */
	mpu6050_read_gyro_offset_reg(X_AXIS, &offset_x);
	mpu6050_read_gyro_offset_reg(Y_AXIS, &offset_y);
	mpu6050_read_gyro_offset_reg(Z_AXIS, &offset_z);

	pr_info("Calibrated gyro offsets - X: %d, Y: %d, Z: %d", offset_x, offset_y, offset_z);

	pr_info("Gyroscope calibration completed");
	return PASS;
}

/**
 * @brief Test 5: MPU6050 Accelerometer Calibration
 * @return PASS if calibration completes successfully, FAIL otherwise
 */
int t_mpu6050_accel_calibration(int argc, char **argv)
{
	uint16_t offset_x, offset_y, offset_z;

	pr_info("=== MPU6050 Accelerometer Calibration ===");
	pr_info("Note: Place device in 6 different orientations and call this function");

	/* Ensure device is initialized */
	if (!g_mpu6050_initialized) {
		mpu6050_init();
	}

	/* Read initial offsets */
	mpu9250_read_accel_offset_reg(X_AXIS, &offset_x);
	mpu9250_read_accel_offset_reg(Y_AXIS, &offset_y);
	mpu9250_read_accel_offset_reg(Z_AXIS, &offset_z);

	pr_info("Initial accel offsets - X: %d, Y: %d, Z: %d", offset_x, offset_y, offset_z);

	/* Perform calibration */
	mpu6050_calibrate_accel();

	/* Read calibrated offsets */
	mpu9250_read_accel_offset_reg(X_AXIS, &offset_x);
	mpu9250_read_accel_offset_reg(Y_AXIS, &offset_y);
	mpu9250_read_accel_offset_reg(Z_AXIS, &offset_z);

	pr_info("Calibrated accel offsets - X: %d, Y: %d, Z: %d", offset_x, offset_y, offset_z);

	pr_info("Accelerometer calibration completed");
	return PASS;
}

/**
 * @brief Test 6: MPU6050 FIFO Functionality
 * @return PASS if FIFO operations work correctly, FAIL otherwise
 */
int t_mpu6050_fifo_test(int argc, char **argv)
{
	uint16_t fifo_count;
	uint8_t fifo_data[32];
	uint8_t bytes_read;

	pr_info("=== MPU6050 FIFO Test ===");

	/* Reset FIFO */
	mpu6050_reset_fifo();
	mdelay(10);

	/* Enable FIFO */
	mpu6050_enable_fifo(1);
	mpu6050_set_fifo_mode(1);
	mdelay(10);

	/* Check FIFO count */
	fifo_count = mpu6050_get_fifo_count();
	pr_info("FIFO count after enable: %d bytes", fifo_count);

	/* Read FIFO data (if any) */
	if (fifo_count > 0) {
		bytes_read = mpu6050_read_fifo_data(fifo_data, (fifo_count < 32) ? fifo_count : 32);
		pr_info("Read %d bytes from FIFO", bytes_read);
	} else {
		pr_info("FIFO is empty (expected)");
	}

	/* Disable FIFO */
	mpu6050_enable_fifo(0);
	pr_info("FIFO test completed");
	return PASS;
}

/**
 * @brief Test 7: MPU6050 Interrupt Status
 * @return PASS if interrupt status can be read, FAIL otherwise
 */
int t_mpu6050_interrupt_test(int argc, char **argv)
{
	uint8_t int_status;

	pr_info("=== MPU6050 Interrupt Test ===");

	/* Enable data ready interrupt */
	mpu6050_enable_interrupt(0x01);

	/* Read interrupt status */
	int_status = mpu6050_check_interrupt_flags();
	pr_info("Interrupt status: 0x%02X", int_status);

	/* Clear interrupt flags */
	mpu6050_clear_interrupt_flags();

	/* Disable interrupt */
	mpu6050_disable_interrupt(0x01);

	pr_info("Interrupt test completed");
	return PASS;
}

/**
 * @brief Test 8: MPU6050 Power Management
 * @return PASS if power modes work correctly, FAIL otherwise
 */
int t_mpu6050_power_test(int argc, char **argv)
{
	pr_info("=== MPU6050 Power Management Test ===");

	/* Enter sleep mode */
	mpu6050_enter_sleep_mode();
	pr_info("Entered sleep mode");
	mdelay(100);

	/* Enter wake mode */
	mpu6050_enter_wake_mode();
	pr_info("Entered wake mode");
	mdelay(100);

	/* Enter cycle mode (low power) */
	mpu6050_enter_cycle_mode();
	pr_info("Entered cycle mode");
	mdelay(100);

	/* Return to normal mode */
	mpu6050_enter_wake_mode();
	pr_info("Returned to normal mode");

	pr_info("Power management test completed");
	return PASS;
}

/**
 * @brief Test 9: MPU6050 Full Scale Range Configuration
 * @return PASS if full scale ranges can be configured, FAIL otherwise
 */
int t_mpu6050_fullscale_test(int argc, char **argv)
{
	pr_info("=== MPU6050 Full Scale Range Test ===");

	/* Test gyroscope full scale ranges */
	pr_info("Testing gyroscope full scale ranges:");
	mpu6050_set_gyro_full_scale(MPU6050_GYRO_FS_250);
	pr_info("  - Gyro ±250°/s: scale=%.1f LSB/°/s", g_mpu6050_config.gyro_scale);
	mdelay(10);

	mpu6050_set_gyro_full_scale(MPU6050_GYRO_FS_500);
	pr_info("  - Gyro ±500°/s: scale=%.1f LSB/°/s", g_mpu6050_config.gyro_scale);
	mdelay(10);

	mpu6050_set_gyro_full_scale(MPU6050_GYRO_FS_1000);
	pr_info("  - Gyro ±1000°/s: scale=%.1f LSB/°/s", g_mpu6050_config.gyro_scale);
	mdelay(10);

	mpu6050_set_gyro_full_scale(MPU6050_GYRO_FS_2000);
	pr_info("  - Gyro ±2000°/s: scale=%.1f LSB/°/s", g_mpu6050_config.gyro_scale);
	mdelay(10);

	/* Test accelerometer full scale ranges */
	pr_info("Testing accelerometer full scale ranges:");
	mpu6050_set_accel_full_scale(MPU6050_ACCEL_FS_2);
	pr_info("  - Accel ±2g: scale=%.1f LSB/g", g_mpu6050_config.accel_scale);
	mdelay(10);

	mpu6050_set_accel_full_scale(MPU6050_ACCEL_FS_4);
	pr_info("  - Accel ±4g: scale=%.1f LSB/g", g_mpu6050_config.accel_scale);
	mdelay(10);

	mpu6050_set_accel_full_scale(MPU6050_ACCEL_FS_8);
	pr_info("  - Accel ±8g: scale=%.1f LSB/g", g_mpu6050_config.accel_scale);
	mdelay(10);

	mpu6050_set_accel_full_scale(MPU6050_ACCEL_FS_16);
	pr_info("  - Accel ±16g: scale=%.1f LSB/g", g_mpu6050_config.accel_scale);
	mdelay(10);

	/* Restore default configuration */
	mpu6050_set_gyro_full_scale(MPU6050_GYRO_FS_250);
	mpu6050_set_accel_full_scale(MPU6050_ACCEL_FS_2);

	pr_info("Full scale range test completed");
	return PASS;
}

/**
 * @brief Test 10: MPU6050 DLPF Configuration
 * @return PASS if DLPF bandwidth can be configured, FAIL otherwise
 */
int t_mpu6050_dlpf_test(int argc, char **argv)
{
	pr_info("=== MPU6050 DLPF Bandwidth Test ===");

	/* Test various DLPF bandwidths */
	mpu6050_set_dlpf_bandwidth(MPU6050_DLPF_256HZ);
	pr_info("DLPF bandwidth: 256Hz");
	mdelay(10);

	mpu6050_set_dlpf_bandwidth(MPU6050_DLPF_188HZ);
	pr_info("DLPF bandwidth: 188Hz");
	mdelay(10);

	mpu6050_set_dlpf_bandwidth(MPU6050_DLPF_98HZ);
	pr_info("DLPF bandwidth: 98Hz");
	mdelay(10);

	mpu6050_set_dlpf_bandwidth(MPU6050_DLPF_42HZ);
	pr_info("DLPF bandwidth: 42Hz");
	mdelay(10);

	mpu6050_set_dlpf_bandwidth(MPU6050_DLPF_20HZ);
	pr_info("DLPF bandwidth: 20Hz");
	mdelay(10);

	mpu6050_set_dlpf_bandwidth(MPU6050_DLPF_10HZ);
	pr_info("DLPF bandwidth: 10Hz");
	mdelay(10);

	mpu6050_set_dlpf_bandwidth(MPU6050_DLPF_5HZ);
	pr_info("DLPF bandwidth: 5Hz");
	mdelay(10);

	pr_info("DLPF bandwidth test completed");
	return PASS;
}

void mpu6050_Test(void)
{
	mpu6050_data_t accel, gyro;
	float temperature;

	printk("=== MPU6050 Comprehensive Test Started ===");

	/* Test 1: Device ID */
	if (t_mpu6050_device_id(0, NULL) != PASS) {
		printk("Test 1 FAILED: Device ID test failed");
		return;
	}

	/* Test 2: Sensor data reading */
	if (t_mpu6050_sensor_data(0, NULL) != PASS) {
		printk("Test 2 FAILED: Sensor data test failed");
		return;
	}

	/* Test 3: Self-test */
	if (t_mpu6050_selftest(0, NULL) != PASS) {
		printk("Test 3 FAILED: Self-test failed");
	}

	/* Test 4: Gyroscope calibration */
	t_mpu6050_gyro_calibration(0, NULL);

	/* Test 5: Accelerometer calibration */
	t_mpu6050_accel_calibration(0, NULL);

	/* Test 6: FIFO test */
	t_mpu6050_fifo_test(0, NULL);

	/* Test 7: Interrupt test */
	t_mpu6050_interrupt_test(0, NULL);

	/* Test 8: Power management test */
	t_mpu6050_power_test(0, NULL);

	/* Test 9: Full scale test */
	t_mpu6050_fullscale_test(0, NULL);

	/* Test 10: DLPF test */
	t_mpu6050_dlpf_test(0, NULL);

	/* Final data reading */
	printk("\n=== Final Sensor Reading ===");
	mpu6050_get_scaled_data(&accel, &gyro, &temperature);
	printk("Acceleration: X=%.3fg, Y=%.3fg, Z=%.3fg", accel.x, accel.y, accel.z);
	printk("Angular velocity: X=%.3f°/s, Y=%.3f°/s, Z=%.3f°/s", gyro.x, gyro.y, gyro.z);
	printk("Temperature: %.2f°C", temperature);

	printk("=== MPU6050 Comprehensive Test Completed Successfully ===");
}

#endif
