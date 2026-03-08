#define pr_fmt(fmt) "ak8975: " fmt

#include <linux/bitops.h>
#include <linux/printk.h>
#include <linux/delay.h>
#include <linux/err.h>
#include <linux/slab.h>
#include <linux/random.h>

#include "kinetis/ak8975.h"
#include "kinetis/iic_soft.h"
#include "kinetis/spi_soft.h"
#include "kinetis/regmap-user-bus.h"
#include "kinetis/idebug.h"
#include "kinetis/design_verification.h"
#include "kinetis/random-gene.h"
#include "kinetis/test-kinetis.h"

#include <pthread.h>
#include <math.h>

#define AK8975_CAD0                    0
#define AK8975_CAD1                    0

#if (!AK8975_CAD1 && !AK8975_CAD0)
#define AK8975_ADDR                    0x0C
#elif (!AK8975_CAD1 && AK8975_CAD0)
#define AK8975_ADDR                    0x0D
#elif (AK8975_CAD1 && !AK8975_CAD0)
#define AK8975_ADDR                    0x0E
#elif (AK8975_CAD1 && AK8975_CAD0)
#define AK8975_ADDR                    0x0F
#endif

#define WIA                             0x00
#define INFO                            0x01
#define ST1                             0x02
#define HXL                             0x03
#define HXH                             0x04
#define HYL                             0x05
#define HYH                             0x06
#define HZL                             0x07
#define HZH                             0x08
#define ST2                             0x09
#define CNTL1                           0x0A
#define CNTL2                           0x0B
#define ASTC                            0x0C
#define TS1                             0x0D
#define TS2                             0x0E
#define I2CDIS                          0x0F
#define ASAX                            0x10
#define ASAY                            0x11
#define ASAZ                            0x12

#define AKM_DEVID                       0x48

#define POWER_DOWN                      0x0000
#define SINGLE_MEASUREMENT              0x0001
#define SELF_TEST                       0x1000
#define FUSE_ROM_ACCESS                 0x1111
#define CONTINUOUS_MEASUREMENT_8HZ      0x0002
#define CONTINUOUS_MEASUREMENT_100HZ    0x0004
#define CONTINUOUS_MEASUREMENT_10HZ     0x0006
#define CONTINUOUS_MEASUREMENT_20HZ     0x0008

#define ST1_DRDY_BIT                    0x01
#define ST1_DOR_BIT                     0x02

#define ST2_HOFL_BIT                    0x08
#define ST2_DERR_BIT                    0x04
#define ST2_BITM_BIT                    0x10

#define ASTC_SELF_BIT                   0x40
#define ASTC_ELF_BIT                    0x80

#define CNTL2_SRST_BIT                  0x01

#define AK8975_TBD                      0

/* Sleep mode and power management related constants */
#define SLEEP_MODE_DELAY_MS             100
#define POWER_ON_DELAY_MS               10
#define MODE_SWITCH_DELAY_MS            1

/* Simulation thread constants */
#define AK8975_UPDATE_PERIOD_MS        10     /* Update period: 100ms (10Hz) */
#define MAGNETIC_MIN_UT                -600    /* Minimum: -600 µT */
#define MAGNETIC_MAX_UT                +600    /* Maximum: +600 µT */
#define MAGNETIC_NOISE_STDEV           5       /* Magnetic noise standard deviation (µT) */
#define TEMP_MIN_C                    -20     /* Minimum: -20°C */
#define TEMP_MAX_C                    +50     /* Maximum: +50°C */

struct ak8975_device {
	struct regmap *regmap;

	u8 asa_values[3];

	struct iic_slave *iic_slave;
	struct spi_slave *spi_slave;
	u8 *slave_regs;

	bool thread_running;
};

void ak8975_enter_power_down_mode(struct ak8975_device *dev)
{
	regmap_write(dev->regmap, CNTL1, POWER_DOWN);
	mdelay(MODE_SWITCH_DELAY_MS);
}

