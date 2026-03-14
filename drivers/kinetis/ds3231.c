
#define pr_fmt(fmt) "ds3231: " fmt

#include <linux/slab.h>
#include <linux/iopoll.h>
#include <linux/printk.h>
#include <linux/bitops.h>
#include <linux/random.h>
#include <linux/limits.h>

#include "kinetis/ds3231.h"
#include "kinetis/iic_soft.h"
#include "kinetis/spi_soft.h"
#include "kinetis/regmap-user-bus.h"
#include "kinetis/idebug.h"
#include "kinetis/delay.h"
#include "kinetis/design_verification.h"

#include <math.h>
#include <pthread.h>

#define DS3231_HOURS24                  0x00
#define DS3231_HOURS12                  0x01
#define DS3231_AM                       0x00
#define DS3231_PM                       0x01
#define DS3231_FORMAT_BIN               0x0U
#define DS3231_FORMAT_BCD               0x1U

/* Register addresses */
#define DS3231_REG_SECONDS              0x00
#define DS3231_REG_MINUTES              0x01
#define DS3231_REG_HOURS                0x02
#define DS3231_REG_DAY                  0x03
#define DS3231_REG_DATE                 0x04
#define DS3231_REG_MONTH_CENTURY        0x05
#define DS3231_REG_YEAR                 0x06
#define DS3231_REG_ALARM1_SECONDS       0x07
#define DS3231_REG_ALARM1_MINUTES       0x08
#define DS3231_REG_ALARM1_HOURS         0x09
#define DS3231_REG_ALARM1_DAY_DATE      0x0A
#define DS3231_REG_ALARM2_MINUTES       0x0B
#define DS3231_REG_ALARM2_HOURS         0x0C
#define DS3231_REG_ALARM2_DAY_DATE      0x0D
#define DS3231_REG_CONTROL              0x0E
#define DS3231_REG_CONTROL_STATUS       0x0F
#define DS3231_REG_AGING_OFFSET         0x10
#define DS3231_REG_TEMP_MSB             0x11
#define DS3231_REG_TEMP_LSB             0x12

/* Bit masks for seconds register */
#define DS3231_SECONDS_MASK             0x7F
#define DS3231_SECONDS_ST_BIT           0x80

/* Bit masks for minutes register */
#define DS3231_MINUTES_MASK             0x7F

/* Bit masks for hours register */
#define DS3231_HOURS_MASK               0x3F
#define DS3231_HOURS_12_24_BIT          0x40
#define DS3231_HOURS_AM_PM_BIT          0x20
#define DS3231_HOURS_10_HOUR_BIT        0x10

/* Bit masks for day register */
#define DS3231_DAY_MASK                 0x07

/* Bit masks for date register */
#define DS3231_DATE_MASK                0x3F
#define DS3231_DATE_10_DATE_BIT         0x30

/* Bit masks for month/century register */
#define DS3231_MONTH_MASK               0x1F
#define DS3231_MONTH_10_MONTH_BIT       0x10
#define DS3231_CENTURY_BIT              0x80

/* Bit masks for year register */
#define DS3231_YEAR_MASK                0xFF
#define DS3231_YEAR_10_YEAR_BIT         0xF0

/* Bit masks for alarm registers */
#define DS3231_ALARM_MASK               0x80
#define DS3231_ALARM_DY_DT_BIT          0x40
#define DS3231_ALARM_10_MINUTE_BIT      0x70
#define DS3231_ALARM_10_HOUR_BIT        0x30
#define DS3231_ALARM_10_DATE_BIT        0x30

/* Bit masks for control register */
#define DS3231_CONTROL_A1IE_BIT         0x01
#define DS3231_CONTROL_A2IE_BIT         0x02
#define DS3231_CONTROL_INTCN_BIT        0x04
#define DS3231_CONTROL_RS1_BIT          0x08
#define DS3231_CONTROL_RS2_BIT          0x10
#define DS3231_CONTROL_CONV_BIT         0x20
#define DS3231_CONTROL_BBSQW_BIT        0x40
#define DS3231_CONTROL_EOSC_BIT         0x80

/* Bit masks for control/status register */
#define DS3231_STATUS_A1F_BIT           0x01
#define DS3231_STATUS_A2F_BIT           0x02
#define DS3231_STATUS_BSY_BIT           0x04
#define DS3231_STATUS_EN32KHZ_BIT       0x08
#define DS3231_STATUS_OSF_BIT           0x80

/* Square wave frequency selection */
#define DS3231_SQW_FREQ_1HZ             0x00
#define DS3231_SQW_FREQ_1024HZ          0x01
#define DS3231_SQW_FREQ_4096HZ          0x02
#define DS3231_SQW_FREQ_8192HZ          0x03

/* Alarm mask settings */
#define DS3231_ALARM_MASK_SECOND        0x01
#define DS3231_ALARM_MASK_MINUTE        0x02
#define DS3231_ALARM_MASK_HOUR          0x04
#define DS3231_ALARM_MASK_DAY_DATE      0x08

/* Day/Date selection for alarms */
#define DS3231_ALARM_DAY                0x40
#define DS3231_ALARM_DATE               0x00

#define DS3231_ADDR                     0x68

#define ALARM_MASK                      0x80
#define ALARM_MASK_1                    0x01
#define ALARM_MASK_2                    0x02
#define ALARM_MASK_3                    0x04
#define ALARM_MASK_4                    0x08
#define TIMEMODE_MASK                   0x40
#define WEEK_CYCLE                      0x00
#define MONTH_CYCLE                     0x01
#define DY_DT_MASK                      0x40
#define SQUARE_WAVE_1HZ                 0x00
#define SQUARE_WAVE_1_024HZ             0x01
#define SQUARE_WAVE_4_096HZ             0x02
#define SQUARE_WAVE_8_192HZ             0x03
#define SECONDS                         0x00
#define MINUTES                         0x01
#define HOURS                           0x02
#define DAY                             0x03
#define DATE                            0x04
#define MONTH_CENTURY                   0x05
#define YEAR                            0x06
#define ALARM_1_SECONDS                 0x07
#define ALARM_1_MINUTES                 0x08
#define ALARM_1_HOURS                   0x09
#define ALARM_1_DAY_DATE                0x0A
#define ALARM_2_MINUTES                 0x0B
#define ALARM_2_HOURS                   0x0C
#define ALARM_2_DAY_DATE                0x0D
#define CONTROL                         0x0E
#define CONTROL_STATUS                  0x0F
#define AGING_OFFSET                    0x10
#define MSB_OF_TEMP                     0x11
#define LSB_OF_TEMP                     0x12

struct ds3231_device {
	struct regmap *regmap;
	u8 time_mode;
	u8 time_region;
	struct iic_slave *iic_slave;
	struct spi_slave *spi_slave;
	u8 *slave_regs;

	bool thread_running;
};

void ds3231_get_time(struct ds3231_device *dev, u8 *pdata, u8 format)
{
	u8 tmp[7];
	u8 hour10 = 0;

	regmap_bulk_read(dev->regmap, SECONDS, tmp, 7);

	if (tmp[2] & 0x40) {
		dev->time_mode = DS3231_HOURS12;
		hour10 = (tmp[2] & 0x10) >> 4;

		if (tmp[2] & 0x20) {
			dev->time_region = DS3231_PM;
		} else {
			dev->time_region = DS3231_AM;
		}
	} else {
		dev->time_mode = DS3231_HOURS24;
		hour10 = (tmp[2] & 0x30) >> 4;
	}

	pdata[0] = (tmp[0] >> 4) * 10 + (tmp[0] & 0x0F);
	pdata[1] = (tmp[1] >> 4) * 10 + (tmp[1] & 0x0F);
	pdata[2] = hour10 * 10 + (tmp[2] & 0x0F);
	pdata[3] = (tmp[4] >> 4) * 10 + (tmp[4] & 0x0F);
	pdata[4] = ((tmp[5] & 0x1F) >> 4) * 10 + (tmp[5] & 0x0F);
	pdata[5] = (tmp[6] >> 4) * 10 + (tmp[6] & 0x0F);
}

