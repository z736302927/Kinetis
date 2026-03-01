
#undef DEBUG
#undef VERBOSE_DEBUG

#include <linux/printk.h>
#include <linux/delay.h>
#include <linux/random.h>
#include <linux/string.h>
#include <linux/errno.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <stdbool.h>

#include <unistd.h>

#include "kinetis/iic_soft.h"
#include "kinetis/idebug.h"
#include "kinetis/design_verification.h"

/* The following program is modified by the user according to the hardware device, otherwise the driver cannot run. */

/*
 * Note: This file now uses pthread instead of kernel threads.
 * When compiling, make sure to link with the pthread library:
 * gcc -lpthread ... (or add -lpthread to your Makefile LDFLAGS)
 */

/**
  * @step 1:  Modify the corresponding function according to the modified area and the corresponding function name.
  * @step 2:  Modify the read-write function of IIC corresponding pin in the header file.
  * @step 3:  Modify the read-write address length in the header file.
  */

#if MCU_PLATFORM_STM32
#else
#endif

#if MCU_PLATFORM_STM32
#include "stm32f4xx_hal.h"
//#include "i2c.h"
#else
#endif

//static u8 IIC_Soft_FastMode = 1;

/* I2C Speed Mode Selection */
#define IIC_SPEED_STANDARD    1  /* 100kHz Standard Mode */
#define IIC_SPEED_FAST        2  /* 400kHz Fast Mode */

/* Select I2C speed mode */
#define IIC_SPEED_MODE        IIC_SPEED_FAST

/* I2C timing parameters based on speed mode */
#if IIC_SPEED_MODE == IIC_SPEED_STANDARD
/* Standard Mode (100kHz) - Clock period 10μs */
#define IIC_DELAY_SCL_LOW       5
#define IIC_DELAY_SCL_HIGH     5
#define IIC_DELAY_SDA          5
#define IIC_DELAY_START_SETUP  5
#define IIC_DELAY_START_HOLD   4
#define IIC_DELAY_STOP_SETUP   4
#define IIC_DELAY_DATA_SETUP   2
#define IIC_DELAY_DATA_HOLD    2
#define IIC_DELAY_ACK_HOLD     5
#else
/* Fast Mode (400kHz) - Clock period 2.5μs */
#define IIC_DELAY_SCL_LOW       1
#define IIC_DELAY_SCL_HIGH     1
#define IIC_DELAY_SDA          1
#define IIC_DELAY_START_SETUP  1
#define IIC_DELAY_START_HOLD   1
#define IIC_DELAY_STOP_SETUP   1
#define IIC_DELAY_DATA_SETUP   1
#define IIC_DELAY_DATA_HOLD    1
#define IIC_DELAY_ACK_HOLD     1
#endif

#define ADDRESS_16      1
#define ADDRESS_8       0
#define ADDRESS_MODE    ADDRESS_8

/* The above procedure is modified by the user according to the hardware device, otherwise the driver cannot run. */

void iic_master_soft_delay(u32 ticks)
{
	// 	pr_debug("iic_master: delay %d us", ticks);
	mdelay(ticks);
}

int iic_master_soft_start(struct iic_master *master)
{
	master->sda_out();
	master->sda_high();
	master->scl_high();
	iic_master_soft_delay(IIC_DELAY_START_SETUP);

	if (!master->sda_read()) {
		return -EBUSY;
	}

	master->sda_low();
	iic_master_soft_delay(IIC_DELAY_START_HOLD);

	if (master->sda_read()) {
		return -EBUSY;
	}

	return 0;
}

void iic_master_soft_stop(struct iic_master *master)
{
	master->sda_out();
	master->sda_low();
	iic_master_soft_delay(IIC_DELAY_STOP_SETUP);
	master->scl_high();
	iic_master_soft_delay(IIC_DELAY_STOP_SETUP);
	master->sda_high();
	iic_master_soft_delay(IIC_DELAY_STOP_SETUP);
}

void iic_master_soft_ack(struct iic_master *master)
{
	master->sda_out();
	master->sda_low();
	master->scl_low();
	iic_master_soft_delay(IIC_DELAY_DATA_SETUP);
	master->scl_high();
	iic_master_soft_delay(IIC_DELAY_DATA_HOLD);
	master->scl_low();
	iic_master_soft_delay(IIC_DELAY_DATA_HOLD);
}

void iic_master_soft_no_ack(struct iic_master *master)
{
	master->sda_out();
	master->sda_high();
	master->scl_low();
	iic_master_soft_delay(IIC_DELAY_DATA_SETUP);
	master->scl_high();
	iic_master_soft_delay(IIC_DELAY_DATA_HOLD);
	master->scl_low();
	iic_master_soft_delay(IIC_DELAY_DATA_HOLD);
}

int iic_master_soft_wait_ack(struct iic_master *master)
{
	master->sda_in();
	master->scl_low();
	iic_master_soft_delay(IIC_DELAY_DATA_SETUP);
	master->scl_high();
	iic_master_soft_delay(IIC_DELAY_DATA_HOLD);

	/* Read SDA while SCL is high - this is when master should see ACK */
	if (master->sda_read()) {
		/* SDA is high = NACK */
		pr_debug("iic_master: Received NACK");
		master->scl_low();
		iic_master_soft_delay(IIC_DELAY_SCL_LOW);
		return -EIO;
	} else {
		/* SDA is low = ACK */
		pr_debug("iic_master: Received ACK");
		master->scl_low();
		iic_master_soft_delay(IIC_DELAY_SCL_LOW);
		return 0;
	}
}

