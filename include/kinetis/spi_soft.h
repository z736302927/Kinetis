#ifndef __SPI_SOFT_H
#define __SPI_SOFT_H

#ifdef __cplusplus
extern "C" {
#endif

/* The following program is modified by the user according to the hardware device, otherwise the driver cannot run. */

/* Includes ------------------------------------------------------------------*/
#include <linux/types.h>

/* The above procedure is modified by the user according to the hardware device, otherwise the driver cannot run. */

void spi_set_baudrate(u16 baudrate_divisor);
void spi_write_data(const u8 reg, const u8 tmp);
u8 spi_read_data(const u8 reg);


#ifdef __cplusplus
}
#endif

#endif /* __SPI_SOFT_H */
