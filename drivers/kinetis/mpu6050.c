#include "kinetis/mpu6050.h"
#include "kinetis/iic_soft.h"
#include "kinetis/delay.h"
#include "kinetis/idebug.h"

#include <linux/bitops.h>
#include <linux/printk.h>

/* The following program is modified by the user according to the hardware device, otherwise the driver cannot run. */

/**
  * @step 1:  Modify the corresponding function according to the modified area and the corresponding function name.
  * @step 2:  Modify four areas: GPIO_PORT/GPIO_PIN/Led_TypeDef/LEDn.
  * @step 3:  .
  * @step 4:  .
  * @step 5:
  */

#define MPU6050_ADDR                    0x00

static inline void mpu6050_port_transmmit(u8 addr, u8 tmp)
{
    iic_port_transmmit(IIC_1, MPU6050_ADDR, addr, tmp);
}

static inline void mpu6050_port_receive(u8 addr, u8 *pdata)
{
    iic_port_receive(IIC_1, MPU6050_ADDR, addr, pdata);
}

static inline void mpu6050_port_multi_transmmit(u8 addr, u8 *pdata, u32 Length)
{
    iic_port_multi_transmmit(IIC_1, MPU6050_ADDR, addr, pdata, Length);
}

static inline void mpu6050_port_multi_receive(u8 addr, u8 *pdata, u32 Length)
{
    iic_port_multi_receive(IIC_1, MPU6050_ADDR, addr, pdata, Length);
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

void mpu6050_read_gyro_selftest__reg(u8 axis, u8 *pdata)
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
            mpu6050_port_transmmit(SELF_TEST_X_GYRO, tmp);
            break;

        case Y_AXIS:
            mpu6050_port_transmmit(SELF_TEST_Y_GYRO, tmp);
            break;

        case Z_AXIS:
            mpu6050_port_transmmit(SELF_TEST_Z_GYRO, tmp);
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
            mpu6050_port_transmmit(SELF_TEST_X_ACCEL, tmp);
            break;

        case Y_AXIS:
            mpu6050_port_transmmit(SELF_TEST_Y_ACCEL, tmp);
            break;

        case Z_AXIS:
            mpu6050_port_transmmit(SELF_TEST_Z_ACCEL, tmp);
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
            mpu6050_port_transmmit(XG_OFFSET_H, high8);
            mpu6050_port_transmmit(XG_OFFSET_L, low8);
            break;

        case Y_AXIS:
            mpu6050_port_transmmit(YG_OFFSET_H, high8);
            mpu6050_port_transmmit(YG_OFFSET_L, low8);
            break;

        case Z_AXIS:
            mpu6050_port_transmmit(ZG_OFFSET_H, high8);
            mpu6050_port_transmmit(ZG_OFFSET_L, low8);
            break;

        default:
            break;
    }
}

void mpu6050_sample_rate_divider(u8 tmp)
{
    mpu6050_port_transmmit(SMPLRT_DIV, tmp);
}

void mpu6050_fifo_mode(u8 tmp)
{
    u8 reg = 0;

    mpu6050_port_receive(CONFIG, &reg);

    __assign_bit(6, (unsigned long *)&reg, tmp);

    mpu6050_port_transmmit(CONFIG, reg);
}

void mpu6050_fsync_set(u8 tmp)
{
    u8 reg = 0;

    mpu6050_port_receive(CONFIG, &reg);

    set_mask_bits(&reg, 0x07 << 3, tmp);

    mpu6050_port_transmmit(CONFIG, reg);
}

void mpu6050_config_dlpf(u8 tmp)
{
    u8 reg = 0;

    mpu6050_port_receive(CONFIG, &reg);

    set_mask_bits(&reg, 0x07, tmp);

    mpu6050_port_transmmit(CONFIG, reg);
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

    mpu6050_port_transmmit(GYRO_CONFIG, reg);
}

