#include "protocol/iic_soft.h"
#include "stdbool.h"

/*The following program is modified by the user according to the hardware device, otherwise the driver cannot run.*/

/**
  * @step 1:  Modify the corresponding function according to the modified area and the corresponding function name.
  * @step 2:  Modify the read-write function of IIC corresponding pin in the header file.
  * @step 3:  Modify the read-write address length in the header file.
  */

#include "timer/k-delay.h"

#include "core/idebug.h"

//static uint8_t IIC_Soft_FastMode = 1;

//#define IIC_Soft_Pin_SCL     GPIO_PIN_6
//#define IIC_Soft_Pin_SDA     GPIO_PIN_3
//#define IIC_Soft_Port_SCL    GPIOH
//#define IIC_Soft_Port_SDA    GPIOI
#define IIC_Soft_Pin_SCL     GPIO_PIN_13
#define IIC_Soft_Pin_SDA     GPIO_PIN_14
#define IIC_Soft_Port_SCL    GPIOB
#define IIC_Soft_Port_SDA    GPIOB

//#define SCL_CLOCK_ENABLE     __HAL_RCC_GPIOH_CLK_ENABLE()
//#define SDA_CLOCK_ENABLE     __HAL_RCC_GPIOI_CLK_ENABLE()
#define SCL_CLOCK_ENABLE     __HAL_RCC_GPIOB_CLK_ENABLE()
#define SDA_CLOCK_ENABLE     __HAL_RCC_GPIOB_CLK_ENABLE()

#define SCL_H           HAL_GPIO_WritePin(IIC_Soft_Port_SCL, IIC_Soft_Pin_SCL, GPIO_PIN_SET)//IIC_Soft_Port_SCL->BSRR = (uint32_t)IIC_Soft_Pin_SCL
#define SCL_L           HAL_GPIO_WritePin(IIC_Soft_Port_SCL, IIC_Soft_Pin_SCL, GPIO_PIN_RESET)//IIC_Soft_Port_SCL->BRR  = (uint32_t)IIC_Soft_Pin_SCL
#define SDA_H           HAL_GPIO_WritePin(IIC_Soft_Port_SDA, IIC_Soft_Pin_SDA, GPIO_PIN_SET)//IIC_Soft_Port_SDA->BSRR = (uint32_t)IIC_Soft_Pin_SDA
#define SDA_L           HAL_GPIO_WritePin(IIC_Soft_Port_SDA, IIC_Soft_Pin_SDA, GPIO_PIN_RESET)//IIC_Soft_Port_SDA->BRR  = (uint32_t)IIC_Soft_Pin_SDA

#define SDA_NUM         14
//#define SDA_IN          IIC_Soft_Port_SDA->CRL &= ~((uint32_t)0xF << (SDA_NUM << 2));IIC_Soft_Port_SDA->CRL |= (uint32_t)0x8 << (SDA_NUM << 2)
//#define SDA_OUT         IIC_Soft_Port_SDA->CRL &= ~((uint32_t)0xF << (SDA_NUM << 2));IIC_Soft_Port_SDA->CRL |= (uint32_t)0x6 << (SDA_NUM << 2)
#define SDA_IN          IIC_Soft_Port_SDA->MODER &= ~(3 << SDA_NUM * 2);IIC_Soft_Port_SDA->MODER |= 0 << SDA_NUM * 2
#define SDA_OUT         IIC_Soft_Port_SDA->MODER &= ~(3 << SDA_NUM * 2);IIC_Soft_Port_SDA->MODER |= 1 << SDA_NUM * 2
#define SDA_READ        (IIC_Soft_Port_SDA->IDR & IIC_Soft_Pin_SDA)

#define ADDRESS_16      1
#define ADDRESS_8       0
#define ADDRESS_MODE    ADDRESS_16

uint8_t IIC_Soft_WriteSingleByteWithAddr(uint8_t SlaveAddr, uint16_t RegAddr, uint8_t Regdata);
uint8_t IIC_Soft_ReadSingleByteWithAddr(uint8_t SlaveAddr, uint16_t RegAddr, uint8_t *Regdata);
uint8_t IIC_Soft_WriteMultiByteWithAddr(uint8_t SlaveAddr, uint16_t RegAddr, uint8_t *pData, uint8_t Len);
uint8_t IIC_Soft_ReadMultiByteWithAddr(uint8_t SlaveAddr, uint16_t RegAddr, uint8_t *pData, uint8_t Len);

