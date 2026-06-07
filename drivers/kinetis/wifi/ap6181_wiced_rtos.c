/**
 * @file ap6181_wiced_rtos.c
 * @brief Kinetis bare-metal RTOS port for WICED SDK
 *
 * Implements all wiced_rtos_* functions without an RTOS kernel.
 * Threads are stored as metadata; real execution happens via
 * wiced_rtos_poll() which dispatches the WWD poll loop,
 * worker-thread events, and lwIP timers cooperatively.
 *
 * Semaphores / mutexes / queues operate on lightweight C structs
 * because the system is single-threaded and non-preemptive.
 */

#include <linux/kernel.h>
#include <linux/printk.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/errno.h>

#include <kinetis/tim-task.h>
#include <kinetis/basic-timer.h>

#include "ap6181_wiced_rtos.h"

/* Pull in WICED API prototypes */
#include "wiced_rtos.h"

/* ── forward declarations from WWD poll layer ─────────── */
extern void wwd_thread_poll_all(void);

/* ── Internal data structures ─────────────────────────── */

#define MAX_THREADS           8
#define MAX_SEMAPHORES       16
#define MAX_MUTEXES          16
#define MAX_QUEUES            8
#define MAX_TIMERS           16
#define MAX_EVENTS           32

struct k_semaphore {
	u8  used;
	u32 count;
};

struct k_mutex {
	u8 used;
	u8 locked;
};

struct k_queue {
	u8   used;
	u8  *buffer;
	u32  msg_size;
	u32  capacity;
	u32  head;
	u32  tail;
	u32  count;
};

struct k_timer {
	u8              used;
	timer_handler_t function;
	void           *arg;
	u32             period_ms;
	u64             next_fire_ms;
	u8              running;
};

struct k_event {
	u8               used;
	event_handler_t  function;
	void            *arg;
	u32              period_ms;
	u64              next_fire_ms;
	wiced_worker_thread_t *thread;
	u8               active;
};

/* Thread metadata — not an OS thread, just a record */
struct k_thread {
	u8                       used;
	wiced_thread_function_t  func;
	void                    *arg;
	const char              *name;
	u8                       priority;
	u8                       is_worker;
	void                    *event_queue_ptr; /* ptr to struct k_queue */
};

struct k_worker {
	u8              used;
	wiced_thread_t  thread_id;
	struct k_queue *event_queue;
};

/* ── Static arrays (single-instance, no dynamic alloc) ── */

static struct k_semaphore  g_semaphores[MAX_SEMAPHORES];
static struct k_mutex      g_mutexes[MAX_MUTEXES];
static struct k_queue      g_queues[MAX_QUEUES];
static struct k_timer      g_timers[MAX_TIMERS];
static struct k_event      g_events[MAX_EVENTS];
static struct k_thread     g_threads[MAX_THREADS];
static struct k_worker     g_workers[2]; /* H/W IO + Network */

/* Global worker thread handles */
wiced_worker_thread_t wiced_hardware_io_worker_thread;
wiced_worker_thread_t wiced_networking_worker_thread;

/* ── helpers ──────────────────────────────────────────── */

static u64 _now_ms(void)
{
	return basic_timer_get_ms();
}

static struct k_semaphore *_find_semaphore(wiced_semaphore_t *s)
{
	ptrdiff_t idx = (ptrdiff_t)(*s);
	if (idx >= 0 && idx < MAX_SEMAPHORES && g_semaphores[idx].used)
		return &g_semaphores[idx];
	return NULL;
}

static struct k_mutex *_find_mutex(wiced_mutex_t *m)
{
	ptrdiff_t idx = (ptrdiff_t)(*m);
	if (idx >= 0 && idx < MAX_MUTEXES && g_mutexes[idx].used)
		return &g_mutexes[idx];
	return NULL;
}

static struct k_queue *_find_queue(wiced_queue_t *q)
{
	ptrdiff_t idx = (ptrdiff_t)(*q);
	if (idx >= 0 && idx < MAX_QUEUES && g_queues[idx].used)
		return &g_queues[idx];
	return NULL;
}

static struct k_timer *_find_timer(wiced_timer_t *timer)
{
	ptrdiff_t idx = (ptrdiff_t)timer->handle;
	if (idx >= 0 && idx < MAX_TIMERS && g_timers[idx].used)
		return &g_timers[idx];
	return NULL;
}

/* ── FreeRTOS stub functions (called by WICED internals) ─ */

void vTaskDelay(uint32_t ticks)
{
	mdelay(ticks);
}

void vTaskDelete(TaskHandle_t task)
{
	(void)task;
}

