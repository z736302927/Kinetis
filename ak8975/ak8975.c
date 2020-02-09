#include "ak8975/ak8975.h"

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

#define ak8975_printf                  p_dbg

#define AK8975_ADDR                    0x00

void ak8975_Delayus(uint32_t ticks)
{
  HAL_Delay(ticks);
}

void ak8975_Delayms(uint32_t ticks)
{
  HAL_Delay(ticks);
}

void ak8975_PortTransmmit(uint8_t Addr, uint8_t Data)
{
  IIC_Soft_WriteSingleByteWithAddr(AK8975_ADDR, Addr, Data);
}

void ak8975_PortReceive(uint8_t Addr, uint8_t *pData)
{
  IIC_Soft_ReadSingleByteWithAddr(AK8975_ADDR, Addr, pData);
}

void ak8975_PortMultiTransmmit(uint8_t Addr, uint8_t *pData, uint32_t Length)
{
  IIC_Soft_WriteMultiByteWithAddr(AK8975_ADDR, Addr, pData, Length);
}

void ak8975_PortMultiReceive(uint8_t Addr, uint8_t *pData, uint32_t Length)
{
  IIC_Soft_ReadMultiByteWithAddr(AK8975_ADDR, Addr, pData, Length);
}

/*The above procedure is modified by the user according to the hardware device, otherwise the driver cannot run.*/

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


void ak8975_WhoAmI(uint8_t *pData)
{
  ak8975_PortReceive(WIA, pData);
}

void ak8975_DeviceInformation(uint8_t *pData)
{
  ak8975_PortReceive(INFO, pData);
}

uint8_t ak8975_DataReady(void)
{
  uint8_t TmpReg = 0;
  
  ak8975_PortReceive(ST1, &TmpReg);
  
  if(TmpReg & 0x01)
  {
    return 1;
  }
  else
  {
    return 0;
  }
}

uint8_t ak8975_DataOverrun(void)
{
  uint8_t TmpReg = 0;
  
  ak8975_PortReceive(ST1, &TmpReg);
  
  if(TmpReg & 0x02)
  {
    return 1;
  }
  else
  {
    return 0;
  }
}

void ak8975_MagneticMeasurements(uint16_t *pData)
{
  uint8_t TmpVal[6];
  
  ak8975_PortMultiReceive(HXL, TmpVal, 6);
  
  pData[0] = (TmpVal[1] << 8) | TmpVal[0];
  pData[1] = (TmpVal[3] << 8) | TmpVal[2];
  pData[2] = (TmpVal[5] << 8) | TmpVal[4];
}

uint8_t ak8975_MagneticSensorOverflow(void)
{
  uint8_t TmpReg = 0;
  
  ak8975_PortReceive(ST2, &TmpReg);
  
  if(TmpReg & 0x08)
  {
    return 1;
  }
  else
  {
    return 0;
  }
}

uint8_t ak8975_OutputBitSettingMirror(void)
{
  uint8_t TmpReg = 0;
  
  ak8975_PortReceive(ST2, &TmpReg);
  
  if(TmpReg & 0x10)
  {
    return 1;
  }
  else
  {
    return 0;
  }
}

void ak8975_OperationModeSetting(uint8_t Data)
{
  uint8_t TmpReg = 0;
  
  ak8975_PortReceive(CNTL1, &TmpReg);
  
  Data &= 0x0F;
  TmpReg &= ~(0x0F << 0);
  TmpReg |= (Data << 0);
  
  ak8975_PortTransmmit(CNTL1, TmpReg);
}

void ak8975_OutputBitSetting(uint8_t Data)
{
  uint8_t TmpReg = 0;
  
  ak8975_PortReceive(CNTL1, &TmpReg);
  
  Data &= 0x01;
  TmpReg &= ~(0x01 << 4);
  TmpReg |= (Data << 4);
  
  ak8975_PortTransmmit(CNTL1, TmpReg);
}

void ak8975_SoftReset(uint8_t Data)
{
  uint8_t TmpReg = 0;
  
  ak8975_PortReceive(CNTL2, &TmpReg);
  
  Data &= 0x01;
  TmpReg &= ~(0x01 << 0);
  TmpReg |= (Data << 0);
  
  ak8975_PortTransmmit(CNTL2, TmpReg);
}

void ak8975_SelfTestControl(uint8_t Data)
{
  uint8_t TmpReg = 0;
  
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

void ak8975_SensitivityAdjustmentValues(uint8_t *pData)
{
  ak8975_PortMultiReceive(ASAX, pData, 3);
}

void ak8975_MagneticAdjustedMeasurements(uint16_t *pData)
{
  uint16_t RawData[3];
  uint8_t Factor[3];
  
  ak8975_MagneticMeasurements(RawData);
  ak8975_SensitivityAdjustmentValues(Factor);
  
  pData[0] = RawData[0] * ((Factor[0] - 128) / 256 + 1);
  pData[1] = RawData[1] * ((Factor[1] - 128) / 256 + 1);
  pData[2] = RawData[2] * ((Factor[2] - 128) / 256 + 1);
}

#if 0

static uint8_t Tx_Buffer[256];
static uint8_t Rx_Buffer[256];

void mpu6050_Test(void)
{
  uint32_t TmpRngdata = 0;
  uint16_t BufferLength = 0;
  uint32_t TestAddr = 0;
  
  HAL_RNG_GenerateRandomNumber(&hrng, &TmpRngdata);
  BufferLength = TmpRngdata & 0xFF;
  mpu6050_printf("BufferLength = %d.", BufferLength);
  
  if(Tx_Buffer == NULL || Rx_Buffer == NULL)
  {
    mpu6050_printf("Failed to allocate memory !");
    return;
  }
  memset(Tx_Buffer, 0, BufferLength);
  memset(Rx_Buffer, 0, BufferLength);
  
  HAL_RNG_GenerateRandomNumber(&hrng, &TmpRngdata);
  TestAddr = TmpRngdata & 0xFF;
  mpu6050_printf("TestAddr = 0x%02X.", TestAddr);

  for(uint16_t i = 0;i < BufferLength;i += 4)
  {
    HAL_RNG_GenerateRandomNumber(&hrng, &TmpRngdata);
    Tx_Buffer[i + 3] = (TmpRngdata & 0xFF000000) >> 24;;
    Tx_Buffer[i + 2] = (TmpRngdata & 0x00FF0000) >> 16;
    Tx_Buffer[i + 1] = (TmpRngdata & 0x0000FF00) >> 8;
    Tx_Buffer[i + 0] = (TmpRngdata & 0x000000FF);
  }
  
  mpu6050_WriteData(TestAddr, Tx_Buffer, BufferLength);
  mpu6050_ReadData(TestAddr, Rx_Buffer, BufferLength);
  
  for(uint16_t i = 0;i < BufferLength;i++)
  {
    if(Tx_Buffer[i] != Rx_Buffer[i])
    {
      mpu6050_printf("Tx_Buffer[%d] = 0x%02X, Rx_Buffer[%d] = 0x%02X", 
                     i, Tx_Buffer[i],
                     i, Rx_Buffer[i]);
      mpu6050_printf("Data writes and reads do not match, TEST FAILED !");
      return ;
    }
  }
  
  mpu6050_printf("mpu6050 Read and write TEST PASSED !");
}

#endif







