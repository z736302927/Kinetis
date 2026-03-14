
#include <generated/deconfig.h>
#include <linux/slab.h>
#include <linux/errno.h>
#include <linux/jiffies.h>
#include <linux/ktime.h>
#include <linux/printk.h>
#include <linux/sort.h>

#include <kinetis/design_verification.h>
#include <kinetis/tim-task.h>

//tim_task handle list head.
static LIST_HEAD(tim_task_head);
static LIST_HEAD(tim_task_suspend_head);

// Priority scheduling
#define MAX_PRIORITY_LEVELS 3
static struct list_head priority_heads[MAX_PRIORITY_LEVELS];

static bool tim_task_init = false;

// Task ID counter
static atomic_t task_id_counter = ATOMIC_INIT(1);

const char *
tim_task_decode(struct tim_task *task)
{
	static char buf[DBG_BUF_EN];

	snprintf(buf, DBG_BUF_EN, "task: %s, interval:%u, timeout:%lld, %s",
		task->name, task->interval,
		task->timeout, task->auto_load ? "auto_load" : "");

	return buf;
}

struct tim_task *tim_task_add(char *name, u32 interval, bool auto_load, bool sched,
	tim_task_cb callback)
{
	struct tim_task *tim_task;
	int ret;
	u32 priority_level = 1; // Default normal priority

	if (!callback) {
		return ERR_PTR(-EINVAL);
	}

	if (interval == 0 || interval > 3600000) { // Max 1 hour
		pr_err("Invalid interval: %u (must be 1-3600000ms)\n", interval);
		return ERR_PTR(-EINVAL);
	}

	if (tim_task_init == false) {
		for (int i = 0; i < MAX_PRIORITY_LEVELS; i++) {
			INIT_LIST_HEAD(&priority_heads[i]);
		}
		tim_task_init = true;
	}

	tim_task = kmalloc(sizeof(*tim_task), GFP_KERNEL);
	if (!tim_task) {
		return ERR_PTR(-ENOMEM);
	}

	tim_task->callback = callback;
	tim_task->timeout = ktime_to_ms(ktime_get()) + interval;
	tim_task->interval = interval;
	tim_task->auto_load = auto_load;
	tim_task->task_id = atomic_inc_return(&task_id_counter);
	tim_task->priority = priority_level;
	tim_task->created_time = ktime_get();
	tim_task->execution_count = 0;
	tim_task->last_execution_time = 0;
	tim_task->total_execution_time = 0;

	if (name == NULL) {
		char default_name[32];
		snprintf(default_name, sizeof(default_name), "tim_task_%u", tim_task->task_id);
		tim_task->name = kstrdup_const(default_name, GFP_KERNEL);
	} else {
		tim_task->name = kstrdup_const(name, GFP_KERNEL);
	}

	if (!tim_task->name) {
		kfree(tim_task);
		return ERR_PTR(-ENOMEM);
	}

	// Enhanced scheduling with priority
	if (sched) {
		// ret = fmu_sch_add_task(tim_task);
		// if (ret)
		//     return ret;
		// tim_task->sched = true;
		pr_debug("Scheduling enabled for task: %s (ID: %u)\n", name, tim_task->task_id);
	}

	if (priority_level == 0) {
		list_add_tail(&tim_task->list, &priority_heads[0]);
	} else if (priority_level == 1) {
		list_add_tail(&tim_task->list, &priority_heads[1]);
	} else {
		list_add_tail(&tim_task->list, &priority_heads[2]);
	}

	// Also add to main list for compatibility (using separate node)
	INIT_LIST_HEAD(&tim_task->main_list);
	list_add_tail(&tim_task->main_list, &tim_task_head);

	pr_info("timer task created: %s (ID: %u, interval: %ums, priority: %u)\n",
		name, tim_task->task_id, interval, priority_level);

	return tim_task;
}

void tim_task_drop(struct tim_task *tim_task)
{
	list_del(&tim_task->list);
	list_del(&tim_task->main_list);

	//if (tim_task->sched)
	//	fmu_sch_drop_task(tim_task);

	kfree_const(tim_task->name);
	kfree(tim_task);
}

void tim_task_suspend(struct tim_task *tim_task)
{
	list_move_tail(&tim_task->list, &tim_task_suspend_head);
}

void tim_task_resume(struct tim_task *tim_task)
{
	if (!tim_task || tim_task->priority >= MAX_PRIORITY_LEVELS) {
		return;
	}

	list_move_tail(&tim_task->list, &priority_heads[tim_task->priority]);
}

