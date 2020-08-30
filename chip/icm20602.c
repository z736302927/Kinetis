#include "icm20602/icm20602.h"

/*The following program is modified by the user according to the hardware device, otherwise the driver cannot run.*/

/**
  * @step 1:  Modify the corresponding function according to the modified area and the corresponding function name.
  * @step 2:  Modify four areas: GPIO_PORT/GPIO_PIN/Led_TypeDef/LEDn.
  * @step 3:  .
  * @step 4:  .
  * @step 5:
  */

#include "iic_soft/iic_soft.h"

#define DEBUG
#include "core/idebug.h"

#define icm20602_printf                  p_dbg

#define ICM20602_ADDR                    0x00

void icm20602_Delayus(uint32_t ticks)
{
    Delay_us(ticks);
}

void icm20602_Delayms(uint32_t ticks)
{
    Delay_ms(ticks);
}

void icm20602_PortTransmmit(uint8_t Addr, uint8_t Data)
{
    IIC_Soft_WriteSingleByteWithAddr(ICM20602_ADDR, Addr, Data);
}

void icm20602_PortReceive(uint8_t Addr, uint8_t *pData)
{
    IIC_Soft_ReadSingleByteWithAddr(ICM20602_ADDR, Addr, pData);
}

void icm20602_PortMultiTransmmit(uint8_t Addr, uint8_t *pData, uint32_t Length)
{
    IIC_Soft_WriteMultiByteWithAddr(ICM20602_ADDR, Addr, pData, Length);
}

void icm20602_PortMultiReceive(uint8_t Addr, uint8_t *pData, uint32_t Length)
{
    IIC_Soft_ReadMultiByteWithAddr(ICM20602_ADDR, Addr, pData, Length);
}

/*The above procedure is modified by the user according to the hardware device, otherwise the driver cannot run.*/

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