void *pvPortMalloc(size_t size)
{
	return kmalloc(size, GFP_KERNEL);
}

void vPortFree(void *ptr)
{
	kfree(ptr);
}

/* ── RTOS port init ───────────────────────────────────── */

void wiced_rtos_port_init(void)
{
	memset(g_semaphores, 0, sizeof(g_semaphores));
	memset(g_mutexes,     0, sizeof(g_mutexes));
	memset(g_queues,      0, sizeof(g_queues));
	memset(g_timers,      0, sizeof(g_timers));
	memset(g_events,      0, sizeof(g_events));
	memset(g_threads,     0, sizeof(g_threads));
	memset(g_workers,     0, sizeof(g_workers));

	memset(&wiced_hardware_io_worker_thread, 0,
	       sizeof(wiced_hardware_io_worker_thread));
	memset(&wiced_networking_worker_thread, 0,
	       sizeof(wiced_networking_worker_thread));

	pr_info("wiced_rtos: port initialized (bare-metal, cooperative)\n");
}

/* ── Threads ──────────────────────────────────────────── */

wiced_result_t wiced_rtos_create_thread(wiced_thread_t *thread,
	uint8_t priority, const char *name,
	wiced_thread_function_t function,
	uint32_t stack_size, void *arg)
{
	int i;
	(void)stack_size;

	for (i = 0; i < MAX_THREADS; i++) {
		if (!g_threads[i].used) {
			g_threads[i].used     = 1;
			g_threads[i].func     = function;
			g_threads[i].arg      = arg;
			g_threads[i].name     = name;
			g_threads[i].priority = priority;
			g_threads[i].is_worker = 0;
			*thread = (wiced_thread_t)(ptrdiff_t)i;
			return WICED_SUCCESS;
		}
	}
	pr_err("wiced_rtos: out of thread slots\n");
	return WICED_ERROR;
}

wiced_result_t wiced_rtos_create_thread_with_stack(
	wiced_thread_t *thread, uint8_t priority, const char *name,
	wiced_thread_function_t function,
	void *stack, uint32_t stack_size, void *arg)
{
	/* Stack is pre-allocated by caller — we don't use it */
	(void)stack;
	return wiced_rtos_create_thread(thread, priority, name,
	                                function, stack_size, arg);
}

wiced_result_t wiced_rtos_delete_thread(wiced_thread_t *thread)
{
	ptrdiff_t idx = (ptrdiff_t)(*thread);
	if (idx >= 0 && idx < MAX_THREADS)
		g_threads[idx].used = 0;
	*thread = NULL;
	return WICED_SUCCESS;
}

wiced_result_t wiced_rtos_delay_milliseconds(uint32_t milliseconds)
{
	mdelay(milliseconds);
	return WICED_SUCCESS;
}

wiced_result_t wiced_rtos_delay_microseconds(uint32_t microseconds)
{
	udelay(microseconds);
	return WICED_SUCCESS;
}

wiced_result_t wiced_rtos_thread_join(wiced_thread_t *thread)
{
	(void)thread;
	return WICED_SUCCESS;
}

wiced_result_t wiced_rtos_thread_force_awake(wiced_thread_t *thread)
{
	(void)thread;
	return WICED_SUCCESS;
}

wiced_result_t wiced_rtos_is_current_thread(wiced_thread_t *thread)
{
	(void)thread;
	return WICED_SUCCESS; /* single-threaded: always current */
}

wiced_result_t wiced_rtos_check_stack(void)
{
	return WICED_SUCCESS;
}

wiced_result_t wiced_rtos_thread_yield(void)
{
	return WICED_SUCCESS;
}

/* ── Semaphores ───────────────────────────────────────── */

wiced_result_t wiced_rtos_init_semaphore(wiced_semaphore_t *semaphore)
{
	int i;
	for (i = 0; i < MAX_SEMAPHORES; i++) {
		if (!g_semaphores[i].used) {
			g_semaphores[i].used  = 1;
			g_semaphores[i].count = 0;
			*semaphore = (wiced_semaphore_t)(ptrdiff_t)i;
			return WICED_SUCCESS;
		}
	}
	return WICED_ERROR;
}

wiced_result_t wiced_rtos_set_semaphore(wiced_semaphore_t *semaphore)
{
	struct k_semaphore *s = _find_semaphore(semaphore);
	if (!s)
		return WICED_ERROR;
	s->count++;
	return WICED_SUCCESS;
}

