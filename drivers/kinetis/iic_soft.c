
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
#define IIC_SPEED_FAST        1  /* 400kHz Fast Mode */

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

static int iic_master_soft_write_bytes_with_addr(u8 iic, u8 slave_addr, u16 reg,
	u8 *pdata, u8 length);
static int iic_master_soft_read_bytes_with_addr(u8 iic, u8 slave_addr, u16 reg,
	u8 *pdata, u8 length);

void iic_master_soft_init(void)
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

void iic_master_soft_delay(u32 ticks)
{
#ifdef KINETIS_FAKE_SIM
	// 	pr_debug("iic_master: delay %d us", ticks);
#endif
	udelay(ticks);
}

void iic_master_port_transmmit(u8 iic, u8 slave_addr, u16 reg, u8 tmp)
{
	if (iic == IIC_SW_1) {
		iic_master_soft_write_bytes_with_addr(iic, slave_addr, reg, &tmp, 1);
	} else if (iic == IIC_HW_1) {
#if MCU_PLATFORM_STM32
		HAL_I2C_Mem_Write_DMA(&hi2c1, (u16)slave_addr, reg, I2C_MEMADD_SIZE_8BIT,
			&tmp, 1);
#else
#endif
	}
}

void iic_master_port_receive(u8 iic, u8 slave_addr, u16 reg, u8 *tmp)
{
	if (iic == IIC_SW_1) {
		iic_master_soft_read_bytes_with_addr(iic, slave_addr, reg, tmp, 1);
	} else if (iic == IIC_HW_1) {
#if MCU_PLATFORM_STM32
		HAL_I2C_Mem_Read_DMA(&hi2c1, (u16)(slave_addr), reg, I2C_MEMADD_SIZE_8BIT,
			tmp, 1);
#else
#endif
	}
}

void iic_master_port_multi_transmmit(u8 iic, u8 slave_addr, u16 reg,
	u8 *pdata, u8 length)
{
	if (iic == IIC_SW_1) {
		iic_master_soft_write_bytes_with_addr(iic, slave_addr, reg, pdata, length);
	} else if (iic == IIC_HW_1) {
#if MCU_PLATFORM_STM32
		HAL_I2C_Mem_Write_DMA(&hi2c1, (u16)slave_addr, reg, I2C_MEMADD_SIZE_8BIT,
			pdata, length);
#else
#endif
	}
}

void iic_master_port_multi_receive(u8 iic, u8 slave_addr, u16 reg,
	u8 *pdata, u8 length)
{
	if (iic == IIC_SW_1) {
		iic_master_soft_read_bytes_with_addr(iic, slave_addr, reg, pdata, length);
	} else if (iic == IIC_HW_1) {
#if MCU_PLATFORM_STM32
		HAL_I2C_Mem_Read_DMA(&hi2c1, (u16)(slave_addr), reg, I2C_MEMADD_SIZE_8BIT,
			pdata, length);
#else
#endif
	}
}

/* The above procedure is modified by the user according to the hardware device, otherwise the driver cannot run. */

static inline void scl_low(u8 iic)
{
#ifdef KINETIS_FAKE_SIM
	pr_debug("iic_master: scl pulled low");
	i2c_bus_simulation.scl_line = 0;
#else
#if MCU_PLATFORM_STM32
	if (iic == IIC_SW_1) {
		HAL_GPIO_WritePin(GPIOB, GPIO_PIN_8, GPIO_PIN_RESET);
	} else if (iic == IIC_SW_2) {
		HAL_GPIO_WritePin(GPIOH, GPIO_PIN_6, GPIO_PIN_RESET);
	}
#else
#endif
#endif
}

static inline void scl_high(u8 iic)
{
#ifdef KINETIS_FAKE_SIM
	pr_debug("iic_master: scl pulled high");
	i2c_bus_simulation.scl_line = 1;
#else
#if MCU_PLATFORM_STM32
	if (iic == IIC_SW_1) {
		HAL_GPIO_WritePin(GPIOB, GPIO_PIN_8, GPIO_PIN_SET);
	} else if (iic == IIC_SW_2) {
		HAL_GPIO_WritePin(GPIOH, GPIO_PIN_6, GPIO_PIN_SET);
	}
#else
#endif
#endif
}

