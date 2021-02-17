#include "hmc5883l/hmc5883l.h"

/* The following program is modified by the user according to the hardware device, otherwise the driver cannot run. */

/**
  * @step 1:  Modify the corresponding function according to the modified area and the corresponding function name.
  * @step 2:  Modify four areas: GPIO_PORT/GPIO_PIN/Led_TypeDef/LEDn.
  * @step 3:  .
  * @step 4:  .
  * @step 5:
  */

#include "kinetis/iic_soft.h"

#define DEBUG
#include "kinetis/idebug.h"

#define hmc5883l_printf                  p_dbg

#define HMC5883L_ADDR                    0x00

void hmc5883l_Delayus(u32 ticks)
{
    udelay(ticks);
}

void hmc5883l_Delayms(u32 ticks)
{
    mdelay(ticks);
}

void hmc5883l_port_transmmit(u8 addr, u8 Data)
{
    IIC_Soft_WriteSingleByteWithAddr(HMC5883L_ADDR, addr, Data);
}

void hmc5883l_port_receive(u8 addr, u8 *pdata)
{
    IIC_Soft_ReadSingleByteWithAddr(HMC5883L_ADDR, addr, pdata);
}

void hmc5883l_port_multi_transmmit(u8 addr, u8 *pdata, u32 Length)
{
    IIC_Soft_WriteMultiByteWithAddr(HMC5883L_ADDR, addr, pdata, Length);
}

void hmc5883l_port_multi_receive(u8 addr, u8 *pdata, u32 Length)
{
    IIC_Soft_ReadMultiByteWithAddr(HMC5883L_ADDR, addr, pdata, Length);
}

/* The above procedure is modified by the user according to the hardware device, otherwise the driver cannot run. */

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
    u8 TmpReg = 0;

    hmc5883l_port_receive(CRA, &TmpReg);

    TmpReg &= ~(0x01 << 7);

    hmc5883l_port_transmmit(CRA, TmpReg);
}

void hmc5883l_SelectSamplesAveraged(u8 Data)
{
    u8 TmpReg = 0;

    hmc5883l_port_receive(CRA, &TmpReg);

    Data &= 0x03;
    TmpReg &= ~(0x03 << 5);
    TmpReg |= (Data << 5);

    hmc5883l_port_transmmit(CRA, TmpReg);
}

void hmc5883l_DataOutputRate(u8 Data)
{
    u8 TmpReg = 0;

    hmc5883l_port_receive(CRA, &TmpReg);

    Data &= 0x07;
    TmpReg &= ~(0x07 << 2);
    TmpReg |= (Data << 2);

    hmc5883l_port_transmmit(CRA, TmpReg);
}

void hmc5883l_MeasurementConfiguration(u8 Data)
{
    u8 TmpReg = 0;

    hmc5883l_port_receive(CRA, &TmpReg);

    Data &= 0x03;
    TmpReg &= ~(0x03 << 0);
    TmpReg |= (Data << 0);

    hmc5883l_port_transmmit(CRA, TmpReg);
}

void hmc5883l_GainConfiguration(u8 Data)
{
    u8 TmpReg = 0;

    hmc5883l_port_receive(CRB, &TmpReg);

    Data &= 0x07;
    TmpReg &= ~(0x07 << 5);
    TmpReg |= (Data << 5);

    hmc5883l_port_transmmit(CRB, TmpReg);
}

void hmc5883l_ClearMR7(void)
{
    u8 TmpReg = 0;

    hmc5883l_port_receive(MR, &TmpReg);

    TmpReg &= ~(0x01 << 7);

    hmc5883l_port_transmmit(MR, TmpReg);
}

void hmc5883l_ModeSelect(u8 Data)
{
    u8 TmpReg = 0;

    hmc5883l_port_receive(MR, &TmpReg);

    Data &= 0x03;
    TmpReg &= ~(0x03 << 0);
    TmpReg |= (Data << 0);

    hmc5883l_port_transmmit(MR, TmpReg);
}

void hmc5883l_MagneticMeasurements(u16 *pdata)
{
    u8 TmpVal[6];

    hmc5883l_port_multi_receive(DXRA, TmpVal, 6);

    pdata[0] = (TmpVal[0] << 8) | TmpVal[1];
    pdata[1] = (TmpVal[2] << 8) | TmpVal[3];
    pdata[2] = (TmpVal[4] << 8) | TmpVal[5];
}

