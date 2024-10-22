
#include <generated/deconfig.h>
#include <linux/slab.h>
#include <linux/errno.h>

#include <kinetis/tim-task.h>

/* 1000 microsecond */
#define FMU_SCH_BW_BOUNDARY	1000

/**
 * To simplify scheduler algorithm, set a upper limit for ESIT,
 * if a synchromous ep's ESIT is larger than @FMU_SCH_MAX_ESIT,
 * round down to the limit value, that means allocating more
 * bandwidth to it. The unit is microsecond.
 */
static u32 fmu_sch_max_esit = 1000;
#define FMU_SCH_BW_INDEX(x)	((x) & (fmu_sch_max_esit - 1))

/* schedule error type */
#define ESCH_SS_Y6		1001
#define ESCH_SS_OVERLAP		1002
#define ESCH_CS_OVERFLOW	1003
#define ESCH_BW_OVERFLOW	1004
#define ESCH_FIXME		1005

static char *sch_error_string(int err_num)
{
	switch (err_num) {
	case ESCH_SS_Y6:
		return "Can't schedule Start-Split in Y6";
	case ESCH_SS_OVERLAP:
		return "Can't find a suitable Start-Split location";
	case ESCH_CS_OVERFLOW:
		return "The last Complete-Split is greater than 7";
	case ESCH_BW_OVERFLOW:
		return "Bandwidth exceeds the maximum limit";
	case ESCH_FIXME:
		return "FIXME, to be resolved";
	default:
		return "Unknown";
	}
}

static struct fmu_sch_task_info *
create_sch_task(struct fmu_sch_bw_info *bw_info, struct tim_task *task)
{
	struct fmu_sch_task_info *sch_task;
	u32 len_bw_budget_table = 1;
	size_t mem_size;

	mem_size = sizeof(struct fmu_sch_task_info) +
		len_bw_budget_table * sizeof(u32);
	sch_task = kzalloc(mem_size, GFP_KERNEL);
	if (!sch_task)
		return ERR_PTR(-ENOMEM);

	sch_task->bw_info = bw_info;
	sch_task->task = task;
	INIT_LIST_HEAD(&sch_task->list);
	INIT_HLIST_NODE(&sch_task->hentry);

	return sch_task;
}

static void setup_sch_info(struct tim_task *task,
	struct fmu_sch_task_info *sch_task)
{
	u32 *bwb_table = sch_task->bw_budget_table;

	sch_task->begin_time_us = ktime_to_us(ktime_get());
	task->callback(task);
	sch_task->elapsed_time_us =
		ktime_to_us(ktime_get()) - sch_task->begin_time_us;

	sch_task->interval = task->interval;
	sch_task->num_budget_frames = 1;
	sch_task->bw_cost_per_frame = sch_task->elapsed_time_us;
	bwb_table[0] = sch_task->bw_cost_per_frame;
}

/* Get maximum bandwidth when we schedule at offset slot. */
static u32 get_max_bw(struct fmu_sch_bw_info *sch_bw,
	struct fmu_sch_task_info *sch_task, u32 offset)
{
	u32 max_bw = 0;
	u32 bw;
	int i, j, k;

	for (i = 0; i < sch_task->interval_repeat; i++) {
		u32 base = offset + i * sch_task->interval;

		for (j = 0; j < sch_task->num_budget_frames; j++) {
			k = FMU_SCH_BW_INDEX(base + j);
			bw = sch_bw->bus_bw[k] + sch_task->bw_budget_table[j];
			if (bw > max_bw)
				max_bw = bw;
		}
	}
	return max_bw;
}

static void update_bus_bw(struct fmu_sch_bw_info *sch_bw,
	struct fmu_sch_task_info *sch_task, bool used)
{
	u32 base;
	int i, j, k;

	for (i = 0; i < sch_task->interval_repeat; i++) {
		base = sch_task->offset + i * sch_task->interval;
		for (j = 0; j < sch_task->num_budget_frames; j++) {
			k = FMU_SCH_BW_INDEX(base + j);
			if (used)
				sch_bw->bus_bw[k] += sch_task->bw_budget_table[j];
			else
				sch_bw->bus_bw[k] -= sch_task->bw_budget_table[j];
		}
	}
}

static int load_task_bw(struct fmu_sch_bw_info *sch_bw,
	struct fmu_sch_task_info *sch_task, bool loaded)
{
	/* update bus bandwidth info */
	update_bus_bw(sch_bw, sch_task, loaded);
	sch_task->allocated = loaded;

