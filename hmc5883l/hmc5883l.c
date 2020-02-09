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
#include "idebug/idebug.h"

#define hmc5883l_printf                  p_dbg

#define HMC5883L_ADDR                    0x00

void hmc5883l_Delayus(uint32_t ticks)
{
  HAL_Delay(ticks);
}

void hmc5883l_Delayms(uint32_t ticks)
{
  HAL_Delay(ticks);
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
  {
    return 1;
  }
  else
  {
    return 0;
  }
}

uint8_t hmc5883l_DataReady(void)
{
  uint8_t TmpReg = 0;
  
  hmc5883l_PortReceive(SR, &TmpReg);
  
  if(TmpReg & 0x01)
  {
    return 1;
  }
  else
  {
    return 0;
  }
}

void hmc5883l_Identification(uint8_t *pData)
{
  hmc5883l_PortMultiReceive(IRA, pData, 3);
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




