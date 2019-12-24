#include "w25q128/w25q128.h"

/*The following program is modified by the user according to the hardware device, otherwise the driver cannot run.*/

/**
  * @step 1:  Modify the corresponding function according to the modified area and the corresponding function name.
  * @step 2:  Modify four areas: GPIO_PORT/GPIO_PIN/Led_TypeDef/LEDn.
  * @step 3:  .
  * @step 4:  .
  * @step 5:
  */

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
#define READ_DATA                       0x03
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

void w25q128_PortTransmmit(uint8_t Data)
{
  
}

uint8_t w25q128_PortReceive(void)
{
  uint8_t Data = 0;
  
  
  return Data;
}

void w25q128_CS_Low(void)
{
  
}

void w25q128_CS_High(void)
{
  
}

void w25q128_HardReset(void)
{
  
}

void w25q128_Delayus(uint32_t ticks)
{
  
}

void w25q128_Delayms(uint32_t ticks)
{
  
}
/*The above procedure is modified by the user according to the hardware device, otherwise the driver cannot run.*/

void w25q128_TransCmd(uint8_t Cmd)
{
  w25q128_CS_Low();
  w25q128_PortTransmmit(Cmd);
  w25q128_CS_High();
}

uint8_t w25q128_Receive(void)
{
  uint8_t Data = 0;
  
  w25q128_CS_Low();
  Data = w25q128_PortReceive();
  w25q128_CS_High();
  
  return Data;
}

uint8_t w25q128_Read_BUSY(void)
{
  uint8_t Data = 0;
  
  Data = w25q128_ReadStatusRegister(READ_STATUS_REGISTER1);
  Data &= 0x01;
  
  return Data;
}

uint8_t w25q128_Read_WEL(void)
{
  uint8_t Data = 0;
  
  Data = w25q128_ReadStatusRegister(READ_STATUS_REGISTER1);
  Data >>= 1;
  Data &= 0x01;
  
  return Data;
}

uint8_t w25q128_Read_BP0(void)
{
  uint8_t Data = 0;
  
  Data = w25q128_ReadStatusRegister(READ_STATUS_REGISTER1);
  Data >>= 2;
  Data &= 0x01;
  
  return Data;
}

uint8_t w25q128_Read_BP1(void)
{
  uint8_t Data = 0;
  
  Data = w25q128_ReadStatusRegister(READ_STATUS_REGISTER1);
  Data >>= 4;
  Data &= 0x01;
  
  return Data;
}

uint8_t w25q128_Read_BP2(void)
{
  uint8_t Data = 0;
  
  Data = w25q128_ReadStatusRegister(READ_STATUS_REGISTER1);
  Data >>= 5;
  Data &= 0x01;
  
  return Data;
}

uint8_t w25q128_Read_SEC(void)
{
  uint8_t Data = 0;
  
  Data = w25q128_ReadStatusRegister(READ_STATUS_REGISTER1);
  Data >>= 6;
  Data &= 0x01;
  
  return Data;
}

uint8_t w25q128_Read_SRP0(void)
{
  uint8_t Data = 0;
  
  Data = w25q128_ReadStatusRegister(READ_STATUS_REGISTER1);
  Data >>= 7;
  Data &= 0x01;
  
  return Data;
}

uint8_t w25q128_Read_SRP1(void)
{
  uint8_t Data = 0;
  
  Data = w25q128_ReadStatusRegister(READ_STATUS_REGISTER2);
  Data &= 0x01;
  
  return Data;
}

uint8_t w25q128_Read_QE(void)
{
  uint8_t Data = 0;
  
  Data = w25q128_ReadStatusRegister(READ_STATUS_REGISTER2);
  Data >>= 1;
  Data &= 0x01;
  
  return Data;
}

uint8_t w25q128_Read_LB1(void)
{
  uint8_t Data = 0;
  
  Data = w25q128_ReadStatusRegister(READ_STATUS_REGISTER2);
  Data >>= 3;
  Data &= 0x01;
  
  return Data;
}

