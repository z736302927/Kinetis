#ifndef __DS3231_H
#define __DS3231_H

#ifdef __cplusplus
extern "C" {
#endif

/* The following program is modified by the user according to the hardware device, otherwise the driver cannot run. */

/* Includes ------------------------------------------------------------------*/
#include "kinetis/core_common.h"

#define DS3231_HOURS24                  0x00
#define DS3231_HOURS12                  0x01
#define DS3231_AM                       0x00
#define DS3231_PM                       0x01
#define DS3231_FORMAT_BIN               0x0U
#define DS3231_FORMAT_BCD               0x1U

u8 ds3231_GetTimeMode(void);
void ds3231_SetTimeMode(u8 Data);
u8 ds3231_GetTimeRegion(void);
void ds3231_SetTimeRegion(u8 Data);
void ds3231_ReadTime(u8 *pData, u8 Format);
void ds3231_SetTime(u8 *pData, u8 Format);
void ds3231_ReadTimeWithString(char *pData);
void ds3231_SetTimeWithString(char *pData);
void ds3231_ReadWeek(u8 *pData);
void ds3231_SetWeek(u8 Data);
void ds3231_SetAlarm1(u8 *pData, u8 DateorDay, u8 Data);
void ds3231_SetAlarm2(u8 *pData, u8 DateorDay, u8 Data);
void ds3231_GetTemperature(float *pData);
void ds3231_Test(void);;

/* The above procedure is modified by the user according to the hardware device, otherwise the driver cannot run. */


#ifdef __cplusplus
}
#endif

#endif /* __DS3231_H */
