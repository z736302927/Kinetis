#include "kinetis/ds3231.h"
#include "kinetis/iic_soft.h"
#include "kinetis/idebug.h"
#include "kinetis/delay.h"
#include "kinetis/design_verification.h"
#include <math.h>

/* The following program is modified by the user according to the hardware device, otherwise the driver cannot run. */

/**
  * @step 1:  Modify the corresponding function according to the modified area and the corresponding function name.
  * @step 2:  Modify four areas: GPIO_PORT/GPIO_PIN/Led_TypeDef/LEDn.
  * @step 3:  .
  * @step 4:  .
  * @step 5:
  */

#define DS3231_ADDR                     0x68

static inline void ds3231_port_transmmit(u8 addr, u8 tmp)
{
	iic_master_port_transmmit(IIC_SW_1, DS3231_ADDR, addr, tmp);
}

static inline void ds3231_port_receive(u8 addr, u8 *pdata)
{
	iic_master_port_receive(IIC_SW_1, DS3231_ADDR, addr, pdata);
}

static inline void ds3231_port_multi_transmmit(u8 addr, u8 *pdata, u32 length)
{
	iic_master_port_multi_transmmit(IIC_SW_1, DS3231_ADDR, addr, pdata, length);
}

static inline void ds3231_port_multi_receive(u8 addr, u8 *pdata, u32 length)
{
	iic_master_port_multi_receive(IIC_SW_1, DS3231_ADDR, addr, pdata, length);
}
/* The above procedure is modified by the user according to the hardware device, otherwise the driver cannot run. */

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

static u8 g_time_mode = DS3231_HOURS24;
static u8 g_time_region = DS3231_AM;

u8 ds3231_get_time_mode(void)
{
	return g_time_mode;
}

void ds3231_set_time_mode(u8 tmp)
{
	g_time_mode = tmp;
}

u8 ds3231_get_time_region(void)
{
	return g_time_region;
}

void ds3231_set_time_region(u8 tmp)
{
	g_time_region = tmp;
}

void ds3231_get_time(u8 *pdata, u8 format)
{
	u8 tmp[7];
	u8 hour10 = 0;

	ds3231_port_multi_receive(SECONDS, tmp, 7);

	if (tmp[2] & 0x40) {
		g_time_mode = DS3231_HOURS12;
		hour10 = (tmp[2] & 0x10) >> 4;

		if (tmp[2] & 0x20) {
			g_time_region = DS3231_PM;
		} else {
			g_time_region = DS3231_AM;
		}
	} else {
		g_time_mode = DS3231_HOURS24;
		hour10 = (tmp[2] & 0x30) >> 4;
	}

	pdata[0] = (tmp[0] >> 4) * 10 + (tmp[0] & 0x0F);
	pdata[1] = (tmp[1] >> 4) * 10 + (tmp[1] & 0x0F);
	pdata[2] = hour10 * 10 + (tmp[2] & 0x0F);
	pdata[3] = (tmp[4] >> 4) * 10 + (tmp[4] & 0x0F);
	pdata[4] = ((tmp[5] & 0x1F) >> 4) * 10 + (tmp[5] & 0x0F);
	pdata[5] = (tmp[6] >> 4) * 10 + (tmp[6] & 0x0F);
}