void ds3231_set_time(struct ds3231_device *dev, u8 *pdata, u8 format)
{
	u8 tmp[7] = {0, 0, 0, 0, 0, 0, 0};

	regmap_bulk_write(dev->regmap, SECONDS, tmp, 3);
	regmap_bulk_write(dev->regmap, DATE, tmp, 3);

	tmp[6] |= (pdata[0] / 10) << 4;
	tmp[6] |= (pdata[0] % 10) << 0;
	tmp[5] |= (pdata[1] / 10) << 4;
	tmp[5] |= (pdata[1] % 10) << 0;
	tmp[4] |= (pdata[2] / 10) << 4;
	tmp[4] |= (pdata[2] % 10) << 0;
	regmap_bulk_write(dev->regmap, DATE, &tmp[4], 3);

	tmp[2] |= (pdata[3] / 10) << 4;
	tmp[2] |= (pdata[3] % 10) << 0;

	if (dev->time_mode == DS3231_HOURS12) {
		if (dev->time_region == DS3231_PM) {
			tmp[2] |= 0x20;
		} else {
			tmp[2] &= ~0x20;
		}

		tmp[2] |= 0x40;
	} else {
		tmp[2] &= ~0x40;
	}

	tmp[1] |= (pdata[4] / 10) << 4;
	tmp[1] |= (pdata[4] % 10) << 0;
	tmp[0] |= (pdata[5] / 10) << 4;
	tmp[0] |= (pdata[5] % 10) << 0;
	regmap_bulk_write(dev->regmap, SECONDS, tmp, 3);
}

void ds3231_get_time_with_string(struct ds3231_device *dev, char *pdata)
{
	u8 tmp[6];

	ds3231_get_time(dev, tmp, DS3231_FORMAT_BIN);
	pdata[11] = (tmp[0] % 10) + '0';
	pdata[10] = (tmp[0] / 10) + '0';
	pdata[9] = (tmp[1] % 10) + '0';
	pdata[8] = (tmp[1] / 10) + '0';
	pdata[7] = (tmp[2] % 10) + '0';
	pdata[6] = (tmp[2] / 10) + '0';
	pdata[5] = (tmp[3] % 10) + '0';
	pdata[4] = (tmp[3] / 10) + '0';
	pdata[3] = (tmp[4] % 10) + '0';
	pdata[2] = (tmp[4] / 10) + '0';
	pdata[1] = (tmp[5] % 10) + '0';
	pdata[0] = (tmp[5] / 10) + '0';
}

void ds3231_set_time_with_string(struct ds3231_device *dev, char *pdata)
{
	u8 tmp[6];

	tmp[0] = (pdata[0] - '0') * 10 + (pdata[1] - '0');
	tmp[1] = (pdata[2] - '0') * 10 + (pdata[3] - '0');
	tmp[2] = (pdata[4] - '0') * 10 + (pdata[5] - '0');
	tmp[3] = (pdata[6] - '0') * 10 + (pdata[7] - '0');
	tmp[4] = (pdata[8] - '0') * 10 + (pdata[9] - '0');
	tmp[5] = (pdata[10] - '0') * 10 + (pdata[11] - '0');
	ds3231_set_time(dev, tmp, DS3231_FORMAT_BIN);
}

u8 ds3231_get_week(struct ds3231_device *dev)
{
	u32 reg;

	regmap_read(dev->regmap, DAY, &reg);

	return reg;
}

void ds3231_set_week(struct ds3231_device *dev, u8 tmp)
{
	regmap_write(dev->regmap, DAY, tmp);
}

void ds3231_alarm1_callback(void)
{
	;
}

void ds3231_set_alarm1(struct ds3231_device *dev, u8 *pdata, u8 date_or_day, u8 alarm)
{
	u8 tmp[4] = {0, 0, 0, 0};
	u8 tens = 0, unit = 0;

	if (alarm & ALARM_MASK_1) {
		tmp[0] |= ALARM_MASK;
	} else {
		tmp[0] &= ~ALARM_MASK;
	}

	if (alarm & ALARM_MASK_2) {
		tmp[1] |= ALARM_MASK;
	} else {
		tmp[1] &= ~ALARM_MASK;
	}

	if (alarm & ALARM_MASK_3) {
		tmp[2] |= ALARM_MASK;
	} else {
		tmp[2] &= ~ALARM_MASK;
	}

	if (alarm & ALARM_MASK_4) {
		tmp[3] |= ALARM_MASK;
	} else {
		tmp[3] &= ~ALARM_MASK;
	}

	unit = (pdata[0] & 0x7F) % 10;
	tens = (pdata[0] & 0x7F) / 10;
	tmp[0] = (tens << 4) | unit;
	unit = (pdata[1] & 0x7F) % 10;
	tens = (pdata[1] & 0x7F) / 10;
	tmp[1] = (tens << 4) | unit;

	if (dev->time_mode == DS3231_HOURS24) {
		unit = (pdata[2] & 0x3F) % 10;
		tens = (pdata[2] & 0x3F) / 10;
		tmp[2] = (tens << 4) | unit;
		tmp[2] &= ~TIMEMODE_MASK;
	} else {
		if (pdata[2] > 12) {
			pdata[2] -= 12;
		}

		unit = (pdata[2] & 0x1F) % 10;
		tens = (pdata[2] & 0x1F) / 10;
		tmp[2] = (tens << 4) | unit;
		tmp[2] |= TIMEMODE_MASK;
	}

	if (date_or_day == WEEK_CYCLE) {
		unit = (pdata[3] & 0x0F) % 10;
		tmp[3] = unit;
		tmp[3] |= DY_DT_MASK;
	} else if (date_or_day == MONTH_CYCLE) {
		unit = (pdata[3] & 0x0F) % 10;
		tens = (pdata[3] & 0x0F) / 10;
		tmp[3] = (tens << 4) | unit;
		tmp[3] &= ~DY_DT_MASK;
	}

	regmap_bulk_write(dev->regmap, ALARM_1_SECONDS, tmp, 4);
}

void ds3231_alarm2_callback(void)
{
	;
}

void ds3231_set_alarm2(struct ds3231_device *dev, u8 *pdata, u8 date_or_day, u8 alarm)
{
	u8 tmp[3] = {0, 0, 0};
	u8 tens = 0, unit = 0;

	if (alarm & ALARM_MASK_2) {
		tmp[0] |= ALARM_MASK;
	} else {
		tmp[0] &= ~ALARM_MASK;
	}

	if (alarm & ALARM_MASK_3) {
		tmp[1] |= ALARM_MASK;
	} else {
		tmp[1] &= ~ALARM_MASK;
	}

	if (alarm & ALARM_MASK_4) {
		tmp[2] |= ALARM_MASK;
	} else {
		tmp[2] &= ~ALARM_MASK;
	}

	unit = (pdata[0] & 0x7F) % 10;
	tens = (pdata[0] & 0x7F) / 10;
	tmp[0] = (tens << 4) | unit;

	if (dev->time_mode == DS3231_HOURS24) {
		unit = (pdata[1] & 0x3F) % 10;
		tens = (pdata[1] & 0x3F) / 10;
		tmp[1] = (tens << 4) | unit;
		tmp[1] &= ~TIMEMODE_MASK;
	} else {
		if (pdata[1] > 12) {
			pdata[1] -= 12;
		}

		unit = (pdata[1] & 0x1F) % 10;
		tens = (pdata[1] & 0x1F) / 10;
		tmp[1] = (tens << 4) | unit;
		tmp[1] |= TIMEMODE_MASK;
	}

	if (date_or_day == WEEK_CYCLE) {
		unit = (pdata[2] & 0x0F) % 10;
		tmp[2] = unit;
		tmp[2] |= DY_DT_MASK;
	} else if (date_or_day == MONTH_CYCLE) {
		unit = (pdata[2] & 0x0F) % 10;
		tens = (pdata[2] & 0x0F) / 10;
		tmp[2] = (tens << 4) | unit;
		tmp[2] &= ~DY_DT_MASK;
	}

	regmap_bulk_write(dev->regmap, ALARM_2_MINUTES, tmp, 3);
}

void ds3231_enable_oscillator(struct ds3231_device *dev)
{
	u32 reg;

	regmap_read(dev->regmap, CONTROL, &reg);

	reg &= ~(0x01 << 7);
	reg |= (0x00 << 7);

	regmap_write(dev->regmap, CONTROL, reg);
}

void ds3231_convert_temperature(struct ds3231_device *dev)
{
	u32 reg;

	regmap_read(dev->regmap, CONTROL, &reg);

	reg &= ~(0x01 << 6);
	reg |= (0x01 << 6);

	regmap_write(dev->regmap, CONTROL, reg);
}

void ds3231_rate_select(struct ds3231_device *dev, u8 tmp)
{
	u32 reg;

	regmap_read(dev->regmap, CONTROL, &reg);

	switch (tmp) {
	case SQUARE_WAVE_1HZ:
		reg &= ~(0x00 << 3);
		reg |= (0x00 << 3);
		break;

	case SQUARE_WAVE_1_024HZ:
		reg &= ~(0x01 << 3);
		reg |= (0x01 << 3);
		break;

	case SQUARE_WAVE_4_096HZ:
		reg &= ~(0x20 << 3);
		reg |= (0x20 << 3);
		break;

	case SQUARE_WAVE_8_192HZ:
		reg &= ~(0x03 << 3);
		reg |= (0x03 << 3);
		break;

	default:
		break;
	}

	regmap_write(dev->regmap, CONTROL, reg);
}

