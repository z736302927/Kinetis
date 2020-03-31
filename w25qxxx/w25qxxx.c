#include "chip/w25qxxx.h"
#include "stdbool.h"

/*The following program is modified by the user according to the hardware device, otherwise the driver cannot run.*/

/**
  * @step 1:  Modify the corresponding function according to the modified area and the corresponding function name.
  * @step 2:  Modify four areas: GPIO_PORT/GPIO_PIN/Led_TypeDef/LEDn.
  * @step 3:  .
  * @step 4:  .
  * @step 5:
  */

#include "stdlib.h"
#include "string.h"
#include "rng.h"
#include "spi.h"

#define DEBUG
#include "idebug.h"

#define w25qxxx_printf                  p_dbg

static inline void w25qxxx_PortTransmmit(uint8_t w25qxxx, uint8_t Data)
{
  if(w25qxxx == W25Q128)
  {
    HAL_SPI_Transmit(&hspi1, &Data, 1, 1000);
  }
  else if(w25qxxx == W25Q256)
  {
    HAL_SPI_Transmit(&hspi5, &Data, 1, 1000);
  }
}

static inline uint8_t w25qxxx_PortReceive(uint8_t w25qxxx)
{
  uint8_t Data = 0;
  
  if(w25qxxx == W25Q128)
  {
    HAL_SPI_Receive(&hspi1, &Data, 1, 1000);
  }
  else if(w25qxxx == W25Q256)
  {
    HAL_SPI_Receive(&hspi5, &Data, 1, 1000);
  }
  
  return Data;
}

static inline void w25qxxx_PortMultiTransmmit(uint8_t w25qxxx, uint8_t *pData, uint32_t Length)
{
  if(w25qxxx == W25Q128)
  {
    HAL_SPI_Transmit(&hspi1, pData, Length, 1000);
  }
  else if(w25qxxx == W25Q256)
  {
    HAL_SPI_Transmit(&hspi5, pData, Length, 1000);
  }
}

static inline void w25qxxx_PortMultiReceive(uint8_t w25qxxx, uint8_t *pData, uint32_t Length)
{
  if(w25qxxx == W25Q128)
  {
    HAL_SPI_Receive(&hspi1, pData, Length, 1000);
  }
  else if(w25qxxx == W25Q256)
  {
    HAL_SPI_Receive(&hspi5, pData, Length, 1000);
  }
}

static inline void w25qxxx_CS_Low(uint8_t w25qxxx)
{
  if(w25qxxx == W25Q128)
  {
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_RESET);
  }
  else if(w25qxxx == W25Q256)
  {
    HAL_GPIO_WritePin(GPIOF, GPIO_PIN_6, GPIO_PIN_RESET);
  }
}

static inline void w25qxxx_CS_High(uint8_t w25qxxx)
{
  if(w25qxxx == W25Q128)
  {
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_SET);
  }
  else if(w25qxxx == W25Q256)
  {
    HAL_GPIO_WritePin(GPIOF, GPIO_PIN_6, GPIO_PIN_SET);
  }
}

void w25qxxx_HardReset(uint8_t w25qxxx)
{
  if(w25qxxx == W25Q128)
  {
    
  }
  else if(w25qxxx == W25Q256)
  {
    
  }
}

void w25qxxx_Delayus(uint32_t ticks)
{
  HAL_Delay(ticks);
}

void w25qxxx_Delayms(uint32_t ticks)
{
  HAL_Delay(ticks);
}

uint32_t w25qxxx_GetTick(void)
{
  return HAL_GetTick();
}

/*The above procedure is modified by the user according to the hardware device, otherwise the driver cannot run.*/

#define PAGE_SIZE                       256
#define SECTOR_SIZE                     4096
#define DUMMY_BYTE                      0xFF
#define WRITE_ENABLE                    0x06
#define WRITE_ENABLE_FORVOLATILE        0x50
#define WRITE_DISABLE                   0x04
#define READ_STATUS_REGISTER1           0x05
#define READ_STATUS_REGISTER2           0x35
#define READ_STATUS_REGISTER3           0x15
#define WRITE_STATUS_REGISTER1          0x01
#define WRITE_STATUS_REGISTER2          0x31
#define WRITE_STATUS_REGISTER3          0x11
#define READ_EXTENDED_ADDRESS_REGISTER  0xC8
#define WRITE_EXTENDED_ADDRESS_REGISTER 0xC5
#define ENTER_4BYTE_ADDRESS_MODE        0xB7
#define EXIT_4BYTE_ADDRESS_MODE         0xE9
#define READ_DATA                       0x03
#define READ_DATA_WITH_4Byte_ADDRESS    0x13
#define FAST_READ                       0x0B
#define FAST_READ_WITH_4Byte_ADDRESS    0x0C
#define PAGE_PROGRAM                    0x02
#define SECTOR_ERASE                    0x20
#define BLOCK_ERASE_32KB                0x52
#define BLOCK_ERASE_64KB                0xD8
#define CHIP_ERASE                      0xC7
#define ERASE_PROGRAM_SUSPEND           0x75
#define ERASE_PROGRAM_RESUME            0x7A
#define POWER_DOWN                      0xB9
#define RELEASE_POWERDOWN_DEVICE_ID     0xAB
#define READ_MANUFACTURER_DEVICE_ID     0x90
#define READ_UNIQUE_ID_NUMBER           0x4B
#define READ_JEDEC_ID                   0x9F
#define READ_SFDP_REGISTER              0x5A
#define SECURITY_REGISTERS_1            0x10
#define SECURITY_REGISTERS_2            0x20
#define SECURITY_REGISTERS_3            0x30
#define ERASE_SECURITY_REGISTERS        0x44
#define PROGRAM_SECURITY_REGISTERS      0x42
#define READ_SECURITY_REGISTERS         0x48
#define ENTER_QPI_MODE                  0x38
#define EXIT_QPI_MODE                   0xFF
#define INDIVIDUAL_BLOCK_SECTOR_LOCK    0x36
#define INDIVIDUAL_BLOCK_SECTOR_UNLOCK  0x39
#define READ_BLOCK_SECTOR_LOCK          0x3D
#define GLOBAL_BLOCK_SECTOR_LOCK        0x7E
#define GLOBAL_BLOCK_SECTOR_UNLOCK      0x98
#define ENABLE_RESET                    0x66
#define RESET_DEVICE                    0x99

typedef struct _StatusRegisters_TypeDef
{
  unsigned BUSY:1;
  unsigned WEL:1;
  unsigned BP0:1;
  unsigned BP1:1;
  unsigned BP2:1;
  unsigned TB:1;
  unsigned SEC:1;
  unsigned SRP0:1;
  
  unsigned SRP1:1;
  unsigned QE:1;
  unsigned LB1:1;
  unsigned LB2:1;
  unsigned LB3:1;
  unsigned CMP:1;
  unsigned SUS:1;
  
  unsigned WPS:1;
  unsigned DRV0:1;
  unsigned DRV1:1;
  unsigned HOLD_RST:1;
}StatusRegisters_TypeDef;

uint32_t g_w25q128_Max_Addr = 0xFFFFFF;
uint32_t g_w25q256_Max_Addr = 0x1FFFFFF;
//uint32_t g_w25qxxx = 0x17;

void w25qxxx_TransCmd(uint8_t w25qxxx, uint8_t Cmd)
{
  w25qxxx_CS_Low(w25qxxx);
  w25qxxx_PortTransmmit(w25qxxx, Cmd);
  w25qxxx_CS_High(w25qxxx);
}