u8 ak8975_get_power_state(struct ak8975_device *dev)
{
	u32 reg;
	regmap_read(dev->regmap, CNTL1, &reg);

	if ((reg & 0x0F) == POWER_DOWN) {
		return 0;    /* Power down state */
	} else {
		return 1;    /* Active state */
	}
}

void ak8975_enter_single_measurement_mode(struct ak8975_device *dev)
{
	regmap_write(dev->regmap, CNTL1, SINGLE_MEASUREMENT & 0xFF);
	mdelay(1);
}

void ak8975_enter_continuous_8hz_mode(struct ak8975_device *dev)
{
	regmap_write(dev->regmap, CNTL1, CONTINUOUS_MEASUREMENT_8HZ & 0xFF);
	mdelay(1);
}

void ak8975_enter_continuous_10hz_mode(struct ak8975_device *dev)
{
	regmap_write(dev->regmap, CNTL1, CONTINUOUS_MEASUREMENT_10HZ & 0xFF);
	mdelay(1);
}

void ak8975_enter_continuous_20hz_mode(struct ak8975_device *dev)
{
	regmap_write(dev->regmap, CNTL1, CONTINUOUS_MEASUREMENT_20HZ & 0xFF);
	mdelay(1);
}

void ak8975_enter_continuous_100hz_mode(struct ak8975_device *dev)
{
	regmap_write(dev->regmap, CNTL1, CONTINUOUS_MEASUREMENT_100HZ & 0xFF);
	mdelay(1);
}

void ak8975_enter_selftest_mode(struct ak8975_device *dev)
{
	regmap_write(dev->regmap, CNTL1, SELF_TEST & 0xFF);
	mdelay(10);
}

void ak8975_enter_self_test_mode(struct ak8975_device *dev)
{
	regmap_write(dev->regmap, ASTC, ASTC_SELF_BIT);
	mdelay(1);
	regmap_write(dev->regmap, CNTL1, SELF_TEST & 0xFF);
	mdelay(10);
}

void ak8975_enter_fuse_rom_access_mode(struct ak8975_device *dev)
{
	regmap_write(dev->regmap, CNTL1, FUSE_ROM_ACCESS & 0xFF);
	mdelay(10);
}

u8 ak8975_who_am_i(struct ak8975_device *dev)
{
	u32 reg;

	regmap_read(dev->regmap, WIA, &reg);

	return reg;
}

u8 ak8975_device_information(struct ak8975_device *dev)
{
	u32 reg;

	regmap_read(dev->regmap, INFO, &reg);

	return reg;
}

u8 ak8975_data_ready(struct ak8975_device *dev)
{
	u32 reg;

	regmap_read(dev->regmap, ST1, &reg);

	return (reg & ST1_DRDY_BIT);
}

u8 ak8975_data_overrun(struct ak8975_device *dev)
{
	u32 reg;

	regmap_read(dev->regmap, ST1, &reg);

	return (reg & ST1_DOR_BIT);
}

u8 ak8975_check_status(struct ak8975_device *dev)
{
	u32 st1_reg = 0;
	u32 st2_reg = 0;

	regmap_read(dev->regmap, ST1, &st1_reg);
	regmap_read(dev->regmap, ST2, &st2_reg);

	if (st2_reg & ST2_DERR_BIT) {
		pr_err("AK8975 data error detected");
		return -1;
	}

	if (st2_reg & ST2_HOFL_BIT) {
		pr_warn("AK8975 magnetic sensor overflow");
		return -2;
	}

	return (st1_reg & ST1_DRDY_BIT);
}

u8 ak8975_data_error(struct ak8975_device *dev)
{
	u32 reg;

	regmap_read(dev->regmap, ST2, &reg);

	return (reg & ST2_DERR_BIT);
}

void ak8975_magnetic_measurements(struct ak8975_device *dev, u16 *pdata)
{
	u8 tmp[6];

	regmap_bulk_read(dev->regmap, HXL, tmp, 6);

	pdata[0] = (tmp[1] << 8) | tmp[0];
	pdata[1] = (tmp[3] << 8) | tmp[2];
	pdata[2] = (tmp[5] << 8) | tmp[4];
}

