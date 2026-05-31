
#include <linux/errno.h>
#include <linux/jiffies.h>
#include <linux/ktime.h>
#include <linux/printk.h>
#include <linux/sort.h>
#include <linux/string.h>

#include <kinetis/design_verification.h>
#include <kinetis/tim-task.h>

static LIST_HEAD(tim_task_head);
static LIST_HEAD(tim_task_suspend_head);

#define MAX_PRIORITY_LEVELS 3
static struct list_head priority_heads[MAX_PRIORITY_LEVELS];

static bool tim_task_init = false;
static atomic_t task_id_counter = ATOMIC_INIT(1);

const char *tim_task_decode(struct tim_task *task)
{
	static char buf[DBG_BUF_EN];
	snprintf(buf, DBG_BUF_EN, "task: %s, interval:%u, timeout:%lld, %s",
		task->name, task->interval_ms,
		task->timeout, task->auto_load ? "auto_load" : "");
	return buf;
}

int tim_task_add(struct tim_task *tim_task, const char *name, u32 interval_ms,
	bool auto_load, bool sched, tim_task_cb callback)
{
	u32 priority_level = 1;

	if (!tim_task || !callback)
		return -EINVAL;

	if (interval_ms == 0 || interval_ms > 3600000)
		return -EINVAL;

	if (tim_task_init == false) {
		for (int i = 0; i < MAX_PRIORITY_LEVELS; i++)
			INIT_LIST_HEAD(&priority_heads[i]);
		tim_task_init = true;
	}

	tim_task->callback = callback;
	tim_task->timeout = ktime_to_ms(ktime_get()) + interval_ms;
	tim_task->interval_ms = interval_ms;
	tim_task->auto_load = auto_load;
	tim_task->sched = sched;
	tim_task->name = name;
	tim_task->task_id = atomic_inc_return(&task_id_counter);
	tim_task->priority = priority_level;
	tim_task->created_time = ktime_get();
	tim_task->execution_count = 0;
	tim_task->last_execution_time = 0;
	tim_task->total_execution_time = 0;

	INIT_LIST_HEAD(&tim_task->list);
	INIT_LIST_HEAD(&tim_task->main_list);

	if (priority_level == 0)
		list_add_tail(&tim_task->list, &priority_heads[0]);
	else if (priority_level == 1)
		list_add_tail(&tim_task->list, &priority_heads[1]);
	else
		list_add_tail(&tim_task->list, &priority_heads[2]);

	list_add_tail(&tim_task->main_list, &tim_task_head);

	return 0;
}

void tim_task_drop(struct tim_task *tim_task)
{
	if (!tim_task)
		return;
	list_del(&tim_task->list);
	list_del(&tim_task->main_list);
}

void tim_task_suspend(struct tim_task *tim_task)
{
	list_move_tail(&tim_task->list, &tim_task_suspend_head);
}

void tim_task_resume(struct tim_task *tim_task)
{
	if (!tim_task || tim_task->priority >= MAX_PRIORITY_LEVELS)
		return;
	list_move_tail(&tim_task->list, &priority_heads[tim_task->priority]);
}

void tim_task_loop(void)
{
	struct tim_task *tim_task, *tmp;
	s64 current_time_ms;

	if (tim_task_init == false)
		return;

	current_time_ms = ktime_to_ms(ktime_get());

	for (int priority = 0; priority < MAX_PRIORITY_LEVELS; priority++) {
		list_for_each_entry_safe(tim_task, tmp, &priority_heads[priority], list) {
			if (tim_task->timeout <= current_time_ms) {
				tim_task->callback(tim_task);
				tim_task->execution_count++;

				if (tim_task->auto_load)
					tim_task->timeout = ktime_to_ms(ktime_get()) + tim_task->interval_ms;
				else
					tim_task_drop(tim_task);
			}
		}
	}
}

struct tim_task *tim_task_find_by_name(const char *name)
{
	struct tim_task *tim_task;

	if (!name)
		return NULL;

