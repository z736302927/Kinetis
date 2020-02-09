#include "max30205/max30205.h"

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
#include "idebug/idebug.h"

#define max30205_printf                  p_dbg

#define MAX30205_ADDR                    0x00

void max30205_Delayus(uint32_t ticks)
{
  HAL_Delay(ticks);
}

void max30205_Delayms(uint32_t ticks)
{
  HAL_Delay(ticks);
}

void max30205_PortTransmmit(uint8_t Addr, uint8_t Data)
{
  IIC_Soft_WriteSingleByteWithAddr(MAX30205_ADDR, Addr, Data);
}

void max30205_PortReceive(uint8_t Addr, uint8_t *pData)
{
  IIC_Soft_ReadSingleByteWithAddr(MAX30205_ADDR, Addr, pData);
}

void max30205_PortMultiTransmmit(uint8_t Addr, uint8_t *pData, uint32_t Length)
{
  IIC_Soft_WriteMultiByteWithAddr(MAX30205_ADDR, Addr, pData, Length);
}

void max30205_PortMultiReceive(uint8_t Addr, uint8_t *pData, uint32_t Length)
{
  IIC_Soft_ReadMultiByteWithAddr(MAX30205_ADDR, Addr, pData, Length);
}

/*The above procedure is modified by the user according to the hardware device, otherwise the driver cannot run.*/

#define TEMPERATURE                     0x00
#define CONFIGRATION                    0x01
#define THYST                           0x02
#define TOS                             0x03

void max30205_TempMeasurement(uint16_t *pData)
{
  uint8_t TmpVal[2];
  
  max30205_PortMultiReceive(TEMPERATURE, TmpVal, 2);
  
  pData[0] = (TmpVal[0] << 8) | TmpVal[1];
}

void max30205_GetTemperature(float *pData)
{
  uint16_t TmpVal;
  
  max30205_TempMeasurement(&TmpVal);
  
  pData[0] = (float)TmpVal * 0.00390625;
}

void max30205_ShutDown(uint8_t Data)
{
  uint8_t TmpReg = 0;
  
  max30205_PortReceive(CONFIGRATION, &TmpReg);
  
  Data &= 0x01;
  TmpReg &= ~(0x01 << 0);
  TmpReg |= (Data << 0);
  
  max30205_PortTransmmit(CONFIGRATION, TmpReg);
}

void max30205_EnterComparatorMode(void)
{
  uint8_t TmpReg = 0;
  
  max30205_PortReceive(CONFIGRATION, &TmpReg);
  
  TmpReg &= ~(0x01 << 1);
  
  max30205_PortTransmmit(CONFIGRATION, TmpReg);
}

void max30205_EnterInterruptMode(void)
{
  uint8_t TmpReg = 0;
  
  max30205_PortReceive(CONFIGRATION, &TmpReg);
  
  TmpReg &= ~(0x01 << 1);
  TmpReg |= (1 << 1);
  
  max30205_PortTransmmit(CONFIGRATION, TmpReg);
}

void max30205_OSPolarity(uint8_t Data)
{
  uint8_t TmpReg = 0;
  
  max30205_PortReceive(CONFIGRATION, &TmpReg);
  
  Data &= 0x01;
  TmpReg &= ~(0x01 << 2);
  TmpReg |= (Data << 2);
  
  max30205_PortTransmmit(CONFIGRATION, TmpReg);
}

void max30205_ConfigFaultQueue(uint8_t Data)
{
  uint8_t TmpReg = 0;
  
  max30205_PortReceive(CONFIGRATION, &TmpReg);
  
  Data &= 0x03;
  TmpReg &= ~(0x03 << 3);
  TmpReg |= (Data << 3);
  
  max30205_PortTransmmit(CONFIGRATION, TmpReg);
}

void max30205_DataFormat(uint8_t Data)
{
  uint8_t TmpReg = 0;
  
  max30205_PortReceive(CONFIGRATION, &TmpReg);
  
  Data &= 0x01;
  TmpReg &= ~(0x01 << 5);
  TmpReg |= (Data << 5);
  
  max30205_PortTransmmit(CONFIGRATION, TmpReg);
}