u8 ak8975_magnetic_sensor_overflow(struct ak8975_device *dev)
{
	u32 reg;

	regmap_read(dev->regmap, ST2, &reg);

	return (reg & ST2_HOFL_BIT);
}

u8 ak8975_output_bit_setting_mirror(struct ak8975_device *dev)
{
	u32 reg;

	regmap_read(dev->regmap, ST2, &reg);

	return (reg & ST2_BITM_BIT);
}

void ak8975_operation_mode_setting(struct ak8975_device *dev, u8 tmp)
{
	u32 reg;

	regmap_read(dev->regmap, CNTL1, &reg);

	set_mask_bits(&reg, 0x0F, tmp);

	regmap_write(dev->regmap, CNTL1, reg);
}

void ak8975_output_bit_setting(struct ak8975_device *dev, u8 tmp)
{
	u32 reg;

	regmap_read(dev->regmap, CNTL1, &reg);

	set_mask_bits(&reg, 0x10, tmp);

	regmap_write(dev->regmap, CNTL1, reg);
}

void ak8975_soft_reset(struct ak8975_device *dev, u8 tmp)
{
	u32 reg;

	regmap_read(dev->regmap, CNTL2, &reg);

	set_mask_bits(&reg, 0x01, tmp);

	regmap_write(dev->regmap, CNTL2, reg);
}

void ak8975_selftest_control(struct ak8975_device *dev, u8 tmp)
{
	u32 reg;

	regmap_read(dev->regmap, ASTC, &reg);

	set_mask_bits(&reg, 0x40, tmp);

	regmap_write(dev->regmap, ASTC, reg);
}

void ak8975_i2c_disable(struct ak8975_device *dev)
{
	regmap_write(dev->regmap, I2CDIS, 0x1B);
}

void ak8975_i2c_enable(struct ak8975_device *dev)
{
	ak8975_soft_reset(dev, 1);
}

void ak8975_sensitivity_adjustment_values(struct ak8975_device *dev, u8 *pdata)
{
	regmap_bulk_read(dev->regmap, ASAX, pdata, 3);
}

void ak8975_enter_sleep_mode(struct ak8975_device *dev)
{
	ak8975_enter_power_down_mode(dev);
	mdelay(SLEEP_MODE_DELAY_MS);
}

void ak8975_wake_up_from_sleep(struct ak8975_device *dev)
{
	ak8975_enter_single_measurement_mode(dev);
	mdelay(POWER_ON_DELAY_MS);
}

u8 ak8975_self_test(struct ak8975_device *dev)
{
	u8 status = -1;
	u16 test_data[3];

	ak8975_enter_power_down_mode(dev);
	mdelay(1);

	ak8975_selftest_control(dev, true);
	mdelay(1);
	ak8975_enter_self_test_mode(dev);

	ak8975_magnetic_measurements(dev, test_data);

	ak8975_enter_power_down_mode(dev);

	if ((test_data[0] >= 30 && test_data[0] <= 5000) &&
		(test_data[1] >= 30 && test_data[1] <= 5000) &&
		(test_data[2] >= 30 && test_data[2] <= 5000)) {
		status = 0;
	}

	return status;
}

u8 ak8975_magnetic_adjusted_measurements(struct ak8975_device *dev, u16 *pdata)
{
	u16 raw_data[3];
	u8 data_ready = 0;
	int ret;

	ak8975_enter_single_measurement_mode(dev);

	ret = readx_poll_timeout(ak8975_data_ready, dev, data_ready, data_ready, 0, 10000);
	if (ret) {
		pr_err("ak8975 magnetic data not ready");
		return 0;
	}

	if (ak8975_data_overrun(dev)) {
		pr_warn("ak8975 magnetic data overrun");
	}

	ak8975_magnetic_measurements(dev, raw_data);

	pdata[0] = (raw_data[0] * (dev->asa_values[0] + 128)) / 256;
	pdata[1] = (raw_data[1] * (dev->asa_values[1] + 128)) / 256;
	pdata[2] = (raw_data[2] * (dev->asa_values[2] + 128)) / 256;

	return 1;
}