uint8_t w25qxxx_Read_BUSY(uint8_t w25qxxx)
{
  uint8_t Data = 0;
  
  Data = w25qxxx_ReadStatusRegister(w25qxxx, READ_STATUS_REGISTER1);
  Data &= 0x01;
  
  return Data;
}

uint8_t w25qxxx_WaitForCmdEnd(uint8_t w25qxxx)
{
  uint32_t begintime = 0, currenttime = 0, timediff = 0;
    
  begintime = HAL_GetTick();
  
  while(1)
  {
    
    if(w25qxxx_Read_BUSY(w25qxxx) == 0)
    {
      return true;
    }
    else
    {
      currenttime = HAL_GetTick();
      timediff = currenttime >= begintime ? currenttime - begintime : 
                                            currenttime + UINT32_MAX - begintime;
      if(timediff > 30000) /* 30s */
      {
        w25qxxx_printf("Command execution timeout !");
        return false;
      }
    }
  }
}

uint8_t w25qxxx_Read_WEL(uint8_t w25qxxx)
{
  uint8_t Data = 0;
  
  Data = w25qxxx_ReadStatusRegister(w25qxxx, READ_STATUS_REGISTER1);
  Data >>= 1;
  Data &= 0x01;
  
  return Data;
}

uint8_t w25qxxx_Read_BP0(uint8_t w25qxxx)
{
  uint8_t Data = 0;
  
  Data = w25qxxx_ReadStatusRegister(w25qxxx, READ_STATUS_REGISTER1);
  Data >>= 2;
  Data &= 0x01;
  
  return Data;
}

uint8_t w25qxxx_Read_BP1(uint8_t w25qxxx)
{
  uint8_t Data = 0;
  
  Data = w25qxxx_ReadStatusRegister(w25qxxx, READ_STATUS_REGISTER1);
  Data >>= 4;
  Data &= 0x01;
  
  return Data;
}

uint8_t w25qxxx_Read_BP2(uint8_t w25qxxx)
{
  uint8_t Data = 0;
  
  Data = w25qxxx_ReadStatusRegister(w25qxxx, READ_STATUS_REGISTER1);
  Data >>= 5;
  Data &= 0x01;
  
  return Data;
}

uint8_t w25qxxx_Read_SEC(uint8_t w25qxxx)
{
  uint8_t Data = 0;
  
  Data = w25qxxx_ReadStatusRegister(w25qxxx, READ_STATUS_REGISTER1);
  Data >>= 6;
  Data &= 0x01;
  
  return Data;
}

uint8_t w25qxxx_Read_BP3(uint8_t w25qxxx)
{
  uint8_t Data = 0;
  
  Data = w25qxxx_ReadStatusRegister(w25qxxx, READ_STATUS_REGISTER1);
  Data >>= 6;
  Data &= 0x01;
  
  return Data;
}

uint8_t w25qxxx_Read_TB(uint8_t w25qxxx)
{
  uint8_t Data = 0;
  
  Data = w25qxxx_ReadStatusRegister(w25qxxx, READ_STATUS_REGISTER1);
  Data >>= 7;
  Data &= 0x01;
  
  return Data;
}

uint8_t w25qxxx_Read_SRP0(uint8_t w25qxxx)
{
  uint8_t Data = 0;
  
  Data = w25qxxx_ReadStatusRegister(w25qxxx, READ_STATUS_REGISTER1);
  Data >>= 7;
  Data &= 0x01;
  
  return Data;
}

uint8_t w25qxxx_Read_SRP1(uint8_t w25qxxx)
{
  uint8_t Data = 0;
  
  Data = w25qxxx_ReadStatusRegister(w25qxxx, READ_STATUS_REGISTER2);
  Data &= 0x01;
  
  return Data;
}

uint8_t w25qxxx_Read_QE(uint8_t w25qxxx)
{
  uint8_t Data = 0;
  
  Data = w25qxxx_ReadStatusRegister(w25qxxx, READ_STATUS_REGISTER2);
  Data >>= 1;
  Data &= 0x01;
  
  return Data;
}

uint8_t w25qxxx_Read_LB1(uint8_t w25qxxx)
{
  uint8_t Data = 0;
  
  Data = w25qxxx_ReadStatusRegister(w25qxxx, READ_STATUS_REGISTER2);
  Data >>= 3;
  Data &= 0x01;
  
  return Data;
}

uint8_t w25qxxx_Read_LB2(uint8_t w25qxxx)
{
  uint8_t Data = 0;
  
  Data = w25qxxx_ReadStatusRegister(w25qxxx, READ_STATUS_REGISTER2);
  Data >>= 4;
  Data &= 0x01;
  
  return Data;
}

uint8_t w25qxxx_Read_LB3(uint8_t w25qxxx)
{
  uint8_t Data = 0;
  
  Data = w25qxxx_ReadStatusRegister(w25qxxx, READ_STATUS_REGISTER2);
  Data >>= 5;
  Data &= 0x01;
  
  return Data;
}

uint8_t w25qxxx_Read_CMP(uint8_t w25qxxx)
{
  uint8_t Data = 0;
  
  Data = w25qxxx_ReadStatusRegister(w25qxxx, READ_STATUS_REGISTER2);
  Data >>= 6;
  Data &= 0x01;
  
  return Data;
}

uint8_t w25qxxx_Read_SUS(uint8_t w25qxxx)
{
  uint8_t Data = 0;
  
  Data = w25qxxx_ReadStatusRegister(w25qxxx, READ_STATUS_REGISTER2);
  Data >>= 7;
  Data &= 0x01;
  
  return Data;
}

uint8_t w25qxxx_Read_ADS(uint8_t w25qxxx)
{
  uint8_t Data = 0;
  
  Data = w25qxxx_ReadStatusRegister(w25qxxx, READ_STATUS_REGISTER3);
  Data &= 0x01;
  
  return Data;
}

uint8_t w25qxxx_Read_ADP(uint8_t w25qxxx)
{
  uint8_t Data = 0;
  
  Data = w25qxxx_ReadStatusRegister(w25qxxx, READ_STATUS_REGISTER3);
  Data >>= 1;
  Data &= 0x01;
  
  return Data;
}

uint8_t w25qxxx_Read_WPS(uint8_t w25qxxx)
{
  uint8_t Data = 0;
  
  Data = w25qxxx_ReadStatusRegister(w25qxxx, READ_STATUS_REGISTER3);
  Data >>= 2;
  Data &= 0x01;
  
  return Data;
}

uint8_t w25qxxx_Read_DRV0(uint8_t w25qxxx)
{
  uint8_t Data = 0;
  
  Data = w25qxxx_ReadStatusRegister(w25qxxx, READ_STATUS_REGISTER3);
  Data >>= 5;
  Data &= 0x01;
  
  return Data;
}

uint8_t w25qxxx_Read_DRV1(uint8_t w25qxxx)
{
  uint8_t Data = 0;
  
  Data = w25qxxx_ReadStatusRegister(w25qxxx, READ_STATUS_REGISTER3);
  Data >>= 6;
  Data &= 0x01;
  
  return Data;
}

uint8_t w25qxxx_Read_HOLD_RST(uint8_t w25qxxx)
{
  uint8_t Data = 0;
  
  Data = w25qxxx_ReadStatusRegister(w25qxxx, READ_STATUS_REGISTER3);
  Data >>= 7;
  Data &= 0x01;
  
  return Data;
}

