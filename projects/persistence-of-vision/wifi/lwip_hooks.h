
#ifndef __LWIP_HOOKS_H__
#define __LWIP_HOOKS_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "kinetis/basic_timer.h"

#define sys_now()   ((u32)basic_timer_get_ms())

#ifdef __cplusplus
}
#endif

#endif /* __LWIP_HOOKS_H__ */