	list_for_each_entry(tim_task, &tim_task_head, main_list) {
		if (tim_task->name && strcmp(tim_task->name, name) == 0)
			return tim_task;
	}
	return NULL;
}

struct tim_task *tim_task_find_by_id(u32 task_id)
{
	struct tim_task *tim_task;

	list_for_each_entry(tim_task, &tim_task_head, main_list) {
		if (tim_task->task_id == task_id)
			return tim_task;
	}
	return NULL;
}

int tim_task_set_priority(struct tim_task *tim_task, u32 new_priority)
{
	if (!tim_task || new_priority >= MAX_PRIORITY_LEVELS)
		return -EINVAL;

	list_del(&tim_task->list);
	tim_task->priority = new_priority;
	list_add_tail(&tim_task->list, &priority_heads[new_priority]);
	return 0;
}

int tim_task_set_interval(struct tim_task *tim_task, u32 new_interval)
{
	if (!tim_task)
		return -EINVAL;

	if (new_interval == 0 || new_interval > 3600000)
		return -EINVAL;

	tim_task->interval_ms = new_interval;
	return 0;
}

void tim_task_cleanup_all(void)
{
	struct tim_task *tim_task, *tmp;

	for (int i = 0; i < MAX_PRIORITY_LEVELS; i++) {
		list_for_each_entry_safe(tim_task, tmp, &priority_heads[i], list) {
			list_del(&tim_task->list);
			list_del(&tim_task->main_list);
		}
	}
	list_for_each_entry_safe(tim_task, tmp, &tim_task_head, main_list) {
		list_del(&tim_task->list);
		list_del(&tim_task->main_list);
	}
}

struct tim_task *tim_task_get_next_task(void)
{
	struct tim_task *next_task = NULL;
	struct tim_task *tim_task;
	s64 current_time_ms = ktime_to_ms(ktime_get());
	s64 earliest_timeout = S64_MAX;

	for (int priority = 0; priority < MAX_PRIORITY_LEVELS; priority++) {
		list_for_each_entry(tim_task, &priority_heads[priority], list) {
			if (tim_task->timeout < earliest_timeout && tim_task->timeout > current_time_ms) {
				earliest_timeout = tim_task->timeout;
				next_task = tim_task;
			}
		}
	}
	return next_task;
}

#ifdef DESIGN_VERIFICATION_TIMTASK
#include "kinetis/test-kinetis.h"

static s64 time_stamp;
static struct tim_task example_task;
static int tim_test_callback_count = 0;

static void tim_task_callback(struct tim_task *task)
{
	s64 delta;
	tim_test_callback_count++;
	delta = ktime_ms_delta(ktime_get(), time_stamp);
	pr_info("timer task '%s' callback #%d executed after %lld ms.\n",
		task ? task->name : "unknown", tim_test_callback_count, delta);
	if (delta >= 900 && delta <= 1100)
		pr_info("timer task timing correct\n");
	else
		pr_info("timer task timing incorrect (expected: 900-1100ms, actual: %lldms)\n", delta);
	time_stamp = ktime_get();
}

static void tim_test_validation_callback(struct tim_task *task)
{
	tim_test_callback_count++;
	pr_info("timer validation task '%s' executed - count: %d\n",
		task ? task->name : "unknown", tim_test_callback_count);
}

static void tim_test_priority_callback(struct tim_task *task)
{
	pr_info("priority task '%s' (id:%u, priority:%u) executed\n",
		task ? task->name : "unknown", task ? task->task_id : 0, task ? task->priority : 0);
}

static void tim_test_cleanup_callback(struct tim_task *task)
{
	pr_info("timer cleanup task executed\n");
}

static void tim_test_suspend_callback(struct tim_task *task)
{
	pr_info("suspend task '%s' executed\n", task ? task->name : "unknown");
}