void ds3231_enable_square_wave_with_bat(struct ds3231_device *dev)
{
	u32 reg;

	regmap_read(dev->regmap, CONTROL, &reg);

	reg &= ~(0x01 << 6);
	reg |= (0x01 << 6);

	regmap_write(dev->regmap, CONTROL, reg);
}

void ds3231_enable_int(struct ds3231_device *dev)
{
	u32 reg;

	regmap_read(dev->regmap, CONTROL, &reg);

	reg &= ~(0x01 << 2);
	reg |= (0x01 << 2);

	regmap_write(dev->regmap, CONTROL, reg);
}

void ds3231_enable_square_wave(struct ds3231_device *dev)
{
	u32 reg;

	regmap_read(dev->regmap, CONTROL, &reg);

	reg &= ~(0x01 << 2);

	regmap_write(dev->regmap, CONTROL, reg);
}

void ds3231_enable_alarm2_int(struct ds3231_device *dev)
{
	u32 reg;

	regmap_read(dev->regmap, CONTROL, &reg);

	reg &= ~(0x01 << 1);
	reg |= (0x01 << 1);

	regmap_write(dev->regmap, CONTROL, reg);
}

void ds3231_enable_alarm1_int(struct ds3231_device *dev)
{
	u32 reg;

	regmap_read(dev->regmap, CONTROL, &reg);

	reg &= ~(0x01 << 0);
	reg |= (0x01 << 0);

	regmap_write(dev->regmap, CONTROL, reg);
}

u8 ds3231_oscillator_stop_flag(struct ds3231_device *dev)
{
	u32 reg;

	regmap_read(dev->regmap, CONTROL_STATUS, &reg);

	if (reg & 0x80) {
		return 1;
	} else {
		return 0;
	}
}

void ds3231_enable_32khz_output(struct ds3231_device *dev)
{
	u32 reg;

	regmap_read(dev->regmap, CONTROL, &reg);

	reg &= ~(0x01 << 3);
	reg |= (0x01 << 3);

	regmap_write(dev->regmap, CONTROL, reg);
}

u8 ds3231_wait_busy(struct ds3231_device *dev)
{
	u32 reg;

	regmap_read(dev->regmap, CONTROL_STATUS, &reg);

	if (reg & 0x02) {
		return 1;
	} else {
		return 0;
	}
}

static u8 alarm2_flag = 0;

u8 ds3231_alarm2_flag(struct ds3231_device *dev)
{
	u32 reg;

	regmap_read(dev->regmap, CONTROL_STATUS, &reg);

	if (reg & 0x02) {
		return 1;
	} else {
		return 0;
	}
}

void ds3231_clear_alarm2_flag(struct ds3231_device *dev)
{
	u32 reg;

	regmap_read(dev->regmap, CONTROL, &reg);

	reg &= ~(0x01 << 1);

	regmap_write(dev->regmap, CONTROL, reg);
}

static u8 alarm1_flag = 0;

u8 ds3231_alarm1_flag(struct ds3231_device *dev)
{
	u32 reg;

	regmap_read(dev->regmap, CONTROL_STATUS, &reg);

	if (reg & 0x01) {
		return 1;
	} else {
		return 0;
	}
}

void ds3231_clear_alarm1_flag(struct ds3231_device *dev)
{
	u32 reg;

	regmap_read(dev->regmap, CONTROL, &reg);

	reg &= ~(0x01 << 0);

	regmap_write(dev->regmap, CONTROL, reg);
}

u8 ds3231_aging_offset(struct ds3231_device *dev)
{
	u32 reg;

	regmap_read(dev->regmap, AGING_OFFSET, &reg);

	return reg;
}

void ds3231_set_aging_offset(struct ds3231_device *dev, u8 offset)
{
	/* Aging offset register is signed 8-bit value
	 * Bit 7: Sign bit (0 = positive, 1 = negative)
	 * Bits 6-0: Magnitude
	 * Range: -128 to +127 (represented as 0x80 to 0x7F)
	 * Each step represents approximately 0.25°C temperature compensation
	 */
	regmap_write(dev->regmap, AGING_OFFSET, offset);
}

void ds3231_get_aging_offset(struct ds3231_device *dev, s32 *paging_offset)
{
	regmap_read(dev->regmap, AGING_OFFSET, (u32 *)paging_offset);
}

float ds3231_get_temperature(struct ds3231_device *dev)
{
	u8 val[2];

	regmap_bulk_read(dev->regmap, MSB_OF_TEMP, val, 2);
	return (float)val[0] + (float)(val[1] >> 6) * 0.25;
}

float ds3231_get_temperature_with_precision(struct ds3231_device *dev)
{
	u8 val[2];
	s16 temp_raw;
	u32 aging_offset;
	float temp_compensated;

	regmap_bulk_read(dev->regmap, MSB_OF_TEMP, val, 2);

	/* Calculate temperature with higher precision */
	temp_raw = ((s16)val[0] << 8) | val[1];
	temp_compensated = (float)temp_raw / 256.0f;

	/* Apply aging offset compensation if available */
	regmap_read(dev->regmap, AGING_OFFSET, &aging_offset);
	if (aging_offset & 0x80) {
		/* Negative offset */
		temp_compensated -= (float)(~aging_offset + 1) / 4.0f;
	} else {
		/* Positive offset */
		temp_compensated += (float)aging_offset / 4.0f;
	}

	return temp_compensated;
}

void ds3231_set_temperature_compensation(struct ds3231_device *dev, float offset)
{
	s8 aging_offset;

	/* Convert float offset to aging offset value */
	if (offset >= 0.0f) {
		aging_offset = (s8)(offset * 4.0f);
		if (aging_offset > 127) {
			aging_offset = 127;
		}
	} else {
		aging_offset = (s8)(offset * 4.0f);
		if (aging_offset < -128) {
			aging_offset = -128;
		}
	}

	regmap_write(dev->regmap, AGING_OFFSET, (u8)aging_offset);
}

float ds3231_get_temperature_compensation(struct ds3231_device *dev)
{
	s32 aging_offset;

	regmap_read(dev->regmap, AGING_OFFSET, (u32 *)&aging_offset);

	if (aging_offset & 0x80) {
		/* Negative offset */
		return -(float)(~aging_offset + 1) / 4.0f;
	} else {
		/* Positive offset */
		return (float)aging_offset / 4.0f;
	}
}

void ds3231_check_temperature_status(struct ds3231_device *dev, u8 *pstatus)
{
	/* Check temperature-related status flags
	 * Return status information about temperature monitoring
	 */
	u32 reg;
	float temp;
	static float last_temp = 0.0f;
	u8 current_status = 0;

	/* Check hardware status flags */
	regmap_read(dev->regmap, CONTROL_STATUS, &reg);

	if (reg & DS3231_STATUS_BSY_BIT) {
		current_status |= 0x01; /* Temperature conversion busy */
	}

	/* Get current temperature and perform advanced analysis */
	temp = ds3231_get_temperature_with_precision(dev);

	/* Check for significant temperature change */
	if (fabs(temp - last_temp) > 1.0f) {
		current_status |= 0x02; /* Temperature changed significantly */
	}

	/* Check for extreme temperatures */
	if (temp > 60.0f) {
		current_status |= 0x04; /* High temperature */
	} else if (temp < -20.0f) {
		current_status |= 0x08; /* Low temperature */
	}

	/* Check for temperature out of operating range */
	if (temp > 70.0f || temp < -40.0f) {
		current_status |= 0x10; /* Out of operating range */
	}

	last_temp = temp;
	*pstatus = current_status;
}

void ds3231_enable_auto_temperature_compensation(struct ds3231_device *dev)
{
	u32 reg;

	regmap_read(dev->regmap, CONTROL, &reg);

	/* Clear CONV bit to enable automatic conversion */
	reg &= ~DS3231_CONTROL_CONV_BIT;

	regmap_write(dev->regmap, CONTROL, reg);
}

void ds3231_disable_auto_temperature_compensation(struct ds3231_device *dev)
{
	u32 reg;

	regmap_read(dev->regmap, CONTROL, &reg);

	/* Set CONV bit to disable automatic conversion */
	reg |= DS3231_CONTROL_CONV_BIT;

	regmap_write(dev->regmap, CONTROL, reg);
}

