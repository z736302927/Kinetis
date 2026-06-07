
#ifndef AP6181_LWIP_SYS_ARCH_H
#define AP6181_LWIP_SYS_ARCH_H

#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

#ifdef __cplusplus
extern "C" {
#endif

#define SYS_MBOX_NULL   ((sys_mbox_t)0)
#define SYS_SEM_NULL    ((sys_sem_t)0)

typedef SemaphoreHandle_t  sys_sem_t;
typedef QueueHandle_t      sys_mbox_t;
typedef TaskHandle_t       sys_thread_t;

uint16_t sys_rand16(void);

int lwip_periodic_handle(void);

#ifdef __cplusplus
}
#endif

#endif /* AP6181_LWIP_SYS_ARCH_H */