void mpu6050_gyro_full_scale_select(u8 tmp)
{
    u8 reg = 0;

    mpu6050_port_receive(GYRO_CONFIG, &reg);

    set_mask_bits(&reg, 0x03 << 3, tmp << 3);

    mpu6050_port_transmmit(GYRO_CONFIG, reg);
}

void mpu6050_fchoice_b(u8 tmp)
{
    u8 reg = 0;

    mpu6050_port_receive(GYRO_CONFIG, &reg);

    set_mask_bits(&reg, 0x03 << 0, tmp << 0);

    mpu6050_port_transmmit(GYRO_CONFIG, reg);
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

    mpu6050_port_transmmit(ACCEL_CONFIG, reg);
}

void mpu6050_accel_full_scale_select(u8 tmp)
{
    u8 reg = 0;

    mpu6050_port_receive(ACCEL_CONFIG, &reg);

    set_mask_bits(&reg, 0x03 << 3, tmp << 3);

    mpu6050_port_transmmit(ACCEL_CONFIG, reg);
}

void mpu6050_accel_fchoice_b(u8 tmp)
{
    u8 reg = 0;

    mpu6050_port_receive(ACCEL_CONFIG2, &reg);

    set_mask_bits(&reg, 0x01 << 3, tmp << 3);

    mpu6050_port_transmmit(ACCEL_CONFIG2, reg);
}

void mpu6050_config_accel_dlpf(u8 tmp)
{
    u8 reg = 0;

    mpu6050_port_receive(ACCEL_CONFIG2, &reg);

    set_mask_bits(&reg, 0x07 << 0, tmp << 0);

    mpu6050_port_transmmit(ACCEL_CONFIG2, reg);
}

void mpu6050_low_power_accel_odr_control(u8 tmp)
{
    u8 reg = 0;

    mpu6050_port_receive(LP_ACCEL_ODR, &reg);

    set_mask_bits(&reg, 0x0F << 0, tmp << 0);

    mpu6050_port_transmmit(LP_ACCEL_ODR, reg);
}

void mpu6050_wake_on_motion_threshold(u8 tmp)
{
    mpu6050_port_transmmit(WOM_THR, tmp);
}

void mpu6050_fifo_enable_with_temp(u8 tmp)
{
    u8 reg = 0;

    mpu6050_port_receive(FIFO_EN, &reg);

    set_mask_bits(&reg, 0x01 << 7, tmp << 7);

    mpu6050_port_transmmit(FIFO_EN, reg);
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

    mpu6050_port_transmmit(FIFO_EN, reg);
}

void mpu6050_fifo_enable_with_accel(u8 tmp)
{
    u8 reg = 0;

    mpu6050_port_receive(FIFO_EN, &reg);

    set_mask_bits(&reg, 0x01 << 3, tmp << 3);

    mpu6050_port_transmmit(FIFO_EN, reg);
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

    mpu6050_port_transmmit(FIFO_EN, reg);
}

void mpu6050_enable_multi_master(u8 tmp)
{
    u8 reg = 0;

    mpu6050_port_receive(I2C_MST_CTRL, &reg);

    set_mask_bits(&reg, 0x01 << 7, tmp << 7);

    mpu6050_port_transmmit(I2C_MST_CTRL, reg);
}

void mpu6050_wait_for_ext_sensor(u8 tmp)
{
    u8 reg = 0;

    mpu6050_port_receive(I2C_MST_CTRL, &reg);

    set_mask_bits(&reg, 0x01 << 6, tmp << 6);

    mpu6050_port_transmmit(I2C_MST_CTRL, reg);
}

void mpu6050_enable_slave3_fifo(u8 tmp)
{
    u8 reg = 0;

    mpu6050_port_receive(I2C_MST_CTRL, &reg);

    set_mask_bits(&reg, 0x01 << 5, tmp << 5);

    mpu6050_port_transmmit(I2C_MST_CTRL, reg);
}

