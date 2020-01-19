#include "iic_soft/iic_soft.h"

/*The following program is modified by the user according to the hardware device, otherwise the driver cannot run.*/

/**
  * @step 1:  Modify the corresponding function according to the modified area and the corresponding function name.
  * @step 2:  Modify the read-write function of IIC corresponding pin in the header file.
  * @step 3:  Modify the read-write address length in the header file.
  */

static uint8_t IIC_Soft_FastMode = 1;

#define IIC_Soft_Pin_SCL     GPIO_PIN_13
#define IIC_Soft_Pin_SDA     GPIO_PIN_14
#define IIC_Soft_Port_SCL    GPIOB
#define IIC_Soft_Port_SDA    GPIOB

#define SCL_H           HAL_GPIO_WritePin(IIC_Soft_Port_SCL, IIC_Soft_Pin_SCL, GPIO_PIN_SET)//IIC_Soft_Port_SCL->BSRR = (uint32_t)IIC_Soft_Pin_SCL
#define SCL_L           HAL_GPIO_WritePin(IIC_Soft_Port_SCL, IIC_Soft_Pin_SCL, GPIO_PIN_RESET)//IIC_Soft_Port_SCL->BRR  = (uint32_t)IIC_Soft_Pin_SCL
#define SDA_H           HAL_GPIO_WritePin(IIC_Soft_Port_SDA, IIC_Soft_Pin_SDA, GPIO_PIN_SET)//IIC_Soft_Port_SDA->BSRR = (uint32_t)IIC_Soft_Pin_SDA
#define SDA_L           HAL_GPIO_WritePin(IIC_Soft_Port_SDA, IIC_Soft_Pin_SDA, GPIO_PIN_RESET)//IIC_Soft_Port_SDA->BRR  = (uint32_t)IIC_Soft_Pin_SDA

#define SDA_NUM         14
#define SDA_IN          IIC_Soft_Port_SDA->MODER &= ~(3 << SDA_NUM * 2);IIC_Soft_Port_SDA->MODER |= 0 << SDA_NUM * 2
#define SDA_OUT         IIC_Soft_Port_SDA->MODER &= ~(3 << SDA_NUM * 2);IIC_Soft_Port_SDA->MODER |= 1 << SDA_NUM * 2
#define SDA_READ        ((IIC_Soft_Port_SDA->IDR  & IIC_Soft_Pin_SDA) != 0x00u)

#define ADDRESS_16      1
#define ADDRESS_8       0
#define ADDRESS_MODE    ADDRESS_8

void IIC_Soft_Init(void)
{

}

/* The current delay time is 5 us on an 80M clock */
void IIC_Soft_Subdelay(void)
{
  int i = 0;
  
  for(i = 0;i < 100;i++)
  {
    asm("nop");
  }
}

void IIC_Soft_delay(void)
{ 
  switch(IIC_Soft_FastMode)
  {
    case 0:
      IIC_Soft_Subdelay();
      IIC_Soft_Subdelay();
      IIC_Soft_Subdelay();
      IIC_Soft_Subdelay();
      break;
    case 1:
      IIC_Soft_Subdelay();
      break;
    default:
      IIC_Soft_Subdelay();
      break;
  }
}

/*The above procedure is modified by the user according to the hardware device, otherwise the driver cannot run.*/


int IIC_Soft_Start(void)
{
  SDA_OUT;
  SDA_H;
  SCL_H;
  IIC_Soft_delay();
  if(!SDA_READ)
  {
    return 0;
  }
  SDA_L;
  IIC_Soft_delay();
  if(SDA_READ)
  {
    return 0;
  }
  SDA_L;
  IIC_Soft_delay();
  
  return 1;
}

void IIC_Soft_Stop(void)
{
  SDA_OUT;
  SCL_L;
  IIC_Soft_delay();
  SDA_L;
  IIC_Soft_delay();
  SCL_H;
  IIC_Soft_delay();
  SDA_H;
  IIC_Soft_delay();
}

void IIC_Soft_Ack(void)
{
  SCL_L;
  IIC_Soft_delay();
  SDA_OUT;
  SDA_L;
  IIC_Soft_delay();
  SCL_H;
  IIC_Soft_delay();
  SCL_L;
  IIC_Soft_delay();
}

void IIC_Soft_NoAck(void)
{
  SCL_L;
  IIC_Soft_delay();
  SDA_OUT;
  SDA_H;
  IIC_Soft_delay();
  SCL_H;
  IIC_Soft_delay();
  SCL_L;
  IIC_Soft_delay();
}

int IIC_Soft_WaitAck(void)
{
  uint8_t ErrTime = 0;
  
  SCL_L;
  IIC_Soft_delay();
  SDA_IN;
  SDA_H;      
  IIC_Soft_delay();
  SCL_H;
  IIC_Soft_delay();
  while(SDA_READ)
  {
    ErrTime++;
    if(ErrTime>50)
    {
      IIC_Soft_Stop();
      return 1;
    }
  }
  SCL_L;
  IIC_Soft_delay();
  
  return 0;
}

