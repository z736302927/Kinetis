
#include <generated/deconfig.h>
#include <linux/slab.h>
#include <linux/errno.h>

#include <kinetis/design_verification.h>

#include <kinetis/tim-task.h>

//tim_task handle list head.
static LIST_HEAD(tim_task_head);
static LIST_HEAD(tim_task_suspend_head);

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

	if (!callback || !name)
		return -EINVAL;

	tim_task->name = kstrdup_const(name, GFP_KERNEL);
	if (!tim_task->name)
		return -ENOMEM;
		
	tim_task->callback = callback;
	tim_task->timeout = ktime_to_ms(ktime_get()) + interval;
	tim_task->interval = interval;
	tim_task->auto_load = auto_load;

	//if (sched) {
	//	ret = fmu_sch_add_task(tim_task);
	//	if (ret)
	//		return ret;
	//	tim_task->sched = true;
	//}

	list_add_tail(&tim_task->list, &tim_task_head);

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
  * @brief  main loop.
  * @param  None.
  * @retval None
  */
void tim_task_loop(void)
{
	struct tim_task *tim_task, *tmp;

	list_for_each_entry_safe(tim_task, tmp, &tim_task_head, list) {
		if (tim_task->timeout <= ktime_to_ms(ktime_get())) {
			tim_task->callback(tim_task);

			if (tim_task->auto_load == true)
				tim_task->timeout = ktime_to_ms(ktime_get()) +
					tim_task->interval;
			else
				list_del(&tim_task->list);
		}
	}
}

#ifdef DESIGN_VERIFICATION_TIMTASK
#include "kinetis/test-kinetis.h"

#include <linux/printk.h>

static u64 time_stamp;
static struct tim_task example_task;

static void tim_task_callback(struct tim_task *task)
{
	s64 delta;

	delta = ktime_ms_delta(ktime_get(),  time_stamp);
	pr_info("timeout! tim_task elapse time = %llu ms.\n", delta);

	if (delta >= 900 && delta <= 1100)
		pr_info("PASS\n");
	else
		pr_info("FAIL\n");

	time_stamp = ktime_get();
}

int t_tim_task_add(int argc, char **argv)
{
	int ret;

	time_stamp = ktime_get();

	ret = tim_task_add(&example_task, "example task",
		1000, true, false, tim_task_callback); //1s loop

	if (ret)
		goto err;

	return PASS;

err:
	pr_err("Failed to execute %s(), error code: %d\n",
		__func__, ret);
	return FAIL;
}

#endif

