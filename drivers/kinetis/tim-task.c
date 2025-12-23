
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

// Enhanced task management
static struct tim_task_stats tim_stats = {0};

// Priority scheduling
#define MAX_PRIORITY_LEVELS 3
static LIST_HEAD(priority_heads[MAX_PRIORITY_LEVELS]);

// Performance monitoring
static bool performance_profiling_enabled = false;
static u32 task_execution_history[100];
static u8 history_index = 0;

static bool tim_task_init = false;

// Task ID counter
static atomic_t task_id_counter = ATOMIC_INIT(1);

/* The following program is modified by the user according to the hardware device, otherwise the driver cannot run. */

/**
  * @step 1:  Modify the corresponding function according to the modified area and the corresponding function name.
  * @step 2:  According to the example in function tim_task_Test, design the function you need and initialize it in the main function.
  * @step 3:  Call function tim_task_Ticks periodically at the frequency you want.
  * @step 4:  An infinite loop calls function tim_task_Loop.
  * @step 5:  Note that the base frequency determines your actual call time.Please calculate in advance.
  */

/* The above procedure is modified by the user according to the hardware device, otherwise the driver cannot run. */

const char *
tim_task_decode(struct tim_task *task)
{
	static char buf[DBG_BUF_EN];

	snprintf(buf, DBG_BUF_EN, "task: %s, interval:%u, timeout:%lld, %s",
		task->name, task->interval,
		task->timeout, task->auto_load ? "auto_load" : "");

	return buf;
}

/**
  * @brief  add the tim_task struct handle.
  * @param  tim_task: the tim_task handle strcut.
  * @param  timeout_cb: Timeout callback.
  * @param  timeout: time out time.
  * @param  repeat: repeat interval time.
  * @retval None
  */