void max30205_EnableTimeout(uint8_t Data)
{
  uint8_t TmpReg = 0;
  
  max30205_PortReceive(CONFIGRATION, &TmpReg);
  
  Data &= 0x01;
  TmpReg &= ~(0x01 << 6);
  TmpReg |= (Data << 6);
  
  max30205_PortTransmmit(CONFIGRATION, TmpReg);
}

void max30205_OneShot(uint8_t Data)
{
  uint8_t TmpReg = 0;
  
  max30205_PortReceive(CONFIGRATION, &TmpReg);
  
  Data &= 0x01;
  TmpReg &= ~(0x01 << 7);
  TmpReg |= (Data << 7);
  
  max30205_PortTransmmit(CONFIGRATION, TmpReg);
}

void max30205_ReadTHYST(uint16_t *pData)
{
  uint8_t TmpVal[2];
  
  max30205_PortMultiReceive(THYST, TmpVal, 2);
  
  pData[0] = (TmpVal[0] << 8) | TmpVal[1];
}

void max30205_WriteTHYST(uint16_t Data)
{
  uint8_t TmpVal[2];
  
  TmpVal[0] = Data >> 8;
  TmpVal[1] = Data & 0xFF;
  
  max30205_PortMultiTransmmit(THYST, TmpVal, 2);
}

void max30205_ReadTOS(uint16_t *pData)
{
  uint8_t TmpVal[2];
  
  max30205_PortMultiReceive(TOS, TmpVal, 2);
  
  pData[0] = (TmpVal[0] << 8) | TmpVal[1];
}

void max30205_WriteTOS(uint16_t Data)
{
  uint8_t TmpVal[2];
  
  TmpVal[0] = Data >> 8;
  TmpVal[1] = Data & 0xFF;
  
  max30205_PortMultiTransmmit(TOS, TmpVal, 2);
}

#if 0

static uint8_t Tx_Buffer[256];
static uint8_t Rx_Buffer[256];

void max30205_Test(void)
{
  uint32_t TmpRngdata = 0;
  uint16_t BufferLength = 0;
  uint32_t TestAddr = 0;
  
  HAL_RNG_GenerateRandomNumber(&hrng, &TmpRngdata);
  BufferLength = TmpRngdata & 0xFF;
  max30205_printf("BufferLength = %d.", BufferLength);
  
  if(Tx_Buffer == NULL || Rx_Buffer == NULL)
  {
    max30205_printf("Failed to allocate memory !");
    return;
  }
  memset(Tx_Buffer, 0, BufferLength);
  memset(Rx_Buffer, 0, BufferLength);
  
  HAL_RNG_GenerateRandomNumber(&hrng, &TmpRngdata);
  TestAddr = TmpRngdata & 0xFF;
  max30205_printf("TestAddr = 0x%02X.", TestAddr);

  for(uint16_t i = 0;i < BufferLength;i += 4)
  {
    HAL_RNG_GenerateRandomNumber(&hrng, &TmpRngdata);
    Tx_Buffer[i + 3] = (TmpRngdata & 0xFF000000) >> 24;;
    Tx_Buffer[i + 2] = (TmpRngdata & 0x00FF0000) >> 16;
    Tx_Buffer[i + 1] = (TmpRngdata & 0x0000FF00) >> 8;
    Tx_Buffer[i + 0] = (TmpRngdata & 0x000000FF);
  }
  
  max30205_WriteData(TestAddr, Tx_Buffer, BufferLength);
  max30205_ReadData(TestAddr, Rx_Buffer, BufferLength);
  
  for(uint16_t i = 0;i < BufferLength;i++)
  {
    if(Tx_Buffer[i] != Rx_Buffer[i])
    {
      max30205_printf("Tx_Buffer[%d] = 0x%02X, Rx_Buffer[%d] = 0x%02X", 
                     i, Tx_Buffer[i],
                     i, Rx_Buffer[i]);
      max30205_printf("Data writes and reads do not match, TEST FAILED !");
      return ;
    }
  }
  
  max30205_printf("max30205 Read and write TEST PASSED !");
}

#endif



