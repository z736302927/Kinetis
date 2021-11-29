#include "real-time-clock.h"


/* The following program is modified by the user according to the hardware device, otherwise the driver cannot run. */

/**
  * @step 1:  Modify the corresponding function according to the modified area and the corresponding function name.
  * @step 2:  Modify four areas: GPIO_PORT/GPIO_PIN/Led_TypeDef/LEDn.
  * @step 3:  .
  * @step 4:  .
  * @step 5:
  */

#include "rtc.h"

/* The above procedure is modified by the user according to the hardware device, otherwise the driver cannot run. */

#define USING_CHIP_RTC
//#define USING_DS3231

#define HOURS24                         0x00

/**
  * @brief  Writes a data in a specified RTC Backup data register.
  * @retval None
  */
void rtc_backup_reg_write(void)
{
#ifdef USING_CHIP_RTC
	/*##-3- Writes a data in a RTC Backup data Register1 #####################*/
	HAL_RTCEx_BKUPWrite(&hrtc, RTC_BKP_DR1, 0x32F2);
#endif
}

/**
  * @brief  Read a data in a specified RTC Backup data register.
  * @retval None
  */
void rtc_backup_reg_read(u32 *tmp)
{
#ifdef USING_CHIP_RTC
	/*##-3- Read a data in a RTC Backup data Register1 #######################*/
	*tmp = HAL_RTCEx_BKUPRead(&hrtc, RTC_BKP_DR1);
#endif
}

/**
  * @brief  Configure the current time and date.
  * @param  None
  * @retval None
  */
void rtc_calendar_set(struct tm *rtc, u8 format)
{
#ifdef USING_CHIP_RTC
	RTC_DateTypeDef sdate;
	RTC_TimeTypeDef stime;

	/*##-1- Configure the Date ###############################################*/
	/* Set Date: Wednesday May 1th 2019 */
	if (format == KRTC_FORMAT_BIN)
		HAL_RTC_GetDate(&hrtc, &sdate, RTC_FORMAT_BIN);
	else
		HAL_RTC_GetDate(&hrtc, &sdate, RTC_FORMAT_BCD);

	sdate.Year = rtc->tm_year;
	sdate.Month = rtc->tm_mon;
	sdate.Date = rtc->tm_mday;

	if (rtc->tm_wday != 0)
		sdate.WeekDay = rtc->tm_wday;

	if (format == KRTC_FORMAT_BIN)
		HAL_RTC_SetDate(&hrtc, &sdate, RTC_FORMAT_BIN);
	else
		HAL_RTC_SetDate(&hrtc, &sdate, RTC_FORMAT_BCD);

	/*##-2- Configure the Time ###############################################*/
	/* Set Time: 00:00:00 */
	if (format == KRTC_FORMAT_BIN)
		HAL_RTC_GetTime(&hrtc, &stime, RTC_FORMAT_BIN);
	else
		HAL_RTC_GetTime(&hrtc, &stime, RTC_FORMAT_BCD);

	stime.Hours = rtc->tm_hour;
	stime.Minutes = rtc->tm_min;
	stime.Seconds = rtc->tm_sec;
	stime.TimeFormat = RTC_HOURFORMAT12_AM;
	stime.DayLightSaving = RTC_DAYLIGHTSAVING_NONE ;
	stime.StoreOperation = RTC_STOREOPERATION_RESET;

	if (format == KRTC_FORMAT_BIN)
		HAL_RTC_SetTime(&hrtc, &stime, RTC_FORMAT_BIN);
	else
		HAL_RTC_SetTime(&hrtc, &stime, RTC_FORMAT_BCD);

	/*##-3- Writes a data in a RTC Backup data Register1 #####################*/
	rtc_backup_reg_write();
#endif

#ifdef USING_DS3231
	char time[13];

	snprintf(time, sizeof(time), "%02d%02d%02d%02d%02d%02d",
		rtc->tm_year, rtc->tm_mon, rtc->tm_mday,
		rtc->tm_hour, rtc->tm_min, rtc->tm_sec);
	ds3231_SetTimeWithString(time);

	if (rtc->tm_wday != 0)
		ds3231_SetWeek(rtc->tm_wday);

#endif
}

