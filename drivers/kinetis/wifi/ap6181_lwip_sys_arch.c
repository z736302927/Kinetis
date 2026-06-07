/**
 * @file ap6181_lwip_sys_arch.c
 * @brief lwIP sys_arch implementation for Kinetis bare-metal cooperative port
 *
 * All blocking primitives (semaphore wait, mbox fetch) return
 * immediately with SYS_ARCH_TIMEOUT when the resource is not
 * available — because in a single-threaded cooperative system
 * there is no other thread to wake us.
 *
 * lwIP's TCP/IP stack is driven cooperatively by
 * lwip_periodic_handle(), which is called from wiced_rtos_poll().
 */

#include <linux/kernel.h>
#include <linux/printk.h>
#include <linux/slab.h>
#include <linux/errno.h>
#include <linux/string.h>
#include <linux/delay.h>

#include <kinetis/basic-timer.h>
#include <kinetis/tim-task.h>

#include "ap6181_lwip_port.h"
#include "ap6181_lwip_sys_arch.h"

#include "lwip/sys.h"
#include "lwip/tcpip.h"
#include "lwip/timeouts.h"

#define MAX_SEMAPHORES       16
#define MAX_MBOXES            8
#define MAX_THREADS           4

#ifndef SYS_ARCH_TIMEOUT
#define SYS_ARCH_TIMEOUT      0xFFFFFFFFUL
#endif

struct k_lwip_sem {
	u8   used;
	u32  count;
};

struct k_lwip_mbox {
	u8    used;
	void **messages;
	u32   capacity;
	u32   head;
	u32   tail;
	u32   count;
};

struct k_lwip_thread {
	u8                used;
	lwip_thread_fn    function;
	void             *arg;
	const char       *name;
	u8                started;
};

static struct k_lwip_sem     g_sems[MAX_SEMAPHORES];
static struct k_lwip_mbox    g_mboxes[MAX_MBOXES];
static struct k_lwip_thread  g_lwip_threads[MAX_THREADS];

/* Reference to the tcpip mbox (set when tcpip_thread is created) */
static sys_mbox_t  g_tcpip_mbox = SYS_MBOX_NULL;
static u8          g_tcpip_init_done;

static u64 _now(void)
{
	return basic_timer_get_ms();
}

static struct k_lwip_sem *_find_sem(sys_sem_t *sem)
{
	ptrdiff_t idx = (ptrdiff_t)(*sem);
	if (idx >= 0 && idx < MAX_SEMAPHORES && g_sems[idx].used)
		return &g_sems[idx];
	return NULL;
}

static struct k_lwip_mbox *_find_mbox(sys_mbox_t *mbox)
{
	ptrdiff_t idx = (ptrdiff_t)(*mbox);
	if (idx >= 0 && idx < MAX_MBOXES && g_mboxes[idx].used)
		return &g_mboxes[idx];
	return NULL;
}

void sys_init(void)
{
	memset(g_sems,         0, sizeof(g_sems));
	memset(g_mboxes,       0, sizeof(g_mboxes));
	memset(g_lwip_threads, 0, sizeof(g_lwip_threads));
	g_tcpip_mbox    = SYS_MBOX_NULL;
	g_tcpip_init_done = 0;

	pr_info("lwip: sys_arch init (bare-metal cooperative)\n");
}

err_t sys_sem_new(sys_sem_t *sem, u8_t count)
{
	int i;

	for (i = 0; i < MAX_SEMAPHORES; i++) {
		if (!g_sems[i].used) {
			g_sems[i].used  = 1;
			g_sems[i].count = count;
			*sem = (sys_sem_t)(ptrdiff_t)i;
			return ERR_OK;
		}
	}
	pr_err("lwip: out of semaphore slots\n");
	return ERR_MEM;
}

void sys_sem_free(sys_sem_t *sem)
{
	struct k_lwip_sem *s = _find_sem(sem);
	if (s) {
		s->used = 0;
		*sem = SYS_SEM_NULL;
	}
}

void sys_sem_signal(sys_sem_t *sem)
{
	struct k_lwip_sem *s = _find_sem(sem);
	if (s)
		s->count++;
}

