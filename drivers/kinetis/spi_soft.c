// #undef DEBUG
// #undef VERBOSE_DEBUG

#include <linux/printk.h>
#include <linux/delay.h>
#include <linux/random.h>
#include <linux/string.h>
#include <linux/errno.h>
#include <linux/sched.h>
#include <linux/slab.h>

#include "kinetis/spi_soft.h"
#include "kinetis/idebug.h"
#include "kinetis/design_verification.h"

#include <unistd.h>

/* The following program is modified by the user according to the hardware device, otherwise the driver cannot run. */

/*
 * Note: This file now uses pthread instead of kernel threads.
 * When compiling, make sure to link with the pthread library:
 * gcc -lpthread ... (or add -lpthread to your Makefile LDFLAGS)
 */

/**
  * @step 1:  Modify the corresponding function according to the modified area and the corresponding function name.
  * @step 2:  Modify the read-write function of SPI corresponding pin in the header file.
  * @step 3:  Configure SPI mode (CPOL/CPHA) and bit order.
  */

#if MCU_PLATFORM_STM32
#include "stm32f4xx_hal.h"
#else
#endif

/* The above procedure is modified by the user according to the hardware device, otherwise the driver cannot run. */

void spi_master_soft_delay(u32 ticks)
{
	// pr_debug("spi_master: delay %d us", ticks);
	mdelay(ticks);
}

int spi_master_soft_init(struct spi_master *master, u8 cpol, u8 cpha, u8 bit_order, u8 speed)
{
	if (!master || !master->init) {
		return -EINVAL;
	}

	master->cpol = cpol;
	master->cpha = cpha;
	master->bit_order = bit_order;
	master->speed = speed;

	master->init();

	master->cs_high();
	if (master->cpol == 0)
		master->sck_low();
	else
		master->sck_high();

	pr_info("spi master initialized with cpol=%d, cpha=%d, speed=%d", cpol, cpha, speed);
	return 0;
}

void spi_soft_set_mode(struct spi_master *master, struct spi_slave *device, u8 cpol, u8 cpha)
{
	master->cpol = cpol;
	master->cpha = cpha;
	device->cpol = cpol;
	device->cpha = cpha;

	if (master->cpol == 0)
		master->sck_low();
	else
		master->sck_high();
}

void spi_soft_set_bit_order(struct spi_master *master, struct spi_slave *device, u8 bit_order)
{
	master->bit_order = bit_order;
	device->bit_order = bit_order;
}

void spi_soft_set_speed(struct spi_master *master, u8 speed)
{
	master->speed = speed;
}

int spi_master_soft_send_byte(struct spi_master *master, u8 data)
{
	int i;
	u8 bit;

	for (i = 7; i >= 0; i--) {
		if (master->bit_order == SPI_BIT_ORDER_MSB) {
			bit = (data >> i) & 0x01;
		} else {
			bit = (data >> (7 - i)) & 0x01;
		}

		if (bit) {
			master->mosi_high();
		} else {
			master->mosi_low();
		}

		if (!master->cpha && !master->cpol) {
			master->sck_low();
			spi_master_soft_delay(master->speed);
			master->sck_high();
			spi_master_soft_delay(master->speed);
		} else if (!master->cpha && master->cpol) {
			master->sck_high();
			spi_master_soft_delay(master->speed);
			master->sck_low();
			spi_master_soft_delay(master->speed);
		} else if (master->cpha && !master->cpol) {
			master->sck_high();
			spi_master_soft_delay(master->speed);
			master->sck_low();
			spi_master_soft_delay(master->speed);
		} else if (master->cpha && master->cpol) {
			master->sck_low();
			spi_master_soft_delay(master->speed);
			master->sck_high();
			spi_master_soft_delay(master->speed);
		}
	}

	return 0;
}

u8 spi_master_soft_read_byte(struct spi_master *master)
{
	int i;
	u8 data = 0;
	u8 bit;

	for (i = 7; i >= 0; i--) {
		if (!master->cpha && !master->cpol) {
			master->sck_high();
			bit = master->miso_read() ? 1 : 0;
			spi_master_soft_delay(master->speed);
			master->sck_low();
			spi_master_soft_delay(master->speed);
		} else if (!master->cpha && master->cpol) {
			master->sck_low();
			bit = master->miso_read() ? 1 : 0;
			spi_master_soft_delay(master->speed);
			master->sck_high();
			spi_master_soft_delay(master->speed);
		} else if (master->cpha && !master->cpol) {
			master->sck_high();
			spi_master_soft_delay(master->speed);
			master->sck_low();
			bit = master->miso_read() ? 1 : 0;
			spi_master_soft_delay(master->speed);
		} else if (master->cpha && master->cpol) {
			master->sck_low();
			spi_master_soft_delay(master->speed);
			master->sck_high();
			bit = master->miso_read() ? 1 : 0;
			spi_master_soft_delay(master->speed);
		}

		if (master->bit_order == SPI_BIT_ORDER_MSB) {
			data |= (bit << i);
		} else {
			data |= (bit << (7 - i));
		}
	}

	return data;
}

