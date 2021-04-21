#include "kinetis/iic_soft.h"
#include "kinetis/idebug.h"
#include "kinetis/delay.h"

#include "stm32f4xx_hal.h"

#include <linux/printk.h>

/* The following program is modified by the user according to the hardware device, otherwise the driver cannot run. */

/**
  * @step 1:  Modify the corresponding function according to the modified area and the corresponding function name.
  * @step 2:  Modify the read-write function of IIC corresponding pin in the header file.
  * @step 3:  Modify the read-write address length in the header file.
  */

//#include "i2c.h"

//static u8 IIC_Soft_FastMode = 1;

#define ADDRESS_16      1
#define ADDRESS_8       0
#define ADDRESS_MODE    ADDRESS_8

static bool iic_soft_write_byte_with_addr(u8 iic, u8 slave_addr, u16 reg, u8 tmp);
static bool iic_soft_read_byte_with_addr(u8 iic, u8 slave_addr, u16 reg, u8 *tmp);
static bool iic_soft_write_bytes_with_addr(u8 iic, u8 slave_addr, u16 reg,
    u8 *pdata, u8 length);
static bool iic_soft_read_bytes_with_addr(u8 iic, u8 slave_addr, u16 reg,
    u8 *pdata, u8 length);

void iic_soft_init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    /* GPIO Ports Clock Enable */
    __HAL_RCC_GPIOB_CLK_ENABLE();

    /*Configure GPIO pin : PF6 */
    GPIO_InitStruct.Pin = GPIO_PIN_8;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
    
    GPIO_InitStruct.Pin = GPIO_PIN_9;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    /*Configure GPIO pin Output Level */
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_8, GPIO_PIN_SET);
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_9, GPIO_PIN_SET);
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
    if (iic == IIC_SW_1)
        iic_soft_write_byte_with_addr(iic, slave_addr, reg, tmp);
//    else if (iic == IIC_HW_1)
//        HAL_I2C_Mem_Write_DMA(&hi2c1, (u16)slave_addr, reg, I2C_MEMADD_SIZE_8BIT,
//            &tmp, 1);
}

void iic_port_receive(u8 iic, u8 slave_addr, u16 reg, u8 *tmp)
{
    if (iic == IIC_SW_1)
        iic_soft_read_byte_with_addr(iic, slave_addr, reg, tmp);
//    else if (iic == IIC_HW_1)
//        HAL_I2C_Mem_Read_DMA(&hi2c1, (u16)(slave_addr), reg, I2C_MEMADD_SIZE_8BIT,
//            tmp, 1);
}

void iic_port_multi_transmmit(u8 iic, u8 slave_addr, u16 reg,
    u8 *pdata, u8 length)
{
    if (iic == IIC_SW_1)
        iic_soft_write_bytes_with_addr(iic, slave_addr, reg, pdata, length);
//    else if (iic == IIC_HW_1)
//        HAL_I2C_Mem_Write_DMA(&hi2c1, (u16)slave_addr, reg, I2C_MEMADD_SIZE_8BIT,
//            pdata, length);
}

void iic_port_multi_receive(u8 iic, u8 slave_addr, u16 reg,
    u8 *pdata, u8 length)
{
    if (iic == IIC_SW_1)
        iic_soft_read_bytes_with_addr(iic, slave_addr, reg, pdata, length);
//    else if (iic == IIC_HW_1)
//        HAL_I2C_Mem_Read_DMA(&hi2c1, (u16)(slave_addr), reg, I2C_MEMADD_SIZE_8BIT,
//            pdata, length);
}

/* The above procedure is modified by the user according to the hardware device, otherwise the driver cannot run. */

static inline void scl_low(u8 iic)
{
    if (iic == IIC_SW_1)
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_8, GPIO_PIN_RESET);
    else if (iic == IIC_SW_2)
        HAL_GPIO_WritePin(GPIOH, GPIO_PIN_6, GPIO_PIN_RESET);
}

static inline void scl_high(u8 iic)
{
    if (iic == IIC_SW_1)
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_8, GPIO_PIN_SET);
    else if (iic == IIC_SW_2)
        HAL_GPIO_WritePin(GPIOH, GPIO_PIN_6, GPIO_PIN_SET);
}

static inline void sda_low(u8 iic)
{
    if (iic == IIC_SW_1)
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_9, GPIO_PIN_RESET);
    else if (iic == IIC_SW_2)
        HAL_GPIO_WritePin(GPIOI, GPIO_PIN_3, GPIO_PIN_RESET);
}