static inline void sda_low(u8 iic)
{
#ifdef KINETIS_FAKE_SIM
	pr_debug("iic_master: sda pulled low");
	i2c_bus_simulation.sda_line = 0;
	i2c_bus_simulation.master_sda_direction = 1;
#else
#if MCU_PLATFORM_STM32
	if (iic == IIC_SW_1) {
		HAL_GPIO_WritePin(GPIOB, GPIO_PIN_9, GPIO_PIN_RESET);
	} else if (iic == IIC_SW_2) {
		HAL_GPIO_WritePin(GPIOI, GPIO_PIN_3, GPIO_PIN_RESET);
	}
#else
#endif
#endif
}

static inline void sda_high(u8 iic)
{
#ifdef KINETIS_FAKE_SIM
	pr_debug("iic_master: sda pulled high");
	i2c_bus_simulation.sda_line = 1;
	i2c_bus_simulation.master_sda_direction = 1;
#else
#if MCU_PLATFORM_STM32
	if (iic == IIC_SW_1) {
		HAL_GPIO_WritePin(GPIOB, GPIO_PIN_9, GPIO_PIN_SET);
	} else if (iic == IIC_SW_2) {
		HAL_GPIO_WritePin(GPIOI, GPIO_PIN_3, GPIO_PIN_SET);
	}
#else
#endif
#endif
}

static inline void sda_in(u8 iic)
{
#ifdef KINETIS_FAKE_SIM
	pr_debug("iic_master: sda set to input");
	/* Set SDA direction to input for master (will be controlled by slave) */
	i2c_bus_simulation.master_sda_direction = 0;
	/* When in input mode, let slave control the line if it's driving */
	if (!i2c_bus_simulation.slave_sda_direction) {
		/* Only set to high if slave is not driving the line */
		i2c_bus_simulation.sda_line = 1;
	}
	/* If slave is driving, keep slave's value (should be low for ACK) */
#else
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
#endif
}

static inline void sda_out(u8 iic)
{
#ifdef KINETIS_FAKE_SIM
	pr_debug("iic_master: sda set to output");
	/* Set SDA direction to output for master */
	i2c_bus_simulation.master_sda_direction = 1;
	i2c_bus_simulation.slave_sda_direction = 0;
	/* When SDA is set to output, it should default to high due to pull-up */
	/* The actual level will be set by sda_high() or sda_low() */
#else
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
#endif
}

static inline int sda_read(u8 iic)
{
#ifdef KINETIS_FAKE_SIM
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
#else
#if MCU_PLATFORM_STM32
	if (iic == IIC_SW_1) {
		return HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_9);
	} else if (iic == IIC_SW_2) {
		return HAL_GPIO_ReadPin(GPIOI, GPIO_PIN_3);
	}
#else
#endif

	return 0;
#endif
}

int iic_master_soft_start(u8 iic)
{
	sda_out(iic);
	sda_high(iic);
	scl_high(iic);
	iic_master_soft_delay(IIC_DELAY_START_SETUP);

	if (!sda_read(iic)) {
		return -EBUSY;
	}

	sda_low(iic);
	iic_master_soft_delay(IIC_DELAY_START_HOLD);

	if (sda_read(iic)) {
		return -EBUSY;
	}

	// 	scl_low(iic);
	// 	iic_master_soft_delay(IIC_DELAY_START_HOLD);

	return 0;
}

void iic_master_soft_stop(u8 iic)
{
	sda_out(iic);
	sda_low(iic);
	iic_master_soft_delay(IIC_DELAY_STOP_SETUP);
	scl_high(iic);
	iic_master_soft_delay(IIC_DELAY_STOP_SETUP);
	sda_high(iic);
	iic_master_soft_delay(IIC_DELAY_STOP_SETUP);
}