int iic_master_soft_send_byte(struct iic_master *master, u8 tmp)
{
	u8 i = 8;
	int ret;

	if (!master) {
		return -EINVAL;
	}

	master->sda_out();

	while (i--) {
		master->scl_low();
		iic_master_soft_delay(IIC_DELAY_SCL_LOW);

		if (tmp & 0x80) {
			master->sda_high();
		} else {
			master->sda_low();
		}

		tmp <<= 1;
		iic_master_soft_delay(IIC_DELAY_DATA_SETUP);
		master->scl_high();
		iic_master_soft_delay(IIC_DELAY_SCL_HIGH);
	}
	master->scl_low();
	iic_master_soft_delay(IIC_DELAY_SCL_LOW);

	ret = iic_master_soft_wait_ack(master);
	if (ret) {
		iic_master_soft_stop(master);
		return ret;
	}

	return 0;
}

u8 iic_master_soft_read_byte(struct iic_master *master, u8 ack)
{
	u8 i = 8, tmp = 0;

	master->sda_in();
	// 	scl_high();
	// 	// sda_high();
	// 	iic_master_soft_delay(IIC_DELAY_DATA_SETUP);

	while (i) {
		master->scl_low();
		iic_master_soft_delay(IIC_DELAY_SCL_LOW);
		master->scl_high();
		iic_master_soft_delay(IIC_DELAY_SCL_HIGH);

		if (master->sda_read()) {
			tmp |= 0x01 << (i - 1);
		}
		i--;
		pr_debug("iic_master: Received data byte 0x%02X",
			tmp);
	}

	master->scl_low();

	if (ack) {
		iic_master_soft_ack(master);
	} else {
		iic_master_soft_no_ack(master);
	}

	return tmp;
}

static int iic_master_soft_write_bytes_with_addr(struct iic_master *master, u8 slave_addr, u16 reg,
	u8 *pdata, u8 length)
{
	int ret;

	if (!master || !pdata || length == 0) {
		return -EINVAL;
	}

	if (iic_master_soft_start(master) != 0) {
		pr_err("Arbitration failed ! Device(addr = 0x%X) cannot obtain the bus.",
			slave_addr);
		return -EBUSY;
	}

	ret = iic_master_soft_send_byte(master, (slave_addr << 1) | 0x00);
	if (ret) {
		return ret;
	}

	if (ADDRESS_MODE == ADDRESS_16) {
		ret = iic_master_soft_send_byte(master, reg >> 8);
		if (ret) {
			return ret;
		}
	}

	ret = iic_master_soft_send_byte(master, reg & 0xFF);
	if (ret) {
		return ret;
	}

	while (length--) {
		ret = iic_master_soft_send_byte(master, *pdata);
		if (ret) {
			return ret;
		}
		pdata++;
	}

	iic_master_soft_stop(master);

	return 0;
}

static int iic_master_soft_read_bytes_with_addr(struct iic_master *master, u8 slave_addr, u16 reg,
	u8 *pdata, u8 length)
{
	int ret;

	if (!master || !pdata || length == 0) {
		return -EINVAL;
	}

	if (iic_master_soft_start(master) != 0) {
		pr_err("Arbitration failed ! Device(addr = 0x%X) cannot obtain the bus.",
			slave_addr);
		return -EBUSY;
	}

	ret = iic_master_soft_send_byte(master, (slave_addr << 1) | 0x00);
	if (ret) {
		return ret;
	}

	if (ADDRESS_MODE == ADDRESS_16) {
		ret = iic_master_soft_send_byte(master, reg >> 8);
		if (ret) {
			return ret;
		}
	}

	ret = iic_master_soft_send_byte(master, reg & 0xFF);
	if (ret) {
		return ret;
	}

	ret = iic_master_soft_start(master);
	if (ret) {
		return ret;
	}

	ret = iic_master_soft_send_byte(master, (slave_addr << 1) | 0x01);
	if (ret) {
		return ret;
	}

	while (length) {
		if (length == 1) {
			*pdata = iic_master_soft_read_byte(master, 0);
		} else {
			*pdata = iic_master_soft_read_byte(master, 1);
		}

		pdata++;
		length--;
	}

	iic_master_soft_stop(master);

	return 0;
}

#ifdef KINETIS_FAKE_SIM
/* I2C Slave state machine */
enum {
	IIC_SLAVE_STATE_IDLE = 0,
	IIC_SLAVE_STATE_MATCH_ADDRESS,
	IIC_SLAVE_STATE_RECEIVING_DATA,
	IIC_SLAVE_STATE_TRANSMITTING_DATA,
	IIC_SLAVE_STATE_STOP_RECEIVED
};

/* Bus simulation variables */
struct {
	bool scl_line;
	bool sda_line;
	u8 master_sda_direction;  /* 0 = input, 1 = output */
	u8 slave_sda_direction;   /* 0 = input, 1 = output */
} i2c_bus_simulation = {
	.scl_line = 1,  /* Pull-up high by default */
	.sda_line = 1,  /* Pull-up high by default */
	.master_sda_direction = 1,  /* Master usually controls SDA */
	.slave_sda_direction = 0    /* Slave usually receives SDA */
};

static const char *iic_slave_get_state_name(u8 state)
{
	switch (state) {
	case IIC_SLAVE_STATE_IDLE:
		return "IDLE";
	case IIC_SLAVE_STATE_MATCH_ADDRESS:
		return "MATCH_ADDRESS";
	case IIC_SLAVE_STATE_RECEIVING_DATA:
		return "RECEIVING_DATA";
	case IIC_SLAVE_STATE_TRANSMITTING_DATA:
		return "TRANSMITTING_DATA";
	case IIC_SLAVE_STATE_STOP_RECEIVED:
		return "STOP_RECEIVED";
	default:
		return "UNKNOWN";
	}
}

/**
 * @brief Read current SCL and SDA line states
 */
static void iic_slave_read_bus_lines(bool *scl, bool *sda)
{
	*scl = i2c_bus_simulation.scl_line;
	*sda = i2c_bus_simulation.sda_line;
}

/**
 * @brief Set SDA line state (for slave output)
 */
static void iic_slave_set_sda(bool state)
{
	i2c_bus_simulation.sda_line = state;
	/* Slave should always drive the SDA line when transmitting */
	i2c_bus_simulation.slave_sda_direction = 1;
}

