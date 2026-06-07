/**
 * @brief Minimal task header stub for Kinetis bare-metal port
 */

#ifndef _TASK_H_
#define _TASK_H_

#include "FreeRTOS.h"

#ifdef __cplusplus
extern "C" {
#endif

void vTaskDelay(uint32_t ticks);
void vTaskDelete(TaskHandle_t task);

#ifdef __cplusplus
}
#endif

#endif /* _TASK_H_ */
