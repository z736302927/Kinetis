// SPDX-License-Identifier: GPL-2.0
/*
 * platform.c - platform 'pseudo' bus for legacy devices
 *
 * Copyright (c) 2002-3 Patrick Mochel
 * Copyright (c) 2002-3 Open Source Development Labs
 *
 * Please see Documentation/driver-api/driver-model/platform.rst for more
 * information.
 */

#include <linux/string.h>
#include <linux/platform_device.h>
#include <linux/init.h>
#include <linux/interrupt.h>
//#include <linux/ioport.h>
#include <linux/err.h>
#include <linux/slab.h>
//#include <linux/idr.h>
#include <linux/limits.h>
#include <linux/types.h>

//#include "base.h"

///* For automatically allocated device IDs */
//static DEFINE_IDA(platform_devid_ida);

struct device platform_bus = {
	.init_name	= "platform",
};
EXPORT_SYMBOL_GPL(platform_bus);

struct platform_object {
	struct platform_device pdev;
	char name[];
};

/**
 * platform_match - bind platform device to platform driver.
 * @dev: device.
 * @drv: driver.
 *
 * Platform device IDs are assumed to be encoded like this:
 * "<name><instance>", where <name> is a short description of the type of
 * device, like "pci" or "floppy", and <instance> is the enumerated
 * instance of the device, like '0' or '42'.  Driver IDs are simply
 * "<name>".  So, extract the <name> from the platform_device structure,
 * and compare it against the name of the driver. Return whether they match
 * or not.
 */
static int platform_match(struct platform_device *pdev, struct platform_driver *pdrv)
{
	/* When driver_override is set, only bind to the matching driver */
	if (pdev->driver_override)
		return !strcmp(pdev->driver_override, pdrv->driver.name);

//	/* Then try to match against the id table */
//	if (pdrv->id_table)
//		return platform_match_id(pdrv->id_table, pdev) != NULL;

	/* fall-back to driver name match */
	return (strcmp(pdev->name, pdrv->driver.name) == 0);
}

static LIST_HEAD(platform_driver_head);

/**
 * platform_driver_register - register a driver for platform-level devices
 * @drv: platform driver structure
 * @owner: owning module/driver
 */
int platform_driver_register(struct platform_driver *drv)
{
	list_add_tail(&drv->list, &platform_driver_head);
	return 0;
}
EXPORT_SYMBOL_GPL(platform_driver_register);

/**
 * platform_driver_unregister - unregister a driver for platform-level devices
 * @drv: platform driver structure
 */
void platform_driver_unregister(struct platform_driver *drv)
{
	list_del(&drv->list);
}
EXPORT_SYMBOL_GPL(platform_driver_unregister);

/**
 * platform_device_add_data - add platform-specific data to a platform device
 * @pdev: platform device allocated by platform_device_alloc to add resources to
 * @data: platform specific data for this platform device
 * @size: size of platform specific data
 *
 * Add a copy of platform specific data to the platform device's
 * platform_data pointer.  The memory associated with the platform data
 * will be freed when the platform device is released.
 */
int platform_device_add_data(struct platform_device *pdev, const void *data,
			     size_t size)
{
	void *d = NULL;

	if (data) {
		d = kmemdup(data, size, GFP_KERNEL);
		if (!d)
			return -ENOMEM;
	}

	kfree(pdev->dev.platform_data);
	pdev->dev.platform_data = d;
	return 0;
}
EXPORT_SYMBOL_GPL(platform_device_add_data);

static LIST_HEAD(platform_device_head);

/**
 * platform_device_add - add a platform device to device hierarchy
 * @pdev: platform device we're adding
 *
 * This is part 2 of platform_device_register(), though may be called
 * separately _iff_ pdev was allocated by platform_device_alloc().
 */
