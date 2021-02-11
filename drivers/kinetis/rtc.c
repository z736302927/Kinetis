#include "kinetis/rtc.h"

/* The following program is modified by the user according to the hardware device, otherwise the driver cannot run. */

/**
  * @step 1:  Modify the corresponding function according to the modified area and the corresponding function name.
  * @step 2:  Modify four areas: GPIO_PORT/GPIO_PIN/Led_TypeDef/LEDn.
  * @step 3:  .
  * @step 4:  .
  * @step 5:
  */

#include "rtc.h"
#include "kinetis/ds3231.h"
#include "stdio.h"
#include "kinetis/idebug.h"

/* The above procedure is modified by the user according to the hardware device, otherwise the driver cannot run. */

#define USING_CHIP_RTC
//#define USING_DS3231

#define HOURS24                         0x00

/**
  * @brief  Writes a data in a specified RTC Backup data register.
  * @retval None
  */
void RTC_BKUPWrite(void)
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
void RTC_BKUPRead(u32 *Data)
{
#ifdef USING_CHIP_RTC
    /*##-3- Read a data in a RTC Backup data Register1 #######################*/
    *Data = HAL_RTCEx_BKUPRead(&hrtc, RTC_BKP_DR1);
#endif
}

/**
  * @brief  Configure the current time and date.
  * @param  None
  * @retval None
  */
void RTC_CalendarConfig(u8 Year, u8 Month, u8 Date,
    u8 Hours, u8 Minutes, u8 Seconds, u8 WeekDay, u8 Format)
{
#ifdef USING_CHIP_RTC
    RTC_DateTypeDef sdate;
    RTC_TimeTypeDef stime;

    /*##-1- Configure the Date ###############################################*/
    /* Set Date: Wednesday May 1th 2019 */
    if (Format == KRTC_FORMAT_BIN)
        HAL_RTC_GetDate(&hrtc, &sdate, RTC_FORMAT_BIN);
    else
        HAL_RTC_GetDate(&hrtc, &sdate, RTC_FORMAT_BCD);

    sdate.Year = Year;
    sdate.Month = Month;
    sdate.Date = Date;

    if (WeekDay != 0)
        sdate.WeekDay = WeekDay;

    if (Format == KRTC_FORMAT_BIN)
        HAL_RTC_SetDate(&hrtc, &sdate, RTC_FORMAT_BIN);
    else
        HAL_RTC_SetDate(&hrtc, &sdate, RTC_FORMAT_BCD);

    /*##-2- Configure the Time ###############################################*/
    /* Set Time: 00:00:00 */
    if (Format == KRTC_FORMAT_BIN)
        HAL_RTC_GetTime(&hrtc, &stime, RTC_FORMAT_BIN);
    else
        HAL_RTC_GetTime(&hrtc, &stime, RTC_FORMAT_BCD);

    stime.Hours = Hours;
    stime.Minutes = Minutes;
    stime.Seconds = Seconds;
    stime.TimeFormat = RTC_HOURFORMAT12_AM;
    stime.DayLightSaving = RTC_DAYLIGHTSAVING_NONE ;
    stime.StoreOperation = RTC_STOREOPERATION_RESET;

    if (Format == KRTC_FORMAT_BIN)
        HAL_RTC_SetTime(&hrtc, &stime, RTC_FORMAT_BIN);
    else
        HAL_RTC_SetTime(&hrtc, &stime, RTC_FORMAT_BCD);

    /*##-3- Writes a data in a RTC Backup data Register1 #####################*/
    RTC_BKUPWrite();
#endif

#ifdef USING_DS3231
    char time[13];

    snprintf(time, sizeof(time), "%02d%02d%02d%02d%02d%02d", Year, Month, Date,
        Hours, Minutes, Seconds);
    ds3231_SetTimeWithString(time);

    if (WeekDay != 0)
        ds3231_SetWeek(WeekDay);

#endif
}

/**
  * @brief  Display the current time and date.
  * @param  showtime : pointer to buffer
  * @param  showdate : pointer to buffer
  * @retval None
  */