void mpu6050_i2c_signal_between_read(u8 tmp)
{
    u8 reg = 0;

    mpu6050_port_receive(I2C_MST_CTRL, &reg);

    set_mask_bits(&reg, 0x01 << 4, tmp << 4);

    mpu6050_port_transmmit(I2C_MST_CTRL, reg);
}

void mpu6050_i2c_master_clock(u8 tmp)
{
    u8 reg = 0;

    mpu6050_port_receive(I2C_MST_CTRL, &reg);

    set_mask_bits(&reg, 0x0F << 0, tmp << 0);

    mpu6050_port_transmmit(I2C_MST_CTRL, reg);
}

void mpu6050_i2c_slave_addr(u8 slave, u8 dir, u8 addr)
{
    u8 reg = 0;

    reg |= (dir << 7);
    reg |= (addr << 0);

    switch (slave) {
        case I2C_SLAVE0:
            mpu6050_port_transmmit(I2C_SLV0_ADDR, reg);
            break;

        case I2C_SLAVE1:
            mpu6050_port_transmmit(I2C_SLV1_ADDR, reg);
            break;

        case I2C_SLAVE2:
            mpu6050_port_transmmit(I2C_SLV2_ADDR, reg);
            break;

        case I2C_SLAVE3:
            mpu6050_port_transmmit(I2C_SLV3_ADDR, reg);
            break;

        case I2C_SLAVE4:
            mpu6050_port_transmmit(I2C_SLV4_ADDR, reg);
            break;

        default:
            break;
    }
}