static inline void sda_high(u8 iic)
{
    if (iic == IIC_SW_1)
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_9, GPIO_PIN_SET);
    else if (iic == IIC_SW_2)
        HAL_GPIO_WritePin(GPIOI, GPIO_PIN_3, GPIO_PIN_SET);
}

static GPIO_InitTypeDef gpio = {
    .Pull = GPIO_PULLUP,
    .Speed = GPIO_SPEED_FREQ_VERY_HIGH
};

static inline void sda_in(u8 iic)
{
    /* 
     * It doesn't have to switch direction in 
     * open drain  mode. 
     */
//    if (iic == IIC_SW_1) {
//        gpio.Pin = GPIO_PIN_9;
//        gpio.Mode = GPIO_MODE_INPUT;
//        HAL_GPIO_Init(GPIOB, &gpio);
//    } else if (iic == IIC_SW_2) {
//        gpio.Pin = GPIO_PIN_3;
//        gpio.Mode = GPIO_MODE_INPUT;
//        HAL_GPIO_Init(GPIOI, &gpio);
//    }
}

static inline void sda_out(u8 iic)
{
    /* 
     * It doesn't have to switch direction in 
     * open drain  mode. 
     */
//    if (iic == IIC_SW_1) {
//        gpio.Pin = GPIO_PIN_9;
//        gpio.Mode = GPIO_MODE_OUTPUT_OD;
//        HAL_GPIO_Init(GPIOB, &gpio);
//    } else if (iic == IIC_SW_2) {
//        gpio.Pin = GPIO_PIN_3;
//        gpio.Mode = GPIO_MODE_OUTPUT_OD;
//        HAL_GPIO_Init(GPIOI, &gpio);
//    }
}

static inline int sda_read(u8 iic)
{
    if (iic == IIC_SW_1) {
        return HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_9);
    } else if (iic == IIC_SW_2) {
        return HAL_GPIO_ReadPin(GPIOI, GPIO_PIN_3);
    }
    
    return 0;
}

int iic_soft_start(u8 iic)
{
    sda_out(iic);
    sda_high(iic);
    scl_high(iic);
    iic_soft_delay(5);

    if (!sda_read(iic))
        return false;

    sda_low(iic);
    iic_soft_delay(4);

    if (sda_read(iic))
        return false;

    scl_low(iic);
    iic_soft_delay(4);

    return true;
}

void iic_soft_stop(u8 iic)
{
    sda_out(iic);
    scl_low(iic);
    iic_soft_delay(4);
    sda_low(iic);
    iic_soft_delay(4);
    scl_high(iic);
    iic_soft_delay(4);
    sda_high(iic);
    iic_soft_delay(4);
}

void iic_soft_ack(u8 iic)
{
    scl_low(iic);
    iic_soft_delay(2);
    sda_out(iic);
    sda_low(iic);
    iic_soft_delay(2);
    scl_high(iic);
    iic_soft_delay(2);
    scl_low(iic);
    iic_soft_delay(2);
}

void iic_soft_no_ack(u8 iic)
{
    scl_low(iic);
    iic_soft_delay(2);
    sda_out(iic);
    sda_high(iic);
    iic_soft_delay(2);
    scl_high(iic);
    iic_soft_delay(2);
    scl_low(iic);
    iic_soft_delay(2);
}

int iic_soft_wait_ack(u8 iic)
{
    u8 err_time = 0;

    scl_low(iic);
    iic_soft_delay(1);
    sda_in(iic);
    sda_high(iic);
    iic_soft_delay(1);
    scl_high(iic);
    iic_soft_delay(1);

    while (sda_read(iic)) {
        err_time++;

        if (err_time > 50) {
            iic_soft_stop(iic);
            return true;
        }
    }

    scl_low(iic);
    iic_soft_delay(1);

    return false;
}

void iic_soft_send_byte(u8 iic, u8 tmp)
{
    u8 i = 8;

    sda_out(iic);

    while (i--) {
        scl_low(iic);
        iic_soft_delay(2);

        if (tmp & 0x80)
            sda_high(iic);
        else
            sda_low(iic);

        tmp <<= 1;
        iic_soft_delay(2);
        scl_high(iic);
        iic_soft_delay(2);
    }

    scl_low(iic);
}

u8 iic_soft_read_byte(u8 iic, u8 ack)
{
    u8 i = 8, tmp = 0;

    sda_in(iic);
    iic_soft_delay(2);
    sda_high(iic);

    while (i--) {
        tmp <<= 1;
        scl_low(iic);
        iic_soft_delay(2);
        scl_high(iic);
        iic_soft_delay(2);

        if (sda_read(iic))
            tmp |= 0x01;
    }

    scl_low(iic);

    if (ack)
        iic_soft_ack(iic);
    else
        iic_soft_no_ack(iic);

    return tmp;
}

