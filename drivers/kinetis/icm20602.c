#include "icm20602/icm20602.h"

/* The following program is modified by the user according to the hardware device, otherwise the driver cannot run. */

/**
  * @step 1:  Modify the corresponding function according to the modified area and the corresponding function name.
  * @step 2:  Modify four areas: GPIO_PORT/GPIO_PIN/Led_TypeDef/LEDn.
  * @step 3:  .
  * @step 4:  .
  * @step 5:
  */

#include "iic_soft/iic_soft.h"

#define DEBUG
#include "kinetis/idebug.h"

#define icm20602_printf                  p_dbg

#define ICM20602_ADDR                    0x00

void icm20602_Delayus(u32 ticks)
{
    udelay(ticks);
}

void icm20602_Delayms(u32 ticks)
{
    mdelay(ticks);
}

void icm20602_PortTransmmit(u8 Addr, u8 Data)
{
    IIC_Soft_WriteSingleByteWithAddr(ICM20602_ADDR, Addr, Data);
}

void icm20602_PortReceive(u8 Addr, u8 *pData)
{
    IIC_Soft_ReadSingleByteWithAddr(ICM20602_ADDR, Addr, pData);
}

void icm20602_port_multi_transmmit(u8 Addr, u8 *pData, u32 Length)
{
    IIC_Soft_WriteMultiByteWithAddr(ICM20602_ADDR, Addr, pData, Length);
}

void icm20602_port_multi_receive(u8 Addr, u8 *pData, u32 Length)
{
    IIC_Soft_ReadMultiByteWithAddr(ICM20602_ADDR, Addr, pData, Length);
}

/* The above procedure is modified by the user according to the hardware device, otherwise the driver cannot run. */

#define X_AXIS                          0x00
#define Y_AXIS                          0x01
#define Z_AXIS                          0x02
#define XG_OFFS_TC_H                    0x00
#define XG_OFFS_TC_L                    0x00
#define YG_OFFS_TC_H                    0x00
#define YG_OFFS_TC_L                    0x00
#define ZG_OFFS_TC_H                    0x00
#define ZG_OFFS_TC_L                    0x00
#define SELF_TEST_X_ACCEL               0x00
#define SELF_TEST_Y_ACCEL               0x00
#define SELF_TEST_Z_ACCEL               0x00
#define XG_OFFS_USRH                    0x00
#define XG_OFFS_USRL                    0x00
#define YG_OFFS_USRH                    0x00
#define YG_OFFS_USRL                    0x00
#define ZG_OFFS_USRH                    0x00
#define ZG_OFFS_USRL                    0x00
#define SMPLRT_DIV                      0x00
#define CONFIG                          0x00
#define GYRO_CONFIG                     0x00
#define ACCEL_CONFIG                    0x00
#define ACCEL_CONFIG2                   0x00
#define LP_MODE_CFG                     0x00
#define ACCEL_WOM_X_THR                 0x00
#define ACCEL_WOM_Y_THR                 0x00
#define ACCEL_WOM_Z_THR                 0x00
#define FIFO_EN                         0x00
#define FSYNC_INT                       0x00
#define INT_PIN_CFG                     0x00
#define INT_ENABLE                      0x00
#define FIFO_WM_INT_STATUS              0x00
#define INT_STATUS                      0x00
#define ACCEL_XOUT_H                    0x00
#define ACCEL_XOUT_L                    0x00
#define ACCEL_YOUT_H                    0x00
#define ACCEL_YOUT_L                    0x00
#define ACCEL_ZOUT_H                    0x00
#define ACCEL_ZOUT_L                    0x00
#define TEMP_OUT_H                      0x00
#define TEMP_OUT_L                      0x00
#define GYRO_XOUT_H                     0x00
#define GYRO_XOUT_L                     0x00
#define GYRO_YOUT_H                     0x00
#define GYRO_YOUT_L                     0x00
#define GYRO_ZOUT_H                     0x00
#define GYRO_ZOUT_L                     0x00
#define SELF_TEST_X_GYRO                0x00
#define SELF_TEST_Y_GYRO                0x00
#define SELF_TEST_Z_GYRO                0x00
#define FIFO_WM_TH1                     0x00
#define FIFO_WM_TH2                     0x00
#define SIGNAL_PATH_RESET               0x00
#define ACCEL_INTEL_CTRL                0x00
#define USER_CTRL                       0x00
#define PWR_MGMT_1                      0x00
#define PWR_MGMT_2                      0x00
#define I2C_IF                          0x00
#define FIFO_COUNTH                     0x00
#define FIFO_COUNTL                     0x00
#define FIFO_R_W                        0x00
#define WHO_AM_I                        0x00
#define XA_OFFSET_H                     0x00
#define XA_OFFSET_L                     0x00
#define YA_OFFSET_H                     0x00
#define YA_OFFSET_L                     0x00
#define ZA_OFFSET_H                     0x00
#define ZA_OFFSET_L                     0x00