static int spi_master_soft_write_bytes_with_reg(struct spi_master *master, u8 reg, u8 *pdata, u8 length)
{
	u8 i;

	if (!master || !pdata || length == 0) {
		return -EINVAL;
	}

	master->cs_low();

	spi_master_soft_send_byte(master, 0x7F & reg);

	for (i = 0; i < length; i++) {
		spi_master_soft_send_byte(master, pdata[i]);
	}

	master->cs_high();

	return 0;
}

static int spi_master_soft_read_bytes_with_reg(struct spi_master *master, u8 reg, u8 *pdata, u8 length)
{
	u8 i;

	if (!master || !pdata || length == 0) {
		return -EINVAL;
	}

	master->cs_low();

	/* Send register address first */
	spi_master_soft_send_byte(master, 0x80 | reg);

	/* Read data bytes */
	for (i = 0; i < length; i++) {
		pdata[i] = spi_master_soft_read_byte(master);
	}

	master->cs_high();

	return 0;
}

int spi_master_port_transmit(struct spi_master *master, u8 reg, u8 *pdata, u8 length)
{
	if (!master || !pdata) {
		return -EINVAL;
	}

	if (master->write_bytes) {
		return master->write_bytes(pdata, length);
	} else {
		return spi_master_soft_write_bytes_with_reg(master, reg, pdata, length);
	}
}

int spi_master_port_receive(struct spi_master *master, u8 reg, u8 *pdata, u8 length)
{
	if (!master || !pdata) {
		return -EINVAL;
	}

	if (master->read_bytes) {
		return master->read_bytes(pdata, length);
	} else {
		return spi_master_soft_read_bytes_with_reg(master, reg, pdata, length);
	}
}

#ifdef KINETIS_FAKE_SIM
/* SPI Slave state machine */
enum {
	SPI_SLAVE_STATE_IDLE = 0,
	SPI_SLAVE_STATE_MATCH_ADDRESS,
	SPI_SLAVE_STATE_RECEIVING_DATA,
	SPI_SLAVE_STATE_TRANSMITTING_DATA
};

/* Bus simulation variables */
static struct {
	bool cs_line;
	bool mosi_line;
	bool miso_line;
	bool sck_line;
} spi_bus_simulation = {
	.cs_line = 1,  /* CS high (deselected) by default */
	.mosi_line = 0,
	.miso_line = 0,
	.sck_line = 0
};

static const char *spi_slave_get_state_name(u8 state)
{
	switch (state) {
	case SPI_SLAVE_STATE_IDLE:
		return "IDLE";
	case SPI_SLAVE_STATE_MATCH_ADDRESS:
		return "MATCH_ADDRESS";
	case SPI_SLAVE_STATE_RECEIVING_DATA:
		return "RECEIVING_DATA";
	case SPI_SLAVE_STATE_TRANSMITTING_DATA:
		return "TRANSMITTING_DATA";
	default:
		return "UNKNOWN";
	}
}

static void spi_slave_read_bus_lines(bool *cs, bool *sck, bool *mosi, bool *miso)
{
	*cs = spi_bus_simulation.cs_line;
	*sck = spi_bus_simulation.sck_line;
	*mosi = spi_bus_simulation.mosi_line;
	*miso = spi_bus_simulation.miso_line;
}

static void spi_slave_set_miso(bool state)
{
	spi_bus_simulation.miso_line = state;;
}

static void spi_slave_set_mosi(bool state)
{
	spi_bus_simulation.mosi_line = state;
}

static void spi_slave_handle_byte_received(struct spi_slave *device, u8 byte)
{
	if (device->index < device->buffer_size) {
		device->buffer[device->index] = byte;
		pr_debug("spi_slave(%s): stored byte 0x%02X at buffer index %d",
			device->name, byte, device->index);
		device->index++;
	}
}

static u8 spi_slave_get_next_tx_byte(struct spi_slave *device)
{
	if (device->index < device->buffer_size) {
		device->index++;
		pr_debug("spi_slave(%s): read byte 0x%02X from buffer index %d",
			device->name, device->buffer[device->index - 1], device->index - 1);
		return device->buffer[device->index - 1];
	}

	return 0xFF; /* Default data if no more data */
}

