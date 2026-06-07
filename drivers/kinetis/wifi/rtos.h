/**
 * @brief WICED RTOS type mappings for Kinetis bare-metal port
 *
 * This file replaces WICED/RTOS/FreeRTOS/WICED/rtos.h.
 * Maps WICED RTOS abstract types to Kinetis-compatible void* handles.
 */

#ifndef _WICED_RTOS_TYPES_H_
#define _WICED_RTOS_TYPES_H_

#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "timers.h"
#include "event_groups.h"
#include "wiced_result.h"
#include "wwd_rtos.h"

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************
 *                      Macros
 ******************************************************/

#define WICED_HARDWARE_IO_WORKER_THREAD             ((wiced_worker_thread_t*)0)
#define WICED_NETWORKING_WORKER_THREAD              ((wiced_worker_thread_t*)1)
#define WICED_PRIORITY_TO_NATIVE_PRIORITY(priority) (uint8_t)(10 - (priority))
#define WICED_END_OF_THREAD(thread)
#define WICED_END_OF_CURRENT_THREAD()
#define WICED_GET_THREAD_HANDLE(thread)             (thread)
#define WICED_GET_QUEUE_HANDLE(queue)               (queue)

/******************************************************
 *                    Constants
 ******************************************************/

#define HARDWARE_IO_WORKER_THREAD_STACK_SIZE             (512)
#define HARDWARE_IO_WORKER_THREAD_QUEUE_SIZE             (10)
#define HARDWARE_IO_WORKER_THREAD_PRIORITY               (4)

#define NETWORKING_WORKER_THREAD_STACK_SIZE              (4096)
#define NETWORKING_WORKER_THREAD_QUEUE_SIZE              (15)
#define NETWORKING_WORKER_THREAD_PRIORITY                (6)

#define RTOS_NAME                     "Kinetis"
#define RTOS_VERSION                  "bare-metal"

/******************************************************
 *                 Type Definitions
 ******************************************************/

typedef EventGroupHandle_t   wiced_event_flags_t;
typedef host_semaphore_type_t wiced_semaphore_t;
typedef SemaphoreHandle_t     wiced_mutex_t;
typedef TaskHandle_t          wiced_thread_t;
typedef QueueHandle_t         wiced_queue_t;

typedef void (*timer_handler_t)(void *arg);

typedef struct {
	TimerHandle_t    handle;
	timer_handler_t  function;
	void            *arg;
} wiced_timer_t;

typedef struct {
	wiced_thread_t   thread;
	wiced_queue_t    event_queue;
} wiced_worker_thread_t;

typedef wiced_result_t (*event_handler_t)(void *arg);

typedef struct {
	event_handler_t         function;
	void                   *arg;
	wiced_timer_t           timer;
	wiced_worker_thread_t  *thread;
} wiced_timed_event_t;

/******************************************************
 *                 Global Variables
 ******************************************************/

extern wiced_worker_thread_t wiced_hardware_io_worker_thread;
extern wiced_worker_thread_t wiced_networking_worker_thread;

#ifdef __cplusplus
}
#endif

#endif /* _WICED_RTOS_TYPES_H_ */
