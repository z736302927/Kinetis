#ifndef __DS3231_H
#define __DS3231_H

#ifdef __cplusplus
extern "C" {
#endif

/*The following program is modified by the user according to the hardware device, otherwise the driver cannot run.*/

/* Includes ------------------------------------------------------------------*/
#include "core_common.h"

#define DS3231_HOURS24                  0x00
#define DS3231_HOURS12                  0x01
#define DS3231_AM                       0x00
#define DS3231_PM                       0x01
#define DS3231_FORMAT_BIN               0x0U
#define DS3231_FORMAT_BCD               0x1U

uint8_t ds3231_GetTimeMode(void);
void ds3231_SetTimeMode(uint8_t Data);
uint8_t ds3231_GetTimeRegion(void);
void ds3231_SetTimeRegion(uint8_t Data);
void ds3231_ReadTime(uint8_t *pData, uint8_t Format);
void ds3231_SetTime(uint8_t *pData, uint8_t Format);
void ds3231_ReadTimeWithString(char *pData);
void ds3231_SetTimeWithString(char *pData);
void ds3231_ReadWeek(uint8_t *pData);
void ds3231_SetWeek(uint8_t Data);
void ds3231_SetAlarm1(uint8_t *pData, uint8_t DateorDay, uint8_t Data);
void ds3231_SetAlarm2(uint8_t *pData, uint8_t DateorDay, uint8_t Data);
void ds3231_GetTemperature(float *pData);
void ds3231_Test(void);;

/*The above procedure is modified by the user according to the hardware device, otherwise the driver cannot run.*/


#ifdef __cplusplus
}
#endif

#endif /* __DS3231_H */