void mpu6050_i2c_slave_reg(u8 slave, u8 reg)
{
    switch (slave) {
        case I2C_SLAVE0:
            mpu6050_port_transmmit(I2C_SLV0_REG, reg);
            break;

        case I2C_SLAVE1:
            mpu6050_port_transmmit(I2C_SLV1_REG, reg);
            break;

        case I2C_SLAVE2:
            mpu6050_port_transmmit(I2C_SLV2_REG, reg);
            break;

        case I2C_SLAVE3:
            mpu6050_port_transmmit(I2C_SLV3_REG, reg);
            break;

        case I2C_SLAVE4:
            mpu6050_port_transmmit(I2C_SLV4_REG, reg);
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
            mpu6050_port_transmmit(I2C_SLV0_CTRL, reg);
            break;

        case I2C_SLAVE1:
            mpu6050_port_receive(I2C_SLV1_CTRL, &reg);
            __assign_bit(7, (unsigned long *)&reg, tmp);
            mpu6050_port_transmmit(I2C_SLV1_CTRL, reg);
            break;

        case I2C_SLAVE2:
            mpu6050_port_receive(I2C_SLV2_CTRL, &reg);
            __assign_bit(7, (unsigned long *)&reg, tmp);
            mpu6050_port_transmmit(I2C_SLV2_CTRL, reg);
            break;

        case I2C_SLAVE3:
            mpu6050_port_receive(I2C_SLV3_CTRL, &reg);
            __assign_bit(7, (unsigned long *)&reg, tmp);
            mpu6050_port_transmmit(I2C_SLV3_CTRL, reg);
            break;

        case I2C_SLAVE4:
            mpu6050_port_receive(I2C_SLV4_CTRL, &reg);
            __assign_bit(7, (unsigned long *)&reg, tmp);
            mpu6050_port_transmmit(I2C_SLV4_CTRL, reg);
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
            mpu6050_port_transmmit(I2C_SLV0_CTRL, reg);
            break;

        case I2C_SLAVE1:
            mpu6050_port_receive(I2C_SLV1_CTRL, &reg);
            __assign_bit(6, (unsigned long *)&reg, tmp);
            mpu6050_port_transmmit(I2C_SLV1_CTRL, reg);
            break;

        case I2C_SLAVE2:
            mpu6050_port_receive(I2C_SLV2_CTRL, &reg);
            __assign_bit(6, (unsigned long *)&reg, tmp);
            mpu6050_port_transmmit(I2C_SLV2_CTRL, reg);
            break;

        case I2C_SLAVE3:
            mpu6050_port_receive(I2C_SLV3_CTRL, &reg);
            __assign_bit(6, (unsigned long *)&reg, tmp);
            mpu6050_port_transmmit(I2C_SLV3_CTRL, reg);
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
            mpu6050_port_transmmit(I2C_SLV0_CTRL, reg);
            break;

        case I2C_SLAVE1:
            mpu6050_port_receive(I2C_SLV1_CTRL, &reg);
            __assign_bit(5, (unsigned long *)&reg, tmp);
            mpu6050_port_transmmit(I2C_SLV1_CTRL, reg);
            break;

        case I2C_SLAVE2:
            mpu6050_port_receive(I2C_SLV2_CTRL, &reg);
            __assign_bit(5, (unsigned long *)&reg, tmp);
            mpu6050_port_transmmit(I2C_SLV2_CTRL, reg);
            break;

        case I2C_SLAVE3:
            mpu6050_port_receive(I2C_SLV3_CTRL, &reg);
            __assign_bit(5, (unsigned long *)&reg, tmp);
            mpu6050_port_transmmit(I2C_SLV3_CTRL, reg);
            break;

        case I2C_SLAVE4:
            mpu6050_port_receive(I2C_SLV4_CTRL, &reg);
            __assign_bit(5, (unsigned long *)&reg, tmp);
            mpu6050_port_transmmit(I2C_SLV4_CTRL, reg);
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
            mpu6050_port_transmmit(I2C_SLV0_CTRL, reg);
            break;

        case I2C_SLAVE1:
            mpu6050_port_receive(I2C_SLV1_CTRL, &reg);
            __assign_bit(4, (unsigned long *)&reg, tmp);
            mpu6050_port_transmmit(I2C_SLV1_CTRL, reg);
            break;

        case I2C_SLAVE2:
            mpu6050_port_receive(I2C_SLV2_CTRL, &reg);
            __assign_bit(4, (unsigned long *)&reg, tmp);
            mpu6050_port_transmmit(I2C_SLV2_CTRL, reg);
            break;

        case I2C_SLAVE3:
            mpu6050_port_receive(I2C_SLV3_CTRL, &reg);
            __assign_bit(4, (unsigned long *)&reg, tmp);
            mpu6050_port_transmmit(I2C_SLV3_CTRL, reg);
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
            mpu6050_port_transmmit(I2C_SLV0_CTRL, reg);
            break;

        case I2C_SLAVE1:
            mpu6050_port_receive(I2C_SLV1_CTRL, &reg);
            set_mask_bits(&reg, 0x0F << 0, tmp << 0);
            mpu6050_port_transmmit(I2C_SLV1_CTRL, reg);
            break;

        case I2C_SLAVE2:
            mpu6050_port_receive(I2C_SLV2_CTRL, &reg);
            set_mask_bits(&reg, 0x0F << 0, tmp << 0);
            mpu6050_port_transmmit(I2C_SLV2_CTRL, reg);
            break;

        case I2C_SLAVE3:
            mpu6050_port_receive(I2C_SLV3_CTRL, &reg);
            set_mask_bits(&reg, 0x0F << 0, tmp << 0);
            mpu6050_port_transmmit(I2C_SLV3_CTRL, reg);
            break;

        case I2C_SLAVE4:
            mpu6050_port_receive(I2C_SLV4_CTRL, &reg);
            set_mask_bits(&reg, 0x0F << 0, tmp << 0);
            mpu6050_port_transmmit(I2C_SLV4_CTRL, reg);
            break;

        default:
            break;
    }
}