static void spi_slave_prepare_bit(struct spi_slave *device)
{
	bool bit;

	device->bit_count++;

	/* Prepare next bit */
	if (device->bit_count <= 8) {
		/* Setting up bit (bit_count-1) because we just incremented */
		if (device->bit_order == SPI_BIT_ORDER_LSB) {
			bit = (device->current_byte >> (device->bit_count - 1)) & 0x01;
		} else {
			bit = (device->current_byte >> (8 - device->bit_count)) & 0x01;
		}
		spi_slave_set_miso(bit);
		pr_debug("spi_slave(%s): Transmitted bit%d(%d)",
			device->name, 8 - device->bit_count, bit);
	} else {
		/* Byte transmitted, prepare next byte */
		device->byte_count++;
		device->current_byte = spi_slave_get_next_tx_byte(device);
		device->bit_count = 1;  /* Reset to 1 for next byte's first bit */
		if (device->bit_order == SPI_BIT_ORDER_LSB) {
			bit = (device->current_byte >> 0) & 0x01;
		} else {
			bit = (device->current_byte >> 7) & 0x01;
		}
		spi_slave_set_miso(bit);
		pr_debug("spi_slave(%s): Transmitted bit7(%d)", device->name, bit);
	}
}

static void spi_slave_receive_bit(struct spi_slave *device, bool mosi_current)
{
	bool bit;

	device->bit_count++;

	if (device->bit_count < 9) {
		if (mosi_current) {
			if (device->bit_order == SPI_BIT_ORDER_LSB) {
				device->current_byte |= 1 << (device->bit_count - 1);
			} else {
				device->current_byte |= 1 << (8 - device->bit_count);
			}
		}

		pr_debug("spi_slave(%s): Received bit%d(%d)",
			device->name, 8 - device->bit_count, mosi_current);
	}

	if (device->bit_count == 8) {
		if (device->current_state == SPI_SLAVE_STATE_MATCH_ADDRESS) {
			device->is_read_operation = device->current_byte & 0x80;
			/* Clear the MSB which indicates read/write */
			device->current_byte &= 0x7F;
			if (device->current_byte < device->buffer_size) {
				device->index = device->current_byte;
			}
			pr_debug("spi_slave(%s): Address 0x%02X, %s operation",
				device->name, device->current_byte, device->is_read_operation ? "READ" : "WRITE");
			/* Transition to next state based on operation type */
			if (device->is_read_operation) {
				device->current_state = SPI_SLAVE_STATE_TRANSMITTING_DATA;
				device->current_byte = spi_slave_get_next_tx_byte(device);
				device->byte_count = 0;

				/* Prepare first bit (bit7) immediately for transmission */
				/* In CPHA=0: bit7 should be ready before first rising edge */
				/* In CPHA=1: bit7 should be ready on first rising edge */
				if (!device->cpha) {
					/* CPHA=0: Setup immediately */
					device->bit_count = 1;
					if (device->bit_order == SPI_BIT_ORDER_LSB) {
						bit = (device->current_byte >> 0) & 0x01;
					} else {
						bit = (device->current_byte >> 7) & 0x01;
					}
					spi_slave_set_miso(bit);
					pr_debug("spi_slave(%s): Transmitted bit7(%d)", device->name, bit);
				} else {
					/* CPHA=1: Wait for first clock edge */
					device->bit_count = 0;
				}
			} else {
				device->current_state = SPI_SLAVE_STATE_RECEIVING_DATA;
				device->current_byte = 0;
				device->bit_count = 0;
			}
		} else if (device->current_state == SPI_SLAVE_STATE_RECEIVING_DATA) {
			/* Byte received */
			device->byte_count++;
			spi_slave_handle_byte_received(device, device->current_byte);
			pr_debug("spi_slave(%s), byte count: %d",
				device->name, device->byte_count);
			/* Reset for next byte */
			device->bit_count = 0;
			device->current_byte = 0;
		}
	}
}