u8 ds3231_is_temperature_conversion_busy(struct ds3231_device *dev)
{
	u32 reg;

	regmap_read(dev->regmap, CONTROL_STATUS, &reg);

	return (reg & DS3231_STATUS_BSY_BIT) ? 1 : 0;
}

void ds3231_wait_for_temperature_conversion(struct ds3231_device *dev)
{
	u32 timeout = 0;
	const u32 max_timeout = 10000; /* 10 seconds timeout */

	while (ds3231_is_temperature_conversion_busy(dev) && timeout < max_timeout) {
		mdelay(1);
		timeout++;
	}
}

void ds3231_trigger_temperature_conversion(struct ds3231_device *dev)
{
	u32 reg;

	regmap_read(dev->regmap, CONTROL, &reg);

	/* Set CONV bit to trigger temperature conversion */
	reg |= DS3231_CONTROL_CONV_BIT;

	regmap_write(dev->regmap, CONTROL, reg);

	/* Wait for conversion to complete */
	ds3231_wait_for_temperature_conversion(dev);
}

void ds3231_calibrate_temperature(struct ds3231_device *dev)
{
	float current_temp;
	float reference_temp = 25.0f; /* Room temperature reference */
	float offset;

	/* Trigger temperature conversion */
	ds3231_trigger_temperature_conversion(dev);

	/* Wait for conversion to complete */
	ds3231_wait_for_temperature_conversion(dev);

	/* Get current temperature */
	current_temp = ds3231_get_temperature_with_precision(dev);

	/* Calculate offset needed */
	offset = reference_temp - current_temp;

	/* Set the compensation offset */
	ds3231_set_temperature_compensation(dev, offset);
}

void ds3231_disable_oscillator(struct ds3231_device *dev)
{
	u32 reg;

	regmap_read(dev->regmap, CONTROL, &reg);

	/* Set EOSC bit to disable oscillator */
	reg |= DS3231_CONTROL_EOSC_BIT;

	regmap_write(dev->regmap, CONTROL, reg);
}

void ds3231_enable_battery_backup(struct ds3231_device *dev)
{
	u32 reg;

	regmap_read(dev->regmap, CONTROL, &reg);

	/* Clear BBSQW bit to enable battery backup square wave */
	reg &= ~DS3231_CONTROL_BBSQW_BIT;

	regmap_write(dev->regmap, CONTROL, reg);
}

void ds3231_disable_battery_backup(struct ds3231_device *dev)
{
	u32 reg;

	regmap_read(dev->regmap, CONTROL, &reg);

	/* Set BBSQW bit to disable battery backup square wave */
	reg |= DS3231_CONTROL_BBSQW_BIT;

	regmap_write(dev->regmap, CONTROL, reg);
}

u8 ds3231_is_battery_backup_enabled(struct ds3231_device *dev)
{
	u32 reg;

	regmap_read(dev->regmap, CONTROL, &reg);

	return (reg & DS3231_CONTROL_BBSQW_BIT) ? 0 : 1;
}

void ds3231_reset_oscillator_stop_flag(struct ds3231_device *dev)
{
	u32 reg;

	regmap_read(dev->regmap, CONTROL_STATUS, &reg);

	/* Clear OSF bit */
	reg &= ~DS3231_STATUS_OSF_BIT;

	regmap_write(dev->regmap, CONTROL_STATUS, reg);
}

void ds3231_set_32khz_output(struct ds3231_device *dev, u8 enable)
{
	u32 reg;

	regmap_read(dev->regmap, CONTROL_STATUS, &reg);

	if (enable) {
		reg |= DS3231_STATUS_EN32KHZ_BIT;
	} else {
		reg &= ~DS3231_STATUS_EN32KHZ_BIT;
	}

	regmap_write(dev->regmap, CONTROL_STATUS, reg);
}

u8 ds3231_is_32khz_output_enabled(struct ds3231_device *dev)
{
	u32 reg;

	regmap_read(dev->regmap, CONTROL_STATUS, &reg);

	return (reg & DS3231_STATUS_EN32KHZ_BIT) ? 1 : 0;
}

void ds3231_configure_square_wave(struct ds3231_device *dev, u8 frequency, u8 battery_backup)
{
	u32 reg;

	regmap_read(dev->regmap, CONTROL, &reg);

	/* Configure frequency bits */
	reg &= ~(DS3231_CONTROL_RS1_BIT | DS3231_CONTROL_RS2_BIT);
	switch (frequency) {
	case DS3231_SQW_FREQ_1HZ:
		/* Both RS1 and RS2 are 0 - default */
		break;
	case DS3231_SQW_FREQ_1024HZ:
		reg |= DS3231_CONTROL_RS1_BIT;
		break;
	case DS3231_SQW_FREQ_4096HZ:
		reg |= DS3231_CONTROL_RS2_BIT;
		break;
	case DS3231_SQW_FREQ_8192HZ:
		reg |= DS3231_CONTROL_RS1_BIT | DS3231_CONTROL_RS2_BIT;
		break;
	default:
		break;
	}

	/* Configure battery backup square wave */
	if (battery_backup) {
		reg &= ~DS3231_CONTROL_BBSQW_BIT;
	} else {
		reg |= DS3231_CONTROL_BBSQW_BIT;
	}

	regmap_write(dev->regmap, CONTROL, reg);
}

void ds3231_enable_interrupt_mode(struct ds3231_device *dev)
{
	u32 reg;

	regmap_read(dev->regmap, CONTROL, &reg);

	/* Set INTCN bit to enable interrupt mode */
	reg |= DS3231_CONTROL_INTCN_BIT;

	regmap_write(dev->regmap, CONTROL, reg);
}

void ds3231_disable_interrupt_mode(struct ds3231_device *dev)
{
	u32 reg;

	regmap_read(dev->regmap, CONTROL, &reg);

	/* Clear INTCN bit to disable interrupt mode (square wave mode) */
	reg &= ~DS3231_CONTROL_INTCN_BIT;

	regmap_write(dev->regmap, CONTROL, reg);
}

u8 ds3231_is_interrupt_mode_enabled(struct ds3231_device *dev)
{
	u32 reg;

	regmap_read(dev->regmap, CONTROL, &reg);

	return (reg & DS3231_CONTROL_INTCN_BIT) ? 1 : 0;
}

void ds3231_emergency_oscillator_enable(struct ds3231_device *dev)
{
	/* This function enables the oscillator in emergency conditions */
	ds3231_enable_oscillator(dev);

	/* Clear any oscillator stop flag */
	ds3231_reset_oscillator_stop_flag(dev);

	/* Enable 32kHz output */
	ds3231_set_32khz_output(dev, 1);
}

u8 ds3231_check_power_status(struct ds3231_device *dev)
{
	u8 status = 0;
	u32 control_status = 0;

	regmap_read(dev->regmap, CONTROL_STATUS, &control_status);

	/* Check oscillator stop flag */
	if (control_status & DS3231_STATUS_OSF_BIT) {
		status |= 0x01; /* Oscillator stopped */
	}

	/* Check if device is running on battery backup */
	/* Note: ds3231 doesn't directly report this, so we infer from other indicators */
	if (control_status & DS3231_STATUS_BSY_BIT) {
		status |= 0x02; /* Busy state - may indicate battery operation */
	}

	return status;
}

void ds3231_soft_reset(struct ds3231_device *dev)
{
	u32 reg;

	/* Save current control register settings */
	regmap_read(dev->regmap, CONTROL, &reg);

	/* Clear all control bits except INTCN */
	u8 new_reg = reg & DS3231_CONTROL_INTCN_BIT;

	/* Temporarily set all bits */
	reg |= 0xFF;
	regmap_write(dev->regmap, CONTROL, reg);

	mdelay(10);

	/* Restore control register with proper configuration */
	regmap_write(dev->regmap, CONTROL, new_reg);

	/* Reset alarm flags */
	ds3231_clear_alarm1_flag(dev);
	ds3231_clear_alarm2_flag(dev);
}

void ds3231_disable_alarm1(struct ds3231_device *dev)
{
	u32 reg;

	regmap_read(dev->regmap, CONTROL, &reg);

	/* Clear A1IE bit to disable alarm 1 */
	reg &= ~DS3231_CONTROL_A1IE_BIT;

	regmap_write(dev->regmap, CONTROL, reg);
}

void ds3231_disable_alarm2(struct ds3231_device *dev)
{
	u32 reg;

	regmap_read(dev->regmap, CONTROL, &reg);

	/* Clear A2IE bit to disable alarm 2 */
	reg &= ~DS3231_CONTROL_A2IE_BIT;

	regmap_write(dev->regmap, CONTROL, reg);
}