int platform_device_add(struct platform_device *pdev)
{
    struct platform_driver *pdrv;
    bool find_pdrv = false;
	u32 i;
	int ret;

	if (!pdev)
		return -EINVAL;

	if (!pdev->dev.parent)
		pdev->dev.parent = &platform_bus;

	switch (pdev->id) {
	default:
		dev_set_name(&pdev->dev, "%s.%d", pdev->name,  pdev->id);
		break;
	case PLATFORM_DEVID_NONE:
		dev_set_name(&pdev->dev, "%s", pdev->name);
		break;
//	case PLATFORM_DEVID_AUTO:
//		/*
//		 * Automatically allocated device ID. We mark it as such so
//		 * that we remember it must be freed, and we append a suffix
//		 * to avoid namespace collision with explicit IDs.
//		 */
//		ret = ida_alloc(&platform_devid_ida, GFP_KERNEL);
//		if (ret < 0)
//			goto err_out;
//		pdev->id = ret;
//		pdev->id_auto = true;
//		dev_set_name(&pdev->dev, "%s.%d.auto", pdev->name, pdev->id);
//		break;
	}

//	for (i = 0; i < pdev->num_resources; i++) {
//		struct resource *p, *r = &pdev->resource[i];
//
//		if (r->name == NULL)
//			r->name = dev_name(&pdev->dev);
//
//		p = r->parent;
//		if (!p) {
//			if (resource_type(r) == IORESOURCE_MEM)
//				p = &iomem_resource;
//			else if (resource_type(r) == IORESOURCE_IO)
//				p = &ioport_resource;
//		}
//
//		if (p) {
//			ret = insert_resource(p, r);
//			if (ret) {
//				dev_err(&pdev->dev, "failed to claim resource %d: %pR\n", i, r);
//				goto failed;
//			}
//		}
//	}

	pr_debug("Registering platform device '%s'. Parent at %s\n",
		 dev_name(&pdev->dev), dev_name(pdev->dev.parent));
    
    list_for_each_entry(pdrv, &platform_driver_head, list) {
        if (!platform_match(pdev, pdrv)) {
            find_pdrv = true;
            break;
        }

    }

    if (find_pdrv) {
        ret = pdrv->probe(pdev);
        if (ret)
            return ret;
    } else
        return -ENODATA;
        
    list_add_tail(&pdev->list, &platform_device_head);

// failed:
//	if (pdev->id_auto) {
//		ida_free(&platform_devid_ida, pdev->id);
//		pdev->id = PLATFORM_DEVID_AUTO;
//	}
//
//	while (i--) {
//		struct resource *r = &pdev->resource[i];
//		if (r->parent)
//			release_resource(r);
//	}

	return 0;
}
EXPORT_SYMBOL_GPL(platform_device_add);

/**
 * platform_device_del - remove a platform-level device
 * @pdev: platform device we're removing
 *
 * Note that this function will also release all memory- and port-based
 * resources owned by the device (@dev->resource).  This function must
 * _only_ be externally called in error cases.  All other usage is a bug.
 */
void platform_device_del(struct platform_device *pdev)
{
	u32 i;

	if (!IS_ERR_OR_NULL(pdev)) {
	    list_del(&pdev->list);
//		if (pdev->id_auto) {
//			ida_free(&platform_devid_ida, pdev->id);
//			pdev->id = PLATFORM_DEVID_AUTO;
//		}
//
//		for (i = 0; i < pdev->num_resources; i++) {
//			struct resource *r = &pdev->resource[i];
//			if (r->parent)
//				release_resource(r);
//		}
	}
}
EXPORT_SYMBOL_GPL(platform_device_del);

/**
 * platform_device_register - add a platform-level device
 * @pdev: platform device we're adding
 */
int platform_device_register(struct platform_device *pdev)
{
	return platform_device_add(pdev);
}
EXPORT_SYMBOL_GPL(platform_device_register);

/**
 * platform_device_unregister - unregister a platform-level device
 * @pdev: platform device we're unregistering
 *
 * Unregistration is done in 2 steps. First we release all resources
 * and remove it from the subsystem, then we drop reference count by
 * calling platform_device_put().
 */
void platform_device_unregister(struct platform_device *pdev)
{
	platform_device_del(pdev);
}
EXPORT_SYMBOL_GPL(platform_device_unregister);