/**
  * @brief  Display the current time and date.
  * @param  showtime : pointer to buffer
  * @param  showdate : pointer to buffer
  * @retval None
  */
void rtc_calendar_get(struct tm *rtc, u8 format)
{
#ifdef USING_CHIP_RTC
	RTC_DateTypeDef sdate;
	RTC_TimeTypeDef stime;

	/* Get the RTC current Date */
	if (format == KRTC_FORMAT_BIN)
		HAL_RTC_GetDate(&hrtc, &sdate, RTC_FORMAT_BIN);
	else
		HAL_RTC_GetDate(&hrtc, &sdate, RTC_FORMAT_BCD);

	rtc->tm_year = sdate.Year;
	rtc->tm_mon = sdate.Month;
	rtc->tm_mday = sdate.Date;

	if (rtc->tm_wday != 0)
		rtc->tm_wday = sdate.WeekDay;

	/* Get the RTC current Time */
	if (format == KRTC_FORMAT_BIN)
		HAL_RTC_GetTime(&hrtc, &stime, RTC_FORMAT_BIN);
	else
		HAL_RTC_GetTime(&hrtc, &stime, RTC_FORMAT_BCD);

	rtc->tm_hour = stime.Hours;
	rtc->tm_min = stime.Minutes;
	rtc->tm_sec = stime.Seconds;
#endif

#ifdef USING_DS3231
	u8 time[6], week;

	if (format == KRTC_FORMAT_BIN)
		ds3231_ReadTime(time, DS3231_FORMAT_BIN);
	else
		ds3231_ReadTime(time, DS3231_FORMAT_BCD);

	ds3231_ReadWeek(&week);
	rtc->tm_year = time[5];
	rtc->tm_mon = time[4];
	rtc->tm_mday = time[3];
	rtc->tm_wday = week;
	rtc->tm_hour = time[2];
	rtc->tm_min = time[1];
	rtc->tm_sec = time[0];
#endif
}

/**
  * @brief  Configure the current time and date.
  * @param  None
  * @retval None
  */
void rtc_set_time_format(u8 tmp)
{
#ifdef USING_CHIP_RTC

#endif

#ifdef USING_DS3231

#endif
}

/**
  * @brief  Configure the current time and date.
  * @param  None
  * @retval None
  */
u8 rtc_get_time_format(void)
{
#ifdef USING_CHIP_RTC

#endif

#ifdef USING_DS3231

#endif
	return 0;
}

#ifdef DESIGN_VERIFICATION_RTC
#include "kinetis/test-kinetis.h"
#include "kinetis/random-gene.h"

#include <linux/printk.h>

#include "stdlib.h"

int t_rtc_set_clock(int argc, char **argv)
{
	struct tm rtc;

	rtc.tm_year = random_get8bit() % 100;
	rtc.tm_mon = random_get8bit() % 12;
	rtc.tm_mday = random_get8bit() % 28;
	rtc.tm_hour = random_get8bit() % 24;
	rtc.tm_min = random_get8bit() % 60;
	rtc.tm_sec = random_get8bit() % 60;
	rtc.tm_wday = random_get8bit() % 7;

	if (argc > 1)
		rtc.tm_year = strtoul(argv[1], &argv[1], 10);

	if (argc > 2)
		rtc.tm_mon = strtoul(argv[2], &argv[2], 10);

	if (argc > 3)
		rtc.tm_mday = strtoul(argv[3], &argv[3], 10);

	if (argc > 4)
		rtc.tm_hour = strtoul(argv[4], &argv[4], 10);

	if (argc > 5)
		rtc.tm_min = strtoul(argv[5], &argv[5], 10);

	if (argc > 6)
		rtc.tm_sec = strtoul(argv[6], &argv[6], 10);

	if (argc > 7)
		rtc.tm_wday = strtoul(argv[7], &argv[7], 10);

	printk(KERN_DEBUG "Set clock is %s", get_rtc_string(&rtc));
	rtc_calendar_set(&rtc, KRTC_FORMAT_BIN);

	return PASS;
}

int t_rtc_get_clock(int argc, char **argv)
{
	struct tm rtc;

	rtc_calendar_get(&rtc, KRTC_FORMAT_BIN);
	printk(KERN_DEBUG "Get clock is %s", get_rtc_string(&rtc));;

	return PASS;
}

#endif
