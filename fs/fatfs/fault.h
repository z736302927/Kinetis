#ifndef _FAULT_DEFINED
#define _FAULT_DEFINED

#ifdef __cplusplus
extern "C" {
#endif

#include "ff_gen_drv.h"

#include <stdint.h>
#if KINETIS_FAKE_SIM
#include <pthread.h>
#endif

enum disk_fault_operation {
	DISK_FAULT_NONE = 0,
	DISK_FAULT_READ,
	DISK_FAULT_WRITE,
};

struct disk_fault_state {
	enum disk_fault_operation operation;
	uint16_t rate_per_mille;
	uint16_t remaining;
	uint32_t seed;
	uint16_t injected;
	uint16_t checks;
	uint8_t enable_ro;
#if KINETIS_FAKE_SIM
	pthread_t owner;
#endif
};

typedef struct Diskio_drvTypeDef Diskio_drvTypeDef;

int disk_fault_should_fail(const Diskio_drvTypeDef *drv,
	enum disk_fault_operation operation);
int disk_fault_alloc(Diskio_drvTypeDef *drv,
	enum disk_fault_operation operation,
	uint16_t rate_per_mille, uint16_t count, uint32_t seed,	uint8_t enable_ro);
void disk_fault_free(Diskio_drvTypeDef *drv);
uint16_t disk_fault_is_injected(Diskio_drvTypeDef *drv);
uint16_t disk_fault_get_ro_checks(Diskio_drvTypeDef *drv);
int disk_fault_is_read_only(Diskio_drvTypeDef *drv);

#ifdef __cplusplus
}
#endif

#endif