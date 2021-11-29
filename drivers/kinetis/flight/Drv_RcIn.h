#ifndef _DRV_RCIN_H_
#define _DRV_RCIN_H_
#include "include.h"
//
void rc_sbus_init(void);
void Sbus_IRQH(void);
void rc_ppm_init(void);
void PPM_IRQH(void);
#endif