static void iic_slave_handle_byte_received(struct iic_slave *device, u8 byte)
{
	if (device->index < device->buffer_size) {
		device->buffer[device->index] = byte;
		pr_debug("iic_slave(%s): stored byte 0x%02X at buffer index %d",
			device->name, byte, device->index);
		device->index++;
	}
}

static u8 iic_slave_get_next_tx_byte(struct iic_slave *device)
{
	if (device->index < device->buffer_size) {
		device->index++;
		pr_debug("iic_slave(%s): read byte 0x%02X from buffer index %d",
			device->name, device->buffer[device->index - 1], device->index - 1);
		return device->buffer[device->index - 1];
	}

	return 0xFF; /* Default data if no more data */
}

/**
 * @brief Main I2C slave state machine loop (thread function)
 */
static void *iic_slave_state_machine_thread(void *data)
{
	struct iic_slave *device = (struct iic_slave *)data;
	bool scl_current, sda_current;
	bool scl_last = 1, sda_last = 1;

	/* Set thread to cancelable */
	pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
	pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, NULL);

	while (device->thread_running) {
		/* Read current bus line states */
		iic_slave_read_bus_lines(&scl_current, &sda_current);

		/* State machine - switch case implementation */
		switch (device->current_state) {
		case IIC_SLAVE_STATE_IDLE:
			/* Detect START condition: SDA falls while SCL is high */
			if (scl_last && scl_current && sda_last && !sda_current) {
				device->current_state = IIC_SLAVE_STATE_MATCH_ADDRESS;
				device->start_condition_detected = 1;
				device->bit_count = 0;
				device->byte_count = 0;
				device->current_byte = 0;
				pr_debug("iic_slave(%s): START condition detected", device->name);
			}
			break;

		case IIC_SLAVE_STATE_MATCH_ADDRESS:
			/* Wait for SCL high to sample address bits */
			if (!scl_last && scl_current) {  /* SCL rising edge */
				device->bit_count++;

				if (device->bit_count < 9) {
					/* Sample SDA bit */
					if (sda_current) {
						device->current_byte |= 1 << (8 - device->bit_count);
					}

					pr_debug("iic_slave(%s): Received bit%d(%d)",
						device->name, 8 - device->bit_count, sda_current);

					if (device->bit_count == 8) {
						/* Address byte complete */
						u8 received_addr = device->current_byte >> 1;
						device->is_read_operation = device->current_byte & 0x01;

						if (received_addr == device->slave_address) {
							device->address_matched = 1;
							pr_debug("iic_slave(%s): Address 0x%02X matched, %s operation",
								device->name, received_addr,
								device->is_read_operation ? "READ" : "WRITE");
						} else {
							device->current_state = IIC_SLAVE_STATE_IDLE;
						}
					}
				}
			} else if ((scl_last && !scl_current) || (!scl_last && !scl_current)) {
				if (device->bit_count == 8) {
					/* Send ACK only if address matched */
					if (device->address_matched) {
						iic_slave_set_sda(0);
						if (scl_last && !scl_current) {
							pr_debug("iic_slave(%s): Pulled SDA low for ACK on 9th clock cycle", device->name);
						}
					} else {
						/* Address not matched, keep SDA high (NACK) */
						iic_slave_set_sda(1);
						if (scl_last && !scl_current) {
							pr_debug("iic_slave(%s): Address not matched, sending NACK", device->name);
						}
					}
				} else if (device->bit_count == 9) {
					/* SCL falling edge again - ACK cycle complete, release SDA and transition */
					iic_slave_set_sda(1);

					/* Transition to next state based on operation type */
					if (device->is_read_operation) {
						device->current_state = IIC_SLAVE_STATE_TRANSMITTING_DATA;
						device->current_byte = iic_slave_get_next_tx_byte(device);
						device->bit_count = 1;  /* bit7 will be set immediately below */
						/* Prepare first bit (bit7) immediately */
						bool bit = (device->current_byte >> 7) & 0x01;
						iic_slave_set_sda(bit);
						pr_debug("iic_slave(%s): Transmitted bit7(%d)", device->name, bit);
					} else {
						device->current_state = IIC_SLAVE_STATE_RECEIVING_DATA;
						device->current_byte = 0;
						device->bit_count = 0;
					}
				}
			}
			break;

		case IIC_SLAVE_STATE_RECEIVING_DATA:
			/* Receive data bytes from master */
			if (!scl_last && scl_current) {  /* SCL rising edge */
				device->bit_count++;

				if (device->bit_count < 9) {
					/* Sample SDA bit */
					if (sda_current) {
						device->current_byte |= 1 << (8 - device->bit_count);
					}

					pr_debug("iic_slave(%s): Received bit%d(%d)",
						device->name, 8 - device->bit_count, sda_current);
				}

				if (device->bit_count == 2) {
					device->detect_start = 0;
					device->detect_stop = 0;
				} else if (device->bit_count == 8) {
					/* Byte received */
					device->byte_count++;
					if (device->byte_count == 1) {
						if (device->current_byte < device->buffer_size) {
							device->index = device->current_byte;
						}
					} else {
						iic_slave_handle_byte_received(device, device->current_byte);
					}
					pr_debug("iic_slave(%s): Received data byte 0x%02X, byte count: %d",
						device->name, device->current_byte, device->byte_count);
				}
			} else if ((scl_last && !scl_current) || (!scl_last && !scl_current)) {
				if (device->bit_count == 8) {
					/* Send ACK only if address matched */
					if (device->address_matched) {
						iic_slave_set_sda(0);
						if (scl_last && !scl_current) {
							pr_debug("iic_slave(%s): Pulled SDA low for ACK on 9th clock cycle", device->name);
						}
					} else {
						/* Address not matched, keep SDA high (NACK) */
						iic_slave_set_sda(1);
						if (scl_last && !scl_current) {
							pr_debug("iic_slave(%s): Address not matched, sending NACK", device->name);
						}
					}
				} else if (device->bit_count == 9) {
					/* SCL high - master should see ACK, release it */
					iic_slave_set_sda(1);

					/* Reset for next byte */
					device->bit_count = 0;
					device->current_byte = 0;
					device->detect_start = 1;
					device->detect_stop = 1;
				}
			} else if (scl_last && scl_current) {
				if (!sda_last && sda_current && device->detect_stop) {
					device->current_state = IIC_SLAVE_STATE_STOP_RECEIVED;
					device->detect_stop = 0;
					device->bit_count = 0;
					device->current_byte = 0;
					pr_debug("iic_slave(%s): STOP condition detected", device->name);
				} else if (sda_last && !sda_current && device->detect_start) {
					device->current_state = IIC_SLAVE_STATE_MATCH_ADDRESS;
					device->detect_start = 0;
					device->bit_count = 0;
					device->current_byte = 0;
					pr_debug("iic_slave(%s): START condition detected", device->name);
				}
			}
			break;

		case IIC_SLAVE_STATE_TRANSMITTING_DATA:
			/* Transmit data bytes to master */
			if (!scl_last && scl_current) {
				if (device->bit_count == 2) {
					device->detect_stop = 0;
				} else if (device->bit_count == 9) {
					if (sda_current) {
						device->master_ack = 0;
						pr_debug("iic_slave(%s): Received NACK from master", device->name);
					} else {
						device->master_ack = 1;
						pr_debug("iic_slave(%s): Received ACK from master", device->name);
					}

					device->detect_stop = 1;
				}
			} else if ((scl_last && !scl_current) || (!scl_last && !scl_current)) {  /* SCL falling edge, prepare data */
				if (scl_last && !scl_current) {
					device->bit_count++;
				}
				/* Prepare next bit */
				if (device->bit_count < 9) {
					bool bit = (device->current_byte >> (8 - device->bit_count)) & 0x01;
					iic_slave_set_sda(bit);
					if (scl_last && !scl_current) {
						pr_debug("iic_slave(%s): Transmitted bit%d(%d)", device->name, 8 - device->bit_count, bit);
					}
				} else if (device->bit_count == 10) {
					if (device->master_ack) {
						device->current_byte = iic_slave_get_next_tx_byte(device);
						device->byte_count++;
						device->bit_count = 1;  /* bit7 will be set immediately below */
						/* Prepare first bit (bit7) of next byte immediately */
						bool bit = (device->current_byte >> 7) & 0x01;
						iic_slave_set_sda(bit);
						if (scl_last && !scl_current) {
							pr_debug("iic_slave(%s): Transmitted bit7(%d)", device->name, bit);
						}
					} else {
						device->bit_count = 0;
						device->current_byte = 0;
					}
				}
			} else if (scl_last && scl_current) {
				if (!sda_last && sda_current && device->detect_stop) {
					device->current_state = IIC_SLAVE_STATE_STOP_RECEIVED;
					device->detect_stop = 0;
					device->bit_count = 0;
					device->current_byte = 0;
					pr_debug("iic_slave(%s): STOP condition detected", device->name);
				}
			}
			break;

		case IIC_SLAVE_STATE_STOP_RECEIVED:
			/* Reset to idle state */
			device->current_state = IIC_SLAVE_STATE_IDLE;
			device->bit_count = 0;
			device->byte_count = 0;
			device->current_byte = 0;
			device->is_read_operation = 0;
			device->address_matched = 0;
			device->start_condition_detected = 0;
			device->stop_condition_detected = 0;
			device->byte_received = 0;

			/* Release SDA line */
			iic_slave_set_sda(1);
			break;

		default:
			/* Unknown state, reset to idle */
			device->current_state = IIC_SLAVE_STATE_IDLE;
			pr_warn("iic_slave(%s): Unknown state, resetting to IDLE", device->name);
			break;
		}

		/* Enable debug logging for address matching only */
		if (device->current_state != device->last_state) {
			pr_debug("iic_slave(%s): slave state: %s", device->name, iic_slave_get_state_name(device->current_state));
		}

		/* Update last states */
		scl_last = scl_current;
		sda_last = sda_current;
		device->last_state = device->current_state;

		/* Small delay to prevent excessive CPU usage */
		// usleep(1);
	}

	pr_info("iic_slave(%s): State machine thread stopped", device->name);
	return NULL;
}

