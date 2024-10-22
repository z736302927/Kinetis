// SPDX-License-Identifier: GPL-2.0
/*
 * SPL06 pressure and temperature sensor driver
 *
 * Copyright (c) Tomasz Duszynski <tduszyns@gmail.com>
 *
 * Data sheet:
 *  http://www.meas-spec.com/downloads/SPL06-01BA03.pdf
 *  http://www.meas-spec.com/downloads/MS5607-02BA03.pdf
 *
 */

#include <generated/deconfig.h>
#include <linux/module.h>
#include <linux/iio/iio.h>
#include <linux/delay.h>
#include <linux/regulator/consumer.h>

#include <linux/iio/sysfs.h>
#include <linux/iio/buffer.h>
#include <linux/iio/triggered_buffer.h>
#include <linux/iio/trigger_consumer.h>
#include "spl06.h"

#include <kinetis/fmu.h>

#define SPL06_INIT_OSR(_cmd, _conv_usec, _rate) \
	{ .cmd = _cmd, .conv_usec = _conv_usec, .rate = _rate }

static struct spl06_osr spl06_avail_pressure_osr[] = {
	SPL06_INIT_OSR(0b000, 524288, 1),
	SPL06_INIT_OSR(0b001, 1572864, 2),
	SPL06_INIT_OSR(0b010, 3670016, 4),
	SPL06_INIT_OSR(0b011, 7864320, 8),
	SPL06_INIT_OSR(0b100, 253952, 16),
	SPL06_INIT_OSR(0b101, 516096, 32),
	SPL06_INIT_OSR(0b110, 1040384, 64),
	SPL06_INIT_OSR(0b111, 2088960, 128)
};

static struct spl06_osr spl06_avail_temp_osr[] = {
	SPL06_INIT_OSR(0b000, 524288, 1),
	SPL06_INIT_OSR(0b001, 1572864, 2),
	SPL06_INIT_OSR(0b010, 3670016, 4),
	SPL06_INIT_OSR(0b011, 7864320, 8),
	SPL06_INIT_OSR(0b100, 253952, 16),
	SPL06_INIT_OSR(0b101, 516096, 32),
	SPL06_INIT_OSR(0b110, 1040384, 64),
	SPL06_INIT_OSR(0b111, 2088960, 128)
};

static const char spl06_show_osr[] = "1 2 4 8 16 32 64 128";

static IIO_CONST_ATTR(oversampling_ratio_available, spl06_show_osr);

static struct attribute *spl06_attributes[] = {
	&iio_const_attr_oversampling_ratio_available.dev_attr.attr,
	NULL,
};

static const struct attribute_group spl06_attribute_group = {
	.attrs = spl06_attributes,
};

static int spl06_read_temp_and_pressure(struct iio_dev *indio_dev,
					 s32 *temp, s32 *pressure)
{
	int ret;
	struct spl06_state *st = iio_priv(indio_dev);

	ret = st->read_adc_temp_and_pressure(&indio_dev->dev, temp, pressure);
	if (ret < 0) {
		dev_err(&indio_dev->dev,
			"failed to read temperature and pressure\n");
		return ret;
	}

	return st->chip_info->temp_and_pressure_compensate(&st->chip_info->coef,
		st->temp_osr->conv_usec, st->pressure_osr->conv_usec, temp, pressure);
}

int spl06_get_temp_and_pressure(void)
{
	struct fmu_core *fmu = get_fmu_core();
	struct iio_dev *indio_dev = dev_get_drvdata(fmu->bmi088_accel_dev);
	s32 temp, pressure;
	float height;
	int ret;

	ret = spl06_read_temp_and_pressure(indio_dev, &temp, &pressure);
	if (ret)
		return ret;

    height = (101400 - pressure) / 1000.0f;
    fmu->height = 0.82f * float_pow(height, 3) + 9.0f * (101400 - pressure);

	return 0;
}

static int spl06_temp_and_pressure_compensate(struct spl06_coefficients *coef,
		u32 kt, u32 kp, s32 *temp, s32 *pressure)
{
	float t = *temp / kt, p = *pressure / kp; 

	*temp = coef->c0 * 0.5 + coef->c1 * t;
	*pressure = coef->c00 + p * (coef->c10 + p * (coef->c20 + p * coef->c30)) +
		t * coef->c01 + t * p * (coef->c11 + p * coef->c21);

	return 0;
}

static int spl06_reset(struct iio_dev *indio_dev)
{
	int ret;
	struct spl06_state *st = iio_priv(indio_dev);

	ret = st->reset(&indio_dev->dev);
	if (ret < 0) {
		dev_err(&indio_dev->dev, "failed to reset device\n");
		return ret;
	}

	usleep_range(3000, 4000);

	return 0;
}