void icm20602_ReadGyroLPOffsetRegisters(u8 Axis, u8 *pData)
{

    switch (Axis) {
        case X_AXIS:
            icm20602_PortReceive(XG_OFFS_TC_H, pData);
            break;

        case Y_AXIS:
            icm20602_PortReceive(YG_OFFS_TC_H, pData);
            break;

        case Z_AXIS:
            icm20602_PortReceive(ZG_OFFS_TC_H, pData);
            break;

        default:
            break;
    }

    pData[0] >>= 2;
}

void icm20602_WriteGyroLPOffsetRegisters(u8 Axis, u16 Data)
{
    u8 TmpReg = 0;

    Data &= 0x3F;

    switch (Axis) {
        case X_AXIS:
            icm20602_PortReceive(XG_OFFS_TC_H, &TmpReg);
            TmpReg &= ~(0x3F << 2);
            TmpReg |= (Data << 2);
            icm20602_PortTransmmit(XG_OFFS_TC_H, TmpReg);
            break;

        case Y_AXIS:
            icm20602_PortReceive(YG_OFFS_TC_H, &TmpReg);
            TmpReg &= ~(0x3F << 2);
            TmpReg |= (Data << 2);
            icm20602_PortTransmmit(YG_OFFS_TC_H, TmpReg);
            break;

        case Z_AXIS:
            icm20602_PortReceive(ZG_OFFS_TC_H, &TmpReg);
            TmpReg &= ~(0x3F << 2);
            TmpReg |= (Data << 2);
            icm20602_PortTransmmit(ZG_OFFS_TC_H, TmpReg);
            break;

        default:
            break;
    }
}

void icm20602_ReadGyroTCOffsetRegisters(u8 Axis, u16 *pData)
{
    u8 SubData1 = 0;
    u8 SubData2 = 0;

    switch (Axis) {
        case X_AXIS:
            icm20602_PortReceive(XG_OFFS_TC_H, &SubData1);
            icm20602_PortReceive(XG_OFFS_TC_L, &SubData2);
            break;

        case Y_AXIS:
            icm20602_PortReceive(YG_OFFS_TC_H, &SubData1);
            icm20602_PortReceive(YG_OFFS_TC_L, &SubData2);
            break;

        case Z_AXIS:
            icm20602_PortReceive(ZG_OFFS_TC_H, &SubData1);
            icm20602_PortReceive(ZG_OFFS_TC_L, &SubData2);
            break;

        default:
            break;
    }

    *pData = ((SubData1 & 0x03) << 8) | SubData2;
}

void icm20602_WriteGyroTCOffsetRegisters(u8 Axis, u16 Data)
{
    u8 SubData1 = 0;
    u8 SubData2 = 0;

    Data &= 0x3FF;

    switch (Axis) {
        case X_AXIS:
            icm20602_PortReceive(XG_OFFS_TC_H, &SubData1);
            icm20602_PortReceive(XG_OFFS_TC_L, &SubData2);
            SubData1 &= ~0x03;
            SubData1 |= Data >> 8;
            SubData2 = Data & 0xFF;
            icm20602_PortTransmmit(XG_OFFS_TC_H, SubData1);
            icm20602_PortTransmmit(XG_OFFS_TC_L, SubData2);
            break;

        case Y_AXIS:
            icm20602_PortReceive(YG_OFFS_TC_H, &SubData1);
            icm20602_PortReceive(YG_OFFS_TC_L, &SubData2);
            SubData1 &= ~0x03;
            SubData1 |= Data >> 8;
            SubData2 = Data & 0xFF;
            icm20602_PortTransmmit(YG_OFFS_TC_H, SubData1);
            icm20602_PortTransmmit(YG_OFFS_TC_L, SubData2);
            break;

        case Z_AXIS:
            icm20602_PortReceive(ZG_OFFS_TC_H, &SubData1);
            icm20602_PortReceive(ZG_OFFS_TC_L, &SubData2);
            SubData1 &= ~0x03;
            SubData1 |= Data >> 8;
            SubData2 = Data & 0xFF;
            icm20602_PortTransmmit(ZG_OFFS_TC_H, SubData1);
            icm20602_PortTransmmit(ZG_OFFS_TC_L, SubData2);
            break;

        default:
            break;
    }
}