void w25qxxx_Write_SRP0(uint8_t w25qxxx, uint8_t Data)
{
  uint8_t Reg = 0;
  
  Reg = w25qxxx_ReadStatusRegister(w25qxxx, READ_STATUS_REGISTER1);
  if(Data == 1)
  {
    Reg |= 0x80;
  }
  else
  {
    Reg &= ~0x80;
  }
  
  w25qxxx_TransCmd(w25qxxx, WRITE_ENABLE);
  w25qxxx_WriteStatusRegister(w25qxxx, WRITE_STATUS_REGISTER1, Reg);
}

void w25qxxx_Write_SEC(uint8_t w25qxxx, uint8_t Data)
{
  uint8_t Reg = 0;
  
  Reg = w25qxxx_ReadStatusRegister(w25qxxx, READ_STATUS_REGISTER1);
  if(Data == 1)
  {
    Reg |= 0x40;
  }
  else
  {
    Reg &= ~0x40;
  }
  
  w25qxxx_TransCmd(w25qxxx, WRITE_ENABLE);
  w25qxxx_WriteStatusRegister(w25qxxx, WRITE_STATUS_REGISTER1, Reg);
}

void w25qxxx_Write_TB(uint8_t w25qxxx, uint8_t Data)
{
  uint8_t Reg = 0;
  
  Reg = w25qxxx_ReadStatusRegister(w25qxxx, READ_STATUS_REGISTER1);
  if(Data == 1)
  {
    Reg |= 0x20;
  }
  else
  {
    Reg &= ~0x20;
  }
  
  w25qxxx_TransCmd(w25qxxx, WRITE_ENABLE);
  w25qxxx_WriteStatusRegister(w25qxxx, WRITE_STATUS_REGISTER1, Reg);
}

//void w25qxxx_Write_BP(uint8_t w25qxxx, uint8_t Data)
//{
//  uint8_t Reg = 0;
//  
//  Reg = w25qxxx_ReadStatusRegister(w25qxxx, READ_STATUS_REGISTER1);
//  if(Data == 1)
//  {
//    Reg |= 0x80;
//  }
//  else
//  {
//    Reg &= ~0x80;
//  }
//  
//  w25qxxx_TransCmd(w25qxxx, WRITE_ENABLE);
//  w25qxxx_WriteStatusRegister(w25qxxx, WRITE_STATUS_REGISTER1, Reg);
//}

void w25qxxx_Write_CMP(uint8_t w25qxxx, uint8_t Data)
{
  uint8_t Reg = 0;
  
  Reg = w25qxxx_ReadStatusRegister(w25qxxx, READ_STATUS_REGISTER2);
  if(Data == 1)
  {
    Reg |= 0x40;
  }
  else
  {
    Reg &= ~0x40;
  }
  
  w25qxxx_TransCmd(w25qxxx, WRITE_ENABLE);
  w25qxxx_WriteStatusRegister(w25qxxx, WRITE_STATUS_REGISTER2, Reg);
}

//void w25qxxx_Write_LB(uint8_t w25qxxx, uint8_t Data)
//{
//  uint8_t Reg = 0;
//  
//  Reg = w25qxxx_ReadStatusRegister(w25qxxx, READ_STATUS_REGISTER2);
//  if(Data == 1)
//  {
//    Reg |= 0x80;
//  }
//  else
//  {
//    Reg &= ~0x80;
//  }
//  
//  w25qxxx_TransCmd(w25qxxx, WRITE_ENABLE);
//  w25qxxx_WriteStatusRegister(w25qxxx, WRITE_STATUS_REGISTER2, Reg);
//}

void w25qxxx_Write_QE(uint8_t w25qxxx, uint8_t Data)
{
  uint8_t Reg = 0;
  
  Reg = w25qxxx_ReadStatusRegister(w25qxxx, READ_STATUS_REGISTER2);
  if(Data == 1)
  {
    Reg |= 0x02;
  }
  else
  {
    Reg &= ~0x02;
  }
  
  w25qxxx_TransCmd(w25qxxx, WRITE_ENABLE);
  w25qxxx_WriteStatusRegister(w25qxxx, WRITE_STATUS_REGISTER2, Reg);
}

void w25qxxx_Write_SRP1(uint8_t w25qxxx, uint8_t Data)
{
  uint8_t Reg = 0;
  
  Reg = w25qxxx_ReadStatusRegister(w25qxxx, READ_STATUS_REGISTER2);
  if(Data == 1)
  {
    Reg |= 0x01;
  }
  else
  {
    Reg &= ~0x01;
  }
  
  w25qxxx_TransCmd(w25qxxx, WRITE_ENABLE);
  w25qxxx_WriteStatusRegister(w25qxxx, WRITE_STATUS_REGISTER2, Reg);
}

void w25qxxx_Write_HOLD_RST(uint8_t w25qxxx, uint8_t Data)
{
  uint8_t Reg = 0;
  
  Reg = w25qxxx_ReadStatusRegister(w25qxxx, READ_STATUS_REGISTER3);
  if(Data == 1)
  {
    Reg |= 0x80;
  }
  else
  {
    Reg &= ~0x80;
  }
  
  w25qxxx_TransCmd(w25qxxx, WRITE_ENABLE);
  w25qxxx_WriteStatusRegister(w25qxxx, WRITE_STATUS_REGISTER3, Reg);
}

void w25qxxx_Write_DRV1(uint8_t w25qxxx, uint8_t Data)
{
  uint8_t Reg = 0;
  
  Reg = w25qxxx_ReadStatusRegister(w25qxxx, READ_STATUS_REGISTER3);
  if(Data == 1)
  {
    Reg |= 0x40;
  }
  else
  {
    Reg &= ~0x40;
  }
  
  w25qxxx_TransCmd(w25qxxx, WRITE_ENABLE);
  w25qxxx_WriteStatusRegister(w25qxxx, WRITE_STATUS_REGISTER3, Reg);
}

void w25qxxx_Write_DRV0(uint8_t w25qxxx, uint8_t Data)
{
  uint8_t Reg = 0;
  
  Reg = w25qxxx_ReadStatusRegister(w25qxxx, READ_STATUS_REGISTER3);
  if(Data == 1)
  {
    Reg |= 0x20;
  }
  else
  {
    Reg &= ~0x20;
  }
  
  w25qxxx_TransCmd(w25qxxx, WRITE_ENABLE);
  w25qxxx_WriteStatusRegister(w25qxxx, WRITE_STATUS_REGISTER3, Reg);
}

void w25qxxx_Write_WPS(uint8_t w25qxxx, uint8_t Data)
{
  uint8_t Reg = 0;
  
  Reg = w25qxxx_ReadStatusRegister(w25qxxx, READ_STATUS_REGISTER3);
  if(Data == 1)
  {
    Reg |= 0x04;
  }
  else
  {
    Reg &= ~0x04;
  }
  
  w25qxxx_TransCmd(w25qxxx, WRITE_ENABLE);
  w25qxxx_WriteStatusRegister(w25qxxx, WRITE_STATUS_REGISTER3, Reg);
}

void w25qxxx_Write_ADP(uint8_t w25qxxx, uint8_t Data)
{
  uint8_t Reg = 0;
  
  Reg = w25qxxx_ReadStatusRegister(w25qxxx, READ_STATUS_REGISTER3);
  if(Data == 1)
  {
    Reg |= 0x02;
  }
  else
  {
    Reg &= ~0x02;
  }
  
  w25qxxx_TransCmd(w25qxxx, WRITE_ENABLE);
  w25qxxx_WriteStatusRegister(w25qxxx, WRITE_STATUS_REGISTER3, Reg);
}

void w25qxxx_WriteEnable(uint8_t w25qxxx)
{
  w25qxxx_TransCmd(w25qxxx, WRITE_ENABLE);
}