wiced_result_t wiced_rtos_get_semaphore(wiced_semaphore_t *semaphore,
	uint32_t timeout_ms)
{
	struct k_semaphore *s = _find_semaphore(semaphore);
	if (!s)
		return WICED_ERROR;

	/*
	 * Cooperative single-threaded: if count > 0, take it.
	 * If timeout_ms == 0, return immediately with TIMEOUT.
	 * If timeout_ms > 0, we busy-wait (blocking) because
	 * in cooperative mode there is no other thread to wake us.
	 * This is safe only because WICED typically uses timeout=0
	 * in the wwd_thread poll path.
	 */
	if (s->count > 0) {
		s->count--;
		return WICED_SUCCESS;
	}
	if (timeout_ms == 0)
		return WICED_TIMEOUT;

	/* Busy-wait (used only during init handshake) */
	{
		u64 deadline = _now_ms() + timeout_ms;
		while (s->count == 0 && _now_ms() < deadline) {
			/* Process pending WWD packets during wait */
			wwd_thread_poll_all();
		}
		if (s->count > 0) {
			s->count--;
			return WICED_SUCCESS;
		}
	}
	return WICED_TIMEOUT;
}

wiced_result_t wiced_rtos_deinit_semaphore(wiced_semaphore_t *semaphore)
{
	struct k_semaphore *s = _find_semaphore(semaphore);
	if (!s)
		return WICED_ERROR;
	s->used = 0;
	*semaphore = NULL;
	return WICED_SUCCESS;
}

/* ── Mutexes ──────────────────────────────────────────── */

wiced_result_t wiced_rtos_init_mutex(wiced_mutex_t *mutex)
{
	int i;
	for (i = 0; i < MAX_MUTEXES; i++) {
		if (!g_mutexes[i].used) {
			g_mutexes[i].used   = 1;
			g_mutexes[i].locked = 0;
			*mutex = (wiced_mutex_t)(ptrdiff_t)i;
			return WICED_SUCCESS;
		}
	}
	return WICED_ERROR;
}

wiced_result_t wiced_rtos_lock_mutex(wiced_mutex_t *mutex)
{
	struct k_mutex *m = _find_mutex(mutex);
	if (!m)
		return WICED_ERROR;
	/* Single-threaded: always succeed */
	m->locked = 1;
	return WICED_SUCCESS;
}

wiced_result_t wiced_rtos_unlock_mutex(wiced_mutex_t *mutex)
{
	struct k_mutex *m = _find_mutex(mutex);
	if (!m)
		return WICED_ERROR;
	m->locked = 0;
	return WICED_SUCCESS;
}

wiced_result_t wiced_rtos_deinit_mutex(wiced_mutex_t *mutex)
{
	struct k_mutex *m = _find_mutex(mutex);
	if (!m)
		return WICED_ERROR;
	m->used = 0;
	*mutex = NULL;
	return WICED_SUCCESS;
}

/* ── Queues ───────────────────────────────────────────── */

wiced_result_t wiced_rtos_init_queue(wiced_queue_t *queue,
	const char *name, uint32_t message_size,
	uint32_t number_of_messages)
{
	int i;
	(void)name;

	for (i = 0; i < MAX_QUEUES; i++) {
		if (!g_queues[i].used) {
			g_queues[i].used     = 1;
			g_queues[i].msg_size = message_size;
			g_queues[i].capacity = number_of_messages;
			g_queues[i].head     = 0;
			g_queues[i].tail     = 0;
			g_queues[i].count    = 0;
			g_queues[i].buffer   = kmalloc(
				message_size * number_of_messages,
				GFP_KERNEL);
			if (!g_queues[i].buffer) {
				g_queues[i].used = 0;
				return WICED_ERROR;
			}
			*queue = (wiced_queue_t)(ptrdiff_t)i;
			return WICED_SUCCESS;
		}
	}
	return WICED_ERROR;
}

wiced_result_t wiced_rtos_push_to_queue(wiced_queue_t *queue,
	void *message, uint32_t timeout_ms)
{
	struct k_queue *q = _find_queue(queue);
	(void)timeout_ms;
	if (!q)
		return WICED_ERROR;
	if (q->count >= q->capacity)
		return WICED_ERROR;

	memcpy(q->buffer + q->head * q->msg_size,
	       message, q->msg_size);
	q->head = (q->head + 1) % q->capacity;
	q->count++;
	return WICED_SUCCESS;
}

wiced_result_t wiced_rtos_pop_from_queue(wiced_queue_t *queue,
	void *message, uint32_t timeout_ms)
{
	struct k_queue *q = _find_queue(queue);
	(void)timeout_ms;
	if (!q)
		return WICED_ERROR;
	if (q->count == 0)
		return WICED_ERROR;

	memcpy(message, q->buffer + q->tail * q->msg_size,
	       q->msg_size);
	q->tail = (q->tail + 1) % q->capacity;
	q->count--;
	return WICED_SUCCESS;
}