static void *spi_slave_state_machine_thread(void *data)
{
	struct spi_slave *device = (struct spi_slave *)data;
	bool cs_current, sck_current, miso_current, mosi_current;
	bool cs_last = 1, sck_last = 0, miso_last = 0, mosi_last = 0;

	/* Set thread to cancelable */
	pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
	pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, NULL);

	while (device->thread_running) {
		/* Read current bus line states */
		cs_current = spi_bus_simulation.cs_line;
		sck_current = spi_bus_simulation.sck_line;
		miso_current = spi_bus_simulation.miso_line;
		mosi_current = spi_bus_simulation.mosi_line;

		/* State machine - switch case implementation */
		switch (device->current_state) {
		case SPI_SLAVE_STATE_IDLE:
			/* Wait for CS to go low (slave selected) */
			if (cs_last && !cs_current) {
				device->current_state = SPI_SLAVE_STATE_MATCH_ADDRESS;
			}
			break;

		case SPI_SLAVE_STATE_MATCH_ADDRESS:
		case SPI_SLAVE_STATE_RECEIVING_DATA:
			if (cs_current) {
				device->current_state = SPI_SLAVE_STATE_IDLE;
				device->bit_count = 0;
				device->byte_count = 0;
				device->current_byte = 0;
				spi_slave_set_mosi(0);
				break;
			}

			if (!device->cpha && !device->cpol) {
				/* CPHA=0, CPOL=0: Sample on rising edge */
				if (!sck_last && sck_current) {
					spi_slave_receive_bit(device, mosi_current);
				}
			} else if (device->cpha && !device->cpol) {
				/* CPHA=1, CPOL=0: Sample on falling edge */
				if (sck_last && !sck_current) {
					spi_slave_receive_bit(device, mosi_current);
				}
			} else if (!device->cpha && device->cpol) {
				/* CPHA=0, CPOL=1: Sample on falling edge */
				if (sck_last && !sck_current) {
					spi_slave_receive_bit(device, mosi_current);
				}
			} else if (device->cpha && device->cpol) {
				/* CPHA=1, CPOL=1: Sample on rising edge */
				if (!sck_last && sck_current) {
					spi_slave_receive_bit(device, mosi_current);
				}
			}

			break;

		case SPI_SLAVE_STATE_TRANSMITTING_DATA:
			if (cs_current) {
				device->current_state = SPI_SLAVE_STATE_IDLE;
				device->bit_count = 0;
				device->byte_count = 0;
				device->current_byte = 0;

				/* Release MISO line */
				spi_slave_set_miso(0);
				break;
			}

			if (!device->cpha && !device->cpol) {
				/* CPHA=0, CPOL=0: Sample on rising edge, setup on falling edge */
				if (sck_last && !sck_current) {
					/* Falling edge: prepare next bit */
					spi_slave_prepare_bit(device);
				}
			} else if (device->cpha && !device->cpol) {
				/* CPHA=1, CPOL=0: Sample on falling edge, setup on rising edge */
				if (!sck_last && sck_current) {
					/* Rising edge: prepare next bit */
					spi_slave_prepare_bit(device);
				}
			} else if (!device->cpha && device->cpol) {
				/* CPHA=0, CPOL=1: Sample on falling edge, setup on rising edge */
				if (!sck_last && sck_current) {
					/* Rising edge: prepare next bit */
					spi_slave_prepare_bit(device);
				}
			} else if (device->cpha && device->cpol) {
				/* CPHA=1, CPOL=1: Sample on rising edge, setup on falling edge */
				if (sck_last && !sck_current) {
					/* Falling edge: prepare next bit */
					spi_slave_prepare_bit(device);
				}
			}

			break;

		default:
			/* Unknown state, reset to idle */
			device->current_state = SPI_SLAVE_STATE_IDLE;
			pr_warn("spi_slave(%s): Unknown state, resetting to IDLE", device->name);
			break;
		}  /* End of switch */

		/* Enable debug logging for state transitions */
		if (device->current_state != device->last_state) {
			pr_debug("spi_slave(%s): slave state: %s", device->name, spi_slave_get_state_name(device->current_state));
		}

		/* Update last states */
		cs_last = cs_current;
		sck_last = sck_current;
		miso_last = miso_current;
		mosi_last = mosi_current;
		device->last_state = device->current_state;

		/* Small delay to prevent excessive CPU usage */
		// usleep(1);
	}

	pr_info("spi_slave(%s): State machine thread stopped", device->name);
	return NULL;
}

/**
 * @brief Start SPI slave state machine thread
 */
static int spi_slave_start_thread(struct spi_slave *device)
{
	int ret;

	if (device->thread_running) {
		pr_warn("spi_slave(%s): Thread already running", device->name);
		return 0;
	}

	device->thread_running = 1;

	ret = pthread_create(&device->mosi_thread, NULL, spi_slave_state_machine_thread, device);
	if (ret != 0) {
		device->thread_running = 0;
		pr_err("spi_slave(%s): Failed to create mosi thread: %d", device->name, ret);
		return -ret;
	}

	pr_info("spi_slave(%s): Thread started successfully", device->name);
	return 0;
}

/**
 * @brief Stop SPI slave state machine thread
 */
static void spi_slave_stop_thread(struct spi_slave *device)
{
	void *thread_ret;

	if (!device->thread_running) {
		return;
	}

	/* Signal thread to stop */
	device->thread_running = 0;

	/* Wait for mosi thread to finish */
	if (pthread_join(device->mosi_thread, &thread_ret) == 0) {
		pr_info("spi_slave(%s): MOSI thread stopped successfully", device->name);
	} else {
		pr_warn("spi_slave(%s): MOSI thread stop had issues, but continuing", device->name);
	}

	pr_info("spi_slave(%s): Thread stopped", device->name);
}