void icm20602_ReadGyroLPOffsetRegisters(uint8_t Axis, uint8_t *pData)
{

    switch(Axis)
    {
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

void icm20602_WriteGyroLPOffsetRegisters(uint8_t Axis, uint16_t Data)
{
    uint8_t TmpReg = 0;

    Data &= 0x3F;

    switch(Axis)
    {
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

void icm20602_ReadGyroTCOffsetRegisters(uint8_t Axis, uint16_t *pData)
{
    uint8_t SubData1 = 0;
    uint8_t SubData2 = 0;

    switch(Axis)
    {
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

void icm20602_WriteGyroTCOffsetRegisters(uint8_t Axis, uint16_t Data)
{
    uint8_t SubData1 = 0;
    uint8_t SubData2 = 0;

    Data &= 0x3FF;

    switch(Axis)
    {
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

void icm20602_ReadAccelSelfTestRegisters(uint8_t Axis, uint8_t *pData)
{
    switch(Axis)
    {
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

void icm20602_WriteAccelSelfTestRegisters(uint8_t Axis, uint8_t Data)
{
    switch(Axis)
    {
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

void icm20602_ReadGyroOffsetRegisters(uint8_t Axis, uint16_t *pData)
{
    uint8_t SubData1 = 0;
    uint8_t SubData2 = 0;

    switch(Axis)
    {
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

void icm20602_WriteGyroOffsetRegisters(uint8_t Axis, uint16_t Data)
{
    uint8_t SubData1 = 0;
    uint8_t SubData2 = 0;

    SubData1 = Data >> 8;
    SubData2 = Data & 0xFF;

    switch(Axis)
    {
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

void icm20602_SampleRateDivider(uint8_t Data)
{
    icm20602_PortTransmmit(SMPLRT_DIV, Data);
}

void icm20602_FIFOMode(uint8_t Data)
{
    uint8_t TmpReg = 0;

    icm20602_PortReceive(CONFIG, &TmpReg);

    Data &= 0x01;
    TmpReg &= ~(0x01 << 6);
    TmpReg |= (Data << 6);

    icm20602_PortTransmmit(CONFIG, TmpReg);
}

void icm20602_FsyncSet(uint8_t Data)
{
    uint8_t TmpReg = 0;

    icm20602_PortReceive(CONFIG, &TmpReg);

    Data &= 0x07;
    TmpReg &= ~(0x07 << 3);
    TmpReg |= (Data << 3);

    icm20602_PortTransmmit(CONFIG, TmpReg);
}

void icm20602_ConfigDLPF(uint8_t Data)
{
    uint8_t TmpReg = 0;

    icm20602_PortReceive(CONFIG, &TmpReg);

    Data &= 0x07;
    TmpReg &= ~(0x07 << 0);
    TmpReg |= (Data << 0);

    icm20602_PortTransmmit(CONFIG, TmpReg);
}

void icm20602_GyroSelfTest(uint8_t Axis, uint8_t Data)
{
    uint8_t TmpReg = 0;

    icm20602_PortReceive(GYRO_CONFIG, &TmpReg);
    Data &= 0x01;

    switch(Axis)
    {
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

void icm20602_GyroFullScaleSelect(uint8_t Data)
{
    uint8_t TmpReg = 0;

    icm20602_PortReceive(GYRO_CONFIG, &TmpReg);

    Data &= 0x03;
    TmpReg &= ~(0x03 << 3);
    TmpReg |= (Data << 3);

    icm20602_PortTransmmit(GYRO_CONFIG, TmpReg);
}

void icm20602_Fchoice_b(uint8_t Data)
{
    uint8_t TmpReg = 0;

    icm20602_PortReceive(GYRO_CONFIG, &TmpReg);

    Data &= 0x03;
    TmpReg &= ~(0x03 << 0);
    TmpReg |= (Data << 0);

    icm20602_PortTransmmit(GYRO_CONFIG, TmpReg);
}

void icm20602_AccelSelfTest(uint8_t Axis, uint8_t Data)
{
    uint8_t TmpReg = 0;

    icm20602_PortReceive(ACCEL_CONFIG, &TmpReg);
    Data &= 0x01;

    switch(Axis)
    {
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

void icm20602_AccelFullScaleSelect(uint8_t Data)
{
    uint8_t TmpReg = 0;

    icm20602_PortReceive(ACCEL_CONFIG, &TmpReg);

    Data &= 0x03;
    TmpReg &= ~(0x03 << 3);
    TmpReg |= (Data << 3);

    icm20602_PortTransmmit(ACCEL_CONFIG, TmpReg);
}

void icm20602_AveragingFilterForLPAccel(uint8_t Data)
{
    uint8_t TmpReg = 0;

    icm20602_PortReceive(ACCEL_CONFIG2, &TmpReg);

    Data &= 0x03;
    TmpReg &= ~(0x03 << 4);
    TmpReg |= (Data << 4);

    icm20602_PortTransmmit(ACCEL_CONFIG2, TmpReg);
}

void icm20602_Accel_Fchoice_b(uint8_t Data)
{
    uint8_t TmpReg = 0;

    icm20602_PortReceive(ACCEL_CONFIG2, &TmpReg);

    Data &= 0x01;
    TmpReg &= ~(0x01 << 3);
    TmpReg |= (Data << 3);

    icm20602_PortTransmmit(ACCEL_CONFIG2, TmpReg);
}

void icm20602_ConfigAccelDLPF(uint8_t Data)
{
    uint8_t TmpReg = 0;

    icm20602_PortReceive(ACCEL_CONFIG2, &TmpReg);

    Data &= 0x07;
    TmpReg &= ~(0x07 << 0);
    TmpReg |= (Data << 0);

    icm20602_PortTransmmit(ACCEL_CONFIG2, TmpReg);
}

void icm20602_ConfigLPGyroMode(uint8_t Data)
{
    uint8_t TmpReg = 0;

    icm20602_PortReceive(LP_MODE_CFG, &TmpReg);

    Data &= 0x01;
    TmpReg &= ~(0x01 << 7);
    TmpReg |= (Data << 7);

    icm20602_PortTransmmit(LP_MODE_CFG, TmpReg);
}

void icm20602_AveragingFilterForLPGyro(uint8_t Data)
{
    uint8_t TmpReg = 0;

    icm20602_PortReceive(LP_MODE_CFG, &TmpReg);

    Data &= 0x03;
    TmpReg &= ~(0x03 << 4);
    TmpReg |= (Data << 4);

    icm20602_PortTransmmit(LP_MODE_CFG, TmpReg);
}

void icm20602_WakeonMotionThreshold(uint8_t Axis, uint8_t Data)
{
    switch(Axis)
    {
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

void icm20602_FIFOEnableWithGyro(uint8_t Data)
{
    uint8_t TmpReg = 0;

    icm20602_PortReceive(FIFO_EN, &TmpReg);

    Data &= 0x01;
    TmpReg &= ~(0x01 << 4);
    TmpReg |= (Data << 4);

    icm20602_PortTransmmit(FIFO_EN, TmpReg);
}

void icm20602_FIFOEnableWithAccel(uint8_t Data)
{
    uint8_t TmpReg = 0;

    icm20602_PortReceive(FIFO_EN, &TmpReg);

    Data &= 0x01;
    TmpReg &= ~(0x01 << 3);
    TmpReg |= (Data << 3);

    icm20602_PortTransmmit(FIFO_EN, TmpReg);
}

uint8_t icm20602_StatusOfFsyncInt(void)
{
    uint8_t TmpReg = 0;

    icm20602_PortReceive(FSYNC_INT, &TmpReg);

    if(TmpReg & 0x80)
        return 1;
    else
        return 0;
}

void icm20602_LogicLevelForInt(uint8_t Data)
{
    uint8_t TmpReg = 0;

    icm20602_PortReceive(INT_PIN_CFG, &TmpReg);

    Data &= 0x01;
    TmpReg &= ~(0x01 << 7);
    TmpReg |= (Data << 7);

    icm20602_PortTransmmit(INT_PIN_CFG, TmpReg);
}

void icm20602_EnablePullup(uint8_t Data)
{
    uint8_t TmpReg = 0;

    icm20602_PortReceive(INT_PIN_CFG, &TmpReg);

    Data &= 0x01;
    TmpReg &= ~(0x01 << 6);
    TmpReg |= (Data << 6);

    icm20602_PortTransmmit(INT_PIN_CFG, TmpReg);
}

void icm20602_LatchIntPin(uint8_t Data)
{
    uint8_t TmpReg = 0;

    icm20602_PortReceive(INT_PIN_CFG, &TmpReg);

    Data &= 0x01;
    TmpReg &= ~(0x01 << 5);
    TmpReg |= (Data << 5);

    icm20602_PortTransmmit(INT_PIN_CFG, TmpReg);
}

void icm20602_IntAnyrd2Clear(uint8_t Data)
{
    uint8_t TmpReg = 0;

    icm20602_PortReceive(INT_PIN_CFG, &TmpReg);

    Data &= 0x01;
    TmpReg &= ~(0x01 << 4);
    TmpReg |= (Data << 4);

    icm20602_PortTransmmit(INT_PIN_CFG, TmpReg);
}

void icm20602_LogicLevelForFsync(uint8_t Data)
{
    uint8_t TmpReg = 0;

    icm20602_PortReceive(INT_PIN_CFG, &TmpReg);

    Data &= 0x01;
    TmpReg &= ~(0x01 << 3);
    TmpReg |= (Data << 3);

    icm20602_PortTransmmit(INT_PIN_CFG, TmpReg);
}

void icm20602_FsyncIntMode(uint8_t Data)
{
    uint8_t TmpReg = 0;

    icm20602_PortReceive(INT_PIN_CFG, &TmpReg);

    Data &= 0x01;
    TmpReg &= ~(0x01 << 2);
    TmpReg |= (Data << 2);

    icm20602_PortTransmmit(INT_PIN_CFG, TmpReg);
}

uint8_t icm20602_StatusOfWatermarkInt(void)
{
    uint8_t TmpReg = 0;

    icm20602_PortReceive(FIFO_WM_INT_STATUS, &TmpReg);

    if(TmpReg & 0x40)
        return 1;
    else
        return 0;
}

uint8_t icm20602_IntStatusForWakeOnMotion(void)
{
    uint8_t TmpReg = 0;

    icm20602_PortReceive(INT_STATUS, &TmpReg);

    if(TmpReg & 0x40)
        return 1;
    else
        return 0;
}

uint8_t icm20602_IntStatusForFIFOOverflow(void)
{
    uint8_t TmpReg = 0;

    icm20602_PortReceive(INT_STATUS, &TmpReg);

    if(TmpReg & 0x10)
        return 1;
    else
        return 0;
}

uint8_t icm20602_IntStatusForFsync(void)
{
    uint8_t TmpReg = 0;

    icm20602_PortReceive(INT_STATUS, &TmpReg);

    if(TmpReg & 0x08)
        return 1;
    else
        return 0;
}

void icm20602_IntStatus(uint8_t *pData)
{
    icm20602_PortReceive(INT_STATUS, pData);
}

void icm20602_AccelMeasurements(uint16_t *pData)
{
    uint8_t TmpVal[6];

    icm20602_PortMultiReceive(ACCEL_XOUT_H, TmpVal, 6);

    pData[0] = (TmpVal[0] << 8) | TmpVal[1];
    pData[1] = (TmpVal[2] << 8) | TmpVal[3];
    pData[2] = (TmpVal[4] << 8) | TmpVal[5];
}

void icm20602_TempMeasurement(uint16_t *pData)
{
    uint8_t TmpVal[2];

    icm20602_PortMultiReceive(TEMP_OUT_H, TmpVal, 2);

    pData[0] = (TmpVal[0] << 8) | TmpVal[1];
}

void icm20602_GetTemperature(float *pData)
{
    uint16_t TmpVal;

    icm20602_TempMeasurement(&TmpVal);

    pData[0] = ((TmpVal - 0) / 326.8) + 25;
}

void icm20602_GyroMeasurements(uint16_t *pData)
{
    uint8_t TmpVal[6];

    icm20602_PortMultiReceive(GYRO_XOUT_H, TmpVal, 6);

    pData[0] = (TmpVal[0] << 8) | TmpVal[1];
    pData[1] = (TmpVal[2] << 8) | TmpVal[3];
    pData[2] = (TmpVal[4] << 8) | TmpVal[5];
}

void icm20602_GetAccelAndGyro(uint16_t *pData)
{
    uint8_t TmpVal[14];

    icm20602_PortMultiReceive(ACCEL_XOUT_H, TmpVal, 14);

    pData[0] = (TmpVal[0] << 8) | TmpVal[1];
    pData[1] = (TmpVal[2] << 8) | TmpVal[3];
    pData[2] = (TmpVal[4] << 8) | TmpVal[5];

    pData[3] = (TmpVal[8] << 8) | TmpVal[9];
    pData[4] = (TmpVal[10] << 8) | TmpVal[11];
    pData[5] = (TmpVal[12] << 8) | TmpVal[13];
}

void icm20602_ReadGyroSelfTestRegisters(uint8_t Axis, uint8_t *pData)
{
    switch(Axis)
    {
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

void icm20602_WriteGyroSelfTestRegisters(uint8_t Axis, uint8_t Data)
{
    switch(Axis)
    {
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
void icm20602_ReadFIFOWatermarkThreshold(uint16_t *pData)
{
    uint8_t TmpReg[2] = {0, 0};

    icm20602_PortMultiReceive(FIFO_WM_TH1, TmpReg, 2);

    *pData = ((TmpReg[0] & 0x03) << 8) | TmpReg[1];
}

void icm20602_WriteFIFOWatermarkThreshold(uint16_t Data)
{
    uint8_t TmpReg[2] = {0, 0};

    Data &= 0x3FF;

    icm20602_PortMultiReceive(FIFO_WM_TH2, TmpReg, 2);
    TmpReg[0] &= ~0x03;
    TmpReg[0] |= Data >> 8;
    TmpReg[1] = Data & 0xFF;
    icm20602_PortMultiTransmmit(FIFO_WM_TH2, TmpReg, 2);
}

void icm20602_AccelSignalPathReset(uint8_t Data)
{
    uint8_t TmpReg = 0;

    icm20602_PortReceive(SIGNAL_PATH_RESET, &TmpReg);

    Data &= 0x01;
    TmpReg &= ~(0x01 << 1);
    TmpReg |= (Data << 1);

    icm20602_PortTransmmit(SIGNAL_PATH_RESET, TmpReg);
}

void icm20602_TempSignalPathReset(uint8_t Data)
{
    uint8_t TmpReg = 0;

    icm20602_PortReceive(SIGNAL_PATH_RESET, &TmpReg);

    Data &= 0x01;
    TmpReg &= ~(0x01 << 0);
    TmpReg |= (Data << 0);

    icm20602_PortTransmmit(SIGNAL_PATH_RESET, TmpReg);
}

void icm20602_EnableWakeonMotion(uint8_t Data)
{
    uint8_t TmpReg = 0;

    icm20602_PortReceive(ACCEL_INTEL_CTRL, &TmpReg);

    Data &= 0x01;
    TmpReg &= ~(0x01 << 7);
    TmpReg |= (Data << 7);

    icm20602_PortTransmmit(ACCEL_INTEL_CTRL, TmpReg);
}

void icm20602_IntelligenceMode(uint8_t Data)
{
    uint8_t TmpReg = 0;

    icm20602_PortReceive(ACCEL_INTEL_CTRL, &TmpReg);

    Data &= 0x01;
    TmpReg &= ~(0x01 << 6);
    TmpReg |= (Data << 6);

    icm20602_PortTransmmit(ACCEL_INTEL_CTRL, TmpReg);
}

void icm20602_OutputLimit(uint8_t Data)
{
    uint8_t TmpReg = 0;

    icm20602_PortReceive(ACCEL_INTEL_CTRL, &TmpReg);

    Data &= 0x01;
    TmpReg &= ~(0x01 << 1);
    TmpReg |= (Data << 1);

    icm20602_PortTransmmit(ACCEL_INTEL_CTRL, TmpReg);
}

void icm20602_WakeonMotionIntMode(uint8_t Data)
{
    uint8_t TmpReg = 0;

    icm20602_PortReceive(ACCEL_INTEL_CTRL, &TmpReg);

    Data &= 0x01;
    TmpReg &= ~(0x01 << 0);
    TmpReg |= (Data << 0);

    icm20602_PortTransmmit(ACCEL_INTEL_CTRL, TmpReg);
}

void icm20602_EnableFIFO(uint8_t Data)
{
    uint8_t TmpReg = 0;

    icm20602_PortReceive(USER_CTRL, &TmpReg);

    Data &= 0x01;
    TmpReg &= ~(0x01 << 6);
    TmpReg |= (Data << 6);

    icm20602_PortTransmmit(USER_CTRL, TmpReg);
}

void icm20602_ResetFIFO(uint8_t Data)
{
    uint8_t TmpReg = 0;

    icm20602_PortReceive(USER_CTRL, &TmpReg);

    Data &= 0x01;
    TmpReg &= ~(0x01 << 2);
    TmpReg |= (Data << 2);

    icm20602_PortTransmmit(USER_CTRL, TmpReg);
}

void icm20602_ResetSignalPath(uint8_t Data)
{
    uint8_t TmpReg = 0;

    icm20602_PortReceive(USER_CTRL, &TmpReg);

    Data &= 0x01;
    TmpReg &= ~(0x01 << 0);
    TmpReg |= (Data << 0);

    icm20602_PortTransmmit(USER_CTRL, TmpReg);
}

void icm20602_SoftReset(uint8_t Data)
{
    uint8_t TmpReg = 0;

    icm20602_PortReceive(PWR_MGMT_1, &TmpReg);

    Data &= 0x01;
    TmpReg &= ~(0x01 << 7);
    TmpReg |= (Data << 7);

    icm20602_PortTransmmit(PWR_MGMT_1, TmpReg);
}

void icm20602_EnterSleep(uint8_t Data)
{
    uint8_t TmpReg = 0;

    icm20602_PortReceive(PWR_MGMT_1, &TmpReg);

    Data &= 0x01;
    TmpReg &= ~(0x01 << 6);
    TmpReg |= (Data << 6);

    icm20602_PortTransmmit(PWR_MGMT_1, TmpReg);
}

void icm20602_CycleSample(uint8_t Data)
{
    uint8_t TmpReg = 0;

    icm20602_PortReceive(PWR_MGMT_1, &TmpReg);

    Data &= 0x01;
    TmpReg &= ~(0x01 << 5);
    TmpReg |= (Data << 5);

    icm20602_PortTransmmit(PWR_MGMT_1, TmpReg);
}

void icm20602_GyroStandby(uint8_t Data)
{
    uint8_t TmpReg = 0;

    icm20602_PortReceive(PWR_MGMT_1, &TmpReg);

    Data &= 0x01;
    TmpReg &= ~(0x01 << 4);
    TmpReg |= (Data << 4);

    icm20602_PortTransmmit(PWR_MGMT_1, TmpReg);
}

void icm20602_DisableTemp(uint8_t Data)
{
    uint8_t TmpReg = 0;

    icm20602_PortReceive(PWR_MGMT_1, &TmpReg);

    Data &= 0x01;
    TmpReg &= ~(0x01 << 3);
    TmpReg |= (Data << 3);

    icm20602_PortTransmmit(PWR_MGMT_1, TmpReg);
}

void icm20602_ClockSourceSelect(uint8_t Data)
{
    uint8_t TmpReg = 0;

    icm20602_PortReceive(PWR_MGMT_1, &TmpReg);

    Data &= 0x07;
    TmpReg &= ~(0x07 << 0);
    TmpReg |= (Data << 0);

    icm20602_PortTransmmit(PWR_MGMT_1, TmpReg);
}

void icm20602_AccelDisabled(uint8_t Axis, uint8_t Data)
{
    uint8_t TmpReg = 0;

    icm20602_PortReceive(PWR_MGMT_2, &TmpReg);
    Data &= 0x01;

    switch(Axis)
    {
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

void icm20602_GyroDisabled(uint8_t Axis, uint8_t Data)
{
    uint8_t TmpReg = 0;

    icm20602_PortReceive(PWR_MGMT_2, &TmpReg);
    Data &= 0x01;

    switch(Axis)
    {
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

void icm20602_EnableSPI(uint8_t Data)
{
    uint8_t TmpReg = 0;

    icm20602_PortReceive(I2C_IF, &TmpReg);

    Data &= 0x01;
    TmpReg &= ~(0x01 << 6);
    TmpReg |= (Data << 6);

    icm20602_PortTransmmit(I2C_IF, TmpReg);
}

void icm20602_FIFOCount(uint16_t *pData)
{
    uint8_t TmpReg[2];

    icm20602_PortMultiReceive(FIFO_COUNTH, TmpReg, 2);

    pData[0] = (TmpReg[0] << 8) | TmpReg[1];
}

void icm20602_FIFORead(uint8_t *pData)
{
    icm20602_PortReceive(FIFO_R_W, pData);
}

void icm20602_FIFOWrite(uint8_t Data)
{
    icm20602_PortTransmmit(FIFO_R_W, Data);
}

void icm20602_WhoAmI(uint8_t *pData)
{
    icm20602_PortReceive(WHO_AM_I, pData);
}
void mpu9250_ReadAccelOffsetRegisters(uint8_t Axis, uint16_t *pData)
{
    uint8_t SubData1 = 0;
    uint8_t SubData2 = 0;

    switch(Axis)
    {
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

void mpu9250_WriteAccelOffsetRegisters(uint8_t Axis, uint16_t Data)
{
    uint8_t SubData1 = 0;
    uint8_t SubData2 = 0;

    SubData1 = Data >> 7;
    SubData2 = (Data & 0xFE) >> 1;

    switch(Axis)
    {
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
static uint8_t Tx_Buffer[256];
static uint8_t Rx_Buffer[256];

void icm20602_Test(void)
{
    uint32_t TmpRngdata = 0;
    uint16_t BufferLength = 0;
    uint32_t TestAddr = 0;

    Random_Get8bit(&hrng, &TmpRngdata);
    BufferLength = TmpRngdata & 0xFF;
    icm20602_printf("BufferLength = %d.", BufferLength);

    if(Tx_Buffer == NULL || Rx_Buffer == NULL)
    {
        icm20602_printf("Failed to allocate memory !");
        return;
    }

    memset(Tx_Buffer, 0, BufferLength);
    memset(Rx_Buffer, 0, BufferLength);

    Random_Get8bit(&hrng, &TmpRngdata);
    TestAddr = TmpRngdata & 0xFF;
    icm20602_printf("TestAddr = 0x%02X.", TestAddr);

    for(uint16_t i = 0; i < BufferLength; i += 4)
    {
        Random_Get8bit(&hrng, &TmpRngdata);
        Tx_Buffer[i + 3] = (TmpRngdata & 0xFF000000) >> 24;;
        Tx_Buffer[i + 2] = (TmpRngdata & 0x00FF0000) >> 16;
        Tx_Buffer[i + 1] = (TmpRngdata & 0x0000FF00) >> 8;
        Tx_Buffer[i + 0] = (TmpRngdata & 0x000000FF);
    }

    icm20602_WriteData(TestAddr, Tx_Buffer, BufferLength);
    icm20602_ReadData(TestAddr, Rx_Buffer, BufferLength);

    for(uint16_t i = 0; i < BufferLength; i++)
    {
        if(Tx_Buffer[i] != Rx_Buffer[i])
        {
            icm20602_printf("Tx_Buffer[%d] = 0x%02X, Rx_Buffer[%d] = 0x%02X",
                i, Tx_Buffer[i],
                i, Rx_Buffer[i]);
            icm20602_printf("Data writes and reads do not match, TEST FAILED !");
            return ;
        }
    }

    icm20602_printf("icm20602 Read and write TEST PASSED !");
}

#endif


