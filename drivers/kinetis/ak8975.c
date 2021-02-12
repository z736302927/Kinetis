#include "ak8975/ak8975.h"
#include "bitop.h"

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

#define ak8975_printf                  p_dbg
#define AK8975_USING_IIC

#define AK8975_CAD0                    0
#define AK8975_CAD1                    0

#ifdef (!AK8975_CAD0 && !AK8975_CAD0)
#define AK8975_ADDR                    0x0C
#elif (!AK8975_CAD0 && AK8975_CAD0)
#define AK8975_ADDR                    0x0D
#elif (AK8975_CAD0 && !AK8975_CAD0)
#define AK8975_ADDR                    0x0E
#elif (AK8975_CAD0 && AK8975_CAD0)
#define AK8975_ADDR                    0x0F
#endif

void ak8975_Delayus(u32 ticks)
{
    udelay(ticks);
}

void ak8975_Delayms(u32 ticks)
{
    mdelay(ticks);
}

void ak8975_CSB_Low(void)
{
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_RESET);
}

void ak8975_CSB_High(void)
{
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_SET);
}

void ak8975_PortTransmmit(u8 Addr, u8 Data)
{
#ifdef AK8975_USING_IIC
    IIC_Soft_WriteSingleByteWithAddr(AK8975_ADDR, Addr, Data);
#else
    ak8975_CSB_Low();
    HAL_SPI_Transmit(&hspi1, (Addr << 1) | 0, 1, 1000);
    HAL_SPI_Transmit(&hspi1, &Data, 1, 1000);
    ak8975_CSB_High();
#endif
}

void ak8975_PortReceive(u8 Addr, u8 *pData)
{
#ifdef AK8975_USING_IIC
    IIC_Soft_ReadSingleByteWithAddr(AK8975_ADDR, Addr, pData);
#else
    ak8975_CSB_Low();
    HAL_SPI_Transmit(&hspi1, (Addr << 1) | 1, 1, 1000);
    HAL_SPI_Receive(&hspi1, pData, 1, 1000);
    ak8975_CSB_High();
#endif
}

void ak8975_port_multi_transmmit(u8 Addr, u8 *pData, u32 Length)
{
#ifdef AK8975_USING_IIC
    IIC_Soft_WriteMultiByteWithAddr(AK8975_ADDR, Addr, pData, Length);
#else
    ak8975_CSB_Low();
    HAL_SPI_Transmit(&hspi1, (Addr << 1) | 0, 1, 1000);
    HAL_SPI_Transmit(&hspi1, pData, Length, 1000);
    ak8975_CSB_High();
#endif
}

void ak8975_port_multi_receive(u8 Addr, u8 *pData, u32 Length)
{
#ifdef AK8975_USING_IIC
    IIC_Soft_ReadMultiByteWithAddr(AK8975_ADDR, 4Addr, pData, Length);
#else
    ak8975_CSB_Low();
    HAL_SPI_Transmit(&hspi1, (Addr << 1) | 1, 1, 1000);
    HAL_SPI_Receive(&hspi1, pData, Length, 1000);
    ak8975_CSB_High();
#endif
}

/* The above procedure is modified by the user according to the hardware device, otherwise the driver cannot run. */

#define WIA                             0x00
#define INFO                            0x01
#define ST1                             0x02
#define HXL                             0x03
#define HXH                             0x04
#define HYL                             0x05
#define HYH                             0x06
#define HZL                             0x07
#define HZH                             0x08
#define ST2                             0x09
#define CNTL1                           0x0A
#define CNTL2                           0x0B
#define ASTC                            0x0C
#define TS1                             0x0D
#define TS2                             0x0E
#define I2CDIS                          0x0F
#define ASAX                            0x10
#define ASAY                            0x11
#define ASAZ                            0x12

#define AKM_DEVID                       0x48
#define POWER_DOWN                      0x0000
#define SINGLE_MEASUREMENT              0x0001
#define SELF_TEST                       0x1000
#define FUSE_ROM_ACCESS                 0x1111
#define AK8975_TBD                      0

void ak8975_EnterPowerdownMode(void)
{
    ak8975_PortTransmmit(CNTL1, POWER_DOWN);
}

void ak8975_EnterSingleMeasurementMode(void)
{
    ak8975_PortTransmmit(CNTL1, SINGLE_MEASUREMENT);
}