/**
 * @brief Stop SPI slave state machine thread
 */
void spi_slave_soft_exit(struct spi_slave *device)
{
	pr_info("spi_slave(%s): Shutting down SPI slave", device->name);

	/* Stop state machine thread */
	spi_slave_stop_thread(device);

	kfree(device->buffer);
	kfree(device);
}

/**
 * @brief Initialize SPI slave
 * @param name Device name for logging
 * @param buffer Pointer to internal buffer for data storage
 * @param buffer_size Size of internal buffer
 */
struct spi_slave *spi_slave_soft_init(char *name, u8 cpol, u8 cpha, u8 bit_order,
	u8 *buffer, u32 buffer_size)
{
	struct spi_slave *device;
	int ret;

	device = kzalloc(sizeof(struct spi_slave), GFP_KERNEL);
	if (!device) {
		return ERR_PTR(-ENOMEM);
	}

	// 	device->name = kstrdup(name, GFP_KERNEL);
	device->name = name;
	device->buffer_size = buffer_size;
	device->buffer = buffer;
	device->last_state = SPI_SLAVE_STATE_IDLE;
	device->current_state = SPI_SLAVE_STATE_IDLE;
	device->index = 0;
	device->cpol = cpol;
	device->cpha = cpha;
	device->bit_order = bit_order;

	pr_debug("spi_slave(%s): initialized", name);

	/* Start state machine thread */
	ret = spi_slave_start_thread(device);
	if (ret != 0) {
		kfree(device);
		return ERR_PTR(ret);
	}

	return device;
}
#endif

#ifdef DESIGN_VERIFICATION_SPI
#include "kinetis/test-kinetis.h"

#ifdef KINETIS_FAKE_SIM
int fake_spi_cs_low(void)
{
	pr_debug("spi_master: cs low");
	spi_bus_simulation.cs_line = 0;
	return 0;
}

int fake_spi_cs_high(void)
{
	pr_debug("spi_master: cs high");
	spi_bus_simulation.cs_line = 1;
	return 0;
}

int fake_spi_mosi_low(void)
{
	pr_debug("spi_master: mosi low");
	spi_bus_simulation.mosi_line = 0;
	return 0;
}

int fake_spi_mosi_high(void)
{
	pr_debug("spi_master: mosi high");
	spi_bus_simulation.mosi_line = 1;
	return 0;
}

int fake_spi_miso_read(void)
{
	/* Read from shared bus simulation */
	pr_debug("spi_master: miso %s", spi_bus_simulation.miso_line ? "high" : "low");
	return spi_bus_simulation.miso_line;
}

int fake_spi_sck_low(void)
{
	pr_debug("spi_master: sck low");
	spi_bus_simulation.sck_line = 0;
	return 0;
}

int fake_spi_sck_high(void)
{
	pr_debug("spi_master: sck high");
	spi_bus_simulation.sck_line = 1;
	return 0;
}

int fake_spi_init(void)
{
	return 0;
}

#else

static int spi_cs_low(void)
{
#if MCU_PLATFORM_STM32
	HAL_GPIO_WritePin(SPI_Soft_Port_CS_1, SPI_Soft_Pin_CS_1, GPIO_PIN_RESET);
#else
#endif
	return 0;
}

static int spi_cs_high(void)
{
#if MCU_PLATFORM_STM32
	HAL_GPIO_WritePin(SPI_Soft_Port_CS_1, SPI_Soft_Pin_CS_1, GPIO_PIN_SET);
#else
#endif
	return 0;
}

static int spi_mosi_low(void)
{
#if MCU_PLATFORM_STM32
	HAL_GPIO_WritePin(SPI_Soft_Port_MOSI_1, SPI_Soft_Pin_MOSI_1, GPIO_PIN_RESET);
#else
#endif
	return 0;
}

static int spi_mosi_high(void)
{
#if MCU_PLATFORM_STM32
	HAL_GPIO_WritePin(SPI_Soft_Port_MOSI_1, SPI_Soft_Pin_MOSI_1, GPIO_PIN_SET);
#else
#endif
	return 0;
}

static int spi_miso_read(void)
{
#if MCU_PLATFORM_STM32
	return HAL_GPIO_ReadPin(SPI_Soft_Port_MISO_1, SPI_Soft_Pin_MISO_1);
#else
	return 0;
#endif
}

static int spi_sck_low(void)
{
#if MCU_PLATFORM_STM32
	HAL_GPIO_WritePin(SPI_Soft_Port_SCK_1, SPI_Soft_Pin_SCK_1, GPIO_PIN_RESET);
#else
#endif
	return 0;
}