static int spl06_set_osr(struct iio_dev *indio_dev)
{
	int ret;
	struct spl06_state *st = iio_priv(indio_dev);

	ret = st->set_osr(&indio_dev->dev);
	if (ret < 0) {
		dev_err(&indio_dev->dev, "failed to set osr\n");
		return ret;
	}

	return 0;
}

static irqreturn_t spl06_trigger_handler(int irq, void *p)
{
	struct iio_poll_func *pf = p;
	struct iio_dev *indio_dev = pf->indio_dev;
	struct spl06_state *st = iio_priv(indio_dev);
	/* Ensure buffer elements are naturally aligned */
	struct {
		s32 channels[2];
		s64 ts __aligned(8);
	} scan;
	int ret;

	mutex_lock(&st->lock);
	ret = spl06_read_temp_and_pressure(indio_dev, &scan.channels[1],
					    &scan.channels[0]);
	mutex_unlock(&st->lock);
	if (ret < 0)
		goto err;

	iio_push_to_buffers_with_timestamp(indio_dev, &scan,
					   iio_get_time_ns(indio_dev));

err:
	iio_trigger_notify_done(indio_dev->trig);

	return IRQ_HANDLED;
}

static int spl06_read_raw(struct iio_dev *indio_dev,
			   struct iio_chan_spec const *chan,
			   int *val, int *val2, long mask)
{
	int ret;
	s32 temp, pressure;
	struct spl06_state *st = iio_priv(indio_dev);

	switch (mask) {
	case IIO_CHAN_INFO_PROCESSED:
		mutex_lock(&st->lock);
		ret = spl06_read_temp_and_pressure(indio_dev,
						    &temp, &pressure);
		mutex_unlock(&st->lock);
		if (ret < 0)
			return ret;

		switch (chan->type) {
		case IIO_TEMP:
			*val = temp * 10;
			return IIO_VAL_INT;
		case IIO_PRESSURE:
			*val = pressure / 1000;
			*val2 = (pressure % 1000) * 1000;
			return IIO_VAL_INT_PLUS_MICRO;
		default:
			return -EINVAL;
		}
	case IIO_CHAN_INFO_SCALE:
		switch (chan->type) {
		case IIO_TEMP:
			*val = 10;
			return IIO_VAL_INT;
		case IIO_PRESSURE:
			*val = 0;
			*val2 = 1000;
			return IIO_VAL_INT_PLUS_MICRO;
		default:
			return -EINVAL;
		}
	case IIO_CHAN_INFO_OVERSAMPLING_RATIO:
		if (chan->type != IIO_TEMP && chan->type != IIO_PRESSURE)
			break;
		mutex_lock(&st->lock);
		if (chan->type == IIO_TEMP)
			*val = (int)st->temp_osr->rate;
		else
			*val = (int)st->pressure_osr->rate;
		mutex_unlock(&st->lock);
		return IIO_VAL_INT;
	}

	return -EINVAL;
}

static struct spl06_osr *spl06_find_osr(int rate,
						struct spl06_osr *osr,
						size_t count)
{
	unsigned int r;

	for (r = 0; r < count; r++)
		if ((unsigned short)rate == osr[r].rate)
			break;
	if (r >= count)
		return NULL;
	return &osr[r];
}

static int spl06_write_raw(struct iio_dev *indio_dev,
			    struct iio_chan_spec const *chan,
			    int val, int val2, long mask)
{
	struct spl06_state *st = iio_priv(indio_dev);
	struct spl06_osr *osr = NULL;
	int ret;

	if (mask != IIO_CHAN_INFO_OVERSAMPLING_RATIO)
		return -EINVAL;

	if (chan->type == IIO_TEMP)
		osr = spl06_find_osr(val, spl06_avail_temp_osr,
				      ARRAY_SIZE(spl06_avail_temp_osr));
	else if (chan->type == IIO_PRESSURE)
		osr = spl06_find_osr(val, spl06_avail_pressure_osr,
				      ARRAY_SIZE(spl06_avail_pressure_osr));
	if (!osr)
		return -EINVAL;

	ret = iio_device_claim_direct_mode(indio_dev);
	if (ret)
		return ret;

	mutex_lock(&st->lock);

	if (chan->type == IIO_TEMP)
		st->temp_osr = osr;
	else
		st->pressure_osr = osr;

	mutex_unlock(&st->lock);
	iio_device_release_direct_mode(indio_dev);

	return 0;
}

static const unsigned long spl06_scan_masks[] = {0x3, 0};

static struct spl06_chip_info chip_info_tbl[] = {
	[SPL06] = {
		.temp_and_pressure_compensate = spl06_temp_and_pressure_compensate,
	},
};

