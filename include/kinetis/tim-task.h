#ifndef __K_TIMTASK_H
#define __K_TIMTASK_H

#ifdef __cplusplus
extern "C" {
#endif

#include <generated/deconfig.h>
#include <linux/types.h>
#include <linux/list.h>
#include <linux/ktime.h>
#include <linux/hashtable.h>

#define DBG_BUF_EN	64

struct tim_task;

typedef void (*tim_task_cb)(struct tim_task *);

struct tim_task {
	const char *name;
	ktime_t timeout;
	u32 interval_ms;
	tim_task_cb callback;
	struct list_head list;
	struct list_head main_list;
	bool auto_load;
	bool sched;

	u32 task_id;
	u32 priority;
	ktime_t created_time;
	u32 execution_count;
	u32 last_execution_time;
	u64 total_execution_time;
};

#define FMU_SCH_HASH_BITS	10

struct fmu_sch_bw_info {
	u16 *bus_bw;
};

struct fmu_sch_task_info {
	ktime_t elapsed_time_us;
	ktime_t begin_time_us;
	u32 interval;
	u32 interval_repeat;
	u32 num_budget_frames;
	u32 bw_cost_per_frame;
	struct list_head list;
	struct hlist_node hentry;
	struct fmu_sch_bw_info *bw_info;
	struct tim_task *task;
	bool allocated;
	u32 offset;
	u32 bw_budget_table[];
};

const char *tim_task_decode(struct tim_task *task);
int tim_task_add(struct tim_task *task, const char *name, u32 interval,
	bool auto_load, bool sched, tim_task_cb callback);
void tim_task_drop(struct tim_task *task);
void tim_task_suspend(struct tim_task *task);
void tim_task_resume(struct tim_task *task);
void tim_task_loop(void);

struct tim_task *tim_task_find_by_name(const char *name);
struct tim_task *tim_task_find_by_id(u32 task_id);
int tim_task_set_priority(struct tim_task *task, u32 new_priority);
int tim_task_set_interval(struct tim_task *task, u32 new_interval);
void tim_task_cleanup_all(void);
struct tim_task *tim_task_get_next_task(void);

int fmu_sch_add_task(struct tim_task *task);
void fmu_sch_drop_task(struct tim_task *task);

#ifdef __cplusplus
}
#endif

#endif