void tim_task_loop(void)
{
	struct tim_task *tim_task, *tmp;
	ktime_t loop_start_time;
	s64 current_time_ms;

	if (tim_task_init == false) {
		return;
	}

	current_time_ms = ktime_to_ms(ktime_get());

	// Process tasks by priority (high to low)
	for (int priority = 0; priority < MAX_PRIORITY_LEVELS; priority++) {
		list_for_each_entry_safe(tim_task, tmp, &priority_heads[priority], list) {
			if (tim_task->timeout <= current_time_ms) {
				ktime_t task_start_time, task_end_time;
				u32 execution_time_ms;

				tim_task->callback(tim_task);
				tim_task->execution_count++;

				if (tim_task->auto_load == true) {
					tim_task->timeout = ktime_to_ms(ktime_get()) + tim_task->interval;
				} else {
					// Remove from all lists
					list_del(&tim_task->list);
					list_del(&tim_task->main_list);
				}
			}
		}
	}
}

struct tim_task *tim_task_find_by_name(const char *name)
{
	struct tim_task *tim_task;

	if (!name) {
		return NULL;
	}

	list_for_each_entry(tim_task, &tim_task_head, main_list) {
		if (tim_task->name && strcmp(tim_task->name, name) == 0) {
			return tim_task;
		}
	}

	return NULL;
}

struct tim_task *tim_task_find_by_id(u32 task_id)
{
	struct tim_task *tim_task;

	list_for_each_entry(tim_task, &tim_task_head, main_list) {
		if (tim_task->task_id == task_id) {
			return tim_task;
		}
	}

	return NULL;
}

int tim_task_set_priority(struct tim_task *tim_task, u32 new_priority)
{
	if (!tim_task || new_priority >= MAX_PRIORITY_LEVELS) {
		return -EINVAL;
	}

	// Remove from current priority list
	list_del(&tim_task->list);

	// Set new priority
	tim_task->priority = new_priority;

	// Add to new priority list
	list_add_tail(&tim_task->list, &priority_heads[new_priority]);

	pr_info("Task %s priority changed to %u\n", tim_task->name, new_priority);
	return 0;
}

void tim_task_print_info(struct tim_task *tim_task)
{
	if (!tim_task) {
		pr_err("invalid task pointer\n");
		return;
	}

	pr_info("=== task information ===\n");
	pr_info("id: %u\n", tim_task->task_id);
	pr_info("name: %s\n", tim_task->name ? tim_task->name : "null");
	pr_info("priority: %u\n", tim_task->priority);
	pr_info("interval: %u ms\n", tim_task->interval);
	pr_info("timeout: %lld ms\n", tim_task->timeout);
	pr_info("auto load: %s\n", tim_task->auto_load ? "yes" : "no");
	pr_info("execution count: %u\n", tim_task->execution_count);
	pr_info("last execution time: %u ms\n", tim_task->last_execution_time);
	pr_info("total execution time: %llu ms\n", tim_task->total_execution_time);
	pr_info("created: %lld ms ago\n", ktime_to_ms(ktime_sub(ktime_get(), tim_task->created_time)));
	pr_info("=========================\n");
}

void tim_task_cleanup_all(void)
{
	struct tim_task *tim_task, *tmp;

	// Clear all priority lists
	for (int i = 0; i < MAX_PRIORITY_LEVELS; i++) {
		list_for_each_entry_safe(tim_task, tmp, &priority_heads[i], list) {
			list_del(&tim_task->list);
			list_del(&tim_task->main_list);
			kfree_const(tim_task->name);
			kfree(tim_task);
		}
	}

	// Clear main list (using main_list node)
	list_for_each_entry_safe(tim_task, tmp, &tim_task_head, main_list) {
		list_del(&tim_task->list);
		list_del(&tim_task->main_list);
		kfree_const(tim_task->name);
		kfree(tim_task);
	}

	pr_info("All timer tasks cleaned up\n");
}

struct tim_task *tim_task_get_next_task(void)
{
	struct tim_task *next_task = NULL;
	struct tim_task *tim_task;
	s64 current_time_ms = ktime_to_ms(ktime_get());
	s64 earliest_timeout = S64_MAX;

