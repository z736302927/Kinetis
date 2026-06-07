/**
 * @file ap6181_wiced_rtos.h
 * @brief Kinetis RTOS port declarations — polls WiFi/lwIP without an RTOS
 *
 * This header is the entry point used by the Kinetis application layer.
 * It declares wiced_rtos_poll() — the cooperative scheduling entry —
 * and re-maps the printf-family macros that WICED uses.
 */

#ifndef AP6181_WICED_RTOS_H
#define AP6181_WICED_RTOS_H

#include <linux/types.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ── RTOS poll entry ──────────────────────────────────── */

/**
 * @brief Poll all WICED RTOS subsystems.
 *
 * Must be called periodically (every 50-100 ms) from the Rotor FSM
 * main loop.  Drives:
 *   - WWD bus thread (SDIO polling)
 *   - WICED worker thread event queues
 *   - lwIP TCP/IP timer and message processing
 *   - Registered RTOS timers
 */
void wiced_rtos_poll(void);

/**
 * @brief One-time initialization of the RTOS port globals
 */
void wiced_rtos_port_init(void);

/* ── printf remapping (WICED uses WPRINT_xxx macros) ──── */

#ifndef WPRINT_MACRO
#define WPRINT_MACRO
#include <linux/printk.h>
#define WPRINT_APP_INFO(args)    pr_info args
#define WPRINT_APP_ERROR(args)   pr_err args
#define WPRINT_APP_DEBUG(args)   pr_debug args
#endif

#ifdef __cplusplus
}
#endif

#endif /* AP6181_WICED_RTOS_H */