void icm20602_ReadAccelSelfTestRegisters(u8 Axis, u8 *pData)
{
    switch (Axis) {
        case X_AXIS:
            icm20602_PortReceive(SELF_TEST_X_ACCEL, pData);
            break;

        case Y_AXIS:
            icm20602_PortReceive(SELF_TEST_Y_ACCEL, pData);
            break;

        case Z_AXIS:
            icm20602_PortReceive(SELF_TEST_Z_ACCEL, pData);
            break;

        default:
            break;
    }
}

void icm20602_WriteAccelSelfTestRegisters(u8 Axis, u8 Data)
{
    switch (Axis) {
        case X_AXIS:
            icm20602_PortTransmmit(SELF_TEST_X_ACCEL, Data);
            break;

        case Y_AXIS:
            icm20602_PortTransmmit(SELF_TEST_Y_ACCEL, Data);
            break;

        case Z_AXIS:
            icm20602_PortTransmmit(SELF_TEST_Z_ACCEL, Data);
            break;

        default:
            break;
    }
}

void icm20602_ReadGyroOffsetRegisters(u8 Axis, u16 *pData)
{
    u8 SubData1 = 0;
    u8 SubData2 = 0;

    switch (Axis) {
        case X_AXIS:
            icm20602_PortReceive(XG_OFFS_USRH, &SubData1);
            icm20602_PortReceive(XG_OFFS_USRL, &SubData2);
            break;

        case Y_AXIS:
            icm20602_PortReceive(YG_OFFS_USRH, &SubData1);
            icm20602_PortReceive(YG_OFFS_USRL, &SubData2);
            break;

        case Z_AXIS:
            icm20602_PortReceive(ZG_OFFS_USRH, &SubData1);
            icm20602_PortReceive(ZG_OFFS_USRL, &SubData2);
            break;

        default:
            break;
    }

    *pData = (SubData1 << 8) | SubData2;
}

void icm20602_WriteGyroOffsetRegisters(u8 Axis, u16 Data)
{
    u8 SubData1 = 0;
    u8 SubData2 = 0;

    SubData1 = Data >> 8;
    SubData2 = Data & 0xFF;

    switch (Axis) {
        case X_AXIS:
            icm20602_PortTransmmit(XG_OFFS_USRH, SubData1);
            icm20602_PortTransmmit(XG_OFFS_USRL, SubData2);
            break;

        case Y_AXIS:
            icm20602_PortTransmmit(YG_OFFS_USRH, SubData1);
            icm20602_PortTransmmit(YG_OFFS_USRL, SubData2);
            break;

        case Z_AXIS:
            icm20602_PortTransmmit(ZG_OFFS_USRH, SubData1);
            icm20602_PortTransmmit(ZG_OFFS_USRL, SubData2);
            break;

        default:
            break;
    }
}

void icm20602_SampleRateDivider(u8 Data)
{
    icm20602_PortTransmmit(SMPLRT_DIV, Data);
}

void icm20602_FIFOMode(u8 Data)
{
    u8 TmpReg = 0;

    icm20602_PortReceive(CONFIG, &TmpReg);

    Data &= 0x01;
    TmpReg &= ~(0x01 << 6);
    TmpReg |= (Data << 6);

    icm20602_PortTransmmit(CONFIG, TmpReg);
}

void icm20602_FsyncSet(u8 Data)
{
    u8 TmpReg = 0;

    icm20602_PortReceive(CONFIG, &TmpReg);

    Data &= 0x07;
    TmpReg &= ~(0x07 << 3);
    TmpReg |= (Data << 3);

    icm20602_PortTransmmit(CONFIG, TmpReg);
}

void icm20602_ConfigDLPF(u8 Data)
{
    u8 TmpReg = 0;

    icm20602_PortReceive(CONFIG, &TmpReg);

    Data &= 0x07;
    TmpReg &= ~(0x07 << 0);
    TmpReg |= (Data << 0);

    icm20602_PortTransmmit(CONFIG, TmpReg);
}

void icm20602_GyroSelfTest(u8 Axis, u8 Data)
{
    u8 TmpReg = 0;

    icm20602_PortReceive(GYRO_CONFIG, &TmpReg);
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

    icm20602_PortTransmmit(GYRO_CONFIG, TmpReg);
}