void ds3231_enable_alarm1_mask(struct ds3231_device *dev, u8 mask)
{
	u32 reg;

	regmap_read(dev->regmap, CONTROL, &reg);

	/* Set A1IE bit to enable alarm 1 */
	reg |= DS3231_CONTROL_A1IE_BIT;

	regmap_write(dev->regmap, CONTROL, reg);
}

void ds3231_enable_alarm2_mask(struct ds3231_device *dev, u8 mask)
{
	u32 reg;

	regmap_read(dev->regmap, CONTROL, &reg);

	/* Set A2IE bit to enable alarm 2 */
	reg |= DS3231_CONTROL_A2IE_BIT;

	regmap_write(dev->regmap, CONTROL, reg);
}

void ds3231_get_alarm1_status(struct ds3231_device *dev, u8 *pstatus)
{
	u32 reg;

	regmap_read(dev->regmap, CONTROL_STATUS, &reg);

	*pstatus = (reg & DS3231_STATUS_A1F_BIT) ? 1 : 0;
}

void ds3231_get_alarm2_status(struct ds3231_device *dev, u8 *pstatus)
{
	u32 reg;

	regmap_read(dev->regmap, CONTROL_STATUS, &reg);

	*pstatus = (reg & DS3231_STATUS_A2F_BIT) ? 1 : 0;
}

void ds3231_clear_all_alarm_flags(struct ds3231_device *dev)
{
	ds3231_clear_alarm1_flag(dev);
	ds3231_clear_alarm2_flag(dev);
}

void ds3231_set_alarm1_with_seconds(struct ds3231_device *dev, u8 seconds, u8 minutes, u8 hours, u8 day_date, u8 mask)
{
	u8 tmp[4] = {0, 0, 0, 0};

	/* Convert values to BCD format */
	tmp[0] = ((seconds / 10) << 4) | (seconds % 10);
	tmp[1] = ((minutes / 10) << 4) | (minutes % 10);

	if (dev->time_mode == DS3231_HOURS12) {
		if (hours > 12) {
			hours -= 12;
		}
		tmp[2] = ((hours / 10) << 4) | (hours % 10);
		tmp[2] |= DS3231_HOURS_12_24_BIT;
		if (dev->time_region == DS3231_PM) {
			tmp[2] |= DS3231_HOURS_AM_PM_BIT;
		}
	} else {
		tmp[2] = ((hours / 10) << 4) | (hours % 10);
		tmp[2] &= ~DS3231_HOURS_12_24_BIT;
	}

	if (day_date & DS3231_ALARM_DAY) {
		tmp[3] = (day_date & 0x0F);
		tmp[3] |= DS3231_ALARM_DY_DT_BIT;
	} else {
		tmp[3] = ((day_date / 10) << 4) | (day_date % 10);
		tmp[3] &= ~DS3231_ALARM_DY_DT_BIT;
	}

	/* Apply mask settings */
	if (mask & DS3231_ALARM_MASK_SECOND) {
		tmp[0] |= DS3231_ALARM_MASK;
	}
	if (mask & DS3231_ALARM_MASK_MINUTE) {
		tmp[1] |= DS3231_ALARM_MASK;
	}
	if (mask & DS3231_ALARM_MASK_HOUR) {
		tmp[2] |= DS3231_ALARM_MASK;
	}
	if (mask & DS3231_ALARM_MASK_DAY_DATE) {
		tmp[3] |= DS3231_ALARM_MASK;
	}

	regmap_bulk_write(dev->regmap, ALARM_1_SECONDS, tmp, 4);
}

void ds3231_set_alarm2_with_minutes(struct ds3231_device *dev, u8 minutes, u8 hours, u8 day_date, u8 mask)
{
	u8 tmp[3] = {0, 0, 0};

	/* Convert values to BCD format */
	tmp[0] = ((minutes / 10) << 4) | (minutes % 10);

	if (dev->time_mode == DS3231_HOURS12) {
		if (hours > 12) {
			hours -= 12;
		}
		tmp[1] = ((hours / 10) << 4) | (hours % 10);
		tmp[1] |= DS3231_HOURS_12_24_BIT;
		if (dev->time_region == DS3231_PM) {
			tmp[1] |= DS3231_HOURS_AM_PM_BIT;
		}
	} else {
		tmp[1] = ((hours / 10) << 4) | (hours % 10);
		tmp[1] &= ~DS3231_HOURS_12_24_BIT;
	}

	if (day_date & DS3231_ALARM_DAY) {
		tmp[2] = (day_date & 0x0F);
		tmp[2] |= DS3231_ALARM_DY_DT_BIT;
	} else {
		tmp[2] = ((day_date / 10) << 4) | (day_date % 10);
		tmp[2] &= ~DS3231_ALARM_DY_DT_BIT;
	}

	/* Apply mask settings */
	if (mask & DS3231_ALARM_MASK_MINUTE) {
		tmp[0] |= DS3231_ALARM_MASK;
	}
	if (mask & DS3231_ALARM_MASK_HOUR) {
		tmp[1] |= DS3231_ALARM_MASK;
	}
	if (mask & DS3231_ALARM_MASK_DAY_DATE) {
		tmp[2] |= DS3231_ALARM_MASK;
	}

	regmap_bulk_write(dev->regmap, ALARM_2_MINUTES, tmp, 3);
}

u8 ds3231_check_alarm_interrupt_source(struct ds3231_device *dev)
{
	u32 reg;
	u8 source = 0;

	regmap_read(dev->regmap, CONTROL_STATUS, &reg);

	if (reg & DS3231_STATUS_A1F_BIT) {
		source |= 0x01; /* Alarm 1 triggered */
	}
	if (reg & DS3231_STATUS_A2F_BIT) {
		source |= 0x02; /* Alarm 2 triggered */
	}

	return source;
}

void ds3231_disable_all_alarms(struct ds3231_device *dev)
{
	ds3231_disable_alarm1(dev);
	ds3231_disable_alarm2(dev);
	ds3231_clear_all_alarm_flags(dev);
}

void ds3231_enable_matched_alarm_only(struct ds3231_device *dev, u8 alarm_number)
{
	ds3231_disable_all_alarms(dev);

	if (alarm_number == 1) {
		ds3231_enable_alarm1_mask(dev, 0xFF);
	} else if (alarm_number == 2) {
		ds3231_enable_alarm2_mask(dev, 0xFF);
	}
}

u8 ds3231_get_alarm1_configuration(struct ds3231_device *dev, u8 *pseconds, u8 *pminutes, u8 *phours, u8 *pday_date, u8 *pmask)
{
	u8 tmp[4] = {0};
	u8 ret = 0;

	regmap_bulk_write(dev->regmap, ALARM_1_SECONDS, tmp, 4);

	/* Extract values */
	*pseconds = ((tmp[0] & 0x70) >> 4) * 10 + (tmp[0] & 0x0F);
	*pminutes = ((tmp[1] & 0x70) >> 4) * 10 + (tmp[1] & 0x0F);

	if (tmp[2] & DS3231_HOURS_12_24_BIT) {
		/* 12-hour mode */
		*phours = ((tmp[2] & 0x10) >> 4) * 10 + (tmp[2] & 0x0F);
		if (tmp[2] & DS3231_HOURS_AM_PM_BIT) {
			*phours += 12; /* PM */
		}
		dev->time_mode = DS3231_HOURS12;
	} else {
		/* 24-hour mode */
		*phours = ((tmp[2] & 0x30) >> 4) * 10 + (tmp[2] & 0x0F);
		dev->time_mode = DS3231_HOURS24;
	}

	if (tmp[3] & DS3231_ALARM_DY_DT_BIT) {
		*pday_date = (tmp[3] & 0x0F) | DS3231_ALARM_DAY;
	} else {
		*pday_date = ((tmp[3] & 0x30) >> 4) * 10 + (tmp[3] & 0x0F);
	}

	/* Extract mask */
	*pmask = 0;
	if (tmp[0] & DS3231_ALARM_MASK) {
		*pmask |= DS3231_ALARM_MASK_SECOND;
	}
	if (tmp[1] & DS3231_ALARM_MASK) {
		*pmask |= DS3231_ALARM_MASK_MINUTE;
	}
	if (tmp[2] & DS3231_ALARM_MASK) {
		*pmask |= DS3231_ALARM_MASK_HOUR;
	}
	if (tmp[3] & DS3231_ALARM_MASK) {
		*pmask |= DS3231_ALARM_MASK_DAY_DATE;
	}

	return ret;
}

