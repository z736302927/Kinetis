#include "at24cxx/at24cxx.h"

/*The following program is modified by the user according to the hardware device, otherwise the driver cannot run.*/

/**
  * @step 1:  Modify the corresponding function according to the modified area and the corresponding function name.
  * @step 2:  Modify four areas: GPIO_PORT/GPIO_PIN/Led_TypeDef/LEDn.
  * @step 3:  .
  * @step 4:  .
  * @step 5:
  */

#include "iic_soft/iic_soft.h"
#include "string.h"
#include "rng.h"

#define DEBUG
#include "idebug/idebug.h"

#define at24cxx_printf                  p_dbg

#define AT24CXX_ADDR                    0x17
#define PAGE_SIZE                       8
#define AT24CXX_MAX_ADDR                256

void at24cxx_Delayus(uint32_t ticks)
{
  HAL_Delay(ticks);
}

void at24cxx_Delayms(uint32_t ticks)
{
  HAL_Delay(ticks);
}

uint8_t at24cxx_PortReceive(uint8_t Ack)
{
  uint8_t Data = 0;
  
  Data = IIC_Soft_ReadByte(Ack);
  
  return Data;
}

void at24cxx_PortMultiTransmmit(uint8_t Addr, uint8_t *pData, uint32_t Length)
{
  IIC_Soft_WriteMultiByteWithAddr(AT24CXX_ADDR, Addr, pData, Length);
}

void at24cxx_PortMultiReceive(uint8_t Addr, uint8_t *pData, uint32_t Length)
{
  IIC_Soft_ReadMultiByteWithAddr(AT24CXX_ADDR, Addr, pData, Length);
}
/*The above procedure is modified by the user according to the hardware device, otherwise the driver cannot run.*/

void at24cxx_ByteWrite(uint8_t Addr, uint8_t Data)
{
  at24cxx_PortMultiTransmmit(Addr, &Data, 1);
  at24cxx_Delayms(5);
}

void at24cxx_PageWrite(uint8_t Addr, uint8_t *pData, uint32_t Length)
{
  at24cxx_PortMultiTransmmit(Addr, pData, Length);
}

void at24cxx_MultiPageWrite(uint32_t Addr, uint8_t* pData, uint16_t Length)
{
  uint8_t NumOfPage = 0, NumOfSingle = 0, SubAddr = 0, Count = 0, Temp = 0;
  
  /* Mod operation, if Addr is an integer multiple of PAGE_SIZE, SubAddr value is 0 */
  SubAddr = Addr % PAGE_SIZE;
  
  /* The difference count is just enough to line up to the page address */
  Count = PAGE_SIZE - SubAddr;  
  /* Figure out how many integer pages to write */
  NumOfPage =  Length / PAGE_SIZE;
  /* mod operation is used to calculate the number of bytes less than one page */
  NumOfSingle = Length % PAGE_SIZE;

  /* SubAddr=0, then Addr is just aligned by page */
  if (SubAddr == 0) 
  {
    /* Length < PAGE_SIZE */
    if (NumOfPage == 0) 
    {
      at24cxx_PageWrite(Addr, pData, Length);
    }
    else /* Length > PAGE_SIZE */
    {
      /* Let me write down all the integer pages */
      while (NumOfPage--)
      {
        at24cxx_PageWrite(Addr, pData, PAGE_SIZE);
        Addr +=  PAGE_SIZE;
        pData += PAGE_SIZE;
      }
      
      /* If you have more than one page of data, write it down*/
      at24cxx_PageWrite(Addr, pData, NumOfSingle);
    }
  }
  /* If the address is not aligned with PAGE_SIZE */
  else 
  {
    /* Length < PAGE_SIZE */
    if (NumOfPage == 0) 
    {
      /* The remaining count positions on the current page are smaller than NumOfSingle */
      if (NumOfSingle > Count) 
      {
        Temp = NumOfSingle - Count;
        
        /* Fill in the front page first */
        at24cxx_PageWrite(Addr, pData, Count);
        Addr +=  Count;
        pData += Count;
        
        /* Let me write the rest of the data */
        at24cxx_PageWrite(Addr, pData, Temp);
      }
      else /* The remaining count position of the current page can write NumOfSingle data */
      {        
        at24cxx_PageWrite(Addr, pData, Length);
      }
    }
    else /* Length > PAGE_SIZE */
    {
      /* The address is not aligned and the extra count is treated separately, not added to the operation */
      Length -= Count;
      NumOfPage =  Length / PAGE_SIZE;
      NumOfSingle = Length % PAGE_SIZE;

      at24cxx_PageWrite(Addr, pData, Count);
      Addr +=  Count;
      pData += Count;
      
      /* Write all the integer pages */
      while (NumOfPage--)
      {
        at24cxx_PageWrite(Addr, pData, PAGE_SIZE);
        Addr +=  PAGE_SIZE;
        pData += PAGE_SIZE;
      }
      /* If you have more than one page of data, write it down */
      if (NumOfSingle != 0)
      {
        at24cxx_PageWrite(Addr, pData, NumOfSingle);
      }
    }
  }
}