void icm20602_GyroFullScaleSelect(u8 Data)
{
    u8 TmpReg = 0;

    icm20602_PortReceive(GYRO_CONFIG, &TmpReg);

    Data &= 0x03;
    TmpReg &= ~(0x03 << 3);
    TmpReg |= (Data << 3);

    icm20602_PortTransmmit(GYRO_CONFIG, TmpReg);
}

void icm20602_Fchoice_b(u8 Data)
{
    u8 TmpReg = 0;

    icm20602_PortReceive(GYRO_CONFIG, &TmpReg);

    Data &= 0x03;
    TmpReg &= ~(0x03 << 0);
    TmpReg |= (Data << 0);

    icm20602_PortTransmmit(GYRO_CONFIG, TmpReg);
}

void icm20602_AccelSelfTest(u8 Axis, u8 Data)
{
    u8 TmpReg = 0;

    icm20602_PortReceive(ACCEL_CONFIG, &TmpReg);
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

    icm20602_PortTransmmit(ACCEL_CONFIG, TmpReg);
}

void icm20602_AccelFullScaleSelect(u8 Data)
{
    u8 TmpReg = 0;

    icm20602_PortReceive(ACCEL_CONFIG, &TmpReg);

    Data &= 0x03;
    TmpReg &= ~(0x03 << 3);
    TmpReg |= (Data << 3);

    icm20602_PortTransmmit(ACCEL_CONFIG, TmpReg);
}

void icm20602_AveragingFilterForLPAccel(u8 Data)
{
    u8 TmpReg = 0;

    icm20602_PortReceive(ACCEL_CONFIG2, &TmpReg);

    Data &= 0x03;
    TmpReg &= ~(0x03 << 4);
    TmpReg |= (Data << 4);

    icm20602_PortTransmmit(ACCEL_CONFIG2, TmpReg);
}

void icm20602_Accel_Fchoice_b(u8 Data)
{
    u8 TmpReg = 0;

    icm20602_PortReceive(ACCEL_CONFIG2, &TmpReg);

    Data &= 0x01;
    TmpReg &= ~(0x01 << 3);
    TmpReg |= (Data << 3);

    icm20602_PortTransmmit(ACCEL_CONFIG2, TmpReg);
}

void icm20602_ConfigAccelDLPF(u8 Data)
{
    u8 TmpReg = 0;

    icm20602_PortReceive(ACCEL_CONFIG2, &TmpReg);

    Data &= 0x07;
    TmpReg &= ~(0x07 << 0);
    TmpReg |= (Data << 0);

    icm20602_PortTransmmit(ACCEL_CONFIG2, TmpReg);
}

void icm20602_ConfigLPGyroMode(u8 Data)
{
    u8 TmpReg = 0;

    icm20602_PortReceive(LP_MODE_CFG, &TmpReg);

    Data &= 0x01;
    TmpReg &= ~(0x01 << 7);
    TmpReg |= (Data << 7);

    icm20602_PortTransmmit(LP_MODE_CFG, TmpReg);
}

void icm20602_AveragingFilterForLPGyro(u8 Data)
{
    u8 TmpReg = 0;

    icm20602_PortReceive(LP_MODE_CFG, &TmpReg);

    Data &= 0x03;
    TmpReg &= ~(0x03 << 4);
    TmpReg |= (Data << 4);

    icm20602_PortTransmmit(LP_MODE_CFG, TmpReg);
}

void icm20602_WakeonMotionThreshold(u8 Axis, u8 Data)
{
    switch (Axis) {
        case X_AXIS:
            icm20602_PortTransmmit(ACCEL_WOM_X_THR, Data);
            break;

        case Y_AXIS:
            icm20602_PortTransmmit(ACCEL_WOM_Y_THR, Data);
            break;

        case Z_AXIS:
            icm20602_PortTransmmit(ACCEL_WOM_Z_THR, Data);
            break;

        default:
            break;
    }
}

void icm20602_FIFOEnableWithGyro(u8 Data)
{
    u8 TmpReg = 0;

    icm20602_PortReceive(FIFO_EN, &TmpReg);

    Data &= 0x01;
    TmpReg &= ~(0x01 << 4);
    TmpReg |= (Data << 4);

    icm20602_PortTransmmit(FIFO_EN, TmpReg);
}

void icm20602_FIFOEnableWithAccel(u8 Data)
{
    u8 TmpReg = 0;

    icm20602_PortReceive(FIFO_EN, &TmpReg);

    Data &= 0x01;
    TmpReg &= ~(0x01 << 3);
    TmpReg |= (Data << 3);

    icm20602_PortTransmmit(FIFO_EN, TmpReg);
}