void w25qxxx_WriteEnableForVolatile(uint8_t w25qxxx)
{
  w25qxxx_TransCmd(w25qxxx, WRITE_ENABLE_FORVOLATILE);
}

void w25qxxx_WriteDisable(uint8_t w25qxxx)
{
  w25qxxx_TransCmd(w25qxxx, WRITE_DISABLE);
}

uint8_t w25qxxx_ReadStatusRegister(uint8_t w25qxxx, uint8_t Number)
{
  uint8_t Data = 0;
  
  w25qxxx_CS_Low(w25qxxx);
  switch(Number)
  {
    case READ_STATUS_REGISTER1:
      w25qxxx_PortTransmmit(w25qxxx, READ_STATUS_REGISTER1);
      Data = w25qxxx_PortReceive(w25qxxx);
      break;
      
    case READ_STATUS_REGISTER2:
      w25qxxx_PortTransmmit(w25qxxx, READ_STATUS_REGISTER2);
      Data = w25qxxx_PortReceive(w25qxxx);
      break;
      
    case READ_STATUS_REGISTER3:
      w25qxxx_PortTransmmit(w25qxxx, READ_STATUS_REGISTER3);
      Data = w25qxxx_PortReceive(w25qxxx);
      break;
      
    default:break;
  }
  w25qxxx_CS_High(w25qxxx);
  
  return Data;
}

void w25qxxx_WriteStatusRegister(uint8_t w25qxxx, uint8_t Number, uint8_t Data)
{
  w25qxxx_WriteEnable(w25qxxx);
  
  w25qxxx_CS_Low(w25qxxx);
  switch(Number)
  {
    case WRITE_STATUS_REGISTER1:
      w25qxxx_PortTransmmit(w25qxxx, WRITE_STATUS_REGISTER1);
      w25qxxx_PortTransmmit(w25qxxx, Data);
      break;
      
    case WRITE_STATUS_REGISTER2:
      w25qxxx_PortTransmmit(w25qxxx, WRITE_STATUS_REGISTER2);
      w25qxxx_PortTransmmit(w25qxxx, Data);
      break;
      
    case WRITE_STATUS_REGISTER3:
      w25qxxx_PortTransmmit(w25qxxx, WRITE_STATUS_REGISTER3);
      w25qxxx_PortTransmmit(w25qxxx, Data);
      break;
      
    default:break;
  }
  w25qxxx_CS_High(w25qxxx);
}

uint8_t w25q256_ReadExtendedAddressRegister(uint8_t w25qxxx)
{
  uint8_t Data = 0;
  
  w25qxxx_CS_Low(w25qxxx);
  w25qxxx_PortTransmmit(w25qxxx, READ_EXTENDED_ADDRESS_REGISTER);
  Data = w25qxxx_PortReceive(w25qxxx);
  w25qxxx_CS_High(w25qxxx);
  
  return Data;
}

void w25q256_WriteExtendedAddressRegister(uint8_t w25qxxx, uint8_t Data)
{
  w25qxxx_WriteEnable(w25qxxx);
  w25qxxx_CS_Low(w25qxxx);
  w25qxxx_PortTransmmit(w25qxxx, WRITE_EXTENDED_ADDRESS_REGISTER);
  w25qxxx_PortTransmmit(w25qxxx, Data);
  w25qxxx_CS_High(w25qxxx);
}

void w25q256_Enter4ByteAddressMode(uint8_t w25qxxx)
{
  w25qxxx_TransCmd(w25qxxx, ENTER_4BYTE_ADDRESS_MODE);
}

void w25q256_Exit4ByteAddressMode(uint8_t w25qxxx)
{
  w25qxxx_TransCmd(w25qxxx, EXIT_4BYTE_ADDRESS_MODE);
}

void w25qxxx_ReadData(uint8_t w25qxxx, uint32_t Addr, uint8_t *pData, uint32_t Length)
{
  uint8_t SubAddr[4];
  
  if(w25qxxx == W25Q256)
  {
    SubAddr[0] = (Addr & 0xFF000000) >> 24;
  }
  SubAddr[1] = (Addr & 0x00FF0000) >> 16;
  SubAddr[2] = (Addr & 0x0000FF00) >> 8;
  SubAddr[3] = (Addr & 0x000000FF);
  
  w25qxxx_CS_Low(w25qxxx);
  w25qxxx_PortTransmmit(w25qxxx, READ_DATA);  
  if(w25qxxx == W25Q256)
  {
    w25qxxx_PortTransmmit(w25qxxx, SubAddr[0]);
  }
  w25qxxx_PortMultiTransmmit(w25qxxx, &SubAddr[1], 3);
  
  w25qxxx_PortMultiReceive(w25qxxx, pData, Length);
  w25qxxx_CS_High(w25qxxx);
}

void w25q256_ReadDatawith4ByteAddress(uint8_t w25qxxx, uint32_t Addr, uint8_t *pData, uint32_t Length)
{
  uint8_t SubAddr[4];
  
  SubAddr[0] = (Addr & 0xFF000000) >> 24;
  SubAddr[1] = (Addr & 0x00FF0000) >> 16;
  SubAddr[2] = (Addr & 0x0000FF00) >> 8;
  SubAddr[3] = (Addr & 0x000000FF);
  
  w25qxxx_CS_Low(w25qxxx);
  w25qxxx_PortTransmmit(w25qxxx, READ_DATA_WITH_4Byte_ADDRESS);
  w25qxxx_PortMultiTransmmit(w25qxxx, SubAddr, 4);
  
  w25qxxx_PortMultiReceive(w25qxxx, pData, Length);
  w25qxxx_CS_High(w25qxxx);
}

void w25qxxx_FastRead(uint8_t w25qxxx, uint32_t Addr, uint8_t *pData, uint32_t Length)
{
  uint8_t SubAddr[4];
  
  if(w25qxxx == W25Q256)
  {
    SubAddr[0] = (Addr & 0xFF000000) >> 24;
  }
  SubAddr[1] = (Addr & 0x00FF0000) >> 16;
  SubAddr[2] = (Addr & 0x0000FF00) >> 8;
  SubAddr[3] = (Addr & 0x000000FF);
  
  w25qxxx_CS_Low(w25qxxx);
  w25qxxx_PortTransmmit(w25qxxx, FAST_READ);  
  if(w25qxxx == W25Q256)
  {
    w25qxxx_PortTransmmit(w25qxxx, SubAddr[0]);
  }
  w25qxxx_PortMultiTransmmit(w25qxxx, &SubAddr[1], 3);
  w25qxxx_PortTransmmit(w25qxxx, DUMMY_BYTE);
  
  w25qxxx_PortMultiReceive(w25qxxx, pData, Length);
  w25qxxx_CS_High(w25qxxx);
}

void w25q256_FastReadwith4ByteAddress(uint8_t w25qxxx, uint32_t Addr, uint8_t *pData, uint32_t Length)
{
  uint8_t SubAddr[4];
  
  SubAddr[0] = (Addr & 0xFF000000) >> 24;
  SubAddr[1] = (Addr & 0x00FF0000) >> 16;
  SubAddr[2] = (Addr & 0x0000FF00) >> 8;
  SubAddr[3] = (Addr & 0x000000FF);
  
  w25qxxx_CS_Low(w25qxxx);
  w25qxxx_PortTransmmit(w25qxxx, FAST_READ_WITH_4Byte_ADDRESS);
  w25qxxx_PortMultiTransmmit(w25qxxx, SubAddr, 4);
  w25qxxx_PortTransmmit(w25qxxx, DUMMY_BYTE);
  
  w25qxxx_PortMultiReceive(w25qxxx, pData, Length);
  w25qxxx_CS_High(w25qxxx);
}