/**
 * @brief Start the I2C slave state machine thread
 */
static int iic_slave_start_thread(struct iic_slave *device)
{
	int ret;

	if (device->thread_running) {
		pr_warn("iic_slave(%s): Thread already running", device->name);
		return 0;
	}

	device->thread_running = 1;

	ret = pthread_create(&device->thread, NULL, iic_slave_state_machine_thread, device);
	if (ret != 0) {
		device->thread_running = 0;
		pr_err("iic_slave(%s): Failed to create thread: %d", device->name, ret);
		return -ret;
	}

	pr_info("iic_slave(%s): Thread started successfully", device->name);
	return 0;
}

/**
 * @brief Stop the I2C slave state machine thread
 */
static void iic_slave_stop_thread(struct iic_slave *device)
{
	void *thread_ret;

	if (!device->thread_running) {
		return;
	}

	/* Signal thread to stop */
	device->thread_running = 0;

	/* Wait for thread to finish */
	if (pthread_join(device->thread, &thread_ret) == 0) {
		pr_info("iic_slave(%s): Thread stopped successfully", device->name);
	} else {
		pr_warn("iic_slave(%s): Thread stop had issues, but continuing", device->name);
	}

	pr_info("iic_slave(%s): Thread stopped", device->name);
}

/**
 * @brief Stop I2C slave state machine thread
 */