u8 icm20602_StatusOfFsyncInt(void)
{
    u8 TmpReg = 0;

    icm20602_PortReceive(FSYNC_INT, &TmpReg);

    if (TmpReg & 0x80)
        return 1;
    else
        return 0;
}

void icm20602_LogicLevelForInt(u8 Data)
{
    u8 TmpReg = 0;

    icm20602_PortReceive(INT_PIN_CFG, &TmpReg);

    Data &= 0x01;
    TmpReg &= ~(0x01 << 7);
    TmpReg |= (Data << 7);

    icm20602_PortTransmmit(INT_PIN_CFG, TmpReg);
}

void icm20602_EnablePullup(u8 Data)
{
    u8 TmpReg = 0;

    icm20602_PortReceive(INT_PIN_CFG, &TmpReg);

    Data &= 0x01;
    TmpReg &= ~(0x01 << 6);
    TmpReg |= (Data << 6);

    icm20602_PortTransmmit(INT_PIN_CFG, TmpReg);
}

void icm20602_LatchIntPin(u8 Data)
{
    u8 TmpReg = 0;

    icm20602_PortReceive(INT_PIN_CFG, &TmpReg);

    Data &= 0x01;
    TmpReg &= ~(0x01 << 5);
    TmpReg |= (Data << 5);

    icm20602_PortTransmmit(INT_PIN_CFG, TmpReg);
}

void icm20602_IntAnyrd2Clear(u8 Data)
{
    u8 TmpReg = 0;

    icm20602_PortReceive(INT_PIN_CFG, &TmpReg);

    Data &= 0x01;
    TmpReg &= ~(0x01 << 4);
    TmpReg |= (Data << 4);

    icm20602_PortTransmmit(INT_PIN_CFG, TmpReg);
}

void icm20602_LogicLevelForFsync(u8 Data)
{
    u8 TmpReg = 0;

    icm20602_PortReceive(INT_PIN_CFG, &TmpReg);

    Data &= 0x01;
    TmpReg &= ~(0x01 << 3);
    TmpReg |= (Data << 3);

    icm20602_PortTransmmit(INT_PIN_CFG, TmpReg);
}

void icm20602_FsyncIntMode(u8 Data)
{
    u8 TmpReg = 0;

    icm20602_PortReceive(INT_PIN_CFG, &TmpReg);

    Data &= 0x01;
    TmpReg &= ~(0x01 << 2);
    TmpReg |= (Data << 2);

    icm20602_PortTransmmit(INT_PIN_CFG, TmpReg);
}

u8 icm20602_StatusOfWatermarkInt(void)
{
    u8 TmpReg = 0;

    icm20602_PortReceive(FIFO_WM_INT_STATUS, &TmpReg);

    if (TmpReg & 0x40)
        return 1;
    else
        return 0;
}

u8 icm20602_IntStatusForWakeOnMotion(void)
{
    u8 TmpReg = 0;

    icm20602_PortReceive(INT_STATUS, &TmpReg);

    if (TmpReg & 0x40)
        return 1;
    else
        return 0;
}

u8 icm20602_IntStatusForFIFOOverflow(void)
{
    u8 TmpReg = 0;

    icm20602_PortReceive(INT_STATUS, &TmpReg);

    if (TmpReg & 0x10)
        return 1;
    else
        return 0;
}

u8 icm20602_IntStatusForFsync(void)
{
    u8 TmpReg = 0;

    icm20602_PortReceive(INT_STATUS, &TmpReg);

    if (TmpReg & 0x08)
        return 1;
    else
        return 0;
}

void icm20602_IntStatus(u8 *pData)
{
    icm20602_PortReceive(INT_STATUS, pData);
}

void icm20602_AccelMeasurements(u16 *pData)
{
    u8 TmpVal[6];

    icm20602_port_multi_receive(ACCEL_XOUT_H, TmpVal, 6);

    pData[0] = (TmpVal[0] << 8) | TmpVal[1];
    pData[1] = (TmpVal[2] << 8) | TmpVal[3];
    pData[2] = (TmpVal[4] << 8) | TmpVal[5];
}

void icm20602_TempMeasurement(u16 *pData)
{
    u8 TmpVal[2];

    icm20602_port_multi_receive(TEMP_OUT_H, TmpVal, 2);

    pData[0] = (TmpVal[0] << 8) | TmpVal[1];
}

void icm20602_GetTemperature(float *pData)
{
    u16 TmpVal;

    icm20602_TempMeasurement(&TmpVal);

    pData[0] = ((TmpVal - 0) / 326.8) + 25;
}

