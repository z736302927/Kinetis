#include "kinetis/spi_soft.h"

/* The following program is modified by the user according to the hardware device, otherwise the driver cannot run. */

#if MCU_PLATFORM_STM32
#include "stm32f4xx_hal.h"
#else
#endif

/**
  * @step 1:  Modify the corresponding function according to the modified area and the corresponding function name.
  * @step 2:  Modify four areas: GPIO_PORT/GPIO_PIN/Led_TypeDef/LEDn.
  * @step 3:  .
  * @step 4:  .
  * @step 5:
  */

#define SPI_SOFT_CR1_LSBFE              0x01
#define SPI_SOFT_CR1_CPHA               0x04
#define SPI_SOFT_CR1_CPOL               0x08
#define SPI_SOFT_CR1_MSTR               0x10
#define SPI_SOFT_BR_SPR0                0x01
#define SPI_SOFT_BR_SPR1                0x02
#define SPI_SOFT_BR_SPR2                0x04
#define SPI_SOFT_BR_SPPR0               0x10
#define SPI_SOFT_BR_SPPR1               0x20
#define SPI_SOFT_BR_SPPR2               0x40

#if MCU_PLATFORM_STM32
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
#else
#define SPI_Soft_Pin_CS
#define SPI_Soft_Pin_MOSI
#define SPI_Soft_Pin_MISO
#define SPI_Soft_Pin_SCK
#define SPI_Soft_Port_CS
#define SPI_Soft_Port_MOSI
#define SPI_Soft_Port_MISO
#define SPI_Soft_Port_SCK

#define SCL_CLOCK_ENABLE
#define SDA_CLOCK_ENABLE

#define SPI_CS_H
#define SPI_CS_L
#define SPI_MOSI_H
#define SPI_MOSI_L
#define SPI_MISO
#define SPI_CK_H
#define SPI_CK_L
#endif

struct spi_baudrate {
	u8 preselection;
	u8 selection;
	u16 divisor;
};

/* The above procedure is modified by the user according to the hardware device, otherwise the driver cannot run. */

void spi_set_baudrate(u16 baudrate_divisor)
{

}

/*
 * spi_write_data
 *
 * Writes to an 8-bit register with the SPI port
 */
void spi_write_data(const u8 reg, const u8 val)
{
	/* cnter used to clock out the data */
	u8 cnt;
	/* Define a data structure for the SPI data */
	u8 tmp;

	/* Make sure we start with active-low CS high and CK low */
	SPI_CS_H;
	SPI_CK_L;

	/* Preload the data to be sent with Address */
	tmp = reg;
	/* Set active-low CS low to start the SPI cycle */
	SPI_CS_L;
	/* Although tmp could be implemented as an "int",
	 * resulting in one loop, the routines run faster when two loops
	 * are implemented with tmp implemented as two "char"s.
	 */

	/* Prepare to clock out the Address byte */
	for (cnt = 0; cnt < 8; cnt++) {
		/* Check for a 1 and set the MOSI line appropriately*/
		if (tmp & 0x80) {
			SPI_MOSI_H;
		} else {
			SPI_MOSI_L;
		}

		/* Toggle the clock line */
		SPI_CK_H;
		SPI_CK_L;
		/* Rotate to get the next bit */
		tmp <<= 1;
		/* and loop back to send the next bit */
	}

	/* Repeat for the tmp byte, Preload the data to be sent with tmp */
	tmp = val;

	for (cnt = 0; cnt < 8; cnt++) {
		if (tmp & 0x80) {
			SPI_MOSI_H;
		} else {
			SPI_MOSI_L;
		}

		SPI_CK_H;
		SPI_CK_L;
		tmp <<= 1;
	}

	SPI_CS_H;
	SPI_MOSI_L;
}

/*
 * spi_read_data
 *
 * Reads an 8-bit register with the SPI port.
 * tmp is returned.
 */
u8 spi_read_data(const u8 reg)
{
	/* cnter used to clock out the data */
	u8 cnt;
	u8 tmp;

	/* Make sure we start with active-low CS high and CK low */
	SPI_CS_H;
	SPI_CK_L;
	/* Preload the data to be sent with Address and tmp */
	tmp = reg;

	/* Set active-low CS low to start the SPI cycle */
	SPI_CS_L;

	/* Prepare to clock out the Address and tmp */
	for (cnt = 0; cnt < 8; cnt++) {
		if (tmp & 0x80) {
			SPI_MOSI_H;
		} else {
			SPI_MOSI_L;
		}

		SPI_CK_H;
		SPI_CK_L;
		/* and loop back to send the next bit */
		tmp <<= 1;
	}

	/* Reset the MOSI data line */
	SPI_MOSI_L;

	tmp = 0;

	/* Prepare to clock in the data to be read */
	for (cnt = 0; cnt < 8; cnt++) {
		/* Rotate the data */
		tmp <<= 1;
		/* Raise the clock to clock the data out of the chip */
		SPI_CK_H;
		/* Read the data bit */
// 		tmp += SPI_MISO;
		/* Drop the clock ready for the next bit and loop back */
		SPI_CK_L;
	}

	/* Raise CS */
	SPI_CS_H;
	/* Finally return the read data */
	return ((u8)tmp);
}