int t_tim_task_add(int argc, char **argv)
{
	u32 interval = 1000;
	bool auto_load = true;
	bool sched = false;
	char task_name[32] = {0};

	if (argc > 1) interval = simple_strtoul(argv[1], NULL, 10);
	if (argc > 2) auto_load = simple_strtoul(argv[2], NULL, 10) ? true : false;
	if (argc > 3) sched = simple_strtoul(argv[3], NULL, 10) ? true : false;
	if (argc > 4) {
		strncpy(task_name, argv[4], sizeof(task_name) - 1);
		task_name[sizeof(task_name) - 1] = '\0';
	}

	int ret = tim_task_add(&example_task, task_name, interval, auto_load, sched, tim_task_callback);
	if (ret) {
		pr_err("timer task creation failed: %d\n", ret);
		return ret;
	}
	pr_info("timer task created: %s (id: %u, interval: %ums, auto_load: %d, sched: %d)\n",
		example_task.name, example_task.task_id, interval, auto_load, sched);
	return 0;
}

int t_tim_task_drop(int argc, char **argv)
{
	if (example_task.task_id) {
		pr_info("dropping task: %s (id: %u)\n", example_task.name, example_task.task_id);
		tim_task_drop(&example_task);
		memset(&example_task, 0, sizeof(example_task));
	} else {
		pr_err("no task to drop\n");
		return -EINVAL;
	}
	return 0;
}

int t_tim_task_validation(int argc, char **argv)
{
	struct tim_task test_task, *found_task;
	int ret;

	ret = tim_task_add(&test_task, "validation-task", 500, true, false, tim_test_validation_callback);
	if (ret) {
		pr_err("validation task creation failed: %d\n", ret);
		return ret;
	}

	found_task = tim_task_find_by_name("validation-task");
	if (!found_task) {
		pr_err("task lookup by name failed\n");
		tim_task_drop(&test_task);
		return -ENOMEM;
	}

	found_task = tim_task_find_by_id(test_task.task_id);
	if (!found_task) {
		pr_err("task lookup by id failed\n");
		tim_task_drop(&test_task);
		return -ENOMEM;
	}

	ret = tim_task_set_priority(&test_task, 0);
	if (ret) {
		pr_err("priority setting failed: %d\n", ret);
		tim_task_drop(&test_task);
		return ret;
	}

	tim_task_drop(&test_task);
	pr_info("all timer validation tests completed\n");
	return 0;
}

int t_tim_task_priority(int argc, char **argv)
{
	struct tim_task priority_tasks[3];
	const char *task_names[] = {"high-priority", "normal-priority", "low-priority"};
	u32 priorities[] = {0, 1, 2};
	int ret;

	for (int i = 0; i < 3; i++) {
		ret = tim_task_add(&priority_tasks[i], task_names[i], 200, true, false, tim_test_priority_callback);
		if (ret) {
			pr_err("priority task %d creation failed: %d\n", i, ret);
			return ret;
		}
		tim_task_set_priority(&priority_tasks[i], priorities[i]);
	}

	ktime_t test_end = ktime_add_ms(ktime_get(), 2000);
	while (ktime_compare(ktime_get(), test_end) < 0)
		tim_task_loop();

	for (int i = 0; i < 3; i++)
		tim_task_drop(&priority_tasks[i]);
	return 0;
}

int t_tim_task_performance(int argc, char **argv)
{
	struct tim_task perf_tasks[20];
	char task_name[32];
	int ret;

	for (int i = 0; i < 20; i++) {
		snprintf(task_name, sizeof(task_name), "perf-task-%d", i);
		ret = tim_task_add(&perf_tasks[i], task_name, 50 + (i * 10), true, false, tim_test_validation_callback);
		if (ret) return ret;
		tim_task_set_priority(&perf_tasks[i], i % 3);
	}

	ktime_t test_end = ktime_add_ms(ktime_get(), 3000);
	while (ktime_compare(ktime_get(), test_end) < 0)
		tim_task_loop();

	for (int i = 0; i < 20; i++)
		tim_task_drop(&perf_tasks[i]);
	return 0;
}

