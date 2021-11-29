#ifndef _DRV_BSP_H_
#define _DRV_BSP_H_

#include <generated/deconfig.h>
#include <linux/types.h>

struct rc_input_state {
	u8 signal_mode;
#define SIG_NULL	0
#define SIG_PPM		1
#define SIG_SBUS	2

	s16 ppm_ch[9];
	s16 sbus_ch[16];
	u8 sbus_flag;
	u16 signal_fre;
	u8 no_signal;
	u8 fail_safe;

	s16 ch[10];
#define CH_1_ROL	0
#define CH_2_PIT	1
#define CH_3_THR	2
#define CH_4_YAW	3
#define CH_5_AUX1	4
#define CH_6_AUX2	5
#define CH_7_AUX3	6
#define CH_8_AUX4	7
#define CH_9_AUX5	8
#define CH_10_AUX6	9

	u16 signal_cnt_tmp;
	u8 rc_in_mode_tmp;
} __attribute__((__packed__));

void rc_module_init(void);
void rc_input_task(float dt_s);
void rc_ppm_get_byte(u16 data);
void rc_sbus_get_byte(u8 data);

#endif