wiced_result_t wiced_rtos_deinit_queue(wiced_queue_t *queue)
{
	struct k_queue *q = _find_queue(queue);
	if (!q)
		return WICED_ERROR;
	if (q->buffer)
		kfree(q->buffer);
	q->used = 0;
	*queue = NULL;
	return WICED_SUCCESS;
}

wiced_result_t wiced_rtos_is_queue_empty(wiced_queue_t *queue)
{
	struct k_queue *q = _find_queue(queue);
	if (!q)
		return WICED_ERROR;
	return (q->count == 0) ? WICED_SUCCESS : WICED_ERROR;
}

wiced_result_t wiced_rtos_is_queue_full(wiced_queue_t *queue)
{
	struct k_queue *q = _find_queue(queue);
	if (!q)
		return WICED_ERROR;
	return (q->count >= q->capacity) ? WICED_SUCCESS : WICED_ERROR;
}

wiced_result_t wiced_rtos_get_queue_occupancy(wiced_queue_t *queue,
	uint32_t *count)
{
	struct k_queue *q = _find_queue(queue);
	if (!q)
		return WICED_ERROR;
	*count = q->count;
	return WICED_SUCCESS;
}

/* ── Timers ───────────────────────────────────────────── */

wiced_result_t wiced_rtos_init_timer(wiced_timer_t *timer,
	uint32_t time_ms, timer_handler_t function, void *arg)
{
	int i;
	for (i = 0; i < MAX_TIMERS; i++) {
		if (!g_timers[i].used) {
			g_timers[i].used      = 1;
			g_timers[i].function  = function;
			g_timers[i].arg       = arg;
			g_timers[i].period_ms = time_ms;
			g_timers[i].next_fire_ms = 0;
			g_timers[i].running   = 0;
			timer->handle   = (TimerHandle_t)(ptrdiff_t)i;
			timer->function = function;
			timer->arg      = arg;
			return WICED_SUCCESS;
		}
	}
	return WICED_ERROR;
}

wiced_result_t wiced_rtos_start_timer(wiced_timer_t *timer)
{
	struct k_timer *t = _find_timer(timer);
	if (!t)
		return WICED_ERROR;
	t->running       = 1;
	t->next_fire_ms  = _now_ms() + t->period_ms;
	return WICED_SUCCESS;
}

wiced_result_t wiced_rtos_stop_timer(wiced_timer_t *timer)
{
	struct k_timer *t = _find_timer(timer);
	if (!t)
		return WICED_ERROR;
	t->running = 0;
	return WICED_SUCCESS;
}

wiced_result_t wiced_rtos_deinit_timer(wiced_timer_t *timer)
{
	struct k_timer *t = _find_timer(timer);
	if (!t)
		return WICED_ERROR;
	t->used = 0;
	memset(timer, 0, sizeof(*timer));
	return WICED_SUCCESS;
}

wiced_result_t wiced_rtos_is_timer_running(wiced_timer_t *timer)
{
	struct k_timer *t = _find_timer(timer);
	if (!t)
		return WICED_ERROR;
	return t->running ? WICED_SUCCESS : WICED_ERROR;
}

/* ── Worker threads ───────────────────────────────────── */

wiced_result_t wiced_rtos_create_worker_thread(
	wiced_worker_thread_t *worker_thread,
	uint8_t priority, uint32_t stack_size,
	uint32_t event_queue_size)
{
	int slot;

	(void)priority;
	(void)stack_size;

	/* Assign to hw_io worker (first call) or networking worker */
	for (slot = 0; slot < 2; slot++) {
		if (!g_workers[slot].used) {
			g_workers[slot].used = 1;
			/* Worker threads don't actually run as threads;
			 * their events are dispatched in wiced_rtos_poll() */
			worker_thread->thread      = NULL;
			worker_thread->event_queue = NULL;
			break;
		}
	}

	pr_info("wiced_rtos: worker thread registered (events=%u)\n",
		event_queue_size);
	return WICED_SUCCESS;
}

wiced_result_t wiced_rtos_delete_worker_thread(
	wiced_worker_thread_t *worker_thread)
{
	(void)worker_thread;
	return WICED_SUCCESS;
}

/* ── Timed events ─────────────────────────────────────── */