uint8_t w25q128_Read_LB2(void)
{
  uint8_t Data = 0;
  
  Data = w25q128_ReadStatusRegister(READ_STATUS_REGISTER2);
  Data >>= 4;
  Data &= 0x01;
  
  return Data;
}

uint8_t w25q128_Read_LB3(void)
{
  uint8_t Data = 0;
  
  Data = w25q128_ReadStatusRegister(READ_STATUS_REGISTER2);
  Data >>= 5;
  Data &= 0x01;
  
  return Data;
}

uint8_t w25q128_Read_CMP(void)
{
  uint8_t Data = 0;
  
  Data = w25q128_ReadStatusRegister(READ_STATUS_REGISTER2);
  Data >>= 6;
  Data &= 0x01;
  
  return Data;
}

uint8_t w25q128_Read_SUS(void)
{
  uint8_t Data = 0;
  
  Data = w25q128_ReadStatusRegister(READ_STATUS_REGISTER2);
  Data >>= 7;
  Data &= 0x01;
  
  return Data;
}

uint8_t w25q128_Read_WPS(void)
{
  uint8_t Data = 0;
  
  Data = w25q128_ReadStatusRegister(READ_STATUS_REGISTER3);
  Data >>= 2;
  Data &= 0x01;
  
  return Data;
}

uint8_t w25q128_Read_DRV0(void)
{
  uint8_t Data = 0;
  
  Data = w25q128_ReadStatusRegister(READ_STATUS_REGISTER3);
  Data >>= 5;
  Data &= 0x01;
  
  return Data;
}

uint8_t w25q128_Read_DRV1(void)
{
  uint8_t Data = 0;
  
  Data = w25q128_ReadStatusRegister(READ_STATUS_REGISTER3);
  Data >>= 6;
  Data &= 0x01;
  
  return Data;
}

uint8_t w25q128_Read_HOLD_RST(void)
{
  uint8_t Data = 0;
  
  Data = w25q128_ReadStatusRegister(READ_STATUS_REGISTER3);
  Data >>= 7;
  Data &= 0x01;
  
  return Data;
}

void w25q128_Write_SRP0(uint8_t Data)
{
  w25q128_TransCmd(WRITE_ENABLE);
  w25q128_WriteStatusRegister(WRITE_STATUS_REGISTER1, Data);
}

void w25q128_Write_SEC(uint8_t Data)
{
  w25q128_TransCmd(WRITE_ENABLE);
  w25q128_WriteStatusRegister(WRITE_STATUS_REGISTER1, Data);
}

void w25q128_Write_TB(uint8_t Data)
{
  w25q128_TransCmd(WRITE_ENABLE);
  w25q128_WriteStatusRegister(WRITE_STATUS_REGISTER1, Data);
}

void w25q128_Write_BP(uint8_t Data)
{
  w25q128_TransCmd(WRITE_ENABLE);
  w25q128_WriteStatusRegister(WRITE_STATUS_REGISTER1, Data);
}

void w25q128_Write_CMP(uint8_t Data)
{
  w25q128_TransCmd(WRITE_ENABLE);
  w25q128_WriteStatusRegister(WRITE_STATUS_REGISTER2, Data);
}

void w25q128_Write_LB(uint8_t Data)
{
  w25q128_TransCmd(WRITE_ENABLE);
  w25q128_WriteStatusRegister(WRITE_STATUS_REGISTER2, Data);
}

void w25q128_Write_QE(uint8_t Data)
{
  w25q128_TransCmd(WRITE_ENABLE);
  w25q128_WriteStatusRegister(WRITE_STATUS_REGISTER2, Data);
}

void w25q128_Write_SRP1(uint8_t Data)
{
  w25q128_TransCmd(WRITE_ENABLE);
  w25q128_WriteStatusRegister(WRITE_STATUS_REGISTER2, Data);
}