void ak8975_EnterSelftestMode(void)
{
    ak8975_PortTransmmit(CNTL1, SELF_TEST);
}

void ak8975_EnterFuseROMAccessMode(void)
{
    ak8975_PortTransmmit(CNTL1, FUSE_ROM_ACCESS);
}

void ak8975_WhoAmI(u8 *pData)
{
    ak8975_PortReceive(WIA, pData);
}

void ak8975_DeviceInformation(u8 *pData)
{
    ak8975_PortReceive(INFO, pData);
}

u8 ak8975_DataReady(void)
{
    u8 TmpReg = 0;

    ak8975_PortReceive(ST1, &TmpReg);

    return TmpReg & 0x01;
}

u8 ak8975_DataOverrun(void)
{
    u8 TmpReg = 0;

    ak8975_PortReceive(ST1, &TmpReg);

    return (TmpReg & 0x02) >> 1;
}

void ak8975_MagneticMeasurements(u16 *pData)
{
    u8 TmpVal[6];

    ak8975_port_multi_receive(HXL, TmpVal, 6);

    pData[0] = (TmpVal[1] << 8) | TmpVal[0];
    pData[1] = (TmpVal[3] << 8) | TmpVal[2];
    pData[2] = (TmpVal[5] << 8) | TmpVal[4];
}

u8 ak8975_MagneticSensorOverflow(void)
{
    u8 TmpReg = 0;

    ak8975_PortReceive(ST2, &TmpReg);

    return (TmpReg & 0x08) >> 3;
}

u8 ak8975_OutputBitSettingMirror(void)
{
    u8 TmpReg = 0;

    ak8975_PortReceive(ST2, &TmpReg);

    return (TmpReg & 0x10) >> 4;
}

void ak8975_OperationModeSetting(u8 Data)
{
    u8 TmpReg = 0;

    ak8975_PortReceive(CNTL1, &TmpReg);

    Data &= 0x0F;
    TmpReg &= ~(0x0F << 0);
    TmpReg |= (Data << 0);

    ak8975_PortTransmmit(CNTL1, TmpReg);
}

void ak8975_OutputBitSetting(u8 Data)
{
    u8 TmpReg = 0;

    ak8975_PortReceive(CNTL1, &TmpReg);

    Data &= 0x01;
    TmpReg &= ~(0x01 << 4);
    TmpReg |= (Data << 4);

    ak8975_PortTransmmit(CNTL1, TmpReg);
}

void ak8975_SoftReset(u8 Data)
{
    u8 TmpReg = 0;

    ak8975_PortReceive(CNTL2, &TmpReg);

    Data &= 0x01;
    TmpReg &= ~(0x01 << 0);
    TmpReg |= (Data << 0);

    ak8975_PortTransmmit(CNTL2, TmpReg);
}

void ak8975_SelfTestControl(u8 Data)
{
    u8 TmpReg = 0;

    ak8975_PortReceive(ASTC, &TmpReg);

    Data &= 0x01;
    TmpReg &= ~(0x01 << 6);
    TmpReg |= (Data << 6);

    ak8975_PortTransmmit(ASTC, TmpReg);
}

void ak8975_I2CDisable(void)
{
    ak8975_PortTransmmit(I2CDIS, 0x1B);
}

void ak8975_I2CEnable(void)
{
    ak8975_SoftReset(1);
}

static u8 ak8975_ASAValues[3];

void ak8975_SensitivityAdjustmentValues(u8 *pData)
{
    ak8975_port_multi_receive(ASAX, pData, 3);
}

u8 ak8975_MagneticAdjustedMeasurements(u16 *pData)
{
    u16 RawData[3];
    u32 Timeout = 1000;

    ak8975_EnterSingleMeasurementMode();

    do {
        if (ak8975_DataReady() == true)
            break;
        else
            mdelay(1);
    } while (Timeout--)

        if (Timeout <= 0) {
            printk(KERN_DEBUG "[Error] ak8975 Magnetic Data not ready");
            return FAIL;
        }

//  Timeout_WaitMSDone(&ak8975_DR_Flag, true, 1000);

    if (ak8975_DataOverrun() == true)
        printk(KERN_DEBUG "[Warning] ak8975 Magnetic Data Overrun");

    ak8975_MagneticMeasurements(RawData);

    pData[0] = RawData[0] * ((ak8975_ASAValues[0] - 128) / 256 + 1);
    pData[1] = RawData[1] * ((ak8975_ASAValues[1] - 128) / 256 + 1);
    pData[2] = RawData[2] * ((ak8975_ASAValues[2] - 128) / 256 + 1);
}

