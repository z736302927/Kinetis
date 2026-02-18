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

/* SPI Master Structure */
struct spi_master {
	u8 id;
	int (*cs_low)(void);
	int (*cs_high)(void);
	int (*mosi_low)(void);
	int (*mosi_high)(void);
	int (*miso_read)(void);
	int (*sck_low)(void);
	int (*sck_high)(void);
	int (*write_bytes)(u8 reg, u8 *pdata, u8 length);
	int (*read_bytes)(u8 reg, u8 *pdata, u8 length);
	int (*init)(void);
};

/* SPI Slave Structure */
struct spi_slave {
	char *name;

	/* State machine */
	u8 last_state;
	u8 current_state;
	u8 bit_count;
	u8 byte_count;
	u8 current_byte;

	/* Data buffer */
	u8 *buffer;
	u32 buffer_size;
	u8 index;

	/* Status flags */
	bool cs_asserted;

	/* Thread control variables */
	bool thread_running;
	pthread_t thread;
};

/* Function Prototypes */
/* Master functions */
int spi_master_soft_send_byte(struct spi_master *master, u8 data);
u8 spi_master_soft_read_byte(struct spi_master *master);
u8 spi_master_soft_transfer_byte(struct spi_master *master, u8 data);
int spi_master_soft_init_s(struct spi_master *master);
int spi_master_port_transmit(struct spi_master *master, u8 reg, u8 *pdata, u8 length);
int spi_master_port_receive(struct spi_master *master, u8 reg, u8 *pdata, u8 length);
void spi_master_soft_init(void);
void spi_master_soft_delay(u32 ticks);
void spi_set_mode(u8 mode);
void spi_set_bit_order(u8 order);
void spi_set_speed(u8 speed);

/* Slave functions */
struct spi_slave *spi_slave_soft_init(char *name, u8 *buffer, u32 buffer_size);
void spi_slave_soft_exit(struct spi_slave *device);

/* Test function */
int spi_slave_test(void);

/* External variables */
extern struct spi_master fake_spi_master;
extern struct spi_master spi_master_1;

/* Legacy functions for backward compatibility */
void spi_write_data(const u8 reg, const u8 val);
u8 spi_read_data(const u8 reg);
void spi_set_baudrate(u16 baudrate_divisor);


#ifdef __cplusplus
}
#endif

#endif /* __SPI_SOFT_H */
