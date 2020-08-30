#include "hmc5883l/hmc5883l.h"

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

#define hmc5883l_printf                  p_dbg

#define HMC5883L_ADDR                    0x00

void hmc5883l_Delayus(uint32_t ticks)
{
    Delay_us(ticks);
}

void hmc5883l_Delayms(uint32_t ticks)
{
    Delay_ms(ticks);
}

void hmc5883l_PortTransmmit(uint8_t Addr, uint8_t Data)
{
    IIC_Soft_WriteSingleByteWithAddr(HMC5883L_ADDR, Addr, Data);
}

void hmc5883l_PortReceive(uint8_t Addr, uint8_t *pData)
{
    IIC_Soft_ReadSingleByteWithAddr(HMC5883L_ADDR, Addr, pData);
}

void hmc5883l_PortMultiTransmmit(uint8_t Addr, uint8_t *pData, uint32_t Length)
{
    IIC_Soft_WriteMultiByteWithAddr(HMC5883L_ADDR, Addr, pData, Length);
}

void hmc5883l_PortMultiReceive(uint8_t Addr, uint8_t *pData, uint32_t Length)
{
    IIC_Soft_ReadMultiByteWithAddr(HMC5883L_ADDR, Addr, pData, Length);
}

/*The above procedure is modified by the user according to the hardware device, otherwise the driver cannot run.*/

#define CRA                             0x00
#define CRB                             0x01
#define MR                              0x02
#define DXRA                            0x03
#define DXRB                            0x04
#define DYRA                            0x05
#define DYRB                            0x06
#define DZRA                            0x07
#define DZRB                            0x08
#define SR                              0x09
#define IRA                             0x10
#define IRB                             0x11
#define IRC                             0x12

void hmc5883l_ClearCRA7(void)
{
    uint8_t TmpReg = 0;

    hmc5883l_PortReceive(CRA, &TmpReg);

    TmpReg &= ~(0x01 << 7);

    hmc5883l_PortTransmmit(CRA, TmpReg);
}

void hmc5883l_SelectSamplesAveraged(uint8_t Data)
{
    uint8_t TmpReg = 0;

    hmc5883l_PortReceive(CRA, &TmpReg);

    Data &= 0x03;
    TmpReg &= ~(0x03 << 5);
    TmpReg |= (Data << 5);

    hmc5883l_PortTransmmit(CRA, TmpReg);
}

void hmc5883l_DataOutputRate(uint8_t Data)
{
    uint8_t TmpReg = 0;

    hmc5883l_PortReceive(CRA, &TmpReg);

    Data &= 0x07;
    TmpReg &= ~(0x07 << 2);
    TmpReg |= (Data << 2);

    hmc5883l_PortTransmmit(CRA, TmpReg);
}

void hmc5883l_MeasurementConfiguration(uint8_t Data)
{
    uint8_t TmpReg = 0;

    hmc5883l_PortReceive(CRA, &TmpReg);

    Data &= 0x03;
    TmpReg &= ~(0x03 << 0);
    TmpReg |= (Data << 0);

    hmc5883l_PortTransmmit(CRA, TmpReg);
}

void hmc5883l_GainConfiguration(uint8_t Data)
{
    uint8_t TmpReg = 0;

    hmc5883l_PortReceive(CRB, &TmpReg);

    Data &= 0x07;
    TmpReg &= ~(0x07 << 5);
    TmpReg |= (Data << 5);

    hmc5883l_PortTransmmit(CRB, TmpReg);
}

void hmc5883l_ClearMR7(void)
{
    uint8_t TmpReg = 0;

    hmc5883l_PortReceive(MR, &TmpReg);

    TmpReg &= ~(0x01 << 7);

    hmc5883l_PortTransmmit(MR, TmpReg);
}

void hmc5883l_ModeSelect(uint8_t Data)
{
    uint8_t TmpReg = 0;

    hmc5883l_PortReceive(MR, &TmpReg);

    Data &= 0x03;
    TmpReg &= ~(0x03 << 0);
    TmpReg |= (Data << 0);

    hmc5883l_PortTransmmit(MR, TmpReg);
}

void hmc5883l_MagneticMeasurements(uint16_t *pData)
{
    uint8_t TmpVal[6];

    hmc5883l_PortMultiReceive(DXRA, TmpVal, 6);

    pData[0] = (TmpVal[0] << 8) | TmpVal[1];
    pData[1] = (TmpVal[2] << 8) | TmpVal[3];
    pData[2] = (TmpVal[4] << 8) | TmpVal[5];
}

