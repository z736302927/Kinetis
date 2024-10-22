// SPDX-License-Identifier: GPL-2.0
/*
 * 3-axis gyroerometer driver supporting following Bosch-Sensortec chips:
 *  - BMI088
 *
 * Copyright (c) 2018-2021, Topic Embedded Products
 */

#include <generated/deconfig.h>
#include <linux/delay.h>
#include <linux/iio/iio.h>
#include <linux/iio/sysfs.h>
#include <linux/interrupt.h>
#include <linux/module.h>
#include <linux/pm.h>
#include <linux/pm_runtime.h>
#include <linux/regmap.h>
#include <linux/slab.h>
#include <asm/unaligned.h>

#include <kinetis/fmu.h>

#include "bmi088-gyro.h"

#define BMI088_GYRO_REG_CHIP_ID			0x00

#define BMI088_GYRO_REG_XOUT_L				0x02
#define BMI088_GYRO_AXIS_TO_REG(axis) \
	(BMI088_GYRO_REG_XOUT_L + (axis * 2))

#define BMI088_GYRO_REG_INT_STATUS			0x0A
#define BMI088_GYRO_INT_STATUS_BIT_DRDY		BIT(7)

#define BMI088_GYRO_REG_GYRO_RANGE			0x0F
#define BMI088_GYRO_RANGE_2000D				0x00
#define BMI088_GYRO_RANGE_1000D				0x01
#define BMI088_GYRO_RANGE_500D				0x02
#define BMI088_GYRO_RANGE_250D				0x03
#define BMI088_GYRO_RANGE_125D				0x04

#define BMI088_GYRO_REG_BANDWIDTH			0x10
#define BMI088_GYRO_BW_532				0x00
#define BMI088_GYRO_BW_230				0x01
#define BMI088_GYRO_BW_116				0x02
#define BMI088_GYRO_BW_47				0x03
#define BMI088_GYRO_BW_23				0x04
#define BMI088_GYRO_BW_12				0x05
#define BMI088_GYRO_BW_64				0x06
#define BMI088_GYRO_BW_32				0x07

#define BMI088_GYRO_REG_LPM1			0x11
#define BMI088_GYRO_LPM1_NORMAL			0x00
#define BMI088_GYRO_LPM1_SUSPEND		0x80
#define BMI088_GYRO_LPM1_DEEP_SUSPEND	0x20

#define BMI088_GYRO_REG_RESET				0x14
#define BMI088_GYRO_RESET_VAL				0xB6

#define BMI088_GYRO_REG_INT_CTRL			0x15
#define BMI088_GYRO_INT_DRDY_ENABLE		BIT(8)

#define BMI088_GYRO_REG_INT4_IO_CONF		0x16
#define BMI088_GYRO_INT4_IO_CONF_BIT_OD		BIT(3)
#define BMI088_GYRO_INT4_IO_CONF_BIT_LVL	BIT(2)
#define BMI088_GYRO_INT3_IO_CONF_BIT_OD		BIT(1)
#define BMI088_GYRO_INT3_IO_CONF_BIT_LVL	BIT(0)

#define BMI088_GYRO_REG_INT4_IO_MAP			0x18
#define BMI088_GYRO_INT_MAP_DATA_BIT_INT3_DRDY		0x01
#define BMI088_GYRO_INT_MAP_DATA_BIT_INT4_DRDY		0x80
#define BMI088_GYRO_INT_MAP_DATA_BIT_INT3_INT4		0x81

#define BMI088_GYRO_REG_SELF_TEST			0x3C
#define BMI088_GYRO_SELF_TEST_RATE_OK		BIT(4)
#define BMI088_GYRO_SELF_TEST_BIST_FAIL		BIT(2)
#define BMI088_GYRO_SELF_TEST_BIST_RDY		BIT(1)
#define BMI088_GYRO_SELF_TEST_TRIG_BIST		BIT(0)

enum bmi088_gyro_axis {
	AXIS_X,
	AXIS_Y,
	AXIS_Z,
};