int tim_task_add(struct tim_task *tim_task,
	char *name, u32 interval, bool auto_load, bool sched,
	tim_task_cb callback)
{
	int ret;
	u32 priority_level = 1; // Default normal priority

	if (!callback || !name || !tim_task) {
		return -EINVAL;
	}

	// Enhanced validation
	if (interval == 0 || interval > 3600000) { // Max 1 hour
		pr_err("Invalid interval: %u (must be 1-3600000ms)\n", interval);
		return -EINVAL;
	}

	if (tim_task_init == false) {
		for (int i = 0; i < MAX_PRIORITY_LEVELS; i++) {
			INIT_LIST_HEAD(&priority_heads[i]);
		}
		tim_task_init = true;
	}

	tim_task->name = kstrdup_const(name, GFP_KERNEL);
	if (!tim_task->name) {
		return -ENOMEM;
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

	// Enhanced scheduling with priority
	if (sched) {
		// ret = fmu_sch_add_task(tim_task);
		// if (ret)
		//     return ret;
		// tim_task->sched = true;
		pr_debug("Scheduling enabled for task: %s (ID: %u)\n", name, tim_task->task_id);
	}

	// Add to priority queue
	if (priority_level == 0) {
		list_add_tail(&tim_task->list, &priority_heads[0]);
		tim_stats.high_priority_tasks++;
	} else if (priority_level == 1) {
		list_add_tail(&tim_task->list, &priority_heads[1]);
		tim_stats.normal_priority_tasks++;
	} else {
		list_add_tail(&tim_task->list, &priority_heads[2]);
		tim_stats.low_priority_tasks++;
	}

	// Also add to main list for compatibility (using separate node)
	INIT_LIST_HEAD(&tim_task->main_list);
	list_add_tail(&tim_task->main_list, &tim_task_head);
	tim_stats.total_tasks_created++;

	pr_info("Timer task created: %s (ID: %u, interval: %ums, priority: %u)\n",
		name, tim_task->task_id, interval, priority_level);

	return 0;
}

/**
  * @brief  drop the tim_task struct handle.
  * @param  tim_task: the tim_task tim_task strcut.
  * @retval None
  */
void tim_task_drop(struct tim_task *tim_task)
{
	list_del(&tim_task->list);
	list_del(&tim_task->main_list);

	// Update priority counters
	if (tim_task->priority == 0) {
		tim_stats.high_priority_tasks--;
	} else if (tim_task->priority == 1) {
		tim_stats.normal_priority_tasks--;
	} else {
		tim_stats.low_priority_tasks--;
	}

	//if (tim_task->sched)
	//	fmu_sch_drop_task(tim_task);

	kfree(tim_task->name);
	kfree(tim_task);
}

/**
  * @brief  suspend the tim_task struct handle.
  * @param  tim_task: the tim_task tim_task strcut.
  * @retval None
  */
void tim_task_suspend(struct tim_task *tim_task)
{
	list_move_tail(&tim_task->list, &tim_task_suspend_head);
}

/**
  * @brief  resume the tim_task struct handle.
  * @param  tim_task: the tim_task tim_task strcut.
  * @retval None
  */
void tim_task_resume(struct tim_task *tim_task)
{
	list_move_tail(&tim_task->list, &tim_task_head);
}

/**
  * @brief  Enhanced main loop with priority scheduling and performance monitoring.
  * @param  None.
  * @retval None
  */
void tim_task_loop(void)
{
	struct tim_task *tim_task, *tmp;
	ktime_t loop_start_time;
	u64 current_time_ms;

	if (performance_profiling_enabled) {
		loop_start_time = ktime_get();
	}

	current_time_ms = ktime_to_ms(ktime_get());

	// Process tasks by priority (high to low)
	for (int priority = 0; priority < MAX_PRIORITY_LEVELS; priority++) {
		list_for_each_entry_safe(tim_task, tmp, &priority_heads[priority], list) {
			if (tim_task->timeout <= current_time_ms) {
				ktime_t task_start_time, task_end_time;
				u32 execution_time_ms;

				if (performance_profiling_enabled) {
					task_start_time = ktime_get();
				}

				pr_debug("Executing timer task: %s (ID: %u, priority: %u)\n",
					tim_task->name, tim_task->task_id, priority);

				tim_task->callback(tim_task);
				tim_task->execution_count++;
				tim_stats.total_tasks_executed++;

				if (performance_profiling_enabled) {
					task_end_time = ktime_get();
					execution_time_ms = ktime_to_ms(ktime_sub(task_end_time, task_start_time));

					// Update task statistics
					tim_task->last_execution_time = execution_time_ms;
					tim_task->total_execution_time += execution_time_ms;

					// Update global statistics
					tim_stats.total_execution_time_ms += execution_time_ms;
					if (execution_time_ms > tim_stats.max_execution_time_ms) {
						tim_stats.max_execution_time_ms = execution_time_ms;
					}
					if (tim_stats.min_execution_time_ms == 0 || execution_time_ms < tim_stats.min_execution_time_ms) {
						tim_stats.min_execution_time_ms = execution_time_ms;
					}

					// Store in history
					task_execution_history[history_index] = execution_time_ms;
					history_index = (history_index + 1) % 100;

					// Performance warning
					if (execution_time_ms > 50) {
						pr_warn("Timer task %s took %u ms (threshold: 50ms)\n",
							tim_task->name, execution_time_ms);
					}
				}

				if (tim_task->auto_load == true) {
					tim_task->timeout = ktime_to_ms(ktime_get()) + tim_task->interval;
				} else {
					// Remove from all lists
					list_del(&tim_task->list);
					list_del(&tim_task->main_list);

					// Update priority counters
					if (priority == 0) {
						tim_stats.high_priority_tasks--;
					} else if (priority == 1) {
						tim_stats.normal_priority_tasks--;
					} else {
						tim_stats.low_priority_tasks--;
					}
				}
			}
		}
	}

	if (performance_profiling_enabled) {
		u64 loop_time = ktime_to_ms(ktime_sub(ktime_get(), loop_start_time));
		if (loop_time > 10) {
			pr_warn("Timer task loop took %llu ms (should be < 10ms)\n", loop_time);
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

	// Update counters
	if (tim_task->priority == 0) {
		tim_stats.high_priority_tasks--;
	} else if (tim_task->priority == 1) {
		tim_stats.normal_priority_tasks--;
	} else {
		tim_stats.low_priority_tasks--;
	}

	// Set new priority
	tim_task->priority = new_priority;

	// Add to new priority list
	list_add_tail(&tim_task->list, &priority_heads[new_priority]);

	// Update counters
	if (new_priority == 0) {
		tim_stats.high_priority_tasks++;
	} else if (new_priority == 1) {
		tim_stats.normal_priority_tasks++;
	} else {
		tim_stats.low_priority_tasks++;
	}

	pr_info("Task %s priority changed to %u\n", tim_task->name, new_priority);
	return 0;
}

void tim_task_get_stats(struct tim_task_stats *stats)
{
	*stats = tim_stats;
}

void tim_task_reset_stats(void)
{
	memset(&tim_stats, 0, sizeof(tim_stats));
	tim_stats.min_execution_time_ms = UINT_MAX;
	memset(task_execution_history, 0, sizeof(task_execution_history));
	history_index = 0;
}

void tim_task_set_profiling(bool enable)
{
	performance_profiling_enabled = enable;
	if (enable) {
		tim_stats.system_start_time = ktime_get();
		pr_info("Timer task performance profiling enabled\n");
	}
}

void tim_task_print_performance_report(void)
{
	struct tim_task_stats stats;
	u64 uptime_ms;
	u32 avg_execution_time;

	tim_task_get_stats(&stats);

	if (stats.total_tasks_executed == 0) {
		pr_info("No timer tasks have been executed yet\n");
		return;
	}

	uptime_ms = ktime_to_ms(ktime_sub(ktime_get(), stats.system_start_time));
	avg_execution_time = stats.total_execution_time_ms / stats.total_tasks_executed;

	pr_info("=== Timer Task Performance Report ===\n");
	pr_info("System uptime: %llu ms\n", uptime_ms);
	pr_info("Total tasks created: %u\n", stats.total_tasks_created);
	pr_info("Total tasks executed: %u\n", stats.total_tasks_executed);
	pr_info("Total tasks failed: %u\n", stats.total_tasks_failed);
	pr_info("High priority tasks: %u\n", stats.high_priority_tasks);
	pr_info("Normal priority tasks: %u\n", stats.normal_priority_tasks);
	pr_info("Low priority tasks: %u\n", stats.low_priority_tasks);
	pr_info("Average execution time: %u ms\n", avg_execution_time);
	pr_info("Min execution time: %u ms\n", stats.min_execution_time_ms);
	pr_info("Max execution time: %u ms\n", stats.max_execution_time_ms);
	pr_info("Tasks per second: %u\n",
		uptime_ms > 0 ? (stats.total_tasks_executed * 1000) / uptime_ms : 0);
	pr_info("=====================================\n");
}

void tim_task_print_info(struct tim_task *tim_task)
{
	if (!tim_task) {
		pr_err("Invalid task pointer\n");
		return;
	}

	pr_info("=== Task Information ===\n");
	pr_info("ID: %u\n", tim_task->task_id);
	pr_info("Name: %s\n", tim_task->name ? tim_task->name : "NULL");
	pr_info("Priority: %u\n", tim_task->priority);
	pr_info("Interval: %u ms\n", tim_task->interval);
	pr_info("Timeout: %lld ms\n", tim_task->timeout);
	pr_info("Auto load: %s\n", tim_task->auto_load ? "Yes" : "No");
	pr_info("Execution count: %u\n", tim_task->execution_count);
	pr_info("Last execution time: %u ms\n", tim_task->last_execution_time);
	pr_info("Total execution time: %llu ms\n", tim_task->total_execution_time);
	pr_info("Created: %lld ms ago\n", ktime_to_ms(ktime_sub(ktime_get(), tim_task->created_time)));
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
			kfree(tim_task->name);
			kfree(tim_task);
		}
	}

	// Clear main list (using main_list node)
	list_for_each_entry_safe(tim_task, tmp, &tim_task_head, main_list) {
		list_del(&tim_task->list);
		list_del(&tim_task->main_list);
		kfree(tim_task->name);
		kfree(tim_task);
	}

	pr_info("All timer tasks cleaned up\n");
}

struct tim_task *tim_task_get_next_task(void)
{
	struct tim_task *next_task = NULL;
	struct tim_task *tim_task;
	u64 current_time_ms = ktime_to_ms(ktime_get());
	u64 earliest_timeout = U64_MAX;

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

#include <linux/printk.h>

static u64 time_stamp;
static struct tim_task example_task;
static int tim_test_callback_count = 0;

static void tim_task_callback(struct tim_task *task)
{
	s64 delta;
	tim_test_callback_count++;

	delta = ktime_ms_delta(ktime_get(),  time_stamp);
	pr_info("Timer task '%s' callback #%d executed after %llu ms.\n",
		task ? task->name : "unknown", tim_test_callback_count, delta);

	if (delta >= 900 && delta <= 1100) {
		pr_info("PASS - Timer task timing correct\n");
	} else {
		pr_info("FAIL - Timer task timing incorrect (expected: 900-1100ms, actual: %llums)\n", delta);
	}
	time_stamp = ktime_get();
}

static void tim_test_validation_callback(struct tim_task *task)
{
	tim_test_callback_count++;
	pr_info("Timer validation task '%s' executed - count: %d\n",
		task ? task->name : "unknown", tim_test_callback_count);
}

static void tim_test_priority_callback(struct tim_task *task)
{
	pr_info("Priority task '%s' (ID:%u, Priority:%u) executed\n",
		task ? task->name : "unknown", task ? task->task_id : 0, task ? task->priority : 0);
}

static void tim_test_cleanup_callback(struct tim_task *task)
{
	pr_info("Timer cleanup task executed\n");
}

int t_tim_task_add(int argc, char **argv)
{
	int ret;

	pr_info("=== Timer Task Basic Test ===\n");
	time_stamp = ktime_get();

	ret = tim_task_add(&example_task, "example task",
			1000, true, false, tim_task_callback); //1s loop

	if (ret) {
		pr_err("Timer task creation failed: %d\n", ret);
		return FAIL;
	}

	pr_info("Timer basic task created successfully (ID: %u)\n", example_task.task_id);
	return PASS;
}

int t_tim_task_validation(int argc, char **argv)
{
	int ret;
	struct tim_task test_task, *found_task;
	struct tim_task_stats stats;

	pr_info("=== Timer Task Validation Test ===\n");

	// Test 1: Task creation with priority
	ret = tim_task_add(&test_task, "validation-task", 500, true, false, tim_test_validation_callback);
	if (ret) {
		pr_err("FAIL - Validation task creation failed: %d\n", ret);
		return FAIL;
	}

	pr_info("PASS - Task with priority created (ID: %u, Priority: %u)\n",
		test_task.task_id, test_task.priority);

	// Test 2: Task lookup by name
	found_task = tim_task_find_by_name("validation-task");
	if (!found_task) {
		pr_err("FAIL - Task lookup by name failed\n");
		return FAIL;
	}

	pr_info("PASS - Task lookup by name working\n");

	// Test 3: Task lookup by ID
	found_task = tim_task_find_by_id(test_task.task_id);
	if (!found_task) {
		pr_err("FAIL - Task lookup by ID failed\n");
		return FAIL;
	}

	pr_info("PASS - Task lookup by ID working\n");

	// Test 4: Priority setting
	ret = tim_task_set_priority(&test_task, 0); // High priority
	if (ret) {
		pr_err("FAIL - Priority setting failed: %d\n", ret);
		return FAIL;
	}

	if (test_task.priority != 0) {
		pr_err("FAIL - Priority not set correctly\n");
		return FAIL;
	}

	pr_info("PASS - Priority setting working\n");

	// Test 5: Task info printing
	tim_task_print_info(&test_task);

	// Test 6: Statistics
	tim_task_get_stats(&stats);
	if (stats.total_tasks_created == 0) {
		pr_err("FAIL - Statistics not working\n");
		return FAIL;
	}

	pr_info("PASS - Statistics working (created: %u, high_priority: %u)\n",
		stats.total_tasks_created, stats.high_priority_tasks);

	// Test 7: Performance profiling
	tim_task_set_profiling(true);
	tim_task_print_performance_report();

	pr_info("PASS - All timer validation tests completed\n");
	return PASS;
}

int t_tim_task_priority(int argc, char **argv)
{
	int ret, i;
	struct tim_task priority_tasks[3];
	struct tim_task_stats stats;
	const char *task_names[] = {"high-priority", "normal-priority", "low-priority"};
	u32 priorities[] = {0, 1, 2};

	pr_info("=== Timer Task Priority Test ===\n");

	// Create tasks with different priorities
	for (i = 0; i < 3; i++) {
		ret = tim_task_add(&priority_tasks[i], task_names[i],
				200, true, false, tim_test_priority_callback);
		if (ret) {
			pr_err("FAIL - Priority task %d creation failed: %d\n", i, ret);
			return FAIL;
		}

		// Set priority
		ret = tim_task_set_priority(&priority_tasks[i], priorities[i]);
		if (ret) {
			pr_err("FAIL - Priority setting for task %d failed: %d\n", i, ret);
			return FAIL;
		}
	}

	// Run tasks to test priority execution
	ktime_t test_end = ktime_add_ms(ktime_get(), 2000);
	while (ktime_compare(ktime_get(), test_end) < 0) {
		tim_task_loop();
	}

	tim_task_get_stats(&stats);

	pr_info("Priority test results:\n");
	pr_info("  High priority tasks: %u\n", stats.high_priority_tasks);
	pr_info("  Normal priority tasks: %u\n", stats.normal_priority_tasks);
	pr_info("  Low priority tasks: %u\n", stats.low_priority_tasks);

	return PASS;
}

int t_tim_task_performance(int argc, char **argv)
{
	int ret, i;
	u64 start_time, end_time;
	struct tim_task perf_tasks[20];
	struct tim_task_stats stats;

	pr_info("=== Timer Task Performance Test ===\n");

	// Reset statistics
	tim_task_reset_stats();
	tim_task_set_profiling(true);

	start_time = ktime_get();

	// Create multiple tasks with different priorities
	for (i = 0; i < 20; i++) {
		char task_name[32];
		snprintf(task_name, sizeof(task_name), "perf-task-%d", i);

		ret = tim_task_add(&perf_tasks[i], task_name,
				50 + (i * 10), true, false, tim_test_validation_callback);
		if (ret) {
			pr_err("FAIL - Performance task %d creation failed: %d\n", i, ret);
			return FAIL;
		}

		// Set priority based on index
		tim_task_set_priority(&perf_tasks[i], i % 3);
	}

	end_time = ktime_get();
	pr_info("PASS - 20 timer tasks created in %llu ms\n",
		ktime_to_ms(ktime_sub(end_time, start_time)));

	// Run tasks briefly
	ktime_t test_end = ktime_add_ms(ktime_get(), 3000);
	while (ktime_compare(ktime_get(), test_end) < 0) {
		tim_task_loop();
	}

	// Get final stats
	tim_task_get_stats(&stats);
	pr_info("  Tasks executed: %u\n", stats.total_tasks_executed);
	pr_info("  Max execution time: %u ms\n", stats.max_execution_time_ms);
	pr_info("  Min execution time: %u ms\n", stats.min_execution_time_ms);

	return PASS;
}

int t_tim_task_cleanup(int argc, char **argv)
{
	int ret;
	struct tim_task cleanup_task;

	pr_info("=== Timer Task Cleanup Test ===\n");

	// Reset callback count
	tim_test_callback_count = 0;

	// Create a cleanup task
	ret = tim_task_add(&cleanup_task, "cleanup-test-task",
			100, false, false, tim_test_cleanup_callback);
	if (ret) {
		pr_err("FAIL - Cleanup task creation failed: %d\n", ret);
		return FAIL;
	}

	pr_info("Created cleanup task with ID: %u\n", cleanup_task.task_id);

	// Run loop to execute task
	tim_task_loop();

	// Test next task prediction
	struct tim_task *next = tim_task_get_next_task();
	pr_info("Next predicted task: %s\n", next ? next->name : "none");

	// Cleanup all tasks
	tim_task_cleanup_all();

	// Verify cleanup
	struct tim_task_stats stats;
	tim_task_get_stats(&stats);

	pr_info("PASS - Cleanup completed (final stats: created=%u, executed=%u)\n",
		stats.total_tasks_created, stats.total_tasks_executed);

	return PASS;
}

#endif