static int spi_sck_high(void)
{
#if MCU_PLATFORM_STM32
	HAL_GPIO_WritePin(SPI_Soft_Port_SCK_1, SPI_Soft_Pin_SCK_1, GPIO_PIN_SET);
#else
#endif
	return 0;
}
#endif

/* Master instances for both simulation and hardware */
#ifdef KINETIS_FAKE_SIM
struct spi_master fake_spi_master = {
	.cs_low = fake_spi_cs_low,
	.cs_high = fake_spi_cs_high,
	.mosi_low = fake_spi_mosi_low,
	.mosi_high = fake_spi_mosi_high,
	.miso_read = fake_spi_miso_read,
	.sck_low = fake_spi_sck_low,
	.sck_high = fake_spi_sck_high,
	.write_bytes = NULL,
	.read_bytes = NULL,
	.init = fake_spi_init,
};

struct spi_slave *fake_spi_device;
#else
struct spi_master spi_master_1 = {
	.cs_low = spi_cs_low,
	.cs_high = spi_cs_high,
	.mosi_low = spi_mosi_low,
	.mosi_high = spi_mosi_high,
	.miso_read = spi_miso_read,
	.sck_low = spi_sck_low,
	.sck_high = spi_sck_high,
	.write_bytes = NULL,
	.read_bytes = NULL,
	.init = spi_master_soft_init,
};
#endif

int t_spi_system_init(int argc, char **argv)
{
	u8 cpol = get_random_int() % 2;
	u8 cpha = get_random_int() % 2;
	u8 bit_order = get_random_int() % 2;
	u8 speed = 2;
	u32 buffer_size = 256;
	u8 *buffer;

	if (argc > 1)
		cpol = simple_strtoul(argv[1], &argv[1], 10);

	if (argc > 2)
		cpha = simple_strtoul(argv[2], &argv[2], 10);

	if (argc > 3)
		bit_order = simple_strtoul(argv[3], &argv[3], 10);

	if (argc > 4)
		buffer_size = simple_strtoul(argv[4], &argv[4], 10);

	buffer = kmalloc(buffer_size, GFP_KERNEL);
	if (buffer == NULL) {
		return -ENOMEM;
	}

	fake_spi_device = spi_slave_soft_init("spi_test", cpol, cpha, bit_order, buffer, buffer_size);
	if (IS_ERR(fake_spi_device)) {
		pr_err("spi slave init failed\n");
		kfree(buffer);
		return FAIL;
	}

	spi_master_soft_init(&fake_spi_master, fake_spi_device->cpol, fake_spi_device->cpha, fake_spi_device->bit_order, speed);

	return 0;
}

int t_spi_system_exit(int argc, char **argv)
{
	spi_slave_soft_exit(fake_spi_device);

	return 0;
}

int t_spi_loopback(int argc, char **argv)
{
	u32 transfer_length = 2;
	u8 *write_data, *read_data;
	int i;
	int ret;

	if (argc > 1)
		transfer_length = simple_strtoul(argv[1], &argv[1], 10);

	write_data = kmalloc(transfer_length, GFP_KERNEL);
	if (write_data == NULL) {
		return -ENOMEM;
	}
	read_data = kzalloc(transfer_length, GFP_KERNEL);
	if (read_data == NULL) {
		kfree(write_data);
		return -ENOMEM;
	}
	get_random_bytes(write_data, transfer_length);

	ret = spi_master_port_transmit(&fake_spi_master, 0, write_data, transfer_length);
	if (ret) {
		pr_err("master write operation failed: %d\n", ret);
		goto err;
	}

	ret = spi_master_port_receive(&fake_spi_master, 0, read_data, transfer_length);
	if (ret) {
		pr_err("master read operation failed: %d\n", ret);
		goto err;
	}

	for (i = 0; i < transfer_length; i++) {
		if (write_data[i] != read_data[i]) {
			pr_err("mismatch at buffer[%d]: expected %02X, got %02X\n",
				i, write_data[i], read_data[i]);
			ret = -EIO;
			break;
		}
	}

	pr_info("spi loopback test %s\n", ret == 0 ? "passed" : "failed");

err:
	kfree(write_data);
	kfree(read_data);
	return ret;
}

int t_spi_set_mode(int argc, char **argv)
{
	u8 cpol = get_random_int() % 2;
	u8 cpha = get_random_int() % 2;
	u8 bit_order = get_random_int() % 2;
	u8 speed = 1;

	if (argc > 1)
		cpol = simple_strtoul(argv[1], &argv[1], 10);

	if (argc > 2)
		cpha = simple_strtoul(argv[2], &argv[2], 10);

	if (argc > 3)
		bit_order = simple_strtoul(argv[3], &argv[3], 10);

	if (argc > 4)
		speed = simple_strtoul(argv[4], &argv[4], 10);

	spi_soft_set_mode(&fake_spi_master, fake_spi_device, cpol, cpha);
	spi_soft_set_bit_order(&fake_spi_master, fake_spi_device, bit_order);
	spi_soft_set_speed(&fake_spi_master, speed);

	pr_info("spi mode set to cpol=%d, cpha=%d, bit_order=%d, speed=%d\n", cpol, cpha, bit_order, speed);

	return 0;
}