static const int bmi088_gyro_sample_freqs[] = {
	532, 0,
	230, 0,
	116, 0,
	47, 0,
	23, 0,
	12, 0,
	64, 0,
	32, 0,
};

/* Available OSR (over sampling rate) sets the 3dB cut-off frequency */
enum bmi088_gyro_odr_modes {
	BMI088_GYRO_MODE_ODR_2000_532 = 0x0,
	BMI088_GYRO_MODE_ODR_2000_230 = 0x1,
	BMI088_GYRO_MODE_ODR_1000_116 = 0x2,
	BMI088_GYRO_MODE_ODR_400_47 = 0x3,
	BMI088_GYRO_MODE_ODR_200_23 = 0x4,
	BMI088_GYRO_MODE_ODR_100_12 = 0x5,
	BMI088_GYRO_MODE_ODR_200_64 = 0x6,
	BMI088_GYRO_MODE_ODR_100_32 = 0x7,
};

struct bmi088_gyro_scale_info {
	int scale;
	u8 reg_range;
};

struct bmi088_gyro_chip_info {
	const char *name;
	u8 chip_id;
	const struct iio_chan_spec *channels;
	int num_channels;
};

struct bmi088_gyro_data {
	struct regmap *regmap;
	const struct bmi088_gyro_chip_info *chip_info;
	u8 buffer[2] ____cacheline_aligned; /* shared DMA safe buffer */
};

static const struct regmap_range bmi088_gyro_volatile_ranges[] = {
	/* All registers below 0x40 are volatile, except the CHIP ID. */
	regmap_reg_range(BMI088_GYRO_REG_XOUT_L, 0x09),
	/* Mark the RESET as volatile too, it is self-clearing */
	regmap_reg_range(BMI088_GYRO_REG_RESET, BMI088_GYRO_REG_RESET),
};

static const struct regmap_access_table bmi088_gyro_volatile_table = {
	.yes_ranges	= bmi088_gyro_volatile_ranges,
	.n_yes_ranges	= ARRAY_SIZE(bmi088_gyro_volatile_ranges),
};

const struct regmap_config bmi088_gyro_regmap_conf = {
	.reg_bits = 8,
	.val_bits = 8,
	.max_register = 0x3C,
	.volatile_table = &bmi088_gyro_volatile_table,
	.cache_type = REGCACHE_RBTREE,
};
EXPORT_SYMBOL_GPL(bmi088_gyro_regmap_conf);

static int bmi088_gyro_power_up(struct bmi088_gyro_data *data)
{
	int ret;

	/* Enable gyro sensor */
	ret = regmap_write(data->regmap,
		BMI088_GYRO_REG_LPM1, BMI088_GYRO_LPM1_NORMAL);
	if (ret)
		return ret;

	/* Datasheet recommends to wait at least 5ms before communication */
	usleep_range(5000, 6000);

	return 0;
}

static int bmi088_gyro_power_down(struct bmi088_gyro_data *data)
{
	int ret;

	/* Disable gyroerometer and temperature sensor */
	ret = regmap_write(data->regmap,
		BMI088_GYRO_REG_LPM1, BMI088_GYRO_LPM1_DEEP_SUSPEND);
	if (ret)
		return ret;

	/* Datasheet recommends to wait at least 5ms before communication */
	usleep_range(5000, 6000);

	return 0;
}

static int bmi088_gyro_get_sample_freq(struct bmi088_gyro_data *data,
					int *val, int *val2)
{
	unsigned int value;
	int ret;

	ret = regmap_read(data->regmap, BMI088_GYRO_REG_BANDWIDTH,
			  &value);
	if (ret)
		return ret;

	value -= BMI088_GYRO_MODE_ODR_2000_532;
	value <<= 1;

	if (value >= ARRAY_SIZE(bmi088_gyro_sample_freqs) - 1)
		return -EINVAL;

	*val = bmi088_gyro_sample_freqs[value];
	*val2 = bmi088_gyro_sample_freqs[value + 1];

	return IIO_VAL_INT_PLUS_MICRO;
}