void w25q128_Write_HOLD_RST(uint8_t Data)
{
  w25q128_TransCmd(WRITE_ENABLE);
  w25q128_WriteStatusRegister(WRITE_STATUS_REGISTER3, Data);
}

void w25q128_Write_DRV1(uint8_t Data)
{
  w25q128_TransCmd(WRITE_ENABLE);
  w25q128_WriteStatusRegister(WRITE_STATUS_REGISTER3, Data);
}

void w25q128_Write_DRV0(uint8_t Data)
{
  w25q128_TransCmd(WRITE_ENABLE);
  w25q128_WriteStatusRegister(WRITE_STATUS_REGISTER3, Data);
}

void w25q128_Write_WPS(uint8_t Data)
{
  w25q128_TransCmd(WRITE_ENABLE);
  w25q128_WriteStatusRegister(WRITE_STATUS_REGISTER3, Data);
}

void w25q128_Write_ADP(uint8_t Data)
{
  w25q128_TransCmd(WRITE_ENABLE);
  w25q128_WriteStatusRegister(WRITE_STATUS_REGISTER3, Data);
}

void w25q128_WriteEnable(void)
{
  w25q128_TransCmd(WRITE_ENABLE);
}

void w25q128_WriteEnableForVolatile(void)
{
  w25q128_TransCmd(WRITE_ENABLE_FORVOLATILE);
}

void w25q128_WriteDisable(void)
{
  w25q128_TransCmd(WRITE_DISABLE);
}

uint8_t w25q128_ReadStatusRegister(uint8_t Number)
{
  uint8_t Data = 0;
  
  w25q128_CS_Low();
  switch(Number)
  {
    case READ_STATUS_REGISTER1:
      w25q128_PortTransmmit(READ_STATUS_REGISTER1);
      Data = w25q128_PortReceive();
      break;
      
    case READ_STATUS_REGISTER2:
      w25q128_PortTransmmit(READ_STATUS_REGISTER2);
      Data = w25q128_PortReceive();
      break;
      
    case READ_STATUS_REGISTER3:
      w25q128_PortTransmmit(READ_STATUS_REGISTER3);
      Data = w25q128_PortReceive();
      break;
      
    default:break;
  }
  w25q128_CS_High();
  
  return Data;
}

void w25q128_WriteStatusRegister(uint8_t Number, uint8_t Data)
{
  w25q128_WriteEnable();
  
  w25q128_CS_Low();
  switch(Number)
  {
    case WRITE_STATUS_REGISTER1:
      w25q128_PortTransmmit(WRITE_STATUS_REGISTER1);
      w25q128_PortTransmmit(Data);
      break;
      
    case WRITE_STATUS_REGISTER2:
      w25q128_PortTransmmit(WRITE_STATUS_REGISTER2);
      w25q128_PortTransmmit(Data);
      break;
      
    case WRITE_STATUS_REGISTER3:
      w25q128_PortTransmmit(WRITE_STATUS_REGISTER3);
      w25q128_PortTransmmit(Data);
      break;
      
    default:break;
  }
  w25q128_CS_High();
}

void w25q128_ReadData(uint32_t Addr, uint8_t *pData, uint32_t Length)
{
  uint8_t SubAddr1 = 0;
  uint8_t SubAddr2 = 0;
  uint8_t SubAddr3 = 0;
  
  SubAddr1 = (Addr & 0x00FF0000) >> 16;
  SubAddr2 = (Addr & 0x0000FF00) >> 8;
  SubAddr3 = (Addr & 0x000000FF);
  
  w25q128_CS_Low();
  w25q128_PortTransmmit(READ_DATA);
  w25q128_PortTransmmit(SubAddr1);
  w25q128_PortTransmmit(SubAddr2);
  w25q128_PortTransmmit(SubAddr3);
  
  for(uint32_t i = 0;i < Length;i++)
  {
    pData[i] = w25q128_PortReceive();
  }
  
  w25q128_CS_High();
}