int t_spi_edge_cases(int argc, char **argv)
{
	u8 write_buffer[4];
	u8 read_buffer[4];
	int ret;

	pr_info("=== spi edge cases test ===\n");

	/* Test 1: Single byte */
	write_buffer[0] = 0xAA;
	ret = spi_master_port_transmit(&fake_spi_master, 0, write_buffer, 1);
	if (ret) {
		pr_err("single byte transmit failed: %d\n", ret);
		return FAIL;
	}
	pr_info("single byte test passed\n");

	/* Test 2: Zero length (should succeed) */
	ret = spi_master_port_transmit(&fake_spi_master, 0, write_buffer, 0);
	if (ret != 0) {
		pr_err("zero length should succeed\n");
		return FAIL;
	}
	pr_info("zero length test passed\n");

	/* Test 3: NULL buffer (should fail) */
	ret = spi_master_port_transmit(&fake_spi_master, 0, NULL, 4);
	if (ret != -EINVAL) {
		pr_err("null buffer should fail\n");
		return FAIL;
	}
	pr_info("null buffer test passed\n");

	/* Test 4: NULL master (should fail) */
	ret = spi_master_port_transmit(NULL, 0, write_buffer, 4);
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
	ret = spi_master_port_transmit(&fake_spi_master, 0, write_buffer, 4);
	if (ret) {
		pr_err("boundary values test failed: %d\n", ret);
		return FAIL;
	}
	pr_info("boundary values test passed\n");

	pr_info("spi edge cases test completed\n");

	return 0;
}

int t_spi_performance(int argc, char **argv)
{
	u8 write_buffer[256];
	u8 read_buffer[256];
	int ret, i, j;
	int iterations = 1000;
	int success_count = 0;
	u64 total_bytes_transferred = 0;
	u64 start_time, end_time;
	u64 elapsed_us;
	double throughput_kbps;

	pr_info("=== spi performance test ===\n");

	/* Parse command line arguments */
	if (argc > 1) {
		iterations = simple_strtoul(argv[1], &argv[1], 10);
		if (iterations <= 0 || iterations > 10000) {
			pr_err("Invalid iterations count (1-10000): %d\n", iterations);
			return -EINVAL;
		}
	}

	/* Prepare test data - filled with pattern for verification */
	for (i = 0; i < ARRAY_SIZE(write_buffer); i++) {
		write_buffer[i] = (u8)(i & 0xFF);
	}

	pr_info("Running %d iterations with %d-byte buffers...\n",
		iterations, (int)ARRAY_SIZE(write_buffer));

	/* Start performance measurement */
	start_time = basic_timer_get_us();

	/* Perform multiple iterations */
	for (i = 0; i < iterations; i++) {
		/* Test write performance */
		ret = spi_master_port_transmit(&fake_spi_master, 0, write_buffer,
						ARRAY_SIZE(write_buffer));
		if (ret == 0) {
			success_count++;
			total_bytes_transferred += ARRAY_SIZE(write_buffer);
		}

		/* Test read performance */
		ret = spi_master_port_receive(&fake_spi_master, 0, read_buffer,
					       ARRAY_SIZE(read_buffer));
		if (ret == 0) {
			success_count++;
			total_bytes_transferred += ARRAY_SIZE(read_buffer);
		}
	}

	/* End performance measurement */
	end_time = basic_timer_get_us();
	elapsed_us = end_time - start_time;

	/* Calculate throughput */
	if (elapsed_us > 0) {
		throughput_kbps = (double)(total_bytes_transferred * 8) / (double)elapsed_us;
	} else {
		throughput_kbps = 0;
	}

	/* Print performance results */
	pr_info("=== Performance Test Results ===\n");
	pr_info("Total operations:    %d\n", iterations * 2);
	pr_info("Successful ops:      %d\n", success_count);
	pr_info("Failed ops:          %d\n", (iterations * 2) - success_count);
	pr_info("Success rate:        %.2f%%\n",
		(double)success_count * 100.0 / (double)(iterations * 2));
	pr_info("Total bytes:         %llu bytes (%.2f KB)\n",
		total_bytes_transferred, (double)total_bytes_transferred / 1024.0);
	pr_info("Elapsed time:        %llu us (%.2f ms)\n",
		elapsed_us, (double)elapsed_us / 1000.0);
	pr_info("Throughput:          %.2f Kbps (%.2f KB/s)\n",
		throughput_kbps, throughput_kbps / 8.0);

	/* Verify data integrity for the last read */
	for (i = 0; i < ARRAY_SIZE(read_buffer); i++) {
		/* Verify data was written to slave buffer correctly */
		if (read_buffer[i] != write_buffer[i]) {
			pr_warn("Data mismatch at buffer[%d]: expected 0x%02X, got 0x%02X\n",
				i, write_buffer[i], read_buffer[i]);
			/* This is not a hard failure for performance test */
		}
	}

	/* Performance test passes if success rate >= 95% */
	if (success_count >= iterations * 2 * 95 / 100) {
		pr_info("spi performance test PASSED\n");
		return 0;
	} else {
		pr_err("spi performance test FAILED: success rate too low\n");
		return FAIL;
	}
}