void IIC_Soft_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    /* GPIO Ports Clock Enable */
    SCL_CLOCK_ENABLE;
    SDA_CLOCK_ENABLE;

    /*Configure GPIO pin : PF6 */
    GPIO_InitStruct.Pin = IIC_Soft_Pin_SCL;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    HAL_GPIO_Init(IIC_Soft_Port_SCL, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = IIC_Soft_Pin_SDA;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    HAL_GPIO_Init(IIC_Soft_Port_SDA, &GPIO_InitStruct);

    /*Configure GPIO pin Output Level */
    HAL_GPIO_WritePin(IIC_Soft_Port_SCL, IIC_Soft_Pin_SCL, GPIO_PIN_SET);
    HAL_GPIO_WritePin(IIC_Soft_Port_SDA, IIC_Soft_Pin_SDA, GPIO_PIN_SET);
}

void IIC_Soft_delay(uint32_t ticks)
{
#ifdef USING_HARDWARE_TIMER
    Delay_us(ticks);
#else
    uint8_t i = 10;

    while(i--) {}

#endif
}

void IIC_PortTransmmit(uint8_t IIC, uint8_t SlaveAddr, uint16_t RegAddr, uint8_t Regdata)
{
    if(IIC == IIC_1)
        IIC_Soft_WriteSingleByteWithAddr(SlaveAddr, RegAddr, Regdata);
    else if(IIC == IIC_2)
    {
//        HAL_I2C_Mem_Write(&hi2c2, (uint16_t)(SlaveAddr << 1), RegAddr, I2C_MEMADD_SIZE_8BIT,
//            &Regdata, 1, 10000);
    }
}

void IIC_PortReceive(uint8_t IIC, uint8_t SlaveAddr, uint16_t RegAddr, uint8_t *Regdata)
{
    if(IIC == IIC_1)
        IIC_Soft_ReadSingleByteWithAddr(SlaveAddr, RegAddr, Regdata);
    else if(IIC == IIC_2)
    {
//        HAL_I2C_Mem_Read(&hi2c2, (uint16_t)(AT24CXX_ADDR << 1), RegAddr, I2C_MEMADD_SIZE_8BIT,
//            Regdata, 1, 10000);
    }
}

void IIC_PortMultiTransmmit(uint8_t IIC, uint8_t SlaveAddr, uint16_t RegAddr, uint8_t *pData, uint8_t Len)
{
    if(IIC == IIC_1)
        IIC_Soft_ReadMultiByteWithAddr(SlaveAddr, RegAddr, pData, Len);
    else if(IIC == IIC_2)
    {
//        HAL_I2C_Mem_Write(&hi2c2, (uint16_t)(SlaveAddr << 1), RegAddr, I2C_MEMADD_SIZE_8BIT,
//            pData, Len, 10000);
    }
}

void IIC_PortMultiReceive(uint8_t IIC, uint8_t SlaveAddr, uint16_t RegAddr, uint8_t *pData, uint8_t Len)
{
    if(IIC == IIC_1)
        IIC_Soft_ReadMultiByteWithAddr(SlaveAddr, RegAddr, pData, Len);
    else if(IIC == IIC_2)
    {
//        HAL_I2C_Mem_Read(&hi2c2, (uint16_t)(AT24CXX_ADDR << 1), RegAddr, I2C_MEMADD_SIZE_8BIT,
//            pData, Len, 10000);
    }
}

/*The above procedure is modified by the user according to the hardware device, otherwise the driver cannot run.*/

int IIC_Soft_ReadSDA(void)
{
    int retVal = 0;

//  SDA_IN;
    retVal = SDA_READ;
//  SDA_OUT;

    return retVal;
}

int IIC_Soft_Start(void)
{
    SDA_OUT;
    SDA_H;
    SCL_H;
    IIC_Soft_delay(5);

    if(!IIC_Soft_ReadSDA())
        return false;

    SDA_L;
    IIC_Soft_delay(4);

    if(IIC_Soft_ReadSDA())
        return false;

    SCL_L;
    IIC_Soft_delay(4);

    return true;
}

void IIC_Soft_Stop(void)
{
    SDA_OUT;
    SCL_L;
    IIC_Soft_delay(4);
    SDA_L;
    IIC_Soft_delay(4);
    SCL_H;
    IIC_Soft_delay(4);
    SDA_H;
    IIC_Soft_delay(4);
}

void IIC_Soft_Ack(void)
{
    SCL_L;
    IIC_Soft_delay(2);
    SDA_OUT;
    SDA_L;
    IIC_Soft_delay(2);
    SCL_H;
    IIC_Soft_delay(2);
    SCL_L;
    IIC_Soft_delay(2);
}

