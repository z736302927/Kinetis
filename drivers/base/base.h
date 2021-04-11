/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2001-2003 Patrick Mochel <mochel@osdl.org>
 * Copyright (c) 2004-2009 Greg Kroah-Hartman <gregkh@suse.de>
 * Copyright (c) 2008-2012 Novell Inc.
 * Copyright (c) 2012-2019 Greg Kroah-Hartman <gregkh@linuxfoundation.org>
 * Copyright (c) 2012-2019 Linux Foundation
 *
 * Core driver model functions and structures that should not be
 * shared outside of the drivers/base/ directory.
 *
 */
#include <linux/klist.h>
#include <linux/device.h>

/**
 * struct subsys_private - structure to hold the private to the driver core portions of the bus_type/class structure.
 *
 * @subsys - the struct kset that defines this subsystem
 * @devices_kset - the subsystem's 'devices' directory
 * @interfaces - list of subsystem interfaces associated
 * @mutex - protect the devices, and interfaces lists.
 *
 * @drivers_kset - the list of drivers associated
 * @klist_devices - the klist to iterate over the @devices_kset
 * @klist_drivers - the klist to iterate over the @drivers_kset
 * @bus_notifier - the bus notifier list for anything that cares about things
 *                 on this bus.
 * @bus - pointer back to the struct bus_type that this structure is associated
 *        with.
 *
 * @glue_dirs - "glue" directory to put in-between the parent device to
 *              avoid namespace conflicts
 * @class - pointer back to the struct class that this structure is associated
 *          with.
 *
 * This structure is the one that is the actual kobject allowing struct
 * bus_type/class to be statically allocated safely.  Nothing outside of the
 * driver core should ever touch these fields.
 */
struct subsys_private {
	struct list_head interfaces;

	struct klist klist_devices;
	struct klist klist_drivers;
	unsigned int drivers_autoprobe:1;
	struct bus_type *bus;

	struct class *class;
};

struct driver_private {
	struct klist klist_devices;
	struct klist_node knode_bus;
	struct device_driver *driver;
};
#define to_driver(obj) container_of(obj, struct driver_private, kobj)

/* initialisation functions */
extern int devices_init(void);
extern int buses_init(void);
extern int classes_init(void);
extern int firmware_init(void);
#ifdef CONFIG_SYS_HYPERVISOR
extern int hypervisor_init(void);
#else
static inline int hypervisor_init(void) { return 0; }
#endif
extern int platform_bus_init(void);
extern void cpu_dev_init(void);
extern void container_dev_init(void);
#ifdef CONFIG_AUXILIARY_BUS
extern void auxiliary_bus_init(void);
#else
static inline void auxiliary_bus_init(void) { }
#endif

struct kobject *virtual_device_parent(struct device *dev);

extern int bus_add_device(struct device *dev);
extern void bus_probe_device(struct device *dev);
extern void bus_remove_device(struct device *dev);

extern int bus_add_driver(struct device_driver *drv);
extern void bus_remove_driver(struct device_driver *drv);
extern void device_release_driver_internal(struct device *dev,
					   struct device_driver *drv,
					   struct device *parent);

extern void driver_detach(struct device_driver *drv);
extern void driver_deferred_probe_del(struct device *dev);
extern void device_set_deferred_probe_reason(const struct device *dev,
					     struct va_format *vaf);
static inline int driver_match_device(struct device_driver *drv,
				      struct device *dev)
{
	return drv->bus->match ? drv->bus->match(dev, drv) : 1;
}
extern bool driver_allows_async_probing(struct device_driver *drv);

int device_driver_attach(struct device_driver *drv, struct device *dev);
void device_driver_detach(struct device *dev);

extern char *make_class_name(const char *name, struct kobject *kobj);

extern int devres_release_all(struct device *dev);
extern void device_block_probing(void);
extern void device_unblock_probing(void);

/* /sys/devices directory */
extern struct kset *devices_kset;
extern void devices_kset_move_last(struct device *dev);

#if defined(CONFIG_MODULES) && defined(CONFIG_SYSFS)
extern void module_add_driver(struct module *mod, struct device_driver *drv);
extern void module_remove_driver(struct device_driver *drv);
#else
static inline void module_add_driver(void *mod,
				     struct device_driver *drv) { }
static inline void module_remove_driver(struct device_driver *drv) { }
#endif

#ifdef CONFIG_DEVTMPFS
extern int devtmpfs_init(void);
#else
static inline int devtmpfs_init(void) { return 0; }
#endif

/* Device links support */
extern int device_links_read_lock(void);
extern void device_links_read_unlock(int idx);
extern int device_links_read_lock_held(void);
extern int device_links_check_suppliers(struct device *dev);
extern void device_links_driver_bound(struct device *dev);
extern void device_links_driver_cleanup(struct device *dev);
extern void device_links_no_driver(struct device *dev);
extern bool device_links_busy(struct device *dev);
extern void device_links_unbind_consumers(struct device *dev);

/* device pm support */
void device_pm_move_to_tail(struct device *dev);

#ifdef CONFIG_DEVTMPFS
int devtmpfs_create_node(struct device *dev);
int devtmpfs_delete_node(struct device *dev);
#else
static inline int devtmpfs_create_node(struct device *dev) { return 0; }
static inline int devtmpfs_delete_node(struct device *dev) { return 0; }
#endif
