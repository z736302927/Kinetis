#include <linux/delay.h>
#include <linux/err.h>
#include <linux/slab.h>
#include <linux/random.h>
#include <linux/limits.h>

#include "kinetis/max30205.h"
#include "kinetis/iic_soft.h"
#include "kinetis/spi_soft.h"
#include "kinetis/delay.h"
#include "kinetis/idebug.h"
#include "kinetis/design_verification.h"

#include <pthread.h>
#include <math.h>

/* Register addresses */
#define MAX30205_REG_TEMP               0x00
#define MAX30205_REG_CONFIG             0x01
#define MAX30205_REG_THYST              0x02
#define MAX30205_REG_TOS                0x03

/* Configuration register bits */
#define MAX30205_CONFIG_SHUTDOWN_BIT    0x01
#define MAX30205_CONFIG_MODE_BIT        0x02
#define MAX30205_CONFIG_OS_POLARITY_BIT 0x04
#define MAX30205_CONFIG_FAULT_QUEUE_MASK 0x18
#define MAX30205_CONFIG_DATA_FORMAT_BIT 0x20
#define MAX30205_CONFIG_TIMEOUT_BIT     0x40
#define MAX30205_CONFIG_ONE_SHOT_BIT    0x80

/* Fault queue values */
#define MAX30205_FAULT_QUEUE_1          0x00
#define MAX30205_FAULT_QUEUE_2          0x08
#define MAX30205_FAULT_QUEUE_4          0x10
#define MAX30205_FAULT_QUEUE_6          0x18

/* Operating modes */
#define MAX30205_MODE_COMPARATOR        0x00
#define MAX30205_MODE_INTERRUPT         0x01

/* Temperature conversion macros */
#define MAX30205_RAW_TO_CELSIUS(raw)    ((float)(raw) * 0.00390625f)
#define MAX30205_CELSIUS_TO_RAW(celsius) ((u16)((celsius) / 0.00390625f))

/* Device address (7-bit, 0x48 is typical for MAX30205) */
#define MAX30205_I2C_ADDR               0x48

struct max30205_device {
	struct regmap *regmap;

	float temperature_offset;
	float min_temperature;
	float max_temperature;
	u8 device_present;

	struct iic_slave *iic_slave;
	struct spi_slave *spi_slave;
	u8 *slave_regs;

	bool thread_running;
};

/* Device detection */
u8 max30205_is_device_present(struct max30205_device *dev)
{
	u32 test_data = 0;

	/* Try to read configuration register */
	regmap_read(dev->regmap, MAX30205_REG_CONFIG, &test_data);

	/* A valid MAX30205 should return any value when reading config register */
	dev->device_present = (test_data != 0xFF) ? 1 : 0;

	return dev->device_present;
}

/* Core temperature measurement functions */
u16 max30205_get_raw_temperature(struct max30205_device *dev)
{
	u8 temp_raw[2];

	regmap_bulk_read(dev->regmap, MAX30205_REG_TEMP, temp_raw, 2);

	return (temp_raw[0] << 8) | temp_raw[1];
}

float max30205_get_temperature(struct max30205_device *dev)
{
	u16 raw_temp;

	raw_temp = max30205_get_raw_temperature(dev);
	return MAX30205_RAW_TO_CELSIUS(raw_temp);
}

float max30205_get_temperature_with_calibration(struct max30205_device *dev)
{
	float temperature;

	temperature = max30205_get_temperature(dev);
	return temperature + dev->temperature_offset;
}

void max30205_set_shutdown_mode(struct max30205_device *dev, u8 enable)
{
	u32 config = 0;

	regmap_read(dev->regmap, MAX30205_REG_CONFIG, &config);

	if (enable) {
		config |= MAX30205_CONFIG_SHUTDOWN_BIT;
	} else {
		config &= ~MAX30205_CONFIG_SHUTDOWN_BIT;
	}

	regmap_write(dev->regmap, MAX30205_REG_CONFIG, config);
}

void max30205_set_operating_mode(struct max30205_device *dev, u8 mode)
{
	u32 config = 0;

	regmap_read(dev->regmap, MAX30205_REG_CONFIG, &config);

	if (mode == MAX30205_MODE_INTERRUPT) {
		config |= MAX30205_CONFIG_MODE_BIT;
	} else {
		config &= ~MAX30205_CONFIG_MODE_BIT;
	}

	regmap_write(dev->regmap, MAX30205_REG_CONFIG, config);
}

