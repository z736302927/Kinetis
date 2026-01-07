#ifndef __IIC_SOFT_H
#define __IIC_SOFT_H

#ifdef __cplusplus
extern "C" {
#endif

/* The following program is modified by the user according to the hardware device, otherwise the driver cannot run. */

/* Includes ------------------------------------------------------------------*/

#include <linux/types.h>

#include <pthread.h>

#define IIC_SW_1                        1
#define IIC_SW_2                        2
#define IIC_HW_1                        3
#define IIC_HW_2                        4


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
int iic_master_soft_start(u8 iic);
void iic_master_soft_stop(u8 iic);
int iic_master_soft_wait_ack(u8 iic);
int iic_master_soft_send_byte(u8 iic, u8 tmp);
u8 iic_master_soft_read_byte(u8 iic, u8 ack);
void iic_master_port_transmmit(u8 iic, u8 slave_addr, u16 reg, u8 tmp);
void iic_master_port_receive(u8 iic, u8 slave_addr, u16 reg, u8 *tmp);
void iic_master_port_multi_transmmit(u8 iic, u8 slave_addr, u16 reg,
    u8 *pdata, u8 length);
void iic_master_port_multi_receive(u8 iic, u8 slave_addr, u16 reg,
    u8 *pdata, u8 length);

int iic_slave_test(void);

/* The following program is modified by the user according to the hardware device, otherwise the driver cannot run. */

#endif /* __IIC_SOFT_H */