void iic_slave_soft_exit(struct iic_slave *device)
{
	pr_info("iic_slave(%s): Shutting down I2C slave", device->name);

	/* Stop the state machine thread */
	iic_slave_stop_thread(device);

	// 	if (device->name) {
	// 		kfree(device->name);
	// 	}
	kfree(device);
}

/**
 * @brief Initialize I2C slave
 * @param name Device name for logging
 * @param slave_addr 7-bit slave address
 * @param buffer Pointer to internal buffer for data storage
 * @param buffer_size Size of the internal buffer
 */
struct iic_slave *iic_slave_soft_init(char *name, u8 slave_addr, u8 *buffer, u32 buffer_size)
{
	struct iic_slave *device;
	int ret;

	device = kzalloc(sizeof(struct iic_slave), GFP_KERNEL);
	if (!device) {
		return ERR_PTR(-ENOMEM);
	}

	// 	device->name = kstrdup(name, GFP_KERNEL);
	device->name = name;
	device->slave_address = slave_addr;
	device->buffer_size = buffer_size;
	device->buffer = buffer;
	device->last_state = IIC_SLAVE_STATE_IDLE;
	device->current_state = IIC_SLAVE_STATE_IDLE;
	device->index = 0;
	device->start_condition_detected = 0;
	device->stop_condition_detected = 0;
	device->detect_start = 0;
	device->detect_stop = 0;
	device->byte_received = 0;

	pr_debug("iic_slave(%s): initialized with address 0x%02X", name, slave_addr);

	/* Start the state machine thread */
	ret = iic_slave_start_thread(device);
	if (ret != 0) {
		kfree(device);
		return ERR_PTR(ret);
	}

	return device;
}
#endif

int iic_master_soft_init(struct iic_master *master)
{
	if (!master || !master->init) {
		return -EINVAL;
	}

	master->init();
	return 0;
}

int iic_master_port_transmit(struct iic_master *master, u8 slave_addr, u16 reg, u8 tmp)
{
	if (!master) {
		return -EINVAL;
	}

	if (master->write_bytes) {
		return master->write_bytes(slave_addr, reg, &tmp, 1);
	} else {
		return iic_master_soft_write_bytes_with_addr(master, slave_addr, reg, &tmp, 1);
	}
}

int iic_master_port_receive(struct iic_master *master, u8 slave_addr, u16 reg, u8 *tmp)
{
	if (!master || !tmp) {
		return -EINVAL;
	}

	if (master->read_bytes) {
		return master->read_bytes(slave_addr, reg, tmp, 1);
	} else {
		return iic_master_soft_read_bytes_with_addr(master, slave_addr, reg, tmp, 1);
	}
}

int iic_master_port_multi_transmit(struct iic_master *master, u8 slave_addr, u16 reg,
	u8 *pdata, u8 length)
{
	if (!master || !pdata) {
		return -EINVAL;
	}

	if (length == 0) {
		return 0;
	}

	if (master->write_bytes) {
		return master->write_bytes(slave_addr, reg, pdata, length);
	} else {
		return iic_master_soft_write_bytes_with_addr(master, slave_addr, reg, pdata, length);
	}
}

int iic_master_port_multi_receive(struct iic_master *master, u8 slave_addr, u16 reg,
	u8 *pdata, u8 length)
{
	if (!master || !pdata) {
		return -EINVAL;
	}

	if (length == 0) {
		return 0;
	}

	if (master->read_bytes) {
		return master->read_bytes(slave_addr, reg, pdata, length);
	} else {
		return iic_master_soft_read_bytes_with_addr(master, slave_addr, reg, pdata, length);
	}
}

#ifdef DESIGN_VERIFICATION_IIC
#include "kinetis/test-kinetis.h"

#ifdef KINETIS_FAKE_SIM
void fake_scl_low()
{
	pr_debug("iic_master: scl low");
	i2c_bus_simulation.scl_line = 0;
}

void fake_scl_high()
{
	pr_debug("iic_master: scl high");
	i2c_bus_simulation.scl_line = 1;
}

void fake_sda_low()
{
	pr_debug("iic_master: sda low");
	i2c_bus_simulation.sda_line = 0;
}

void fake_sda_high()
{
	pr_debug("iic_master: sda high");
	i2c_bus_simulation.sda_line = 1;
}

void fake_sda_in()
{
	pr_debug("iic_master: sda dir input");
	/* Set SDA direction to input for master (will be controlled by slave) */
	i2c_bus_simulation.master_sda_direction = 0;
	/* When in input mode, let slave control the line if it's driving */
	if (!i2c_bus_simulation.slave_sda_direction) {
		/* Only set to high if slave is not driving the line */
		i2c_bus_simulation.sda_line = 1;
	}
	/* If slave is driving, keep slave's value (should be low for ACK) */
}

void fake_sda_out()
{
	pr_debug("iic_master: sda dir output");
	/* Set SDA direction to output for master */
	i2c_bus_simulation.master_sda_direction = 1;
	i2c_bus_simulation.slave_sda_direction = 0;
	/* When SDA is set to output, it should default to high due to pull-up */
	/* The actual level will be set by sda_high() or sda_low() */
}

int fake_sda_read()
{
	/* Read from shared bus simulation */
	/* Slave driving SDA has highest priority */
	if (i2c_bus_simulation.slave_sda_direction) {
		pr_debug("iic_master: reading sda(%d) from slave", i2c_bus_simulation.sda_line);
		return i2c_bus_simulation.sda_line;
	}
	/* If master is driving SDA, return master's last set value */
	else if (i2c_bus_simulation.master_sda_direction) {
		pr_debug("iic_master: reading sda(%d) from master", i2c_bus_simulation.sda_line);
		return i2c_bus_simulation.sda_line;
	}
	/* Otherwise, return default pull-up state */
	else {
		pr_debug("iic_master: reading sda(1) pull-up");
		return 1; /* Pull-up high by default */
	}
}