void mpu6050_i2c_slave4_do(u8 slave, u8 tmp)
{
    switch (slave) {
        case I2C_SLAVE0:
            mpu6050_port_transmmit(I2C_SLV0_DO, tmp);
            break;

        case I2C_SLAVE1:
            mpu6050_port_transmmit(I2C_SLV1_DO, tmp);
            break;

        case I2C_SLAVE2:
            mpu6050_port_transmmit(I2C_SLV2_DO, tmp);
            break;

        case I2C_SLAVE3:
            mpu6050_port_transmmit(I2C_SLV3_DO, tmp);
            break;

        case I2C_SLAVE4:
            mpu6050_port_transmmit(I2C_SLV4_DO, tmp);
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

    mpu6050_port_transmmit(I2C_SLV4_CTRL, reg);
}

void mpu6050_i2c_slave_master_delay(u8 tmp)
{
    u8 reg = 0;

    mpu6050_port_receive(I2C_SLV4_CTRL, &reg);

    set_mask_bits(&reg, 0x1F << 0, tmp << 0);

    mpu6050_port_transmmit(I2C_SLV4_CTRL, reg);
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

    mpu6050_port_transmmit(INT_PIN_CFG, reg);
}

void mpu6050_enable_pull_up(u8 tmp)
{
    u8 reg = 0;

    mpu6050_port_receive(INT_PIN_CFG, &reg);

    set_mask_bits(&reg, 0x01 << 6, tmp << 6);

    mpu6050_port_transmmit(INT_PIN_CFG, reg);
}

void mpu6050_latch_int_pin(u8 tmp)
{
    u8 reg = 0;

    mpu6050_port_receive(INT_PIN_CFG, &reg);

    set_mask_bits(&reg, 0x01 << 5, tmp << 5);

    mpu6050_port_transmmit(INT_PIN_CFG, reg);
}

void mpu6050_int_anyrd2_clear(u8 tmp)
{
    u8 reg = 0;

    mpu6050_port_receive(INT_PIN_CFG, &reg);

    set_mask_bits(&reg, 0x01 << 4, tmp << 4);

    mpu6050_port_transmmit(INT_PIN_CFG, reg);
}

void mpu6050_logic_level_for_fsync(u8 tmp)
{
    u8 reg = 0;

    mpu6050_port_receive(INT_PIN_CFG, &reg);

    set_mask_bits(&reg, 0x01 << 3, tmp << 3);

    mpu6050_port_transmmit(INT_PIN_CFG, reg);
}

void mpu6050_fsync_int_mode(u8 tmp)
{
    u8 reg = 0;

    mpu6050_port_receive(INT_PIN_CFG, &reg);

    set_mask_bits(&reg, 0x01 << 2, tmp << 2);

    mpu6050_port_transmmit(INT_PIN_CFG, reg);
}

void mpu6050_bypass_mode(u8 tmp)
{
    u8 reg = 0;

    mpu6050_port_receive(INT_PIN_CFG, &reg);

    set_mask_bits(&reg, 0x01 << 1, tmp << 1);

    mpu6050_port_transmmit(INT_PIN_CFG, reg);
}

void mpu6050_int_for_wake_on_motion(u8 tmp)
{
    u8 reg = 0;

    mpu6050_port_receive(INT_ENABLE, &reg);

    set_mask_bits(&reg, 0x01 << 6, tmp << 6);

    mpu6050_port_transmmit(INT_ENABLE, reg);
}

void mpu6050_int_for_fifo_overflow(u8 tmp)
{
    u8 reg = 0;

    mpu6050_port_receive(INT_ENABLE, &reg);

    set_mask_bits(&reg, 0x01 << 4, tmp << 4);

    mpu6050_port_transmmit(INT_ENABLE, reg);
}

void mpu6050_int_for_fsync(u8 tmp)
{
    u8 reg = 0;

    mpu6050_port_receive(INT_ENABLE, &reg);

    set_mask_bits(&reg, 0x01 << 3, tmp << 3);

    mpu6050_port_transmmit(INT_ENABLE, reg);
}

void mpu6050_int_for_raw_sensor_tmp_ready(u8 tmp)
{
    u8 reg = 0;

    mpu6050_port_receive(INT_ENABLE, &reg);

    set_mask_bits(&reg, 0x01 << 0, tmp << 0);

    mpu6050_port_transmmit(INT_ENABLE, reg);
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

    mpu6050_port_transmmit(I2C_MST_DELAY_CTRL, reg);
}

void mpu6050_i2c_slave_delay_enable(u8 slave, u8 tmp)
{
    u8 reg = 0;

    tmp &= 0x01;

    switch (slave) {
        case I2C_SLAVE0:
            mpu6050_port_receive(I2C_MST_DELAY_CTRL, &reg);
            set_mask_bits(&reg, 0x01 << 0, tmp << 0);
            mpu6050_port_transmmit(I2C_MST_DELAY_CTRL, reg);
            break;

        case I2C_SLAVE1:
            mpu6050_port_receive(I2C_MST_DELAY_CTRL, &reg);
            set_mask_bits(&reg, 0x01 << 1, tmp << 1);
            mpu6050_port_transmmit(I2C_MST_DELAY_CTRL, reg);
            break;

        case I2C_SLAVE2:
            mpu6050_port_receive(I2C_MST_DELAY_CTRL, &reg);
            set_mask_bits(&reg, 0x01 << 2, tmp << 2);
            mpu6050_port_transmmit(I2C_MST_DELAY_CTRL, reg);
            break;

        case I2C_SLAVE3:
            mpu6050_port_receive(I2C_MST_DELAY_CTRL, &reg);
            set_mask_bits(&reg, 0x01 << 3, tmp << 3);
            mpu6050_port_transmmit(I2C_MST_DELAY_CTRL, reg);
            break;

        case I2C_SLAVE4:
            mpu6050_port_receive(I2C_MST_DELAY_CTRL, &reg);
            set_mask_bits(&reg, 0x01 << 4, tmp << 4);
            mpu6050_port_transmmit(I2C_MST_DELAY_CTRL, reg);
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

    mpu6050_port_transmmit(SIGNAL_PATH_RESET, reg);
}

void mpu6050_accel_signal_path_reset(u8 tmp)
{
    u8 reg = 0;

    mpu6050_port_receive(SIGNAL_PATH_RESET, &reg);

    set_mask_bits(&reg, 0x01 << 1, tmp << 1);

    mpu6050_port_transmmit(SIGNAL_PATH_RESET, reg);
}

void mpu6050_temp_signal_path_reset(u8 tmp)
{
    u8 reg = 0;

    mpu6050_port_receive(SIGNAL_PATH_RESET, &reg);

    set_mask_bits(&reg, 0x01 << 0, tmp << 0);

    mpu6050_port_transmmit(SIGNAL_PATH_RESET, reg);
}

void mpu6050_enable_wake_on_motion(u8 tmp)
{
    u8 reg = 0;

    mpu6050_port_receive(MOT_DETECT_CTRL, &reg);

    set_mask_bits(&reg, 0x01 << 7, tmp << 7);

    mpu6050_port_transmmit(MOT_DETECT_CTRL, reg);
}

void mpu6050_accel_int_mode(u8 tmp)
{
    u8 reg = 0;

    mpu6050_port_receive(MOT_DETECT_CTRL, &reg);

    set_mask_bits(&reg, 0x01 << 6, tmp << 6);

    mpu6050_port_transmmit(MOT_DETECT_CTRL, reg);
}

void mpu6050_enable_fifo(u8 tmp)
{
    u8 reg = 0;

    mpu6050_port_receive(USER_CTRL, &reg);

    set_mask_bits(&reg, 0x01 << 6, tmp << 6);

    mpu6050_port_transmmit(USER_CTRL, reg);
}

void mpu6050_enable_i2c_master(u8 tmp)
{
    u8 reg = 0;

    mpu6050_port_receive(USER_CTRL, &reg);

    set_mask_bits(&reg, 0x01 << 5, tmp << 5);

    mpu6050_port_transmmit(USER_CTRL, reg);
}

void mpu6050_disable_i2c_slave(u8 tmp)
{
    u8 reg = 0;

    mpu6050_port_receive(USER_CTRL, &reg);

    set_mask_bits(&reg, 0x01 << 4, tmp << 4);

    mpu6050_port_transmmit(USER_CTRL, reg);
}

void mpu6050_reset_fifo(u8 tmp)
{
    u8 reg = 0;

    mpu6050_port_receive(USER_CTRL, &reg);

    set_mask_bits(&reg, 0x01 << 2, tmp << 2);

    mpu6050_port_transmmit(USER_CTRL, reg);
}

void mpu6050_reset_i2c_master(u8 tmp)
{
    u8 reg = 0;

    mpu6050_port_receive(USER_CTRL, &reg);

    set_mask_bits(&reg, 0x01 << 1, tmp << 1);

    mpu6050_port_transmmit(USER_CTRL, reg);
}

void mpu6050_reset_signal_path(u8 tmp)
{
    u8 reg = 0;

    mpu6050_port_receive(USER_CTRL, &reg);

    set_mask_bits(&reg, 0x01 << 0, tmp << 0);

    mpu6050_port_transmmit(USER_CTRL, reg);
}

void mpu6050_soft_reset(u8 tmp)
{
    u8 reg = 0;

    mpu6050_port_receive(PWR_MGMT_1, &reg);

    set_mask_bits(&reg, 0x01 << 7, tmp << 7);

    mpu6050_port_transmmit(PWR_MGMT_1, reg);
}

void mpu6050_enter_sleep(u8 tmp)
{
    u8 reg = 0;

    mpu6050_port_receive(PWR_MGMT_1, &reg);

    set_mask_bits(&reg, 0x01 << 6, tmp << 6);

    mpu6050_port_transmmit(PWR_MGMT_1, reg);
}

void mpu6050_cycle_sample(u8 tmp)
{
    u8 reg = 0;

    mpu6050_port_receive(PWR_MGMT_1, &reg);

    set_mask_bits(&reg, 0x01 << 5, tmp << 5);

    mpu6050_port_transmmit(PWR_MGMT_1, reg);
}

void mpu6050_gyro_standby(u8 tmp)
{
    u8 reg = 0;

    mpu6050_port_receive(PWR_MGMT_1, &reg);

    set_mask_bits(&reg, 0x01 << 4, tmp << 4);

    mpu6050_port_transmmit(PWR_MGMT_1, reg);
}

void mpu6050_power_down_ptat(u8 tmp)
{
    u8 reg = 0;

    mpu6050_port_receive(PWR_MGMT_1, &reg);

    set_mask_bits(&reg, 0x01 << 3, tmp << 3);

    mpu6050_port_transmmit(PWR_MGMT_1, reg);
}

void mpu6050_clock_source_select(u8 tmp)
{
    u8 reg = 0;

    mpu6050_port_receive(PWR_MGMT_1, &reg);

    set_mask_bits(&reg, 0x07 << 0, tmp << 0);

    mpu6050_port_transmmit(PWR_MGMT_1, reg);
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

    mpu6050_port_transmmit(PWR_MGMT_2, reg);
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

    mpu6050_port_transmmit(PWR_MGMT_2, reg);
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
    mpu6050_port_transmmit(FIFO_R_W, tmp);
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
            mpu6050_port_transmmit(XA_OFFSET_H, high8);
            mpu6050_port_transmmit(XA_OFFSET_L, low8);
            break;

        case Y_AXIS:
            mpu6050_port_transmmit(YA_OFFSET_H, high8);
            mpu6050_port_transmmit(YA_OFFSET_L, low8);
            break;

        case Z_AXIS:
            mpu6050_port_transmmit(ZA_OFFSET_H, high8);
            mpu6050_port_transmmit(ZA_OFFSET_L, low8);
            break;

        default:
            break;
    }
}

#ifdef DESIGN_VERIFICATION_MPU6050

void mpu6050_Test(void)
{

}

#endif