void IIC_Soft_NoAck(void)
{
    SCL_L;
    IIC_Soft_delay(2);
    SDA_OUT;
    SDA_H;
    IIC_Soft_delay(2);
    SCL_H;
    IIC_Soft_delay(2);
    SCL_L;
    IIC_Soft_delay(2);
}

int IIC_Soft_WaitAck(void)
{
    uint8_t ErrTime = 0;

    SCL_L;
    IIC_Soft_delay(1);
    SDA_IN;
    SDA_H;
    IIC_Soft_delay(1);
    SCL_H;
    IIC_Soft_delay(1);

    while(IIC_Soft_ReadSDA())
    {
        ErrTime++;

        if(ErrTime > 50)
        {
            IIC_Soft_Stop();
            return true;
        }
    }

    SCL_L;
    IIC_Soft_delay(1);

    return false;
}

void IIC_Soft_SendByte(uint8_t Data)
{
    uint8_t i = 8;

    SDA_OUT;

    while(i--)
    {
        SCL_L;
        IIC_Soft_delay(2);

        if(Data & 0x80)
            SDA_H;
        else
            SDA_L;

        Data <<= 1;
        IIC_Soft_delay(2);
        SCL_H;
        IIC_Soft_delay(2);
    }

    SCL_L;
}

uint8_t IIC_Soft_ReadByte(uint8_t Ack)
{
    uint8_t i = 8;
    uint8_t RcvByte = 0;

    SDA_IN;
    IIC_Soft_delay(2);
    SDA_H;

    while(i--)
    {
        RcvByte <<= 1;
        SCL_L;
        IIC_Soft_delay(2);
        SCL_H;
        IIC_Soft_delay(2);

        if(IIC_Soft_ReadSDA())
            RcvByte |= 0x01;
    }

    SCL_L;

    if(Ack)
        IIC_Soft_Ack();
    else
        IIC_Soft_NoAck();

    return RcvByte;
}

uint8_t IIC_Soft_WriteSingleByteWithAddr(uint8_t SlaveAddr, uint16_t RegAddr, uint8_t Regdata)
{
    if(IIC_Soft_Start() == false)
    {
        kinetis_debug_trace(KERN_ERR, "Arbitration failed ! Device(addr = 0x%X) cannot obtain the bus.", SlaveAddr);
//    return false;
    }

    IIC_Soft_SendByte((SlaveAddr << 1) | 0x00);

    if(IIC_Soft_WaitAck())
    {
        IIC_Soft_Stop();
        return false;
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

    return true;
}

uint8_t IIC_Soft_ReadSingleByteWithAddr(uint8_t SlaveAddr, uint16_t RegAddr, uint8_t *Regdata)
{
    if(IIC_Soft_Start() == false)
    {
        kinetis_debug_trace(KERN_ERR, "Arbitration failed ! Device(addr = 0x%X) cannot obtain the bus.", SlaveAddr);
//    return false;
    }

    IIC_Soft_SendByte((SlaveAddr << 1) | 0x00);

    if(IIC_Soft_WaitAck())
    {
        IIC_Soft_Stop();
        return false;
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

    return true;
}

uint8_t IIC_Soft_WriteMultiByteWithAddr(uint8_t SlaveAddr, uint16_t RegAddr, uint8_t *pData, uint8_t Len)
{
    if(IIC_Soft_Start() == false)
    {
        kinetis_debug_trace(KERN_ERR, "Arbitration failed ! Device(addr = 0x%X) cannot obtain the bus.", SlaveAddr);
//    return false;
    }

    IIC_Soft_SendByte((SlaveAddr << 1) | 0x00);

    if(IIC_Soft_WaitAck())
    {
        IIC_Soft_Stop();
        return false;
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

    return true;
}

uint8_t IIC_Soft_ReadMultiByteWithAddr(uint8_t SlaveAddr, uint16_t RegAddr, uint8_t *pData, uint8_t Len)
{
    if(IIC_Soft_Start() == false)
    {
        kinetis_debug_trace(KERN_ERR, "Arbitration failed ! Device(addr = 0x%X) cannot obtain the bus.", SlaveAddr);
//    return false;
    }

    IIC_Soft_SendByte((SlaveAddr << 1) | 0x00);

    if(IIC_Soft_WaitAck())
    {
        IIC_Soft_Stop();
        return false;
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
            *pData = IIC_Soft_ReadByte(0);
        else
            *pData = IIC_Soft_ReadByte(1);

        pData++;
        Len--;
    }

    IIC_Soft_Stop();

    return true;
}

#ifdef DESIGN_VERIFICATION_IIC

#endif