wiced_result_t wiced_rtos_register_timed_event(
	wiced_timed_event_t *event_object,
	wiced_worker_thread_t *worker_thread,
	event_handler_t function, uint32_t time_ms, void *arg)
{
	int i;
	(void)worker_thread;

	for (i = 0; i < MAX_EVENTS; i++) {
		if (!g_events[i].used) {
			g_events[i].used       = 1;
			g_events[i].function   = function;
			g_events[i].arg        = arg;
			g_events[i].period_ms  = time_ms;
			g_events[i].next_fire_ms = _now_ms() + time_ms;
			g_events[i].active     = 1;
			event_object->function = function;
			event_object->arg      = arg;
			return WICED_SUCCESS;
		}
	}
	return WICED_ERROR;
}

wiced_result_t wiced_rtos_deregister_timed_event(
	wiced_timed_event_t *event_object)
{
	int i;
	for (i = 0; i < MAX_EVENTS; i++) {
		if (g_events[i].used &&
		    g_events[i].function == event_object->function &&
		    g_events[i].arg == event_object->arg) {
			g_events[i].used   = 0;
			g_events[i].active = 0;
			return WICED_SUCCESS;
		}
	}
	return WICED_ERROR;
}

wiced_result_t wiced_rtos_send_asynchronous_event(
	wiced_worker_thread_t *worker_thread,
	event_handler_t function, void *arg)
{
	(void)worker_thread;
	/* Execute immediately in cooperative mode */
	if (function)
		return function(arg);
	return WICED_SUCCESS;
}

/* ── Event flags ──────────────────────────────────────── */

static uint32_t g_event_flags_val;
static uint8_t  g_event_flags_used;

wiced_result_t wiced_rtos_init_event_flags(
	wiced_event_flags_t *event_flags)
{
	g_event_flags_val  = 0;
	g_event_flags_used = 1;
	*event_flags = (wiced_event_flags_t)(ptrdiff_t)0;
	return WICED_SUCCESS;
}

wiced_result_t wiced_rtos_wait_for_event_flags(
	wiced_event_flags_t *event_flags,
	uint32_t flags_to_wait_for, uint32_t *flags_set,
	wiced_bool_t clear_set_flags,
	wiced_event_flags_wait_option_t wait_option,
	uint32_t timeout_ms)
{
	(void)event_flags;
	(void)timeout_ms;
	(void)wait_option;

	if (!g_event_flags_used)
		return WICED_ERROR;

	*flags_set = g_event_flags_val & flags_to_wait_for;

	if (*flags_set == 0) {
		/* Poll SDIO during wait */
		u64 deadline = _now_ms() + timeout_ms;
		while (_now_ms() < deadline) {
			wwd_thread_poll_all();
			*flags_set = g_event_flags_val & flags_to_wait_for;
			if (*flags_set)
				break;
		}
		if (*flags_set == 0)
			return WICED_TIMEOUT;
	}

	if (clear_set_flags)
		g_event_flags_val &= ~(*flags_set);
	return WICED_SUCCESS;
}

wiced_result_t wiced_rtos_set_event_flags(
	wiced_event_flags_t *event_flags,
	uint32_t flags_to_set)
{
	(void)event_flags;
	g_event_flags_val |= flags_to_set;
	return WICED_SUCCESS;
}

wiced_result_t wiced_rtos_deinit_event_flags(
	wiced_event_flags_t *event_flags)
{
	(void)event_flags;
	g_event_flags_val  = 0;
	g_event_flags_used = 0;
	return WICED_SUCCESS;
}

/* ── Cooperative poll entry ───────────────────────────── */

/**
 * @brief Main cooperative poll — call from Rotor FSM loop
 */
void wiced_rtos_poll(void)
{
	u64 now = _now_ms();
	int i;

	/* 1. WWD bus thread — SDIO packet I/O */
	wwd_thread_poll_all();

	/* 2. Fire expired RTOS timers */
	for (i = 0; i < MAX_TIMERS; i++) {
		struct k_timer *t = &g_timers[i];
		if (!t->used || !t->running)
			continue;
		if (now >= t->next_fire_ms) {
			t->next_fire_ms = now + t->period_ms;
			if (t->function)
				t->function(t->arg);
		}
	}

	/* 3. Fire expired timed events */
	for (i = 0; i < MAX_EVENTS; i++) {
		struct k_event *e = &g_events[i];
		if (!e->used || !e->active)
			continue;
		if (now >= e->next_fire_ms) {
			e->next_fire_ms = now + e->period_ms;
			if (e->function)
				e->function(e->arg);
		}
	}
}

/* ── Stub for wwd_FreeRTOS_systick.h reference ────────── */
/* The WICED WWD layer references a systick function.
 * We map it to our basic_timer. */

uint32_t host_rtos_get_systick_count(void)
{
	return (uint32_t)basic_timer_get_ms();
}

void host_rtos_systick_delay(uint32_t ms)
{
	mdelay(ms);
}
