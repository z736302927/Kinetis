/* SPDX-License-Identifier: GPL-2.0 */
#ifndef __ASM_GENERIC_CURRENT_H
#define __ASM_GENERIC_CURRENT_H

#include <linux/thread_info.h>

extern struct task_struct init_task;

#define get_current() (&init_task)
#define current get_current()

#endif /* __ASM_GENERIC_CURRENT_H */