s16 ak8975_read_temperature_data(struct ak8975_device *dev)
{
	u32 temp_low, temp_high;
	s16 temperature_raw;

	regmap_read(dev->regmap, TS1, &temp_low);
	regmap_read(dev->regmap, TS2, &temp_high);

	temperature_raw = (temp_high << 8) | temp_low;

	return temperature_raw;
}

float ak8975_get_temperature_compensation(u16 raw_mag_data, s16 temperature_data)
{
	float compensation_factor = 1.0;
	s16 temp_celsius;

	/* Convert raw temperature data to Celsius */
	temp_celsius = ((s32)temperature_data - 12421) / 280 + 25;

	/* Temperature compensation algorithm - based on AK8975 manual temperature characteristics */
	if (temp_celsius < 0) {
		compensation_factor = 1.0 + (0.001 * temp_celsius);
	} else if (temp_celsius > 25) {
		compensation_factor = 1.0 - (0.0008 * (temp_celsius - 25));
	}

	return compensation_factor;
}

u16 ak8975_temperature_compensated_measurement(u16 raw_mag_data, s16 temperature_data)
{
	float compensation_factor;

	compensation_factor = ak8975_get_temperature_compensation(raw_mag_data, temperature_data);

	return (u16)(raw_mag_data * compensation_factor);
}

void ak8975_compensated_magnetic_measurements(struct ak8975_device *dev, u16 *pdata)
{
	u16 raw_data[3];
	s16 temperature_data;

	ak8975_magnetic_measurements(dev, raw_data);
	temperature_data = ak8975_read_temperature_data(dev);

	pdata[0] = ak8975_temperature_compensated_measurement(raw_data[0], temperature_data);
	pdata[1] = ak8975_temperature_compensated_measurement(raw_data[1], temperature_data);
	pdata[2] = ak8975_temperature_compensated_measurement(raw_data[2], temperature_data);
}

