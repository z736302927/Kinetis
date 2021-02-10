#include "kinetis/iic_soft.h"
#include "kinetis/idebug.h"
#include "kinetis/delay.h"

#include "stm32f4xx_hal.h"

/* The following program is modified by the user according to the hardware device, otherwise the driver cannot run. */

/**
  * @step 1:  Modify the corresponding function according to the modified area and the corresponding function name.
  * @step 2:  Modify the read-write function of IIC corresponding pin in the header file.
  * @step 3:  Modify the read-write address length in the header file.
  */


//static u8 IIC_Soft_FastMode = 1;

//#define IIC_SOFT_PIN_SCL     GPIO_PIN_6
//#define IIC_SOFT_PIN_SDA     GPIO_PIN_3
//#define IIC_SOFT_PORT_SCL    GPIOH
//#define IIC_SOFT_PORT_SDA    GPIOI
#define IIC_SOFT_PIN_SCL     GPIO_PIN_13
#define IIC_SOFT_PIN_SDA     GPIO_PIN_14
#define IIC_SOFT_PORT_SCL    GPIOB
#define IIC_SOFT_PORT_SDA    GPIOB

//#define SCL_CLOCK_ENABLE     __HAL_RCC_GPIOH_CLK_ENABLE()
//#define SDA_CLOCK_ENABLE     __HAL_RCC_GPIOI_CLK_ENABLE()
#define SCL_CLOCK_ENABLE     __HAL_RCC_GPIOB_CLK_ENABLE()
#define SDA_CLOCK_ENABLE     __HAL_RCC_GPIOB_CLK_ENABLE()

#define SCL_H           HAL_GPIO_WritePin(IIC_SOFT_PORT_SCL, IIC_SOFT_PIN_SCL, GPIO_PIN_SET)//IIC_SOFT_PORT_SCL->BSRR = (u32)IIC_SOFT_PIN_SCL
#define SCL_L           HAL_GPIO_WritePin(IIC_SOFT_PORT_SCL, IIC_SOFT_PIN_SCL, GPIO_PIN_RESET)//IIC_SOFT_PORT_SCL->BRR  = (u32)IIC_SOFT_PIN_SCL
#define SDA_H           HAL_GPIO_WritePin(IIC_SOFT_PORT_SDA, IIC_SOFT_PIN_SDA, GPIO_PIN_SET)//IIC_SOFT_PORT_SDA->BSRR = (u32)IIC_SOFT_PIN_SDA
#define SDA_L           HAL_GPIO_WritePin(IIC_SOFT_PORT_SDA, IIC_SOFT_PIN_SDA, GPIO_PIN_RESET)//IIC_SOFT_PORT_SDA->BRR  = (u32)IIC_SOFT_PIN_SDA

#define SDA_NUM         14
//#define SDA_IN          IIC_SOFT_PORT_SDA->CRL &= ~((u32)0xF << (SDA_NUM << 2));IIC_SOFT_PORT_SDA->CRL |= (u32)0x8 << (SDA_NUM << 2)
//#define SDA_OUT         IIC_SOFT_PORT_SDA->CRL &= ~((u32)0xF << (SDA_NUM << 2));IIC_SOFT_PORT_SDA->CRL |= (u32)0x6 << (SDA_NUM << 2)
#define SDA_IN          IIC_SOFT_PORT_SDA->MODER &= ~(3 << SDA_NUM * 2);IIC_SOFT_PORT_SDA->MODER |= 0 << SDA_NUM * 2
#define SDA_OUT         IIC_SOFT_PORT_SDA->MODER &= ~(3 << SDA_NUM * 2);IIC_SOFT_PORT_SDA->MODER |= 1 << SDA_NUM * 2
#define SDA_READ        (IIC_SOFT_PORT_SDA->IDR & IIC_SOFT_PIN_SDA)

#define ADDRESS_16      1
#define ADDRESS_8       0
#define ADDRESS_MODE    ADDRESS_16

static bool iic_soft_write_byte_with_addr(u8 slave_addr, u16 reg, u8 tmp);
static bool iic_soft_read_byte_with_addr(u8 slave_addr, u16 reg, u8 *tmp);
static bool iic_soft_write_bytes_with_addr(u8 slave_addr, u16 reg,
    u8 *pdata, u8 length);
static bool iic_soft_read_bytes_with_addr(u8 slave_addr, u16 reg,
    u8 *pdata, u8 length);

