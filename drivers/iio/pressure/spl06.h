/* SPDX-License-Identifier: GPL-2.0 */
/*
 * SPL06 pressure and temperature sensor driver
 *
 * Copyright (c) Tomasz Duszynski <tduszyns@gmail.com>
 *
 */

#ifndef _SPL06_H
#define _SPL06_H

#include <linux/device.h>
#include <linux/iio/iio.h>
#include <linux/mutex.h>

struct regulator;

#define SPL06_PSR_B2			0x00
#define SPL06_PSR_B1			0x01
#define SPL06_PSR_B0			0x02
#define SPL06_TMP_B2			0x03
#define SPL06_TMP_B1			0x04
#define SPL06_TMP_B0			0x05
#define SPL06_PRS_CFG			0x06

#define SPL06_TMP_CFG			0x07
#define SPL06_TMP_CFG_EXT		0x80

#define SPL06_MEAS_CFG			0x08
#define SPL06_STANDBY_MODE			0b000
#define SPL06_CMD_MODE_P			0b001
#define SPL06_CMD_MODE_T			0b010
#define SPL06_BG_MODE_P			0b101
#define SPL06_BG_MODE_T			0b110
#define SPL06_BG_MODE_PT			0b111

#define SPL06_CFG_REG			0x09
#define SPL06_INT_STS			0x0A
#define SPL06_FIFO_STS			0x0B
#define SPL06_RESET			0x0C
#define SPL06_ID			0x0D
#define SPL06_COEF			0x10
#define SPL06_COEF_SRCE			0x28

enum {
	SPL06,
};

struct spl06_coefficients {
	signed c0:12;
	signed c1:12;
	signed c00:20;
	signed c10:20;
	s16 c01;
	s16 c11;
	s16 c20;
	s16 c21;
	s16 c30;
};

struct spl06_chip_info {
	s32 pressure;
	s32 temp;

	struct spl06_coefficients coef;

	int (*temp_and_pressure_compensate)(struct spl06_coefficients *coef,
		u32 kt, u32 kp, s32 *temp, s32 *pressure);
};

/*
 * OverSampling Rate descriptor.
 * Warning: cmd MUST be kept aligned on a word boundary (see
 * m5611_spi_read_adc_temp_and_pressure in spl06_spi.c).
 */
struct spl06_osr {
	unsigned long conv_usec;
	u8 cmd;
	unsigned short rate;
};

struct spl06_state {
	void *client;
	struct mutex lock;

	struct spl06_osr *pressure_osr;
	struct spl06_osr *pressure_mr;
	struct spl06_osr *temp_osr;
	struct spl06_osr *temp_mr;

	int (*reset)(struct device *dev);
	int (*config)(struct device *dev, int index, u16 *word);
	int (*read_adc_temp_and_pressure)(struct device *dev,
					  s32 *temp, s32 *pressure);
	int (*coefficients)(struct device *dev,
					    struct spl06_coefficients *coef);
	int (*set_osr)(struct device *dev);
	int (*set_op_mode)(struct device *dev, u8 mode);

	struct spl06_chip_info *chip_info;
	struct regulator *vdd;
};

int spl06_probe(struct iio_dev *indio_dev, struct device *dev,
		 const char *name, int type);
int spl06_remove(struct iio_dev *indio_dev);

#endif /* _SPL06_H */
