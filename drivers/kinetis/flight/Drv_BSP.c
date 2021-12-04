
#include <linux/ktime.h>

#include "Drv_BSP.h"
#include "Drv_RcIn.h"


int rc_module_init(struct rc_input_state *rc, void)
{
	/* The first mark bit is missing */
	rc->no_signal = 1;

	/* Any initialization of a pattern */
	return rc_sbus_init();
}

void rc_sbus_get_byte(struct rc_input_state *rc, u8 data)
{
	/*
	 * as following the structure of sbus flags
	 * flags：
	 * bit7 = ch17 = digital channel (0x80)
	 * bit6 = ch18 = digital channel (0x40)
	 * bit5 = Frame lost, equivalent red LED on receiver (0x20)
	 * bit4 = failsafe activated (0x10) b: 0001 0000
	 * bit3 = n/a
	 * bit2 = n/a
	 * bit1 = n/a
	 * bit0 = n/a
	 */
	const u8 frame_end[4] = {0x04, 0x14, 0x24, 0x34};
	static u32 sbus_time[2];
	static u8 datatmp[25];
	static u8 cnt = 0;
	static u8 frame_cnt;

	sbus_time[0] = sbus_time[1];
	sbus_time[1] = ktime_to_us(ktime_get());

	if ((sbus_time[1] - sbus_time[0]) > 2500)
		cnt = 0;

	datatmp[cnt++] = data;

	if (cnt == 25) {
		cnt = 24;

		if ((datatmp[0] == 0x0F &&
			(datatmp[24] == 0x00 || datatmp[24] == frame_end[frame_cnt]))) {
			cnt = 0;
			rc->sbus_ch[0] = (s16)(datatmp[2] & 0x07) << 8 | datatmp[1];
			rc->sbus_ch[1] = (s16)(datatmp[3] & 0x3f) << 5 | (datatmp[2] >> 3);
			rc->sbus_ch[2] = (s16)(datatmp[5] & 0x01) << 10 | ((s16)datatmp[4] << 2) | (datatmp[3] >> 6);
			rc->sbus_ch[3] = (s16)(datatmp[6] & 0x0F) << 7 | (datatmp[5] >> 1);
			rc->sbus_ch[4] = (s16)(datatmp[7] & 0x7F) << 4 | (datatmp[6] >> 4);
			rc->sbus_ch[5] = (s16)(datatmp[9] & 0x03) << 9 | ((s16)datatmp[8] << 1) | (datatmp[7] >> 7);
			rc->sbus_ch[6] = (s16)(datatmp[10] & 0x1F) << 6 | (datatmp[9] >> 2);
			rc->sbus_ch[7] = (s16)datatmp[11] << 3 | (datatmp[10] >> 5);

			rc->sbus_ch[8] = (s16)(datatmp[13] & 0x07) << 8 | datatmp[12];
			rc->sbus_ch[9] = (s16)(datatmp[14] & 0x3f) << 5 | (datatmp[13] >> 3);
			rc->sbus_ch[10] = (s16)(datatmp[16] & 0x01) << 10 | ((s16)datatmp[15] << 2) | (datatmp[14] >> 6);
			rc->sbus_ch[11] = (s16)(datatmp[17] & 0x0F) << 7 | (datatmp[16] >> 1);
			rc->sbus_ch[12] = (s16)(datatmp[18] & 0x7F) << 4 | (datatmp[17] >> 4);
			rc->sbus_ch[13] = (s16)(datatmp[20] & 0x03) << 9 | ((s16)datatmp[19] << 1) | (datatmp[18] >> 7);
			rc->sbus_ch[14] = (s16)(datatmp[21] & 0x1F) << 6 | (datatmp[20] >> 2);
			rc->sbus_ch[15] = (s16)datatmp[22] << 3 | (datatmp[21] >> 5);
			rc->sbus_flag = datatmp[23];

			if (rc->sbus_flag & 0x08) {
				//如果有数据且能接收到有失控标记，则不处理，转嫁成无数据失控。
			} else {
				rc->signal_cnt_tmp++;
				rc->rc_in_mode_tmp = 2; //切换模式标记为sbus
			}

			//帧尾处理
			frame_cnt++;
			frame_cnt %= 4;
		} else {
			u8 i;
			for (i = 0; i < 24; i++)
				datatmp[i] = datatmp[i + 1];
		}
	}
}

void rc_ppm_get_byte(struct rc_input_state *rc, u16 data)
{
	static u8 ch_sta = 0;

	if ((data > 2500 && ch_sta > 3) || ch_sta == 10) {
		ch_sta = 0;
		rc->signal_cnt_tmp++;
		//切换模式标记为ppm
		rc->rc_in_mode_tmp = 1;
	} else if (data > 300 && data < 3000) {
		//异常的脉冲过滤掉
		rc->ppm_ch[ch_sta] = data;
		ch_sta++;
	}
}

static void signal_check(struct rc_input_state *rc, float *dt_s)
{
	static u8 cnt_tmp;
	static u16 time_dly;
	time_dly += (*dt_s) * 1e3f;

	//==1000ms==
	if (time_dly > 1000) {
		time_dly = 0;
		rc->signal_fre = rc->signal_cnt_tmp;

		//==判断信号是否丢失
		if (rc->signal_fre < 5)
			rc->no_signal = 1;
		else
			rc->no_signal = 0;

		//==判断是否切换输入方式
		if (rc->no_signal) {
			//初始0
			if (rc->signal_mode == 0) {
				cnt_tmp++;
				cnt_tmp %= 2;

				if (cnt_tmp == 1)
					rc_sbus_init();
				else
					rc_ppm_init();
			}
		} else
			rc->signal_mode = rc->rc_in_mode_tmp;

		rc->signal_cnt_tmp = 0;
	}
}

void rc_input_task(struct rc_input_state *rc, float dt_s)
{
	u8 i;

	//信号检测
	signal_check(&dt_s);

	//有信号
	if (rc->no_signal == SIG_NULL) {
		if (rc->signal_mode == SIG_PPM) {
			for (i = 0; i < 10; i++) //注意只有10个通道
				rc->ch[i] = rc->ppm_ch[i];
		} else if (rc->signal_mode == SIG_SBUS) {
			for (i = 0; i < 10; i++) { //注意只有10个通道
				rc->ch[i] = 0.644f * (rc->sbus_ch[i] - 1024) + 1500; //248 --1024 --1800转换到1000-2000
			}
		}

		//检查失控保护设置, 满足设置，标记为失控
		if ((rc->ch[CH_5_AUX1] > 1200 && rc->ch[CH_5_AUX1] < 1400) ||
			(rc->ch[CH_5_AUX1] > 1600 && rc->ch[CH_5_AUX1] < 1800))
			rc->fail_safe = 1;
		else
			rc->fail_safe = 0;
	} else { //无信号
		//失控标记置位
		rc->fail_safe = 1;

		for (i = 0; i < 10; i++)   //注意只有10个通道
			rc->ch[i] = 0;
	}
}
