/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * platform_device.h - generic, centralized driver model
 *
 * Copyright (c) 2001-2003 Patrick Mochel <mochel@osdl.org>
 *
 * See Documentation/driver-api/driver-model/ for more information.
 */

#ifndef _PLATFORM_DEVICE_H_
#define _PLATFORM_DEVICE_H_

#include <linux/device.h>
#include <linux/interrupt.h>

#define PLATFORM_DEVID_NONE	(-1)
#define PLATFORM_DEVID_AUTO	(-2)

struct platform_device_id;

struct platform_device {
	const char	*name;
	int		id;
	bool		id_auto;
	struct device	dev;
	u64		platform_dma_mask;
	u32		num_resources;

	const struct platform_device_id	*id_entry;
	char *driver_override; /* Driver name to force a match */
    struct list_head list;
};

#define to_platform_device(x) container_of((x), struct platform_device, dev)

extern int platform_device_register(struct platform_device *);
extern void platform_device_unregister(struct platform_device *);

struct platform_device_info {
		struct device *parent;
		struct fwnode_handle *fwnode;
		bool of_node_reused;

		const char *name;
		int id;

		unsigned int num_res;

		const void *data;
		size_t size_data;
		u64 dma_mask;
};
extern struct platform_device *platform_device_register_full(
		const struct platform_device_info *pdevinfo);

typedef struct pm_message {
	int event;
} pm_message_t;

struct platform_driver {
	int (*probe)(struct platform_device *);
	int (*remove)(struct platform_device *);
	void (*shutdown)(struct platform_device *);
	int (*suspend)(struct platform_device *, pm_message_t state);
	int (*resume)(struct platform_device *);
	struct device_driver driver;
	const struct platform_device_id *id_table;
	bool prevent_deferred_probe;
    irq_handler_t handler[4];
    struct list_head list;
};

#define to_platform_driver(drv)	(container_of((drv), struct platform_driver, \
				 driver))

/*
 * use a macro to avoid include chaining to get THIS_MODULE
 */
extern int platform_driver_register(struct platform_driver *);
extern void platform_driver_unregister(struct platform_driver *);

static inline void *platform_get_drvdata(const struct platform_device *pdev)
{
	return dev_get_drvdata(&pdev->dev);
}

static inline void platform_set_drvdata(struct platform_device *pdev,
					void *data)
{
	dev_set_drvdata(&pdev->dev, data);
}

/* module_platform_driver() - Helper macro for drivers that don't do
 * anything special in module init/exit.  This eliminates a lot of
 * boilerplate.  Each module may only use this macro once, and
 * calling it replaces module_init() and module_exit()
 */
#define module_platform_driver(__platform_driver) \
	module_driver(__platform_driver, platform_driver_register, \
			platform_driver_unregister)

/* builtin_platform_driver() - Helper macro for builtin drivers that
 * don't do anything special in driver init.  This eliminates some
 * boilerplate.  Each driver may only use this macro once, and
 * calling it replaces device_initcall().  Note this is meant to be
 * a parallel of module_platform_driver() above, but w/o _exit stuff.
 */
#define builtin_platform_driver(__platform_driver) \
	builtin_driver(__platform_driver, platform_driver_register)

/* module_platform_driver_probe() - Helper macro for drivers that don't do
 * anything special in module init/exit.  This eliminates a lot of
 * boilerplate.  Each module may only use this macro once, and
 * calling it replaces module_init() and module_exit()
 */
#define module_platform_driver_probe(__platform_driver, __platform_probe) \
static int __init __platform_driver##_init(void) \
{ \
	return platform_driver_probe(&(__platform_driver), \
				     __platform_probe);    \
} \
module_init(__platform_driver##_init); \
static void __exit __platform_driver##_exit(void) \
{ \
	platform_driver_unregister(&(__platform_driver)); \
} \
module_exit(__platform_driver##_exit);

/* builtin_platform_driver_probe() - Helper macro for drivers that don't do
 * anything special in device init.  This eliminates some boilerplate.  Each
 * driver may only use this macro once, and using it replaces device_initcall.
 * This is meant to be a parallel of module_platform_driver_probe above, but
 * without the __exit parts.
 */
#define builtin_platform_driver_probe(__platform_driver, __platform_probe) \
static int __init __platform_driver##_init(void) \
{ \
	return platform_driver_probe(&(__platform_driver), \
				     __platform_probe);    \
} \
device_initcall(__platform_driver##_init); \


#define platform_register_drivers(drivers, count) \
	__platform_register_drivers(drivers, count, THIS_MODULE)

#ifdef CONFIG_SUSPEND
extern int platform_pm_suspend(struct device *dev);
extern int platform_pm_resume(struct device *dev);
#else
#define platform_pm_suspend		NULL
#define platform_pm_resume		NULL
#endif

#ifdef CONFIG_HIBERNATE_CALLBACKS
extern int platform_pm_freeze(struct device *dev);
extern int platform_pm_thaw(struct device *dev);
extern int platform_pm_poweroff(struct device *dev);
extern int platform_pm_restore(struct device *dev);
#else
#define platform_pm_freeze		NULL
#define platform_pm_thaw		NULL
#define platform_pm_poweroff		NULL
#define platform_pm_restore		NULL
#endif

extern int platform_dma_configure(struct device *dev);

#ifdef CONFIG_PM_SLEEP
#define USE_PLATFORM_PM_SLEEP_OPS \
	.suspend = platform_pm_suspend, \
	.resume = platform_pm_resume, \
	.freeze = platform_pm_freeze, \
	.thaw = platform_pm_thaw, \
	.poweroff = platform_pm_poweroff, \
	.restore = platform_pm_restore,
#else
#define USE_PLATFORM_PM_SLEEP_OPS
#endif

#ifndef CONFIG_SUPERH
/*
 * REVISIT: This stub is needed for all non-SuperH users of early platform
 * drivers. It should go away once we introduce the new platform_device-based
 * early driver framework.
 */
static inline int is_sh_early_platform_device(struct platform_device *pdev)
{
	return 0;
}
#endif /* CONFIG_SUPERH */

#endif /* _PLATFORM_DEVICE_H_ */