void max30205_set_os_polarity(struct max30205_device *dev, u8 polarity)
{
	u32 config = 0;

	regmap_read(dev->regmap, MAX30205_REG_CONFIG, &config);

	if (polarity) {
		config |= MAX30205_CONFIG_OS_POLARITY_BIT;
	} else {
		config &= ~MAX30205_CONFIG_OS_POLARITY_BIT;
	}

	regmap_write(dev->regmap, MAX30205_REG_CONFIG, config);
}

void max30205_set_fault_queue(struct max30205_device *dev, u8 fault_count)
{
	u32 config = 0;

	regmap_read(dev->regmap, MAX30205_REG_CONFIG, &config);

	config &= ~MAX30205_CONFIG_FAULT_QUEUE_MASK;
	config |= fault_count;

	regmap_write(dev->regmap, MAX30205_REG_CONFIG, config);
}

void max30205_set_data_format(struct max30205_device *dev, u8 format)
{
	u32 config = 0;

	regmap_read(dev->regmap, MAX30205_REG_CONFIG, &config);

	if (format) {
		config |= MAX30205_CONFIG_DATA_FORMAT_BIT;
	} else {
		config &= ~MAX30205_CONFIG_DATA_FORMAT_BIT;
	}

	regmap_write(dev->regmap, MAX30205_REG_CONFIG, config);
}

void max30205_enable_timeout(struct max30205_device *dev, u8 enable)
{
	u32 config = 0;

	regmap_read(dev->regmap, MAX30205_REG_CONFIG, &config);

	if (enable) {
		config |= MAX30205_CONFIG_TIMEOUT_BIT;
	} else {
		config &= ~MAX30205_CONFIG_TIMEOUT_BIT;
	}

	regmap_write(dev->regmap, MAX30205_REG_CONFIG, config);
}

void max30205_trigger_one_shot(struct max30205_device *dev)
{
	u32 config = 0;

	regmap_read(dev->regmap, MAX30205_REG_CONFIG, &config);
	config |= MAX30205_CONFIG_ONE_SHOT_BIT;
	regmap_write(dev->regmap, MAX30205_REG_CONFIG, config);

	/* Clear the bit after triggering */
	udelay(10);
	config &= ~MAX30205_CONFIG_ONE_SHOT_BIT;
	regmap_write(dev->regmap, MAX30205_REG_CONFIG, config);
}

/* Threshold management functions */
void max30205_set_threshold_high(struct max30205_device *dev, u16 threshold_raw)
{
	u8 threshold_data[2];

	dev->max_temperature = MAX30205_RAW_TO_CELSIUS(threshold_raw);
	threshold_data[0] = (threshold_raw >> 8) & 0xFF;
	threshold_data[1] = threshold_raw & 0xFF;

	regmap_bulk_write(dev->regmap, MAX30205_REG_TOS, threshold_data, 2);
}

void max30205_set_threshold_low(struct max30205_device *dev, u16 threshold_raw)
{
	u8 threshold_data[2];

	dev->min_temperature = MAX30205_RAW_TO_CELSIUS(threshold_raw);
	threshold_data[0] = (threshold_raw >> 8) & 0xFF;
	threshold_data[1] = threshold_raw & 0xFF;

	regmap_bulk_write(dev->regmap, MAX30205_REG_THYST, threshold_data, 2);
}

u16 max30205_get_threshold_high(struct max30205_device *dev)
{
	u8 threshold_data[2];

	regmap_bulk_read(dev->regmap, MAX30205_REG_TOS, threshold_data, 2);

	return (threshold_data[0] << 8) | threshold_data[1];
}

u16 max30205_get_threshold_low(struct max30205_device *dev)
{
	u8 threshold_data[2];

	regmap_bulk_read(dev->regmap, MAX30205_REG_THYST, threshold_data, 2);

	return (threshold_data[0] << 8) | threshold_data[1];
}

/* Status and flag management */
u8 max30205_check_os_flag(struct max30205_device *dev)
{
	u32 config = 0;

	regmap_read(dev->regmap, MAX30205_REG_CONFIG, &config);

	/* OS flag is bit 0 of config register when in comparator mode */
	return (config & 0x01);
}