void ds3231_set_time(u8 *pdata, u8 format)
{
	u8 tmp[7] = {0, 0, 0, 0, 0, 0, 0};

	ds3231_port_multi_transmmit(SECONDS, tmp, 3);
	ds3231_port_multi_transmmit(DATE, tmp, 3);

	tmp[6] |= (pdata[0] / 10) << 4;
	tmp[6] |= (pdata[0] % 10) << 0;
	tmp[5] |= (pdata[1] / 10) << 4;
	tmp[5] |= (pdata[1] % 10) << 0;
	tmp[4] |= (pdata[2] / 10) << 4;
	tmp[4] |= (pdata[2] % 10) << 0;
	ds3231_port_multi_transmmit(DATE, &tmp[4], 3);

	tmp[2] |= (pdata[3] / 10) << 4;
	tmp[2] |= (pdata[3] % 10) << 0;

	if (g_time_mode == DS3231_HOURS12) {
		if (g_time_region == DS3231_PM) {
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
	ds3231_port_multi_transmmit(SECONDS, tmp, 3);
}

void ds3231_get_time_with_string(char *pdata)
{
	u8 tmp[6];

	ds3231_get_time(tmp, DS3231_FORMAT_BIN);
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

void ds3231_set_time_with_string(char *pdata)
{
	u8 tmp[6];

	tmp[0] = (pdata[0] - '0') * 10 + (pdata[1] - '0');
	tmp[1] = (pdata[2] - '0') * 10 + (pdata[3] - '0');
	tmp[2] = (pdata[4] - '0') * 10 + (pdata[5] - '0');
	tmp[3] = (pdata[6] - '0') * 10 + (pdata[7] - '0');
	tmp[4] = (pdata[8] - '0') * 10 + (pdata[9] - '0');
	tmp[5] = (pdata[10] - '0') * 10 + (pdata[11] - '0');
	ds3231_set_time(tmp, DS3231_FORMAT_BIN);
}

void ds3231_get_week(u8 *pdata)
{
	ds3231_port_receive(DAY, pdata);
}

void ds3231_set_week(u8 tmp)
{
	ds3231_port_transmmit(DAY, tmp);
}

void ds3231_alarm1_callback(void)
{
	;
}

void ds3231_set_alarm1(u8 *pdata, u8 date_or_day, u8 alarm)
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

	if (g_time_mode == DS3231_HOURS24) {
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

	ds3231_port_multi_transmmit(ALARM_1_SECONDS, tmp, 4);
}

void ds3231_alarm2_callback(void)
{
	;
}

void ds3231_set_alarm2(u8 *pdata, u8 date_or_day, u8 alarm)
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

	if (g_time_mode == DS3231_HOURS24) {
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

	ds3231_port_multi_transmmit(ALARM_2_MINUTES, tmp, 3);
}

void ds3231_enable_oscillator(void)
{
	u8 reg = 0;

	ds3231_port_receive(CONTROL, &reg);

	reg &= ~(0x01 << 7);
	reg |= (0x00 << 7);

	ds3231_port_transmmit(CONTROL, reg);
}

void ds3231_convert_temperature(void)
{
	u8 reg = 0;

	ds3231_port_receive(CONTROL, &reg);

	reg &= ~(0x01 << 6);
	reg |= (0x01 << 6);

	ds3231_port_transmmit(CONTROL, reg);
}

void ds3231_rate_select(u8 tmp)
{
	u8 reg = 0;

	ds3231_port_receive(CONTROL, &reg);

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

	ds3231_port_transmmit(CONTROL, reg);
}

void ds3231_enable_square_wave_with_bat(void)
{
	u8 reg = 0;

	ds3231_port_receive(CONTROL, &reg);

	reg &= ~(0x01 << 6);
	reg |= (0x01 << 6);

	ds3231_port_transmmit(CONTROL, reg);
}

void ds3231_enable_int(void)
{
	u8 reg = 0;

	ds3231_port_receive(CONTROL, &reg);

	reg &= ~(0x01 << 2);
	reg |= (0x01 << 2);

	ds3231_port_transmmit(CONTROL, reg);
}

void ds3231_enable_square_wave(void)
{
	u8 reg = 0;

	ds3231_port_receive(CONTROL, &reg);

	reg &= ~(0x01 << 2);

	ds3231_port_transmmit(CONTROL, reg);
}

void ds3231_enable_alarm2_int(void)
{
	u8 reg = 0;

	ds3231_port_receive(CONTROL, &reg);

	reg &= ~(0x01 << 1);
	reg |= (0x01 << 1);

	ds3231_port_transmmit(CONTROL, reg);
}

void ds3231_enable_alarm1_int(void)
{
	u8 reg = 0;

	ds3231_port_receive(CONTROL, &reg);

	reg &= ~(0x01 << 0);
	reg |= (0x01 << 0);

	ds3231_port_transmmit(CONTROL, reg);
}

u8 ds3231_oscillator_stop_flag(void)
{
	u8 reg = 0;

	ds3231_port_receive(CONTROL_STATUS, &reg);

	if (reg & 0x80) {
		return 1;
	} else {
		return 0;
	}
}

void ds3231_enable_32khz_output(void)
{
	u8 reg = 0;

	ds3231_port_receive(CONTROL, &reg);

	reg &= ~(0x01 << 3);
	reg |= (0x01 << 3);

	ds3231_port_transmmit(CONTROL, reg);
}

u8 ds3231_wait_busy(void)
{
	u8 reg = 0;

	ds3231_port_receive(CONTROL_STATUS, &reg);

	if (reg & 0x02) {
		return 1;
	} else {
		return 0;
	}
}

static u8 alarm2_flag = 0;

u8 ds3231_alarm2_flag(void)
{
	u8 reg = 0;

	ds3231_port_receive(CONTROL_STATUS, &reg);

	if (reg & 0x02) {
		return 1;
	} else {
		return 0;
	}
}

void ds3231_clear_alarm2_flag(void)
{
	u8 reg = 0;

	ds3231_port_receive(CONTROL, &reg);

	reg &= ~(0x01 << 1);

	ds3231_port_transmmit(CONTROL, reg);
}

static u8 alarm1_flag = 0;

u8 ds3231_alarm1_flag(void)
{
	u8 reg = 0;

	ds3231_port_receive(CONTROL_STATUS, &reg);

	if (reg & 0x01) {
		return 1;
	} else {
		return 0;
	}
}

void ds3231_clear_alarm1_flag(void)
{
	u8 reg = 0;

	ds3231_port_receive(CONTROL, &reg);

	reg &= ~(0x01 << 0);

	ds3231_port_transmmit(CONTROL, reg);
}

void ds3231_aging_offset(u8 *pdata)
{
	ds3231_port_receive(AGING_OFFSET, pdata);
}

void ds3231_set_aging_offset(u8 offset)
{
	/* Aging offset register is signed 8-bit value
	 * Bit 7: Sign bit (0 = positive, 1 = negative)
	 * Bits 6-0: Magnitude
	 * Range: -128 to +127 (represented as 0x80 to 0x7F)
	 * Each step represents approximately 0.25°C temperature compensation
	 */
	ds3231_port_transmmit(AGING_OFFSET, offset);
}

void ds3231_get_aging_offset(s8 *paging_offset)
{
	ds3231_port_receive(AGING_OFFSET, (u8 *)paging_offset);
}

void ds3231_set_temperature_threshold(float high_threshold, float low_threshold)
{
	/* DS3231 doesn't have built-in temperature threshold registers
	 * This function can be used by application to monitor temperature
	 * and take actions based on thresholds
	 */
	/* Store thresholds in global variables for application use */
	/* This is a placeholder implementation */
	/* Actual implementation would depend on application requirements */
}

void ds3231_get_temperature_threshold(float *p_high, float *p_low)
{
	/* Return stored threshold values
	 * This is a placeholder implementation
	 * Actual implementation would depend on application requirements */
	if (p_high) {
		*p_high = 0.0f;
	}
	if (p_low) {
		*p_low = 0.0f;
	}
}

void ds3231_check_temperature_status(u8 *pstatus)
{
	/* Check temperature-related status flags
	 * Return status information about temperature monitoring
	 */
	u8 reg = 0;
	float temp;
	static float last_temp = 0.0f;
	u8 current_status = 0;

	/* Check hardware status flags */
	ds3231_port_receive(CONTROL_STATUS, &reg);

	if (reg & DS3231_STATUS_BSY_BIT) {
		current_status |= 0x01; /* Temperature conversion busy */
	}

	/* Get current temperature and perform advanced analysis */
	ds3231_get_temperature_with_precision(&temp);

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

void ds3231_get_temperature(float *pdata)
{
	u8 val[2];

	ds3231_port_multi_receive(MSB_OF_TEMP, val, 2);
	*pdata = (float)val[0] + (float)(val[1] >> 6) * 0.25;
}

void ds3231_get_temperature_with_precision(float *pdata)
{
	u8 val[2];
	s16 temp_raw;
	float temp_compensated;

	ds3231_port_multi_receive(MSB_OF_TEMP, val, 2);

	/* Calculate temperature with higher precision */
	temp_raw = ((s16)val[0] << 8) | val[1];
	temp_compensated = (float)temp_raw / 256.0f;

	/* Apply aging offset compensation if available */
	u8 aging_offset;
	ds3231_port_receive(AGING_OFFSET, &aging_offset);
	if (aging_offset & 0x80) {
		/* Negative offset */
		temp_compensated -= (float)(~aging_offset + 1) / 4.0f;
	} else {
		/* Positive offset */
		temp_compensated += (float)aging_offset / 4.0f;
	}

	*pdata = temp_compensated;
}

void ds3231_set_temperature_compensation(float offset)
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

	ds3231_port_transmmit(AGING_OFFSET, (u8)aging_offset);
}

void ds3231_get_temperature_compensation(float *pdata)
{
	s8 aging_offset;

	ds3231_port_receive(AGING_OFFSET, (u8 *)&aging_offset);

	if (aging_offset & 0x80) {
		/* Negative offset */
		*pdata = -(float)(~aging_offset + 1) / 4.0f;
	} else {
		/* Positive offset */
		*pdata = (float)aging_offset / 4.0f;
	}
}

void ds3231_enable_auto_temperature_compensation(void)
{
	u8 reg = 0;

	ds3231_port_receive(CONTROL, &reg);

	/* Clear CONV bit to enable automatic conversion */
	reg &= ~DS3231_CONTROL_CONV_BIT;

	ds3231_port_transmmit(CONTROL, reg);
}

void ds3231_disable_auto_temperature_compensation(void)
{
	u8 reg = 0;

	ds3231_port_receive(CONTROL, &reg);

	/* Set CONV bit to disable automatic conversion */
	reg |= DS3231_CONTROL_CONV_BIT;

	ds3231_port_transmmit(CONTROL, reg);
}

void ds3231_trigger_temperature_conversion(void)
{
	u8 reg = 0;

	ds3231_port_receive(CONTROL, &reg);

	/* Set CONV bit to trigger temperature conversion */
	reg |= DS3231_CONTROL_CONV_BIT;

	ds3231_port_transmmit(CONTROL, reg);

	/* Wait for conversion to complete */
	ds3231_wait_for_temperature_conversion();
}

u8 ds3231_is_temperature_conversion_busy(void)
{
	u8 reg = 0;

	ds3231_port_receive(CONTROL_STATUS, &reg);

	return (reg & DS3231_STATUS_BSY_BIT) ? 1 : 0;
}

void ds3231_wait_for_temperature_conversion(void)
{
	u32 timeout = 0;
	const u32 max_timeout = 10000; /* 10 seconds timeout */

	while (ds3231_is_temperature_conversion_busy() && timeout < max_timeout) {
		/* Small delay */
		for (volatile u32 i = 0; i < 1000; i++);
		timeout++;
	}
}

void ds3231_calibrate_temperature(void)
{
	float current_temp;
	float reference_temp = 25.0f; /* Room temperature reference */
	float offset;

	/* Trigger temperature conversion */
	ds3231_trigger_temperature_conversion();

	/* Wait for conversion to complete */
	ds3231_wait_for_temperature_conversion();

	/* Get current temperature */
	ds3231_get_temperature_with_precision(&current_temp);

	/* Calculate offset needed */
	offset = reference_temp - current_temp;

	/* Set the compensation offset */
	ds3231_set_temperature_compensation(offset);
}

void ds3231_disable_oscillator(void)
{
	u8 reg = 0;

	ds3231_port_receive(CONTROL, &reg);

	/* Set EOSC bit to disable oscillator */
	reg |= DS3231_CONTROL_EOSC_BIT;

	ds3231_port_transmmit(CONTROL, reg);
}

void ds3231_enable_battery_backup(void)
{
	u8 reg = 0;

	ds3231_port_receive(CONTROL, &reg);

	/* Clear BBSQW bit to enable battery backup square wave */
	reg &= ~DS3231_CONTROL_BBSQW_BIT;

	ds3231_port_transmmit(CONTROL, reg);
}

void ds3231_disable_battery_backup(void)
{
	u8 reg = 0;

	ds3231_port_receive(CONTROL, &reg);

	/* Set BBSQW bit to disable battery backup square wave */
	reg |= DS3231_CONTROL_BBSQW_BIT;

	ds3231_port_transmmit(CONTROL, reg);
}

u8 ds3231_is_battery_backup_enabled(void)
{
	u8 reg = 0;

	ds3231_port_receive(CONTROL, &reg);

	return (reg & DS3231_CONTROL_BBSQW_BIT) ? 0 : 1;
}

void ds3231_reset_oscillator_stop_flag(void)
{
	u8 reg = 0;

	ds3231_port_receive(CONTROL_STATUS, &reg);

	/* Clear OSF bit */
	reg &= ~DS3231_STATUS_OSF_BIT;

	ds3231_port_transmmit(CONTROL_STATUS, reg);
}

void ds3231_set_32khz_output(u8 enable)
{
	u8 reg = 0;

	ds3231_port_receive(CONTROL_STATUS, &reg);

	if (enable) {
		reg |= DS3231_STATUS_EN32KHZ_BIT;
	} else {
		reg &= ~DS3231_STATUS_EN32KHZ_BIT;
	}

	ds3231_port_transmmit(CONTROL_STATUS, reg);
}

u8 ds3231_is_32khz_output_enabled(void)
{
	u8 reg = 0;

	ds3231_port_receive(CONTROL_STATUS, &reg);

	return (reg & DS3231_STATUS_EN32KHZ_BIT) ? 1 : 0;
}

void ds3231_configure_square_wave(u8 frequency, u8 battery_backup)
{
	u8 reg = 0;

	ds3231_port_receive(CONTROL, &reg);

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

	ds3231_port_transmmit(CONTROL, reg);
}

void ds3231_enable_interrupt_mode(void)
{
	u8 reg = 0;

	ds3231_port_receive(CONTROL, &reg);

	/* Set INTCN bit to enable interrupt mode */
	reg |= DS3231_CONTROL_INTCN_BIT;

	ds3231_port_transmmit(CONTROL, reg);
}

void ds3231_disable_interrupt_mode(void)
{
	u8 reg = 0;

	ds3231_port_receive(CONTROL, &reg);

	/* Clear INTCN bit to disable interrupt mode (square wave mode) */
	reg &= ~DS3231_CONTROL_INTCN_BIT;

	ds3231_port_transmmit(CONTROL, reg);
}

u8 ds3231_is_interrupt_mode_enabled(void)
{
	u8 reg = 0;

	ds3231_port_receive(CONTROL, &reg);

	return (reg & DS3231_CONTROL_INTCN_BIT) ? 1 : 0;
}

void ds3231_emergency_oscillator_enable(void)
{
	/* This function enables the oscillator in emergency conditions */
	ds3231_enable_oscillator();

	/* Clear any oscillator stop flag */
	ds3231_reset_oscillator_stop_flag();

	/* Enable 32kHz output */
	ds3231_set_32khz_output(1);
}

u8 ds3231_check_power_status(void)
{
	u8 status = 0;
	u8 control_status = 0;

	ds3231_port_receive(CONTROL_STATUS, &control_status);

	/* Check oscillator stop flag */
	if (control_status & DS3231_STATUS_OSF_BIT) {
		status |= 0x01; /* Oscillator stopped */
	}

	/* Check if device is running on battery backup */
	/* Note: DS3231 doesn't directly report this, so we infer from other indicators */
	if (control_status & DS3231_STATUS_BSY_BIT) {
		status |= 0x02; /* Busy state - may indicate battery operation */
	}

	return status;
}

void ds3231_initialize_device(void)
{
	/* Enable oscillator */
	ds3231_enable_oscillator();

	/* Reset oscillator stop flag */
	ds3231_reset_oscillator_stop_flag();

	/* Configure for 1Hz square wave output */
	ds3231_configure_square_wave(DS3231_SQW_FREQ_1HZ, 1);

	/* Enable interrupt mode */
	ds3231_enable_interrupt_mode();

	/* Enable automatic temperature compensation */
	ds3231_enable_auto_temperature_compensation();

	/* Enable battery backup square wave */
	ds3231_enable_battery_backup();

	/* Disable 32kHz output by default */
	ds3231_set_32khz_output(0);
}

void ds3231_soft_reset(void)
{
	u8 reg = 0;

	/* Save current control register settings */
	ds3231_port_receive(CONTROL, &reg);

	/* Clear all control bits except INTCN */
	u8 new_reg = reg & DS3231_CONTROL_INTCN_BIT;

	/* Temporarily set all bits */
	reg |= 0xFF;
	ds3231_port_transmmit(CONTROL, reg);

	/* Small delay */
	for (volatile u32 i = 0; i < 10000; i++);

	/* Restore control register with proper configuration */
	ds3231_port_transmmit(CONTROL, new_reg);

	/* Reset alarm flags */
	ds3231_clear_alarm1_flag();
	ds3231_clear_alarm2_flag();
}

void ds3231_disable_alarm1(void)
{
	u8 reg = 0;

	ds3231_port_receive(CONTROL, &reg);

	/* Clear A1IE bit to disable alarm 1 */
	reg &= ~DS3231_CONTROL_A1IE_BIT;

	ds3231_port_transmmit(CONTROL, reg);
}

void ds3231_disable_alarm2(void)
{
	u8 reg = 0;

	ds3231_port_receive(CONTROL, &reg);

	/* Clear A2IE bit to disable alarm 2 */
	reg &= ~DS3231_CONTROL_A2IE_BIT;

	ds3231_port_transmmit(CONTROL, reg);
}

void ds3231_enable_alarm1_mask(u8 mask)
{
	u8 reg = 0;

	ds3231_port_receive(CONTROL, &reg);

	/* Set A1IE bit to enable alarm 1 */
	reg |= DS3231_CONTROL_A1IE_BIT;

	ds3231_port_transmmit(CONTROL, reg);
}

void ds3231_enable_alarm2_mask(u8 mask)
{
	u8 reg = 0;

	ds3231_port_receive(CONTROL, &reg);

	/* Set A2IE bit to enable alarm 2 */
	reg |= DS3231_CONTROL_A2IE_BIT;

	ds3231_port_transmmit(CONTROL, reg);
}

void ds3231_get_alarm1_status(u8 *pstatus)
{
	u8 reg = 0;

	ds3231_port_receive(CONTROL_STATUS, &reg);

	*pstatus = (reg & DS3231_STATUS_A1F_BIT) ? 1 : 0;
}

void ds3231_get_alarm2_status(u8 *pstatus)
{
	u8 reg = 0;

	ds3231_port_receive(CONTROL_STATUS, &reg);

	*pstatus = (reg & DS3231_STATUS_A2F_BIT) ? 1 : 0;
}

void ds3231_clear_all_alarm_flags(void)
{
	ds3231_clear_alarm1_flag();
	ds3231_clear_alarm2_flag();
}

void ds3231_set_alarm1_with_seconds(u8 seconds, u8 minutes, u8 hours, u8 day_date, u8 mask)
{
	u8 tmp[4] = {0, 0, 0, 0};

	/* Convert values to BCD format */
	tmp[0] = ((seconds / 10) << 4) | (seconds % 10);
	tmp[1] = ((minutes / 10) << 4) | (minutes % 10);

	if (g_time_mode == DS3231_HOURS12) {
		if (hours > 12) {
			hours -= 12;
		}
		tmp[2] = ((hours / 10) << 4) | (hours % 10);
		tmp[2] |= DS3231_HOURS_12_24_BIT;
		if (g_time_region == DS3231_PM) {
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

	ds3231_port_multi_transmmit(ALARM_1_SECONDS, tmp, 4);
}

void ds3231_set_alarm2_with_minutes(u8 minutes, u8 hours, u8 day_date, u8 mask)
{
	u8 tmp[3] = {0, 0, 0};

	/* Convert values to BCD format */
	tmp[0] = ((minutes / 10) << 4) | (minutes % 10);

	if (g_time_mode == DS3231_HOURS12) {
		if (hours > 12) {
			hours -= 12;
		}
		tmp[1] = ((hours / 10) << 4) | (hours % 10);
		tmp[1] |= DS3231_HOURS_12_24_BIT;
		if (g_time_region == DS3231_PM) {
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

	ds3231_port_multi_transmmit(ALARM_2_MINUTES, tmp, 3);
}

u8 ds3231_check_alarm_interrupt_source(void)
{
	u8 reg = 0;
	u8 source = 0;

	ds3231_port_receive(CONTROL_STATUS, &reg);

	if (reg & DS3231_STATUS_A1F_BIT) {
		source |= 0x01; /* Alarm 1 triggered */
	}
	if (reg & DS3231_STATUS_A2F_BIT) {
		source |= 0x02; /* Alarm 2 triggered */
	}

	return source;
}

void ds3231_disable_all_alarms(void)
{
	ds3231_disable_alarm1();
	ds3231_disable_alarm2();
	ds3231_clear_all_alarm_flags();
}

void ds3231_enable_matched_alarm_only(u8 alarm_number)
{
	ds3231_disable_all_alarms();

	if (alarm_number == 1) {
		ds3231_enable_alarm1_mask(0xFF);
	} else if (alarm_number == 2) {
		ds3231_enable_alarm2_mask(0xFF);
	}
}

u8 ds3231_get_alarm1_configuration(u8 *pseconds, u8 *pminutes, u8 *phours, u8 *pday_date, u8 *pmask)
{
	u8 tmp[4] = {0};
	u8 ret = 0;

	ds3231_port_multi_transmmit(ALARM_1_SECONDS, tmp, 4);

	/* Extract values */
	*pseconds = ((tmp[0] & 0x70) >> 4) * 10 + (tmp[0] & 0x0F);
	*pminutes = ((tmp[1] & 0x70) >> 4) * 10 + (tmp[1] & 0x0F);

	if (tmp[2] & DS3231_HOURS_12_24_BIT) {
		/* 12-hour mode */
		*phours = ((tmp[2] & 0x10) >> 4) * 10 + (tmp[2] & 0x0F);
		if (tmp[2] & DS3231_HOURS_AM_PM_BIT) {
			*phours += 12; /* PM */
		}
		g_time_mode = DS3231_HOURS12;
	} else {
		/* 24-hour mode */
		*phours = ((tmp[2] & 0x30) >> 4) * 10 + (tmp[2] & 0x0F);
		g_time_mode = DS3231_HOURS24;
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

#ifdef DESIGN_VERIFICATION_DS3231
#include "kinetis/test-kinetis.h"

#include <linux/iopoll.h>
#include <linux/printk.h>

int t_ds3231_set_clock(int argc, char **argv)
{
	char time[16];

	ds3231_set_time_mode(DS3231_HOURS24);
	ds3231_set_time_region(DS3231_PM);
	ds3231_set_time_with_string("200308202020");
	ds3231_set_week(7);

	ds3231_get_time_with_string(time);

	if (strcmp("200308202020", time) != 0) {
		return false;
	}

	if (g_time_region == DS3231_AM) {
		snprintf(&time[12], 4, " DS3231_AM");
	} else {
		snprintf(&time[12], 4, " PM");
	}

	pr_debug("%s", time);

	return PASS;
}

int t_ds3231_get_clock(int argc, char **argv)
{
	char time[16];
	u8 i = 0;
	u16 times = 1;

	if (argc > 1) {
		times = simple_strtoul(argv[1], &argv[1], 10);
	}

	for (i = 0; i < times; i++) {
		ds3231_get_time_with_string(time);

		if (g_time_region == DS3231_AM) {
			snprintf(&time[12], 4, " DS3231_AM");
		} else {
			snprintf(&time[12], 4, " PM");
		}

		pr_debug("%s", time);
		mdelay(1000);
	}

	return PASS;
}

int t_ds3231_set_alarm1(int argc, char **argv)
{
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

	ds3231_enable_int();
	ds3231_enable_alarm1_int();

	switch (alarm_rate) {
	case 0x0F:
		ds3231_set_alarm1(tmp, 0,
			ALARM_MASK_1 | ALARM_MASK_2 | ALARM_MASK_3 | ALARM_MASK_4);
		delta = basic_timer_get_ss();
		readb_poll_timeout_atomic(&alarm1_flag, flag, flag == true, 1, 2000);
		delta = basic_timer_get_ss() - delta;
		ds3231_clear_alarm1_flag();

		if (delta == 1) {
			ret = PASS;
		} else {
			ret = FAIL;
		}

		break;

	case 0x0E:
		tmp[0] = 0;
		ds3231_set_alarm1(tmp, 0,
			ALARM_MASK_1 | ALARM_MASK_2 | ALARM_MASK_3);
		ds3231_get_time(time, DS3231_FORMAT_BIN);
		ds3231_clear_alarm1_flag();

		if (time[6] == 0) {
			ret = PASS;
		} else {
			ret = FAIL;
		}

		break;

	case 0x0C:
		tmp[0] = 0;
		tmp[1] = 0;
		ds3231_set_alarm1(tmp, 0,
			ALARM_MASK_1 | ALARM_MASK_2);
		ds3231_get_time(time, DS3231_FORMAT_BIN);
		ds3231_clear_alarm1_flag();

		if (time[5] == 0 && time[6] == 0) {
			ret = PASS;
		} else {
			ret = FAIL;
		}

		break;

	case 0x08:
		tmp[0] = 0;
		tmp[1] = 0;
		tmp[2] = 0;
		ds3231_set_alarm1(tmp, 0,
			ALARM_MASK_1);
		ds3231_get_time(time, DS3231_FORMAT_BIN);
		ds3231_clear_alarm1_flag();

		if (time[4] == 0 && time[5] == 0 && time[6] == 0) {
			ret = PASS;
		} else {
			ret = FAIL;
		}

		break;

	case 0x00:
		if (dy_dt) {
			tmp[0] = 0;
			tmp[1] = 0;
			tmp[2] = 0;
			tmp[3] = 0;
			ds3231_set_alarm1(tmp, 0,
				0);
			ds3231_get_time(time, DS3231_FORMAT_BIN);
			ds3231_clear_alarm1_flag();

			if (time[3] == 0 && time[4] == 0 && time[5] == 0 && time[6] == 0) {
				ret = PASS;
			} else {
				ret = FAIL;
			}
		} else {
			tmp[0] = 0;
			tmp[1] = 0;
			tmp[2] = 0;
			tmp[3] = 0;
			ds3231_set_alarm1(tmp, 0,
				0);
			ds3231_get_time(time, DS3231_FORMAT_BIN);
			ds3231_clear_alarm1_flag();

			if (time[3] == 0 && time[4] == 0 && time[5] == 0 && time[6] == 0) {
				ret = PASS;
			} else {
				ret = FAIL;
			}
		}

		break;
	}

	return ret;
}

int t_ds3231_set_alarm2(int argc, char **argv)
{
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

	ds3231_enable_int();
	ds3231_enable_alarm2_int();

	switch (alarm_rate) {
	case 0x0F:
		ds3231_set_alarm2(tmp, 0,
			ALARM_MASK_1 | ALARM_MASK_2 | ALARM_MASK_3 | ALARM_MASK_4);
		delta = basic_timer_get_ss();
		readb_poll_timeout_atomic(&alarm2_flag, flag, flag == true, 1, 2000);
		delta = basic_timer_get_ss() - delta;
		ds3231_clear_alarm2_flag();

		if (delta == 1) {
			ret = true;
		} else {
			ret = false;
		}

		break;

	case 0x0E:
		tmp[0] = 0;
		ds3231_set_alarm2(tmp, 0,
			ALARM_MASK_1 | ALARM_MASK_2 | ALARM_MASK_3);
		ds3231_get_time(time, DS3231_FORMAT_BIN);
		ds3231_clear_alarm2_flag();

		if (time[6] == 0) {
			ret = true;
		} else {
			ret = false;
		}

		break;

	case 0x0C:
		tmp[0] = 0;
		tmp[1] = 0;
		ds3231_set_alarm2(tmp, 0, ALARM_MASK_1 | ALARM_MASK_2);
		ds3231_get_time(time, DS3231_FORMAT_BIN);
		ds3231_clear_alarm2_flag();

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
		ds3231_set_alarm2(tmp, 0, ALARM_MASK_1);
		ds3231_get_time(time, DS3231_FORMAT_BIN);
		ds3231_clear_alarm2_flag();

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
			ds3231_set_alarm2(tmp, 0, 0);
			ds3231_get_time(time, DS3231_FORMAT_BIN);
			ds3231_clear_alarm2_flag();

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
			ds3231_set_alarm2(tmp, 0, 0);
			ds3231_get_time(time, DS3231_FORMAT_BIN);
			ds3231_clear_alarm2_flag();

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
		ds3231_rate_select(SQUARE_WAVE_1HZ);
		break;

	case SQUARE_WAVE_1_024HZ:
		ds3231_rate_select(SQUARE_WAVE_1_024HZ);
		break;

	case SQUARE_WAVE_4_096HZ:
		ds3231_rate_select(SQUARE_WAVE_4_096HZ);
		break;

	case SQUARE_WAVE_8_192HZ:
		ds3231_rate_select(SQUARE_WAVE_8_192HZ);
		break;

	default:
		break;
	}

	ds3231_rate_select(SQUARE_WAVE_1HZ);
	ds3231_enable_square_wave();

	return PASS;
}

int t_ds3231_32khz_wave(int argc, char **argv)
{
	ds3231_enable_32khz_output();

	return PASS;
}

int t_ds3231_get_temprature(int argc, char **argv)
{
	float tmp = 0;

	ds3231_convert_temperature();
	ds3231_get_temperature(&tmp);

	pr_debug("Temperature is %f", tmp);

	return PASS;
}

/* Enhanced test functions for comprehensive device testing */
int t_ds3231_comprehensive_test(int argc, char **argv)
{
	u8 ret = PASS;
	u8 status;
	float temperature;
	u8 time_data[7];
	u8 alarm_config[5];

	pr_debug("Starting DS3231 comprehensive test...");

	/* Test 1: Initialize device */
	pr_debug("Test 1: Device initialization");
	ds3231_initialize_device();
	if (ds3231_check_power_status() != 1) {
		pr_err("Power status check failed");
		ret = FAIL;
		goto test_end;
	}

	/* Test 2: Temperature functionality */
	pr_debug("Test 2: Temperature functionality");
	ds3231_trigger_temperature_conversion();
	ds3231_wait_for_temperature_conversion();
	ds3231_get_temperature_with_precision(&temperature);
	pr_debug("Temperature: %.2f°C", temperature);

	if (temperature < -40.0f || temperature > 85.0f) {
		pr_err("Temperature out of expected range");
		ret = FAIL;
	}

	/* Test 3: Time functionality */
	pr_debug("Test 3: Time functionality");
	ds3231_set_time_mode(DS3231_HOURS24);
	u8 test_time[7] = {30, 45, 14, 5, 25, 12, 23}; /* 23:14:45, Dec 25, 2023 */
	ds3231_set_time(test_time, DS3231_FORMAT_BIN);

	/* Verify time was set */
	ds3231_get_time(time_data, DS3231_FORMAT_BIN);
	if (time_data[6] != 23 || time_data[5] != 14 || time_data[4] != 45) {
		pr_err("Time setting verification failed");
		ret = FAIL;
	}

	/* Test 4: Alarm functionality */
	pr_debug("Test 4: Alarm functionality");
	ds3231_disable_all_alarms();
	ds3231_clear_all_alarm_flags();

	/* Set alarm1 for testing */
	ds3231_set_alarm1_with_seconds(35, 45, 14, 25, 0x00);
	ds3231_enable_alarm1_mask(0x01);

	/* Verify alarm configuration */
	if (ds3231_get_alarm1_configuration(&alarm_config[0], &alarm_config[1],
		&alarm_config[2], &alarm_config[3],
		&alarm_config[4]) != 0) {
		pr_err("Alarm configuration read failed");
		ret = FAIL;
	}

	/* Test 5: Status monitoring */
	pr_debug("Test 5: Status monitoring");
	ds3231_get_alarm1_status(&status);
	ds3231_get_alarm2_status(&status);

	/* Test 6: Oscillator functionality */
	pr_debug("Test 6: Oscillator functionality");
	if (ds3231_oscillator_stop_flag()) {
		pr_debug("Oscillator stop flag is set");
		ds3231_reset_oscillator_stop_flag();
	}

	/* Test 7: Square wave output */
	pr_debug("Test 7: Square wave output");
	ds3231_configure_square_wave(DS3231_SQW_FREQ_1024HZ, 1);

	/* Test 8: Battery backup */
	pr_debug("Test 8: Battery backup functionality");
	ds3231_enable_battery_backup();
	if (!ds3231_is_battery_backup_enabled()) {
		pr_err("Battery backup enable failed");
		ret = FAIL;
	}

	/* Test 9: 32kHz output */
	pr_debug("Test 9: 32kHz output");
	ds3231_set_32khz_output(1);
	if (!ds3231_is_32khz_output_enabled()) {
		pr_err("32kHz output enable failed");
		ret = FAIL;
	}

test_end:
	if (ret == PASS) {
		pr_debug("DS3231 comprehensive test PASSED");
	} else {
		pr_err("DS3231 comprehensive test FAILED");
	}

	return ret;
}

int t_ds3231_stress_test(int argc, char **argv)
{
	u8 ret = PASS;
	u32 test_cycles = 1000;
	u32 i;
	float temperature;
	u8 status;

	pr_debug("Starting DS3231 stress test (%lu cycles)...", test_cycles);

	for (i = 0; i < test_cycles; i++) {
		/* Temperature test */
		ds3231_trigger_temperature_conversion();
		ds3231_wait_for_temperature_conversion();
		ds3231_get_temperature_with_precision(&temperature);

		if (temperature < -40.0f || temperature > 85.0f) {
			pr_err("Temperature out of range at cycle %lu: %.2f°C", i, temperature);
			ret = FAIL;
			break;
		}

		/* Alarm flag test */
		ds3231_get_alarm1_status(&status);
		ds3231_get_alarm2_status(&status);

		/* Power status test */
		if (ds3231_check_power_status() != 1) {
			pr_err("Power status failed at cycle %lu", i);
			ret = FAIL;
			break;
		}

		if (i % 100 == 0) {
			pr_debug("Stress test progress: %lu/%lu cycles completed", i, test_cycles);
		}
	}

	if (ret == PASS) {
		pr_debug("DS3231 stress test PASSED (%lu cycles)", test_cycles);
	} else {
		pr_err("DS3231 stress test FAILED at cycle %lu", i);
	}

	return ret;
}

#endif