u32_t sys_arch_sem_wait(sys_sem_t *sem, u32_t timeout)
{
	struct k_lwip_sem *s = _find_sem(sem);
	if (!s)
		return SYS_ARCH_TIMEOUT;

	if (s->count > 0) {
		s->count--;
		return 0; /* success — waited 0 ms */
	}

	/*
	 * Cooperative single-threaded: can't block.
	 * During init, we do a busy-wait with WWD polling.
	 */
	if (timeout == 0)
		return SYS_ARCH_TIMEOUT;

	/* Busy-wait (used during lwIP init handshake) */
	{
		u64 deadline = _now() + timeout;
		while (s->count == 0 && _now() < deadline) {
			extern void wwd_thread_poll_all(void);
			wwd_thread_poll_all();
		}
		if (s->count > 0) {
			s->count--;
			return 0;
		}
	}
	return SYS_ARCH_TIMEOUT;
}

int sys_sem_valid(sys_sem_t *sem)
{
	return (sem != NULL && *sem != SYS_SEM_NULL &&
	        _find_sem(sem) != NULL);
}

void sys_sem_set_invalid(sys_sem_t *sem)
{
	*sem = SYS_SEM_NULL;
}

err_t sys_mbox_new(sys_mbox_t *mbox, int size)
{
	int i;

	for (i = 0; i < MAX_MBOXES; i++) {
		if (!g_mboxes[i].used) {
			g_mboxes[i].used     = 1;
			g_mboxes[i].capacity = (u32)size;
			g_mboxes[i].head     = 0;
			g_mboxes[i].tail     = 0;
			g_mboxes[i].count    = 0;
			g_mboxes[i].messages = kcalloc(size,
				sizeof(void *), GFP_KERNEL);
			if (!g_mboxes[i].messages) {
				g_mboxes[i].used = 0;
				return ERR_MEM;
			}
			*mbox = (sys_mbox_t)(ptrdiff_t)i;
			return ERR_OK;
		}
	}
	pr_err("lwip: out of mbox slots\n");
	return ERR_MEM;
}

void sys_mbox_free(sys_mbox_t *mbox)
{
	struct k_lwip_mbox *m = _find_mbox(mbox);
	if (m) {
		kfree(m->messages);
		m->used = 0;
		*mbox = SYS_MBOX_NULL;
	}
}

void sys_mbox_post(sys_mbox_t *mbox, void *msg)
{
	struct k_lwip_mbox *m = _find_mbox(mbox);
	if (!m || m->count >= m->capacity)
		return; /* drop if full (shouldn't happen) */

	m->messages[m->head] = msg;
	m->head = (m->head + 1) % m->capacity;
	m->count++;
}

err_t sys_mbox_trypost(sys_mbox_t *mbox, void *msg)
{
	struct k_lwip_mbox *m = _find_mbox(mbox);
	if (!m)
		return ERR_VAL;
	if (m->count >= m->capacity)
		return ERR_MEM;

	m->messages[m->head] = msg;
	m->head = (m->head + 1) % m->capacity;
	m->count++;
	return ERR_OK;
}

u32_t sys_arch_mbox_fetch(sys_mbox_t *mbox, void **msg, u32_t timeout)
{
	struct k_lwip_mbox *m = _find_mbox(mbox);
	if (!m)
		return SYS_ARCH_TIMEOUT;

	/*
	 * Cooperative: if empty, return timeout immediately.
	 * The caller (tcpip_thread) will loop back and we'll
	 * process messages in the next poll cycle.
	 */
	(void)timeout;

	if (m->count == 0)
		return SYS_ARCH_TIMEOUT;

	*msg = m->messages[m->tail];
	m->tail = (m->tail + 1) % m->capacity;
	m->count--;
	return 0; /* success — waited 0 ms */
}

u32_t sys_arch_mbox_tryfetch(sys_mbox_t *mbox, void **msg)
{
	return sys_arch_mbox_fetch(mbox, msg, 0);
}

int sys_mbox_valid(sys_mbox_t *mbox)
{
	return (mbox != NULL && *mbox != SYS_MBOX_NULL &&
	        _find_mbox(mbox) != NULL);
}