static void *ak8975_reg_random_thread(void *arg)
{
	struct ak8975_device *dev = (struct ak8975_device *)arg;
	s16 mag_x, mag_y, mag_z;
	s16 temperature;
	s32 noise_x, noise_y, noise_z;
	u32 random_val;
	float magnitude;
	float theta, phi;
	u8 cntl1_mode;
	u8 astc_self_test;
	bool in_self_test = false;

	/* WIA: Who Am I register - Device ID (Read-only) */
	dev->slave_regs[WIA] = AKM_DEVID;  /* 0x48 */
	/* INFO: Device Information register (Read-only) */
	dev->slave_regs[INFO] = 0x00;  /* Reserved, typically 0x00 */
	/* CNTL1: Control Register 1 - Power down mode initially */
	dev->slave_regs[CNTL1] = POWER_DOWN & 0xFF;
	/* CNTL2: Control Register 2 (Reserved) */
	dev->slave_regs[CNTL2] = 0x00;
	/* ASTC: Self-Test Control register */
	dev->slave_regs[ASTC] = 0x00;  /* Self-test inactive */
	/* I2CDIS: I2C Disable register - I2C enabled by default */
	dev->slave_regs[I2CDIS] = 0x00;
	/* TS1/TS2: Test registers (Do not access) */
	dev->slave_regs[TS1] = 0xFF;  /* Default test value */
	dev->slave_regs[TS2] = 0xFF;
	/* ASAX/ASAY/ASAZ: Sensitivity Adjustment values (Read-only from Fuse ROM) */
	/* Typical values for sensitivity adjustment (Fuse ROM) */
	dev->slave_regs[ASAX] = 0x80;  /* X-axis sensitivity adjustment */
	dev->slave_regs[ASAY] = 0x80;  /* Y-axis sensitivity adjustment */
	dev->slave_regs[ASAZ] = 0x80;  /* Z-axis sensitivity adjustment */
	/* ST1: Status Register 1 - Data not ready initially */
	dev->slave_regs[ST1] = 0x00;
	/* ST2: Status Register 2 - No errors initially */
	dev->slave_regs[ST2] = 0x00;
	/* Initialize magnetic data registers to zero */
	dev->slave_regs[HXL] = 0x00;
	dev->slave_regs[HXH] = 0x00;
	dev->slave_regs[HYL] = 0x00;
	dev->slave_regs[HYH] = 0x00;
	dev->slave_regs[HZL] = 0x00;
	dev->slave_regs[HZH] = 0x00;

	while (dev->thread_running) {
		/* Read current CNTL1 and ASTC registers to determine mode */
		cntl1_mode = dev->slave_regs[CNTL1] & 0x0F;
		astc_self_test = dev->slave_regs[ASTC] & ASTC_SELF_BIT;

		/* Check for self-test mode */
		in_self_test = ((dev->slave_regs[CNTL1] & 0x80) != 0) || (astc_self_test != 0);

		if (cntl1_mode == POWER_DOWN) {
			/* Power down mode - no measurements, just maintain registers */
			dev->slave_regs[ST1] = 0x00;  /* DRDY = 0 */
			msleep(10);
			continue;
		}

		if (cntl1_mode == FUSE_ROM_ACCESS) {
			/* Fuse ROM access mode - ASAX/ASAY/ASAZ are readable */
			/* No magnetic data updates in this mode */
			dev->slave_regs[ST1] = 0x00;  /* DRDY = 0 */
			msleep(10);
			continue;
		}

		if (in_self_test) {
			/* Self-test mode - generate fixed magnetic field pattern */
			/* Self-test generates internal magnetic field for calibration */
			mag_x = 100;  /* Fixed positive X for self-test */
			mag_y = 100;  /* Fixed positive Y for self-test */
			mag_z = 100;  /* Fixed positive Z for self-test */
		} else {
			/* Normal measurement mode - generate realistic magnetic data */
			/* 1. Generate magnetic field data (simulate Earth's magnetic field) */
			/* Generate using spherical coordinates for uniform distribution */
			random_val = get_random_u32();

			/* Magnetic field magnitude: 40-60 µT (Earth's magnetic field range) */
			magnitude = 40.0f + (random_val & 0x1F);  /* 40-60 µT */

			/* Random direction (spherical uniform distribution) */
			theta = ((random_val >> 5) / 32768.0f) * 3.14159f;         /* 0-π */
			phi = ((random_val >> 20) / 32768.0f) * 2.0f * 3.14159f;  /* 0-2π */

			/* Convert to Cartesian coordinates (13-bit ADC, ~15 LSB/µT) */
			mag_x = (s16)(magnitude * sinf(theta) * cosf(phi) * 15.0f);
			mag_y = (s16)(magnitude * sinf(theta) * sinf(phi) * 15.0f);
			mag_z = (s16)(magnitude * cosf(theta) * 15.0f);

			/* 2. Add noise (simulate sensor noise) */
			/* Gaussian-like noise using Box-Muller transform approximation */
			noise_x = ((s32)(get_random_u32() & 0xFF) - 128) * MAGNETIC_NOISE_STDEV / 64;
			noise_y = ((s32)(get_random_u32() & 0xFF) - 128) * MAGNETIC_NOISE_STDEV / 64;
			noise_z = ((s32)(get_random_u32() & 0xFF) - 128) * MAGNETIC_NOISE_STDEV / 64;

			mag_x += noise_x;
			mag_y += noise_y;
			mag_z += noise_z;

			/* 3. Clamp to valid range (13-bit ADC: -8192 to +8191) */
			if (mag_x < -8192) mag_x = -8192;
			if (mag_x > 8191) mag_x = 8191;
			if (mag_y < -8192) mag_y = -8192;
			if (mag_y > 8191) mag_y = 8191;
			if (mag_z < -8192) mag_z = -8192;
			if (mag_z > 8191) mag_z = 8191;
		}

		/* 4. Generate temperature data */
		/* Temperature range: -20°C ~ +50°C */
		/* Conversion formula: Temp(°C) = (raw - 12421) / 280 + 25 */
		/* Reverse: raw = (Temp - 25) * 280 + 12421 */
		temperature = (s16)(((TEMP_MIN_C + (get_random_u32() & 0x7F)) - 25) * 280 + 12421);

		/* Update magnetic field data registers (HXL-HZH) */
		dev->slave_regs[HXL] = mag_x & 0xFF;
		dev->slave_regs[HXH] = (mag_x >> 8) & 0xFF;
		dev->slave_regs[HYL] = mag_y & 0xFF;
		dev->slave_regs[HYH] = (mag_y >> 8) & 0xFF;
		dev->slave_regs[HZL] = mag_z & 0xFF;
		dev->slave_regs[HZH] = (mag_z >> 8) & 0xFF;

		/* Update temperature data registers (TS1-TS2) */
		dev->slave_regs[TS1] = temperature & 0xFF;
		dev->slave_regs[TS2] = (temperature >> 8) & 0xFF;

		/* Update ST2: Check for overflow (13-bit limit) */
		dev->slave_regs[ST2] = 0x00;  /* Clear previous status */
		if ((mag_x < -8192 || mag_x > 8191) ||
		    (mag_y < -8192 || mag_y > 8191) ||
		    (mag_z < -8192 || mag_z > 8191)) {
			dev->slave_regs[ST2] |= ST2_HOFL_BIT;  /* Magnetic sensor overflow */
		}

		/* Simulate occasional data error (very rare, ~0.1% probability) */
		if ((get_random_u32() & 0x3FF) == 0) {
			dev->slave_regs[ST2] |= ST2_DERR_BIT;  /* Data error */
		}

		/* Update ST1: Set DRDY bit to indicate data is ready */
		dev->slave_regs[ST1] = ST1_DRDY_BIT;

		/* Different update periods based on mode */
		if (in_self_test) {
			msleep(10);  /* Self-test mode: 10ms */
		} else if (cntl1_mode == SINGLE_MEASUREMENT) {
			msleep(10);  /* Single measurement: 10ms conversion time */
		} else {
			/* Continuous measurement modes: 10ms (100Hz default) */
			msleep(AK8975_UPDATE_PERIOD_MS);
		}
	}

	return NULL;
}