void icm20602_GyroMeasurements(u16 *pData)
{
    u8 TmpVal[6];

    icm20602_port_multi_receive(GYRO_XOUT_H, TmpVal, 6);

    pData[0] = (TmpVal[0] << 8) | TmpVal[1];
    pData[1] = (TmpVal[2] << 8) | TmpVal[3];
    pData[2] = (TmpVal[4] << 8) | TmpVal[5];
}

void icm20602_GetAccelAndGyro(u16 *pData)
{
    u8 TmpVal[14];

    icm20602_port_multi_receive(ACCEL_XOUT_H, TmpVal, 14);

    pData[0] = (TmpVal[0] << 8) | TmpVal[1];
    pData[1] = (TmpVal[2] << 8) | TmpVal[3];
    pData[2] = (TmpVal[4] << 8) | TmpVal[5];

    pData[3] = (TmpVal[8] << 8) | TmpVal[9];
    pData[4] = (TmpVal[10] << 8) | TmpVal[11];
    pData[5] = (TmpVal[12] << 8) | TmpVal[13];
}

void icm20602_ReadGyroSelfTestRegisters(u8 Axis, u8 *pData)
{
    switch (Axis) {
        case X_AXIS:
            icm20602_PortReceive(SELF_TEST_X_GYRO, pData);
            break;

        case Y_AXIS:
            icm20602_PortReceive(SELF_TEST_Y_GYRO, pData);
            break;

        case Z_AXIS:
            icm20602_PortReceive(SELF_TEST_Z_GYRO, pData);
            break;

        default:
            break;
    }
}

void icm20602_WriteGyroSelfTestRegisters(u8 Axis, u8 Data)
{
    switch (Axis) {
        case X_AXIS:
            icm20602_PortTransmmit(SELF_TEST_X_GYRO, Data);
            break;

        case Y_AXIS:
            icm20602_PortTransmmit(SELF_TEST_Y_GYRO, Data);
            break;

        case Z_AXIS:
            icm20602_PortTransmmit(SELF_TEST_Z_GYRO, Data);
            break;

        default:
            break;
    }
}
void icm20602_ReadFIFOWatermarkThreshold(u16 *pData)
{
    u8 TmpReg[2] = {0, 0};

    icm20602_port_multi_receive(FIFO_WM_TH1, TmpReg, 2);

    *pData = ((TmpReg[0] & 0x03) << 8) | TmpReg[1];
}

void icm20602_WriteFIFOWatermarkThreshold(u16 Data)
{
    u8 TmpReg[2] = {0, 0};

    Data &= 0x3FF;

    icm20602_port_multi_receive(FIFO_WM_TH2, TmpReg, 2);
    TmpReg[0] &= ~0x03;
    TmpReg[0] |= Data >> 8;
    TmpReg[1] = Data & 0xFF;
    icm20602_port_multi_transmmit(FIFO_WM_TH2, TmpReg, 2);
}

void icm20602_AccelSignalPathReset(u8 Data)
{
    u8 TmpReg = 0;

    icm20602_PortReceive(SIGNAL_PATH_RESET, &TmpReg);

    Data &= 0x01;
    TmpReg &= ~(0x01 << 1);
    TmpReg |= (Data << 1);

    icm20602_PortTransmmit(SIGNAL_PATH_RESET, TmpReg);
}

void icm20602_TempSignalPathReset(u8 Data)
{
    u8 TmpReg = 0;

    icm20602_PortReceive(SIGNAL_PATH_RESET, &TmpReg);

    Data &= 0x01;
    TmpReg &= ~(0x01 << 0);
    TmpReg |= (Data << 0);

    icm20602_PortTransmmit(SIGNAL_PATH_RESET, TmpReg);
}

void icm20602_EnableWakeonMotion(u8 Data)
{
    u8 TmpReg = 0;

    icm20602_PortReceive(ACCEL_INTEL_CTRL, &TmpReg);

    Data &= 0x01;
    TmpReg &= ~(0x01 << 7);
    TmpReg |= (Data << 7);

    icm20602_PortTransmmit(ACCEL_INTEL_CTRL, TmpReg);
}

void icm20602_IntelligenceMode(u8 Data)
{
    u8 TmpReg = 0;

    icm20602_PortReceive(ACCEL_INTEL_CTRL, &TmpReg);

    Data &= 0x01;
    TmpReg &= ~(0x01 << 6);
    TmpReg |= (Data << 6);

    icm20602_PortTransmmit(ACCEL_INTEL_CTRL, TmpReg);
}

