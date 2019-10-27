#ifndef __I2C_SOFT_H
#define __I2C_SOFT_H

#ifdef __cplusplus
 extern "C" {
#endif
   
/*The following program is modified by the user according to the hardware device, otherwise the driver cannot run.*/
                                              
/* Includes ------------------------------------------------------------------*/
#include "stm32l4xx_hal.h"

#define I2C_Pin_SCL     GPIO_PIN_13
#define I2C_Pin_SDA     GPIO_PIN_14
#define I2C_Port_SCL    GPIOB
#define I2C_Port_SDA    GPIOB

#define SCL_H           HAL_GPIO_WritePin(I2C_Port_SCL, I2C_Pin_SCL, GPIO_PIN_SET)//I2C_Port_SCL->BSRR = (uint32_t)I2C_Pin_SCL
#define SCL_L           HAL_GPIO_WritePin(I2C_Port_SCL, I2C_Pin_SCL, GPIO_PIN_RESET)//I2C_Port_SCL->BRR  = (uint32_t)I2C_Pin_SCL
#define SDA_H           HAL_GPIO_WritePin(I2C_Port_SDA, I2C_Pin_SDA, GPIO_PIN_SET)//I2C_Port_SDA->BSRR = (uint32_t)I2C_Pin_SDA
#define SDA_L           HAL_GPIO_WritePin(I2C_Port_SDA, I2C_Pin_SDA, GPIO_PIN_RESET)//I2C_Port_SDA->BRR  = (uint32_t)I2C_Pin_SDA

#define SDA_NUM         14
#define SDA_IN          I2C_Port_SDA->MODER &= ~(3 << SDA_NUM * 2);I2C_Port_SDA->MODER |= 0 << SDA_NUM * 2
#define SDA_OUT         I2C_Port_SDA->MODER &= ~(3 << SDA_NUM * 2);I2C_Port_SDA->MODER |= 1 << SDA_NUM * 2
#define SDA_READ        ((I2C_Port_SDA->IDR  & I2C_Pin_SDA) != 0x00u)

#define ADDRESS_16      1
#define ADDRESS_8       0
#define ADDRESS_MODE    ADDRESS_8


void I2c_Soft_Init(void);

//int I2c_Soft_Single_Write(uint8_t SlaveAddress,uint8_t REG_Address,uint8_t REG_data);
//int I2c_Soft_Single_Read(uint8_t SlaveAddress,uint8_t REG_Address);
//int I2c_Soft_Mult_Read(uint8_t SlaveAddress,uint8_t REG_Address,uint8_t * ptChar,uint8_t size);

uint8_t IIC_Write_1Byte(uint8_t SlaveAddress,uint16_t REG_Address,uint8_t REG_data);
uint8_t IIC_Read_1Byte(uint8_t SlaveAddress,uint16_t REG_Address,uint8_t *REG_data);
uint8_t IIC_Write_nByte(uint8_t SlaveAddress, uint16_t REG_Address, uint8_t *buf, uint8_t len);
uint8_t IIC_Read_nByte(uint8_t SlaveAddress, uint16_t REG_Address, uint8_t *buf, uint8_t len);

/*The following program is modified by the user according to the hardware device, otherwise the driver cannot run.*/

#endif /* __I2C_SOFT_H */