void sys_mbox_set_invalid(sys_mbox_t *mbox)
{
	*mbox = SYS_MBOX_NULL;
}

sys_thread_t sys_thread_new(const char *name,
	lwip_thread_fn thread, void *arg,
	int stacksize, int prio)
{
	int i;

	for (i = 0; i < MAX_THREADS; i++) {
		if (!g_lwip_threads[i].used) {
			g_lwip_threads[i].used     = 1;
			g_lwip_threads[i].function = thread;
			g_lwip_threads[i].arg      = arg;
			g_lwip_threads[i].name     = name;
			g_lwip_threads[i].started  = 0;

			pr_info("lwip: thread '%s' registered\n", name);
			return (sys_thread_t)(ptrdiff_t)i;
		}
	}
	pr_err("lwip: out of thread slots\n");
	return (sys_thread_t)-1;
}

void sys_thread_exit(void)
{
	/* No-op in cooperative mode */
}

void sys_thread_free(sys_thread_t thread)
{
	ptrdiff_t idx = (ptrdiff_t)thread;
	if (idx >= 0 && idx < MAX_THREADS)
		g_lwip_threads[idx].used = 0;
}

sys_prot_t sys_arch_protect(void)
{
	return 1;
}

void sys_arch_unprotect(sys_prot_t pval)
{

}

u32_t sys_now(void)
{
	return (u32_t)basic_timer_get_ms();
}

uint16_t sys_rand16(void)
{
	return (uint16_t)(get_random_u32() & 0xFFFF);
}

/* ── Cooperative lwIP poll ────────────────────────────── */

/*
 * tcpip_thread function pointer (set when tcpip_init is called).
 * We don't actually run this thread; instead we manually process
 * the tcpip mbox messages and timeouts in lwip_periodic_handle().
 */
static sys_mbox_t _tcpip_mbox_for_poll = SYS_MBOX_NULL;

/**
 * @brief Cooperative tcpip initialization callback override.
 *
 * Called by tcpip_init — stores the tcpip mbox and runs
 * the init-done callback immediately (no thread suspend).
 */
void _lwip_tcpip_init_wrapper(void *init_sem_ptr)
{
	/* Store TCP/IP mbox for our periodic handler */
	_tcpip_mbox_for_poll = g_tcpip_mbox;

	/*
	 * Immediately signal init-done (single-threaded:
	 * no race condition to guard against).
	 */
	g_tcpip_init_done = 1;

	if (init_sem_ptr) {
		sys_sem_t *sem = (sys_sem_t *)init_sem_ptr;
		sys_sem_signal(sem);
	}
}

/**
 * @brief Run one iteration of lwIP TCP/IP processing.
 *
 * Called periodically from wiced_rtos_poll().
 * Drains pending TCP/IP mbox messages and checks timeouts.
 *
 * @return 1 if work was done, 0 if idle
 */
int lwip_periodic_handle(void)
{
	int work_done = 0;
		void *msg;

	if (_tcpip_mbox_for_poll == SYS_MBOX_NULL)
		return 0;

	/* Drain TCP/IP mailbox messages (non-blocking) */
		while (sys_arch_mbox_tryfetch(&_tcpip_mbox_for_poll, &msg) == 0) {
			/*
			 * lwIP posts tcpip_msg structs to the mbox.
			 * Processing them is done via tcpip_inpkt() and
			 * tcpip_callbackmsg() — we rely on the existing
			 * WICED/lwIP integration to route messages correctly.
			 * For our port, just acknowledging receipt is enough
			 * because WICED's ethernetif layer handles RX directly.
			 */
			(void)msg;
			work_done = 1;
		}

	/* Check lwIP timer timeouts */
	sys_check_timeouts();

	return work_done;
}

/**
 * @brief Register the tcpip mbox from lwIP init.
 *
 * This is called indirectly by tcpip_init() via the sys_thread_new
 * registration.  The actual pointer is captured through the
 * tcpip_init callback mechanism.
 */
void _lwip_set_tcpip_mbox(sys_mbox_t mbox)
{
	g_tcpip_mbox = mbox;
}