void at24cxx_WriteData(uint8_t Addr, uint8_t *pData, uint32_t Length)
{
  uint32_t RemainSpace = 0;
  
  RemainSpace = AT24CXX_MAX_ADDR - Addr;
  
  if(RemainSpace < Length)
  {
    at24cxx_printf("There is not enough space left to write the specified length.");
    return ;
  }
  at24cxx_MultiPageWrite(Addr, pData, Length);
}

void at24cxx_CurrentAddrRead(uint8_t *pData)
{
  IIC_Soft_Start();
  IIC_Soft_SendByte((AT24CXX_ADDR << 1) | 0x01);
  if(IIC_Soft_WaitAck())
  {
    IIC_Soft_Stop();
    return ;
  }
  *pData = IIC_Soft_ReadByte(0);
  IIC_Soft_Stop();
}

void at24cxx_RandomRead(uint8_t Addr, uint8_t *pData, uint32_t Length)
{
  uint32_t RemainSpace = 0;
  
  RemainSpace = AT24CXX_MAX_ADDR - Addr;
  
  if(RemainSpace < Length)
  {
    at24cxx_printf("There is not enough space left to read the specified length.");
    return ;
  }
  at24cxx_PortMultiReceive(Addr, pData, Length);
}

void at24cxx_ReadData(uint8_t Addr, uint8_t *pData, uint32_t Length)
{
  at24cxx_RandomRead(Addr, pData, Length);
}

void at24cxx_SequentialRead(uint8_t *pData, uint32_t Length)
{  
  IIC_Soft_Start();
  IIC_Soft_SendByte((AT24CXX_ADDR << 1) | 0x01); 
  if(IIC_Soft_WaitAck())
  {
    IIC_Soft_Stop();
    return ;
  }
  while(Length) 
  {
    if(Length == 1)
    {
      *pData = IIC_Soft_ReadByte(0);
    }
    else
    {
      *pData = IIC_Soft_ReadByte(1);
    }
    pData++;
    Length--;
  }
  IIC_Soft_Stop();
}

#if 1

static uint8_t Tx_Buffer[256];
static uint8_t Rx_Buffer[256];

void at24cxx_Test(void)
{
  uint32_t TmpRngdata = 0;
  uint16_t BufferLength = 0;
  uint32_t TestAddr = 0;
  
  HAL_RNG_GenerateRandomNumber(&hrng, &TmpRngdata);
  BufferLength = TmpRngdata & 0xFF;
  at24cxx_printf("BufferLength = %d.", BufferLength);
  
  if(Tx_Buffer == NULL || Rx_Buffer == NULL)
  {
    at24cxx_printf("Failed to allocate memory !");
    return;
  }
  memset(Tx_Buffer, 0, BufferLength);
  memset(Rx_Buffer, 0, BufferLength);
  
  HAL_RNG_GenerateRandomNumber(&hrng, &TmpRngdata);
  TestAddr = TmpRngdata & 0xFF;
  at24cxx_printf("TestAddr = 0x%02X.", TestAddr);

  for(uint16_t i = 0;i < BufferLength;i += 4)
  {
    HAL_RNG_GenerateRandomNumber(&hrng, &TmpRngdata);
    Tx_Buffer[i + 3] = (TmpRngdata & 0xFF000000) >> 24;;
    Tx_Buffer[i + 2] = (TmpRngdata & 0x00FF0000) >> 16;
    Tx_Buffer[i + 1] = (TmpRngdata & 0x0000FF00) >> 8;
    Tx_Buffer[i + 0] = (TmpRngdata & 0x000000FF);
  }
  
  at24cxx_WriteData(TestAddr, Tx_Buffer, BufferLength);
  at24cxx_ReadData(TestAddr, Rx_Buffer, BufferLength);
  
  for(uint16_t i = 0;i < BufferLength;i++)
  {
    if(Tx_Buffer[i] != Rx_Buffer[i])
    {
      at24cxx_printf("Tx_Buffer[%d] = 0x%02X, Rx_Buffer[%d] = 0x%02X", 
                     i, Tx_Buffer[i],
                     i, Rx_Buffer[i]);
      at24cxx_printf("Data writes and reads do not match, TEST FAILED !");
      return ;
    }
  }
  
  at24cxx_printf("at24cxx Read and write TEST PASSED !");
}

#endif