void w25q128_PageProgram(uint32_t Addr, uint8_t *pData, uint16_t Length)
{
  uint8_t SubAddr1 = 0;
  uint8_t SubAddr2 = 0;
  uint8_t SubAddr3 = 0;
  uint16_t Page_RemainLength = 0;
  uint16_t NextPage_WriteLength = 0;
  
  Page_RemainLength = 256 - SubAddr3;
  if(Page_RemainLength >= Length)
  { 
    SubAddr1 = (Addr & 0x00FF0000) >> 16;
    SubAddr2 = (Addr & 0x0000FF00) >> 8;
    SubAddr3 = (Addr & 0x000000FF);
    
    w25q128_WriteEnable();
    w25q128_CS_Low();
    w25q128_PortTransmmit(PAGE_PROGRAM);
    w25q128_PortTransmmit(SubAddr1);
    w25q128_PortTransmmit(SubAddr2);
    w25q128_PortTransmmit(SubAddr3);
    
    for(uint16_t i = 0;i < Length;i++)
    {
      w25q128_PortTransmmit(pData[i]);
    }
    
    w25q128_CS_High();
  }
  else
  {
    SubAddr1 = (Addr & 0x00FF0000) >> 16;
    SubAddr2 = (Addr & 0x0000FF00) >> 8;
    SubAddr3 = (Addr & 0x000000FF);
    
    w25q128_WriteEnable();
    w25q128_CS_Low();
    w25q128_PortTransmmit(PAGE_PROGRAM);
    w25q128_PortTransmmit(SubAddr1);
    w25q128_PortTransmmit(SubAddr2);
    w25q128_PortTransmmit(SubAddr3);
    
    for(uint16_t i = 0;i < Page_RemainLength;i++)
    {
      w25q128_PortTransmmit(pData[i]);
    }
    
    w25q128_CS_High();
    
    NextPage_WriteLength = Length - (256 - SubAddr3);

    Addr += 0x100;
    SubAddr1 = (Addr & 0x00FF0000) >> 16;
    SubAddr2 = (Addr & 0x0000FF00) >> 8;
    SubAddr3 = (Addr & 0x000000FF);
    
    w25q128_WriteEnable();
    w25q128_CS_Low();
    w25q128_PortTransmmit(PAGE_PROGRAM);
    w25q128_PortTransmmit(SubAddr1);
    w25q128_PortTransmmit(SubAddr2);
    w25q128_PortTransmmit(SubAddr3);
    
    for(uint16_t i = Page_RemainLength;i < NextPage_WriteLength;i++)
    {
      w25q128_PortTransmmit(pData[i]);
    }
    
    w25q128_CS_High();
  }
}

void w25q128_SectorErase(uint32_t Addr)
{
  uint8_t SubAddr1 = 0;
  uint8_t SubAddr2 = 0;
  uint8_t SubAddr3 = 0;
  
  SubAddr1 = (Addr & 0x00FF0000) >> 16;
  SubAddr2 = (Addr & 0x0000FF00) >> 8;
  SubAddr3 = (Addr & 0x000000FF);
  
  w25q128_WriteEnable();
  w25q128_CS_Low();
  w25q128_PortTransmmit(SECTOR_ERASE);
  w25q128_PortTransmmit(SubAddr1);
  w25q128_PortTransmmit(SubAddr2);
  w25q128_PortTransmmit(SubAddr3);
  w25q128_CS_High();
}

void w25q128_BlockEraseWith32KB(uint32_t Addr)
{
  uint8_t SubAddr1 = 0;
  uint8_t SubAddr2 = 0;
  uint8_t SubAddr3 = 0;
  
  SubAddr1 = (Addr & 0x00FF0000) >> 16;
  SubAddr2 = (Addr & 0x0000FF00) >> 8;
  SubAddr3 = (Addr & 0x000000FF);
  
  w25q128_WriteEnable();
  w25q128_CS_Low();
  w25q128_PortTransmmit(BLOCK_ERASE_32KB);
  w25q128_PortTransmmit(SubAddr1);
  w25q128_PortTransmmit(SubAddr2);
  w25q128_PortTransmmit(SubAddr3);
  w25q128_CS_High();
}

