#ifndef __I2C_SOFT_H
#define __I2C_SOFT_H

#ifdef __cplusplus
 extern "C" {
#endif
                                              
/* Includes ------------------------------------------------------------------*/
#include "stm32f4xx_hal.h"

#define I2C_Pin_SCL     GPIO_PIN_6
#define I2C_Pin_SDA     GPIO_PIN_3

#define SCL_H           GPIOH->BSRR = I2C_Pin_SCL
#define SCL_L           GPIOH->BSRR = (uint32_t)I2C_Pin_SCL << 16U
#define SDA_H           GPIOI->BSRR = I2C_Pin_SDA
#define SDA_L           GPIOI->BSRR = (uint32_t)I2C_Pin_SDA << 16U

#define SDA_NUM         3
#define SDA_IN          GPIOI->MODER &= ~(3 << SDA_NUM * 2);GPIOI->MODER |= 0 << SDA_NUM * 2
#define SDA_OUT         GPIOI->MODER &= ~(3 << SDA_NUM * 2);GPIOI->MODER |= 1 << SDA_NUM * 2
#define SDA_READ        GPIOI->IDR  & I2C_Pin_SDA

#define ADDRESS_16      1
#define ADDRESS_8       0
#define ADDRESS_MODE    ADDRESS_16


void I2c_Soft_Init(void);

//int I2c_Soft_Single_Write(uint8_t SlaveAddress,uint8_t REG_Address,uint8_t REG_data);
//int I2c_Soft_Single_Read(uint8_t SlaveAddress,uint8_t REG_Address);
//int I2c_Soft_Mult_Read(uint8_t SlaveAddress,uint8_t REG_Address,uint8_t * ptChar,uint8_t size);

uint8_t IIC_Write_1Byte(uint8_t SlaveAddress,uint16_t REG_Address,uint8_t REG_data);
uint8_t IIC_Read_1Byte(uint8_t SlaveAddress,uint16_t REG_Address,uint8_t *REG_data);
uint8_t IIC_Write_nByte(uint8_t SlaveAddress, uint16_t REG_Address, uint8_t *buf, uint8_t len);
uint8_t IIC_Read_nByte(uint8_t SlaveAddress, uint16_t REG_Address, uint8_t *buf, uint8_t len);

#endif /* __I2C_SOFT_H */