static int bmi088_gyro_set_sample_freq(struct bmi088_gyro_data *data, int val)
{
	unsigned int regval;
	int index = 0;

	while (index < ARRAY_SIZE(bmi088_gyro_sample_freqs) &&
	       bmi088_gyro_sample_freqs[index] != val)
		index += 2;

	if (index >= ARRAY_SIZE(bmi088_gyro_sample_freqs))
		return -EINVAL;

	regval = (index >> 1) + BMI088_GYRO_MODE_ODR_2000_532;

	return regmap_write(data->regmap, BMI088_GYRO_REG_BANDWIDTH,
				  regval);
}

static int bmi088_gyro_get_axis(struct bmi088_gyro_data *data,
				 struct iio_chan_spec const *chan,
				 int *val)
{
	int ret;
	s16 raw_val;

	ret = regmap_bulk_read(data->regmap,
			       BMI088_GYRO_AXIS_TO_REG(chan->scan_index),
			       data->buffer, sizeof(__le16));
	if (ret)
		return ret;

	raw_val = le16_to_cpu(*(__le16 *)data->buffer);
	*val = raw_val;

	return IIO_VAL_INT;
}

int bmi088_gyro_get_all_axis(void)
{
	struct fmu_core *fmu = get_fmu_core();
	struct iio_dev *indio_dev = dev_get_drvdata(fmu->bmi088_accel_dev);
	struct bmi088_gyro_data *data = iio_priv(indio_dev);
	u16 buffer[3];
	int ret;

	ret = regmap_bulk_read(data->regmap,
			       BMI088_GYRO_AXIS_TO_REG(0),
			       (u8 *)buffer, 6);
	if (ret)
		return ret;

	fmu->ahrs.gyro.x = (float)buffer[0];
	fmu->ahrs.gyro.y = (float)buffer[1];
	fmu->ahrs.gyro.z = (float)buffer[2];

	fmu->gyro_degree.x = ((float)buffer[0] - fmu->para.gyro_offset.x) * fmu->gyro_scale.x;
	fmu->gyro_degree.y = ((float)buffer[1] - fmu->para.gyro_offset.y) * fmu->gyro_scale.y;
	fmu->gyro_degree.z = ((float)buffer[2] - fmu->para.gyro_offset.z) * fmu->gyro_scale.z;

	vector_x_matrix_t(&fmu->gyro_degree.x, fmu->para.iem, &fmu->gyro_degree_nb.x);

	fmu->gyro_radian_nb.x = fmu->gyro_degree_nb.x * RAD_PER_DEG;
	fmu->gyro_radian_nb.y = fmu->gyro_degree_nb.y * RAD_PER_DEG;
	fmu->gyro_radian_nb.z = fmu->gyro_degree_nb.z * RAD_PER_DEG;

	return 0;
}

static int bmi088_gyro_read_raw(struct iio_dev *indio_dev,
				 struct iio_chan_spec const *chan,
				 int *val, int *val2, long mask)
{
	struct bmi088_gyro_data *data = iio_priv(indio_dev);
	struct device *dev = regmap_get_device(data->regmap);
	int ret;

	switch (mask) {
	case IIO_CHAN_INFO_RAW:
		switch (chan->type) {
		case IIO_ANGL_VEL:
			pm_runtime_get_sync(dev);
			ret = iio_device_claim_direct_mode(indio_dev);
			if (ret)
				goto out_read_raw_pm_put;

			ret = bmi088_gyro_get_axis(data, chan, val);
			iio_device_release_direct_mode(indio_dev);
			if (!ret)
				ret = IIO_VAL_INT;

			goto out_read_raw_pm_put;
		default:
			return -EINVAL;
		}
	case IIO_CHAN_INFO_SCALE:
		switch (chan->type) {
		case IIO_ANGL_VEL:
			pm_runtime_get_sync(dev);
			ret = regmap_read(data->regmap,
					  BMI088_GYRO_REG_GYRO_RANGE, (unsigned int *)val);
			if (ret)
				goto out_read_raw_pm_put;

			*val2 = 15 - (*val & 0x3);
			*val = 3 * 980;
			ret = IIO_VAL_FRACTIONAL_LOG2;

			goto out_read_raw_pm_put;
		default:
			return -EINVAL;
		}
	case IIO_CHAN_INFO_SAMP_FREQ:
		ret = pm_runtime_resume_and_get(dev);
		if (ret)
			return ret;

		ret = bmi088_gyro_get_sample_freq(data, val, val2);
		goto out_read_raw_pm_put;
	default:
		break;
	}

	return -EINVAL;

out_read_raw_pm_put:
	pm_runtime_mark_last_busy(dev);
	pm_runtime_put_autosuspend(dev);

	return ret;
}