void max30205_clear_os_flag(struct max30205_device *dev)
{
	/* OS flag is cleared by reading the temperature register */
	u8 temp_data[2];
	regmap_bulk_read(dev->regmap, MAX30205_REG_TEMP, temp_data, 2);
}

static const struct regmap_range max30205_volatile_ranges[] = {
	regmap_reg_range(0x00, 0x00),  /* Temperature register */
};

static const struct regmap_access_table max30205_volatile_table = {
	.yes_ranges = max30205_volatile_ranges,
	.n_yes_ranges = ARRAY_SIZE(max30205_volatile_ranges),
};

static const struct regmap_config max30205_regmap_config = {
	.reg_bits = 8,
	.val_bits = 16,  /* Temperature is 16-bit */
	.max_register = MAX30205_REG_TOS,
	.volatile_table = &max30205_volatile_table,
	.cache_type = REGCACHE_NONE,
};

/* Box-Muller transform for Gaussian noise */
static float max30205_gaussian_noise(float mean, float std_dev)
{
	static u32 use_last = 0;
	static float z2, z1;
	float u1, u2, z0;

	if (!use_last) {
		u1 = (float)get_random_u32() / (float)U32_MAX;
		u2 = (float)get_random_u32() / (float)U32_MAX;

		/* Ensure u1 is not zero */
		if (u1 == 0.0f)
			u1 = 0.0001f;

		z0 = sqrtf(-2.0f * logf(u1)) * sinf(2.0f * M_PI * u2);
		z2 = sqrtf(-2.0f * logf(u1)) * cosf(2.0f * M_PI * u2);

		z1 = z0;
		use_last = 1;
	} else {
		z1 = z2;
		use_last = 0;
	}

	return z1 * std_dev + mean;
}

static void *max30205_reg_random_thread(void *arg)
{
	struct max30205_device *dev = arg;
	u16 temp_raw;
	float temperature;
	u8 config, conv_bits;
	u32 update_period_ms;
	bool one_shot_triggered;

	/* Initialize fixed registers at thread startup */
	dev->slave_regs[MAX30205_REG_CONFIG] = 0x00;  /* Normal mode */

	one_shot_triggered = false;

	while (dev->thread_running) {
		/* Read CONFIG register */
		config = dev->slave_regs[MAX30205_REG_CONFIG];

		/* Check shutdown bit (bit 7) */
		if (config & 0x80) {
			/* Shutdown mode - no temperature updates */
			msleep(10);
			continue;
		}

		/* Check ONE_SHOT bit (bit 7 of CONFIG when not in shutdown) */
		/* In MAX30205, when not in shutdown, bit 7 is reserved */
		/* We'll check a simulated one-shot trigger */
		if (one_shot_triggered) {
			/* One-shot conversion complete */
			one_shot_triggered = false;

			/* Generate temperature data: ~25°C with noise */
			temperature = max30205_gaussian_noise(25.0f, 0.5f);

			/* Clamp to valid range (-55°C to +125°C) */
			if (temperature < -55.0f)
				temperature = -55.0f;
			if (temperature > 125.0f)
				temperature = 125.0f;

			/* Convert to raw units (16-bit, 0.00390625°C/LSB) */
			temp_raw = (u16)(temperature / 0.00390625f);

			/* Write temperature data to slave registers (big-endian) */
			dev->slave_regs[MAX30205_REG_TEMP] = (u8)((temp_raw >> 8) & 0xFF);
			dev->slave_regs[MAX30205_REG_TEMP + 1] = (u8)(temp_raw & 0xFF);
		} else {
			/* Continuous mode - check conversion rate */
			conv_bits = config & 0x07;  /* CONV_BITS[2:0] */

			/* Calculate update period based on conversion rate */
			switch (conv_bits) {
			case 0x00:  /* 0.5Hz */
				update_period_ms = 2000;
				break;
			case 0x01:  /* 1Hz */
				update_period_ms = 1000;
				break;
			case 0x02:  /* 2Hz */
				update_period_ms = 500;
				break;
			case 0x03:  /* 4Hz */
				update_period_ms = 250;
				break;
			default:     /* Default to 1Hz */
				update_period_ms = 1000;
				break;
			}

			/* Generate temperature data: ~25°C with noise */
			temperature = max30205_gaussian_noise(25.0f, 0.5f);

			/* Clamp to valid range (-55°C to +125°C) */
			if (temperature < -55.0f)
				temperature = -55.0f;
			if (temperature > 125.0f)
				temperature = 125.0f;

			/* Convert to raw units (16-bit, 0.00390625°C/LSB) */
			temp_raw = (u16)(temperature / 0.00390625f);

			/* Write temperature data to slave registers (big-endian) */
			dev->slave_regs[MAX30205_REG_TEMP] = (u8)((temp_raw >> 8) & 0xFF);
			dev->slave_regs[MAX30205_REG_TEMP + 1] = (u8)(temp_raw & 0xFF);

			/* Sleep based on conversion rate */
			msleep(update_period_ms);
		}
	}

	return 0;
}

