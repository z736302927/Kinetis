
#ifndef POV_HALL_H
#define POV_HALL_H

#include <linux/types.h>

#include <kinetis/tim-task.h>

struct hall_device {
	u32 angular_resolution;  /*< Columns per revolution (e.g. 720) */
	u32 period_ticks;        /*< Ticks between consecutive hall pulses */

	u32(*read_rotated_time)(struct hall_device *dev);
	/*< Platform-specific time read function (simulation or hardware TIM) */

	struct tim_task fake_pulse_task;
};

u32 hall_get_angle(struct hall_device *dev);
s32 hall_get_rpm(struct hall_device *dev);
void hall_hall_isr(struct hall_device *dev, u32 capture_value);
struct hall_device *hall_alloc_dev(u32 angular_resolution,
	u32(*read_rotated_time)(struct hall_device *dev));
u32 hall_read_rotated_time(struct hall_device *dev);

#endif /* POV_HALL_H */
