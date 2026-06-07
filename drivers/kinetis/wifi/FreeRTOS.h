/**
 * @brief Minimal FreeRTOS header stubs for Kinetis bare-metal port
 *
 * Provides type definitions and macros that WICED SDK expects from FreeRTOS,
 * without requiring the actual FreeRTOS kernel.
 */

#ifndef _FREERTOS_H_
#define _FREERTOS_H_

#ifdef __cplusplus
extern "C" {
#endif

#define configMAX_PRIORITIES             10

#ifndef pdFALSE
#define pdFALSE                          0
#endif
#ifndef pdTRUE
#define pdTRUE                           1
#endif

#define pdPASS                           1
#define pdFAIL                           0
#define pdMS_TO_TICKS(ms)                (ms)
#define portMAX_DELAY                    0xFFFFFFFFUL
#define portTICK_PERIOD_MS               1
#define tskIDLE_PRIORITY                 0

typedef void *TaskHandle_t;
typedef void *QueueHandle_t;
typedef void *SemaphoreHandle_t;
typedef void *TimerHandle_t;
typedef void *EventGroupHandle_t;
typedef void *StaticSemaphore_t;

typedef int32_t  BaseType_t;
typedef uint32_t UBaseType_t;
typedef uint32_t TickType_t;

#define taskVALIDATE_TCB(thread)

#ifdef __cplusplus
}
#endif

#endif /* _FREERTOS_H_ */
