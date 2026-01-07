#ifndef __SPI_SOFT_H
#define __SPI_SOFT_H

#ifdef __cplusplus
extern "C" {
#endif

/* The following program is modified by user according to the hardware device, otherwise the driver cannot run. */

/* Includes ------------------------------------------------------------------*/
#include <linux/types.h>

/* The above procedure is modified by user according to the hardware device, otherwise the driver cannot run. */

/* SPI Software/Hardware definitions */
#define SPI_SW_1    1  /* Software SPI 1 */
#define SPI_SW_2    2  /* Software SPI 2 */
#define SPI_HW_1    3  /* Hardware SPI 1 */

/* SPI Mode Configuration */
#define SPI_MODE_0    0  /* CPOL=0, CPHA=0 */
#define SPI_MODE_1    1  /* CPOL=0, CPHA=1 */
#define SPI_MODE_2    2  /* CPOL=1, CPHA=0 */
#define SPI_MODE_3    3  /* CPOL=1, CPHA=1 */

/* Bit Order */
#define SPI_BIT_ORDER_MSB    0  /* Most Significant Bit First */
#define SPI_BIT_ORDER_LSB    1  /* Least Significant Bit First */

/* Speed Mode */
#define SPI_SPEED_SLOW    0  /* 100kHz Slow Mode */
#define SPI_SPEED_FAST    1  /* 1MHz Fast Mode */

/* Function Prototypes */
void spi_master_soft_init(void);
void spi_master_soft_delay(u32 ticks);
void spi_master_port_transmit(u8 spi, u8 reg, u8 *pdata, u8 length);
void spi_master_port_receive(u8 spi, u8 reg, u8 *pdata, u8 length);
int spi_master_soft_send_byte(u8 spi, u8 data);
u8 spi_master_soft_read_byte(u8 spi);
void spi_set_mode(u8 mode);
void spi_set_bit_order(u8 order);
void spi_set_speed(u8 speed);


#ifdef __cplusplus
}
#endif

#endif /* __SPI_SOFT_H */