void w25qxxx_PageProgram(uint8_t w25qxxx, uint32_t Addr, uint8_t *pData, uint16_t Length)
{
  uint8_t SubAddr[4];
  
  if(Length == 0)
  {
    return ;
  }
  
  if(w25qxxx == W25Q256)
  {
    SubAddr[0] = (Addr & 0xFF000000) >> 24;
  }
  SubAddr[1] = (Addr & 0x00FF0000) >> 16;
  SubAddr[2] = (Addr & 0x0000FF00) >> 8;
  SubAddr[3] = (Addr & 0x000000FF);
  
  w25qxxx_WriteEnable(w25qxxx);
  w25qxxx_CS_Low(w25qxxx);
  w25qxxx_PortTransmmit(w25qxxx, PAGE_PROGRAM);
  if(w25qxxx == W25Q256)
  {
    w25qxxx_PortTransmmit(w25qxxx, SubAddr[0]);
  }
  w25qxxx_PortMultiTransmmit(w25qxxx, &SubAddr[1], 3);
  w25qxxx_PortMultiTransmmit(w25qxxx, pData, Length);
  w25qxxx_CS_High(w25qxxx);
  
  w25qxxx_WaitForCmdEnd(w25qxxx);
}

void w25qxxx_MultiPageProgram(uint8_t w25qxxx, uint32_t Addr, uint8_t* pData, uint16_t Length)
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
      w25qxxx_PageProgram(w25qxxx, Addr, pData, Length);
    }
    else /* Length > PAGE_SIZE */
    {
      /* Let me write down all the integer pages */
      while (NumOfPage--)
      {
        w25qxxx_PageProgram(w25qxxx, Addr, pData, PAGE_SIZE);
        Addr +=  PAGE_SIZE;
        pData += PAGE_SIZE;
      }
      
      /* If you have more than one page of data, write it down*/
      w25qxxx_PageProgram(w25qxxx, Addr, pData, NumOfSingle);
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
        w25qxxx_PageProgram(w25qxxx, Addr, pData, Count);
        Addr +=  Count;
        pData += Count;
        
        /* Let me write the rest of the data */
        w25qxxx_PageProgram(w25qxxx, Addr, pData, Temp);
      }
      else /* The remaining count position of the current page can write NumOfSingle data */
      {        
        w25qxxx_PageProgram(w25qxxx, Addr, pData, Length);
      }
    }
    else /* Length > PAGE_SIZE */
    {
      /* The address is not aligned and the extra count is treated separately, not added to the operation */
      Length -= Count;
      NumOfPage =  Length / PAGE_SIZE;
      NumOfSingle = Length % PAGE_SIZE;

      w25qxxx_PageProgram(w25qxxx, Addr, pData, Count);
      Addr +=  Count;
      pData += Count;
      
      /* Write all the integer pages */
      while (NumOfPage--)
      {
        w25qxxx_PageProgram(w25qxxx, Addr, pData, PAGE_SIZE);
        Addr +=  PAGE_SIZE;
        pData += PAGE_SIZE;
      }
      /* If you have more than one page of data, write it down */
      if (NumOfSingle != 0)
      {
        w25qxxx_PageProgram(w25qxxx, Addr, pData, NumOfSingle);
      }
    }
  }
}

static uint8_t g_w25qxxx_SingleSector[SECTOR_SIZE];

void w25qxxx_ProcessPartialSector(uint8_t w25qxxx, uint32_t BeginAddr, uint8_t* pData, uint16_t Length)
{
  uint32_t SectorOffset = 0;
  uint32_t SectorAddr = 0;
  uint32_t i = 0;
  
  SectorOffset = BeginAddr % SECTOR_SIZE;
  SectorAddr = BeginAddr - SectorOffset;
  
  w25qxxx_ReadData(w25qxxx, BeginAddr, &g_w25qxxx_SingleSector[SectorOffset], Length);
  
  for(i = SectorOffset;i < SectorOffset + Length;i++)
  {
    if(g_w25qxxx_SingleSector[i] != 0xFF)
    {
      w25qxxx_ReadData(w25qxxx, SectorAddr, &g_w25qxxx_SingleSector[0], SectorOffset);
      w25qxxx_ReadData(w25qxxx, BeginAddr + Length, &g_w25qxxx_SingleSector[SectorOffset + Length], SECTOR_SIZE - SectorOffset - Length);
      memcpy(&g_w25qxxx_SingleSector[SectorOffset], pData, Length);
      w25qxxx_SectorErase(w25qxxx, SectorAddr);
      w25qxxx_MultiPageProgram(w25qxxx, SectorAddr, g_w25qxxx_SingleSector, SECTOR_SIZE);
      return ;
    }
  }
  w25qxxx_MultiPageProgram(w25qxxx, BeginAddr, pData, Length);
}

void w25qxxx_MultiSectorProgram(uint8_t w25qxxx, uint32_t Addr, uint8_t* pData, uint32_t Length)
{
  uint32_t NumOfSector = 0, NumOfSingle = 0, SectorOffset = 0;
  uint32_t Count = 0, Temp = 0;
  
  /* Mod operation, if Addr is an integer multiple of SECTOR_SIZE, SectorOffset value is 0 */
  SectorOffset = Addr % SECTOR_SIZE;
  
  /* The difference count is just enough to line up to the page address */
  Count = SECTOR_SIZE - SectorOffset;  
  /* Figure out how many integer pages to write */
  NumOfSector =  Length / SECTOR_SIZE;
  /* mod operation is used to calculate the number of bytes less than one page */
  NumOfSingle = Length % SECTOR_SIZE;

  /* SectorOffset=0, then Addr is just aligned by page */
  if (SectorOffset == 0) 
  {
    /* Length < SECTOR_SIZE */
    if (NumOfSector == 0) 
    {
      w25qxxx_ProcessPartialSector(w25qxxx, Addr, pData, Length);
    }
    else /* Length > SECTOR_SIZE */
    {
      /* Let me write down all the integer pages */
      while (NumOfSector--)
      {
        w25qxxx_SectorErase(w25qxxx, Addr);
        w25qxxx_MultiPageProgram(w25qxxx, Addr, pData, SECTOR_SIZE);
        Addr +=  SECTOR_SIZE;
        pData += SECTOR_SIZE;
      }
      
      /* If you have more than one page of data, write it down*/
      w25qxxx_ProcessPartialSector(w25qxxx, Addr, pData, NumOfSingle);
    }
  }
  /* If the address is not aligned with SECTOR_SIZE */
  else 
  {
    /* Length < SECTOR_SIZE */
    if (NumOfSector == 0) 
    {
      /* The remaining count positions on the current page are smaller than NumOfSingle */
      if (NumOfSingle > Count) 
      {
        Temp = NumOfSingle - Count;
        
        /* Fill in the front page first */
        w25qxxx_ProcessPartialSector(w25qxxx, Addr, pData, Count);
        Addr +=  Count;
        pData += Count;
        
        /* Let me write the rest of the data */
        w25qxxx_ProcessPartialSector(w25qxxx, Addr, pData, Temp);
      }
      else /* The remaining count position of the current page can write NumOfSingle data */
      {
        w25qxxx_ProcessPartialSector(w25qxxx, Addr, pData, Length);
      }
    }
    else /* Length > SECTOR_SIZE */
    {
      /* The address is not aligned and the extra count is treated separately, not added to the operation */
      Length -= Count;
      NumOfSector =  Length / SECTOR_SIZE;
      NumOfSingle = Length % SECTOR_SIZE;

      w25qxxx_ProcessPartialSector(w25qxxx, Addr, pData, Count);
      Addr +=  Count;
      pData += Count;
      
      /* Write all the integer pages */
      while (NumOfSector--)
      {
        w25qxxx_SectorErase(w25qxxx, Addr);
        w25qxxx_MultiPageProgram(w25qxxx, Addr, pData, SECTOR_SIZE);
        Addr +=  SECTOR_SIZE;
        pData += SECTOR_SIZE;
      }
      /* If you have more than one page of data, write it down */
      if (NumOfSingle != 0)
      {
        w25qxxx_ProcessPartialSector(w25qxxx, Addr, pData, NumOfSingle);
      }
    }
  }
}

