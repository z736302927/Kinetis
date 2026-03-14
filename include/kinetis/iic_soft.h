#ifndef __IIC_SOFT_H
#define __IIC_SOFT_H

#ifdef __cplusplus
extern "C" {
#endif

/* The following program is modified by the user according to the hardware device, otherwise the driver cannot run. */

/* Includes ------------------------------------------------------------------*/

#include <linux/types.h>

#include <pthread.h>

/**
 * @brief I2C master device structure
 */
struct iic_master {
	u8 id;                         /* Device identifier */
	void (*scl_low)(void);         /* Set SCL pin low */
	void (*scl_high)(void);        /* Set SCL pin high */
	void (*sda_low)(void);         /* Set SDA pin low */
	void (*sda_high)(void);        /* Set SDA pin high */
	void (*sda_in)(void);         /* Set SDA pin to input mode */
	void (*sda_out)(void);        /* Set SDA pin to output mode */
	int (*sda_read)(void);        /* Read SDA pin level */
	int (*write_bytes)(u8 slave_addr, u16 reg,
		u8 *pdata, u8 length);
	int (*read_bytes)(u8 slave_addr, u16 reg,
		u8 *pdata, u8 length);	
	int (*init)(void);
};

/* Slave communication variables */
struct iic_slave {
	char *name;

	/* State machine */
	u8 last_state;
	u8 current_state;
	u8 bit_count;
	u8 byte_count;
	u8 current_byte;

	/* Address and direction */
	u8 slave_address;
	bool is_read_operation;
	bool address_matched;

	/* Data buffer */
	u8 *buffer;
	u32 buffer_size;
	u8 index;

	/* Status flags */
	bool start_condition_detected;
	bool stop_condition_detected;
	bool master_ack;
	bool detect_start;
	bool detect_stop;
	bool byte_received;

	/* Thread control variables */
	bool thread_running;
	pthread_t thread;
};

struct iic_slave *iic_slave_soft_init(char *name, u8 slave_addr, u8 *buffer, u32 buffer_size);
void iic_slave_soft_exit(struct iic_slave *device);
int iic_master_soft_start(struct iic_master *master);
void iic_master_soft_stop(struct iic_master *master);
int iic_master_soft_wait_ack(struct iic_master *master);
int iic_master_soft_send_byte(struct iic_master *master, u8 tmp);
u8 iic_master_soft_read_byte(struct iic_master *master, u8 ack);
void iic_master_soft_ack(struct iic_master *master);
void iic_master_soft_no_ack(struct iic_master *master);
int iic_master_port_transmit (struct iic_master *master, u8 slave_addr, u16 reg, u8 tmp);
int iic_master_port_receive(struct iic_master *master, u8 slave_addr, u16 reg, u8 *tmp);
int iic_master_port_multi_transmit (struct iic_master *master, u8 slave_addr, u16 reg,
	u8 *pdata, u8 length);
int iic_master_port_multi_receive(struct iic_master *master, u8 slave_addr, u16 reg,
	u8 *pdata, u8 length);
int iic_master_soft_init(struct iic_master *master);

extern struct iic_master fake_iic_master;

/* The following program is modified by the user according to the hardware device, otherwise the driver cannot run. */

#endif /* __IIC_SOFT_H */
