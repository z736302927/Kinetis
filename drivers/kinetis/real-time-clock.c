#include "kinetis/real-time-clock.h"
#include "kinetis/ds3231.h"
#include "kinetis/idebug.h"

#include "stdio.h"

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
void rtc_calendar_config(u8 year, u8 month, u8 date,
    u8 hours, u8 minutes, u8 seconds, u8 weekday, u8 format)
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

    sdate.Year = year;
    sdate.Month = month;
    sdate.Date = date;

    if (weekday != 0)
        sdate.WeekDay = weekday;

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

    stime.Hours = hours;
    stime.Minutes = minutes;
    stime.Seconds = seconds;
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

    snprintf(time, sizeof(time), "%02d%02d%02d%02d%02d%02d", year, month, date,
        hours, minutes, seconds);
    ds3231_SetTimeWithString(time);

    if (weekday != 0)
        ds3231_SetWeek(weekday);

#endif
}

/**
  * @brief  Display the current time and date.
  * @param  showtime : pointer to buffer
  * @param  showdate : pointer to buffer
  * @retval None
  */
void rtc_calendar_show(u8 *year, u8 *month, u8 *date,
    u8 *hours, u8 *minutes, u8 *seconds, u8 *weekday, u8 format)
{
#ifdef USING_CHIP_RTC
    RTC_DateTypeDef sdate;
    RTC_TimeTypeDef stime;

    /* Get the RTC current Date */
    if (format == KRTC_FORMAT_BIN)
        HAL_RTC_GetDate(&hrtc, &sdate, RTC_FORMAT_BIN);
    else
        HAL_RTC_GetDate(&hrtc, &sdate, RTC_FORMAT_BCD);

    *year = sdate.Year;
    *month = sdate.Month;
    *date = sdate.Date;

    if (weekday != NULL)
        *weekday = sdate.WeekDay;

    /* Get the RTC current Time */
    if (format == KRTC_FORMAT_BIN)
        HAL_RTC_GetTime(&hrtc, &stime, RTC_FORMAT_BIN);
    else
        HAL_RTC_GetTime(&hrtc, &stime, RTC_FORMAT_BCD);

    *hours = stime.Hours;
    *minutes = stime.Minutes;
    *seconds = stime.Seconds;
#endif

#ifdef USING_DS3231
    u8 time[6], week;

    if (Format == KRTC_FORMAT_BIN)
        ds3231_ReadTime(time, DS3231_FORMAT_BIN);
    else
        ds3231_ReadTime(time, DS3231_FORMAT_BCD);

    ds3231_ReadWeek(&week);
    *year = time[5];
    *month = time[4];
    *date = time[3];
    *weekday = week;
    *hours = time[2];
    *minutes = time[1];
    *seconds = time[0];
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
    u8 year, month, date;
    u8 hours, minutes, seconds, weekday;
    char time[25];

    year = random_get8bit() % 100;
    month = random_get8bit() % 12;
    date = random_get8bit() % 28;
    hours = random_get8bit() % 24;
    minutes = random_get8bit() % 60;
    seconds = random_get8bit() % 60;
    weekday = random_get8bit() % 7;

    if (argc > 1)
        year = strtoul(argv[1], &argv[1], 10);

    if (argc > 2)
        month = strtoul(argv[2], &argv[2], 10);

    if (argc > 3)
        date = strtoul(argv[3], &argv[3], 10);

    if (argc > 4)
        hours = strtoul(argv[4], &argv[4], 10);

    if (argc > 5)
        minutes = strtoul(argv[5], &argv[5], 10);

    if (argc > 6)
        seconds = strtoul(argv[6], &argv[6], 10);

    if (argc > 7)
        weekday = strtoul(argv[7], &argv[7], 10);

    snprintf(time, sizeof(time), "20%02d/%02d/%02d/% 02d:%02d:%02d",
    year, month, date,hours, minutes, seconds);
    printk(KERN_DEBUG "Set clock is %s", time);
    rtc_calendar_config(year, month, date,
        hours, minutes, seconds, weekday, KRTC_FORMAT_BIN);

    return PASS;
}

int t_rtc_get_clock(int argc, char **argv)
{
    u8 year, month, date;
    u8 hours, minutes, seconds, weekday;
    char time[25];

    rtc_calendar_show(&year, &month, &date,
        &hours, &minutes, &seconds, &weekday, KRTC_FORMAT_BIN);
    snprintf(time, sizeof(time), "20%02d/%02d/%02d/% 02d:%02d:%02d",
        year, month, date,hours, minutes, seconds);
    printk(KERN_DEBUG "Get clock is %s", time);

    return PASS;
}

#endif