void iic_master_soft_ack(u8 iic)
{
	sda_out(iic);
	sda_low(iic);
	scl_low(iic);
	iic_master_soft_delay(IIC_DELAY_DATA_SETUP);
	scl_high(iic);
	iic_master_soft_delay(IIC_DELAY_DATA_HOLD);
	scl_low(iic);
	iic_master_soft_delay(IIC_DELAY_DATA_HOLD);
}

void iic_master_soft_no_ack(u8 iic)
{
	sda_out(iic);
	sda_high(iic);
	scl_low(iic);
	iic_master_soft_delay(IIC_DELAY_DATA_SETUP);
	scl_high(iic);
	iic_master_soft_delay(IIC_DELAY_DATA_HOLD);
	scl_low(iic);
	iic_master_soft_delay(IIC_DELAY_DATA_HOLD);
}

int iic_master_soft_wait_ack(u8 iic)
{
	u8 err_time = 0;

	sda_in(iic);
	scl_low(iic);
	iic_master_soft_delay(IIC_DELAY_DATA_SETUP);
	scl_high(iic);
	iic_master_soft_delay(IIC_DELAY_DATA_HOLD);

	/* Read SDA while SCL is high - this is when master should see ACK */
	if (sda_read(iic)) {
		/* SDA is high = NACK */
		pr_debug("iic_master: Received NACK");
		scl_low(iic);
		iic_master_soft_delay(IIC_DELAY_SCL_LOW);
		return -EIO;
	} else {
		/* SDA is low = ACK */
		pr_debug("iic_master: Received ACK");
		scl_low(iic);
		iic_master_soft_delay(IIC_DELAY_SCL_LOW);
		return 0;
	}
}

int iic_master_soft_send_byte(u8 iic, u8 tmp)
{
	u8 i = 8;
	int ret;

	sda_out(iic);

	while (i--) {
		scl_low(iic);
		iic_master_soft_delay(IIC_DELAY_SCL_LOW);

		if (tmp & 0x80) {
			sda_high(iic);
		} else {
			sda_low(iic);
		}

		tmp <<= 1;
		iic_master_soft_delay(IIC_DELAY_DATA_SETUP);
		scl_high(iic);
		iic_master_soft_delay(IIC_DELAY_SCL_HIGH);
	}
	scl_low(iic);
	iic_master_soft_delay(IIC_DELAY_SCL_LOW);

	ret = iic_master_soft_wait_ack(iic);
	if (ret) {
		iic_master_soft_stop(iic);
		return ret;
	}
}

u8 iic_master_soft_read_byte(u8 iic, u8 ack)
{
	u8 i = 8, tmp = 0;

	sda_in(iic);
	// 	scl_high(iic);
	// 	// sda_high(iic);
	// 	iic_master_soft_delay(IIC_DELAY_DATA_SETUP);

	while (i) {
		scl_low(iic);
		iic_master_soft_delay(IIC_DELAY_SCL_LOW);
		scl_high(iic);
		iic_master_soft_delay(IIC_DELAY_SCL_HIGH);

		if (sda_read(iic)) {
			tmp |= 0x01 << (i - 1);
		}
		i--;
		pr_debug("iic_master: Received data byte 0x%02X",
			tmp);
	}

	scl_low(iic);

	if (ack) {
		iic_master_soft_ack(iic);
	} else {
		iic_master_soft_no_ack(iic);
	}

	return tmp;
}

static int iic_master_soft_write_bytes_with_addr(u8 iic, u8 slave_addr, u16 reg,
	u8 *pdata, u8 length)
{
	int ret;

	if (iic_master_soft_start(iic) != 0) {
		pr_err("Arbitration failed ! Device(addr = 0x%X) cannot obtain the bus.",
			slave_addr);
		return -EBUSY;
	}

	ret = iic_master_soft_send_byte(iic, (slave_addr << 1) | 0x00);
	if (ret) {
		return ret;
	}

	if (ADDRESS_MODE == ADDRESS_16) {
		ret = iic_master_soft_send_byte(iic, reg >> 8);
		if (ret) {
			return ret;
		}
	}

	ret = iic_master_soft_send_byte(iic, reg & 0xFF);
	if (ret) {
		return ret;
	}

	while (length--) {
		ret = iic_master_soft_send_byte(iic, *pdata);
		if (ret) {
			return ret;
		}
		pdata++;
	}

	iic_master_soft_stop(iic);

	return 0;
}

