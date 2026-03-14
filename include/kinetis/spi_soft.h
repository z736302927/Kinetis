#ifndef __SPI_SOFT_H
#define __SPI_SOFT_H

#ifdef __cplusplus
extern "C" {
#endif

/* The following program is modified by the user according to the hardware device, otherwise the driver cannot run. */

/* Includes ------------------------------------------------------------------*/
#include <linux/types.h>

#include <pthread.h>

/* The above procedure is modified by the user according to the hardware device, otherwise the driver cannot run. */

/* Bit Order */
#define SPI_BIT_ORDER_MSB    0  /* Most Significant Bit First */
#define SPI_BIT_ORDER_LSB    1  /* Least Significant Bit First */

/* SPI Master Structure */
struct spi_master {
	u8 cpol;
	u8 cpha;
	u8 speed;
	u8 bit_order;

	int (*cs_low)(void);
	int (*cs_high)(void);
	int (*mosi_low)(void);
	int (*mosi_high)(void);
	int (*miso_read)(void);
	int (*sck_low)(void);
	int (*sck_high)(void);
	int (*write_byte)(u8 data);
	u8 (*read_byte)();
	int (*write_bytes)(u8 *pdata, u8 length);
	int (*read_bytes)(u8 *pdata, u8 length);
	int (*init)(void);
};

/* SPI Slave Structure */
struct spi_slave {
	char *name;

	u8 cpol;
	u8 cpha;
	u8 bit_order;

	/* State machine */
	u8 last_state;
	u8 current_state;
	u8 bit_count;
	u8 byte_count;
	u8 current_byte;

	/* Direction */
	bool is_read_operation;

	/* Data buffer */
	u8 *buffer;
	u32 buffer_size;
	u8 index;

	/* Thread control variables */
	bool thread_running;
	pthread_t miso_thread;
	pthread_t mosi_thread;
};

int spi_master_soft_init(struct spi_master *master, u8 cpol, u8 cpha, u8 bit_order, u8 speed);
int spi_master_port_transmit(struct spi_master *master, u8 reg, u8 *pdata, u8 length);
int spi_master_port_receive(struct spi_master *master, u8 reg, u8 *pdata, u8 length);
struct spi_slave *spi_slave_soft_init(char *name, u8 cpol, u8 cpha, u8 bit_order,
	u8 *buffer, u32 buffer_size);
void spi_slave_soft_exit(struct spi_slave *device);

extern struct spi_master fake_spi_master;

#ifdef __cplusplus
}
#endif

#endif /* __SPI_SOFT_H */
