#include "kinetis/mpu6050.h"

/* The following program is modified by the user according to the hardware device, otherwise the driver cannot run. */

/**
  * @step 1:  Modify the corresponding function according to the modified area and the corresponding function name.
  * @step 2:  Modify four areas: GPIO_PORT/GPIO_PIN/Led_TypeDef/LEDn.
  * @step 3:  .
  * @step 4:  .
  * @step 5:
  */

#include "kinetis/iic_soft.h"
#include <linux/delay.h>
#include "kinetis/idebug.h"

#define MPU6050_ADDR                    0x00

void mpu6050_PortTransmmit(u8 Addr, u8 Data)
{
    iic_port_transmmit(IIC_1, MPU6050_ADDR, Addr, Data);
}

void mpu6050_PortReceive(u8 Addr, u8 *pData)
{
    iic_port_transmmit(IIC_1, MPU6050_ADDR, Addr, pData);
}

void mpu6050_port_multi_transmmit(u8 Addr, u8 *pData, u32 Length)
{
    iic_port_multi_transmmit(IIC_1, MPU6050_ADDR, Addr, pData, Length);
}

void mpu6050_port_multi_receive(u8 Addr, u8 *pData, u32 Length)
{
    iic_port_multi_receive(IIC_1, MPU6050_ADDR, Addr, pData, Length);
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

u8 g_Temp_Sensitivity = 0;

void mpu6050_ReadGyroSelfTestRegisters(u8 Axis, u8 *pData)
{
    switch (Axis) {
        case X_AXIS:
            mpu6050_PortReceive(SELF_TEST_X_GYRO, pData);
            break;

        case Y_AXIS:
            mpu6050_PortReceive(SELF_TEST_Y_GYRO, pData);
            break;

        case Z_AXIS:
            mpu6050_PortReceive(SELF_TEST_Z_GYRO, pData);
            break;

        default:
            break;
    }
}

void mpu6050_WriteGyroSelfTestRegisters(u8 Axis, u8 Data)
{
    switch (Axis) {
        case X_AXIS:
            mpu6050_PortTransmmit(SELF_TEST_X_GYRO, Data);
            break;

        case Y_AXIS:
            mpu6050_PortTransmmit(SELF_TEST_Y_GYRO, Data);
            break;

        case Z_AXIS:
            mpu6050_PortTransmmit(SELF_TEST_Z_GYRO, Data);
            break;

        default:
            break;
    }
}

void mpu6050_ReadAccelSelfTestRegisters(u8 Axis, u8 *pData)
{
    switch (Axis) {
        case X_AXIS:
            mpu6050_PortReceive(SELF_TEST_X_ACCEL, pData);
            break;

        case Y_AXIS:
            mpu6050_PortReceive(SELF_TEST_Y_ACCEL, pData);
            break;

        case Z_AXIS:
            mpu6050_PortReceive(SELF_TEST_Z_ACCEL, pData);
            break;

        default:
            break;
    }
}

void mpu6050_WriteAccelSelfTestRegisters(u8 Axis, u8 Data)
{
    switch (Axis) {
        case X_AXIS:
            mpu6050_PortTransmmit(SELF_TEST_X_ACCEL, Data);
            break;

        case Y_AXIS:
            mpu6050_PortTransmmit(SELF_TEST_Y_ACCEL, Data);
            break;

        case Z_AXIS:
            mpu6050_PortTransmmit(SELF_TEST_Z_ACCEL, Data);
            break;

        default:
            break;
    }
}

void mpu6050_ReadGyroOffsetRegisters(u8 Axis, u16 *pData)
{
    u8 SubData1 = 0;
    u8 SubData2 = 0;

    switch (Axis) {
        case X_AXIS:
            mpu6050_PortReceive(XG_OFFSET_H, &SubData1);
            mpu6050_PortReceive(XG_OFFSET_L, &SubData2);
            break;

        case Y_AXIS:
            mpu6050_PortReceive(YG_OFFSET_H, &SubData1);
            mpu6050_PortReceive(YG_OFFSET_L, &SubData2);
            break;

        case Z_AXIS:
            mpu6050_PortReceive(ZG_OFFSET_H, &SubData1);
            mpu6050_PortReceive(ZG_OFFSET_L, &SubData2);
            break;

        default:
            break;
    }

    *pData = (SubData1 << 8) | SubData2;
}

void mpu6050_WriteGyroOffsetRegisters(u8 Axis, u16 Data)
{
    u8 SubData1 = 0;
    u8 SubData2 = 0;

    SubData1 = Data >> 8;
    SubData2 = Data & 0xFF;

    switch (Axis) {
        case X_AXIS:
            mpu6050_PortTransmmit(XG_OFFSET_H, SubData1);
            mpu6050_PortTransmmit(XG_OFFSET_L, SubData2);
            break;

        case Y_AXIS:
            mpu6050_PortTransmmit(YG_OFFSET_H, SubData1);
            mpu6050_PortTransmmit(YG_OFFSET_L, SubData2);
            break;

        case Z_AXIS:
            mpu6050_PortTransmmit(ZG_OFFSET_H, SubData1);
            mpu6050_PortTransmmit(ZG_OFFSET_L, SubData2);
            break;

        default:
            break;
    }
}

void mpu6050_SampleRateDivider(u8 Data)
{
    mpu6050_PortTransmmit(SMPLRT_DIV, Data);
}

void mpu6050_FIFOMode(u8 Data)
{
    u8 TmpReg = 0;

    mpu6050_PortReceive(CONFIG, &TmpReg);

    assign_bit(6, &TmpReg, Data);

    mpu6050_PortTransmmit(CONFIG, TmpReg);
}

void mpu6050_FsyncSet(u8 Data)
{
    u8 TmpReg = 0;

    mpu6050_PortReceive(CONFIG, &TmpReg);

    set_mask_bits(&TmpReg, 0x07 << 3, Data);

    mpu6050_PortTransmmit(CONFIG, TmpReg);
}

void mpu6050_ConfigDLPF(u8 Data)
{
    u8 TmpReg = 0;

    mpu6050_PortReceive(CONFIG, &TmpReg);

    set_mask_bits(&TmpReg, 0x07, Data);

    mpu6050_PortTransmmit(CONFIG, TmpReg);
}

void mpu6050_GyroSelfTest(u8 Axis, u8 Data)
{
    u8 TmpReg = 0;

    mpu6050_PortReceive(GYRO_CONFIG, &TmpReg);
    Data &= 0x01;

    switch (Axis) {
        case X_AXIS:
            TmpReg &= ~(0x01 << 7);
            TmpReg |= (Data << 7);
            break;

        case Y_AXIS:
            TmpReg &= ~(0x01 << 6);
            TmpReg |= (Data << 6);
            break;

        case Z_AXIS:
            TmpReg &= ~(0x01 << 5);
            TmpReg |= (Data << 5);
            break;

        default:
            break;
    }

    mpu6050_PortTransmmit(GYRO_CONFIG, TmpReg);
}

void mpu6050_GyroFullScaleSelect(u8 Data)
{
    u8 TmpReg = 0;

    mpu6050_PortReceive(GYRO_CONFIG, &TmpReg);

    Data &= 0x03;
    TmpReg &= ~(0x03 << 3);
    TmpReg |= (Data << 3);

    mpu6050_PortTransmmit(GYRO_CONFIG, TmpReg);
}

void mpu6050_Fchoice_b(u8 Data)
{
    u8 TmpReg = 0;

    mpu6050_PortReceive(GYRO_CONFIG, &TmpReg);

    Data &= 0x03;
    TmpReg &= ~(0x03 << 0);
    TmpReg |= (Data << 0);

    mpu6050_PortTransmmit(GYRO_CONFIG, TmpReg);
}

void mpu6050_AccelSelfTest(u8 Axis, u8 Data)
{
    u8 TmpReg = 0;

    mpu6050_PortReceive(ACCEL_CONFIG, &TmpReg);
    Data &= 0x01;

    switch (Axis) {
        case X_AXIS:
            TmpReg &= ~(0x01 << 7);
            TmpReg |= (Data << 7);
            break;

        case Y_AXIS:
            TmpReg &= ~(0x01 << 6);
            TmpReg |= (Data << 6);
            break;

        case Z_AXIS:
            TmpReg &= ~(0x01 << 5);
            TmpReg |= (Data << 5);
            break;

        default:
            break;
    }

    mpu6050_PortTransmmit(ACCEL_CONFIG, TmpReg);
}

void mpu6050_AccelFullScaleSelect(u8 Data)
{
    u8 TmpReg = 0;

    mpu6050_PortReceive(ACCEL_CONFIG, &TmpReg);

    Data &= 0x03;
    TmpReg &= ~(0x03 << 3);
    TmpReg |= (Data << 3);

    mpu6050_PortTransmmit(ACCEL_CONFIG, TmpReg);
}

void mpu6050_Accel_Fchoice_b(u8 Data)
{
    u8 TmpReg = 0;

    mpu6050_PortReceive(ACCEL_CONFIG2, &TmpReg);

    Data &= 0x01;
    TmpReg &= ~(0x01 << 3);
    TmpReg |= (Data << 3);

    mpu6050_PortTransmmit(ACCEL_CONFIG2, TmpReg);
}

void mpu6050_ConfigAccelDLPF(u8 Data)
{
    u8 TmpReg = 0;

    mpu6050_PortReceive(ACCEL_CONFIG2, &TmpReg);

    Data &= 0x07;
    TmpReg &= ~(0x07 << 0);
    TmpReg |= (Data << 0);

    mpu6050_PortTransmmit(ACCEL_CONFIG2, TmpReg);
}

void mpu6050_LowPowerAccelODRControl(u8 Data)
{
    u8 TmpReg = 0;

    mpu6050_PortReceive(LP_ACCEL_ODR, &TmpReg);

    Data &= 0x0F;
    TmpReg &= ~(0x0F << 0);
    TmpReg |= (Data << 0);

    mpu6050_PortTransmmit(LP_ACCEL_ODR, TmpReg);
}

void mpu6050_WakeonMotionThreshold(u8 Data)
{
    mpu6050_PortTransmmit(WOM_THR, Data);
}

void mpu6050_FIFOEnableWithTemp(u8 Data)
{
    u8 TmpReg = 0;

    mpu6050_PortReceive(FIFO_EN, &TmpReg);

    Data &= 0x01;
    TmpReg &= ~(0x01 << 7);
    TmpReg |= (Data << 7);

    mpu6050_PortTransmmit(FIFO_EN, TmpReg);
}

void mpu6050_FIFOEnableWithGyro(u8 Axis, u8 Data)
{
    u8 TmpReg = 0;

    mpu6050_PortReceive(FIFO_EN, &TmpReg);
    Data &= 0x01;

    switch (Axis) {
        case X_AXIS:
            TmpReg &= ~(0x01 << 6);
            TmpReg |= (Data << 6);
            break;

        case Y_AXIS:
            TmpReg &= ~(0x01 << 5);
            TmpReg |= (Data << 5);
            break;

        case Z_AXIS:
            TmpReg &= ~(0x01 << 4);
            TmpReg |= (Data << 4);
            break;

        default:
            break;
    }

    mpu6050_PortTransmmit(FIFO_EN, TmpReg);
}

void mpu6050_FIFOEnableWithAccel(u8 Data)
{
    u8 TmpReg = 0;

    mpu6050_PortReceive(FIFO_EN, &TmpReg);

    Data &= 0x01;
    TmpReg &= ~(0x01 << 3);
    TmpReg |= (Data << 3);

    mpu6050_PortTransmmit(FIFO_EN, TmpReg);
}

void mpu6050_FIFOEnableWithExtSensor(u8 Slave, u8 Data)
{
    u8 TmpReg = 0;

    mpu6050_PortReceive(FIFO_EN, &TmpReg);
    Data &= 0x01;

    switch (Slave) {
        case I2C_SLAVE2:
            TmpReg &= ~(0x01 << 2);
            TmpReg |= (Data << 2);
            break;

        case I2C_SLAVE1:
            TmpReg &= ~(0x01 << 1);
            TmpReg |= (Data << 1);
            break;

        case I2C_SLAVE0:
            TmpReg &= ~(0x01 << 0);
            TmpReg |= (Data << 0);
            break;

        default:
            break;
    }

    mpu6050_PortTransmmit(FIFO_EN, TmpReg);
}

void mpu6050_EnableMultimaster(u8 Data)
{
    u8 TmpReg = 0;

    mpu6050_PortReceive(I2C_MST_CTRL, &TmpReg);

    Data &= 0x01;
    TmpReg &= ~(0x01 << 7);
    TmpReg |= (Data << 7);

    mpu6050_PortTransmmit(I2C_MST_CTRL, TmpReg);
}

void mpu6050_WaitForExtSensor(u8 Data)
{
    u8 TmpReg = 0;

    mpu6050_PortReceive(I2C_MST_CTRL, &TmpReg);

    Data &= 0x01;
    TmpReg &= ~(0x01 << 6);
    TmpReg |= (Data << 6);

    mpu6050_PortTransmmit(I2C_MST_CTRL, TmpReg);
}

void mpu6050_EnableSlave3FIFO(u8 Data)
{
    u8 TmpReg = 0;

    mpu6050_PortReceive(I2C_MST_CTRL, &TmpReg);

    Data &= 0x01;
    TmpReg &= ~(0x01 << 5);
    TmpReg |= (Data << 5);

    mpu6050_PortTransmmit(I2C_MST_CTRL, TmpReg);
}

void mpu6050_I2CSignalBetweenRead(u8 Data)
{
    u8 TmpReg = 0;

    mpu6050_PortReceive(I2C_MST_CTRL, &TmpReg);

    Data &= 0x01;
    TmpReg &= ~(0x01 << 4);
    TmpReg |= (Data << 4);

    mpu6050_PortTransmmit(I2C_MST_CTRL, TmpReg);
}

void mpu6050_I2CMasterClock(u8 Data)
{
    u8 TmpReg = 0;

    mpu6050_PortReceive(I2C_MST_CTRL, &TmpReg);

    Data &= 0x0F;
    TmpReg &= ~(0x0F << 0);
    TmpReg |= (Data << 0);

    mpu6050_PortTransmmit(I2C_MST_CTRL, TmpReg);
}

void mpu6050_I2CSlaveAddr(u8 Slave, u8 Dir, u8 Addr)
{
    u8 TmpReg = 0;

    TmpReg |= (Dir << 7);
    TmpReg |= (Addr << 0);

    switch (Slave) {
        case I2C_SLAVE0:
            mpu6050_PortTransmmit(I2C_SLV0_ADDR, TmpReg);
            break;

        case I2C_SLAVE1:
            mpu6050_PortTransmmit(I2C_SLV1_ADDR, TmpReg);
            break;

        case I2C_SLAVE2:
            mpu6050_PortTransmmit(I2C_SLV2_ADDR, TmpReg);
            break;

        case I2C_SLAVE3:
            mpu6050_PortTransmmit(I2C_SLV3_ADDR, TmpReg);
            break;

        case I2C_SLAVE4:
            mpu6050_PortTransmmit(I2C_SLV4_ADDR, TmpReg);
            break;

        default:
            break;
    }
}

void mpu6050_I2CSlaveReg(u8 Slave, u8 Reg)
{
    switch (Slave) {
        case I2C_SLAVE0:
            mpu6050_PortTransmmit(I2C_SLV0_REG, Reg);
            break;

        case I2C_SLAVE1:
            mpu6050_PortTransmmit(I2C_SLV1_REG, Reg);
            break;

        case I2C_SLAVE2:
            mpu6050_PortTransmmit(I2C_SLV2_REG, Reg);
            break;

        case I2C_SLAVE3:
            mpu6050_PortTransmmit(I2C_SLV3_REG, Reg);
            break;

        case I2C_SLAVE4:
            mpu6050_PortTransmmit(I2C_SLV4_REG, Reg);
            break;

        default:
            break;
    }
}

void mpu6050_I2CSlaveEnable(u8 Slave, u8 Data)
{
    u8 TmpReg = 0;

    Data &= 0x01;

    switch (Slave) {
        case I2C_SLAVE0:
            mpu6050_PortReceive(I2C_SLV0_CTRL, &TmpReg);
            TmpReg &= ~(0x01 << 7);
            TmpReg |= (Data << 7);
            mpu6050_PortTransmmit(I2C_SLV0_CTRL, TmpReg);
            break;

        case I2C_SLAVE1:
            mpu6050_PortReceive(I2C_SLV1_CTRL, &TmpReg);
            TmpReg &= ~(0x01 << 7);
            TmpReg |= (Data << 7);
            mpu6050_PortTransmmit(I2C_SLV1_CTRL, TmpReg);
            break;

        case I2C_SLAVE2:
            mpu6050_PortReceive(I2C_SLV2_CTRL, &TmpReg);
            TmpReg &= ~(0x01 << 7);
            TmpReg |= (Data << 7);
            mpu6050_PortTransmmit(I2C_SLV2_CTRL, TmpReg);
            break;

        case I2C_SLAVE3:
            mpu6050_PortReceive(I2C_SLV3_CTRL, &TmpReg);
            TmpReg &= ~(0x01 << 7);
            TmpReg |= (Data << 7);
            mpu6050_PortTransmmit(I2C_SLV3_CTRL, TmpReg);
            break;

        case I2C_SLAVE4:
            mpu6050_PortReceive(I2C_SLV4_CTRL, &TmpReg);
            TmpReg &= ~(0x01 << 7);
            TmpReg |= (Data << 7);
            mpu6050_PortTransmmit(I2C_SLV4_CTRL, TmpReg);
            break;

        default:
            break;
    }
}

void mpu6050_I2CSlaveSwapBytes(u8 Slave, u8 Data)
{
    u8 TmpReg = 0;

    Data &= 0x01;

    switch (Slave) {
        case I2C_SLAVE0:
            mpu6050_PortReceive(I2C_SLV0_CTRL, &TmpReg);
            TmpReg &= ~(0x01 << 6);
            TmpReg |= (Data << 6);
            mpu6050_PortTransmmit(I2C_SLV0_CTRL, TmpReg);
            break;

        case I2C_SLAVE1:
            mpu6050_PortReceive(I2C_SLV1_CTRL, &TmpReg);
            TmpReg &= ~(0x01 << 6);
            TmpReg |= (Data << 6);
            mpu6050_PortTransmmit(I2C_SLV1_CTRL, TmpReg);
            break;

        case I2C_SLAVE2:
            mpu6050_PortReceive(I2C_SLV2_CTRL, &TmpReg);
            TmpReg &= ~(0x01 << 6);
            TmpReg |= (Data << 6);
            mpu6050_PortTransmmit(I2C_SLV2_CTRL, TmpReg);
            break;

        case I2C_SLAVE3:
            mpu6050_PortReceive(I2C_SLV3_CTRL, &TmpReg);
            TmpReg &= ~(0x01 << 6);
            TmpReg |= (Data << 6);
            mpu6050_PortTransmmit(I2C_SLV3_CTRL, TmpReg);
            break;

        default:
            break;
    }
}

void mpu6050_I2CSlaveDisReg(u8 Slave, u8 Data)
{
    u8 TmpReg = 0;

    Data &= 0x01;

    switch (Slave) {
        case I2C_SLAVE0:
            mpu6050_PortReceive(I2C_SLV0_CTRL, &TmpReg);
            TmpReg &= ~(0x01 << 5);
            TmpReg |= (Data << 5);
            mpu6050_PortTransmmit(I2C_SLV0_CTRL, TmpReg);
            break;

        case I2C_SLAVE1:
            mpu6050_PortReceive(I2C_SLV1_CTRL, &TmpReg);
            TmpReg &= ~(0x01 << 5);
            TmpReg |= (Data << 5);
            mpu6050_PortTransmmit(I2C_SLV1_CTRL, TmpReg);
            break;

        case I2C_SLAVE2:
            mpu6050_PortReceive(I2C_SLV2_CTRL, &TmpReg);
            TmpReg &= ~(0x01 << 5);
            TmpReg |= (Data << 5);
            mpu6050_PortTransmmit(I2C_SLV2_CTRL, TmpReg);
            break;

        case I2C_SLAVE3:
            mpu6050_PortReceive(I2C_SLV3_CTRL, &TmpReg);
            TmpReg &= ~(0x01 << 5);
            TmpReg |= (Data << 5);
            mpu6050_PortTransmmit(I2C_SLV3_CTRL, TmpReg);
            break;

        case I2C_SLAVE4:
            mpu6050_PortReceive(I2C_SLV4_CTRL, &TmpReg);
            TmpReg &= ~(0x01 << 5);
            TmpReg |= (Data << 5);
            mpu6050_PortTransmmit(I2C_SLV4_CTRL, TmpReg);
            break;

        default:
            break;
    }
}

void mpu6050_I2CSlaveGroupType(u8 Slave, u8 Data)
{
    u8 TmpReg = 0;

    Data &= 0x01;

    switch (Slave) {
        case I2C_SLAVE0:
            mpu6050_PortReceive(I2C_SLV0_CTRL, &TmpReg);
            TmpReg &= ~(0x01 << 4);
            TmpReg |= (Data << 4);
            mpu6050_PortTransmmit(I2C_SLV0_CTRL, TmpReg);
            break;

        case I2C_SLAVE1:
            mpu6050_PortReceive(I2C_SLV1_CTRL, &TmpReg);
            TmpReg &= ~(0x01 << 4);
            TmpReg |= (Data << 4);
            mpu6050_PortTransmmit(I2C_SLV1_CTRL, TmpReg);
            break;

        case I2C_SLAVE2:
            mpu6050_PortReceive(I2C_SLV2_CTRL, &TmpReg);
            TmpReg &= ~(0x01 << 4);
            TmpReg |= (Data << 4);
            mpu6050_PortTransmmit(I2C_SLV2_CTRL, TmpReg);
            break;

        case I2C_SLAVE3:
            mpu6050_PortReceive(I2C_SLV3_CTRL, &TmpReg);
            TmpReg &= ~(0x01 << 4);
            TmpReg |= (Data << 4);
            mpu6050_PortTransmmit(I2C_SLV3_CTRL, TmpReg);
            break;

        default:
            break;
    }
}

void mpu6050_I2CSlaveNumberOfReadBytes(u8 Slave, u8 Data)
{
    u8 TmpReg = 0;

    Data &= 0x0F;

    switch (Slave) {
        case I2C_SLAVE0:
            mpu6050_PortReceive(I2C_SLV0_CTRL, &TmpReg);
            TmpReg &= ~(0x0F << 0);
            TmpReg |= (Data << 0);
            mpu6050_PortTransmmit(I2C_SLV0_CTRL, TmpReg);
            break;

        case I2C_SLAVE1:
            mpu6050_PortReceive(I2C_SLV1_CTRL, &TmpReg);
            TmpReg &= ~(0x0F << 0);
            TmpReg |= (Data << 0);
            mpu6050_PortTransmmit(I2C_SLV1_CTRL, TmpReg);
            break;

        case I2C_SLAVE2:
            mpu6050_PortReceive(I2C_SLV2_CTRL, &TmpReg);
            TmpReg &= ~(0x0F << 0);
            TmpReg |= (Data << 0);
            mpu6050_PortTransmmit(I2C_SLV2_CTRL, TmpReg);
            break;

        case I2C_SLAVE3:
            mpu6050_PortReceive(I2C_SLV3_CTRL, &TmpReg);
            TmpReg &= ~(0x0F << 0);
            TmpReg |= (Data << 0);
            mpu6050_PortTransmmit(I2C_SLV3_CTRL, TmpReg);
            break;

        case I2C_SLAVE4:
            mpu6050_PortReceive(I2C_SLV4_CTRL, &TmpReg);
            TmpReg &= ~(0x0F << 0);
            TmpReg |= (Data << 0);
            mpu6050_PortTransmmit(I2C_SLV4_CTRL, TmpReg);
            break;

        default:
            break;
    }
}

void mpu6050_I2CSlave4DO(u8 Slave, u8 Data)
{
    switch (Slave) {
        case I2C_SLAVE0:
            mpu6050_PortTransmmit(I2C_SLV0_DO, Data);
            break;

        case I2C_SLAVE1:
            mpu6050_PortTransmmit(I2C_SLV1_DO, Data);
            break;

        case I2C_SLAVE2:
            mpu6050_PortTransmmit(I2C_SLV2_DO, Data);
            break;

        case I2C_SLAVE3:
            mpu6050_PortTransmmit(I2C_SLV3_DO, Data);
            break;

        case I2C_SLAVE4:
            mpu6050_PortTransmmit(I2C_SLV4_DO, Data);
            break;

        default:
            break;
    }
}

void mpu6050_I2CSlaveEnableInt(u8 Data)
{
    u8 TmpReg = 0;

    mpu6050_PortReceive(I2C_SLV4_CTRL, &TmpReg);

    Data &= 0x01;
    TmpReg &= ~(0x01 << 6);
    TmpReg |= (Data << 6);

    mpu6050_PortTransmmit(I2C_SLV4_CTRL, TmpReg);
}

void mpu6050_I2CSlaveMasterDelay(u8 Data)
{
    u8 TmpReg = 0;

    mpu6050_PortReceive(I2C_SLV4_CTRL, &TmpReg);

    Data &= 0x1F;
    TmpReg &= ~(0x1F << 0);
    TmpReg |= (Data << 0);

    mpu6050_PortTransmmit(I2C_SLV4_CTRL, TmpReg);
}

void mpu6050_I2CSlave4DI(u8 *pData)
{
    mpu6050_PortReceive(I2C_SLV4_DI, pData);
}

u8 mpu6050_StatusOfFsyncInt(void)
{
    u8 TmpReg = 0;

    mpu6050_PortReceive(I2C_MST_STATUS, &TmpReg);

    if (TmpReg & 0x80)
        return 1;
    else
        return 0;
}

u8 mpu6050_Slave4TransferDone(void)
{
    u8 TmpReg = 0;

    mpu6050_PortReceive(I2C_MST_STATUS, &TmpReg);

    if (TmpReg & 0x40)
        return 1;
    else
        return 0;
}

u8 mpu6050_SlaveLoosesArbitration(void)
{
    u8 TmpReg = 0;

    mpu6050_PortReceive(I2C_MST_STATUS, &TmpReg);

    if (TmpReg & 0x20)
        return 1;
    else
        return 0;
}

u8 mpu6050_SlaveReceivesNACK(u8 Slave)
{
    u8 TmpReg = 0;
    u8 TmpValue = 0;

    mpu6050_PortReceive(I2C_MST_STATUS, &TmpReg);

    switch (Slave) {
        case I2C_SLAVE4:
            if (TmpReg & 0x10)
                TmpValue = 1;
            else
                TmpValue = 0;

            break;

        case I2C_SLAVE3:
            if (TmpReg & 0x08)
                TmpValue = 1;
            else
                TmpValue = 0;

            break;

        case I2C_SLAVE2:
            if (TmpReg & 0x04)
                TmpValue = 1;
            else
                TmpValue = 0;

            break;

        case I2C_SLAVE1:
            if (TmpReg & 0x02)
                TmpValue = 1;
            else
                TmpValue = 0;

            break;

        case I2C_SLAVE0:
            if (TmpReg & 0x01)
                TmpValue = 1;
            else
                TmpValue = 0;

            break;

        default:
            break;
    }

    return TmpValue;
}

void mpu6050_LogicLevelForInt(u8 Data)
{
    u8 TmpReg = 0;

    mpu6050_PortReceive(INT_PIN_CFG, &TmpReg);

    Data &= 0x01;
    TmpReg &= ~(0x01 << 7);
    TmpReg |= (Data << 7);

    mpu6050_PortTransmmit(INT_PIN_CFG, TmpReg);
}

void mpu6050_EnablePullup(u8 Data)
{
    u8 TmpReg = 0;

    mpu6050_PortReceive(INT_PIN_CFG, &TmpReg);

    Data &= 0x01;
    TmpReg &= ~(0x01 << 6);
    TmpReg |= (Data << 6);

    mpu6050_PortTransmmit(INT_PIN_CFG, TmpReg);
}

void mpu6050_LatchIntPin(u8 Data)
{
    u8 TmpReg = 0;

    mpu6050_PortReceive(INT_PIN_CFG, &TmpReg);

    Data &= 0x01;
    TmpReg &= ~(0x01 << 5);
    TmpReg |= (Data << 5);

    mpu6050_PortTransmmit(INT_PIN_CFG, TmpReg);
}

void mpu6050_IntAnyrd2Clear(u8 Data)
{
    u8 TmpReg = 0;

    mpu6050_PortReceive(INT_PIN_CFG, &TmpReg);

    Data &= 0x01;
    TmpReg &= ~(0x01 << 4);
    TmpReg |= (Data << 4);

    mpu6050_PortTransmmit(INT_PIN_CFG, TmpReg);
}

void mpu6050_LogicLevelForFsync(u8 Data)
{
    u8 TmpReg = 0;

    mpu6050_PortReceive(INT_PIN_CFG, &TmpReg);

    Data &= 0x01;
    TmpReg &= ~(0x01 << 3);
    TmpReg |= (Data << 3);

    mpu6050_PortTransmmit(INT_PIN_CFG, TmpReg);
}

void mpu6050_FsyncIntMode(u8 Data)
{
    u8 TmpReg = 0;

    mpu6050_PortReceive(INT_PIN_CFG, &TmpReg);

    Data &= 0x01;
    TmpReg &= ~(0x01 << 2);
    TmpReg |= (Data << 2);

    mpu6050_PortTransmmit(INT_PIN_CFG, TmpReg);
}

void mpu6050_BypassMode(u8 Data)
{
    u8 TmpReg = 0;

    mpu6050_PortReceive(INT_PIN_CFG, &TmpReg);

    Data &= 0x01;
    TmpReg &= ~(0x01 << 1);
    TmpReg |= (Data << 1);

    mpu6050_PortTransmmit(INT_PIN_CFG, TmpReg);
}

void mpu6050_IntForWakeOnMotion(u8 Data)
{
    u8 TmpReg = 0;

    mpu6050_PortReceive(INT_ENABLE, &TmpReg);

    Data &= 0x01;
    TmpReg &= ~(0x01 << 6);
    TmpReg |= (Data << 6);

    mpu6050_PortTransmmit(INT_ENABLE, TmpReg);
}

void mpu6050_IntForFIFOOverflow(u8 Data)
{
    u8 TmpReg = 0;

    mpu6050_PortReceive(INT_ENABLE, &TmpReg);

    Data &= 0x01;
    TmpReg &= ~(0x01 << 4);
    TmpReg |= (Data << 4);

    mpu6050_PortTransmmit(INT_ENABLE, TmpReg);
}

void mpu6050_IntForFsync(u8 Data)
{
    u8 TmpReg = 0;

    mpu6050_PortReceive(INT_ENABLE, &TmpReg);

    Data &= 0x01;
    TmpReg &= ~(0x01 << 3);
    TmpReg |= (Data << 3);

    mpu6050_PortTransmmit(INT_ENABLE, TmpReg);
}

void mpu6050_IntForRawSensorDataReady(u8 Data)
{
    u8 TmpReg = 0;

    mpu6050_PortReceive(INT_ENABLE, &TmpReg);

    Data &= 0x01;
    TmpReg &= ~(0x01 << 0);
    TmpReg |= (Data << 0);

    mpu6050_PortTransmmit(INT_ENABLE, TmpReg);
}

void mpu6050_IntStatus(u8 *pData)
{
    mpu6050_PortReceive(INT_STATUS, pData);
}

void mpu6050_AccelMeasurements(u16 *pData)
{
    u8 TmpVal[6];

    mpu6050_port_multi_receive(ACCEL_XOUT_H, TmpVal, 6);

    pData[0] = (TmpVal[0] << 8) | TmpVal[1];
    pData[1] = (TmpVal[2] << 8) | TmpVal[3];
    pData[2] = (TmpVal[4] << 8) | TmpVal[5];
}

void mpu6050_TempMeasurement(u16 *pData)
{
    u8 TmpVal[2];

    mpu6050_port_multi_receive(TEMP_OUT_H, TmpVal, 2);

    pData[0] = (TmpVal[0] << 8) | TmpVal[1];
}

void mpu6050_GetTemperature(float *pData)
{
    u16 TmpVal;

    mpu6050_TempMeasurement(&TmpVal);

    pData[0] = ((TmpVal - 0) / g_Temp_Sensitivity) + 21;
}

void mpu6050_GyroMeasurements(u16 *pData)
{
    u8 TmpVal[6];

    mpu6050_port_multi_receive(GYRO_XOUT_H, TmpVal, 6);

    pData[0] = (TmpVal[0] << 8) | TmpVal[1];
    pData[1] = (TmpVal[2] << 8) | TmpVal[3];
    pData[2] = (TmpVal[4] << 8) | TmpVal[5];
}

void mpu6050_GetAccelAndGyro(u16 *pData)
{
    u8 TmpVal[14];

    mpu6050_port_multi_receive(ACCEL_XOUT_H, TmpVal, 14);

    pData[0] = (TmpVal[0] << 8) | TmpVal[1];
    pData[1] = (TmpVal[2] << 8) | TmpVal[3];
    pData[2] = (TmpVal[4] << 8) | TmpVal[5];

    pData[3] = (TmpVal[8] << 8) | TmpVal[9];
    pData[4] = (TmpVal[10] << 8) | TmpVal[11];
    pData[5] = (TmpVal[12] << 8) | TmpVal[13];
}

void mpu6050_ExternalSensorData(u8 Reg, u8 *pData)
{
    mpu6050_PortReceive(Reg, pData);
}

void mpu6050_DelaysShadowOfExtSensor(u8 Data)
{
    u8 TmpReg = 0;

    mpu6050_PortReceive(I2C_MST_DELAY_CTRL, &TmpReg);

    Data &= 0x01;
    TmpReg &= ~(0x01 << 7);
    TmpReg |= (Data << 7);

    mpu6050_PortTransmmit(I2C_MST_DELAY_CTRL, TmpReg);
}

void mpu6050_I2CSlaveDelayEnable(u8 Slave, u8 Data)
{
    u8 TmpReg = 0;

    Data &= 0x01;

    switch (Slave) {
        case I2C_SLAVE0:
            mpu6050_PortReceive(I2C_MST_DELAY_CTRL, &TmpReg);
            TmpReg &= ~(0x01 << 0);
            TmpReg |= (Data << 0);
            mpu6050_PortTransmmit(I2C_MST_DELAY_CTRL, TmpReg);
            break;

        case I2C_SLAVE1:
            mpu6050_PortReceive(I2C_MST_DELAY_CTRL, &TmpReg);
            TmpReg &= ~(0x01 << 1);
            TmpReg |= (Data << 1);
            mpu6050_PortTransmmit(I2C_MST_DELAY_CTRL, TmpReg);
            break;

        case I2C_SLAVE2:
            mpu6050_PortReceive(I2C_MST_DELAY_CTRL, &TmpReg);
            TmpReg &= ~(0x01 << 2);
            TmpReg |= (Data << 2);
            mpu6050_PortTransmmit(I2C_MST_DELAY_CTRL, TmpReg);
            break;

        case I2C_SLAVE3:
            mpu6050_PortReceive(I2C_MST_DELAY_CTRL, &TmpReg);
            TmpReg &= ~(0x01 << 3);
            TmpReg |= (Data << 3);
            mpu6050_PortTransmmit(I2C_MST_DELAY_CTRL, TmpReg);
            break;

        case I2C_SLAVE4:
            mpu6050_PortReceive(I2C_MST_DELAY_CTRL, &TmpReg);
            TmpReg &= ~(0x01 << 4);
            TmpReg |= (Data << 4);
            mpu6050_PortTransmmit(I2C_MST_DELAY_CTRL, TmpReg);
            break;

        default:
            break;
    }
}

void mpu6050_GyroSignalPathReset(u8 Data)
{
    u8 TmpReg = 0;

    mpu6050_PortReceive(SIGNAL_PATH_RESET, &TmpReg);

    Data &= 0x01;
    TmpReg &= ~(0x01 << 2);
    TmpReg |= (Data << 2);

    mpu6050_PortTransmmit(SIGNAL_PATH_RESET, TmpReg);
}

void mpu6050_AccelSignalPathReset(u8 Data)
{
    u8 TmpReg = 0;

    mpu6050_PortReceive(SIGNAL_PATH_RESET, &TmpReg);

    Data &= 0x01;
    TmpReg &= ~(0x01 << 1);
    TmpReg |= (Data << 1);

    mpu6050_PortTransmmit(SIGNAL_PATH_RESET, TmpReg);
}

void mpu6050_TempSignalPathReset(u8 Data)
{
    u8 TmpReg = 0;

    mpu6050_PortReceive(SIGNAL_PATH_RESET, &TmpReg);

    Data &= 0x01;
    TmpReg &= ~(0x01 << 0);
    TmpReg |= (Data << 0);

    mpu6050_PortTransmmit(SIGNAL_PATH_RESET, TmpReg);
}

void mpu6050_EnableWakeonMotion(u8 Data)
{
    u8 TmpReg = 0;

    mpu6050_PortReceive(MOT_DETECT_CTRL, &TmpReg);

    Data &= 0x01;
    TmpReg &= ~(0x01 << 7);
    TmpReg |= (Data << 7);

    mpu6050_PortTransmmit(MOT_DETECT_CTRL, TmpReg);
}

void mpu6050_AccelIntMode(u8 Data)
{
    u8 TmpReg = 0;

    mpu6050_PortReceive(MOT_DETECT_CTRL, &TmpReg);

    Data &= 0x01;
    TmpReg &= ~(0x01 << 6);
    TmpReg |= (Data << 6);

    mpu6050_PortTransmmit(MOT_DETECT_CTRL, TmpReg);
}

void mpu6050_EnableFIFO(u8 Data)
{
    u8 TmpReg = 0;

    mpu6050_PortReceive(USER_CTRL, &TmpReg);

    Data &= 0x01;
    TmpReg &= ~(0x01 << 6);
    TmpReg |= (Data << 6);

    mpu6050_PortTransmmit(USER_CTRL, TmpReg);
}

void mpu6050_EnableI2CMaster(u8 Data)
{
    u8 TmpReg = 0;

    mpu6050_PortReceive(USER_CTRL, &TmpReg);

    Data &= 0x01;
    TmpReg &= ~(0x01 << 5);
    TmpReg |= (Data << 5);

    mpu6050_PortTransmmit(USER_CTRL, TmpReg);
}

void mpu6050_DisableI2CSlave(u8 Data)
{
    u8 TmpReg = 0;

    mpu6050_PortReceive(USER_CTRL, &TmpReg);

    Data &= 0x01;
    TmpReg &= ~(0x01 << 4);
    TmpReg |= (Data << 4);

    mpu6050_PortTransmmit(USER_CTRL, TmpReg);
}

void mpu6050_ResetFIFO(u8 Data)
{
    u8 TmpReg = 0;

    mpu6050_PortReceive(USER_CTRL, &TmpReg);

    Data &= 0x01;
    TmpReg &= ~(0x01 << 2);
    TmpReg |= (Data << 2);

    mpu6050_PortTransmmit(USER_CTRL, TmpReg);
}

void mpu6050_ResetI2CMaster(u8 Data)
{
    u8 TmpReg = 0;

    mpu6050_PortReceive(USER_CTRL, &TmpReg);

    Data &= 0x01;
    TmpReg &= ~(0x01 << 1);
    TmpReg |= (Data << 1);

    mpu6050_PortTransmmit(USER_CTRL, TmpReg);
}

void mpu6050_ResetSignalPath(u8 Data)
{
    u8 TmpReg = 0;

    mpu6050_PortReceive(USER_CTRL, &TmpReg);

    Data &= 0x01;
    TmpReg &= ~(0x01 << 0);
    TmpReg |= (Data << 0);

    mpu6050_PortTransmmit(USER_CTRL, TmpReg);
}

void mpu6050_SoftReset(u8 Data)
{
    u8 TmpReg = 0;

    mpu6050_PortReceive(PWR_MGMT_1, &TmpReg);

    Data &= 0x01;
    TmpReg &= ~(0x01 << 7);
    TmpReg |= (Data << 7);

    mpu6050_PortTransmmit(PWR_MGMT_1, TmpReg);
}

void mpu6050_EnterSleep(u8 Data)
{
    u8 TmpReg = 0;

    mpu6050_PortReceive(PWR_MGMT_1, &TmpReg);

    Data &= 0x01;
    TmpReg &= ~(0x01 << 6);
    TmpReg |= (Data << 6);

    mpu6050_PortTransmmit(PWR_MGMT_1, TmpReg);
}

void mpu6050_CycleSample(u8 Data)
{
    u8 TmpReg = 0;

    mpu6050_PortReceive(PWR_MGMT_1, &TmpReg);

    Data &= 0x01;
    TmpReg &= ~(0x01 << 5);
    TmpReg |= (Data << 5);

    mpu6050_PortTransmmit(PWR_MGMT_1, TmpReg);
}

void mpu6050_GyroStandby(u8 Data)
{
    u8 TmpReg = 0;

    mpu6050_PortReceive(PWR_MGMT_1, &TmpReg);

    Data &= 0x01;
    TmpReg &= ~(0x01 << 4);
    TmpReg |= (Data << 4);

    mpu6050_PortTransmmit(PWR_MGMT_1, TmpReg);
}

void mpu6050_PowerDownPTAT(u8 Data)
{
    u8 TmpReg = 0;

    mpu6050_PortReceive(PWR_MGMT_1, &TmpReg);

    Data &= 0x01;
    TmpReg &= ~(0x01 << 3);
    TmpReg |= (Data << 3);

    mpu6050_PortTransmmit(PWR_MGMT_1, TmpReg);
}

void mpu6050_ClockSourceSelect(u8 Data)
{
    u8 TmpReg = 0;

    mpu6050_PortReceive(PWR_MGMT_1, &TmpReg);

    Data &= 0x07;
    TmpReg &= ~(0x07 << 0);
    TmpReg |= (Data << 0);

    mpu6050_PortTransmmit(PWR_MGMT_1, TmpReg);
}

void mpu6050_AccelDisabled(u8 Axis, u8 Data)
{
    u8 TmpReg = 0;

    mpu6050_PortReceive(PWR_MGMT_2, &TmpReg);
    Data &= 0x01;

    switch (Axis) {
        case X_AXIS:
            TmpReg &= ~(0x01 << 5);
            TmpReg |= (Data << 5);
            break;

        case Y_AXIS:
            TmpReg &= ~(0x01 << 4);
            TmpReg |= (Data << 4);
            break;

        case Z_AXIS:
            TmpReg &= ~(0x01 << 3);
            TmpReg |= (Data << 3);
            break;

        default:
            break;
    }

    mpu6050_PortTransmmit(PWR_MGMT_2, TmpReg);
}

void mpu6050_GyroDisabled(u8 Axis, u8 Data)
{
    u8 TmpReg = 0;

    mpu6050_PortReceive(PWR_MGMT_2, &TmpReg);
    Data &= 0x01;

    switch (Axis) {
        case X_AXIS:
            TmpReg &= ~(0x01 << 2);
            TmpReg |= (Data << 2);
            break;

        case Y_AXIS:
            TmpReg &= ~(0x01 << 1);
            TmpReg |= (Data << 1);
            break;

        case Z_AXIS:
            TmpReg &= ~(0x01 << 0);
            TmpReg |= (Data << 0);
            break;

        default:
            break;
    }

    mpu6050_PortTransmmit(PWR_MGMT_2, TmpReg);
}

void mpu6050_FIFOCount(u16 *pData)
{
    u8 TmpReg[2];

    mpu6050_port_multi_receive(FIFO_COUNTH, TmpReg, 2);

    TmpReg[0] &= 0x1F;
    pData[0] = (TmpReg[0] << 8) | TmpReg[1];
}

void mpu6050_FIFORead(u8 *pData)
{
    mpu6050_PortReceive(FIFO_R_W, pData);
}

void mpu6050_FIFOWrite(u8 Data)
{
    mpu6050_PortTransmmit(FIFO_R_W, Data);
}

void mpu6050_WhoAmI(u8 *pData)
{
    mpu6050_PortReceive(WHO_AM_I, pData);
}

void mpu9250_ReadAccelOffsetRegisters(u8 Axis, u16 *pData)
{
    u8 SubData1 = 0;
    u8 SubData2 = 0;

    switch (Axis) {
        case X_AXIS:
            mpu6050_PortReceive(XA_OFFSET_H, &SubData1);
            mpu6050_PortReceive(XA_OFFSET_L, &SubData2);
            break;

        case Y_AXIS:
            mpu6050_PortReceive(YA_OFFSET_H, &SubData1);
            mpu6050_PortReceive(YA_OFFSET_L, &SubData2);
            break;

        case Z_AXIS:
            mpu6050_PortReceive(ZA_OFFSET_H, &SubData1);
            mpu6050_PortReceive(ZA_OFFSET_L, &SubData2);
            break;

        default:
            break;
    }

    *pData = (SubData1 << 7) | (SubData2 >> 1);
}

void mpu9250_WriteAccelOffsetRegisters(u8 Axis, u16 Data)
{
    u8 SubData1 = 0;
    u8 SubData2 = 0;

    SubData1 = Data >> 7;
    SubData2 = (Data & 0xFE) >> 1;

    switch (Axis) {
        case X_AXIS:
            mpu6050_PortTransmmit(XA_OFFSET_H, SubData1);
            mpu6050_PortTransmmit(XA_OFFSET_L, SubData2);
            break;

        case Y_AXIS:
            mpu6050_PortTransmmit(YA_OFFSET_H, SubData1);
            mpu6050_PortTransmmit(YA_OFFSET_L, SubData2);
            break;

        case Z_AXIS:
            mpu6050_PortTransmmit(ZA_OFFSET_H, SubData1);
            mpu6050_PortTransmmit(ZA_OFFSET_L, SubData2);
            break;

        default:
            break;
    }
}

#ifdef DESIGN_VERIFICATION_MPU6050
static u8 tx_buffer[256];
static u8 rx_buffer[256];

void mpu6050_Test(void)
{
    u32 TmpRngdata = 0;
    u16 BufferLength = 0;
    u32 TestAddr = 0;

    Random_Get8bit(&hrng, &TmpRngdata);
    BufferLength = TmpRngdata & 0xFF;
    kinetis_print_trace(KERN_DEBUG, "BufferLength = %d.", BufferLength);

    if (tx_buffer == NULL || rx_buffer == NULL) {
        kinetis_print_trace(KERN_DEBUG, "Failed to allocate memory !");
        return;
    }

    memset(tx_buffer, 0, BufferLength);
    memset(rx_buffer, 0, BufferLength);

    Random_Get8bit(&hrng, &TmpRngdata);
    TestAddr = TmpRngdata & 0xFF;
    kinetis_print_trace(KERN_DEBUG, "TestAddr = 0x%02X.", TestAddr);

    for (u16 i = 0; i < BufferLength; i += 4) {
        Random_Get8bit(&hrng, &TmpRngdata);
        tx_buffer[i + 3] = (TmpRngdata & 0xFF000000) >> 24;;
        tx_buffer[i + 2] = (TmpRngdata & 0x00FF0000) >> 16;
        tx_buffer[i + 1] = (TmpRngdata & 0x0000FF00) >> 8;
        tx_buffer[i + 0] = (TmpRngdata & 0x000000FF);
    }

    mpu6050_WriteData(TestAddr, tx_buffer, BufferLength);
    mpu6050_ReadData(TestAddr, rx_buffer, BufferLength);

    for (u16 i = 0; i < BufferLength; i++) {
        if (tx_buffer[i] != rx_buffer[i]) {
            kinetis_print_trace(KERN_DEBUG, "tx_buffer[%d] = 0x%02X, rx_buffer[%d] = 0x%02X",
                i, tx_buffer[i],
                i, rx_buffer[i]);
            kinetis_print_trace(KERN_DEBUG, "Data writes and reads do not match, TEST FAILED !");
            return ;
        }
    }

    kinetis_print_trace(KERN_DEBUG, "mpu6050 Read and write TEST PASSED !");
}

#endif