u8 hmc5883l_DataLock(void)
{
    u8 TmpReg = 0;

    hmc5883l_port_receive(SR, &TmpReg);

    if (TmpReg & 0x02)
        return 1;
    else
        return 0;
}

u8 hmc5883l_DataReady(void)
{
    u8 TmpReg = 0;

    hmc5883l_port_receive(SR, &TmpReg);

    if (TmpReg & 0x01)
        return 1;
    else
        return 0;
}

void hmc5883l_Identification(u8 *pdata)
{
    hmc5883l_port_multi_receive(IRA, pdata, 3);
}

#ifdef DESIGN_VERIFICATION_HMC5883L
#include "kinetis/test-kinetis.h"

int t_hmc5883l_basic_info(int argc, char **argv)
{
    u8 Data = 0;

    hmc5883l_WhoAmI(&Data);
    printk(KERN_DEBUG "Device ID of AKM8975 is 0x%x", Data);

    if (Data != AKM_DEVID)
        return PASS;
    else {
        printk(KERN_DEBUG "Device ID of AKM8975 is not correct");
        return FAIL;
    }

    hmc5883l_DeviceInformation(&Data);
    printk(KERN_DEBUG "Device information for AKM8975 %x", Data);

    if (Data != 0x00)
        return PASS;
    else
        return FAIL;
}

int t_hmc5883l_Magnetic(int argc, char **argv)
{
    u16 Magnetic[3] = {0, 0, 0};
    u16 times = 128;
    u8 i = 0;
    u32 Timeout = 1000;

    if (argc > 1)
        times = strtoul(argv[1], &argv[1], 10);

    hmc5883l_EnterPowerdownMode();

    for (i = 0; i < times; i++) {
        hmc5883l_EnterSingleMeasurementMode();

        do {
            if (hmc5883l_DataReady() == true)
                break;
            else
                mdelay(1);
        } while (Timeout--)

            if (Timeout <= 0) {
                printk(KERN_DEBUG "[Error] hmc5883l Magnetic Data not ready");
                return FAIL;
            }

//    Timeout_WaitMSDone(&hmc5883l_DR_Flag, true, 1000);

        if (hmc5883l_DataOverrun() == true)
            printk(KERN_DEBUG "[Warning] hmc5883l Magnetic Data Overrun");

        hmc5883l_MagneticMeasurements(Magnetic);
        printk(KERN_DEBUG "hmc5883l Magnetic Data %x, %x, %x", Magnetic[0], Magnetic[1], Magnetic[2]);
    }

    return PASS;
}

int t_hmc5883l_Selftest(int argc, char **argv)
{
    u16 Magnetic[3] = {0, 0, 0};
    u32 Timeout = 1000;

    hmc5883l_EnterPowerdownMode();
    hmc5883l_selftestControl(true);
    hmc5883l_EnterSelftestMode();

    do {
        if (hmc5883l_DataReady() == true)
            break;
        else
            mdelay(1);
    } while (Timeout--)

        if (Timeout <= 0) {
            printk(KERN_DEBUG "[Error] hmc5883l Magnetic Data not ready");
            return FAIL;
        }

//  Timeout_WaitMSDone(&hmc5883l_DR_Flag, true, 1000);

    hmc5883l_MagneticMeasurements(Magnetic);
    hmc5883l_selftestControl(false);
    printk(KERN_DEBUG "hmc5883l Selftest Magnetic Data %x, %x, %x", Magnetic[0], Magnetic[1], Magnetic[2]);

    if (Magnetic[0] <= AK8975_TBD && Magnetic[1] <= AK8975_TBD && Magnetic[2] >= AK8975_TBD)
        printk(KERN_DEBUG "AK8975/B is working normally");
    else
        return FAIL;

    return PASS;
}

int t_hmc5883l_FuseROMAccess(int argc, char **argv)
{
    u8 Magnetic[3] = {0, 0, 0};

    hmc5883l_EnterFuseROMAccessMode();
    hmc5883l_SensitivityAdjustmentValues(Magnetic);
    printk(KERN_DEBUG "hmc5883l Adjustment Values %x, %x, %x", Magnetic[0], Magnetic[1], Magnetic[2]);
    hmc5883l_EnterPowerdownMode();

    return PASS;
}

#endif