void IIC_Soft_SendByte(uint8_t Data)
{
  uint8_t i = 8;
  
  SDA_OUT;
  while(i--)
  {
    SCL_L;
    IIC_Soft_delay();
    if(Data & 0x80)
    {
      SDA_H;
    }
    else
    {
      SDA_L;
    }
    Data <<= 1;
    IIC_Soft_delay();
    SCL_H;
    IIC_Soft_delay();
  }
  SCL_L;
}  

uint8_t IIC_Soft_ReadByte(uint8_t Ack)
{ 
  uint8_t i = 8;
  uint8_t ReceiveByte=0;

  SDA_IN;
  IIC_Soft_delay();
  SDA_H;        
  while(i--)
  {
    ReceiveByte<<=1;      
    SCL_L;
    IIC_Soft_delay();
    SCL_H;
    IIC_Soft_delay();  
    if(SDA_READ)
    {
      ReceiveByte|=0x01;
    }
  }
  SCL_L;

  if (Ack)
    IIC_Soft_Ack();
  else
    IIC_Soft_NoAck();  
  
  return ReceiveByte;
} 

uint8_t IIC_Soft_WriteSingleByteWithAddr(uint8_t SlaveAddr,uint16_t RegAddr,uint8_t Regdata)
{
  IIC_Soft_Start();
  IIC_Soft_SendByte((SlaveAddr << 1) | 0x00);   
  if(IIC_Soft_WaitAck())
  {
    IIC_Soft_Stop();
    return 1;
  }
  if(ADDRESS_MODE == ADDRESS_16)
  {
    IIC_Soft_SendByte(RegAddr >> 8);       
    IIC_Soft_WaitAck();
  }
  IIC_Soft_SendByte(RegAddr & 0xFF);       
  IIC_Soft_WaitAck();  
  IIC_Soft_SendByte(Regdata);
  IIC_Soft_WaitAck();
  IIC_Soft_Stop();
  
  return 0;
}

uint8_t IIC_Soft_ReadSingleByteWithAddr(uint8_t SlaveAddr,uint16_t RegAddr,uint8_t *Regdata)
{
  IIC_Soft_Start();
  IIC_Soft_SendByte((SlaveAddr << 1) | 0x00); 
  if(IIC_Soft_WaitAck())
  {
    IIC_Soft_Stop();
    return 1;
  }
  if(ADDRESS_MODE == ADDRESS_16)
  {
    IIC_Soft_SendByte(RegAddr >> 8);       
    IIC_Soft_WaitAck();
  }
  IIC_Soft_SendByte(RegAddr & 0xFF);       
  IIC_Soft_WaitAck();
  
  IIC_Soft_Start();
  IIC_Soft_SendByte((SlaveAddr << 1) | 0x01);
  IIC_Soft_WaitAck();
  *Regdata = IIC_Soft_ReadByte(0);
  IIC_Soft_Stop();
  
  return 0;
}  

uint8_t IIC_Soft_WriteMultiByteWithAddr(uint8_t SlaveAddr, uint16_t RegAddr, uint8_t *pData, uint8_t Len)
{
  IIC_Soft_Start();
  IIC_Soft_SendByte((SlaveAddr << 1) | 0x00); 
  if(IIC_Soft_WaitAck())
  {
    IIC_Soft_Stop();
    return 1;
  }
  if(ADDRESS_MODE == ADDRESS_16)
  {
    IIC_Soft_SendByte(RegAddr >> 8);       
    IIC_Soft_WaitAck();
  }
  IIC_Soft_SendByte(RegAddr & 0xFF);       
  IIC_Soft_WaitAck();
  while(Len--) 
  {
    IIC_Soft_SendByte(*pData); 
    IIC_Soft_WaitAck();
    pData++;
  }
  IIC_Soft_Stop();
  
  return 0;
}

uint8_t IIC_Soft_ReadMultiByteWithAddr(uint8_t SlaveAddr, uint16_t RegAddr, uint8_t *pData, uint8_t Len)
{  
  IIC_Soft_Start();
  IIC_Soft_SendByte((SlaveAddr << 1) | 0x00); 
  if(IIC_Soft_WaitAck())
  {
    IIC_Soft_Stop();
    return 1;
  }
  if(ADDRESS_MODE == ADDRESS_16)
  {
    IIC_Soft_SendByte(RegAddr >> 8);       
    IIC_Soft_WaitAck();
  }
  IIC_Soft_SendByte(RegAddr & 0xFF);       
  IIC_Soft_WaitAck();
  
  IIC_Soft_Start();
  IIC_Soft_SendByte((SlaveAddr << 1) | 0x01); 
  IIC_Soft_WaitAck();
  while(Len) 
  {
    if(Len == 1)
    {
      *pData = IIC_Soft_ReadByte(0);
    }
    else
    {
      *pData = IIC_Soft_ReadByte(1);
    }
    pData++;
    Len--;
  }
  IIC_Soft_Stop();
  
  return 0;
}