void icm20602_OutputLimit(u8 Data)
{
    u8 TmpReg = 0;

    icm20602_PortReceive(ACCEL_INTEL_CTRL, &TmpReg);

    Data &= 0x01;
    TmpReg &= ~(0x01 << 1);
    TmpReg |= (Data << 1);

    icm20602_PortTransmmit(ACCEL_INTEL_CTRL, TmpReg);
}

void icm20602_WakeonMotionIntMode(u8 Data)
{
    u8 TmpReg = 0;

    icm20602_PortReceive(ACCEL_INTEL_CTRL, &TmpReg);

    Data &= 0x01;
    TmpReg &= ~(0x01 << 0);
    TmpReg |= (Data << 0);

    icm20602_PortTransmmit(ACCEL_INTEL_CTRL, TmpReg);
}

void icm20602_EnableFIFO(u8 Data)
{
    u8 TmpReg = 0;

    icm20602_PortReceive(USER_CTRL, &TmpReg);

    Data &= 0x01;
    TmpReg &= ~(0x01 << 6);
    TmpReg |= (Data << 6);

    icm20602_PortTransmmit(USER_CTRL, TmpReg);
}

void icm20602_ResetFIFO(u8 Data)
{
    u8 TmpReg = 0;

    icm20602_PortReceive(USER_CTRL, &TmpReg);

    Data &= 0x01;
    TmpReg &= ~(0x01 << 2);
    TmpReg |= (Data << 2);

    icm20602_PortTransmmit(USER_CTRL, TmpReg);
}

void icm20602_ResetSignalPath(u8 Data)
{
    u8 TmpReg = 0;

    icm20602_PortReceive(USER_CTRL, &TmpReg);

    Data &= 0x01;
    TmpReg &= ~(0x01 << 0);
    TmpReg |= (Data << 0);

    icm20602_PortTransmmit(USER_CTRL, TmpReg);
}

void icm20602_SoftReset(u8 Data)
{
    u8 TmpReg = 0;

    icm20602_PortReceive(PWR_MGMT_1, &TmpReg);

    Data &= 0x01;
    TmpReg &= ~(0x01 << 7);
    TmpReg |= (Data << 7);

    icm20602_PortTransmmit(PWR_MGMT_1, TmpReg);
}

void icm20602_EnterSleep(u8 Data)
{
    u8 TmpReg = 0;

    icm20602_PortReceive(PWR_MGMT_1, &TmpReg);

    Data &= 0x01;
    TmpReg &= ~(0x01 << 6);
    TmpReg |= (Data << 6);

    icm20602_PortTransmmit(PWR_MGMT_1, TmpReg);
}

void icm20602_CycleSample(u8 Data)
{
    u8 TmpReg = 0;

    icm20602_PortReceive(PWR_MGMT_1, &TmpReg);

    Data &= 0x01;
    TmpReg &= ~(0x01 << 5);
    TmpReg |= (Data << 5);

    icm20602_PortTransmmit(PWR_MGMT_1, TmpReg);
}

void icm20602_GyroStandby(u8 Data)
{
    u8 TmpReg = 0;

    icm20602_PortReceive(PWR_MGMT_1, &TmpReg);

    Data &= 0x01;
    TmpReg &= ~(0x01 << 4);
    TmpReg |= (Data << 4);

    icm20602_PortTransmmit(PWR_MGMT_1, TmpReg);
}

void icm20602_DisableTemp(u8 Data)
{
    u8 TmpReg = 0;

    icm20602_PortReceive(PWR_MGMT_1, &TmpReg);

    Data &= 0x01;
    TmpReg &= ~(0x01 << 3);
    TmpReg |= (Data << 3);

    icm20602_PortTransmmit(PWR_MGMT_1, TmpReg);
}

void icm20602_ClockSourceSelect(u8 Data)
{
    u8 TmpReg = 0;

    icm20602_PortReceive(PWR_MGMT_1, &TmpReg);

    Data &= 0x07;
    TmpReg &= ~(0x07 << 0);
    TmpReg |= (Data << 0);

    icm20602_PortTransmmit(PWR_MGMT_1, TmpReg);
}

void icm20602_AccelDisabled(u8 Axis, u8 Data)
{
    u8 TmpReg = 0;

    icm20602_PortReceive(PWR_MGMT_2, &TmpReg);
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

    icm20602_PortTransmmit(PWR_MGMT_2, TmpReg);
}