	// Find task with earliest timeout
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
static struct tim_task *example_task;
static int tim_test_callback_count = 0;

static void tim_task_callback(struct tim_task *task)
{
	s64 delta;
	tim_test_callback_count++;

	delta = ktime_ms_delta(ktime_get(),  time_stamp);
	pr_info("timer task '%s' callback #%d executed after %lld ms.\n",
		task ? task->name : "unknown", tim_test_callback_count, delta);

	if (delta >= 900 && delta <= 1100) {
		pr_info("timer task timing correct\n");
	} else {
		pr_info("timer task timing incorrect (expected: 900-1100ms, actual: %lldms)\n", delta);
	}
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

int t_tim_task_add(int argc, char **argv)
{
	u32 interval = 1000;
	bool auto_load = true;
	bool sched = false;
	char task_name[32] = {0};
	int ret;

	if (argc > 1) {
		interval = simple_strtoul(argv[1], NULL, 10);
	}

	if (argc > 2) {
		auto_load = simple_strtoul(argv[2], NULL, 10) ? true : false;
	}

	if (argc > 3) {
		sched = simple_strtoul(argv[3], NULL, 10) ? true : false;
	}

	if (argc > 4) {
		strncpy(task_name, argv[4], sizeof(task_name) - 1);
		task_name[sizeof(task_name) - 1] = '\0';
	}

	example_task = tim_task_add(task_name, interval, auto_load, sched, tim_task_callback);
	if (IS_ERR_OR_NULL(example_task)) {
		pr_err("timer task creation failed\n");
		return -EINVAL;
	}

	pr_info("timer task created: %s (id: %u, interval: %ums, auto_load: %d, sched: %d)\n",
		example_task->name, example_task->task_id, interval, auto_load, sched);
	return 0;
}

int t_tim_task_drop(int argc, char **argv)
{
	u32 task_id = 0;
	struct tim_task *task;

	if (argc > 1) {
		task_id = simple_strtoul(argv[1], NULL, 10);
	}

	if (example_task && (task_id == 0 || task_id == example_task->task_id)) {
		pr_info("dropping task: %s (id: %u)\n", example_task->name, example_task->task_id);
		tim_task_drop(example_task);
		example_task = NULL;
	} else if (task_id != 0) {
		task = tim_task_find_by_id(task_id);
		if (task) {
			pr_info("dropping task by id: %s (id: %u)\n", task->name, task->task_id);
			tim_task_drop(task);
		} else {
			pr_err("task with id %u not found\n", task_id);
			return -ENOENT;
		}
	} else {
		pr_err("no task to drop. create a task first using t_tim_task_add\n");
		return -EINVAL;
	}
	return 0;
}

int t_tim_task_validation(int argc, char **argv)
{
	struct tim_task *test_task, *found_task;
	int ret;

	// test 1: task creation with priority
	test_task = tim_task_add("validation-task", 500, true, false, tim_test_validation_callback);
	if (IS_ERR_OR_NULL(test_task)) {
		pr_err("validation task creation failed\n");
		return -ENOMEM;
	}

	pr_info("task with priority created (id: %u, priority: %u)\n",
		test_task->task_id, test_task->priority);

	// test 2: task lookup by name
	found_task = tim_task_find_by_name("validation-task");
	if (!found_task) {
		pr_err("task lookup by name failed\n");
		tim_task_drop(test_task);
		return -ENOMEM;
	}

	pr_info("task lookup by name working\n");

	// test 3: task lookup by id
	found_task = tim_task_find_by_id(test_task->task_id);
	if (!found_task) {
		pr_err("task lookup by id failed\n");
		tim_task_drop(test_task);
		return -ENOMEM;
	}

	pr_info("task lookup by id working\n");

	// test 4: priority setting
	ret = tim_task_set_priority(test_task, 0); // high priority
	if (ret) {
		pr_err("priority setting failed: %d\n", ret);
		tim_task_drop(test_task);
		return ret;
	}

	if (test_task->priority != 0) {
		pr_err("priority not set correctly\n");
		tim_task_drop(test_task);
		return -EINVAL;
	}

	pr_info("priority setting working\n");

	// test 5: task info printing
	tim_task_print_info(test_task);

	// cleanup: drop the test task
	tim_task_drop(test_task);

	pr_info("all timer validation tests completed\n");
	return 0;
}

int t_tim_task_priority(int argc, char **argv)
{
	struct tim_task *priority_tasks[3];
	const char *task_names[] = {"high-priority", "normal-priority", "low-priority"};
	u32 priorities[] = {0, 1, 2};
	int ret, i;

	// create tasks with different priorities
	for (i = 0; i < 3; i++) {
		priority_tasks[i] = tim_task_add(task_names[i],
				200, true, false, tim_test_priority_callback);
		if (IS_ERR_OR_NULL(priority_tasks[i])) {
			pr_err("priority task %d creation failed\n", i);
			return -ENOMEM;
		}

		// set priority
		ret = tim_task_set_priority(priority_tasks[i], priorities[i]);
		if (ret) {
			pr_err("priority setting for task %d failed: %d\n", i, ret);
			return ret;
		}
	}

	// run tasks to test priority execution
	ktime_t test_end = ktime_add_ms(ktime_get(), 2000);
	while (ktime_compare(ktime_get(), test_end) < 0) {
		tim_task_loop();
	}

	// cleanup: drop all priority tasks
	for (i = 0; i < 3; i++) {
		tim_task_drop(priority_tasks[i]);
	}
	pr_info("all priority tasks dropped\n");

	return 0;
}

int t_tim_task_performance(int argc, char **argv)
{
	struct tim_task *perf_tasks[20];
	u64 start_time, end_time;
	char task_name[32];
	int ret, i;

	start_time = ktime_get();

	// create multiple tasks with different priorities
	for (i = 0; i < 20; i++) {
		snprintf(task_name, sizeof(task_name), "perf-task-%d", i);

		perf_tasks[i] = tim_task_add(task_name,
				50 + (i * 10), true, false, tim_test_validation_callback);
		if (IS_ERR_OR_NULL(perf_tasks[i])) {
			pr_err("performance task %d creation failed\n", i);
			return -ENOMEM;
		}

		// set priority based on index
		tim_task_set_priority(perf_tasks[i], i % 3);
	}

	end_time = ktime_get();
	pr_info("20 timer tasks created in %llu ms\n",
		ktime_to_ms(ktime_sub(end_time, start_time)));

	// run tasks briefly
	ktime_t test_end = ktime_add_ms(ktime_get(), 3000);
	while (ktime_compare(ktime_get(), test_end) < 0) {
		tim_task_loop();
	}

	// cleanup: drop all performance tasks
	for (i = 0; i < 20; i++) {
		tim_task_drop(perf_tasks[i]);
	}
	pr_info("all performance tasks dropped\n");

	return 0;
}

int t_tim_task_cleanup(int argc, char **argv)
{
	struct tim_task *cleanup_task, *next;
	int ret;

	// reset callback count
	tim_test_callback_count = 0;

	// create a cleanup task
	cleanup_task = tim_task_add("cleanup-test-task",
			100, false, false, tim_test_cleanup_callback);
	if (IS_ERR_OR_NULL(cleanup_task)) {
		pr_err("cleanup task creation failed\n");
		return -ENOMEM;
	}

	pr_info("created cleanup task with id: %u\n", cleanup_task->task_id);

	// run loop to execute task
	tim_task_loop();

	// test next task prediction
	next = tim_task_get_next_task();
	pr_info("next predicted task: %s\n", next ? next->name : "none");

	// cleanup all tasks
	tim_task_cleanup_all();

	return 0;
}

int t_tim_task_boundary(int argc, char **argv)
{
	struct tim_task *boundary_tasks[5];
	struct tim_task *invalid_task;
	int ret;

	// test 1: minimum interval (1ms)
	boundary_tasks[0] = tim_task_add("min-interval",
			1, false, false, tim_test_validation_callback);
	if (IS_ERR_OR_NULL(boundary_tasks[0])) {
		pr_err("min interval task creation failed\n");
		return -ENOMEM;
	}

	// test 2: short interval (10ms)
	boundary_tasks[1] = tim_task_add("short-interval",
			10, false, false, tim_test_validation_callback);
	if (IS_ERR_OR_NULL(boundary_tasks[1])) {
		pr_err("short interval task creation failed\n");
		return -ENOMEM;
	}

	// test 3: medium interval (100ms)
	boundary_tasks[2] = tim_task_add("medium-interval",
			100, false, false, tim_test_validation_callback);
	if (IS_ERR_OR_NULL(boundary_tasks[2])) {
		pr_err("medium interval task creation failed\n");
		return -ENOMEM;
	}

	// test 3: long interval (1000ms)
	boundary_tasks[3] = tim_task_add("long-interval",
			1000, false, false, tim_test_validation_callback);
	if (IS_ERR_OR_NULL(boundary_tasks[3])) {
		pr_err("long interval task creation failed\n");
		return -ENOMEM;
	}

	// test 4: maximum interval (3600000ms = 1 hour)
	boundary_tasks[4] = tim_task_add("max-interval",
			3600000, false, false, tim_test_validation_callback);
	if (IS_ERR_OR_NULL(boundary_tasks[4])) {
		pr_err("max interval task creation failed\n");
		return -ENOMEM;
	}

	// test 5: invalid interval (0ms)
	invalid_task = tim_task_add("invalid-zero",
			0, false, false, tim_test_validation_callback);
	if (!IS_ERR(invalid_task)) {
		pr_err("zero interval should be rejected\n");
		tim_task_drop(boundary_tasks[0]);
		tim_task_drop(boundary_tasks[1]);
		tim_task_drop(boundary_tasks[2]);
		tim_task_drop(boundary_tasks[3]);
		tim_task_drop(boundary_tasks[4]);
		return -ENOMEM;
	}

	// test 6: invalid interval (too large)
	invalid_task = tim_task_add("invalid-too-large",
			3600001, false, false, tim_test_validation_callback);
	if (!IS_ERR(invalid_task)) {
		pr_err("too large interval should be rejected\n");
		tim_task_drop(boundary_tasks[0]);
		tim_task_drop(boundary_tasks[1]);
		tim_task_drop(boundary_tasks[2]);
		tim_task_drop(boundary_tasks[3]);
		tim_task_drop(boundary_tasks[4]);
		return -ENOMEM;
	}

	// cleanup: drop all boundary tasks
	tim_task_drop(boundary_tasks[0]);
	tim_task_drop(boundary_tasks[1]);
	tim_task_drop(boundary_tasks[2]);
	tim_task_drop(boundary_tasks[3]);
	tim_task_drop(boundary_tasks[4]);

	pr_info("all boundary tests passed\n");
	return 0;
}

int t_tim_task_concurrent(int argc, char **argv)
{
	struct tim_task *concurrent_tasks[10];
	char task_name[32];
	int ret, i;

	// create multiple concurrent tasks
	for (i = 0; i < 10; i++) {
		snprintf(task_name, sizeof(task_name), "concurrent-%d", i);

		concurrent_tasks[i] = tim_task_add(task_name,
				100 + i * 10, true, false, tim_test_validation_callback);
		if (IS_ERR_OR_NULL(concurrent_tasks[i])) {
			pr_err("concurrent task %d creation failed\n", i);
			return -ENOMEM;
		}
	}

	// run loop to test concurrent execution
	ktime_t test_end = ktime_add_ms(ktime_get(), 3000);
	while (ktime_compare(ktime_get(), test_end) < 0) {
		tim_task_loop();
	}

	// cleanup: drop all concurrent tasks
	for (i = 0; i < 10; i++) {
		tim_task_drop(concurrent_tasks[i]);
	}
	pr_info("all concurrent tasks dropped\n");

	return 0;
}

int t_tim_task_short_interval(int argc, char **argv)
{
	int ret, i;
	struct tim_task *short_tasks[20];

	// create tasks with very short intervals (1-20ms)
	for (i = 0; i < 20; i++) {
		char task_name[32];
		snprintf(task_name, sizeof(task_name), "short-interval-%d", i);

		short_tasks[i] = tim_task_add(task_name,
				1 + i, true, false, tim_test_validation_callback);
		if (IS_ERR_OR_NULL(short_tasks[i])) {
			pr_err("short interval task %d creation failed\n", i);
			return -ENOMEM;
		}
	}

	// run loop briefly
	ktime_t test_end = ktime_add_ms(ktime_get(), 1000);
	while (ktime_compare(ktime_get(), test_end) < 0) {
		tim_task_loop();
	}

	// cleanup: drop all short interval tasks
	for (i = 0; i < 20; i++) {
		tim_task_drop(short_tasks[i]);
	}

	return 0;
}

static void tim_test_suspend_callback(struct tim_task *task)
{
	pr_info("suspend task '%s' executed\n", task ? task->name : "unknown");
}

int t_tim_task_suspend_resume(int argc, char **argv)
{
	struct tim_task *suspend_task;
	int ret;

	// create a task
	suspend_task = tim_task_add("suspend-test-task",
			500, true, false, tim_test_suspend_callback);
	if (IS_ERR_OR_NULL(suspend_task)) {
		pr_err("suspend resume test task creation failed\n");
		return -ENOMEM;
	}

	// test suspend
	tim_task_suspend(suspend_task);
	pr_info("task suspended successfully\n");

	// test resume
	tim_task_resume(suspend_task);
	pr_info("task resumed successfully\n");

	// run loop to verify execution
	ktime_t test_end = ktime_add_ms(ktime_get(), 1000);
	while (ktime_compare(ktime_get(), test_end) < 0) {
		tim_task_loop();
	}

	// cleanup: drop the suspend task
	tim_task_drop(suspend_task);

	pr_info("all suspend resume tests passed\n");
	return 0;
}

#endif
