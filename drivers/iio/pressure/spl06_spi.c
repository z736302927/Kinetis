// SPDX-License-Identifier: GPL-2.0
/*
 * SPL06 pressure and temperature sensor driver (SPI bus)
 *
 * Copyright (c) Tomasz Duszynski <tduszyns@gmail.com>
 *
 */

#include <generated/deconfig.h>
#include <linux/delay.h>
#include <linux/module.h>
#include <linux/spi/spi.h>
#include <linux/mod_devicetable.h>

#include <asm/unaligned.h>

#include "spl06.h"

static int spl06_spi_reset(struct device *dev)
{
	u8 cmd[2];

	struct spl06_state *st = iio_priv(dev_to_iio_dev(dev));

	cmd[0] = SPL06_RESET;
	cmd[1] = 0x89;

	return spi_write(st->client, &cmd, 2);
}

static int spl06_spi_read_adc(struct device *dev, u8 reg, s32 *val)
{
	int ret;
	u8 buf[3] = { reg };
	struct spl06_state *st = iio_priv(dev_to_iio_dev(dev));

	ret = spi_write_then_read(st->client, buf, 1, buf, 3);
	if (ret < 0)
		return ret;

	*val = get_unaligned_be24(&buf[0]);

	return 0;
}

static int spl06_spi_read_adc_temp_and_pressure(struct device *dev,
						 s32 *temp, s32 *pressure)
{
	int ret;
	struct spl06_state *st = iio_priv(dev_to_iio_dev(dev));
	const struct spl06_osr *osr = st->temp_osr;

//	/*
//	 * Warning: &osr->cmd MUST be aligned on a word boundary since used as
//	 * 2nd argument (void*) of spi_write_then_read.
//	 */
//	ret = spi_write_then_read(st->client, &osr->cmd, 1, NULL, 0);
//	if (ret < 0)
//		return ret;

//	usleep_range(osr->conv_usec, osr->conv_usec + (osr->conv_usec / 10UL));
	ret = spl06_spi_read_adc(dev, SPL06_TMP_B2, temp);
	if (ret < 0)
		return ret;

//	osr = st->pressure_osr;
//	ret = spi_write_then_read(st->client, &osr->cmd, 1, NULL, 0);
//	if (ret < 0)
//		return ret;

//	usleep_range(osr->conv_usec, osr->conv_usec + (osr->conv_usec / 10UL));
	return spl06_spi_read_adc(dev, SPL06_PSR_B2, pressure);
}

static int spl06_spi_read_coefficients(struct device *dev,
	struct spl06_coefficients *coef)
{
	struct spl06_state *st = iio_priv(dev_to_iio_dev(dev));
	u8 addr = SPL06_COEF | BIT(7);
	int ret;

	ret = spi_write_then_read(st->client, &addr, 1, coef, sizeof(*coef));
	if (ret < 0)
		return ret;

	return 0;
}

static int spl06_spi_set_osr(struct device *dev)
{
	struct spl06_state *st = iio_priv(dev_to_iio_dev(dev));
	u8 addr[2];
	int ret;

	addr[0] = SPL06_PRS_CFG;
	addr[1] = (st->pressure_mr->cmd << 4) | st->pressure_osr->cmd;
	ret = spi_write(st->client, &addr, 2);
	if (ret < 0)
		return ret;

	if (st->pressure_osr->cmd > 3) {
		ret = spi_w8r8(st->client, SPL06_CFG_REG | BIT(7));
		if (ret < 0)
			return ret;
		addr[0] = SPL06_CFG_REG;
		addr[1] = ret | 0x04;
		ret = spi_write(st->client, &addr, 2);
		if (ret < 0)
			return ret;
	}

	addr[0] = SPL06_TMP_CFG;
	addr[1] = (st->temp_mr->cmd << 4) | st->temp_osr->cmd | SPL06_TMP_CFG_EXT;
	ret = spi_write(st->client, &addr, 2);
	if (ret < 0)
		return ret;

	if (st->temp_osr->cmd > 3) {
		ret = spi_w8r8(st->client, SPL06_CFG_REG | BIT(7));
		if (ret < 0)
			return ret;
		addr[0] = SPL06_CFG_REG;
		addr[1] = ret | 0x08;
		ret = spi_write(st->client, &addr, 2);
		if (ret < 0)
			return ret;
	}

	return 0;
}

static int spl06_spi_set_op_mode_status(struct device *dev, u8 mode)
{
	u8 cmd[2];
	struct spl06_state *st = iio_priv(dev_to_iio_dev(dev));

	cmd[0] = SPL06_MEAS_CFG;
	cmd[1] = mode;

	return spi_write(st->client, &cmd, 2);
}

static int spl06_spi_probe(struct spi_device *spi)
{
	int ret;
	struct spl06_state *st;
	struct iio_dev *indio_dev;

	indio_dev = devm_iio_device_alloc(&spi->dev, sizeof(*st));
	if (!indio_dev)
		return -ENOMEM;

	spi_set_drvdata(spi, indio_dev);

	spi->mode = SPI_MODE_0;
	spi->max_speed_hz = 84000000;
	spi->bits_per_word = 8;
	ret = spi_setup(spi);
	if (ret < 0)
		return ret;

	st = iio_priv(indio_dev);
	st->reset = spl06_spi_reset;
	st->read_adc_temp_and_pressure = spl06_spi_read_adc_temp_and_pressure;
	st->coefficients = spl06_spi_read_coefficients;
	st->set_osr = spl06_spi_set_osr;
	st->set_op_mode = spl06_spi_set_op_mode_status;
	st->client = spi;

	return spl06_probe(indio_dev, &spi->dev, spi_get_device_id(spi)->name,
			    spi_get_device_id(spi)->driver_data);
}

static int spl06_spi_remove(struct spi_device *spi)
{
	return spl06_remove(spi_get_drvdata(spi));
}

static const struct of_device_id spl06_spi_matches[] = {
	{ .compatible = "GoerTek,spl06" },
	{ }
};
MODULE_DEVICE_TABLE(of, spl06_spi_matches);

static const struct spi_device_id spl06_id[] = {
	{ "spl06", SPL06 },
	{ }
};
MODULE_DEVICE_TABLE(spi, spl06_id);

static struct spi_driver spl06_driver = {
	.driver = {
		.name = "spl06",
		.of_match_table = spl06_spi_matches
	},
	.id_table = spl06_id,
	.probe = spl06_spi_probe,
	.remove = spl06_spi_remove,
};
module_spi_driver(spl06_driver);

MODULE_AUTHOR("Tomasz Duszynski <tduszyns@gmail.com>");
MODULE_DESCRIPTION("SPL06 spi driver");
MODULE_LICENSE("GPL v2");