void w25qxxx_WriteData(uint8_t w25qxxx, uint32_t Addr, uint8_t* pData, uint16_t Length)
{
  uint32_t RemainSpace = 0;
  
  if(w25qxxx == W25Q128)
  {
    RemainSpace = g_w25q128_Max_Addr - Addr;
  }
  else if(w25qxxx == W25Q256)
  {
    RemainSpace = g_w25q256_Max_Addr - Addr;
  }
  
  if(RemainSpace < Length)
  {
    w25qxxx_printf("There is not enough space left to write the specified length.");
    return ;
  }
  
  w25qxxx_MultiSectorProgram(w25qxxx, Addr, pData, Length);
}

void w25qxxx_SectorErase(uint8_t w25qxxx, uint32_t Addr)
{
  uint8_t SubAddr[4];
  
  if(w25qxxx == W25Q256)
  {
    SubAddr[0] = (Addr & 0xFF000000) >> 24;
  }
  SubAddr[1] = (Addr & 0x00FF0000) >> 16;
  SubAddr[2] = (Addr & 0x0000FF00) >> 8;
  SubAddr[3] = (Addr & 0x000000FF);
  
  w25qxxx_WriteEnable(w25qxxx);
  w25qxxx_CS_Low(w25qxxx);
  w25qxxx_PortTransmmit(w25qxxx, SECTOR_ERASE);
  if(w25qxxx == W25Q256)
  {
    w25qxxx_PortTransmmit(w25qxxx, SubAddr[0]);
  }
  w25qxxx_PortMultiTransmmit(w25qxxx, &SubAddr[1], 3);
  w25qxxx_CS_High(w25qxxx);
  
  w25qxxx_WaitForCmdEnd(w25qxxx);
}

void w25qxxx_BlockEraseWith32KB(uint8_t w25qxxx, uint32_t Addr)
{
  uint8_t SubAddr[4];
  
  if(w25qxxx == W25Q256)
  {
    SubAddr[0] = (Addr & 0xFF000000) >> 24;
  }
  SubAddr[1] = (Addr & 0x00FF0000) >> 16;
  SubAddr[2] = (Addr & 0x0000FF00) >> 8;
  SubAddr[3] = (Addr & 0x000000FF);
  
  w25qxxx_WriteEnable(w25qxxx);
  w25qxxx_CS_Low(w25qxxx);
  w25qxxx_PortTransmmit(w25qxxx, BLOCK_ERASE_32KB);
  if(w25qxxx == W25Q256)
  {
    w25qxxx_PortTransmmit(w25qxxx, SubAddr[0]);
  }
  w25qxxx_PortMultiTransmmit(w25qxxx, &SubAddr[1], 3);
  w25qxxx_CS_High(w25qxxx);
  
  w25qxxx_WaitForCmdEnd(w25qxxx);
}

void w25qxxx_BlockEraseWith64KB(uint8_t w25qxxx, uint32_t Addr)
{
  uint8_t SubAddr[4];
  
  if(w25qxxx == W25Q256)
  {
    SubAddr[0] = (Addr & 0xFF000000) >> 24;
  }
  SubAddr[1] = (Addr & 0x00FF0000) >> 16;
  SubAddr[2] = (Addr & 0x0000FF00) >> 8;
  SubAddr[3] = (Addr & 0x000000FF);
  
  w25qxxx_WriteEnable(w25qxxx);
  w25qxxx_CS_Low(w25qxxx);
  w25qxxx_PortTransmmit(w25qxxx, BLOCK_ERASE_64KB);
  if(w25qxxx == W25Q256)
  {
    w25qxxx_PortTransmmit(w25qxxx, SubAddr[0]);
  }
  w25qxxx_PortMultiTransmmit(w25qxxx, &SubAddr[1], 3);
  w25qxxx_CS_High(w25qxxx);
  
  w25qxxx_WaitForCmdEnd(w25qxxx);
}

void w25qxxx_ChipErase(uint8_t w25qxxx)
{
  w25qxxx_WriteEnable(w25qxxx);
  w25qxxx_TransCmd(w25qxxx, CHIP_ERASE);
  
  w25qxxx_WaitForCmdEnd(w25qxxx);
}

void w25qxxx_EraseProgram_Suspend(uint8_t w25qxxx)
{
  if(w25qxxx_Read_BUSY(w25qxxx) == 0 && w25qxxx_Read_SUS(w25qxxx) == 1)
  {
    return ;
  }
  w25qxxx_TransCmd(w25qxxx, ERASE_PROGRAM_SUSPEND);
  w25qxxx_Delayus(20);
}

void w25qxxx_EraseProgram_Resume(uint8_t w25qxxx)
{
  if(w25qxxx_Read_BUSY(w25qxxx) == 1 && w25qxxx_Read_SUS(w25qxxx) == 0)
  {
    return ;
  }
  w25qxxx_TransCmd(w25qxxx, ERASE_PROGRAM_RESUME);
}

void w25qxxx_Powerdown(uint8_t w25qxxx)
{
  w25qxxx_TransCmd(w25qxxx, POWER_DOWN);
  w25qxxx_Delayus(3);
}

void w25qxxx_ReleasePowerdown(uint8_t w25qxxx)
{
  w25qxxx_TransCmd(w25qxxx, RELEASE_POWERDOWN_DEVICE_ID);
  w25qxxx_Delayus(3);
}

uint8_t w25qxxx_ReleaseDeviceID(uint8_t w25qxxx)
{
  uint8_t Data = 0;
  
  w25qxxx_CS_Low(w25qxxx);
  w25qxxx_PortTransmmit(w25qxxx, RELEASE_POWERDOWN_DEVICE_ID);
  w25qxxx_PortTransmmit(w25qxxx, DUMMY_BYTE);
  w25qxxx_PortTransmmit(w25qxxx, DUMMY_BYTE);
  w25qxxx_PortTransmmit(w25qxxx, DUMMY_BYTE);
  Data = w25qxxx_PortReceive(w25qxxx);
  w25qxxx_CS_High(w25qxxx);
  
  return Data;
}

void w25qxxx_ReadManufacturer_DeviceID(uint8_t w25qxxx, uint8_t* ManufacturerID, uint8_t* DeviceID)
{
  w25qxxx_CS_Low(w25qxxx);
  w25qxxx_PortTransmmit(w25qxxx, READ_MANUFACTURER_DEVICE_ID);
  w25qxxx_PortTransmmit(w25qxxx, 0x00);
  w25qxxx_PortTransmmit(w25qxxx, 0x00);
  w25qxxx_PortTransmmit(w25qxxx, 0x00);
  *ManufacturerID = w25qxxx_PortReceive(w25qxxx);
  *DeviceID = w25qxxx_PortReceive(w25qxxx);
  w25qxxx_CS_High(w25qxxx);
}

