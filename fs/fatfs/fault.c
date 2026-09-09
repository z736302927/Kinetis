#include <linux/errno.h>
#include <linux/slab.h>

#include "fault.h"

int disk_fault_should_fail(const Diskio_drvTypeDef *drv,
	enum disk_fault_operation operation)
{
	uint32_t value;

	if (!drv || !drv->fault)
		return 0;

	value = drv->fault->seed;

	if (drv->fault->operation != operation ||
	    drv->fault->remaining <= 0 ||
	    pthread_equal(drv->fault->owner, pthread_self()) == 0)
		return 0;

	if (!value)
		value = 1u;

	value ^= value << 13;
	value ^= value >> 17;
	value ^= value << 5;
	drv->fault->seed = value;

	if (drv->fault->rate_per_mille <= value % 1000u)
		return 0;

	drv->fault->remaining--;
	drv->fault->injected++;

	return 1;
}

int disk_fault_alloc(Diskio_drvTypeDef *drv, enum disk_fault_operation operation,
	uint16_t rate_per_mille, uint16_t count, uint32_t seed, uint8_t enable_ro)
{
	if (!drv)
		return -EINVAL;

	drv->fault = kmalloc(sizeof(*drv->fault), GFP_KERNEL);
	if (!drv->fault)
		return -ENOMEM;

	drv->fault->owner = pthread_self();
	drv->fault->seed = seed ? seed : 1;
	drv->fault->rate_per_mille = min(rate_per_mille, 1000u);
	drv->fault->remaining = count;
	drv->fault->injected = 0;
	drv->fault->operation = operation;
	drv->fault->enable_ro = !!enable_ro;
	drv->fault->checks = 0;

	return 0;
}

void disk_fault_free(Diskio_drvTypeDef *drv)
{
	if (!drv || !drv->fault)
		return;

	kfree(drv->fault);
	drv->fault = NULL;
}

uint16_t disk_fault_is_injected(Diskio_drvTypeDef *drv)
{
	if (!drv || !drv->fault)
		return 0;

	return drv->fault->injected;
}

uint16_t disk_fault_get_ro_checks(Diskio_drvTypeDef *drv)
{
	if (!drv || !drv->fault)
		return 0;

	return drv->fault->checks;
}

int disk_fault_is_read_only(Diskio_drvTypeDef *drv)
{
	if (!drv || !drv->fault)
		return 0;

	if (!drv->fault->enable_ro ||
	    !pthread_equal(drv->fault->owner, pthread_self()))
		return 0;

	drv->fault->checks++;

	return 1;
}