static int bmi088_gyro_read_avail(struct iio_dev *indio_dev,
			     struct iio_chan_spec const *chan,
			     const int **vals, int *type, int *length,
			     long mask)
{
	switch (mask) {
	case IIO_CHAN_INFO_SAMP_FREQ:
		*type = IIO_VAL_INT_PLUS_MICRO;
		*vals = bmi088_gyro_sample_freqs;
		*length = ARRAY_SIZE(bmi088_gyro_sample_freqs);
		return IIO_AVAIL_LIST;
	default:
		return -EINVAL;
	}
}

static int bmi088_gyro_write_raw(struct iio_dev *indio_dev,
				  struct iio_chan_spec const *chan,
				  int val, int val2, long mask)
{
	struct bmi088_gyro_data *data = iio_priv(indio_dev);
	struct device *dev = regmap_get_device(data->regmap);
	int ret;

	switch (mask) {
	case IIO_CHAN_INFO_SAMP_FREQ:
		ret = pm_runtime_resume_and_get(dev);
		if (ret)
			return ret;

		ret = bmi088_gyro_set_sample_freq(data, val);
		pm_runtime_mark_last_busy(dev);
		pm_runtime_put_autosuspend(dev);
		return ret;
	default:
		return -EINVAL;
	}
}

#define BMI088_GYRO_CHANNEL(_axis) { \
	.type = IIO_ANGL_VEL, \
	.modified = 1, \
	.channel2 = IIO_MOD_##_axis, \
	.info_mask_separate = BIT(IIO_CHAN_INFO_RAW), \
	.info_mask_shared_by_type = BIT(IIO_CHAN_INFO_SCALE) | \
				BIT(IIO_CHAN_INFO_SAMP_FREQ), \
	.info_mask_shared_by_type_available = BIT(IIO_CHAN_INFO_SAMP_FREQ), \
	.scan_index = AXIS_##_axis, \
}

static const struct iio_chan_spec bmi088_gyro_channels[] = {
	BMI088_GYRO_CHANNEL(X),
	BMI088_GYRO_CHANNEL(Y),
	BMI088_GYRO_CHANNEL(Z),
};

static const struct bmi088_gyro_chip_info bmi088_gyro_chip_info_tbl[] = {
	[0] = {
		.name = "bmi088g",
		.chip_id = 0x0F,
		.channels = bmi088_gyro_channels,
		.num_channels = ARRAY_SIZE(bmi088_gyro_channels),
	},
};

static const struct iio_info bmi088_gyro_info = {
	.read_raw	= bmi088_gyro_read_raw,
	.write_raw	= bmi088_gyro_write_raw,
	.read_avail	= bmi088_gyro_read_avail,
};

static const unsigned long bmi088_gyro_scan_masks[] = {
	BIT(AXIS_X) | BIT(AXIS_Y) | BIT(AXIS_Z),
	0
};