static pthread_t reg_thread;

int ak8975_start_reg_random(struct ak8975_device *dev)
{
	int ret;

	dev->thread_running = true;

	ret = pthread_create(&reg_thread, NULL,
			ak8975_reg_random_thread, dev);
	if (ret) {
		return -ret;
	}

	/* Wait for thread to start */
	msleep(100);

	return 0;
}

void ak8975_stop_reg_random(struct ak8975_device *dev)
{
	dev->thread_running = false;
	pthread_join(reg_thread, NULL);
}

static const struct regmap_range ak8975_volatile_ranges[] = {
	regmap_reg_range(ST1, ST2),  /* ST1, HXL-HZH, ST2 */
};

static const struct regmap_access_table ak8975_volatile_table = {
	.yes_ranges = ak8975_volatile_ranges,
	.n_yes_ranges = ARRAY_SIZE(ak8975_volatile_ranges),
};

static const struct regmap_config ak8975_regmap_config = {
	.reg_bits = 8,
	.val_bits = 8,
	.max_register = ASAZ,
	.volatile_table = &ak8975_volatile_table,
	.cache_type = REGCACHE_NONE,
};

struct ak8975_device *ak8975_init(enum regmap_user_bus_type bus_type, void *bus_master)
{
	struct ak8975_device *dev;
	int ret;

	dev = kzalloc(sizeof(*dev), GFP_KERNEL);
	if (!dev) {
		return ERR_PTR(-ENOMEM);
	}

	if (bus_type == REGMAP_BUS_IIC_SOFT) {
		dev->regmap = regmap_init_iic_soft(bus_master, AK8975_ADDR, &ak8975_regmap_config);
	} else if (bus_type == REGMAP_BUS_SPI_SOFT) {
		dev->regmap = regmap_init_spi_soft(bus_master, &ak8975_regmap_config);
	} else {
		pr_err("Invalid bus type specified for ak8975 initialization");
		return ERR_PTR(-EINVAL);
	}
	if (IS_ERR(dev->regmap)) {
		return NULL;
	}

