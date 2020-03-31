#ifndef __BSP_RTC_H
#define __BSP_RTC_H

#ifdef __cplusplus
 extern "C" {
#endif

/*The following program is modified by the user according to the hardware device, otherwise the driver cannot run.*/
   
/* Includes ------------------------------------------------------------------*/
#include "core_common.h"

void RTC_CalendarConfig(uint8_t Year, uint8_t Month, uint8_t Date, \
                        uint8_t Hours, uint8_t Minutes, uint8_t Seconds, uint8_t WeekDay);
void RTC_CalendarShow(uint8_t *Year, uint8_t *Month, uint8_t *Date, \
                      uint8_t *Hours, uint8_t *Minutes, uint8_t *Seconds, uint8_t *WeekDay);

/*The above procedure is modified by the user according to the hardware device, otherwise the driver cannot run.*/


#ifdef __cplusplus
}
#endif

#endif /* __BSP_RTC_H */