static pthread_t reg_thread;

static int max30205_start_reg_random(struct max30205_device *dev)
{
	int ret;

	dev->thread_running = true;

	ret = pthread_create(&reg_thread, NULL,
			max30205_reg_random_thread, dev);
	if (ret) {
		return -ret;
	}

	/* Wait for thread to initialize */
	msleep(100);

	return 0;
}

static void max30205_stop_reg_random(struct max30205_device *dev)
{
	dev->thread_running = false;
	pthread_join(reg_thread, NULL);
}

struct max30205_device *max30205_init(enum regmap_user_bus_type bus_type, void *bus_master)
{
	struct max30205_device *dev;
	u32 config = 0;
	u16 temp_raw;
	int ret;

	dev = kzalloc(sizeof(*dev), GFP_KERNEL);
	if (!dev) {
		return ERR_PTR(-ENOMEM);
	}

	if (bus_type == REGMAP_BUS_IIC_SOFT) {
		dev->regmap = regmap_init_iic_soft(bus_master, MAX30205_I2C_ADDR, &max30205_regmap_config);
	} else if (bus_type == REGMAP_BUS_SPI_SOFT) {
		/* Default SPI attributes: CPOL=0, CPHA=0, MSB first */
		dev->regmap = regmap_init_spi_soft(bus_master,
						   0, 0, SPI_BIT_ORDER_MSB, 2, &max30205_regmap_config);
	} else {
		pr_err("Invalid bus type specified for max30205 initialization");
		return ERR_PTR(-EINVAL);
	}
	if (IS_ERR(dev->regmap)) {
		return NULL;
	}

	dev->slave_regs = kmalloc(max30205_regmap_config.max_register + 1, GFP_KERNEL);
	if (!dev->slave_regs) {
		pr_err("Failed to allocate register memory");
		return ERR_PTR(-ENOMEM);
	}

	/* Initialize simulated registers with default values */
	/* Temperature: ~25°C = 6400 in raw units (16-bit, big-endian) */
	temp_raw = 0x1900;
	dev->slave_regs[MAX30205_REG_TEMP] = (u8)((temp_raw >> 8) & 0xFF);     /* High byte */
	dev->slave_regs[MAX30205_REG_TEMP + 1] = (u8)(temp_raw & 0xFF);        /* Low byte */
	dev->slave_regs[MAX30205_REG_CONFIG] = 0x00;                          /* Normal mode */

	/* THYST: 20°C = 5120 in raw units (big-endian) */
	temp_raw = 0x1400;
	dev->slave_regs[MAX30205_REG_THYST] = (u8)((temp_raw >> 8) & 0xFF);
	dev->slave_regs[MAX30205_REG_THYST + 1] = (u8)(temp_raw & 0xFF);

	/* TOS: 30°C = 7680 in raw units (big-endian) */
	temp_raw = 0x1E00;
	dev->slave_regs[MAX30205_REG_TOS] = (u8)((temp_raw >> 8) & 0xFF);
	dev->slave_regs[MAX30205_REG_TOS + 1] = (u8)(temp_raw & 0xFF);

	ret = max30205_start_reg_random(dev);
	if (ret) {
		return NULL;
	}

