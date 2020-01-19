#include "ds3231/ds3231.h"

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

#define ds3231_printf                  p_dbg

#define AT24CXX_ADDR                   0x00
#define AT24CXX_ADDR                   0x00
#define AT24CXX_ADDR                   0x00
#define AT24CXX_ADDR                   0x00
#define AT24CXX_ADDR                   0x00
#define AT24CXX_ADDR                   0x00
#define AT24CXX_ADDR                   0x00
#define AT24CXX_ADDR                   0x00
#define AT24CXX_ADDR                   0x00
#define AT24CXX_ADDR                   0x00
#define AT24CXX_ADDR                   0x00
#define AT24CXX_ADDR                   0x00
#define AT24CXX_ADDR                   0x00
#define AT24CXX_ADDR                   0x00
#define AT24CXX_ADDR                   0x00
#define AT24CXX_ADDR                   0x00
#define AT24CXX_ADDR                   0x00
#define AT24CXX_ADDR                   0x00
#define AT24CXX_ADDR                   0x00

void ds3231_Delayus(uint32_t ticks)
{
  HAL_Delay(ticks);
}

void ds3231_Delayms(uint32_t ticks)
{
  HAL_Delay(ticks);
}

uint8_t ds3231_PortReceive(uint8_t Ack)
{
  uint8_t Data = 0;
  
  Data = IIC_Soft_ReadByte(Ack);
  
  return Data;
}
void ds3231_PortMultiTransmmit(uint8_t Addr, uint8_t *pData, uint32_t Length)
{
  IIC_Soft_WriteMultiByteWithAddr(AT24CXX_ADDR, Addr, pData, Length);
}

void ds3231_PortMultiReceive(uint8_t Addr, uint8_t *pData, uint32_t Length)
{
  IIC_Soft_ReadMultiByteWithAddr(AT24CXX_ADDR, Addr, pData, Length);
}
/*The above procedure is modified by the user according to the hardware device, otherwise the driver cannot run.*/

#if 1

static uint8_t Tx_Buffer[256];
static uint8_t Rx_Buffer[256];

void ds3231_Test(void)
{
  uint32_t TmpRngdata = 0;
  uint16_t BufferLength = 0;
  uint32_t TestAddr = 0;
  
  HAL_RNG_GenerateRandomNumber(&hrng, &TmpRngdata);
  BufferLength = TmpRngdata & 0xFF;
  ds3231_printf("BufferLength = %d.", BufferLength);
  
  if(Tx_Buffer == NULL || Rx_Buffer == NULL)
  {
    ds3231_printf("Failed to allocate memory !");
    return;
  }
  memset(Tx_Buffer, 0, BufferLength);
  memset(Rx_Buffer, 0, BufferLength);
  
  HAL_RNG_GenerateRandomNumber(&hrng, &TmpRngdata);
  TestAddr = TmpRngdata & 0xFF;
  ds3231_printf("TestAddr = 0x%02X.", TestAddr);

  for(uint16_t i = 0;i < BufferLength;i += 4)
  {
    HAL_RNG_GenerateRandomNumber(&hrng, &TmpRngdata);
    Tx_Buffer[i + 3] = (TmpRngdata & 0xFF000000) >> 24;;
    Tx_Buffer[i + 2] = (TmpRngdata & 0x00FF0000) >> 16;
    Tx_Buffer[i + 1] = (TmpRngdata & 0x0000FF00) >> 8;
    Tx_Buffer[i + 0] = (TmpRngdata & 0x000000FF);
  }
  
  ds3231_WriteData(TestAddr, Tx_Buffer, BufferLength);
  ds3231_ReadData(TestAddr, Rx_Buffer, BufferLength);
  
  for(uint16_t i = 0;i < BufferLength;i++)
  {
    if(Tx_Buffer[i] != Rx_Buffer[i])
    {
      ds3231_printf("Tx_Buffer[%d] = 0x%02X, Rx_Buffer[%d] = 0x%02X", 
                     i, Tx_Buffer[i],
                     i, Rx_Buffer[i]);
      ds3231_printf("Data writes and reads do not match, TEST FAILED !");
      return ;
    }
  }
  
  ds3231_printf("ds3231 Read and write TEST PASSED !");
}

#endif