void RTC_CalendarShow(u8 *Year, u8 *Month, u8 *Date,
    u8 *Hours, u8 *Minutes, u8 *Seconds, u8 *WeekDay, u8 Format)
{
#ifdef USING_CHIP_RTC
    RTC_DateTypeDef sdate;
    RTC_TimeTypeDef stime;

    /* Get the RTC current Date */
    if (Format == KRTC_FORMAT_BIN)
        HAL_RTC_GetDate(&hrtc, &sdate, RTC_FORMAT_BIN);
    else
        HAL_RTC_GetDate(&hrtc, &sdate, RTC_FORMAT_BCD);

    *Year = sdate.Year;
    *Month = sdate.Month;
    *Date = sdate.Date;

    if (WeekDay != NULL)
        *WeekDay = sdate.WeekDay;

    /* Get the RTC current Time */
    if (Format == KRTC_FORMAT_BIN)
        HAL_RTC_GetTime(&hrtc, &stime, RTC_FORMAT_BIN);
    else
        HAL_RTC_GetTime(&hrtc, &stime, RTC_FORMAT_BCD);

    *Hours = stime.Hours;
    *Minutes = stime.Minutes;
    *Seconds = stime.Seconds;
#endif

#ifdef USING_DS3231
    u8 time[6], week;

    if (Format == KRTC_FORMAT_BIN)
        ds3231_ReadTime(time, DS3231_FORMAT_BIN);
    else
        ds3231_ReadTime(time, DS3231_FORMAT_BCD);

    ds3231_ReadWeek(&week);
    *Year = time[5];
    *Month = time[4];
    *Date = time[3];
    *WeekDay = week;
    *Hours = time[2];
    *Minutes = time[1];
    *Seconds = time[0];
#endif
}

/**
  * @brief  Configure the current time and date.
  * @param  None
  * @retval None
  */
void RTC_SetTimeFormat(u8 Data)
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
u8 RTC_GetTimeFormat(void)
{
#ifdef USING_CHIP_RTC

#endif

#ifdef USING_DS3231

#endif
}

#ifdef DESIGN_VERIFICATION_RTC
#include "kinetis/test-kinetis.h"
#include "kinetis/random-gene.h"
#include "stdlib.h"

int t_RTC_SetClock(int argc, char **argv)
{
    u8 Year, Month, Date;
    u8 Hours, Minutes, Seconds, WeekDay;
    char Time[25];

    Year = random_get8bit() % 100;
    Month = random_get8bit() % 12;
    Date = random_get8bit() % 28;
    Hours = random_get8bit() % 24;
    Minutes = random_get8bit() % 60;
    Seconds = random_get8bit() % 60;
    WeekDay = random_get8bit() % 7;

    if (argc > 1)
        Year = strtoul(argv[1], &argv[1], 10);

    if (argc > 2)
        Month = strtoul(argv[2], &argv[2], 10);

    if (argc > 3)
        Date = strtoul(argv[3], &argv[3], 10);

    if (argc > 4)
        Hours = strtoul(argv[4], &argv[4], 10);

    if (argc > 5)
        Minutes = strtoul(argv[5], &argv[5], 10);

    if (argc > 6)
        Seconds = strtoul(argv[6], &argv[6], 10);

    if (argc > 7)
        WeekDay = strtoul(argv[7], &argv[7], 10);

    snprintf(Time, sizeof(Time), "20%02d/%02d/%02d/% 02d:%02d:%02d", Year, Month, Date,
        Hours, Minutes, Seconds);
    kinetis_print_trace(KERN_DEBUG, "Set clock is %s", Time);
    RTC_CalendarConfig(Year, Month, Date, Hours, Minutes, Seconds, WeekDay, KRTC_FORMAT_BIN);

    return PASS;
}

int t_RTC_GetClock(int argc, char **argv)
{
    u8 Year, Month, Date;
    u8 Hours, Minutes, Seconds, WeekDay;
    char Time[25];

    RTC_CalendarShow(&Year, &Month, &Date, &Hours, &Minutes, &Seconds, &WeekDay, KRTC_FORMAT_BIN);
    snprintf(Time, sizeof(Time), "20%02d/%02d/%02d/% 02d:%02d:%02d", Year, Month, Date,
        Hours, Minutes, Seconds);
    kinetis_print_trace(KERN_DEBUG, "Get clock is %s", Time);

    return PASS;
}

#endif
