#include "protocol/spi_soft.h"

/*The following program is modified by the user according to the hardware device, otherwise the driver cannot run.*/

/**
  * @step 1:  Modify the corresponding function according to the modified area and the corresponding function name.
  * @step 2:  Modify four areas: GPIO_PORT/GPIO_PIN/Led_TypeDef/LEDn.
  * @step 3:  .
  * @step 4:  .
  * @step 5:
  */

#define SPI_SOFT_CR1_LSBFE                   0x01
#define SPI_SOFT_CR1_CPHA                    0x04
#define SPI_SOFT_CR1_CPOL                    0x08
#define SPI_SOFT_CR1_MSTR                    0x10
#define SPI_SOFT_BR_SPR0                     0x01
#define SPI_SOFT_BR_SPR1                     0x02
#define SPI_SOFT_BR_SPR2                     0x04
#define SPI_SOFT_BR_SPPR0                    0x10
#define SPI_SOFT_BR_SPPR1                    0x20
#define SPI_SOFT_BR_SPPR2                    0x40

#define SPI_Soft_Pin_CS                 GPIO_PIN_13
#define SPI_Soft_Pin_MOSI               GPIO_PIN_14
#define SPI_Soft_Pin_MISO               GPIO_PIN_13
#define SPI_Soft_Pin_SCK                GPIO_PIN_14
#define SPI_Soft_Port_CS                GPIOB
#define SPI_Soft_Port_MOSI              GPIOB
#define SPI_Soft_Port_MISO              GPIOB
#define SPI_Soft_Port_SCK               GPIOB

#define SCL_CLOCK_ENABLE                __HAL_RCC_GPIOB_CLK_ENABLE()
#define SDA_CLOCK_ENABLE                __HAL_RCC_GPIOB_CLK_ENABLE()

#define SPI_CS_H                        HAL_GPIO_WritePin(SPI_Soft_Port_CS, SPI_Soft_Pin_CS, GPIO_PIN_SET)
#define SPI_CS_L                        HAL_GPIO_WritePin(SPI_Soft_Port_CS, SPI_Soft_Pin_CS, GPIO_PIN_RESET)
#define SPI_MOSI_H                      HAL_GPIO_WritePin(SPI_Soft_Port_MOSI, SPI_Soft_Pin_MOSI, GPIO_PIN_SET)
#define SPI_MOSI_L                      HAL_GPIO_WritePin(SPI_Soft_Port_MOSI, SPI_Soft_Pin_MOSI, GPIO_PIN_RESET)
#define SPI_MISO                        (SPI_Soft_Port_MISO->IDR & SPI_Soft_Pin_MISO)
#define SPI_CK_H                        HAL_GPIO_WritePin(SPI_Soft_Port_SCK, SPI_Soft_Pin_SCK, GPIO_PIN_SET)
#define SPI_CK_L                        HAL_GPIO_WritePin(SPI_Soft_Port_SCK, SPI_Soft_Pin_SCK, GPIO_PIN_RESET)

uint8_t SPI_BaudRatePreselection = 0;
uint8_t SPI_BaudRateSelection = 0;
uint16_t SPI_BaudRateDivisor = 1;

/*The above procedure is modified by the user according to the hardware device, otherwise the driver cannot run.*/

void SPI_SetBaudRate(uint16_t BaudRateDivisor)
{

}

/*
 * SPI_WriteData
 *
 * Writes to an 8-bit register with the SPI port
 */
void SPI_WriteData(const uint8_t regAddr, const uint8_t regData)
{
    /* Counter used to clock out the data */
    uint8_t Count;
    /* Define a data structure for the SPI data */
    uint8_t Data;

    /* Make sure we start with active-low CS high and CK low */
    SPI_CS_H;
    SPI_CK_L;

    /* Preload the data to be sent with Address */
    Data = regAddr;
    /* Set active-low CS low to start the SPI cycle */
    SPI_CS_L;
    /* Although Data could be implemented as an "int",
     * resulting in one loop, the routines run faster when two loops
     * are implemented with Data implemented as two "char"s.
     */

    /* Prepare to clock out the Address byte */
    for (Count = 0; Count < 8; Count++) {
        /* Check for a 1 and set the MOSI line appropriately*/
        if (Data & 0x80)
            SPI_MOSI_H;
        else
            SPI_MOSI_L;

        /* Toggle the clock line */
        SPI_CK_H;
        SPI_CK_L;
        /* Rotate to get the next bit */
        Data <<= 1;
        /* and loop back to send the next bit */
    }

    /* Repeat for the Data byte, Preload the data to be sent with Data */
    Data = regData;

    for (Count = 0; Count < 8; Count++) {
        if (Data & 0x80)
            SPI_MOSI_H;
        else
            SPI_MOSI_L;

        SPI_CK_H;
        SPI_CK_L;
        Data <<= 1;
    }

    SPI_CS_H;
    SPI_MOSI_L;
}

/*
 * SPI_ReadData
 *
 * Reads an 8-bit register with the SPI port.
 * Data is returned.
 */
uint8_t SPI_ReadData(const uint8_t regAddr)
{
    /* Counter used to clock out the data */
    uint8_t Count;
    uint8_t Data;

    /* Make sure we start with active-low CS high and CK low */
    SPI_CS_H;
    SPI_CK_L;
    /* Preload the data to be sent with Address and Data */
    Data = regAddr;

    /* Set active-low CS low to start the SPI cycle */
    SPI_CS_L;

    /* Prepare to clock out the Address and Data */
    for (Count = 0; Count < 8; Count++) {
        if (Data & 0x80)
            SPI_MOSI_H;
        else
            SPI_MOSI_L;

        SPI_CK_H;
        SPI_CK_L;
        /* and loop back to send the next bit */
        Data <<= 1;
    }

    /* Reset the MOSI data line */
    SPI_MOSI_L;

    Data = 0;

    /* Prepare to clock in the data to be read */
    for (Count = 0; Count < 8; Count++) {
        /* Rotate the data */
        Data <<= 1;
        /* Raise the clock to clock the data out of the chip */
        SPI_CK_H;
        /* Read the data bit */
        Data += SPI_MISO;
        /* Drop the clock ready for the next bit and loop back */
        SPI_CK_L;
    }

    /* Raise CS */
    SPI_CS_H;
    /* Finally return the read data */
    return ((uint8_t)Data);
}