static const struct regmap_range ds3231_volatile_ranges[] = {
	regmap_reg_range(0x00, 0x06),  /* Time/date registers */
	regmap_reg_range(0x0F, 0x0F),  /* Control status */
	regmap_reg_range(0x11, 0x12),  /* Temperature registers */
};

static const struct regmap_access_table ds3231_volatile_table = {
	.yes_ranges = ds3231_volatile_ranges,
	.n_yes_ranges = ARRAY_SIZE(ds3231_volatile_ranges),
};

static const struct regmap_config ds3231_regmap_config = {
	.reg_bits = 8,
	.val_bits = 8,
	.max_register = 0x12,  /* LSB_OF_TEMP */
	.volatile_table = &ds3231_volatile_table,
	.cache_type = REGCACHE_NONE,
};

/* Box-Muller transform for Gaussian noise */
static float ds3231_gaussian_noise(float mean, float std_dev)
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

static void *ds3231_reg_random_thread(void *arg)
{
	struct ds3231_device *dev = arg;
	static u8 seconds = 0, minutes = 0, hours = 0;
	static u8 day = 1, date = 1, month = 1, year = 24;
	static u32 tick_count = 0;
	s16 temperature;

	/* Initialize fixed registers at thread startup */
	dev->slave_regs[DS3231_REG_CONTROL] = 0x1C;  /* INTCN=1, BBSQW=1 */
	dev->slave_regs[DS3231_REG_CONTROL_STATUS] = 0x00;
	dev->slave_regs[DS3231_REG_AGING_OFFSET] = 0x00;

	while (dev->thread_running) {
		/* Update time every second */
		tick_count++;

		if (tick_count % 100 == 0) {  /* 100 ticks = 1 second */
			seconds++;

			/* Handle minute rollover */
			if (seconds >= 60) {
				seconds = 0;
				minutes++;

				/* Handle hour rollover */
				if (minutes >= 60) {
					minutes = 0;
					hours++;

					/* Handle day rollover */
					if (hours >= 24) {
						hours = 0;
						day = (day % 7) + 1;
						date++;

						/* Handle month rollover */
						if (date > 31) {
							date = 1;
							month++;

							/* Handle year rollover */
							if (month > 12) {
								month = 1;
								year = (year + 1) % 100;
							}
						}
					}
				}
			}

			/* Convert to BCD format */
			dev->slave_regs[DS3231_REG_SECONDS] = ((seconds / 10) << 4) | (seconds % 10);
			dev->slave_regs[DS3231_REG_MINUTES] = ((minutes / 10) << 4) | (minutes % 10);
			dev->slave_regs[DS3231_REG_HOURS] = ((hours / 10) << 4) | (hours % 10);
			dev->slave_regs[DS3231_REG_DAY] = day;
			dev->slave_regs[DS3231_REG_DATE] = ((date / 10) << 4) | (date % 10);
			dev->slave_regs[DS3231_REG_MONTH_CENTURY] = ((month / 10) << 4) | (month % 10);
			dev->slave_regs[DS3231_REG_YEAR] = ((year / 10) << 4) | (year % 10);
		}

		/* Update temperature every 100ms with noise */
		temperature = (s16)ds3231_gaussian_noise(25.0f, 0.5f);

		/* Convert temperature to DS3231 format (0.25°C/LSB) */
		dev->slave_regs[DS3231_REG_TEMP_MSB] = (u8)((temperature >> 6) & 0xFF);
		dev->slave_regs[DS3231_REG_TEMP_LSB] = (u8)((temperature << 2) & 0xC0);

		/* Sleep for 10ms (100Hz update rate for temp, 1Hz for time) */
		msleep(10);
	}

	return 0;
}

static pthread_t reg_thread;

static int ds3231_start_reg_random(struct ds3231_device *dev)
{
	int ret;

	dev->thread_running = true;

	ret = pthread_create(&reg_thread, NULL,
			ds3231_reg_random_thread, dev);
	if (ret) {
		return -ret;
	}

	/* Wait for thread to initialize */
	msleep(100);

	return 0;
}

void ds3231_stop_reg_random(struct ds3231_device *dev)
{
	dev->thread_running = false;
	pthread_join(reg_thread, NULL);
}

struct ds3231_device *ds3231_init(enum regmap_user_bus_type bus_type, void *bus_master)
{
	struct ds3231_device *dev;
	int ret;

	dev = kzalloc(sizeof(*dev), GFP_KERNEL);
	if (!dev) {
		return ERR_PTR(-ENOMEM);
	}

	/* Initialize regmap based on bus type */
	if (bus_type == REGMAP_BUS_IIC_SOFT) {
		dev->regmap = regmap_init_iic_soft(bus_master, DS3231_ADDR, &ds3231_regmap_config);
	} else if (bus_type == REGMAP_BUS_SPI_SOFT) {
		/* Default SPI attributes: CPOL=0, CPHA=0, MSB first */
		dev->regmap = regmap_init_spi_soft(bus_master,
						   0, 0, SPI_BIT_ORDER_MSB, 2, &ds3231_regmap_config);
	} else {
		pr_err("Invalid bus type specified for ds3231 initialization\n");
		kfree(dev);
		return ERR_PTR(-EINVAL);
	}

	if (IS_ERR(dev->regmap)) {
		kfree(dev);
		return NULL;
	}

	/* Allocate register space for slave simulation */
	dev->slave_regs = kmalloc(ds3231_regmap_config.max_register + 1, GFP_KERNEL);
	if (!dev->slave_regs) {
		regmap_exit(dev->regmap);
		kfree(dev);
		return ERR_PTR(-ENOMEM);
	}

	/* Initialize simulated registers with default values */
	memset(dev->slave_regs, 0, ds3231_regmap_config.max_register + 1);
	dev->slave_regs[DS3231_REG_SECONDS] = 0x00;
	dev->slave_regs[DS3231_REG_CONTROL] = 0x1C;  /* INTCN=1, A1IE=0, A2IE=0, BBSQW=1 */
	dev->slave_regs[DS3231_REG_TEMP_MSB] = 0x0D;
	dev->slave_regs[DS3231_REG_TEMP_LSB] = 0x48;  /* ~25°C */
	ret = ds3231_start_reg_random(dev);
	if (ret) {
		return NULL;
	}

	/* Initialize slave for testing */
	if (bus_type == REGMAP_BUS_SPI_SOFT) {
		dev->spi_slave = spi_slave_soft_init("ds3231", 0, 0, SPI_BIT_ORDER_MSB,
				dev->slave_regs, ds3231_regmap_config.max_register + 1);
		if (IS_ERR(dev->spi_slave)) {
			pr_err("ds3231: Failed to initialize SPI slave\n");
			kfree(dev->slave_regs);
			regmap_exit(dev->regmap);
			kfree(dev);
			return NULL;
		}
	} else {
		dev->iic_slave = iic_slave_soft_init("ds3231", DS3231_ADDR,
				dev->slave_regs, ds3231_regmap_config.max_register + 1);
		if (IS_ERR(dev->iic_slave)) {
			pr_err("ds3231: Failed to initialize I2C slave\n");
			kfree(dev->slave_regs);
			regmap_exit(dev->regmap);
			kfree(dev);
			return NULL;
		}
	}

	/* Initialize default time mode and region */
	dev->time_mode = DS3231_HOURS24;
	dev->time_region = DS3231_FORMAT_BCD;

	/* Enable oscillator */
	ds3231_enable_oscillator(dev);

	/* Reset oscillator stop flag */
	ds3231_reset_oscillator_stop_flag(dev);

	/* Configure for 1Hz square wave output */
	ds3231_configure_square_wave(dev, DS3231_SQW_FREQ_1HZ, 1);

	/* Enable interrupt mode */
	ds3231_enable_interrupt_mode(dev);

	/* Enable automatic temperature compensation */
	ds3231_enable_auto_temperature_compensation(dev);

	/* Enable battery backup square wave */
	ds3231_enable_battery_backup(dev);

	/* Disable 32kHz output by default */
	ds3231_set_32khz_output(dev, 0);

	pr_info("ds3231 device initialized successfully\n");

	return dev;
}

void ds3231_exit(struct ds3231_device *dev)
{
	ds3231_stop_reg_random(dev);

	if (dev->iic_slave) {
		iic_slave_soft_exit(dev->iic_slave);
	}

	if (dev->spi_slave) {
		spi_slave_soft_exit(dev->spi_slave);
	}

	regmap_exit(dev->regmap);
	kfree(dev->slave_regs);
	kfree(dev);
}

#ifdef DESIGN_VERIFICATION_DS3231
#include "kinetis/test-kinetis.h"

static struct ds3231_device *ds3231_dev;