int fake_iic_init(void)
{
	return 0;
}
#else
static void scl_low()
{
#if MCU_PLATFORM_STM32
	if (iic == IIC_SW_1) {
		HAL_GPIO_WritePin(GPIOB, GPIO_PIN_8, GPIO_PIN_RESET);
	} else if (iic == IIC_SW_2) {
		HAL_GPIO_WritePin(GPIOH, GPIO_PIN_6, GPIO_PIN_RESET);
	}
#else
#endif
}

static void scl_high()
{
#if MCU_PLATFORM_STM32
	if (iic == IIC_SW_1) {
		HAL_GPIO_WritePin(GPIOB, GPIO_PIN_8, GPIO_PIN_SET);
	} else if (iic == IIC_SW_2) {
		HAL_GPIO_WritePin(GPIOH, GPIO_PIN_6, GPIO_PIN_SET);
	}
#else
#endif
}

static void sda_low()
{
#if MCU_PLATFORM_STM32
	if (iic == IIC_SW_1) {
		HAL_GPIO_WritePin(GPIOB, GPIO_PIN_9, GPIO_PIN_RESET);
	} else if (iic == IIC_SW_2) {
		HAL_GPIO_WritePin(GPIOI, GPIO_PIN_3, GPIO_PIN_RESET);
	}
#else
#endif
}

static void sda_high()
{
#if MCU_PLATFORM_STM32
	if (iic == IIC_SW_1) {
		HAL_GPIO_WritePin(GPIOB, GPIO_PIN_9, GPIO_PIN_SET);
	} else if (iic == IIC_SW_2) {
		HAL_GPIO_WritePin(GPIOI, GPIO_PIN_3, GPIO_PIN_SET);
	}
#else
#endif
}

static void sda_in()
{
#if MCU_PLATFORM_STM32
	GPIO_InitTypeDef gpio = {
		.Pull = GPIO_PULLUP,
		.Speed = GPIO_SPEED_FREQ_VERY_HIGH
	};
	/*
	 * It doesn't have to switch direction in
	 * open drain mode.
	 */
	if (iic == IIC_SW_1) {
		gpio.Pin = GPIO_PIN_9;
		gpio.Mode = GPIO_MODE_INPUT;
		HAL_GPIO_Init(GPIOB, &gpio);
	} else if (iic == IIC_SW_2) {
		gpio.Pin = GPIO_PIN_3;
		gpio.Mode = GPIO_MODE_INPUT;
		HAL_GPIO_Init(GPIOI, &gpio);
	}
#else
#endif
}

static void sda_out()
{
#if MCU_PLATFORM_STM32
	GPIO_InitTypeDef gpio = {
		.Pull = GPIO_PULLUP,
		.Speed = GPIO_SPEED_FREQ_VERY_HIGH
	};
	/*
	 * It doesn't have to switch direction in
	 * open drain mode.
	 */
	if (iic == IIC_SW_1) {
		gpio.Pin = GPIO_PIN_9;
		gpio.Mode = GPIO_MODE_OUTPUT_OD;
		HAL_GPIO_Init(GPIOB, &gpio);
	} else if (iic == IIC_SW_2) {
		gpio.Pin = GPIO_PIN_3;
		gpio.Mode = GPIO_MODE_OUTPUT_OD;
		HAL_GPIO_Init(GPIOI, &gpio);
	}
#else
#endif
}

static int sda_read()
{
#if MCU_PLATFORM_STM32
	if (iic == IIC_SW_1) {
		return HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_9);
	} else if (iic == IIC_SW_2) {
		return HAL_GPIO_ReadPin(GPIOI, GPIO_PIN_3);
	}
#else
	return 0;
#endif
}

void iic_master_soft_init()
{
#if MCU_PLATFORM_STM32
	GPIO_InitTypeDef GPIO_InitStruct = {0};

	/* GPIO Ports Clock Enable */
	__HAL_RCC_GPIOB_CLK_ENABLE();

	/*Configure GPIO pin : PF6 */
	GPIO_InitStruct.Pin = GPIO_PIN_8;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;
	GPIO_InitStruct.Pull = GPIO_PULLUP;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
	HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

	GPIO_InitStruct.Pin = GPIO_PIN_9;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;
	GPIO_InitStruct.Pull = GPIO_PULLUP;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
	HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

	/*Configure GPIO pin Output Level */
	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_8, GPIO_PIN_SET);
	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_9, GPIO_PIN_SET);
#else
#endif
}

void stm32_iic_master_transmit(u8 slave_addr, u16 reg,
	u8 *pdata, u8 length)
{
#if MCU_PLATFORM_STM32
	HAL_I2C_Mem_Write_DMA(&hi2c1, (u16)slave_addr, reg, I2C_MEMADD_SIZE_8BIT,
		pdata, length);
#else
#endif
}

void stm32_master_receive(u8 slave_addr, u16 reg,
	u8 *pdata, u8 length)
{
#if MCU_PLATFORM_STM32
	HAL_I2C_Mem_Read_DMA(&hi2c1, (u16)(slave_addr), reg, I2C_MEMADD_SIZE_8BIT,
		pdata, length);
#else
#endif
}
#endif

struct iic_master fake_master = {
	.init = fake_iic_init,
	.sda_out = fake_sda_out,
	.sda_in = fake_sda_in,
	.sda_high = fake_sda_high,
	.sda_low = fake_sda_low,
	.sda_read = fake_sda_read,
	.scl_high = fake_scl_high,
	.scl_low = fake_scl_low,
	.write_bytes = NULL,
	.read_bytes = NULL,
};

