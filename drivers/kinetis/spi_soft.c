// #undef DEBUG
// #undef VERBOSE_DEBUG

#include <linux/printk.h>
#include <linux/delay.h>
#include <linux/random.h>
#include <linux/string.h>
#include <linux/errno.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <stdbool.h>

#include <unistd.h>

#include "kinetis/spi_soft.h"
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
  * @step 2:  Modify the read-write function of SPI corresponding pin in the header file.
  * @step 3:  Configure SPI mode (CPOL/CPHA) and bit order.
  */

#if MCU_PLATFORM_STM32
#include "stm32f4xx_hal.h"
#else
#endif

/* SPI Speed Mode Selection */
#define SPI_SPEED_SLOW         0  /* 100kHz Slow Mode */
#define SPI_SPEED_FAST         1  /* 1MHz Fast Mode */

/* Select SPI speed mode */
#define SPI_SPEED_MODE         SPI_SPEED_FAST

/* SPI timing parameters based on speed mode */
#if SPI_SPEED_MODE == SPI_SPEED_SLOW
/* Slow Mode (100kHz) - Clock period 10us */
#define SPI_DELAY_SETUP        2
#define SPI_DELAY_HOLD         2
#define SPI_DELAY_HALF_CYCLE   5
#else
/* Fast Mode (1MHz) - Clock period 1us */
#define SPI_DELAY_SETUP        0
#define SPI_DELAY_HOLD         0
#define SPI_DELAY_HALF_CYCLE   0
#endif

/* SPI Configuration */
#define SPI_MODE_CONFIG        SPI_MODE_0    /* Default to Mode 0 */
#define SPI_BIT_ORDER_CONFIG   SPI_BIT_ORDER_MSB  /* Default to MSB first */

/* Default SPI instance */
static u8 current_spi = SPI_SW_1;

/* Global SPI configuration */
static struct {
	u8 mode;          /* SPI mode (0-3) */
	u8 bit_order;     /* Bit order (MSB/LSB) */
	u8 speed;         /* Speed mode */
} spi_config = {
	.mode = SPI_MODE_CONFIG,
	.bit_order = SPI_BIT_ORDER_CONFIG,
	.speed = SPI_SPEED_MODE
};

#if MCU_PLATFORM_STM32
#define SPI_Soft_Pin_CS_1            GPIO_PIN_13
#define SPI_Soft_Pin_MOSI_1          GPIO_PIN_14
#define SPI_Soft_Pin_MISO_1          GPIO_PIN_15
#define SPI_Soft_Pin_SCK_1           GPIO_PIN_16
#define SPI_Soft_Port_CS_1           GPIOB
#define SPI_Soft_Port_MOSI_1         GPIOB
#define SPI_Soft_Port_MISO_1         GPIOB
#define SPI_Soft_Port_SCK_1          GPIOB

#define SPI_Soft_Pin_CS_2            GPIO_PIN_5
#define SPI_Soft_Pin_MOSI_2          GPIO_PIN_6
#define SPI_Soft_Pin_MISO_2          GPIO_PIN_7
#define SPI_Soft_Pin_SCK_2           GPIO_PIN_8
#define SPI_Soft_Port_CS_2           GPIOC
#define SPI_Soft_Port_MOSI_2         GPIOC
#define SPI_Soft_Port_MISO_2         GPIOC
#define SPI_Soft_Port_SCK_2          GPIOC

#define SPI_CLOCK_ENABLE_1           __HAL_RCC_GPIOB_CLK_ENABLE()
#define SPI_CLOCK_ENABLE_2           __HAL_RCC_GPIOC_CLK_ENABLE()
#else
#define SPI_Soft_Pin_CS_1
#define SPI_Soft_Pin_MOSI_1
#define SPI_Soft_Pin_MISO_1
#define SPI_Soft_Pin_SCK_1
#define SPI_Soft_Port_CS_1
#define SPI_Soft_Port_MOSI_1
#define SPI_Soft_Port_MISO_1
#define SPI_Soft_Port_SCK_1

#define SPI_Soft_Pin_CS_2
#define SPI_Soft_Pin_MOSI_2
#define SPI_Soft_Pin_MISO_2
#define SPI_Soft_Pin_SCK_2
#define SPI_Soft_Port_CS_2
#define SPI_Soft_Port_MOSI_2
#define SPI_Soft_Port_MISO_2
#define SPI_Soft_Port_SCK_2

#define SPI_CLOCK_ENABLE_1
#define SPI_CLOCK_ENABLE_2
#endif