int t_spi_stress(int argc, char **argv)
{
	u8 write_buffer[128];
	u8 read_buffer[128];
	int ret, i, j;

	pr_info("=== spi stress test ===\n");

// 	/* Test rapid mode switching */
// 	for (i = 0; i < 10; i++) {
// 		for (j = 0; j < 4; j++) {
// 			spi_set_mode(j);
// 		}
// 		spi_set_bit_order(SPI_BIT_ORDER_MSB);
// 		spi_set_bit_order(SPI_BIT_ORDER_LSB);
// 	}

	/* Test rapid data transfer */
	for (i = 0; i < 50; i++) {
		int size = 1 + (i % 127);  /* Vary size from 1 to 127 */

		for (j = 0; j < size; j++) {
			write_buffer[j] = (u8)((i + j) & 0xFF);
		}

		ret = spi_master_port_transmit(&fake_spi_master, 0, write_buffer, size);
		if (ret) {
			pr_err("stress test iteration %d failed: %d\n", i, ret);
			return FAIL;
		}
	}

// 	/* Restore defaults */
// 	spi_set_mode(SPI_MODE_CONFIG);
// 	spi_set_bit_order(SPI_BIT_ORDER_CONFIG);

	pr_info("spi stress test passed\n");

	return 0;
}

int t_spi_read_write_reg(int argc, char **argv)
{
	u8 test_data[] = {0x12, 0x34, 0x56, 0x78};
	u8 read_data[4];
	u8 reg_addr = 0x05;
	int ret;

	pr_info("=== spi read write reg test ===\n");

	/* Test write to register */
	ret = spi_master_port_transmit(&fake_spi_master, reg_addr, test_data, ARRAY_SIZE(test_data));
	if (ret) {
		pr_err("register write failed: %d\n", ret);
		return FAIL;
	}
	pr_info("register write succeeded\n");

	/* Test read from register */
	ret = spi_master_port_receive(&fake_spi_master, reg_addr, read_data, ARRAY_SIZE(read_data));
	if (ret) {
		pr_err("register read failed: %d\n", ret);
		return FAIL;
	}
	pr_info("register read succeeded\n");

	/* Test with different register addresses */
	for (reg_addr = 0x00; reg_addr < 0x10; reg_addr++) {
		ret = spi_master_port_transmit(&fake_spi_master, reg_addr, test_data, 2);
		if (ret) {
			pr_err("register 0x%02X write failed: %d\n", reg_addr, ret);
			return FAIL;
		}
	}

	pr_info("spi read write reg test passed\n");

	return 0;
}

int t_spi_boundary_large(int argc, char **argv)
{
	u8 large_buffer[512];
	u8 read_buffer[512];
	int ret, i;
	int large_sizes[] = {200, 300, 400, 500, 511};

	pr_info("=== spi boundary large test ===\n");

	/* Prepare test data */
	for (i = 0; i < ARRAY_SIZE(large_buffer); i++) {
		large_buffer[i] = (u8)(i & 0xFF);
	}

	/* Test various large buffer sizes */
	for (i = 0; i < ARRAY_SIZE(large_sizes); i++) {
		int size = large_sizes[i];

		ret = spi_master_port_transmit(&fake_spi_master, 0, large_buffer, size);
		if (ret) {
			pr_err("large buffer %d bytes failed: %d\n", size, ret);
			return FAIL;
		}

		ret = spi_master_port_receive(&fake_spi_master, 0, read_buffer, size);
		if (ret) {
			pr_err("large buffer %d bytes receive failed: %d\n", size, ret);
			return FAIL;
		}

		pr_info("large buffer %d bytes test passed\n", size);
	}

	pr_info("spi boundary large test passed\n");

	return 0;
}

#endif /* DESIGN_VERIFICATION_SPI */