static int iic_master_soft_read_bytes_with_addr(u8 iic, u8 slave_addr, u16 reg,
	u8 *pdata, u8 length)
{
	int ret;

	if (iic_master_soft_start(iic) != 0) {
		pr_err("Arbitration failed ! Device(addr = 0x%X) cannot obtain the bus.",
			slave_addr);
		return -EBUSY;
	}

	ret = iic_master_soft_send_byte(iic, (slave_addr << 1) | 0x00);
	if (ret) {
		return ret;
	}

	if (ADDRESS_MODE == ADDRESS_16) {
		ret = iic_master_soft_send_byte(iic, reg >> 8);
		if (ret) {
			return ret;
		}
	}

	ret = iic_master_soft_send_byte(iic, reg & 0xFF);
	if (ret) {
		return ret;
	}

	ret = iic_master_soft_start(iic);
	if (ret) {
		return ret;
	}

	ret = iic_master_soft_send_byte(iic, (slave_addr << 1) | 0x01);
	if (ret) {
		return ret;
	}

	while (length) {
		if (length == 1) {
			*pdata = iic_master_soft_read_byte(iic, 0);
		} else {
			*pdata = iic_master_soft_read_byte(iic, 1);
		}

		pdata++;
		length--;
	}

	iic_master_soft_stop(iic);

	return 0;
}

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
#ifdef KINETIS_FAKE_SIM
	*scl = i2c_bus_simulation.scl_line;
	*sda = i2c_bus_simulation.sda_line;
#else
	/* In real hardware, read GPIO pins */
	*scl = 1;  /* Default high (should read actual GPIO) */
	*sda = 1;  /* Default high (should read actual GPIO) */
#endif
}

/**
 * @brief Set SDA line state (for slave output)
 */
static void iic_slave_set_sda(bool state)
{
#ifdef KINETIS_FAKE_SIM
	i2c_bus_simulation.sda_line = state;
	/* Slave should always drive the SDA line when transmitting */
	i2c_bus_simulation.slave_sda_direction = 1;
#endif
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
		}  /* End of switch */

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

#ifdef DESIGN_VERIFICATION_IIC

int iic_slave_test(void)
{
	struct iic_slave *device;
	u8 test_buffer[100] = {1, 2, 3, 4, 5, 6, 7, 8};
	u8 write_test_data[] = {0x55, 0xAA, 0x12, 0x34, 0x78, 0x9C};
	u8 read_test_data[8] = {1, 2, 3, 4, 5, 6, 7, 8};
	int ret;

	pr_info("=== I2C Slave State Machine Test Started ===");

	device = iic_slave_soft_init("iic_test", 0x48, test_buffer, ARRAY_SIZE(test_buffer));
	if (IS_ERR(device)) {
		return PTR_ERR(device);
	}

	// 	ret = iic_master_soft_write_bytes_with_addr(IIC_SW_1, 0x48, 0, write_test_data, 3);
	// 	if (ret) {
	// 		pr_err("Master write operation failed");
	// 		return ret;
	// 	}

	ret = iic_master_soft_read_bytes_with_addr(IIC_SW_1, 0x48, 0, read_test_data, 3);
	if (ret) {
		pr_err("Master read operation failed");
		return ret;
	}
	for (int i = 0; i < ARRAY_SIZE(read_test_data); i++) {
		pr_info("read_test_data[%d]: %02X",
			i, read_test_data[i]);
	}

	// 	for (int i = 0; i < 3; i++) {
	// 		if (write_test_data[i] != read_test_data[i]) {
	// 			pr_err("Mismatch at buffer[%d]: expected %02X, got %02X",
	// 				i, write_test_data[i], read_test_data[i]);
	// 			return -EIO;
	// 		}
	// 	}

	iic_slave_soft_exit(device);

	return -1;
}

#endif /* DESIGN_VERIFICATION_IIC */
