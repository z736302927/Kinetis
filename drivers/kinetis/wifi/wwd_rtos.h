/**
 * @brief WWD RTOS type mappings for Kinetis bare-metal port
 *
 * This file replaces WICED/RTOS/FreeRTOS/WWD/wwd_rtos.h.
 * Defines low-level RTOS types used by the WWD (WiFi driver) layer.
 */

#ifndef _INCLUDED_WWD_RTOS_H_
#define _INCLUDED_WWD_RTOS_H_

#include "FreeRTOS.h"
#include "semphr.h"
#include "task.h"

#ifdef __cplusplus
extern "C" {
#endif

#define RTOS_LOWEST_PRIORITY              (0)
#define RTOS_HIGHEST_PRIORITY             (9)
#define RTOS_DEFAULT_THREAD_PRIORITY      (5)

#define RTOS_USE_DYNAMIC_THREAD_STACK

#ifndef WWD_THREAD_STACK_SIZE
#define WWD_THREAD_STACK_SIZE             (2048)
#endif

typedef SemaphoreHandle_t   host_semaphore_type_t;
typedef SemaphoreHandle_t   host_mutex_type_t;
typedef TaskHandle_t        host_thread_type_t;
typedef QueueHandle_t       host_queue_type_t;

typedef struct {
	uint8_t info;
} host_rtos_thread_config_type_t;

#ifdef __cplusplus
}
#endif

#endif /* _INCLUDED_WWD_RTOS_H_ */