	return 0;
}

static int check_sch_bw(struct fmu_sch_task_info *sch_task)
{
	struct fmu_sch_bw_info *sch_bw = sch_task->bw_info;
	u32 offset;
	u32 worst_bw;
	u32 min_bw = ~0;
	int min_index = -1;
	int ret = 0;

	/*
	 * Search through all possible schedule microframes.
	 * and find a microframe where its worst bandwidth is minimum.
	 */
	for (offset = 0; offset < sch_task->interval; offset++) {
		worst_bw = get_max_bw(sch_bw, sch_task, offset);
		if (worst_bw > FMU_SCH_BW_BOUNDARY)
			continue;

		if (min_bw > worst_bw) {
			min_bw = worst_bw;
			min_index = offset;
		}

		if (min_bw == 0)
			break;
	}

	if (min_index < 0)
		return ret ? ret : -ESCH_BW_OVERFLOW;

	sch_task->offset = min_index;

	return load_task_bw(sch_bw, sch_task, true);
}

static void destroy_sch_task(struct fmu_sch_task_info *sch_task)
{
	/* only release task bw check passed by check_sch_bw() */
	if (sch_task->allocated)
		load_task_bw(sch_task->bw_info, sch_task, false);

	list_del(&sch_task->list);
	hlist_del(&sch_task->hentry);
	kfree(sch_task);
}

static LIST_HEAD(bw_task_chk_list);
static DEFINE_HASHTABLE(sch_task_hash, FMU_SCH_HASH_BITS);
static struct fmu_sch_bw_info sch_bandwidth;

struct fmu_sch_bw_info *task_sch_init(u32 max_esit)
{
	if (max_esit < fmu_sch_max_esit)
		return ERR_PTR(-EINVAL);

	sch_bandwidth.bus_bw = kzalloc(max_esit * sizeof(*sch_bandwidth.bus_bw),
		GFP_KERNEL);
	if (sch_bandwidth.bus_bw == NULL)
		return ERR_PTR(-ENOMEM);

	fmu_sch_max_esit = max_esit;

	return &sch_bandwidth;
}

void task_sch_exit(struct fmu_sch_bw_info *sch_bw)
{
	kfree(sch_bw);
}

static int sch_check_bandwidth(void)
{
	struct fmu_sch_task_info *sch_task;
	int ret;

	pr_debug("Called %s()\n", __func__);

	list_for_each_entry(sch_task, &bw_task_chk_list, list) {
		ret = check_sch_bw(sch_task);
		if (ret) {
			pr_err("Not enough bandwidth! (%s)\n",
				sch_error_string(-ret));
			return -ENOSPC;
		}

		sch_task->task->interval += sch_task->offset;

		pr_info("elapsed time:%lld us, offset:%u\n",
			sch_task->elapsed_time_us, sch_task->offset);
	}

	return 0;
}

static void sch_reset_bandwidth(void)
{
	struct fmu_sch_task_info *sch_task, *tmp;

	pr_debug("Called %s()\n", __func__);

	list_for_each_entry_safe(sch_task, tmp, &bw_task_chk_list, list)
	destroy_sch_task(sch_task);
}

int fmu_sch_add_task(struct tim_task *task)
{
	struct fmu_sch_task_info *sch_task;
	int ret;

	pr_debug("%s %s\n", __func__, tim_task_decode(task));

	if (task->interval > fmu_sch_max_esit)
		return -ERANGE;

	sch_task = create_sch_task(&sch_bandwidth, task);
	if (IS_ERR_OR_NULL(sch_task))
		return -ENOMEM;

	setup_sch_info(task, sch_task);

	list_add_tail(&sch_task->list, &bw_task_chk_list);
	hash_add(sch_task_hash, &sch_task->hentry, (unsigned long)task);

	ret = sch_check_bandwidth();
	if (ret) {
		sch_reset_bandwidth();
		return ret;
	}

	return 0;
}

void fmu_sch_drop_task(struct tim_task *task)
{
	struct fmu_sch_task_info *sch_task;
	struct hlist_node *hn;

	pr_debug("%s %s\n", __func__, tim_task_decode(task));

	hash_for_each_possible_safe(sch_task_hash, sch_task,
		hn, hentry, (unsigned long)task) {
		if (sch_task->task == task) {
			destroy_sch_task(sch_task);
			break;
		}
	}
}

