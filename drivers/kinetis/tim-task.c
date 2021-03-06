#include "kinetis/tim-task.h"
#include "kinetis/basic-timer.h"

#include <linux/slab.h>
#include <linux/errno.h>

#include "string.h"

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

/**
  * @brief  add the tim_task struct handle.
  * @param  tim_task: the tim_task handle strcut.
  * @param  timeout_cb: Timeout callback.
  * @param  timeout: time out time.
  * @param  repeat: repeat interval time.
  * @retval None
  */
int tim_task_add(u32 interval, bool auto_load, void(*callback)())
{
    struct tim_task *tim_task;

    tim_task = kmalloc(sizeof(*tim_task), GFP_KERNEL);

    if (!tim_task)
        return -ENOMEM;

    if (!callback)
        return -EINVAL;

    tim_task->callback = callback;
    tim_task->timeout = basic_timer_get_ms() + interval;
    tim_task->interval = interval;
    tim_task->auto_load = auto_load;
    tim_task->self_alloc = true;

    list_add_tail(&tim_task->list, &tim_task_head);

    return 0;
}

/**
  * @brief  drop the tim_task struct handle.
  * @param  tim_task: the tim_task tim_task strcut.
  * @retval None
  */
int tim_task_drop(void(*callback)())
{
    struct tim_task *tim_task, *tmp;

    list_for_each_entry_safe(tim_task, tmp, &tim_task_head, list) {
        if (tim_task->callback == callback) {
            list_del(&tim_task->list);
            kfree(tim_task);
            return 0;
        }
    }

    return -EINVAL;
}

/**
  * @brief  add the tim_task struct handle.
  * @param  tim_task: the tim_task handle strcut.
  * @param  timeout_cb: Timeout callback.
  * @param  timeout: time out time.
  * @param  repeat: repeat interval time.
  * @retval None
  */
int tim_task_enqueue(struct tim_task *tim_task,
    u32 interval, void(*callback)(struct tim_task *))
{
    tim_task->callback = callback;
    tim_task->timeout = basic_timer_get_ms() + interval;
    tim_task->interval = interval;
    tim_task->auto_load = false;
    tim_task->self_alloc = false;

    list_add_tail(&tim_task->list, &tim_task_head);

    return 0;
}

/**
  * @brief  drop the tim_task struct handle.
  * @param  tim_task: the tim_task tim_task strcut.
  * @retval None
  */
void tim_task_dequeue(struct tim_task *tim_task)
{
    list_del(&tim_task->list);
}

/**
  * @brief  suspend the tim_task struct handle.
  * @param  tim_task: the tim_task tim_task strcut.
  * @retval None
  */
int tim_task_suspend(void(*callback)())
{
    struct tim_task *tim_task, *tmp;

    list_for_each_entry_safe(tim_task, tmp, &tim_task_head, list) {
        if (tim_task->callback == callback) {
            list_move_tail(&tim_task->list, &tim_task_suspend_head);
            return 0;
        }
    }

    return -EINVAL;
}

/**
  * @brief  resume the tim_task struct handle.
  * @param  tim_task: the tim_task tim_task strcut.
  * @retval None
  */
int tim_task_resume(void(*callback)())
{
    struct tim_task *tim_task, *tmp;

    list_for_each_entry_safe(tim_task, tmp, &tim_task_suspend_head, list) {
        if (tim_task->callback == callback) {
            list_move_tail(&tim_task->list, &tim_task_head);
            return 0;
        }
    }

    return -EINVAL;
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
        if (tim_task->timeout <= basic_timer_get_ms()) {
            tim_task->callback(tim_task);

            if (tim_task->auto_load == true)
                tim_task->timeout =
                    basic_timer_get_ms() + tim_task->interval;
            else {
                list_del(&tim_task->list);

                if (tim_task->self_alloc == true)
                    kfree(tim_task);
            }
        }
    }
}

#ifdef DESIGN_VERIFICATION_TIMTASK
#include "kinetis/test-kinetis.h"
#include "kinetis/idebug.h"

#include <linux/iopoll.h>
#include <linux/printk.h>

static u64 time_stamp;

void tim_task_callback(void)
{
    time_stamp = basic_timer_get_ms() - time_stamp;
    printk(KERN_DEBUG "timeout! tim_task elapse time = %llu ms.\n", time_stamp);

    if (time_stamp >= 900 && time_stamp <= 1100)
        printk(KERN_DEBUG "PASS\n");
    else
        printk(KERN_DEBUG "FAIL\n");
}

int t_tim_task_add(int argc, char **argv)
{
    int ret;

    time_stamp = basic_timer_get_ms();

    ret = tim_task_add(1000, false, tim_task_callback); //1s loop

    if (ret)
        goto err;

    return PASS;

err:
    printk(KERN_ERR "Failed to execute %s(), error code: %d\n",
        __func__, ret);
    return FAIL;
}

#endif