	/* Initialize slave for testing (MUST be done before thread starts) */
	if (bus_type == REGMAP_BUS_IIC_SOFT) {
		dev->iic_slave = iic_slave_soft_init("max30205", 0x48,
				dev->slave_regs, max30205_regmap_config.max_register + 1);
		if (IS_ERR(dev->iic_slave)) {
			pr_err("Failed to initialize I2C slave");
			kfree(dev->slave_regs);
			dev->slave_regs = NULL;
			return NULL;
		}
	} else {
		dev->spi_slave = spi_slave_soft_init("max30205", 0, 0, SPI_BIT_ORDER_MSB,
				dev->slave_regs, max30205_regmap_config.max_register + 1);
		if (IS_ERR(dev->spi_slave)) {
			pr_err("Failed to initialize SPI slave");
			kfree(dev->slave_regs);
			dev->slave_regs = NULL;
			return NULL;
		}
	}

	pr_info("Initial temperature = %.2f°C",
		((float)(dev->slave_regs[MAX30205_REG_TEMP] << 8 | dev->slave_regs[MAX30205_REG_TEMP + 1])) * 0.00390625f);

	dev->device_present = max30205_is_device_present(dev);
	if (!dev->device_present) {
		pr_err("max30205 device not found!");
		return  ERR_PTR(-ENODEV);
	}

	/* Read current config to preserve settings */
	regmap_read(dev->regmap, MAX30205_REG_CONFIG, &config);
	config |= MAX30205_CONFIG_SHUTDOWN_BIT; /* Start in shutdown mode */
	regmap_write(dev->regmap, MAX30205_REG_CONFIG, config);

	mdelay(10); /* Wait for configuration to take effect */

	/* Set default configuration */
	max30205_set_operating_mode(dev, MAX30205_MODE_COMPARATOR);
	max30205_set_os_polarity(dev, 0); /* Active low OS pin */
	max30205_set_fault_queue(dev, MAX30205_FAULT_QUEUE_1);
	max30205_set_data_format(dev, 0); /* Normal data format */
	max30205_enable_timeout(dev, 1);

	max30205_set_shutdown_mode(dev, 0);

	return dev;
}

void max30205_exit(struct max30205_device *dev)
{
	max30205_stop_reg_random(dev);

	if (dev->iic_slave)
		iic_slave_soft_exit(dev->iic_slave);
	if (dev->spi_slave)
		spi_slave_soft_exit(dev->spi_slave);
	regmap_exit(dev->regmap);
	kfree(dev->slave_regs);
	kfree(dev);
}

#ifdef DESIGN_VERIFICATION_MAX30205
#include "kinetis/test-kinetis.h"

static struct max30205_device *max30205_dev;

int t_max30205_initialize(int argc, char **argv)
{
	enum regmap_user_bus_type bus_type = REGMAP_BUS_IIC_SOFT;
	bool on_off = true;
	int ret;

	if (argc > 1) {
		if (!strcmp(argv[1], "on")) {
			on_off = true;
		} else if (!strcmp(argv[1], "off")) {
			on_off = false;
		} else {
			return -EINVAL;
		}
	}

	if (argc > 2) {
		if (!strcmp(argv[2], "spi")) {
			bus_type = REGMAP_BUS_SPI_SOFT;
		} else if (!strcmp(argv[2], "i2c")) {
			bus_type = REGMAP_BUS_IIC_SOFT;
		} else {
			pr_err("Invalid bus type: %s (use 'i2c' or 'spi')", argv[2]);
			return -EINVAL;
		}
	}

	if (on_off) {
		pr_info("starting max30205 slave with %s mode", bus_type ? "spi" : "i2c");
		max30205_dev = max30205_init(bus_type, bus_type == REGMAP_BUS_IIC_SOFT ? (void *)&fake_iic_master : (void *)&fake_spi_master);
		if (IS_ERR_OR_NULL(max30205_dev)) {
			return -EINVAL;
		}
		return 0;
	}

	max30205_exit(max30205_dev);
	return 0;
}

int t_max30205_device_id(int argc, char **argv)
{
	struct max30205_device *dev = max30205_dev;

	/* Check if device is present */
	if (!max30205_is_device_present(dev)) {
		pr_err("max30205 device not found");
		return -ENODEV;
	}

	pr_info("max30205 device detected successfully");
	return 0;
}