void iic_soft_init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    /* GPIO Ports Clock Enable */
    SCL_CLOCK_ENABLE;
    SDA_CLOCK_ENABLE;

    /*Configure GPIO pin : PF6 */
    GPIO_InitStruct.Pin = IIC_SOFT_PIN_SCL;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    HAL_GPIO_Init(IIC_SOFT_PORT_SCL, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = IIC_SOFT_PIN_SDA;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    HAL_GPIO_Init(IIC_SOFT_PORT_SDA, &GPIO_InitStruct);

    /*Configure GPIO pin Output Level */
    HAL_GPIO_WritePin(IIC_SOFT_PORT_SCL, IIC_SOFT_PIN_SCL, GPIO_PIN_SET);
    HAL_GPIO_WritePin(IIC_SOFT_PORT_SDA, IIC_SOFT_PIN_SDA, GPIO_PIN_SET);
}

void iic_soft_delay(u32 ticks)
{
#ifdef USING_HARDWARE_TIMER
    udelay(ticks);
#else
    u8 i = 10;

    while (i--) {}

#endif
}

void iic_port_transmmit(u8 iic, u8 slave_addr, u16 reg, u8 tmp)
{
    if (iic == IIC_1)
        iic_soft_write_byte_with_addr(slave_addr, reg, tmp);
    else if (iic == IIC_2) {
//        HAL_I2C_Mem_Write(&hi2c2, (u16)(slave_addr << 1), reg, I2C_MEMADD_SIZE_8BIT,
//            &tmp, 1, 10000);
    }
}

void iic_port_receive(u8 iic, u8 slave_addr, u16 reg, u8 *tmp)
{
    if (iic == IIC_1)
        iic_soft_read_byte_with_addr(slave_addr, reg, tmp);
    else if (iic == IIC_2) {
//        HAL_I2C_Mem_Read(&hi2c2, (u16)(AT24CXX_ADDR << 1), reg, I2C_MEMADD_SIZE_8BIT,
//            tmp, 1, 10000);
    }
}

void iic_port_multi_transmmit(u8 iic, u8 slave_addr, u16 reg,
    u8 *pdata, u8 length)
{
    if (iic == IIC_1)
        iic_soft_write_bytes_with_addr(slave_addr, reg, pdata, length);
    else if (iic == IIC_2) {
//        HAL_I2C_Mem_Write(&hi2c2, (u16)(slave_addr << 1), reg, I2C_MEMADD_SIZE_8BIT,
//            pdata, length, 10000);
    }
}

void iic_port_multi_receive(u8 iic, u8 slave_addr, u16 reg,
    u8 *pdata, u8 length)
{
    if (iic == IIC_1)
        iic_soft_read_bytes_with_addr(slave_addr, reg, pdata, length);
    else if (iic == IIC_2) {
//        HAL_I2C_Mem_Read(&hi2c2, (u16)(AT24CXX_ADDR << 1), reg, I2C_MEMADD_SIZE_8BIT,
//            pdata, length, 10000);
    }
}

/* The above procedure is modified by the user according to the hardware device, otherwise the driver cannot run. */

static int iic_soft_read_sda(void)
{
    int retVal = 0;

//  SDA_IN;
    retVal = SDA_READ;
//  SDA_OUT;

    return retVal;
}

int iic_soft_start(void)
{
    SDA_OUT;
    SDA_H;
    SCL_H;
    iic_soft_delay(5);

    if (!iic_soft_read_sda())
        return false;

    SDA_L;
    iic_soft_delay(4);

    if (iic_soft_read_sda())
        return false;

    SCL_L;
    iic_soft_delay(4);

    return true;
}

void iic_soft_stop(void)
{
    SDA_OUT;
    SCL_L;
    iic_soft_delay(4);
    SDA_L;
    iic_soft_delay(4);
    SCL_H;
    iic_soft_delay(4);
    SDA_H;
    iic_soft_delay(4);
}

void iic_soft_ack(void)
{
    SCL_L;
    iic_soft_delay(2);
    SDA_OUT;
    SDA_L;
    iic_soft_delay(2);
    SCL_H;
    iic_soft_delay(2);
    SCL_L;
    iic_soft_delay(2);
}

void iic_soft_no_ack(void)
{
    SCL_L;
    iic_soft_delay(2);
    SDA_OUT;
    SDA_H;
    iic_soft_delay(2);
    SCL_H;
    iic_soft_delay(2);
    SCL_L;
    iic_soft_delay(2);
}

int iic_soft_wait_ack(void)
{
    u8 err_time = 0;

    SCL_L;
    iic_soft_delay(1);
    SDA_IN;
    SDA_H;
    iic_soft_delay(1);
    SCL_H;
    iic_soft_delay(1);

    while (iic_soft_read_sda()) {
        err_time++;

        if (err_time > 50) {
            iic_soft_stop();
            return true;
        }
    }

    SCL_L;
    iic_soft_delay(1);

    return false;
}