int t_ds3231_initialize(int argc, char **argv)
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
		pr_info("starting ds3231 slave with %s mode", bus_type ? "spi" : "i2c");
		ds3231_dev = ds3231_init(bus_type, bus_type == REGMAP_BUS_IIC_SOFT ? (void *)&fake_iic_master : (void *)&fake_spi_master);
		if (IS_ERR_OR_NULL(ds3231_dev)) {
			return -EINVAL;
		}
		return 0;
	}

	ds3231_exit(ds3231_dev);
	return 0;
}

int t_ds3231_set_clock(int argc, char **argv)
{
	struct ds3231_device *dev = ds3231_dev;
	char time[16];

	dev->time_mode = DS3231_HOURS24;
	dev->time_region = DS3231_PM;
	ds3231_set_time_with_string(dev, "200308202020");
	ds3231_set_week(dev, 7);

	ds3231_get_time_with_string(dev, time);

	if (strcmp("200308202020", time) != 0) {
		return false;
	}

	if (dev->time_region == DS3231_AM) {
		snprintf(&time[12], 4, " DS3231_AM");
	} else {
		snprintf(&time[12], 4, " PM");
	}

	pr_debug("%s", time);

	return 0;
}

int t_ds3231_get_clock(int argc, char **argv)
{
	struct ds3231_device *dev = ds3231_dev;
	char time[16];
	u8 i = 0;
	u16 times = 1;

	if (argc > 1) {
		times = simple_strtoul(argv[1], &argv[1], 10);
	}

	for (i = 0; i < times; i++) {
		ds3231_get_time_with_string(dev, time);

		if (dev->time_region == DS3231_AM) {
			snprintf(&time[12], 4, " DS3231_AM");
		} else {
			snprintf(&time[12], 4, " PM");
		}

		pr_debug("%s", time);
		mdelay(1000);
	}

	return 0;
}

int t_ds3231_set_alarm1(int argc, char **argv)
{
	struct ds3231_device *dev = ds3231_dev;
	u8 ret = 0;
	u8 dy_dt = 0;
	u8 a1m4 = 0, a1m3 = 0, a1m2 = 0, a1m1 = 0;
	u8 alarm_rate = 0;
	u8 tmp[4] = {0, 0, 0, 0};
	u8 time[7] = {0, 0, 0, 0, 0, 0, 0};
	u8 flag;
	u32 delta = 0;

	if (argc > 1) {
		dy_dt = simple_strtoul(argv[1], &argv[1], 10);
	}

	if (argc > 2) {
		a1m4 = simple_strtoul(argv[2], &argv[2], 10);
	}

	if (argc > 3) {
		a1m3 = simple_strtoul(argv[3], &argv[3], 10);
	}

	if (argc > 4) {
		a1m2 = simple_strtoul(argv[4], &argv[4], 10);
	}

	if (argc > 5) {
		a1m1 = simple_strtoul(argv[5], &argv[5], 10);
	}

	alarm_rate = (a1m4 << 3) | (a1m3 << 2) | (a1m2 << 1) | a1m1;

	ds3231_enable_int(dev);
	ds3231_enable_alarm1_int(dev);

	switch (alarm_rate) {
	case 0x0F:
		ds3231_set_alarm1(dev, tmp, 0,
			ALARM_MASK_1 | ALARM_MASK_2 | ALARM_MASK_3 | ALARM_MASK_4);
		delta = basic_timer_get_ss();
		readb_poll_timeout_atomic(&alarm1_flag, flag, flag == true, 1, 2000);
		delta = basic_timer_get_ss() - delta;
		ds3231_clear_alarm1_flag(dev);

		if (delta == 1) {
			ret = 0;
		} else {
			ret = -1;
		}

		break;

	case 0x0E:
		tmp[0] = 0;
		ds3231_set_alarm1(dev, tmp, 0,
			ALARM_MASK_1 | ALARM_MASK_2 | ALARM_MASK_3);
		ds3231_get_time(dev, time, DS3231_FORMAT_BIN);
		ds3231_clear_alarm1_flag(dev);

		if (time[6] == 0) {
			ret = 0;
		} else {
			ret = -1;
		}

		break;

	case 0x0C:
		tmp[0] = 0;
		tmp[1] = 0;
		ds3231_set_alarm1(dev, tmp, 0,
			ALARM_MASK_1 | ALARM_MASK_2);
		ds3231_get_time(dev, time, DS3231_FORMAT_BIN);
		ds3231_clear_alarm1_flag(dev);

		if (time[5] == 0 && time[6] == 0) {
			ret = 0;
		} else {
			ret = -1;
		}

		break;

	case 0x08:
		tmp[0] = 0;
		tmp[1] = 0;
		tmp[2] = 0;
		ds3231_set_alarm1(dev, tmp, 0,
			ALARM_MASK_1);
		ds3231_get_time(dev, time, DS3231_FORMAT_BIN);
		ds3231_clear_alarm1_flag(dev);

		if (time[4] == 0 && time[5] == 0 && time[6] == 0) {
			ret = 0;
		} else {
			ret = -1;
		}

		break;

	case 0x00:
		if (dy_dt) {
			tmp[0] = 0;
			tmp[1] = 0;
			tmp[2] = 0;
			tmp[3] = 0;
			ds3231_set_alarm1(dev, tmp, 0,
				0);
			ds3231_get_time(dev, time, DS3231_FORMAT_BIN);
			ds3231_clear_alarm1_flag(dev);

			if (time[3] == 0 && time[4] == 0 && time[5] == 0 && time[6] == 0) {
				ret = 0;
			} else {
				ret = -1;
			}
		} else {
			tmp[0] = 0;
			tmp[1] = 0;
			tmp[2] = 0;
			tmp[3] = 0;
			ds3231_set_alarm1(dev, tmp, 0,
				0);
			ds3231_get_time(dev, time, DS3231_FORMAT_BIN);
			ds3231_clear_alarm1_flag(dev);

			if (time[3] == 0 && time[4] == 0 && time[5] == 0 && time[6] == 0) {
				ret = 0;
			} else {
				ret = -1;
			}
		}

		break;
	}

	return ret;
}

int t_ds3231_set_alarm2(int argc, char **argv)
{
	struct ds3231_device *dev = ds3231_dev;
	u8 ret = 0;
	u8 dy_dt = 0;
	u8 a1m4 = 0, a1m3 = 0, a1m2 = 0, a1m1 = 0;
	u8 alarm_rate = 0;
	u8 tmp[4] = {0, 0, 0, 0};
	u8 time[7] = {0, 0, 0, 0, 0, 0, 0};
	u8 flag;
	u32 delta = 0;

	if (argc > 1) {
		dy_dt = simple_strtoul(argv[1], &argv[1], 10);
	}

	if (argc > 2) {
		a1m4 = simple_strtoul(argv[2], &argv[2], 10);
	}

	if (argc > 3) {
		a1m3 = simple_strtoul(argv[3], &argv[3], 10);
	}

	if (argc > 4) {
		a1m2 = simple_strtoul(argv[4], &argv[4], 10);
	}

	if (argc > 5) {
		a1m1 = simple_strtoul(argv[5], &argv[5], 10);
	}

	alarm_rate = (a1m4 << 3) | (a1m3 << 2) | (a1m2 << 1) | a1m1;

	ds3231_enable_int(dev);
	ds3231_enable_alarm2_int(dev);

	switch (alarm_rate) {
	case 0x0F:
		ds3231_set_alarm2(dev, tmp, 0,
			ALARM_MASK_1 | ALARM_MASK_2 | ALARM_MASK_3 | ALARM_MASK_4);
		delta = basic_timer_get_ss();
		readb_poll_timeout_atomic(&alarm2_flag, flag, flag == true, 1, 2000);
		delta = basic_timer_get_ss() - delta;
		ds3231_clear_alarm2_flag(dev);

		if (delta == 1) {
			ret = true;
		} else {
			ret = false;
		}

		break;

	case 0x0E:
		tmp[0] = 0;
		ds3231_set_alarm2(dev, tmp, 0,
			ALARM_MASK_1 | ALARM_MASK_2 | ALARM_MASK_3);
		ds3231_get_time(dev, time, DS3231_FORMAT_BIN);
		ds3231_clear_alarm2_flag(dev);

		if (time[6] == 0) {
			ret = true;
		} else {
			ret = false;
		}

		break;

	case 0x0C:
		tmp[0] = 0;
		tmp[1] = 0;
		ds3231_set_alarm2(dev, tmp, 0, ALARM_MASK_1 | ALARM_MASK_2);
		ds3231_get_time(dev, time, DS3231_FORMAT_BIN);
		ds3231_clear_alarm2_flag(dev);

		if (time[5] == 0 && time[6] == 0) {
			ret = true;
		} else {
			ret = false;
		}

		break;

	case 0x08:
		tmp[0] = 0;
		tmp[1] = 0;
		tmp[2] = 0;
		ds3231_set_alarm2(dev, tmp, 0, ALARM_MASK_1);
		ds3231_get_time(dev, time, DS3231_FORMAT_BIN);
		ds3231_clear_alarm2_flag(dev);

		if (time[4] == 0 && time[5] == 0 && time[6] == 0) {
			ret = true;
		} else {
			ret = false;
		}

		break;

	case 0x00:
		if (dy_dt) {
			tmp[0] = 0;
			tmp[1] = 0;
			tmp[2] = 0;
			tmp[3] = 0;
			ds3231_set_alarm2(dev, tmp, 0, 0);
			ds3231_get_time(dev, time, DS3231_FORMAT_BIN);
			ds3231_clear_alarm2_flag(dev);

			if (time[3] == 0 && time[4] == 0 && time[5] == 0 && time[6] == 0) {
				ret = true;
			} else {
				ret = false;
			}
		} else {
			tmp[0] = 0;
			tmp[1] = 0;
			tmp[2] = 0;
			tmp[3] = 0;
			ds3231_set_alarm2(dev, tmp, 0, 0);
			ds3231_get_time(dev, time, DS3231_FORMAT_BIN);
			ds3231_clear_alarm2_flag(dev);

			if (time[3] == 0 && time[4] == 0 && time[5] == 0 && time[6] == 0) {
				ret = true;
			} else {
				ret = false;
			}
		}

		break;
	}

	return ret;
}

