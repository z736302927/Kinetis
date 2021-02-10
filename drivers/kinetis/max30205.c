#include "max30205/max30205.h"

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

#define max30205_printf                  p_dbg

#define MAX30205_ADDR                    0x00

void max30205_Delayus(u32 ticks)
{
    udelay(ticks);
}

void max30205_Delayms(u32 ticks)
{
    mdelay(ticks);
}

void max30205_PortTransmmit(u8 Addr, u8 Data)
{
    IIC_Soft_WriteSingleByteWithAddr(MAX30205_ADDR, Addr, Data);
}

void max30205_PortReceive(u8 Addr, u8 *pData)
{
    IIC_Soft_ReadSingleByteWithAddr(MAX30205_ADDR, Addr, pData);
}

void max30205_port_multi_transmmit(u8 Addr, u8 *pData, u32 Length)
{
    IIC_Soft_WriteMultiByteWithAddr(MAX30205_ADDR, Addr, pData, Length);
}

void max30205_port_multi_receive(u8 Addr, u8 *pData, u32 Length)
{
    IIC_Soft_ReadMultiByteWithAddr(MAX30205_ADDR, Addr, pData, Length);
}

/* The above procedure is modified by the user according to the hardware device, otherwise the driver cannot run. */

#define TEMPERATURE                     0x00
#define CONFIGRATION                    0x01
#define THYST                           0x02
#define TOS                             0x03

void max30205_TempMeasurement(u16 *pData)
{
    u8 TmpVal[2];

    max30205_port_multi_receive(TEMPERATURE, TmpVal, 2);

    pData[0] = (TmpVal[0] << 8) | TmpVal[1];
}

void max30205_GetTemperature(float *pData)
{
    u16 TmpVal;

    max30205_TempMeasurement(&TmpVal);

    pData[0] = (float)TmpVal * 0.00390625;
}

void max30205_ShutDown(u8 Data)
{
    u8 TmpReg = 0;

    max30205_PortReceive(CONFIGRATION, &TmpReg);

    Data &= 0x01;
    TmpReg &= ~(0x01 << 0);
    TmpReg |= (Data << 0);

    max30205_PortTransmmit(CONFIGRATION, TmpReg);
}

void max30205_EnterComparatorMode(void)
{
    u8 TmpReg = 0;

    max30205_PortReceive(CONFIGRATION, &TmpReg);

    TmpReg &= ~(0x01 << 1);

    max30205_PortTransmmit(CONFIGRATION, TmpReg);
}

void max30205_EnterInterruptMode(void)
{
    u8 TmpReg = 0;

    max30205_PortReceive(CONFIGRATION, &TmpReg);

    TmpReg &= ~(0x01 << 1);
    TmpReg |= (1 << 1);

    max30205_PortTransmmit(CONFIGRATION, TmpReg);
}

void max30205_OSPolarity(u8 Data)
{
    u8 TmpReg = 0;

    max30205_PortReceive(CONFIGRATION, &TmpReg);

    Data &= 0x01;
    TmpReg &= ~(0x01 << 2);
    TmpReg |= (Data << 2);

    max30205_PortTransmmit(CONFIGRATION, TmpReg);
}

void max30205_ConfigFaultQueue(u8 Data)
{
    u8 TmpReg = 0;

    max30205_PortReceive(CONFIGRATION, &TmpReg);

    Data &= 0x03;
    TmpReg &= ~(0x03 << 3);
    TmpReg |= (Data << 3);

    max30205_PortTransmmit(CONFIGRATION, TmpReg);
}

void max30205_DataFormat(u8 Data)
{
    u8 TmpReg = 0;

    max30205_PortReceive(CONFIGRATION, &TmpReg);

    Data &= 0x01;
    TmpReg &= ~(0x01 << 5);
    TmpReg |= (Data << 5);

    max30205_PortTransmmit(CONFIGRATION, TmpReg);
}

void max30205_EnableTimeout(u8 Data)
{
    u8 TmpReg = 0;

    max30205_PortReceive(CONFIGRATION, &TmpReg);

    Data &= 0x01;
    TmpReg &= ~(0x01 << 6);
    TmpReg |= (Data << 6);

    max30205_PortTransmmit(CONFIGRATION, TmpReg);
}

void max30205_OneShot(u8 Data)
{
    u8 TmpReg = 0;

    max30205_PortReceive(CONFIGRATION, &TmpReg);

    Data &= 0x01;
    TmpReg &= ~(0x01 << 7);
    TmpReg |= (Data << 7);

    max30205_PortTransmmit(CONFIGRATION, TmpReg);
}

void max30205_ReadTHYST(u16 *pData)
{
    u8 TmpVal[2];

    max30205_port_multi_receive(THYST, TmpVal, 2);

    pData[0] = (TmpVal[0] << 8) | TmpVal[1];
}

void max30205_WriteTHYST(u16 Data)
{
    u8 TmpVal[2];

    TmpVal[0] = Data >> 8;
    TmpVal[1] = Data & 0xFF;

    max30205_port_multi_transmmit(THYST, TmpVal, 2);
}

void max30205_ReadTOS(u16 *pData)
{
    u8 TmpVal[2];

    max30205_port_multi_receive(TOS, TmpVal, 2);

    pData[0] = (TmpVal[0] << 8) | TmpVal[1];
}

void max30205_WriteTOS(u16 Data)
{
    u8 TmpVal[2];

    TmpVal[0] = Data >> 8;
    TmpVal[1] = Data & 0xFF;

    max30205_port_multi_transmmit(TOS, TmpVal, 2);
}

#ifdef DESIGN_VERIFICATION_MAX30205
static u8 tx_buffer[256];
static u8 rx_buffer[256];

void max30205_Test(void)
{
    u32 TmpRngdata = 0;
    u16 BufferLength = 0;
    u32 TestAddr = 0;

    Random_Get8bit(&hrng, &TmpRngdata);
    BufferLength = TmpRngdata & 0xFF;
    max30205_printf("BufferLength = %d.", BufferLength);

    if (tx_buffer == NULL || rx_buffer == NULL) {
        max30205_printf("Failed to allocate memory !");
        return;
    }

    memset(tx_buffer, 0, BufferLength);
    memset(rx_buffer, 0, BufferLength);

    Random_Get8bit(&hrng, &TmpRngdata);
    TestAddr = TmpRngdata & 0xFF;
    max30205_printf("TestAddr = 0x%02X.", TestAddr);

    for (u16 i = 0; i < BufferLength; i += 4) {
        Random_Get8bit(&hrng, &TmpRngdata);
        tx_buffer[i + 3] = (TmpRngdata & 0xFF000000) >> 24;;
        tx_buffer[i + 2] = (TmpRngdata & 0x00FF0000) >> 16;
        tx_buffer[i + 1] = (TmpRngdata & 0x0000FF00) >> 8;
        tx_buffer[i + 0] = (TmpRngdata & 0x000000FF);
    }

    max30205_WriteData(TestAddr, tx_buffer, BufferLength);
    max30205_ReadData(TestAddr, rx_buffer, BufferLength);

    for (u16 i = 0; i < BufferLength; i++) {
        if (tx_buffer[i] != rx_buffer[i]) {
            max30205_printf("tx_buffer[%d] = 0x%02X, rx_buffer[%d] = 0x%02X",
                i, tx_buffer[i],
                i, rx_buffer[i]);
            max30205_printf("Data writes and reads do not match, TEST FAILED !");
            return ;
        }
    }

    max30205_printf("max30205 Read and write TEST PASSED !");
}

#endif



