#ifndef __K_TIMTASK_H
#define __K_TIMTASK_H

#ifdef __cplusplus
extern "C" {
#endif

/* The following program is modified by the user according to the hardware device, otherwise the driver cannot run. */

/* Includes ------------------------------------------------------------------*/
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
	u32 interval;
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

struct tim_task_stats {
	u32 total_tasks_created;
	u32 total_tasks_executed;
	u32 total_tasks_failed;
	u64 total_execution_time_ms;
	u32 max_execution_time_ms;
	u32 min_execution_time_ms;
	ktime_t system_start_time;
	u32 high_priority_tasks;
	u32 normal_priority_tasks;
	u32 low_priority_tasks;
};

/* support at most 1024 task, use 1024 size hash table */
#define FMU_SCH_HASH_BITS	10

/**
 * struct fmu_sch_bw_info: schedule information for bandwidth domain
 *
 * @bus_bw: array to keep track of bandwidth already used at each uframes
 *
 * treat a HS root port as a bandwidth domain, but treat a SS root port as
 * two bandwidth domains, one for IN eps and another for OUT eps.
 */
struct fmu_sch_bw_info {
	u16 *bus_bw;
};

/**
 * struct fmu_sch_task_info: schedule information for endpoint
 *
 * @esit: unit is 125us, equal to 2 << Interval field in ep-context
 * @interval_repeat: number of @esit in a period
 * @num_budget_frames: number of continuous uframes
 *		(@repeat==1) scheduled within the interval
 * @bw_cost_per_frame: bandwidth cost per microframe
 * @hentry: hash table entry
 * @list: linked into bandwidth domain which it belongs to
 * @tt_endpoint: linked into mu3h_sch_tt's list which it belongs to
 * @bw_info: bandwidth domain which this endpoint belongs
 * @sch_tt: mu3h_sch_tt linked into
 * @ep_type: endpoint type
 * @maxpkt: max packet size of endpoint
 * @ep: address of usb_host_endpoint struct
 * @allocated: the bandwidth is aready allocated from bus_bw
 * @offset: which uframe of the interval that transfer should be
 *		scheduled first time within the interval
 * @repeat: the time gap between two uframes that transfers are
 *		scheduled within a interval. in the simple algorithm, only
 *		assign 0 or 1 to it; 0 means using only one uframe in a
 *		interval, and 1 means using @num_budget_frames
 *		continuous uframes
 * @pkts: number of packets to be transferred in the scheduled uframes
 * @cs_count: number of CS that host will trigger
 * @burst_mode: burst mode for scheduling. 0: normal burst mode,
 *		distribute the bMaxBurst+1 packets for a single burst
 *		according to @pkts and @repeat, repeate the burst multiple
 *		times; 1: distribute the (bMaxBurst+1)*(Mult+1) packets
 *		according to @pkts and @repeat. normal mode is used by
 *		default
 * @bw_budget_table: table to record bandwidth budget per microframe
 */
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
	/*
	 * mtk xHCI scheduling information put into reserved DWs
	 * in ep context
	 */
	u32 offset;
	u32 bw_budget_table[];
};

const char *
tim_task_decode(struct tim_task *task);
int tim_task_add(struct tim_task *tim_task,
	char *name, u32 interval, bool auto_load, bool sched,
	tim_task_cb callback);
void tim_task_drop(struct tim_task *tim_task);
void tim_task_suspend(struct tim_task *tim_task);
void tim_task_resume(struct tim_task *tim_task);
void tim_task_loop(void);

// Enhanced functions for optimization
struct tim_task *tim_task_find_by_name(const char *name);
struct tim_task *tim_task_find_by_id(u32 task_id);
int tim_task_set_priority(struct tim_task *tim_task, u32 new_priority);
void tim_task_get_stats(struct tim_task_stats *stats);
void tim_task_reset_stats(void);
void tim_task_set_profiling(bool enable);
void tim_task_print_performance_report(void);
void tim_task_print_info(struct tim_task *tim_task);
void tim_task_cleanup_all(void);
struct tim_task *tim_task_get_next_task(void);

int fmu_sch_add_task(struct tim_task *task);
void fmu_sch_drop_task(struct tim_task *task);

struct fmu_sch_bw_info *task_sch_init(u32 max_esit);
void task_sch_exit(struct fmu_sch_bw_info *sch_bw);

/* The above procedure is modified by the user according to the hardware device, otherwise the driver cannot run. */

#ifdef __cplusplus
}
#endif

#endif /* __K_TIMTASK_H */