void w25q128_BlockEraseWith64KB(uint32_t Addr)
{
  uint8_t SubAddr1 = 0;
  uint8_t SubAddr2 = 0;
  uint8_t SubAddr3 = 0;
  
  SubAddr1 = (Addr & 0x00FF0000) >> 16;
  SubAddr2 = (Addr & 0x0000FF00) >> 8;
  SubAddr3 = (Addr & 0x000000FF);
  
  w25q128_WriteEnable();
  w25q128_CS_Low();
  w25q128_PortTransmmit(BLOCK_ERASE_64KB);
  w25q128_PortTransmmit(SubAddr1);
  w25q128_PortTransmmit(SubAddr2);
  w25q128_PortTransmmit(SubAddr3);
  w25q128_CS_High();
}

void w25q128_ChipErase(void)
{
  w25q128_WriteEnable();
  w25q128_TransCmd(CHIP_ERASE);
}

void w25q128_EraseProgram_Suspend(void)
{
  w25q128_TransCmd(ERASE_PROGRAM_SUSPEND);
}

void w25q128_EraseProgram_Resume(void)
{
  w25q128_TransCmd(ERASE_PROGRAM_RESUME);
}

void w25q128_Powerdown(void)
{
  w25q128_TransCmd(POWER_DOWN);
}

void w25q128_ReleasePowerdown(void)
{
  w25q128_TransCmd(RELEASE_POWERDOWN_DEVICE_ID);
}

uint8_t w25q128_ReleaseDeviceID(void)
{
  uint8_t Data = 0;
  
  w25q128_CS_Low();
  w25q128_PortTransmmit(RELEASE_POWERDOWN_DEVICE_ID);
  w25q128_PortTransmmit(DUMMY_BYTE);
  w25q128_PortTransmmit(DUMMY_BYTE);
  w25q128_PortTransmmit(DUMMY_BYTE);
  Data = w25q128_PortReceive();
  w25q128_CS_High();
  
  return Data;
}

void w25q128_ReadManufacturer_DeviceID(uint8_t* ManufacturerID, uint8_t* DeviceID)
{
  w25q128_CS_Low();
  w25q128_PortTransmmit(READ_MANUFACTURER_DEVICE_ID);
  w25q128_PortTransmmit(0x00);
  w25q128_PortTransmmit(0x00);
  w25q128_PortTransmmit(0x00);
  *ManufacturerID = w25q128_PortReceive();
  *DeviceID = w25q128_PortReceive();
  w25q128_CS_High();
}

void w25q128_ReadUniqueIDNumber(uint8_t* UniqueID)
{
  w25q128_CS_Low();
  w25q128_PortTransmmit(READ_UNIQUE_ID_NUMBER);
  w25q128_PortTransmmit(DUMMY_BYTE);
  w25q128_PortTransmmit(DUMMY_BYTE);
  w25q128_PortTransmmit(DUMMY_BYTE);
  w25q128_PortTransmmit(DUMMY_BYTE);
  
  for(uint8_t i = 0;i < 4;i++)
  {
    UniqueID[i] = w25q128_PortReceive();
  }
    
  w25q128_CS_High();
}

void w25q128_ReadJEDECID(uint8_t* JEDECID)
{
  w25q128_CS_Low();
  w25q128_PortTransmmit(READ_JEDEC_ID);
  
  for(uint8_t i = 0;i < 4;i++)
  {
    JEDECID[i] = w25q128_PortReceive();
  }
    
  w25q128_CS_High();
}