void w25qxxx_ReadUniqueIDNumber(uint8_t w25qxxx, uint8_t* UniqueID)
{
  w25qxxx_CS_Low(w25qxxx);
  w25qxxx_PortTransmmit(w25qxxx, READ_UNIQUE_ID_NUMBER);
  if(w25qxxx == W25Q256)
  {
    if(w25qxxx_Read_ADS(w25qxxx) == 1)
    {
      w25qxxx_PortTransmmit(w25qxxx, DUMMY_BYTE);
    }
  }
  w25qxxx_PortTransmmit(w25qxxx, DUMMY_BYTE);
  w25qxxx_PortTransmmit(w25qxxx, DUMMY_BYTE);
  w25qxxx_PortTransmmit(w25qxxx, DUMMY_BYTE);
  w25qxxx_PortTransmmit(w25qxxx, DUMMY_BYTE);
  
  w25qxxx_PortMultiReceive(w25qxxx, UniqueID, 8);
  w25qxxx_CS_High(w25qxxx);
}

void w25qxxx_ReadJEDECID(uint8_t w25qxxx, uint8_t* JEDECID)
{
  w25qxxx_CS_Low(w25qxxx);
  w25qxxx_PortTransmmit(w25qxxx, READ_JEDEC_ID);
  
  w25qxxx_PortMultiReceive(w25qxxx, JEDECID, 3);
  w25qxxx_CS_High(w25qxxx);
}

void w25qxxx_ReadSFDPRegister(uint8_t w25qxxx, uint32_t Addr, uint8_t* pData, uint16_t Length)
{
  uint8_t SubAddr = 0;
  
  SubAddr = (Addr & 0x000000FF);
  
  w25qxxx_CS_Low(w25qxxx);
  w25qxxx_PortTransmmit(w25qxxx, READ_SFDP_REGISTER);
  w25qxxx_PortTransmmit(w25qxxx, 0x00);
  w25qxxx_PortTransmmit(w25qxxx, 0x00);
  w25qxxx_PortTransmmit(w25qxxx, SubAddr);
  w25qxxx_PortTransmmit(w25qxxx, DUMMY_BYTE);
  
  w25qxxx_PortMultiReceive(w25qxxx, pData, Length);
  w25qxxx_CS_High(w25qxxx);
}

void w25qxxx_EraseSecurityRegisters(uint8_t w25qxxx, uint8_t Addr)
{
  w25qxxx_WriteEnable(w25qxxx);
  w25qxxx_CS_Low(w25qxxx);
  w25qxxx_PortTransmmit(w25qxxx, ERASE_SECURITY_REGISTERS);  
  if(w25qxxx == W25Q256)
  {
    w25qxxx_PortTransmmit(w25qxxx, 0x00);
  }
  w25qxxx_PortTransmmit(w25qxxx, 0x00);
  w25qxxx_PortTransmmit(w25qxxx, Addr);
  w25qxxx_PortTransmmit(w25qxxx, 0x00);
  w25qxxx_CS_High(w25qxxx);
  
  w25qxxx_WaitForCmdEnd(w25qxxx);
}

//void w25qxxx_ProgramSecurityRegisters(uint8_t w25qxxx, uint32_t RegNum, uint32_t ByteAddr, uint8_t *pData, uint16_t Length)
//{
//  w25qxxx_WriteEnable(w25qxxx);
//  w25qxxx_CS_Low(w25qxxx);
//  w25qxxx_PortTransmmit(w25qxxx, PROGRAM_SECURITY_REGISTERS);
//  if(w25qxxx == W25Q256)
//  {
//    w25qxxx_PortTransmmit(w25qxxx, 0x00);
//  }
//  w25qxxx_PortTransmmit(w25qxxx, 0x00);
//  w25qxxx_PortTransmmit(w25qxxx, RegNum);
//  w25qxxx_PortTransmmit(w25qxxx, ByteAddr);
//  w25qxxx_CS_High(w25qxxx);  
//}
//
//void w25qxxx_ReadSecurityRegisters(uint8_t w25qxxx, uint8_t RegNum, uint8_t ByteAddr, uint8_t *pData, uint16_t Length)
//{
//  w25qxxx_CS_Low(w25qxxx);
//  w25qxxx_PortTransmmit(w25qxxx, READ_SECURITY_REGISTERS);
//  w25qxxx_PortTransmmit(w25qxxx, 0x00);
//  w25qxxx_PortTransmmit(w25qxxx, RegNum);
//  w25qxxx_PortTransmmit(w25qxxx, ByteAddr);
//  w25qxxx_PortTransmmit(w25qxxx, DUMMY_BYTE);
//  
//  for(uint32_t i = 0;i < Length;i++)
//  {
//    pData[i] = w25qxxx_PortReceive(w25qxxx);
//  }
//  
//  w25qxxx_CS_High(w25qxxx);
//}

void w25qxxx_EnterQPIMode(uint8_t w25qxxx)
{
  w25qxxx_Write_QE(w25qxxx, 1);
  w25qxxx_TransCmd(w25qxxx, ENTER_QPI_MODE);
}

void w25qxxx_ExitQPIMode(uint8_t w25qxxx)
{
  w25qxxx_TransCmd(w25qxxx, EXIT_QPI_MODE);
}

void w25qxxx_IndividualBlock_SectorLock(uint8_t w25qxxx, uint32_t Addr)
{
  uint8_t SubAddr[4];
  
  if(w25qxxx == W25Q256)
  {
    SubAddr[0] = (Addr & 0xFF000000) >> 24;
  }
  SubAddr[1] = (Addr & 0x00FF0000) >> 16;
  SubAddr[2] = (Addr & 0x0000FF00) >> 8;
  SubAddr[3] = (Addr & 0x000000FF);
  
  w25qxxx_Write_WPS(w25qxxx, 1);
  w25qxxx_CS_Low(w25qxxx);
  w25qxxx_PortTransmmit(w25qxxx, INDIVIDUAL_BLOCK_SECTOR_LOCK);
  if(w25qxxx == W25Q256)
  {
    w25qxxx_PortTransmmit(w25qxxx, SubAddr[0]);
  }
  w25qxxx_PortMultiTransmmit(w25qxxx, &SubAddr[1], 3);
  w25qxxx_CS_High(w25qxxx);
}

void w25qxxx_IndividualBlock_SectorUnlock(uint8_t w25qxxx, uint32_t Addr)
{
  uint8_t SubAddr[4];
  
  if(w25qxxx == W25Q256)
  {
    SubAddr[0] = (Addr & 0xFF000000) >> 24;
  }
  SubAddr[1] = (Addr & 0x00FF0000) >> 16;
  SubAddr[2] = (Addr & 0x0000FF00) >> 8;
  SubAddr[3] = (Addr & 0x000000FF);
  
  w25qxxx_Write_WPS(w25qxxx, 1);
  w25qxxx_CS_Low(w25qxxx);
  w25qxxx_PortTransmmit(w25qxxx, INDIVIDUAL_BLOCK_SECTOR_UNLOCK);
  if(w25qxxx == W25Q256)
  {
    w25qxxx_PortTransmmit(w25qxxx, SubAddr[0]);
  }
  w25qxxx_PortMultiTransmmit(w25qxxx, &SubAddr[1], 3);
  w25qxxx_CS_High(w25qxxx);
}