volatile u8 ak8975_DR_Flag = 0;

void ak8975_Init(void)
{
    ak8975_EnterFuseROMAccessMode();
    ak8975_SensitivityAdjustmentValues(ak8975_ASAValues);
    printk(KERN_DEBUG "ak8975 Adjustment Values %x, %x, %x", ak8975_ASAValues[0],
        ak8975_ASAValues[1], ak8975_ASAValues[2]);
    ak8975_EnterPowerdownMode();
}

#ifdef DESIGN_VERIFICATION_AK8975
#include "kinetis/test-kinetis.h"

int t_ak8975_BasicInfo(int argc, char **argv)
{
    u8 Data = 0;

    ak8975_WhoAmI(&Data);
    printk(KERN_DEBUG "Device ID of AKM8975 is 0x%x", Data);

    if (Data != AKM_DEVID)
        return PASS;
    else {
        printk(KERN_DEBUG "Device ID of AKM8975 is not correct");
        return FAIL;
    }

    ak8975_DeviceInformation(&Data);
    printk(KERN_DEBUG "Device information for AKM8975 %x", Data);

    if (Data != 0x00)
        return PASS;
    else
        return FAIL;
}

int t_ak8975_Magnetic(int argc, char **argv)
{
    u16 Magnetic[3] = {0, 0, 0};
    u16 times = 128;
    u8 i = 0;
    u32 Timeout = 1000;


    if (argc > 1)
        times = strtoul(argv[1], &argv[1], 10);

    ak8975_EnterPowerdownMode();

    for (i = 0; i < times; i++) {
        ak8975_EnterSingleMeasurementMode();

        do {
            if (ak8975_DataReady() == true)
                break;
            else
                mdelay(1);
        } while (Timeout--)

            if (Timeout <= 0) {
                printk(KERN_DEBUG "[Error] ak8975 Magnetic Data not ready");
                return FAIL;
            }

//    Timeout_WaitMSDone(&ak8975_DR_Flag, true, 1000);

        if (ak8975_DataOverrun() == true)
            printk(KERN_DEBUG "[Warning] ak8975 Magnetic Data Overrun");

        ak8975_MagneticMeasurements(Magnetic);
        printk(KERN_DEBUG "ak8975 Magnetic Data %x, %x, %x", Magnetic[0], Magnetic[1], Magnetic[2]);
    }

    return PASS;
}

int t_ak8975_Selftest(int argc, char **argv)
{
    u16 Magnetic[3] = {0, 0, 0};
    u32 Timeout = 1000;

    ak8975_EnterPowerdownMode();
    ak8975_SelfTestControl(true);
    ak8975_EnterSelftestMode();

    do {
        if (ak8975_DataReady() == true)
            break;
        else
            mdelay(1);
    } while (Timeout--)

        if (Timeout <= 0) {
            printk(KERN_DEBUG "[Error] ak8975 Magnetic Data not ready");
            return FAIL;
        }

//  Timeout_WaitMSDone(&ak8975_DR_Flag, true, 1000);

    ak8975_MagneticMeasurements(Magnetic);
    ak8975_SelfTestControl(false);
    printk(KERN_DEBUG "ak8975 Selftest Magnetic Data %x, %x, %x", Magnetic[0], Magnetic[1], Magnetic[2]);

    if (Magnetic[0] <= AK8975_TBD && Magnetic[1] <= AK8975_TBD && Magnetic[2] >= AK8975_TBD)
        printk(KERN_DEBUG "AK8975/B is working normally");
    else
        return FAIL;

    return PASS;
}

int t_ak8975_FuseROMAccess(int argc, char **argv)
{
    u8 Magnetic[3] = {0, 0, 0};

    ak8975_EnterFuseROMAccessMode();
    ak8975_SensitivityAdjustmentValues(Magnetic);
    printk(KERN_DEBUG "ak8975 Adjustment Values %x, %x, %x", Magnetic[0], Magnetic[1], Magnetic[2]);
    ak8975_EnterPowerdownMode();

    return PASS;
}

#endif