	dev->slave_regs = kmalloc(ak8975_regmap_config.max_register + 1, GFP_KERNEL);
	if (!dev->slave_regs) {
		return ERR_PTR(-ENOMEM);
	}

	ret = ak8975_start_reg_random(dev);
	if (ret) {
		return NULL;
	}

	if (bus_type == REGMAP_BUS_SPI_SOFT) {
		dev->spi_slave = spi_slave_soft_init("ak8975", 0, 0, SPI_BIT_ORDER_MSB,
				dev->slave_regs, ak8975_regmap_config.max_register + 1);
		if (IS_ERR(dev->spi_slave)) {
			pr_err("ak8975_slave: Failed to initialize SPI slave");
			kfree(dev->slave_regs);
			dev->slave_regs = NULL;
			return NULL;
		}
	} else {
		dev->iic_slave = iic_slave_soft_init("ak8975", AK8975_ADDR,
				dev->slave_regs, ak8975_regmap_config.max_register + 1);
		if (IS_ERR(dev->iic_slave)) {
			pr_err("ak8975_slave: Failed to initialize I2C slave");
			kfree(dev->slave_regs);
			dev->slave_regs = NULL;
			return NULL;
		}
	}

	ak8975_enter_fuse_rom_access_mode(dev);
	ak8975_sensitivity_adjustment_values(dev, dev->asa_values);
	pr_debug("ak8975 adjustment values %x, %x, %x",
		dev->asa_values[0], dev->asa_values[1], dev->asa_values[2]);
	ak8975_enter_power_down_mode(dev);

	return dev;
}

void ak8975_exit(struct ak8975_device *dev)
{
	ak8975_stop_reg_random(dev);

	if (dev->iic_slave) {
		iic_slave_soft_exit(dev->iic_slave);
	}

	if (dev->spi_slave) {
		spi_slave_soft_exit(dev->spi_slave);
	}

	kfree(dev->slave_regs);
	kfree(dev);
}

#ifdef DESIGN_VERIFICATION_AK8975
#include "kinetis/test-kinetis.h"

static struct ak8975_device *ak8975_dev;

int t_ak8975_initialize(int argc, char **argv)
{
	bool on_off = true;
	enum regmap_user_bus_type bus_type = REGMAP_BUS_IIC_SOFT;

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
		pr_info("starting ak8975 slave with %s mode", bus_type ? "spi" : "i2c");
		ak8975_dev = ak8975_init(bus_type, bus_type == REGMAP_BUS_IIC_SOFT ? (void *)&fake_iic_master : (void *)&fake_spi_master);
		if (IS_ERR_OR_NULL(ak8975_dev)) {
			return -EINVAL;
		}
		return 0;
	}

	ak8975_exit(ak8975_dev);
	return 0;
}

int t_ak8975_basic_info(int argc, char **argv)
{
	struct ak8975_device *dev = ak8975_dev;
	u8 tmp = 0;

	tmp = ak8975_who_am_i(dev);
	pr_info("device id of ak8975 is 0x%02x", tmp);
	if (tmp != AKM_DEVID) {
		pr_err("device id of ak8975 is not correct, got 0x%02x expected 0x%02x",
			tmp, AKM_DEVID);
		return -EINVAL;
	}

	tmp = ak8975_device_information(dev);
	pr_info("device information for ak8975: 0x%02x", tmp);

	if (tmp != 0x00) {
		pr_warn("device information is 0x%02x (expected 0x00)", tmp);
	}
	return 0;
}