void w25q128_ReadSFDPRegister(uint32_t Addr, uint8_t* pData, uint16_t Length)
{
  uint8_t SubAddr = 0;
  
  SubAddr = (Addr & 0x000000FF);
  
  w25q128_CS_Low();
  w25q128_PortTransmmit(READ_SFDP_REGISTER);
  w25q128_PortTransmmit(0x00);
  w25q128_PortTransmmit(0x00);
  w25q128_PortTransmmit(SubAddr);
  w25q128_PortTransmmit(DUMMY_BYTE);
  
  for(uint16_t i = 0;i < Length;i++)
  {
    pData[i] = w25q128_PortReceive();
  }
    
  w25q128_CS_High();
}

void w25q128_EraseSecurityRegisters(uint8_t Addr)
{
  w25q128_WriteEnable();
  w25q128_CS_Low();
  w25q128_PortTransmmit(ERASE_SECURITY_REGISTERS);
  w25q128_PortTransmmit(0x00);
  w25q128_PortTransmmit(Addr);
  w25q128_PortTransmmit(0x00);
  w25q128_CS_High();
}

void w25q128_ProgramSecurityRegisters(uint8_t RegNum, uint8_t ByteAddr, uint8_t *pData, uint16_t Length)
{
  uint32_t Addr;
    
  w25q128_WriteEnable();
  w25q128_CS_Low();
  w25q128_PortTransmmit(PROGRAM_SECURITY_REGISTERS);
  w25q128_PortTransmmit(0x00);
  w25q128_PortTransmmit(RegNum);
  w25q128_PortTransmmit(ByteAddr);
  w25q128_CS_High();  
  
  uint8_t SubAddr1 = 0;
  uint8_t SubAddr2 = 0;
  uint8_t SubAddr3 = 0;
  uint16_t Page_RemainLength = 0;
  uint16_t NextPage_WriteLength = 0;
  
  Page_RemainLength = 256 - SubAddr3;
  if(Page_RemainLength >= Length)
  { 
    SubAddr1 = (Addr & 0x00FF0000) >> 16;
    SubAddr2 = (Addr & 0x0000FF00) >> 8;
    SubAddr3 = (Addr & 0x000000FF);
    
    w25q128_WriteEnable();
    w25q128_CS_Low();
    w25q128_PortTransmmit(PAGE_PROGRAM);
    w25q128_PortTransmmit(SubAddr1);
    w25q128_PortTransmmit(SubAddr2);
    w25q128_PortTransmmit(SubAddr3);
    
    for(uint16_t i = 0;i < Length;i++)
    {
      w25q128_PortTransmmit(pData[i]);
    }
    
    w25q128_CS_High();
  }
  else
  {
    SubAddr1 = (Addr & 0x00FF0000) >> 16;
    SubAddr2 = (Addr & 0x0000FF00) >> 8;
    SubAddr3 = (Addr & 0x000000FF);
    
    w25q128_WriteEnable();
    w25q128_CS_Low();
    w25q128_PortTransmmit(PAGE_PROGRAM);
    w25q128_PortTransmmit(SubAddr1);
    w25q128_PortTransmmit(SubAddr2);
    w25q128_PortTransmmit(SubAddr3);
    
    for(uint16_t i = 0;i < Page_RemainLength;i++)
    {
      w25q128_PortTransmmit(pData[i]);
    }
    
    w25q128_CS_High();
    
    NextPage_WriteLength = Length - (256 - SubAddr3);

    Addr += 0x100;
    SubAddr1 = (Addr & 0x00FF0000) >> 16;
    SubAddr2 = (Addr & 0x0000FF00) >> 8;
    SubAddr3 = (Addr & 0x000000FF);
    
    w25q128_WriteEnable();
    w25q128_CS_Low();
    w25q128_PortTransmmit(PAGE_PROGRAM);
    w25q128_PortTransmmit(SubAddr1);
    w25q128_PortTransmmit(SubAddr2);
    w25q128_PortTransmmit(SubAddr3);
    
    for(uint16_t i = Page_RemainLength;i < NextPage_WriteLength;i++)
    {
      w25q128_PortTransmmit(pData[i]);
    }
    
    w25q128_CS_High();
  }
}

