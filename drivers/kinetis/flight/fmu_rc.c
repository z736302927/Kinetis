
#include <generated/deconfig.h>
#include <linux/string.h>


struct fmu_rc_sbus {
	u8 head;
	unsigned channel_0:11;
	unsigned channel_1:11;
	unsigned channel_2:11;
	unsigned channel_3:11;
	unsigned channel_4:11;
	unsigned channel_5:11;
	unsigned channel_6:11;
	unsigned channel_7:11;
	unsigned channel_8:11;
	unsigned channel_9:11;
	unsigned channel_10:11;
	unsigned channel_11:11;
	unsigned channel_12:11;
	unsigned channel_13:11;
	unsigned channel_14:11;
	unsigned channel_15:11;
	u8 flag;
	u8 tail;
} __packed;

struct fmu_rc_sbus r9ds;

void sbus_get_raw_data(u8 buf)
{
	static u8 sbus[25];
	static u8 cnt;
	
	if (buf == 0x0F || cnt == 25)
		cnt = 0;
	if (sbus[0] == 0x0F)
		sbus[cnt++] = buf;

	if (cnt == 25)
		memcpy(&r9ds, sbus, sizeof(sbus));
}

int rc_check_sbus(void)
{
	return 0;
}