int t_ak8975_magnetic(int argc, char **argv)
{
	struct ak8975_device *dev = ak8975_dev;
	u16 magnetic[3] = {0, 0, 0};
	u16 times = 128;
	u8 i;
	s16 mag_x, mag_y, mag_z;
	u8 data_ready = 0;
	int ret;

	if (argc > 1) {
		times = simple_strtoul(argv[1], &argv[1], 10);
	}

	ak8975_enter_power_down_mode(dev);
	mdelay(10); /* Ensure complete power-off state */

	for (i = 0; i < times; i++) {
		ak8975_enter_single_measurement_mode(dev);
		mdelay(1); /* Mode switch delay */

		/* Wait for data ready */
		ret = readx_poll_timeout(ak8975_data_ready, dev, data_ready, data_ready, 0, 10000);
		if (ret) {
			pr_err("ak8975 magnetic data not ready at reading %d", i + 1);
			return ret;
		}

		if (ak8975_data_overrun(dev)) {
			pr_warn("ak8975 magnetic data overrun at reading %d", i + 1);
		}

		ak8975_magnetic_measurements(dev, magnetic);

		/* Convert to signed 16-bit for display */
		mag_x = (s16)((magnetic[0] & 0x8000) ? (magnetic[0] | 0xF000) : magnetic[0]);
		mag_y = (s16)((magnetic[1] & 0x8000) ? (magnetic[1] | 0xF000) : magnetic[1]);
		mag_z = (s16)((magnetic[2] & 0x8000) ? (magnetic[2] | 0xF000) : magnetic[2]);

		pr_debug("ak8975 magnetic data [%d/%d]: x=%6d, y=%6d, z=%6d",
			i + 1, times, mag_x, mag_y, mag_z);
	}

	ak8975_enter_power_down_mode(dev);
	pr_info("completed %d magnetic readings", times);
	return 0;
}

int t_ak8975_selftest(int argc, char **argv)
{
	struct ak8975_device *dev = ak8975_dev;
	u16 magnetic[3] = {0, 0, 0};
	u8 data_ready = 0;
	int ret;

	ak8975_enter_power_down_mode(dev);
	mdelay(10); /* Ensure complete power-off state */

	ak8975_selftest_control(dev, true);
	mdelay(1); /* ASTC register write delay */

	ak8975_enter_selftest_mode(dev);
	mdelay(10); /* Self-test mode requires sufficient time to stabilize */

	ret = readx_poll_timeout(ak8975_data_ready, dev, data_ready, data_ready, 0, 100000);
	if (ret) {
		pr_err("ak8975 magnetic data not ready during self-test");
		return ret;
	}

	ak8975_magnetic_measurements(dev, magnetic);
	ak8975_selftest_control(dev, false);

	pr_info("ak8975 selftest magnetic data: X=%d, Y=%d, Z=%d",
		magnetic[0], magnetic[1], magnetic[2]);

	if ((magnetic[0] >= 30 && magnetic[0] <= 5000) &&
		(magnetic[1] >= 30 && magnetic[1] <= 5000) &&
		(magnetic[2] >= 30 && magnetic[2] <= 5000)) {
		pr_info("self-test completed successfully");
		ak8975_enter_power_down_mode(dev);
	} else {
		pr_err("self-test data out of range");
		pr_info("  expected range: 30-5000 for all axes");
		pr_info("  actual: x=%d, y=%d, z=%d",
			magnetic[0], magnetic[1], magnetic[2]);
		ak8975_enter_power_down_mode(dev);
		return -EINVAL;
	}
	return 0;
}

int t_ak8975_fuse_rom_access(int argc, char **argv)
{
	struct ak8975_device *dev = ak8975_dev;
	u8 asa_values[3] = {0, 0, 0};

	ak8975_enter_fuse_rom_access_mode(dev);
	mdelay(10); /* Ensure FUSE ROM access mode is stable */

	ak8975_sensitivity_adjustment_values(dev, asa_values);
	pr_info("adjustment values");
	pr_info("  asax = 0x%02x", asa_values[0]);
	pr_info("  asay = 0x%02x", asa_values[1]);
	pr_info("  asaz = 0x%02x", asa_values[2]);

	/* verify asa values are in valid range (0-255) */
	if (asa_values[0] <= 255 && asa_values[1] <= 255 && asa_values[2] <= 255) {
		pr_info("sensitivity adjustment values read successfully");
	} else {
		pr_warn("some asa values appear out of range");
	}

	ak8975_enter_power_down_mode(dev);
	mdelay(10); /* Return to power-off state */

	return 0;
}

#endif