int t_max30205_temperature_single(int argc, char **argv)
{
	struct max30205_device *dev = max30205_dev;
	float temperature;
	u16 raw_temp;

	max30205_set_shutdown_mode(dev, 0);
	mdelay(10);

	max30205_trigger_one_shot(dev);
	mdelay(100);

	temperature = max30205_get_temperature(dev);
	pr_info("temperature: %.2f°c", temperature);

	raw_temp = max30205_get_raw_temperature(dev);
	pr_info("raw temperature: 0x%04x (%.2f°c)", raw_temp,
		MAX30205_RAW_TO_CELSIUS(raw_temp));

	if (temperature < -20.0f || temperature > 100.0f) {
		pr_warn("temperature out of expected range");
	}

	max30205_set_shutdown_mode(dev, 1);

	pr_info("single-shot temperature reading completed");

	return 0;
}

int t_max30205_temperature_continuous(int argc, char **argv)
{
	struct max30205_device *dev = max30205_dev;
	float temperature;
	u16 readings = 100;
	u16 i;
	float temp_min = 1000.0f, temp_max = -1000.0f, temp_sum = 0.0f;

	if (argc > 1) {
		readings = simple_strtoul(argv[1], &argv[1], 10);
		if (readings > 1000) {
			readings = 1000;
		}
	}

	/* Exit shutdown mode and enter continuous mode */
	max30205_set_shutdown_mode(dev, 0);
	max30205_set_operating_mode(dev, MAX30205_MODE_INTERRUPT);
	mdelay(10);

	for (i = 0; i < readings; i++) {
		temperature = max30205_get_temperature(dev);

		if (temperature < temp_min) {
			temp_min = temperature;
		}
		if (temperature > temp_max) {
			temp_max = temperature;
		}
		temp_sum += temperature;

		pr_info("Reading %d/%d: %.2f°C", i + 1, readings, temperature);

		mdelay(10);
	}

	pr_info("temperature statistics:");
	pr_info("  min: %.2f°c", temp_min);
	pr_info("  max: %.2f°c", temp_max);
	pr_info("  avg: %.2f°c", temp_sum / readings);

	max30205_set_shutdown_mode(dev, 1);

	pr_info("continuous temperature monitoring completed");
	return 0;
}

int t_max30205_threshold_test(int argc, char **argv)
{
	struct max30205_device *dev = max30205_dev;
	u16 tos_raw, thyst_raw;
	float temp;

	max30205_set_shutdown_mode(dev, 0);
	mdelay(10);

	/* Set high temperature threshold to 30°C */
	max30205_set_threshold_high(dev, MAX30205_CELSIUS_TO_RAW(30.0f));
	tos_raw = max30205_get_threshold_high(dev);
	pr_info("high threshold set: 30.00°c (raw: 0x%04x)", tos_raw);

	/* Set low temperature threshold to 25°C */
	max30205_set_threshold_low(dev, MAX30205_CELSIUS_TO_RAW(25.0f));
	thyst_raw = max30205_get_threshold_low(dev);
	pr_info("low threshold set: 25.00°c (raw: 0x%04x)", thyst_raw);

	/* Read temperature to check against thresholds */
	temp = max30205_get_temperature(dev);
	pr_info("current temperature: %.2f°c", temp);

	max30205_set_shutdown_mode(dev, 1);

	pr_info("threshold configuration completed");
	return 0;
}

int t_max30205_config_test(int argc, char **argv)
{
	struct max30205_device *dev = max30205_dev;

	max30205_set_shutdown_mode(dev, 0);
	mdelay(10);

	max30205_set_shutdown_mode(dev, 1);
	mdelay(10);

	max30205_set_shutdown_mode(dev, 0);
	mdelay(10);

	pr_info("testing operating mode...");
	max30205_set_operating_mode(dev, MAX30205_MODE_COMPARATOR);
	mdelay(10);

	max30205_set_operating_mode(dev, MAX30205_MODE_INTERRUPT);
	mdelay(10);

	pr_info("testing os polarity...");
	max30205_set_os_polarity(dev, 0);
	mdelay(10);

	max30205_set_os_polarity(dev, 1);
	mdelay(10);

	pr_info("testing fault queue...");
	max30205_set_fault_queue(dev, MAX30205_FAULT_QUEUE_1);
	mdelay(10);

	max30205_set_fault_queue(dev, MAX30205_FAULT_QUEUE_6);
	mdelay(10);

	pr_info("configuration test completed");
	return 0;
}