void iic_soft_send_byte(u8 tmp)
{
    u8 i = 8;

    SDA_OUT;

    while (i--) {
        SCL_L;
        iic_soft_delay(2);

        if (tmp & 0x80)
            SDA_H;
        else
            SDA_L;

        tmp <<= 1;
        iic_soft_delay(2);
        SCL_H;
        iic_soft_delay(2);
    }

    SCL_L;
}

u8 iic_soft_read_byte(u8 ack)
{
    u8 i = 8, tmp = 0;

    SDA_IN;
    iic_soft_delay(2);
    SDA_H;

    while (i--) {
        tmp <<= 1;
        SCL_L;
        iic_soft_delay(2);
        SCL_H;
        iic_soft_delay(2);

        if (iic_soft_read_sda())
            tmp |= 0x01;
    }

    SCL_L;

    if (ack)
        iic_soft_ack();
    else
        iic_soft_no_ack();

    return tmp;
}

static bool iic_soft_write_byte_with_addr(u8 slave_addr, u16 reg, u8 tmp)
{
    if (iic_soft_start() == false) {
        kinetis_print_trace(KERN_ERR,
            "Arbitration failed ! Device(addr = 0x%X) cannot obtain the bus.",
            slave_addr);
//    return false;
    }

    iic_soft_send_byte((slave_addr << 1) | 0x00);

    if (iic_soft_wait_ack()) {
        iic_soft_stop();
        return false;
    }

    if (ADDRESS_MODE == ADDRESS_16) {
        iic_soft_send_byte(reg >> 8);
        iic_soft_wait_ack();
    }

    iic_soft_send_byte(reg & 0xFF);
    iic_soft_wait_ack();
    iic_soft_send_byte(tmp);
    iic_soft_wait_ack();
    iic_soft_stop();

    return true;
}

static bool iic_soft_read_byte_with_addr(u8 slave_addr, u16 reg, u8 *tmp)
{
    if (iic_soft_start() == false) {
        kinetis_print_trace(KERN_ERR,
            "Arbitration failed ! Device(addr = 0x%X) cannot obtain the bus.",
            slave_addr);
//    return false;
    }

    iic_soft_send_byte((slave_addr << 1) | 0x00);

    if (iic_soft_wait_ack()) {
        iic_soft_stop();
        return false;
    }

    if (ADDRESS_MODE == ADDRESS_16) {
        iic_soft_send_byte(reg >> 8);
        iic_soft_wait_ack();
    }

    iic_soft_send_byte(reg & 0xFF);
    iic_soft_wait_ack();

    iic_soft_start();
    iic_soft_send_byte((slave_addr << 1) | 0x01);
    iic_soft_wait_ack();
    *tmp = iic_soft_read_byte(0);
    iic_soft_stop();

    return true;
}

static bool iic_soft_write_bytes_with_addr(u8 slave_addr, u16 reg,
    u8 *pdata, u8 length)
{
    if (iic_soft_start() == false) {
        kinetis_print_trace(KERN_ERR,
            "Arbitration failed ! Device(addr = 0x%X) cannot obtain the bus.",
            slave_addr);
//    return false;
    }

    iic_soft_send_byte((slave_addr << 1) | 0x00);

    if (iic_soft_wait_ack()) {
        iic_soft_stop();
        return false;
    }

    if (ADDRESS_MODE == ADDRESS_16) {
        iic_soft_send_byte(reg >> 8);
        iic_soft_wait_ack();
    }

    iic_soft_send_byte(reg & 0xFF);
    iic_soft_wait_ack();

    while (length--) {
        iic_soft_send_byte(*pdata);
        iic_soft_wait_ack();
        pdata++;
    }

    iic_soft_stop();

    return true;
}

static bool iic_soft_read_bytes_with_addr(u8 slave_addr, u16 reg,
    u8 *pdata, u8 length)
{
    if (iic_soft_start() == false) {
        kinetis_print_trace(KERN_ERR,
            "Arbitration failed ! Device(addr = 0x%X) cannot obtain the bus.",
            slave_addr);
//    return false;
    }

    iic_soft_send_byte((slave_addr << 1) | 0x00);

    if (iic_soft_wait_ack()) {
        iic_soft_stop();
        return false;
    }

    if (ADDRESS_MODE == ADDRESS_16) {
        iic_soft_send_byte(reg >> 8);
        iic_soft_wait_ack();
    }

    iic_soft_send_byte(reg & 0xFF);
    iic_soft_wait_ack();

    iic_soft_start();
    iic_soft_send_byte((slave_addr << 1) | 0x01);
    iic_soft_wait_ack();

    while (length) {
        if (length == 1)
            *pdata = iic_soft_read_byte(0);
        else
            *pdata = iic_soft_read_byte(1);

        pdata++;
        length--;
    }

    iic_soft_stop();

    return true;
}

#ifdef DESIGN_VERIFICATION_IIC

#endif