int t_tim_task_cleanup(int argc, char **argv)
{
	struct tim_task cleanup_task;
	tim_test_callback_count = 0;

	int ret = tim_task_add(&cleanup_task, "cleanup-test-task", 100, false, false, tim_test_cleanup_callback);
	if (ret) return ret;

	tim_task_loop();
	tim_task_cleanup_all();
	return 0;
}

int t_tim_task_boundary(int argc, char **argv)
{
	struct tim_task boundary_tasks[5];
	struct tim_task invalid_task;
	int ret;

	ret = tim_task_add(&boundary_tasks[0], "min-interval", 1, false, false, tim_test_validation_callback);
	if (ret) return ret;

	ret = tim_task_add(&boundary_tasks[1], "short-interval", 10, false, false, tim_test_validation_callback);
	if (ret) return ret;

	ret = tim_task_add(&boundary_tasks[2], "medium-interval", 100, false, false, tim_test_validation_callback);
	if (ret) return ret;

	ret = tim_task_add(&boundary_tasks[3], "long-interval", 1000, false, false, tim_test_validation_callback);
	if (ret) return ret;

	ret = tim_task_add(&boundary_tasks[4], "max-interval", 3600000, false, false, tim_test_validation_callback);
	if (ret) return ret;

	/* Test invalid intervals should return error */
	ret = tim_task_add(&invalid_task, "invalid-zero", 0, false, false, tim_test_validation_callback);
	if (ret == 0) {
		pr_err("zero interval should be rejected\n");
		goto cleanup;
	}

	ret = tim_task_add(&invalid_task, "invalid-too-large", 3600001, false, false, tim_test_validation_callback);
	if (ret == 0) {
		pr_err("too large interval should be rejected\n");
		goto cleanup;
	}

	pr_info("all boundary tests passed\n");

cleanup:
	for (int i = 0; i < 5; i++)
		tim_task_drop(&boundary_tasks[i]);
	return 0;
}

int t_tim_task_concurrent(int argc, char **argv)
{
	struct tim_task concurrent_tasks[10];
	char task_name[32];
	int ret;

	for (int i = 0; i < 10; i++) {
		snprintf(task_name, sizeof(task_name), "concurrent-%d", i);
		ret = tim_task_add(&concurrent_tasks[i], task_name, 100 + i * 10, true, false, tim_test_validation_callback);
		if (ret) return ret;
	}

	ktime_t test_end = ktime_add_ms(ktime_get(), 3000);
	while (ktime_compare(ktime_get(), test_end) < 0)
		tim_task_loop();

	for (int i = 0; i < 10; i++)
		tim_task_drop(&concurrent_tasks[i]);
	return 0;
}

int t_tim_task_short_interval(int argc, char **argv)
{
	struct tim_task short_tasks[20];
	char task_name[32];
	int ret;

	for (int i = 0; i < 20; i++) {
		snprintf(task_name, sizeof(task_name), "short-interval-%d", i);
		ret = tim_task_add(&short_tasks[i], task_name, 1 + i, true, false, tim_test_validation_callback);
		if (ret) return ret;
	}

	ktime_t test_end = ktime_add_ms(ktime_get(), 1000);
	while (ktime_compare(ktime_get(), test_end) < 0)
		tim_task_loop();

	for (int i = 0; i < 20; i++)
		tim_task_drop(&short_tasks[i]);
	return 0;
}

int t_tim_task_suspend_resume(int argc, char **argv)
{
	struct tim_task suspend_task;
	int ret;

	ret = tim_task_add(&suspend_task, "suspend-test-task", 500, true, false, tim_test_suspend_callback);
	if (ret) return ret;

	tim_task_suspend(&suspend_task);
	tim_task_resume(&suspend_task);

	ktime_t test_end = ktime_add_ms(ktime_get(), 1000);
	while (ktime_compare(ktime_get(), test_end) < 0)
		tim_task_loop();

	tim_task_drop(&suspend_task);
	return 0;
}

#endif
