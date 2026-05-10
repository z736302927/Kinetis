#ifndef POV_MOTOR_H
#define POV_MOTOR_H

#include <linux/types.h>

#if KINETIS_FAKE_SIM
#include <pthread.h>
#endif

struct motor_controller {
	u16 pwm_duty;            /* 0~10000 = 0.00%~100.00% */
	u8 direction;
	s32 measured_rpm;
	u8 status;           /* 0=stopped, 1=running, 2=fault */

#if KINETIS_FAKE_SIM
	u8 thread_running;
	pthread_t thread;
#endif
};

int motor_set_pwm(struct motor_controller *motor, u16 duty);
int motor_set_direction(struct motor_controller *motor, u8 forward);
void motor_stop(struct motor_controller *motor);

#if KINETIS_FAKE_SIM
int motor_sim_start(struct motor_controller *motor);
void motor_sim_stop(struct motor_controller *motor);
#endif

#endif