static const struct iio_chan_spec spl06_channels[] = {
	{
		.type = IIO_PRESSURE,
		.info_mask_separate = BIT(IIO_CHAN_INFO_PROCESSED) |
			BIT(IIO_CHAN_INFO_SCALE) |
			BIT(IIO_CHAN_INFO_OVERSAMPLING_RATIO),
		.scan_index = 0,
		.scan_type = {
			.sign = 's',
			.realbits = 32,
			.storagebits = 32,
			.endianness = IIO_CPU,
		},
	},
	{
		.type = IIO_TEMP,
		.info_mask_separate = BIT(IIO_CHAN_INFO_PROCESSED) |
			BIT(IIO_CHAN_INFO_SCALE) |
			BIT(IIO_CHAN_INFO_OVERSAMPLING_RATIO),
		.scan_index = 1,
		.scan_type = {
			.sign = 's',
			.realbits = 32,
			.storagebits = 32,
			.endianness = IIO_CPU,
		},
	},
	IIO_CHAN_SOFT_TIMESTAMP(2),
};

static const struct iio_info spl06_info = {
	.read_raw = &spl06_read_raw,
	.write_raw = &spl06_write_raw,
};

static int spl06_init(struct iio_dev *indio_dev)
{
	struct spl06_state *st = iio_priv(indio_dev);
	int ret;

//	/* Enable attached regulator if any. */
//	st->vdd = devm_regulator_get(indio_dev->dev.parent, "vdd");
//	if (IS_ERR(st->vdd))
//		return PTR_ERR(st->vdd);
//
//	ret = regulator_enable(st->vdd);
//	if (ret) {
//		dev_err(indio_dev->dev.parent,
//			"failed to enable Vdd supply: %d\n", ret);
//		return ret;
//	}

	ret = spl06_reset(indio_dev);
	if (ret < 0)
		goto err_regulator_disable;

	ret = st->coefficients(&indio_dev->dev, &st->chip_info->coef);
	if (ret < 0) {
		dev_err(&indio_dev->dev, "failed to get coefficients\n");
		goto err_regulator_disable;
	}

	ret = spl06_set_osr(indio_dev);
	if (ret < 0)
		goto err_regulator_disable;

	ret = st->set_op_mode(&indio_dev->dev, SPL06_BG_MODE_PT);
	if (ret < 0) {
		dev_err(&indio_dev->dev, "failed to set sensor operating mode\n");
		goto err_regulator_disable;
	}

	return 0;

err_regulator_disable:
//	regulator_disable(st->vdd);
	return ret;
}

static void spl06_fini(const struct iio_dev *indio_dev)
{
//	const struct spl06_state *st = iio_priv(indio_dev);

//	regulator_disable(st->vdd);
}

int spl06_probe(struct iio_dev *indio_dev, struct device *dev,
		 const char *name, int type)
{
	int ret;
	struct spl06_state *st = iio_priv(indio_dev);

	mutex_init(&st->lock);
	st->chip_info = &chip_info_tbl[type];
	st->temp_mr =
		&spl06_avail_temp_osr[3];
	st->temp_osr =
		&spl06_avail_temp_osr[3];
	st->pressure_mr =
		&spl06_avail_pressure_osr[7];
	st->pressure_osr =
		&spl06_avail_pressure_osr[4];
	indio_dev->name = name;
	indio_dev->info = &spl06_info;
	indio_dev->channels = spl06_channels;
	indio_dev->num_channels = ARRAY_SIZE(spl06_channels);
	indio_dev->modes = INDIO_DIRECT_MODE;
	indio_dev->available_scan_masks = spl06_scan_masks;

	ret = spl06_init(indio_dev);
	if (ret < 0)
		return ret;

//	ret = iio_triggered_buffer_setup(indio_dev, NULL,
//					 spl06_trigger_handler, NULL);
//	if (ret < 0) {
//		dev_err(dev, "iio triggered buffer setup failed\n");
//		goto err_fini;
//	}

	ret = iio_device_register(indio_dev);
	if (ret < 0) {
		dev_err(dev, "unable to register iio device\n");
		goto err_buffer_cleanup;
	}

	return 0;

err_buffer_cleanup:
//	iio_triggered_buffer_cleanup(indio_dev);
err_fini:
	spl06_fini(indio_dev);
	return ret;
}
EXPORT_SYMBOL(spl06_probe);

int spl06_remove(struct iio_dev *indio_dev)
{
	iio_device_unregister(indio_dev);
//	iio_triggered_buffer_cleanup(indio_dev);
	spl06_fini(indio_dev);

	return 0;
}
EXPORT_SYMBOL(spl06_remove);

MODULE_AUTHOR("Tomasz Duszynski <tduszyns@gmail.com>");
MODULE_DESCRIPTION("SPL06 core driver");
MODULE_LICENSE("GPL v2");