int t_max30205_os_flag_test(int argc, char **argv)
{
	struct max30205_device *dev = max30205_dev;
	u8 os_flag;
	float temp;

	max30205_set_shutdown_mode(dev, 0);
	mdelay(10);

	os_flag = max30205_check_os_flag(dev);
	pr_info("OS flag status: %d", os_flag);

	max30205_clear_os_flag(dev);
	mdelay(10);

	os_flag = max30205_check_os_flag(dev);
	pr_info("OS flag after clear: %d", os_flag);

	/* Set threshold to trigger OS flag */
	max30205_set_threshold_high(dev, MAX30205_CELSIUS_TO_RAW(0.0f));
	max30205_set_threshold_low(dev, MAX30205_CELSIUS_TO_RAW(-10.0f));
	mdelay(100);

	temp = max30205_get_temperature(dev);
	pr_info("current temperature: %.2f°c", temp);

	os_flag = max30205_check_os_flag(dev);
	pr_info("OS flag after threshold change: %d", os_flag);

	max30205_clear_os_flag(dev);

	max30205_set_shutdown_mode(dev, 1);

	pr_info("OS flag test completed");
	return 0;
}

int t_max30205_timeout_test(int argc, char **argv)
{
	struct max30205_device *dev = max30205_dev;

	max30205_set_shutdown_mode(dev, 0);
	mdelay(10);

	max30205_enable_timeout(dev, 1);
	mdelay(10);

	max30205_enable_timeout(dev, 0);
	mdelay(10);

	max30205_set_shutdown_mode(dev, 1);

	pr_info("timeout test completed");
	return 0;
}

int t_max30205_oneshot_test(int argc, char **argv)
{
	struct max30205_device *dev = max30205_dev;
	u16 readings = 10;
	u16 i;
	float temperature;

	max30205_set_shutdown_mode(dev, 1);
	mdelay(10);

	for (i = 0; i < readings; i++) {
		max30205_trigger_one_shot(dev);

		/* Wait for conversion to complete */
		mdelay(100);

		temperature = max30205_get_temperature(dev);

		pr_info("reading %d/%d: %.2f°c", i + 1, readings, temperature);
	}

	pr_info("one-shot mode test completed");
	return 0;
}

int t_max30205_calibration_test(int argc, char **argv)
{
	struct max30205_device *dev = max30205_dev;
	float temp_normal, temp_calibrated;

	max30205_set_shutdown_mode(dev, 0);
	max30205_set_operating_mode(dev, MAX30205_MODE_INTERRUPT);
	mdelay(10);

	/* Read normal temperature */
	temp_normal = max30205_get_temperature(dev);
	pr_info("normal temperature: %.2f°c", temp_normal);

	/* Set calibration offset */
	dev->temperature_offset = 0.5f;

	/* Read calibrated temperature */
	temp_calibrated = max30205_get_temperature_with_calibration(dev);
	pr_info("calibrated temperature: %.2f°c (offset +0.5°c)", temp_calibrated);

	/* Reset calibration */
	dev->temperature_offset = 0.0f;

	max30205_set_shutdown_mode(dev, 1);

	pr_info("calibration test completed");
	return 0;
}

int t_max30205_range_test(int argc, char **argv)
{
	struct max30205_device *dev = max30205_dev;
	float temperature;
	u16 readings = 50;
	u16 i;
	float prev_temp = 0.0f;
	float max_delta = 0.0f;

	max30205_set_shutdown_mode(dev, 0);
	max30205_set_operating_mode(dev, MAX30205_MODE_INTERRUPT);
	mdelay(10);

	for (i = 0; i < readings; i++) {
		temperature = max30205_get_temperature(dev);

		/* Calculate temperature change */
		if (i > 0) {
			float delta = temperature - prev_temp;
			if (delta < 0) {
				delta = -delta;
			}
			if (delta > max_delta) {
				max_delta = delta;
			}
		}

		prev_temp = temperature;

		mdelay(20);
	}

	pr_info("maximum temperature delta: %.2f°c", max_delta);

	if (max_delta > 2.0f) {
		pr_warn("temperature readings vary significantly");
	}

	max30205_set_shutdown_mode(dev, 1);

	pr_info("temperature range test completed");
	return 0;
}

#endif