int t_iic_slave_basic(int argc, char **argv)
{
	struct iic_slave *device;
	u8 test_buffer[100] = {1, 2, 3, 4, 5, 6, 7, 8};
	u8 write_test_data[] = {0x55, 0xAA, 0x12, 0x34, 0x78, 0x9C};
	u8 read_test_data[8] = {1, 2, 3, 4, 5, 6, 7, 8};
	int ret;
	int i;

	pr_info("=== i2c slave basic test ===\n");

	iic_master_soft_init(&fake_master);

	device = iic_slave_soft_init("iic_test", 0x48, test_buffer, ARRAY_SIZE(test_buffer));
	if (IS_ERR(device)) {
		pr_err("i2c slave init failed\n");
		return FAIL;
	}

	ret = iic_master_port_multi_transmit(&fake_master, 0x48, 0, write_test_data, 3);
	if (ret) {
		pr_err("master write operation failed: %d\n", ret);
		iic_slave_soft_exit(device);
		return FAIL;
	}

	ret = iic_master_port_multi_receive(&fake_master, 0x48, 0, read_test_data, 3);
	if (ret) {
		pr_err("master read operation failed: %d\n", ret);
		iic_slave_soft_exit(device);
		return FAIL;
	}

	for (i = 0; i < 3; i++) {
		if (write_test_data[i] != read_test_data[i]) {
			pr_err("mismatch at buffer[%d]: expected %02X, got %02X\n",
				i, write_test_data[i], read_test_data[i]);
			iic_slave_soft_exit(device);
			return FAIL;
		}
	}

	pr_info("i2c slave basic test passed\n");

	iic_slave_soft_exit(device);

	return PASS;
}

int t_iic_transfer_byte(int argc, char **argv)
{
	u8 test_values[] = {0x00, 0x01, 0x7F, 0x80, 0xFF, 0xAA, 0x55};
	u8 received;
	int ret, i;

	pr_info("=== i2c transfer byte test ===\n");

	iic_master_soft_init(&fake_master);

	for (i = 0; i < ARRAY_SIZE(test_values); i++) {
		ret = iic_master_port_transmit(&fake_master, 0x48, 0, test_values[i]);
		if (ret) {
			pr_err("transmit failed for 0x%02X: %d\n", test_values[i], ret);
			return FAIL;
		}

		pr_info("sent: 0x%02X\n", test_values[i]);
	}

	pr_info("i2c transfer byte test completed\n");

	return PASS;
}

int t_iic_transfer_bytes(int argc, char **argv)
{
	u8 write_buffer[256];
	u8 read_buffer[256];
	int ret, i;
	int test_sizes[] = {1, 2, 4, 8, 16, 32, 64, 128, 255};

	pr_info("=== i2c transfer bytes test ===\n");

	iic_master_soft_init(&fake_master);

	/* Test different buffer sizes */
	for (i = 0; i < ARRAY_SIZE(test_sizes); i++) {
		int size = test_sizes[i];
		int j;

		/* Prepare test data */
		for (j = 0; j < size; j++) {
			write_buffer[j] = (u8)(j & 0xFF);
		}

		/* Transmit data */
		ret = iic_master_port_multi_transmit(&fake_master, 0x48, 0, write_buffer, size);
		if (ret) {
			pr_err("transmit failed for size %d: %d\n", size, ret);
			return FAIL;
		}

		/* Receive data */
		ret = iic_master_port_multi_receive(&fake_master, 0x48, 0, read_buffer, size);
		if (ret) {
			pr_err("receive failed for size %d: %d\n", size, ret);
			return FAIL;
		}

		pr_info("transferred %d bytes successfully\n", size);
	}

	pr_info("i2c transfer bytes test passed\n");

	return PASS;
}

int t_iic_edge_cases(int argc, char **argv)
{
	u8 write_buffer[4];
	u8 read_buffer[4];
	int ret;

	pr_info("=== i2c edge cases test ===\n");

	iic_master_soft_init(&fake_master);

	/* Test 1: Single byte */
	write_buffer[0] = 0xAA;
	ret = iic_master_port_multi_transmit(&fake_master, 0x48, 0, write_buffer, 1);
	if (ret) {
		pr_err("single byte transmit failed: %d\n", ret);
		return FAIL;
	}
	pr_info("single byte test passed\n");

	/* Test 2: Zero length (should succeed) */
	ret = iic_master_port_multi_transmit(&fake_master, 0x48, 0, write_buffer, 0);
	if (ret != 0) {
		pr_err("zero length should succeed\n");
		return FAIL;
	}
	pr_info("zero length test passed\n");

	/* Test 3: NULL buffer (should fail) */
	ret = iic_master_port_multi_transmit(&fake_master, 0x48, 0, NULL, 4);
	if (ret != -EINVAL) {
		pr_err("null buffer should fail\n");
		return FAIL;
	}
	pr_info("null buffer test passed\n");

	/* Test 4: NULL master (should fail) */
	ret = iic_master_port_multi_transmit(NULL, 0x48, 0, write_buffer, 4);
	if (ret != -EINVAL) {
		pr_err("null master should fail\n");
		return FAIL;
	}
	pr_info("null master test passed\n");

	/* Test 5: Boundary values */
	write_buffer[0] = 0x00;
	write_buffer[1] = 0xFF;
	write_buffer[2] = 0x55;
	write_buffer[3] = 0xAA;
	ret = iic_master_port_multi_transmit(&fake_master, 0x48, 0, write_buffer, 4);
	if (ret) {
		pr_err("boundary values test failed: %d\n", ret);
		return FAIL;
	}
	pr_info("boundary values test passed\n");

	pr_info("i2c edge cases test passed\n");

	return PASS;
}

int t_iic_performance(int argc, char **argv)
{
	u8 write_buffer[256];
	u8 read_buffer[256];
	int ret, i;
	int iterations = 50;
	int success_count = 0;

	pr_info("=== i2c performance test ===\n");

	iic_master_soft_init(&fake_master);

	/* Prepare test data */
	for (i = 0; i < ARRAY_SIZE(write_buffer); i++) {
		write_buffer[i] = (u8)(i & 0xFF);
	}

	/* Perform multiple iterations */
	for (i = 0; i < iterations; i++) {
		ret = iic_master_port_multi_transmit(&fake_master, 0x48, 0, write_buffer, 32);
		if (ret == 0) {
			success_count++;
		}
	}

	pr_info("performance test: %d/%d transfers succeeded\n", success_count, iterations);

	if (success_count == iterations) {
		pr_info("i2c performance test passed\n");
		return PASS;
	} else {
		pr_err("i2c performance test failed: only %d/%d succeeded\n", success_count, iterations);
		return FAIL;
	}
}

