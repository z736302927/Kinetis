#ifndef __SPI_SOFT_H
#define __SPI_SOFT_H

#ifdef __cplusplus
extern "C" {
#endif

/* The following program is modified by the user according to the hardware device, otherwise the driver cannot run. */

/* Includes ------------------------------------------------------------------*/
#include <linux/types.h>

#ifdef KINETIS_FAKE_SIM
#include <pthread.h>
#endif

/* The above procedure is modified by the user according to the hardware device, otherwise the driver cannot run. */

/* Bit Order */
#define SPI_BIT_ORDER_MSB    0  /* Most Significant Bit First */
#define SPI_BIT_ORDER_LSB    1  /* Least Significant Bit First */

enum qspi_read_mode {
	QSPI_READ_STANDARD     = 0,
	QSPI_READ_FAST         = 1,
	QSPI_READ_DUAL_OUTPUT  = 2,
	QSPI_READ_DUAL_IO      = 3,
	QSPI_READ_QUAD_OUTPUT  = 4,
	QSPI_READ_QUAD_IO      = 5,
};

struct qspi_request_block {
	u32 instruction;
	u8 instruction_mode;
	u32 address;
	u32 address_size;
	u8 address_mode;
	u32 alternate_bytes;
	u32 alternate_bytes_size;
	u8 alternate_byte_mode;
	u8 dummy_cycles;
	u8 transfer_mode;
	void* transfer_buffer;
	u32 transfer_length;
	u8 ddr_mode;
	u8 ddr_hold_half_cycle;
};

/* SPI Master Structure */
struct spi_master {
	u8 cpol;
	u8 cpha;
	u8 speed;
	u8 bit_order;

	int (*init)(void);
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

	void (*qspi_read)(struct qspi_request_block *qrb);
	void (*qspi_write)(struct qspi_request_block *qrb);
	void (*set_mode)(enum qspi_read_mode mode);
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

#ifdef KINETIS_FAKE_SIM
	/* Thread control variables */
	bool thread_running;
	pthread_t miso_thread;
	pthread_t mosi_thread;
#endif
};

int spi_master_soft_init(struct spi_master *master, u8 cpol, u8 cpha, u8 bit_order, u8 speed);
int spi_master_port_transmit(struct spi_master *master, u8 reg, u8 *pdata, u8 length);
int spi_master_port_receive(struct spi_master *master, u8 reg, u8 *pdata, u8 length);
struct spi_slave *spi_slave_soft_init(char *name, u8 cpol, u8 cpha, u8 bit_order,
	u8 *buffer, u32 buffer_size);
void spi_slave_soft_exit(struct spi_slave *device);

extern struct spi_master general_spi_master;

#ifdef __cplusplus
}
#endif

#endif /* __SPI_SOFT_H */
