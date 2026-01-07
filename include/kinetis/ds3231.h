#ifndef __DS3231_H
#define __DS3231_H

#ifdef __cplusplus
extern "C" {
#endif

/* The following program is modified by the user according to the hardware device, otherwise the driver cannot run. */

/* Includes ------------------------------------------------------------------*/
#include <linux/types.h>

#include "kinetis/core_common.h"

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

u8 ds3231_get_time_mode(void);
void ds3231_set_time_mode(u8 data);
u8 ds3231_get_time_region(void);
void ds3231_set_time_region(u8 data);
void ds3231_get_time(u8 *pdata, u8 format);
void ds3231_set_time(u8 *pdata, u8 format);
void ds3231_get_time_with_string(char *pdata);
void ds3231_set_time_with_string(char *pdata);
void ds3231_get_week(u8 *pdata);
void ds3231_set_week(u8 data);
void ds3231_set_alarm1(u8 *pdata, u8 date_or_day, u8 alarm);
void ds3231_set_alarm2(u8 *pdata, u8 date_or_day, u8 alarm);
void ds3231_get_temperature(float *pdata);
void ds3231_test(void);
void ds3231_enable_oscillator(void);
void ds3231_convert_temperature(void);
void ds3231_rate_select(u8 rate);
void ds3231_enable_square_wave_with_bat(void);
void ds3231_enable_int(void);
void ds3231_enable_square_wave(void);
void ds3231_enable_alarm2_int(void);
void ds3231_enable_alarm1_int(void);
u8 ds3231_oscillator_stop_flag(void);
void ds3231_enable_32khz_output(void);
u8 ds3231_wait_busy(void);
u8 ds3231_alarm2_flag(void);
void ds3231_clear_alarm2_flag(void);
u8 ds3231_alarm1_flag(void);
void ds3231_clear_alarm1_flag(void);
void ds3231_aging_offset(u8 *pdata);
void ds3231_set_aging_offset(u8 offset);
void ds3231_alarm1_callback(void);
void ds3231_alarm2_callback(void);

/* Temperature compensation and calibration functions */
void ds3231_set_temperature_compensation(float offset);
void ds3231_get_temperature_compensation(float *pdata);
void ds3231_enable_auto_temperature_compensation(void);
void ds3231_disable_auto_temperature_compensation(void);
void ds3231_set_temperature_threshold(float high_threshold, float low_threshold);
void ds3231_get_temperature_threshold(float *p_high, float *p_low);
void ds3231_check_temperature_status(u8 *status);
void ds3231_calibrate_temperature(void);
void ds3231_get_temperature_with_precision(float *pdata);
void ds3231_trigger_temperature_conversion(void);
u8 ds3231_is_temperature_conversion_busy(void);
void ds3231_wait_for_temperature_conversion(void);

/* Oscillator and power management functions */
void ds3231_disable_oscillator(void);
void ds3231_enable_battery_backup(void);
void ds3231_disable_battery_backup(void);
u8 ds3231_is_battery_backup_enabled(void);
void ds3231_reset_oscillator_stop_flag(void);
void ds3231_set_32khz_output(u8 enable);
u8 ds3231_is_32khz_output_enabled(void);
void ds3231_configure_square_wave(u8 frequency, u8 battery_backup);
void ds3231_enable_interrupt_mode(void);
void ds3231_disable_interrupt_mode(void);
u8 ds3231_is_interrupt_mode_enabled(void);
void ds3231_emergency_oscillator_enable(void);
u8 ds3231_check_power_status(void);
void ds3231_initialize_device(void);
void ds3231_soft_reset(void);

/* Enhanced alarm and interrupt management functions */
void ds3231_disable_alarm1(void);
void ds3231_disable_alarm2(void);
void ds3231_enable_alarm1_mask(u8 mask);
void ds3231_enable_alarm2_mask(u8 mask);
void ds3231_get_alarm1_status(u8 *pstatus);
void ds3231_get_alarm2_status(u8 *pstatus);
void ds3231_clear_all_alarm_flags(void);
void ds3231_set_alarm1_with_seconds(u8 seconds, u8 minutes, u8 hours, u8 day_date, u8 mask);
void ds3231_set_alarm2_with_minutes(u8 minutes, u8 hours, u8 day_date, u8 mask);
void ds3231_set_alarm1_repeat_mode(u8 mode);
void ds3231_set_alarm2_repeat_mode(u8 mode);
u8 ds3231_get_alarm1_configuration(u8 *pseconds, u8 *pminutes, u8 *phours, u8 *pday_date, u8 *pmask);
u8 ds3231_get_alarm2_configuration(u8 *pminutes, u8 *phours, u8 *pday_date, u8 *pmask);
void ds3231_wait_for_alarm1(u32 timeout_ms);
void ds3231_wait_for_alarm2(u32 timeout_ms);
void ds3231_set_alarm_callback1(void (*callback)(void));
void ds3231_set_alarm_callback2(void (*callback)(void));
u8 ds3231_check_alarm_interrupt_source(void);
void ds3231_disable_all_alarms(void);
void ds3231_enable_matched_alarm_only(u8 alarm_number);

/* The above procedure is modified by the user according to the hardware device, otherwise the driver cannot run. */


#ifdef __cplusplus
}
#endif

#endif /* __DS3231_H */