/* The above procedure is modified by the user according to the hardware device, otherwise the driver cannot run. */

void spi_master_soft_delay(u32 ticks)
{
#ifdef KINETIS_FAKE_SIM
	// pr_debug("spi_master: delay %d us", ticks);
#endif
	udelay(ticks);
}

void spi_master_soft_cs_select(struct spi_master *master)
{
	if (master && master->cs_low) {
		master->cs_low();
	}
}

void spi_master_soft_cs_deselect(struct spi_master *master)
{
	if (master && master->cs_high) {
		master->cs_high();
	}
}

void spi_master_soft_init(void)
{
#if MCU_PLATFORM_STM32
	GPIO_InitTypeDef GPIO_InitStruct = {0};

	/* GPIO Ports Clock Enable */
	SPI_CLOCK_ENABLE_1;

	GPIO_InitStruct.Pin = SPI_Soft_Pin_CS_1;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Pull = GPIO_PULLUP;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
	HAL_GPIO_Init(SPI_Soft_Port_CS_1, &GPIO_InitStruct);

	GPIO_InitStruct.Pin = SPI_Soft_Pin_MOSI_1;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
	HAL_GPIO_Init(SPI_Soft_Port_MOSI_1, &GPIO_InitStruct);

	GPIO_InitStruct.Pin = SPI_Soft_Pin_MISO_1;
	GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
	HAL_GPIO_Init(SPI_Soft_Port_MISO_1, &GPIO_InitStruct);

	GPIO_InitStruct.Pin = SPI_Soft_Pin_SCK_1;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
	HAL_GPIO_Init(SPI_Soft_Port_SCK_1, &GPIO_InitStruct);

	HAL_GPIO_WritePin(SPI_Soft_Port_CS_1, SPI_Soft_Pin_CS_1, GPIO_PIN_SET);
	HAL_GPIO_WritePin(SPI_Soft_Port_SCK_1, SPI_Soft_Pin_SCK_1, GPIO_PIN_RESET);
	HAL_GPIO_WritePin(SPI_Soft_Port_MOSI_1, SPI_Soft_Pin_MOSI_1, GPIO_PIN_RESET);
#else
#endif
}

int spi_master_soft_init_s(struct spi_master *master)
{
	if (!master || !master->init) {
		return -EINVAL;
	}

	master->init();
	return 0;
}

void spi_set_mode(u8 mode)
{
	if (mode <= SPI_MODE_3) {
		spi_config.mode = mode;
		pr_debug("spi_master: Set mode to %d", mode);
	}
}

void spi_set_bit_order(u8 order)
{
	if (order == SPI_BIT_ORDER_MSB || order == SPI_BIT_ORDER_LSB) {
		spi_config.bit_order = order;
		pr_debug("spi_master: Set bit order to %s",
			order == SPI_BIT_ORDER_MSB ? "MSB" : "LSB");
	}
}

void spi_set_speed(u8 speed)
{
	if (speed == SPI_SPEED_SLOW || speed == SPI_SPEED_FAST) {
		spi_config.speed = speed;
		pr_debug("spi_master: Set speed to %d", speed);
	}
}

int spi_master_soft_send_byte(struct spi_master *master, u8 data)
{
	u8 i = 8;
	u8 bit;

	if (!master) {
		return -EINVAL;
	}

	if (spi_config.mode & 0x01) {
		/* CPHA=1: Sample on second edge, setup on first edge */
		master->sck_low();
	} else {
		/* CPHA=0: Sample on first edge, setup before edge */
		master->sck_low();
	}

	while (i--) {
		if (spi_config.bit_order == SPI_BIT_ORDER_MSB) {
			/* MSB first */
			bit = (data >> i) & 0x01;
		} else {
			/* LSB first */
			bit = (data >> (7 - i)) & 0x01;
		}

		if (bit) {
			master->mosi_high();
		} else {
			master->mosi_low();
		}

		spi_master_soft_delay(SPI_DELAY_SETUP);

		if (spi_config.mode & 0x01) {
			/* CPHA=1: Clock rising edge for data setup */
			master->sck_high();
			spi_master_soft_delay(SPI_DELAY_HALF_CYCLE);
			master->sck_low();
			spi_master_soft_delay(SPI_DELAY_HOLD);
		} else {
			/* CPHA=0: Clock rising edge for data sampling */
			master->sck_high();
			spi_master_soft_delay(SPI_DELAY_HALF_CYCLE);
			master->sck_low();
			spi_master_soft_delay(SPI_DELAY_HOLD);
		}
	}

	return 0;
}

