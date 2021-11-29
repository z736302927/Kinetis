#ifndef _PWM_OUT_H_
#define _PWM_OUT_H_

#include "stm32f4xx.h"

u8 pwm_out_init(void);
void SetPwm(int16_t pwm[]);

#endif