int t_iic_stress(int argc, char **argv)
{
	u8 write_buffer[128];
	u8 read_buffer[128];
	int ret, i, j;

	pr_info("=== i2c stress test ===\n");

	iic_master_soft_init(&fake_master);

	/* Test rapid data transfer */
	for (i = 0; i < 30; i++) {
		int size = 1 + (i % 127);  /* Vary size from 1 to 127 */

		for (j = 0; j < size; j++) {
			write_buffer[j] = (u8)((i + j) & 0xFF);
		}

		ret = iic_master_port_multi_transmit(&fake_master, 0x48, 0, write_buffer, size);
		if (ret) {
			pr_err("stress test iteration %d failed: %d\n", i, ret);
			return FAIL;
		}

		/* Small delay to allow slave to process */
		mdelay(1);
	}

	pr_info("i2c stress test passed\n");

	return PASS;
}

int t_iic_read_write_reg(int argc, char **argv)
{
	u8 test_data[] = {0x12, 0x34, 0x56, 0x78};
	u8 read_data[4];
	u8 reg_addr;
	int ret, i;

	pr_info("=== i2c read write reg test ===\n");

	iic_master_soft_init(&fake_master);

	/* Test write to register */
	for (i = 0; i < ARRAY_SIZE(test_data); i++) {
		reg_addr = i;
		ret = iic_master_port_transmit(&fake_master, 0x48, reg_addr, test_data[i]);
		if (ret) {
			pr_err("register 0x%02X write failed: %d\n", reg_addr, ret);
			return FAIL;
		}
	}
	pr_info("register write succeeded\n");

	/* Test read from register */
	for (i = 0; i < ARRAY_SIZE(test_data); i++) {
		reg_addr = i;
		ret = iic_master_port_receive(&fake_master, 0x48, reg_addr, &read_data[i]);
		if (ret) {
			pr_err("register 0x%02X read failed: %d\n", reg_addr, ret);
			return FAIL;
		}
	}
	pr_info("register read succeeded\n");

	/* Test with different register addresses */
	for (reg_addr = 0x00; reg_addr < 0x10; reg_addr++) {
		ret = iic_master_port_transmit(&fake_master, 0x48, reg_addr, 0x55);
		if (ret) {
			pr_err("register 0x%02X write failed: %d\n", reg_addr, ret);
			return FAIL;
		}
	}

	pr_info("i2c read write reg test passed\n");

	return PASS;
}

int t_iic_boundary_large(int argc, char **argv)
{
	u8 large_buffer[200];
	u8 read_buffer[200];
	int ret, i;
	int large_sizes[] = {100, 150, 190, 199};

	pr_info("=== i2c boundary large test ===\n");

	iic_master_soft_init(&fake_master);

	/* Prepare test data */
	for (i = 0; i < ARRAY_SIZE(large_buffer); i++) {
		large_buffer[i] = (u8)(i & 0xFF);
	}

	/* Test various large buffer sizes */
	for (i = 0; i < ARRAY_SIZE(large_sizes); i++) {
		int size = large_sizes[i];

		ret = iic_master_port_multi_transmit(&fake_master, 0x48, 0, large_buffer, size);
		if (ret) {
			pr_err("large buffer %d bytes failed: %d\n", size, ret);
			return FAIL;
		}

		ret = iic_master_port_multi_receive(&fake_master, 0x48, 0, read_buffer, size);
		if (ret) {
			pr_err("large buffer %d bytes receive failed: %d\n", size, ret);
			return FAIL;
		}

		pr_info("large buffer %d bytes test passed\n", size);
	}

	pr_info("i2c boundary large test passed\n");

	return PASS;
}

int t_iic_address_modes(int argc, char **argv)
{
	u8 write_buffer[10];
	u8 read_buffer[10];
	int ret, i;

	pr_info("=== i2c address modes test ===\n");

	iic_master_soft_init(&fake_master);

	/* Test 8-bit addressing mode */
	for (i = 0; i < ARRAY_SIZE(write_buffer); i++) {
		write_buffer[i] = (u8)(i & 0xFF);
	}

	ret = iic_master_port_multi_transmit(&fake_master, 0x48, 0, write_buffer, 5);
	if (ret) {
		pr_err("8-bit addressing failed: %d\n", ret);
		return FAIL;
	}
	pr_info("8-bit addressing test passed\n");

	/* Test with different addresses */
	for (i = 0; i < 5; i++) {
		ret = iic_master_port_transmit(&fake_master, 0x48 + i, 0, write_buffer[i]);
		if (ret) {
			pr_err("address 0x%02X failed: %d\n", 0x48 + i, ret);
			return FAIL;
		}
	}

	pr_info("i2c address modes test passed\n");

	return PASS;
}

int t_iic_start_stop(int argc, char **argv)
{
	u8 write_buffer[5];
	int ret, i;

	pr_info("=== i2c start stop test ===\n");

	iic_master_soft_init(&fake_master);

	/* Test repeated start/stop conditions */
	for (i = 0; i < 10; i++) {
		write_buffer[0] = (u8)(i & 0xFF);

		ret = iic_master_port_multi_transmit(&fake_master, 0x48, 0, write_buffer, 1);
		if (ret) {
			pr_err("iteration %d failed: %d\n", i, ret);
			return FAIL;
		}

		/* Small delay between transactions */
		mdelay(1);
	}

	pr_info("i2c start stop test passed\n");

	return PASS;
}

#endif /* DESIGN_VERIFICATION_IIC */