u8 spi_master_soft_read_byte(struct spi_master *master)
{
	u8 i = 8;
	u8 data = 0;
	u8 bit;

	if (!master) {
		return 0;
	}

	if (spi_config.mode & 0x01) {
		master->sck_low();
	} else {
		master->sck_low();
	}

	while (i--) {
		if (spi_config.mode & 0x01) {
			/* CPHA=1 */
			master->sck_high();
			spi_master_soft_delay(SPI_DELAY_HALF_CYCLE);
			bit = master->miso_read() ? 1 : 0;
			master->sck_low();
			spi_master_soft_delay(SPI_DELAY_HOLD);
		} else {
			/* CPHA=0 */
			master->sck_high();
			spi_master_soft_delay(SPI_DELAY_HALF_CYCLE);
			bit = master->miso_read() ? 1 : 0;
			master->sck_low();
			spi_master_soft_delay(SPI_DELAY_HOLD);
		}

		if (spi_config.bit_order == SPI_BIT_ORDER_MSB) {
			data |= (bit << i);
		} else {
			data |= (bit << (7 - i));
		}
	}

	return data;
}

u8 spi_master_soft_transfer_byte(struct spi_master *master, u8 data)
{
	u8 i = 8;
	u8 bit;
	u8 received_data = 0;

	if (!master) {
		return 0;
	}

	if (spi_config.mode & 0x01) {
		master->sck_low();
	} else {
		master->sck_low();
	}

	while (i--) {
		if (spi_config.bit_order == SPI_BIT_ORDER_MSB) {
			bit = (data >> i) & 0x01;
		} else {
			bit = (data >> (7 - i)) & 0x01;
		}

		if (bit) {
			master->mosi_high();
		} else {
			master->mosi_low();
		}

		spi_master_soft_delay(SPI_DELAY_SETUP);

		if (spi_config.mode & 0x01) {
			master->sck_high();
			spi_master_soft_delay(SPI_DELAY_HALF_CYCLE);
			bit = master->miso_read() ? 1 : 0;
			master->sck_low();
			spi_master_soft_delay(SPI_DELAY_HOLD);
		} else {
			master->sck_high();
			spi_master_soft_delay(SPI_DELAY_HALF_CYCLE);
			bit = master->miso_read() ? 1 : 0;
			master->sck_low();
			spi_master_soft_delay(SPI_DELAY_HOLD);
		}

		if (spi_config.bit_order == SPI_BIT_ORDER_MSB) {
			received_data |= (bit << i);
		} else {
			received_data |= (bit << (7 - i));
		}
	}

	return received_data;
}

static int spi_master_soft_write_bytes_with_reg(struct spi_master *master, u8 reg, u8 *pdata, u8 length)
{
	int ret;
	u8 i;

	if (!master || !pdata || length == 0) {
		return -EINVAL;
	}

	master->cs_low();
	spi_master_soft_delay(SPI_DELAY_SETUP);

	/* Send register address first */
	ret = spi_master_soft_send_byte(master, reg);
	if (ret) {
		master->cs_high();
		return ret;
	}

	/* Send data bytes */
	for (i = 0; i < length; i++) {
		ret = spi_master_soft_send_byte(master, pdata[i]);
		if (ret) {
			master->cs_high();
			return ret;
		}
	}

	spi_master_soft_delay(SPI_DELAY_HOLD);
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
	spi_master_soft_delay(SPI_DELAY_SETUP);

	/* Send register address first */
	spi_master_soft_send_byte(master, reg);

	/* Read data bytes */
	for (i = 0; i < length; i++) {
		pdata[i] = spi_master_soft_read_byte(master);
	}

	spi_master_soft_delay(SPI_DELAY_HOLD);
	master->cs_high();

	return 0;
}

int spi_master_port_transmit(struct spi_master *master, u8 reg, u8 *pdata, u8 length)
{
	if (!master || !pdata) {
		return -EINVAL;
	}

	if (length == 0) {
		return 0;
	}

	if (master->write_bytes) {
		return master->write_bytes(reg, pdata, length);
	} else {
		return spi_master_soft_write_bytes_with_reg(master, reg, pdata, length);
	}
}

int spi_master_port_receive(struct spi_master *master, u8 reg, u8 *pdata, u8 length)
{
	if (!master || !pdata) {
		return -EINVAL;
	}

	if (length == 0) {
		return 0;
	}

	if (master->read_bytes) {
		return master->read_bytes(reg, pdata, length);
	} else {
		return spi_master_soft_read_bytes_with_reg(master, reg, pdata, length);
	}
}