uint8_t w25qxxx_ReadBlock_SectorLock(uint8_t w25qxxx, uint32_t Addr)
{
  uint8_t Data = 0;
  uint8_t SubAddr[4];
  
  if(w25qxxx == W25Q256)
  {
    SubAddr[0] = (Addr & 0xFF000000) >> 24;
  }
  SubAddr[1] = (Addr & 0x00FF0000) >> 16;
  SubAddr[2] = (Addr & 0x0000FF00) >> 8;
  SubAddr[3] = (Addr & 0x000000FF);
  
  w25qxxx_Write_WPS(w25qxxx, 1);
  w25qxxx_CS_Low(w25qxxx);
  w25qxxx_PortTransmmit(w25qxxx, READ_BLOCK_SECTOR_LOCK);
  if(w25qxxx == W25Q256)
  {
    w25qxxx_PortTransmmit(w25qxxx, SubAddr[0]);
  }
  w25qxxx_PortMultiTransmmit(w25qxxx, &SubAddr[1], 3);
  Data = w25qxxx_PortReceive(w25qxxx);
  w25qxxx_CS_High(w25qxxx);
  
  return (Data & 0x01);
}

void w25qxxx_GlobalBlock_SectorLock(uint8_t w25qxxx)
{
  w25qxxx_WriteEnable(w25qxxx);
  w25qxxx_TransCmd(w25qxxx, GLOBAL_BLOCK_SECTOR_LOCK);
}

void w25qxxx_Global_Block_SectorUnlock(uint8_t w25qxxx)
{
  w25qxxx_WriteEnable(w25qxxx);
  w25qxxx_TransCmd(w25qxxx, GLOBAL_BLOCK_SECTOR_UNLOCK);
}

void w25qxxx_EnableReset(uint8_t w25qxxx)
{
  w25qxxx_TransCmd(w25qxxx, ENABLE_RESET);
}

uint8_t w25qxxx_SoftReset(uint8_t w25qxxx)
{
  if(w25qxxx_Read_BUSY(w25qxxx) == 1 || w25qxxx_Read_SUS(w25qxxx) == 1)
  {
    return false;
  }
  w25qxxx_EnableReset(w25qxxx);
  w25qxxx_TransCmd(w25qxxx, RESET_DEVICE);
  w25qxxx_Delayus(30);
  
  return true;
}

void w25qxxx_Init(uint8_t w25qxxx)
{
  w25qxxx_ReleasePowerdown(w25qxxx);
  w25qxxx_SoftReset(w25qxxx);
  w25qxxx_ReleaseDeviceID(w25qxxx);

  switch(w25qxxx)
  {
    case W25Q128:
    break;
    
    case W25Q256:
      if(w25qxxx_Read_ADS(w25qxxx) == 0)
      {
        w25q256_Enter4ByteAddressMode(w25qxxx);
      }
    break;
    
    default:
    break;
  }
}

void w25qxxx_ReadInfo(uint8_t w25qxxx)
{
  uint8_t ManufacturerID = 0, DeviceID = 0;
  uint8_t JEDECID[3];
  uint8_t UniqueID[8];
  
  w25qxxx_printf("g_w25qxxx is 0x%02X.", w25qxxx);
  w25qxxx_ReadJEDECID(w25qxxx, JEDECID);
  w25qxxx_printf("JEDEC ID is 0x%02X%02X%02X", JEDECID[0], JEDECID[1], JEDECID[2]);
  w25qxxx_ReadManufacturer_DeviceID(w25qxxx, &ManufacturerID, &DeviceID);
  w25qxxx_printf("Manufacturer ID is 0x%02X, Device ID is 0x%02X.", ManufacturerID, DeviceID);
  w25qxxx_ReadUniqueIDNumber(w25qxxx, UniqueID);
  w25qxxx_printf("Unique ID is %02X%02X%02X%02X%02X%02X%02X%02X", UniqueID[0], UniqueID[1], UniqueID[2], UniqueID[3],\
                                                                  UniqueID[4], UniqueID[5], UniqueID[6], UniqueID[7]);
}

#if 0
static uint8_t Tx_Buffer[32767];
static uint8_t Rx_Buffer[32767];

void w25qxxx_Test(uint8_t w25qxxx)
{
//  uint8_t *Tx_Buffer = NULL;
//  uint8_t *Rx_Buffer = NULL;
  uint32_t TmpRngdata = 0;
  uint16_t BufferLength = 0;
  uint32_t TestAddr = 0;
  
  HAL_RNG_GenerateRandomNumber(&hrng, &TmpRngdata);
  BufferLength = TmpRngdata & 0x7FFF;
  w25qxxx_printf("BufferLength = %d.", BufferLength);
//  Tx_Buffer = (uint8_t*)malloc(BufferLength);
//  Rx_Buffer = (uint8_t*)malloc(BufferLength);
  
  if(Tx_Buffer == NULL || Rx_Buffer == NULL)
  {
    w25qxxx_printf("Failed to allocate memory !");
    return;
  }
  memset(Tx_Buffer, 0, BufferLength);
  memset(Rx_Buffer, 0, 1);
  
  HAL_RNG_GenerateRandomNumber(&hrng, &TmpRngdata);
  
  switch(g_w25qxxx)
  {
    case W25Q128:
      TestAddr = TmpRngdata & 0xFFFFFF;
    break;
    
    case W25Q256:
      TestAddr = TmpRngdata & 0x1FFFFFF;
    break;
    
    default:
      TestAddr = 0;
    break;
  }

  w25qxxx_printf("TestAddr = 0x%08X.", TestAddr);

  for(uint16_t i = 0;i < BufferLength;i += 4)
  {
    HAL_RNG_GenerateRandomNumber(&hrng, &TmpRngdata);
    Tx_Buffer[i + 3] = (TmpRngdata & 0xFF000000) >> 24;;
    Tx_Buffer[i + 2] = (TmpRngdata & 0x00FF0000) >> 16;
    Tx_Buffer[i + 1] = (TmpRngdata & 0x0000FF00) >> 8;
    Tx_Buffer[i + 0] = (TmpRngdata & 0x000000FF);
    
//    w25qxxx_printf("Tx_Buffer[%d] = 0x%02X, Tx_Buffer[%d] = 0x%02X, Tx_Buffer[%d] = 0x%02X, Tx_Buffer[%d] = 0x%02X,",\
//                   i + 0, Tx_Buffer[i + 0],\
//                   i + 1, Tx_Buffer[i + 1],\
//                   i + 2, Tx_Buffer[i + 2],\
//                   i + 3, Tx_Buffer[i + 3]);
  }
  
  w25qxxx_WriteData(w25qxxx, TestAddr, Tx_Buffer, BufferLength);
  w25qxxx_ReadData(w25qxxx, TestAddr, Rx_Buffer, BufferLength);
  
  for(uint16_t i = 0;i < BufferLength;i++)
  {
//    w25qxxx_printf("Tx_Buffer[%d] = 0x%02X, Rx_Buffer[%d] = 0x%02X", 
//                   i, Tx_Buffer[i],
//                   i, Rx_Buffer[i]);
    if(Tx_Buffer[i] != Rx_Buffer[i])
    {
      w25qxxx_printf("Tx_Buffer[%d] = 0x%02X, Rx_Buffer[%d] = 0x%02X", 
                     i, Tx_Buffer[i],
                     i, Rx_Buffer[i]);
      w25qxxx_printf("Data writes and reads do not match, TEST FAILED !");
//      free(Tx_Buffer);
//      free(Rx_Buffer);
      return ;
    }
  }
//  free(Tx_Buffer);
//  free(Rx_Buffer);
  
  w25qxxx_printf("w25qxxx Read and write TEST PASSED !");
}
#endif