int t_ds3231_square_wave(int argc, char **argv)
{
	struct ds3231_device *dev = ds3231_dev;
	u8 tmp = 0;
	u8 rs1 = 0, rs2 = 0;

	if (argc > 1) {
		rs2 = simple_strtoul(argv[1], &argv[1], 10);
	}

	if (argc > 2) {
		rs1 = simple_strtoul(argv[2], &argv[2], 10);
	}

	tmp = (rs2 << 1) | rs1;

	switch (tmp) {
	case SQUARE_WAVE_1HZ:
		ds3231_rate_select(dev, SQUARE_WAVE_1HZ);
		break;

	case SQUARE_WAVE_1_024HZ:
		ds3231_rate_select(dev, SQUARE_WAVE_1_024HZ);
		break;

	case SQUARE_WAVE_4_096HZ:
		ds3231_rate_select(dev, SQUARE_WAVE_4_096HZ);
		break;

	case SQUARE_WAVE_8_192HZ:
		ds3231_rate_select(dev, SQUARE_WAVE_8_192HZ);
		break;

	default:
		break;
	}

	ds3231_rate_select(dev, SQUARE_WAVE_1HZ);
	ds3231_enable_square_wave(dev);

	return 0;
}

int t_ds3231_32khz_wave(int argc, char **argv)
{
	struct ds3231_device *dev = ds3231_dev;
	ds3231_enable_32khz_output(dev);

	return 0;
}

int t_ds3231_get_temprature(int argc, char **argv)
{
	struct ds3231_device *dev = ds3231_dev;
	float tmp;

	ds3231_convert_temperature(dev);
	tmp = ds3231_get_temperature(dev);

	pr_debug("temperature is %f", tmp);

	return 0;
}

/* Enhanced test functions for comprehensive device testing */
int t_ds3231_comprehensive_test(int argc, char **argv)
{
	struct ds3231_device *dev = ds3231_dev;
	u8 ret = 0;
	u8 status;
	float temperature;
	u8 time_data[7];
	u8 alarm_config[5];

	pr_debug("Starting ds3231 comprehensive test...");

	/* Test 1: Initialize device */
	pr_debug("Test 1: Device initialization");
	if (ds3231_check_power_status(dev) != 1) {
		pr_err("Power status check failed");
		ret = -1;
		goto test_end;
	}

	/* Test 2: Temperature functionality */
	pr_debug("Test 2: Temperature functionality");
	ds3231_trigger_temperature_conversion(dev);
	ds3231_wait_for_temperature_conversion(dev);
	temperature = ds3231_get_temperature_with_precision(dev);
	pr_debug("Temperature: %.2f°C", temperature);

	if (temperature < -40.0f || temperature > 85.0f) {
		pr_err("Temperature out of expected range");
		ret = -1;
	}

	/* Test 3: Time functionality */
	pr_debug("Test 3: Time functionality");
	dev->time_mode = DS3231_HOURS24;
	u8 test_time[7] = {30, 45, 14, 5, 25, 12, 23}; /* 23:14:45, Dec 25, 2023 */
	ds3231_set_time(dev, test_time, DS3231_FORMAT_BIN);

	/* Verify time was set */
	ds3231_get_time(dev, time_data, DS3231_FORMAT_BIN);
	if (time_data[6] != 23 || time_data[5] != 14 || time_data[4] != 45) {
		pr_err("Time setting verification failed");
		ret = -1;
	}

	/* Test 4: Alarm functionality */
	pr_debug("Test 4: Alarm functionality");
	ds3231_disable_all_alarms(dev);
	ds3231_clear_all_alarm_flags(dev);

	/* Set alarm1 for testing */
	ds3231_set_alarm1_with_seconds(dev, 35, 45, 14, 25, 0x00);
	ds3231_enable_alarm1_mask(dev, 0x01);

	/* Verify alarm configuration */
	if (ds3231_get_alarm1_configuration(dev, &alarm_config[0], &alarm_config[1],
		&alarm_config[2], &alarm_config[3], &alarm_config[4]) != 0) {
		pr_err("Alarm configuration read failed");
		ret = -1;
	}

	/* Test 5: Status monitoring */
	pr_debug("Test 5: Status monitoring");
	ds3231_get_alarm1_status(dev, &status);
	ds3231_get_alarm2_status(dev, &status);

	/* Test 6: Oscillator functionality */
	pr_debug("Test 6: Oscillator functionality");
	if (ds3231_oscillator_stop_flag(dev)) {
		pr_debug("Oscillator stop flag is set");
		ds3231_reset_oscillator_stop_flag(dev);
	}

	/* Test 7: Square wave output */
	pr_debug("Test 7: Square wave output");
	ds3231_configure_square_wave(dev, DS3231_SQW_FREQ_1024HZ, 1);

	/* Test 8: Battery backup */
	pr_debug("Test 8: Battery backup functionality");
	ds3231_enable_battery_backup(dev);
	if (!ds3231_is_battery_backup_enabled(dev)) {
		pr_err("Battery backup enable failed");
		ret = -1;
	}

	/* Test 9: 32kHz output */
	pr_debug("Test 9: 32kHz output");
	ds3231_set_32khz_output(dev, 1);
	if (!ds3231_is_32khz_output_enabled(dev)) {
		pr_err("32kHz output enable failed");
		ret = -1;
	}

test_end:
	if (ret == 0) {
		pr_debug("ds3231 comprehensive test PASSED");
	} else {
		pr_err("ds3231 comprehensive test FAILED");
	}

	return ret;
}

int t_ds3231_stress_test(int argc, char **argv)
{
	struct ds3231_device *dev = ds3231_dev;
	u8 ret = 0;
	u32 test_cycles = 1000;
	u32 i;
	float temperature;
	u8 status;

	pr_debug("Starting ds3231 stress test (%lu cycles)...", test_cycles);

	for (i = 0; i < test_cycles; i++) {
		/* Temperature test */
		ds3231_trigger_temperature_conversion(dev);
		ds3231_wait_for_temperature_conversion(dev);
		temperature = ds3231_get_temperature_with_precision(dev);

		if (temperature < -40.0f || temperature > 85.0f) {
			pr_err("Temperature out of range at cycle %lu: %.2f°C", i, temperature);
			ret = -1;
			break;
		}

		/* Alarm flag test */
		ds3231_get_alarm1_status(dev, &status);
		ds3231_get_alarm2_status(dev, &status);

		/* Power status test */
		if (ds3231_check_power_status(dev) != 1) {
			pr_err("Power status failed at cycle %lu", i);
			ret = -1;
			break;
		}

		if (i % 100 == 0) {
			pr_debug("Stress test progress: %lu/%lu cycles completed", i, test_cycles);
		}
	}

	if (ret == 0) {
		pr_debug("ds3231 stress test PASSED (%lu cycles)", test_cycles);
	} else {
		pr_err("ds3231 stress test FAILED at cycle %lu", i);
	}

	return ret;
}

#endif