void icm20602_GyroDisabled(u8 Axis, u8 Data)
{
    u8 TmpReg = 0;

    icm20602_PortReceive(PWR_MGMT_2, &TmpReg);
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

    icm20602_PortTransmmit(PWR_MGMT_2, TmpReg);
}

void icm20602_EnableSPI(u8 Data)
{
    u8 TmpReg = 0;

    icm20602_PortReceive(I2C_IF, &TmpReg);

    Data &= 0x01;
    TmpReg &= ~(0x01 << 6);
    TmpReg |= (Data << 6);

    icm20602_PortTransmmit(I2C_IF, TmpReg);
}

void icm20602_FIFOCount(u16 *pData)
{
    u8 TmpReg[2];

    icm20602_port_multi_receive(FIFO_COUNTH, TmpReg, 2);

    pData[0] = (TmpReg[0] << 8) | TmpReg[1];
}

void icm20602_FIFORead(u8 *pData)
{
    icm20602_PortReceive(FIFO_R_W, pData);
}

void icm20602_FIFOWrite(u8 Data)
{
    icm20602_PortTransmmit(FIFO_R_W, Data);
}

void icm20602_WhoAmI(u8 *pData)
{
    icm20602_PortReceive(WHO_AM_I, pData);
}
void mpu9250_ReadAccelOffsetRegisters(u8 Axis, u16 *pData)
{
    u8 SubData1 = 0;
    u8 SubData2 = 0;

    switch (Axis) {
        case X_AXIS:
            icm20602_PortReceive(XA_OFFSET_H, &SubData1);
            icm20602_PortReceive(XA_OFFSET_L, &SubData2);
            break;

        case Y_AXIS:
            icm20602_PortReceive(YA_OFFSET_H, &SubData1);
            icm20602_PortReceive(YA_OFFSET_L, &SubData2);
            break;

        case Z_AXIS:
            icm20602_PortReceive(ZA_OFFSET_H, &SubData1);
            icm20602_PortReceive(ZA_OFFSET_L, &SubData2);
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
            icm20602_PortTransmmit(XA_OFFSET_H, SubData1);
            icm20602_PortTransmmit(XA_OFFSET_L, SubData2);
            break;

        case Y_AXIS:
            icm20602_PortTransmmit(YA_OFFSET_H, SubData1);
            icm20602_PortTransmmit(YA_OFFSET_L, SubData2);
            break;

        case Z_AXIS:
            icm20602_PortTransmmit(ZA_OFFSET_H, SubData1);
            icm20602_PortTransmmit(ZA_OFFSET_L, SubData2);
            break;

        default:
            break;
    }
}

#ifdef DESIGN_VERIFICATION_ICM20602
static u8 tx_buffer[256];
static u8 rx_buffer[256];

void icm20602_Test(void)
{
    u32 TmpRngdata = 0;
    u16 BufferLength = 0;
    u32 TestAddr = 0;

    random_get8bit(&hrng, &TmpRngdata);
    BufferLength = TmpRngdata & 0xFF;
    icm20602_printf("BufferLength = %d.", BufferLength);

    if (tx_buffer == NULL || rx_buffer == NULL) {
        icm20602_printf("Failed to allocate memory !");
        return;
    }

    memset(tx_buffer, 0, BufferLength);
    memset(rx_buffer, 0, BufferLength);

    random_get8bit(&hrng, &TmpRngdata);
    TestAddr = TmpRngdata & 0xFF;
    icm20602_printf("TestAddr = 0x%02X.", TestAddr);

    for (u16 i = 0; i < BufferLength; i += 4) {
        random_get8bit(&hrng, &TmpRngdata);
        tx_buffer[i + 3] = (TmpRngdata & 0xFF000000) >> 24;;
        tx_buffer[i + 2] = (TmpRngdata & 0x00FF0000) >> 16;
        tx_buffer[i + 1] = (TmpRngdata & 0x0000FF00) >> 8;
        tx_buffer[i + 0] = (TmpRngdata & 0x000000FF);
    }

    icm20602_WriteData(TestAddr, tx_buffer, BufferLength);
    icm20602_ReadData(TestAddr, rx_buffer, BufferLength);

    for (u16 i = 0; i < BufferLength; i++) {
        if (tx_buffer[i] != rx_buffer[i]) {
            icm20602_printf("tx_buffer[%d] = 0x%02X, rx_buffer[%d] = 0x%02X",
                i, tx_buffer[i],
                i, rx_buffer[i]);
            icm20602_printf("Data writes and reads do not match, TEST FAILED !");
            return ;
        }
    }

    icm20602_printf("icm20602 Read and write TEST PASSED !");
}

#endif


