
#include <generated/deconfig.h>
#include <linux/string.h>
#include <linux/err.h>
#include <linux/bits.h>
#include <linux/printk.h>
#include <linux/ktime.h>

#include <kinetis/fmu.h>

void sbus_get_byte(struct fmu_rc *rc, u8 data)
{
	static u8 buf[25];
	static u8 cnt;
	static ktime_t time;

	if (ktime_us_delta(ktime_get(), time) > 2500 && time) {
		cnt = 0;
	}
	time = ktime_get();

	if (data == 0x0F || cnt == 25) {
		cnt = 0;
	}

	buf[cnt++] = data;

	if (cnt == 25) {
		memcpy(&rc->sbus, buf, sizeof(buf));
		rc->get_data_cnt++;
	}
}

int sbus_get_frame(struct fmu_rc_sbus *sbus)
{
	if (sbus->tail == 0xC0) {
		return -EPIPE;
	}

	if (sbus->head != 0x0F) {
		return -EFAULT;
	}

	if (sbus->flag & SBUS_CH17) {
		pr_debug("ch17 = digital channel");
	}
	if (sbus->flag & SBUS_CH18) {
		pr_debug("ch18 = digital channel");
	}
	if (sbus->flag & SBUS_FRAME_LOST) {
		pr_err("Frame lost, equivalent red LED on receiver");
	}
	if (sbus->flag & SBUS_FAILSAFE) {
		pr_err("failsafe activated");
	}

	return 0;
}

static void rc_detect_signal(struct tim_task *task)
{
	struct fmu_rc *rc = container_of(task, struct fmu_rc, detect_task);

	rc->lost_signal = rc->get_data_cnt < 5 ? true : false;

	if (rc->lost_signal) {
		;
	}

	rc->get_data_cnt = 0;
}

static void rc_process_data(struct tim_task *task)
{
	struct fmu_rc *rc = container_of(task, struct fmu_rc, conv_data_task);

	rc->lost_signal = rc->get_data_cnt < 5 ? true : false;

	if (rc->lost_signal) {
		rc->fail_safe = true;
		rc->channel_rol  = 0;
		rc->channel_pit  = 0;
		rc->channel_thr  = 0;
		rc->channel_yaw  = 0;
		rc->channel_aux1 = 0;
		rc->channel_aux2 = 0;
		rc->channel_aux3 = 0;
		rc->channel_aux4 = 0;
		rc->channel_aux5 = 0;
		rc->channel_aux6 = 0;
	} else {
		rc->channel_rol  = 0.644f * ((float)rc->sbus.channel_0 - 1024) + 1500;
		rc->channel_pit  = 0.644f * ((float)rc->sbus.channel_1 - 1024) + 1500;
		rc->channel_thr  = 0.644f * ((float)rc->sbus.channel_2 - 1024) + 1500;
		rc->channel_yaw  = 0.644f * ((float)rc->sbus.channel_3 - 1024) + 1500;
		rc->channel_aux1 = 0.644f * ((float)rc->sbus.channel_4 - 1024) + 1500;
		rc->channel_aux2 = 0.644f * ((float)rc->sbus.channel_5 - 1024) + 1500;
		rc->channel_aux3 = 0.644f * ((float)rc->sbus.channel_6 - 1024) + 1500;
		rc->channel_aux4 = 0.644f * ((float)rc->sbus.channel_7 - 1024) + 1500;
		rc->channel_aux5 = 0.644f * ((float)rc->sbus.channel_8 - 1024) + 1500;
		rc->channel_aux6 = 0.644f * ((float)rc->sbus.channel_9 - 1024) + 1500;
	}

	rc->get_data_cnt = 0;
}

int rc_module_init(struct fmu_rc *rc)
{
	int ret;

	ret = sbus_get_frame(&rc->sbus);
	if (ret) {
		return ret;
	}

	if (rc->sbus.flag & SBUS_FRAME_LOST) {
		return -ENODATA;
	}

	ret = tim_task_add(&rc->conv_data_task, "rc convert data",
			5, true, true, rc_process_data);
	if (ret) {
		return ret;
	}
	ret = tim_task_add(&rc->detect_task, "rc detect signal",
			1000, true, true, rc_detect_signal);
	if (ret) {
		return ret;
	}

	return 0;
}

void rc_module_exit(struct fmu_rc *rc)
{
	tim_task_drop(&rc->conv_data_task);
	tim_task_drop(&rc->detect_task);
}