#ifdef KINETIS_FAKE_SIM
/* SPI Slave state machine */
enum {
	SPI_SLAVE_STATE_IDLE = 0,
	SPI_SLAVE_STATE_RECEIVING_DATA,
	SPI_SLAVE_STATE_TRANSMITTING_DATA,
	SPI_SLAVE_STATE_CS_DEASSERTED
};

/* Bus simulation variables */
static struct {
	bool cs_line;
	bool mosi_line;
	bool miso_line;
	bool sck_line;
	u8 master_mosi_direction;
	u8 slave_miso_direction;
} spi_bus_simulation = {
	.cs_line = 1,  /* CS high (deselected) by default */
	.mosi_line = 0,
	.miso_line = 0,
	.sck_line = 0,
	.master_mosi_direction = 1,
	.slave_miso_direction = 0
};

static const char *spi_slave_get_state_name(u8 state)
{
	switch (state) {
	case SPI_SLAVE_STATE_IDLE:
		return "IDLE";
	case SPI_SLAVE_STATE_RECEIVING_DATA:
		return "RECEIVING_DATA";
	case SPI_SLAVE_STATE_TRANSMITTING_DATA:
		return "TRANSMITTING_DATA";
	case SPI_SLAVE_STATE_CS_DEASSERTED:
		return "CS_DEASSERTED";
	default:
		return "UNKNOWN";
	}
}

/**
 * @brief Read current CS, SCK, MOSI, MISO line states
 */
static void spi_slave_read_bus_lines(bool *cs, bool *sck, bool *mosi, bool *miso)
{
	*cs = spi_bus_simulation.cs_line;
	*sck = spi_bus_simulation.sck_line;
	*mosi = spi_bus_simulation.mosi_line;
	*miso = spi_bus_simulation.miso_line;
}

/**
 * @brief Set MISO line state (for slave output)
 */
