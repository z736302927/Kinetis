#include "i2c_soft.h"

static uint8_t I2C_FastMode = 0;
/*The following program is modified by the user according to the hardware device, otherwise the driver cannot run.*/

void I2c_Soft_Init(void)
{

}

void I2c_Soft_delay(void)
{ 
//  Delay_us(5);
}

/*The above procedure is modified by the user according to the hardware device, otherwise the driver cannot run.*/


static int I2c_Soft_Start(void)
{
  SDA_OUT;
  SDA_H;
  SCL_H;
  I2c_Soft_delay();
  if(!SDA_READ)
    return 0;  
  SDA_L;
  I2c_Soft_delay();
  if(SDA_READ) 
    return 0;
  SDA_L;
  I2c_Soft_delay();
  return 1;
}

static void I2c_Soft_Stop(void)
{
  SDA_OUT;
  SCL_L;
  I2c_Soft_delay();
  SDA_L;
  I2c_Soft_delay();
  SCL_H;
  I2c_Soft_delay();
  SDA_H;
  I2c_Soft_delay();
}

static void I2c_Soft_Ask(void)
{
  SCL_L;
  I2c_Soft_delay();
  SDA_OUT;
  SDA_L;
  I2c_Soft_delay();
  SCL_H;
  I2c_Soft_delay();
  SCL_L;
  I2c_Soft_delay();
}

static void I2c_Soft_NoAsk(void)
{
  SCL_L;
  I2c_Soft_delay();
  SDA_OUT;
  SDA_H;
  I2c_Soft_delay();
  SCL_H;
  I2c_Soft_delay();
  SCL_L;
  I2c_Soft_delay();
}

static int I2c_Soft_WaitAsk(void)
{
  uint8_t ErrTime = 0;
  SCL_L;
  I2c_Soft_delay();
  SDA_IN;
  SDA_H;      
  I2c_Soft_delay();
  SCL_H;
  I2c_Soft_delay();
  while(SDA_READ)
  {
    ErrTime++;
    if(ErrTime>50)
    {
      I2c_Soft_Stop();
      return 1;
    }
  }
  SCL_L;
  I2c_Soft_delay();
  return 0;
}

static void I2c_Soft_SendByte(uint8_t SendByte)
{
  uint8_t i=8;
  
  SDA_OUT;
  while(i--)
  {
    SCL_L;
    I2c_Soft_delay();
    if(SendByte & 0x80)
      SDA_H;  
    else 
      SDA_L;   
    SendByte <<= 1;
    I2c_Soft_delay();
    SCL_H;
    I2c_Soft_delay();
  }
  SCL_L;
}  

static uint8_t I2c_Soft_ReadByte(uint8_t ask)
{ 
  uint8_t i=8;
  uint8_t ReceiveByte=0;

  SDA_IN;
  I2c_Soft_delay();
  SDA_H;        
  while(i--)
  {
    ReceiveByte<<=1;      
    SCL_L;
    I2c_Soft_delay();
    SCL_H;
    I2c_Soft_delay();  
    if(SDA_READ)
      ReceiveByte|=0x01;
  }
  SCL_L;

  if (ask)
    I2c_Soft_Ask();
  else
    I2c_Soft_NoAsk();  
  return ReceiveByte;
} 

uint8_t IIC_Write_1Byte(uint8_t SlaveAddress,uint16_t REG_Address,uint8_t REG_data)
{
  I2c_Soft_Start();
  I2c_Soft_SendByte(SlaveAddress << 1);   
  if(I2c_Soft_WaitAsk())
  {
    I2c_Soft_Stop();
    return 1;
  }
  if(ADDRESS_MODE == ADDRESS_16)
  {
    I2c_Soft_SendByte(REG_Address >> 8);       
    I2c_Soft_WaitAsk();
  }
  I2c_Soft_SendByte(REG_Address & 0xFF);       
  I2c_Soft_WaitAsk();  
  I2c_Soft_SendByte(REG_data);
  I2c_Soft_WaitAsk();   
  I2c_Soft_Stop(); 
  return 0;
}

uint8_t IIC_Read_1Byte(uint8_t SlaveAddress,uint16_t REG_Address,uint8_t *REG_data)
{          
  I2c_Soft_Start();
  I2c_Soft_SendByte(SlaveAddress << 1); 
  if(I2c_Soft_WaitAsk())
  {
    I2c_Soft_Stop();
    return 1;
  }
  if(ADDRESS_MODE == ADDRESS_16)
  {
    I2c_Soft_SendByte(REG_Address >> 8);       
    I2c_Soft_WaitAsk();
  }
  I2c_Soft_SendByte(REG_Address & 0xFF);       
  I2c_Soft_WaitAsk();
  
  I2c_Soft_Start();
  I2c_Soft_SendByte(SlaveAddress << 1 | 0x01);
  I2c_Soft_WaitAsk();
  *REG_data = I2c_Soft_ReadByte(0);
  I2c_Soft_Stop();
  return 0;
}  

uint8_t IIC_Write_nByte(uint8_t SlaveAddress, uint16_t REG_Address, uint8_t *buf, uint8_t len)
{  
  I2c_Soft_Start();
  I2c_Soft_SendByte(SlaveAddress << 1); 
  if(I2c_Soft_WaitAsk())
  {
    I2c_Soft_Stop();
    return 1;
  }
  if(ADDRESS_MODE == ADDRESS_16)
  {
    I2c_Soft_SendByte(REG_Address >> 8);       
    I2c_Soft_WaitAsk();
  }
  I2c_Soft_SendByte(REG_Address & 0xFF);       
  I2c_Soft_WaitAsk();
  while(len--) 
  {
    I2c_Soft_SendByte(*buf++); 
    I2c_Soft_WaitAsk();
  }
  I2c_Soft_Stop();
  return 0;
}

uint8_t IIC_Read_nByte(uint8_t SlaveAddress, uint16_t REG_Address, uint8_t *buf, uint8_t len)
{  
  I2c_Soft_Start();
  I2c_Soft_SendByte(SlaveAddress << 1); 
  if(I2c_Soft_WaitAsk())
  {
    I2c_Soft_Stop();
    return 1;
  }
  if(ADDRESS_MODE == ADDRESS_16)
  {
    I2c_Soft_SendByte(REG_Address >> 8);       
    I2c_Soft_WaitAsk();
  }
  I2c_Soft_SendByte(REG_Address & 0xFF);       
  I2c_Soft_WaitAsk();
  
  I2c_Soft_Start();
  I2c_Soft_SendByte(SlaveAddress << 1 | 0x01); 
  I2c_Soft_WaitAsk();
  while(len) 
  {
    if(len == 1)
      *buf = I2c_Soft_ReadByte(0);
    else
      *buf = I2c_Soft_ReadByte(1);
    buf++;
    len--;
  }
  I2c_Soft_Stop();
  return 0;
}