static bool iic_soft_write_byte_with_addr(u8 iic, u8 slave_addr, u16 reg, u8 tmp)
{
    if (iic_soft_start(iic) == false) {
        printk(KERN_ERR
            "Arbitration failed ! Device(addr = 0x%X) cannot obtain the bus.",
            slave_addr);
//    return false;
    }

    iic_soft_send_byte(iic, (slave_addr << 1) | 0x00);

    if (iic_soft_wait_ack(iic)) {
        iic_soft_stop(iic);
        return false;
    }

    if (ADDRESS_MODE == ADDRESS_16) {
        iic_soft_send_byte(iic, reg >> 8);
        iic_soft_wait_ack(iic);
    }

    iic_soft_send_byte(iic, reg & 0xFF);
    iic_soft_wait_ack(iic);
    iic_soft_send_byte(iic, tmp);
    iic_soft_wait_ack(iic);
    iic_soft_stop(iic);

    return true;
}

static bool iic_soft_read_byte_with_addr(u8 iic, u8 slave_addr, u16 reg, u8 *tmp)
{
    if (iic_soft_start(iic) == false) {
        printk(KERN_ERR
            "Arbitration failed ! Device(addr = 0x%X) cannot obtain the bus.",
            slave_addr);
//    return false;
    }

    iic_soft_send_byte(iic, (slave_addr << 1) | 0x00);

    if (iic_soft_wait_ack(iic)) {
        iic_soft_stop(iic);
        return false;
    }

    if (ADDRESS_MODE == ADDRESS_16) {
        iic_soft_send_byte(iic, reg >> 8);
        iic_soft_wait_ack(iic);
    }

    iic_soft_send_byte(iic, reg & 0xFF);
    iic_soft_wait_ack(iic);

    iic_soft_start(iic);
    iic_soft_send_byte(iic, (slave_addr << 1) | 0x01);
    iic_soft_wait_ack(iic);
    *tmp = iic_soft_read_byte(iic, 0);
    iic_soft_stop(iic);

    return true;
}

static bool iic_soft_write_bytes_with_addr(u8 iic, u8 slave_addr, u16 reg,
    u8 *pdata, u8 length)
{
    if (iic_soft_start(iic) == false) {
        printk(KERN_ERR
            "Arbitration failed ! Device(addr = 0x%X) cannot obtain the bus.",
            slave_addr);
//    return false;
    }

    iic_soft_send_byte(iic, (slave_addr << 1) | 0x00);

    if (iic_soft_wait_ack(iic)) {
        iic_soft_stop(iic);
        return false;
    }

    if (ADDRESS_MODE == ADDRESS_16) {
        iic_soft_send_byte(iic, reg >> 8);
        iic_soft_wait_ack(iic);
    }

    iic_soft_send_byte(iic, reg & 0xFF);
    iic_soft_wait_ack(iic);

    while (length--) {
        iic_soft_send_byte(iic, *pdata);
        iic_soft_wait_ack(iic);
        pdata++;
    }

    iic_soft_stop(iic);

    return true;
}

static bool iic_soft_read_bytes_with_addr(u8 iic, u8 slave_addr, u16 reg,
    u8 *pdata, u8 length)
{
    if (iic_soft_start(iic) == false) {
        printk(KERN_ERR
            "Arbitration failed ! Device(addr = 0x%X) cannot obtain the bus.",
            slave_addr);
//    return false;
    }

    iic_soft_send_byte(iic, (slave_addr << 1) | 0x00);

    if (iic_soft_wait_ack(iic)) {
        iic_soft_stop(iic);
        return false;
    }

    if (ADDRESS_MODE == ADDRESS_16) {
        iic_soft_send_byte(iic, reg >> 8);
        iic_soft_wait_ack(iic);
    }

    iic_soft_send_byte(iic, reg & 0xFF);
    iic_soft_wait_ack(iic);

    iic_soft_start(iic);
    iic_soft_send_byte(iic, (slave_addr << 1) | 0x01);
    iic_soft_wait_ack(iic);

    while (length) {
        if (length == 1)
            *pdata = iic_soft_read_byte(iic, 0);
        else
            *pdata = iic_soft_read_byte(iic, 1);

        pdata++;
        length--;
    }

    iic_soft_stop(iic);

    return true;
}

#ifdef DESIGN_VERIFICATION_IIC

#endif