uint8_t hmc5883l_DataLock(void)
{
    uint8_t TmpReg = 0;

    hmc5883l_PortReceive(SR, &TmpReg);

    if(TmpReg & 0x02)
        return 1;
    else
        return 0;
}

uint8_t hmc5883l_DataReady(void)
{
    uint8_t TmpReg = 0;

    hmc5883l_PortReceive(SR, &TmpReg);

    if(TmpReg & 0x01)
        return 1;
    else
        return 0;
}

void hmc5883l_Identification(uint8_t *pData)
{
    hmc5883l_PortMultiReceive(IRA, pData, 3);
}

#ifdef DESIGN_VERIFICATION_HMC5883L
#include "dv/k-test.h"

int t_hmc5883l_BasicInfo(int argc, char **argv)
{
    uint8_t Data = 0;

    hmc5883l_WhoAmI(&Data);
    kinetis_debug_trace(KERN_DEBUG, "Device ID of AKM8975 is 0x%x", Data);

    if(Data != AKM_DEVID)
        return PASS;
    else
    {
        kinetis_debug_trace(KERN_DEBUG, "Device ID of AKM8975 is not correct");
        return FAIL;
    }

    hmc5883l_DeviceInformation(&Data);
    kinetis_debug_trace(KERN_DEBUG, "Device information for AKM8975 %x", Data);

    if(Data != 0x00)
        return PASS;
    else
        return FAIL;
}

int t_hmc5883l_Magnetic(int argc, char **argv)
{
    uint16_t Magnetic[3] = {0, 0, 0};
    uint16_t times = 128;
    uint8_t i = 0;
    uint32_t Timeout = 1000;

    if(argc > 1)
        times = strtoul(argv[1], &argv[1], 10);

    hmc5883l_EnterPowerdownMode();

    for(i = 0; i < times; i++)
    {
        hmc5883l_EnterSingleMeasurementMode();

        do
        {
            if(hmc5883l_DataReady() == true)
                break;
            else
                Delay_ms(1);
        }
        while(Timeout--)

            if(Timeout <= 0)
            {
                kinetis_debug_trace(KERN_DEBUG, "[Error] hmc5883l Magnetic Data not ready");
                return FAIL;
            }

//    Timeout_WaitMSDone(&hmc5883l_DR_Flag, true, 1000);

        if(hmc5883l_DataOverrun() == true)
            kinetis_debug_trace(KERN_DEBUG, "[Warning] hmc5883l Magnetic Data Overrun");

        hmc5883l_MagneticMeasurements(Magnetic);
        kinetis_debug_trace(KERN_DEBUG, "hmc5883l Magnetic Data %x, %x, %x", Magnetic[0], Magnetic[1], Magnetic[2]);
    }

    return PASS;
}

int t_hmc5883l_Selftest(int argc, char **argv)
{
    uint16_t Magnetic[3] = {0, 0, 0};
    uint32_t Timeout = 1000;

    hmc5883l_EnterPowerdownMode();
    hmc5883l_SelfTestControl(true);
    hmc5883l_EnterSelftestMode();

    do
    {
        if(hmc5883l_DataReady() == true)
            break;
        else
            Delay_ms(1);
    }
    while(Timeout--)

        if(Timeout <= 0)
        {
            kinetis_debug_trace(KERN_DEBUG, "[Error] hmc5883l Magnetic Data not ready");
            return FAIL;
        }

//  Timeout_WaitMSDone(&hmc5883l_DR_Flag, true, 1000);

    hmc5883l_MagneticMeasurements(Magnetic);
    hmc5883l_SelfTestControl(false);
    kinetis_debug_trace(KERN_DEBUG, "hmc5883l Selftest Magnetic Data %x, %x, %x", Magnetic[0], Magnetic[1], Magnetic[2]);

    if(Magnetic[0] <= AK8975_TBD && Magnetic[1] <= AK8975_TBD && Magnetic[2] >= AK8975_TBD)
        kinetis_debug_trace(KERN_DEBUG, "AK8975/B is working normally");
    else
        return FAIL;

    return PASS;
}

int t_hmc5883l_FuseROMAccess(int argc, char **argv)
{
    uint8_t Magnetic[3] = {0, 0, 0};

    hmc5883l_EnterFuseROMAccessMode();
    hmc5883l_SensitivityAdjustmentValues(Magnetic);
    kinetis_debug_trace(KERN_DEBUG, "hmc5883l Adjustment Values %x, %x, %x", Magnetic[0], Magnetic[1], Magnetic[2]);
    hmc5883l_EnterPowerdownMode();

    return PASS;
}

#endif