static void spi_slave_set_miso(bool state)
{
	spi_bus_simulation.miso_line = state;
	spi_bus_simulation.slave_miso_direction = 1;
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

/**
 * @brief Main SPI slave state machine loop (thread function)
 */
static void *spi_slave_state_machine_thread(void *data)
{
	struct spi_slave *device = (struct spi_slave *)data;
	bool cs_current, sck_current, mosi_current, miso_current;
	bool cs_last = 1, sck_last = 0, mosi_last = 0;
	u8 bit_count = 0;
	u8 current_byte = 0;

	/* Set thread to cancelable */
	pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
	pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, NULL);

	while (device->thread_running) {
		/* Read current bus line states */
		spi_slave_read_bus_lines(&cs_current, &sck_current, &mosi_current, &miso_current);

		/* State machine - switch case implementation */
		switch (device->current_state) {
		case SPI_SLAVE_STATE_IDLE:
			/* Wait for CS to go low (slave selected) */
			if (!cs_current && cs_last) {
				device->current_state = SPI_SLAVE_STATE_RECEIVING_DATA;
				device->cs_asserted = 1;
				device->bit_count = 0;
				device->byte_count = 0;
				device->current_byte = 0;
				device->index = 0;
				pr_debug("spi_slave(%s): CS asserted, starting reception", device->name);
			}
			break;

		case SPI_SLAVE_STATE_RECEIVING_DATA:
			/* Check for CS deassertion */
			if (cs_current) {
				device->current_state = SPI_SLAVE_STATE_CS_DEASSERTED;
				pr_debug("spi_slave(%s): CS deasserted, stopping reception", device->name);
				break;
			}

			/* Clock edge detection for data sampling */
			if (!sck_last && sck_current) {  /* SCK rising edge */
				/* Sample MOSI data based on CPHA */
				if (spi_config.mode & 0x01) {
					/* CPHA=1: Sample on rising edge */
					if (device->bit_count < 8) {
						if (mosi_current) {
							device->current_byte |= 1 << (7 - device->bit_count);
						}

						pr_debug("spi_slave(%s): Received bit%d(%d)",
							device->name, 7 - device->bit_count, mosi_current);
						device->bit_count++;
					}
				} else {
					/* CPHA=0: Sample on rising edge */
					if (device->bit_count < 8) {
						if (mosi_current) {
							device->current_byte |= 1 << (7 - device->bit_count);
						}

						pr_debug("spi_slave(%s): Received bit%d(%d)",
							device->name, 7 - device->bit_count, mosi_current);
						device->bit_count++;
					}
				}

				if (device->bit_count == 8) {
					/* Byte received */
					device->byte_count++;
					spi_slave_handle_byte_received(device, device->current_byte);
					pr_debug("spi_slave(%s): Received data byte 0x%02X, byte count: %d",
						device->name, device->current_byte, device->byte_count);
					device->bit_count = 0;
					device->current_byte = 0;

					/* Switch to transmit mode for full-duplex SPI */
					device->current_state = SPI_SLAVE_STATE_TRANSMITTING_DATA;
					device->current_byte = spi_slave_get_next_tx_byte(device);
					/* Set up first bit (bit7) immediately */
					bool bit = (device->current_byte >> 7) & 0x01;
					spi_slave_set_miso(bit);
					pr_debug("spi_slave(%s): Set up bit7(%d) for transmission", device->name, bit);
				}
			} else if (sck_last && !sck_current) {  /* SCK falling edge */
				/* CPHA=1: Setup data on falling edge */
				if (spi_config.mode & 0x01) {
					if (device->current_state == SPI_SLAVE_STATE_TRANSMITTING_DATA) {
						/* Prepare next bit */
						if (device->bit_count < 7) {
							bool bit = (device->current_byte >> (6 - device->bit_count)) & 0x01;
							spi_slave_set_miso(bit);
							pr_debug("spi_slave(%s): Set up bit%d(%d)",
								device->name, 6 - device->bit_count, bit);
						}
					}
				}
			}
			break;

		case SPI_SLAVE_STATE_TRANSMITTING_DATA:
			/* Check for CS deassertion */
			if (cs_current) {
				device->current_state = SPI_SLAVE_STATE_CS_DEASSERTED;
				pr_debug("spi_slave(%s): CS deasserted, stopping transmission", device->name);
				break;
			}

			/* Clock edge detection for data transmission */
			if (!sck_last && sck_current) {  /* SCK rising edge */
				if (spi_config.mode & 0x01) {
					/* CPHA=1: Data was set on falling edge, master samples here */
					/* Prepare next bit after sampling */
					if (device->bit_count < 7) {
						device->bit_count++;
						bool bit = (device->current_byte >> (7 - device->bit_count)) & 0x01;
						spi_slave_set_miso(bit);
						pr_debug("spi_slave(%s): Set up bit%d(%d)",
							device->name, 7 - device->bit_count, bit);
					} else if (device->bit_count == 7) {
						/* Last bit transmitted */
						device->byte_count++;
						device->current_byte = spi_slave_get_next_tx_byte(device);
						device->bit_count = 0;
						pr_debug("spi_slave(%s): Byte transmitted, preparing next byte", device->name);

						/* Prepare first bit of next byte */
						bool bit = (device->current_byte >> 7) & 0x01;
						spi_slave_set_miso(bit);
					}
				}
			} else if (sck_last && !sck_current) {  /* SCK falling edge */
				if (!(spi_config.mode & 0x01)) {
					/* CPHA=0: Setup data for master to sample on rising edge */
					if (device->bit_count < 8) {
						bool bit = (device->current_byte >> (7 - device->bit_count)) & 0x01;
						spi_slave_set_miso(bit);
						pr_debug("spi_slave(%s): Set up bit%d(%d)",
							device->name, 7 - device->bit_count, bit);
						device->bit_count++;
					} else {
						/* Byte transmitted */
						device->byte_count++;
						device->current_byte = spi_slave_get_next_tx_byte(device);
						device->bit_count = 0;
						pr_debug("spi_slave(%s): Byte transmitted, preparing next byte", device->name);
					}
				}
			}
			break;

		case SPI_SLAVE_STATE_CS_DEASSERTED:
			/* Reset to idle state */
			device->current_state = SPI_SLAVE_STATE_IDLE;
			device->bit_count = 0;
			device->byte_count = 0;
			device->current_byte = 0;
			device->cs_asserted = 0;

			/* Release MISO line */
			spi_slave_set_miso(0);
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

	ret = pthread_create(&device->thread, NULL, spi_slave_state_machine_thread, device);
	if (ret != 0) {
		device->thread_running = 0;
		pr_err("spi_slave(%s): Failed to create thread: %d", device->name, ret);
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

	/* Wait for thread to finish */
	if (pthread_join(device->thread, &thread_ret) == 0) {
		pr_info("spi_slave(%s): Thread stopped successfully", device->name);
	} else {
		pr_warn("spi_slave(%s): Thread stop had issues, but continuing", device->name);
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

	kfree(device);
}

/**
 * @brief Initialize SPI slave
 * @param name Device name for logging
 * @param buffer Pointer to internal buffer for data storage
 * @param buffer_size Size of internal buffer
 */
struct spi_slave *spi_slave_soft_init(char *name, u8 *buffer, u32 buffer_size)
{
	struct spi_slave *device;
	int ret;

	device = kzalloc(sizeof(struct spi_slave), GFP_KERNEL);
	if (!device) {
		return ERR_PTR(-ENOMEM);
	}

	device->name = name;
	device->buffer_size = buffer_size;
	device->buffer = buffer;
	device->last_state = SPI_SLAVE_STATE_IDLE;
	device->current_state = SPI_SLAVE_STATE_IDLE;
	device->index = 0;
	device->cs_asserted = 0;

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
	pr_debug("spi_master: cs pulled low");
	spi_bus_simulation.cs_line = 0;
	return 0;
}

int fake_spi_cs_high(void)
{
	pr_debug("spi_master: cs pulled high");
	spi_bus_simulation.cs_line = 1;
	return 0;
}

int fake_spi_mosi_low(void)
{
	pr_debug("spi_master: mosi pulled low");
	spi_bus_simulation.mosi_line = 0;
	spi_bus_simulation.master_mosi_direction = 1;
	return 0;
}

int fake_spi_mosi_high(void)
{
	pr_debug("spi_master: mosi pulled high");
	spi_bus_simulation.mosi_line = 1;
	spi_bus_simulation.master_mosi_direction = 1;
	return 0;
}

int fake_spi_miso_read(void)
{
	/* Read from shared bus simulation */
	/* Slave driving MISO has highest priority */
	if (spi_bus_simulation.slave_miso_direction) {
		pr_debug("spi_master: reading miso(%d) from slave", spi_bus_simulation.miso_line);
		return spi_bus_simulation.miso_line;
	}
	/* If master is driving MISO (unlikely in SPI), return master's last set value */
	else if (spi_bus_simulation.master_mosi_direction) {
		pr_debug("spi_master: reading miso(%d) from master", spi_bus_simulation.miso_line);
		return spi_bus_simulation.miso_line;
	}
	/* Otherwise, return default state */
	else {
		pr_debug("spi_master: reading miso(0) default");
		return 0;
	}
}

int fake_spi_sck_low(void)
{
	pr_debug("spi_master: sck pulled low");
	spi_bus_simulation.sck_line = 0;
	return 0;
}

int fake_spi_sck_high(void)
{
	pr_debug("spi_master: sck pulled high");
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
	.id = SPI_SW_1,
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
#else
struct spi_master spi_master_1 = {
	.id = SPI_SW_1,
	.cs_low = spi_cs_low,
	.cs_high = spi_cs_high,
	.mosi_low = spi_mosi_low,
	.mosi_high = spi_mosi_high,
	.miso_read = spi_miso_read,
	.sck_low = spi_sck_low,
	.sck_high = spi_sck_high,
	.write_bytes = NULL,
	.read_bytes = NULL,
	.init = spi_master_soft_init_s,
};
#endif

int t_spi_slave_basic(int argc, char **argv)
{
	struct spi_slave *device;
	u8 test_buffer[100] = {0xAA, 0x55, 0x12, 0x34, 0x78, 0x9C, 0xFF, 0x00};
	u8 write_test_data[] = {0x55, 0xAA, 0x12, 0x34, 0x78, 0x9C};
	u8 read_test_data[8] = {1, 2, 3, 4, 5, 6, 7, 8};
	int ret;
	int i;

	pr_info("=== spi slave basic test ===\n");

	spi_master_soft_init_s(&fake_spi_master);

	device = spi_slave_soft_init("spi_test", test_buffer, ARRAY_SIZE(test_buffer));
	if (IS_ERR(device)) {
		pr_err("spi slave init failed\n");
		return FAIL;
	}

	ret = spi_master_port_transmit(&fake_spi_master, 0, write_test_data, 6);
	if (ret) {
		pr_err("master write operation failed: %d\n", ret);
		spi_slave_soft_exit(device);
		return FAIL;
	}

	ret = spi_master_port_receive(&fake_spi_master, 0, read_test_data, 6);
	if (ret) {
		pr_err("master read operation failed: %d\n", ret);
		spi_slave_soft_exit(device);
		return FAIL;
	}

	for (i = 0; i < 6; i++) {
		if (test_buffer[i] != read_test_data[i]) {
			pr_err("mismatch at buffer[%d]: expected %02X, got %02X\n",
				i, test_buffer[i], read_test_data[i]);
			spi_slave_soft_exit(device);
			return FAIL;
		}
	}

	pr_info("spi slave basic test passed! data matched.\n");

	spi_slave_soft_exit(device);

	return PASS;
}

int t_spi_transfer_byte(int argc, char **argv)
{
	u8 test_values[] = {0x00, 0x01, 0x7F, 0x80, 0xFF, 0xAA, 0x55};
	u8 received;
	int ret, i;

	pr_info("=== spi transfer byte test ===\n");

	spi_master_soft_init_s(&fake_spi_master);

	for (i = 0; i < ARRAY_SIZE(test_values); i++) {
		received = spi_master_soft_transfer_byte(&fake_spi_master, test_values[i]);

		pr_info("sent: 0x%02X, received: 0x%02X\n", test_values[i], received);

		/* In loopback mode, received should equal sent */
		/* Note: In actual hardware, MISO might be driven by slave device */
	}

	pr_info("spi transfer byte test completed\n");

	return PASS;
}

int t_spi_transfer_bytes(int argc, char **argv)
{
	u8 write_buffer[256];
	u8 read_buffer[256];
	int ret, i;
	int test_sizes[] = {1, 2, 4, 8, 16, 32, 64, 128, 255};

	pr_info("=== spi transfer bytes test ===\n");

	spi_master_soft_init_s(&fake_spi_master);

	/* Test different buffer sizes */
	for (i = 0; i < ARRAY_SIZE(test_sizes); i++) {
		int size = test_sizes[i];
		int j;

		/* Prepare test data */
		for (j = 0; j < size; j++) {
			write_buffer[j] = (u8)(j & 0xFF);
		}

		/* Transmit data */
		ret = spi_master_port_transmit(&fake_spi_master, 0, write_buffer, size);
		if (ret) {
			pr_err("transmit failed for size %d: %d\n", size, ret);
			return FAIL;
		}

		/* Receive data */
		ret = spi_master_port_receive(&fake_spi_master, 0, read_buffer, size);
		if (ret) {
			pr_err("receive failed for size %d: %d\n", size, ret);
			return FAIL;
		}

		pr_info("transferred %d bytes successfully\n", size);
	}

	pr_info("spi transfer bytes test passed\n");

	return PASS;
}

int t_spi_mode_test(int argc, char **argv)
{
	u8 test_data = 0xAA;
	u8 received;
	int i;

	pr_info("=== spi mode test ===\n");

	spi_master_soft_init_s(&fake_spi_master);

	/* Test all 4 SPI modes */
	for (i = 0; i <= 3; i++) {
		spi_set_mode(i);
		pr_info("testing spi mode %d\n", i);

		received = spi_master_soft_transfer_byte(&fake_spi_master, test_data);
		pr_info("mode %d: sent 0x%02X, received 0x%02X\n", i, test_data, received);
	}

	/* Restore default mode */
	spi_set_mode(SPI_MODE_CONFIG);

	pr_info("spi mode test completed\n");

	return PASS;
}

int t_spi_bit_order_test(int argc, char **argv)
{
	u8 test_data = 0x81;  /* Binary: 10000001 */
	u8 received;

	pr_info("=== spi bit order test ===\n");

	spi_master_soft_init_s(&fake_spi_master);

	/* Test MSB first */
	spi_set_bit_order(SPI_BIT_ORDER_MSB);
	received = spi_master_soft_transfer_byte(&fake_spi_master, test_data);
	pr_info("msb first: sent 0x%02X, received 0x%02X\n", test_data, received);

	/* Test LSB first */
	spi_set_bit_order(SPI_BIT_ORDER_LSB);
	received = spi_master_soft_transfer_byte(&fake_spi_master, test_data);
	pr_info("lsb first: sent 0x%02X, received 0x%02X\n", test_data, received);

	/* Restore default */
	spi_set_bit_order(SPI_BIT_ORDER_CONFIG);

	pr_info("spi bit order test completed\n");

	return PASS;
}

int t_spi_speed_test(int argc, char **argv)
{
	u8 write_buffer[100];
	u8 read_buffer[100];
	int ret, i;

	pr_info("=== spi speed test ===\n");

	spi_master_soft_init_s(&fake_spi_master);

	/* Prepare test data */
	for (i = 0; i < ARRAY_SIZE(write_buffer); i++) {
		write_buffer[i] = (u8)(i & 0xFF);
	}

	/* Test slow mode */
	spi_set_speed(SPI_SPEED_SLOW);
	pr_info("testing slow mode (100kHz)\n");
	ret = spi_master_port_transmit(&fake_spi_master, 0, write_buffer, ARRAY_SIZE(write_buffer));
	if (ret) {
		pr_err("slow mode transmit failed: %d\n", ret);
		return FAIL;
	}

	/* Test fast mode */
	spi_set_speed(SPI_SPEED_FAST);
	pr_info("testing fast mode (1MHz)\n");
	ret = spi_master_port_transmit(&fake_spi_master, 0, write_buffer, ARRAY_SIZE(write_buffer));
	if (ret) {
		pr_err("fast mode transmit failed: %d\n", ret);
		return FAIL;
	}

	/* Restore default */
	spi_set_speed(SPI_SPEED_MODE);

	pr_info("spi speed test completed\n");

	return PASS;
}

int t_spi_edge_cases(int argc, char **argv)
{
	u8 write_buffer[4];
	u8 read_buffer[4];
	int ret;

	pr_info("=== spi edge cases test ===\n");

	spi_master_soft_init_s(&fake_spi_master);

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

	return PASS;
}

int t_spi_performance(int argc, char **argv)
{
	u8 write_buffer[256];
	u8 read_buffer[256];
	int ret, i;
	int iterations = 100;
	int success_count = 0;

	pr_info("=== spi performance test ===\n");

	spi_master_soft_init_s(&fake_spi_master);

	/* Prepare test data */
	for (i = 0; i < ARRAY_SIZE(write_buffer); i++) {
		write_buffer[i] = (u8)(i & 0xFF);
	}

	/* Perform multiple iterations */
	for (i = 0; i < iterations; i++) {
		ret = spi_master_port_transmit(&fake_spi_master, 0, write_buffer, ARRAY_SIZE(write_buffer));
		if (ret == 0) {
			success_count++;
		}
	}

	pr_info("performance test: %d/%d transfers succeeded\n", success_count, iterations);

	if (success_count == iterations) {
		pr_info("spi performance test passed\n");
		return PASS;
	} else {
		pr_err("spi performance test failed: only %d/%d succeeded\n", success_count, iterations);
		return FAIL;
	}
}

int t_spi_stress(int argc, char **argv)
{
	u8 write_buffer[128];
	u8 read_buffer[128];
	int ret, i, j;

	pr_info("=== spi stress test ===\n");

	spi_master_soft_init_s(&fake_spi_master);

	/* Test rapid mode switching */
	for (i = 0; i < 10; i++) {
		for (j = 0; j < 4; j++) {
			spi_set_mode(j);
		}
		spi_set_bit_order(SPI_BIT_ORDER_MSB);
		spi_set_bit_order(SPI_BIT_ORDER_LSB);
	}

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

	/* Restore defaults */
	spi_set_mode(SPI_MODE_CONFIG);
	spi_set_bit_order(SPI_BIT_ORDER_CONFIG);

	pr_info("spi stress test passed\n");

	return PASS;
}

int t_spi_read_write_reg(int argc, char **argv)
{
	u8 test_data[] = {0x12, 0x34, 0x56, 0x78};
	u8 read_data[4];
	u8 reg_addr = 0x05;
	int ret;

	pr_info("=== spi read write reg test ===\n");

	spi_master_soft_init_s(&fake_spi_master);

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

	return PASS;
}

int t_spi_boundary_large(int argc, char **argv)
{
	u8 large_buffer[512];
	u8 read_buffer[512];
	int ret, i;
	int large_sizes[] = {200, 300, 400, 500, 511};

	pr_info("=== spi boundary large test ===\n");

	spi_master_soft_init_s(&fake_spi_master);

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

	return PASS;
}

#endif /* DESIGN_VERIFICATION_SPI */

/* Legacy functions for backward compatibility */
#ifdef KINETIS_FAKE_SIM
void spi_write_data(const u8 reg, const u8 val)
{
	u8 data = val;
	spi_master_port_transmit(&fake_spi_master, reg, &data, 1);
}

u8 spi_read_data(const u8 reg)
{
	u8 data;
	spi_master_port_receive(&fake_spi_master, reg, &data, 1);
	return data;
}
#else
void spi_write_data(const u8 reg, const u8 val)
{
	u8 data = val;
	spi_master_port_transmit(&spi_master_1, reg, &data, 1);
}

u8 spi_read_data(const u8 reg)
{
	u8 data;
	spi_master_port_receive(&spi_master_1, reg, &data, 1);
	return data;
}
#endif

void spi_set_baudrate(u16 baudrate_divisor)
{
	pr_debug("spi_master: Set baudrate divisor %d (not implemented)", baudrate_divisor);
}
