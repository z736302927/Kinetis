#include "timer/bsp_rtc.h"

/*The following program is modified by the user according to the hardware device, otherwise the driver cannot run.*/

/**
  * @step 1:  Modify the corresponding function according to the modified area and the corresponding function name.
  * @step 2:  Modify four areas: GPIO_PORT/GPIO_PIN/Led_TypeDef/LEDn.
  * @step 3:  .
  * @step 4:  .
  * @step 5:
  */

#include "rtc.h"
#include "chip/ds3231.h"
#include "stdio.h"

/*The above procedure is modified by the user according to the hardware device, otherwise the driver cannot run.*/

#define RTC_NUMBER                      2
#define HOURS24                         0x00
#define HOURS24                         0x00
#define HOURS24                         0x00
#define HOURS24                         0x00
#define HOURS24                         0x00
#define HOURS24                         0x00
#define HOURS24                         0x00

/**
  * @brief  Writes a data in a specified RTC Backup data register.
  * @retval None
  */
void RTC_BKUPWrite(void)
{
  if(RTC_NUMBER > 0)
  {
    /*##-3- Writes a data in a RTC Backup data Register1 #######################*/
    HAL_RTCEx_BKUPWrite(&hrtc, RTC_BKP_DR1, 0x32F2);
  }
  
  if(RTC_NUMBER > 1)
  {
    
  }
}

/**
  * @brief  Read a data in a specified RTC Backup data register.
  * @retval None
  */
void RTC_BKUPRead(uint32_t *Data)
{
  if(RTC_NUMBER > 0)
  {
    /*##-3- Read a data in a RTC Backup data Register1 #######################*/
    *Data = HAL_RTCEx_BKUPRead(&hrtc, RTC_BKP_DR1);
  }
  
  if(RTC_NUMBER > 1)
  {
    
  }
}

/**
  * @brief  Configure the current time and date.
  * @param  None
  * @retval None
  */
void RTC_CalendarConfig(uint8_t Year, uint8_t Month, uint8_t Date, \
                        uint8_t Hours, uint8_t Minutes, uint8_t Seconds, uint8_t WeekDay)
{
  if(RTC_NUMBER > 0)
  {
    RTC_DateTypeDef sdatestructure;
    RTC_TimeTypeDef stimestructure;

    /*##-1- Configure the Date #################################################*/
    /* Set Date: Wednesday May 1th 2019 */
    HAL_RTC_GetDate(&hrtc, &sdatestructure, RTC_FORMAT_BIN);
    sdatestructure.Year = Year;
    sdatestructure.Month = Month;
    sdatestructure.Date = Date;
    if(WeekDay != 0)
    {
      sdatestructure.WeekDay = WeekDay;
    }
    
    if(HAL_RTC_SetDate(&hrtc,&sdatestructure,RTC_FORMAT_BCD) != HAL_OK)
    {
      /* Initialization Error */
      Error_Handler();
    }

    /*##-2- Configure the Time #################################################*/
    /* Set Time: 00:00:00 */
    HAL_RTC_GetTime(&hrtc, &stimestructure, RTC_FORMAT_BIN);
    stimestructure.Hours = Hours;
    stimestructure.Minutes = Minutes;
    stimestructure.Seconds = Seconds;
//    stimestructure.TimeFormat = RTC_HOURFORMAT12_AM;
//    stimestructure.DayLightSaving = RTC_DAYLIGHTSAVING_NONE ;
//    stimestructure.StoreOperation = RTC_STOREOPERATION_RESET;

    if (HAL_RTC_SetTime(&hrtc, &stimestructure, RTC_FORMAT_BCD) != HAL_OK)
    {
      /* Initialization Error */
      Error_Handler();
    }

    /*##-3- Writes a data in a RTC Backup data Register1 #######################*/
    RTC_BKUPWrite();
  }
  
  if(RTC_NUMBER > 1)
  {
    char time[13];
    
    snprintf(time, sizeof(time), "%02d%02d%02d%02d%02d%02d", Year, Month, Date, \
                                                             Hours, Minutes, Seconds);
    ds3231_SetTimeWithString(time);
    if(WeekDay != 0)
    {
      ds3231_SetWeek(WeekDay);
    }
  }
}

/**
  * @brief  Display the current time and date.
  * @param  showtime : pointer to buffer
  * @param  showdate : pointer to buffer
  * @retval None
  */
void RTC_CalendarShow(uint8_t *Year, uint8_t *Month, uint8_t *Date, \
                      uint8_t *Hours, uint8_t *Minutes, uint8_t *Seconds, uint8_t *WeekDay)
{
  if(RTC_NUMBER > 0)
  {
    RTC_DateTypeDef sdatestructure;
    RTC_TimeTypeDef stimestructure;
    
    /* Get the RTC current Date */
    HAL_RTC_GetDate(&hrtc, &sdatestructure, RTC_FORMAT_BIN);
    *Year = sdatestructure.Year;
    *Month = sdatestructure.Month;
    *Date = sdatestructure.Date;
    if(WeekDay != NULL)
    {
      *WeekDay = sdatestructure.WeekDay;
    }
      
    /* Get the RTC current Time */
    HAL_RTC_GetTime(&hrtc, &stimestructure, RTC_FORMAT_BIN);
    *Hours = stimestructure.Hours;
    *Minutes = stimestructure.Minutes;
    *Seconds = stimestructure.Seconds;
  }
  
  if(RTC_NUMBER > 1)
  {
    uint8_t time[6], week;
    
    ds3231_ReadTimeWithDec(time);
    ds3231_ReadWeek(&week);
    *Year = time[5];
    *Month = time[4];
    *Date = time[3];
    *WeekDay = week;
    *Hours = time[2];
    *Minutes = time[1];
    *Seconds = time[0];
  }
}

/**
  * @brief  Configure the current time and date.
  * @param  None
  * @retval None
  */
void RTC_SetTimeFormat(uint8_t Data)
{
  if(RTC_NUMBER > 0)
  {
    /*##-3- Writes a data in a RTC Backup data Register1 #######################*/
    HAL_RTCEx_BKUPWrite(&hrtc, RTC_BKP_DR1, 0x32F2);
  }
  
  if(RTC_NUMBER > 1)
  {
//    ds3231_SetTimeMode
//    ds3231_SetTimeRegion  
  }
}

/**
  * @brief  Configure the current time and date.
  * @param  None
  * @retval None
  */
uint8_t RTC_GetTimeFormat(void)
{
  if(RTC_NUMBER > 0)
  {
    /*##-3- Writes a data in a RTC Backup data Register1 #######################*/
    HAL_RTCEx_BKUPWrite(&hrtc, RTC_BKP_DR1, 0x32F2);
  }
  
  if(RTC_NUMBER > 1)
  {
//    ds3231_GetTimeMode
//    ds3231_GetTimeRegion  
  }
}