void w25q128_ReadSecurityRegisters(uint8_t RegNum, uint8_t ByteAddr, uint8_t *pData, uint16_t Length)
{
  w25q128_CS_Low();
  w25q128_PortTransmmit(READ_SECURITY_REGISTERS);
  w25q128_PortTransmmit(0x00);
  w25q128_PortTransmmit(RegNum);
  w25q128_PortTransmmit(ByteAddr);
  w25q128_PortTransmmit(DUMMY_BYTE);
  
  for(uint32_t i = 0;i < Length;i++)
  {
    pData[i] = w25q128_PortReceive();
  }
  
  w25q128_CS_High();
}

void w25q128_EnterQPIMode(void)
{
//  ();
  w25q128_TransCmd(ENTER_QPI_MODE);
}

void w25q128_ExitQPIMode(void)
{
  w25q128_TransCmd(EXIT_QPI_MODE);
}

void w25q128_IndividualBlock_SectorLock(uint32_t Addr)
{
  uint8_t SubAddr1 = 0;
  uint8_t SubAddr2 = 0;
  uint8_t SubAddr3 = 0;
  
  SubAddr1 = (Addr & 0x00FF0000) >> 16;
  SubAddr2 = (Addr & 0x0000FF00) >> 8;
  SubAddr3 = (Addr & 0x000000FF);
  
//  ();
  w25q128_WriteEnable();
  w25q128_CS_Low();
  w25q128_PortTransmmit(INDIVIDUAL_BLOCK_SECTOR_LOCK);
  w25q128_PortTransmmit(SubAddr1);
  w25q128_PortTransmmit(SubAddr2);
  w25q128_PortTransmmit(SubAddr3);
  w25q128_CS_High();
}

void w25q128_IndividualBlock_SectorUnlock(uint32_t Addr)
{
  uint8_t SubAddr1 = 0;
  uint8_t SubAddr2 = 0;
  uint8_t SubAddr3 = 0;
  
  SubAddr1 = (Addr & 0x00FF0000) >> 16;
  SubAddr2 = (Addr & 0x0000FF00) >> 8;
  SubAddr3 = (Addr & 0x000000FF);
  
//  ();
  w25q128_WriteEnable();
  w25q128_CS_Low();
  w25q128_PortTransmmit(INDIVIDUAL_BLOCK_SECTOR_UNLOCK);
  w25q128_PortTransmmit(SubAddr1);
  w25q128_PortTransmmit(SubAddr2);
  w25q128_PortTransmmit(SubAddr3);
  w25q128_CS_High();
}

uint8_t w25q128_ReadBlock_SectorLock(uint32_t Addr)
{
  uint8_t SubAddr1 = 0;
  uint8_t SubAddr2 = 0;
  uint8_t SubAddr3 = 0;
  uint8_t Data = 0;
  
  SubAddr1 = (Addr & 0x00FF0000) >> 16;
  SubAddr2 = (Addr & 0x0000FF00) >> 8;
  SubAddr3 = (Addr & 0x000000FF);
  
//  ();
  w25q128_CS_Low();
  w25q128_PortTransmmit(READ_BLOCK_SECTOR_LOCK);
  w25q128_PortTransmmit(SubAddr1);
  w25q128_PortTransmmit(SubAddr2);
  w25q128_PortTransmmit(SubAddr3);
  Data = w25q128_PortReceive();
  w25q128_CS_High();
  
  return Data;
}

void w25q128_GlobalBlock_SectorLock(void)
{
  w25q128_WriteEnable();
  w25q128_TransCmd(GLOBAL_BLOCK_SECTOR_LOCK);
}

void w25q128_Global_Block_SectorUnlock(void)
{
  w25q128_WriteEnable();
  w25q128_TransCmd(GLOBAL_BLOCK_SECTOR_UNLOCK);
}

void w25q128_EnableReset(void)
{
  w25q128_TransCmd(ENABLE_RESET);
}

void w25q128_SoftReset(void)
{
  w25q128_EnableReset();
  w25q128_TransCmd(RESET_DEVICE);
}