static int bmi088_gyro_chip_init(struct bmi088_gyro_data *data)
{
	struct device *dev = regmap_get_device(data->regmap);
	int ret, i;
	unsigned int val;

	/* Do a dummy read to enable SPI interface, won't harm I2C */
	regmap_read(data->regmap, BMI088_GYRO_REG_INT_STATUS, &val);

	/*
	 * Reset chip to get it in a known good state. A delay of 1ms after
	 * reset is required according to the data sheet
	 */
	ret = regmap_write(data->regmap, BMI088_GYRO_REG_RESET,
			   BMI088_GYRO_RESET_VAL);
	if (ret)
		return ret;

	usleep_range(1000, 2000);

	/* Do a dummy read again after a reset to enable the SPI interface */
	regmap_read(data->regmap, BMI088_GYRO_REG_INT_STATUS, &val);

	/* Read chip ID */
	ret = regmap_read(data->regmap, BMI088_GYRO_REG_CHIP_ID, &val);
	if (ret) {
		dev_err(dev, "Error: Reading gyro chip id\n");
		return ret;
	}

	/* Validate chip ID */
	for (i = 0; i < ARRAY_SIZE(bmi088_gyro_chip_info_tbl); i++) {
		if (bmi088_gyro_chip_info_tbl[i].chip_id == val) {
			data->chip_info = &bmi088_gyro_chip_info_tbl[i];
			break;
		}
	}
	if (i == ARRAY_SIZE(bmi088_gyro_chip_info_tbl)) {
		dev_err(dev, "Invalid gyro chip %x\n", val);
		return -ENODEV;
	}

	return 0;
}

int bmi088_gyro_core_probe(struct device *dev, struct regmap *regmap,
	int irq, const char *name, bool block_supported)
{
	struct bmi088_gyro_data *data;
	struct iio_dev *indio_dev;
	int ret;

	indio_dev = devm_iio_device_alloc(dev, sizeof(*data));
	if (!indio_dev)
		return -ENOMEM;

	data = iio_priv(indio_dev);
	dev_set_drvdata(dev, indio_dev);

	data->regmap = regmap;

	ret = bmi088_gyro_chip_init(data);
	if (ret)
		return ret;

	indio_dev->channels = data->chip_info->channels;
	indio_dev->num_channels = data->chip_info->num_channels;
	indio_dev->name = name ? name : data->chip_info->name;
	indio_dev->available_scan_masks = bmi088_gyro_scan_masks;
	indio_dev->modes = INDIO_DIRECT_MODE;
	indio_dev->info = &bmi088_gyro_info;

	/* Enable runtime PM */
	pm_runtime_get_noresume(dev);
	pm_runtime_set_suspended(dev);
	pm_runtime_enable(dev);
	/* We need ~6ms to startup, so set the delay to 6 seconds */
	pm_runtime_set_autosuspend_delay(dev, 6000);
	pm_runtime_use_autosuspend(dev);
	pm_runtime_put(dev);

	ret = iio_device_register(indio_dev);
	if (ret)
		dev_err(dev, "Unable to register iio device\n");

	return ret;
}
EXPORT_SYMBOL_GPL(bmi088_gyro_core_probe);


void bmi088_gyro_core_remove(struct device *dev)
{
	struct iio_dev *indio_dev = dev_get_drvdata(dev);
	struct bmi088_gyro_data *data = iio_priv(indio_dev);

	iio_device_unregister(indio_dev);

	pm_runtime_disable(dev);
	pm_runtime_set_suspended(dev);
	bmi088_gyro_power_down(data);
}
EXPORT_SYMBOL_GPL(bmi088_gyro_core_remove);

static int __maybe_unused bmi088_gyro_runtime_suspend(struct device *dev)
{
	struct iio_dev *indio_dev = dev_get_drvdata(dev);
	struct bmi088_gyro_data *data = iio_priv(indio_dev);

	return bmi088_gyro_power_down(data);
}

static int __maybe_unused bmi088_gyro_runtime_resume(struct device *dev)
{
	struct iio_dev *indio_dev = dev_get_drvdata(dev);
	struct bmi088_gyro_data *data = iio_priv(indio_dev);

	return bmi088_gyro_power_up(data);
}

const struct dev_pm_ops bmi088_gyro_pm_ops = {
	SET_SYSTEM_SLEEP_PM_OPS(pm_runtime_force_suspend,
				pm_runtime_force_resume)
	SET_RUNTIME_PM_OPS(bmi088_gyro_runtime_suspend,
			   bmi088_gyro_runtime_resume, NULL)
};
EXPORT_SYMBOL_GPL(bmi088_gyro_pm_ops);

MODULE_AUTHOR("Yaqi.Wu <yaqii.wu@medaitek.com>");
MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("BMI088 gyro driver (core)");
