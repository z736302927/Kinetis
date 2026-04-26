/**
 * @file host_main.c
 * @brief Host (rotating end) main state machine
 * @note Implements the display workflow:
 *   WAIT_SLAVE → SEND_SPEED → WAIT_STABLE → DISPLAYING
 */

#include <linux/kernel.h>
#include <linux/printk.h>
#include <linux/errno.h>
#include <linux/delay.h>

#include "host_pov_config.h"
#include "host_mavlink.h"
#include "host_angle.h"
#include "host_spi_multi.h"
#include "host_flash_fs.h"
#include "mavlink_config.h"

#include <kinetis/basic-timer.h>

/*********************************************************************
 * Host State Machine
 *********************************************************************/

enum host_state {
	HOST_STATE_WAIT_SLAVE = 0,
	HOST_STATE_SEND_SPEED,
	HOST_STATE_WAIT_STABLE,
	HOST_STATE_DISPLAYING,
	HOST_STATE_ERROR
};

struct host_device {
	enum host_state state;
	u64 state_enter_ms;       /* Timestamp when entering current state */
	u64 stable_start_ms;      /* When RPM first became stable */
	u8 display_active;        /* 1 if display loop is running */
	u8 error_code;            /* Error code if in ERROR state */

	struct host_mavlink_device mavlink;
	struct host_angle_device angle;
	struct host_spi_multi_device spi_multi;
	struct host_flash_fs_device flash_fs;
};

static struct host_device g_host;

/*********************************************************************
 * State Transition
 *********************************************************************/

static void host_set_state(struct host_device *dev, enum host_state new_state)
{
	pr_info("Host: state %d → %d\n", dev->state, new_state);
	dev->state = new_state;
	dev->state_enter_ms = basic_timer_get_ms();
}

/*********************************************************************
 * State: WAIT_SLAVE
 *********************************************************************/

static void host_state_wait_slave(struct host_device *dev)
{
	/* Process MAVLink messages to detect slave */
	host_mavlink_process(&dev->mavlink, 50);

	if (host_mavlink_is_slave_online(&dev->mavlink)) {
		pr_info("Host: slave online!\n");
		host_set_state(dev, HOST_STATE_SEND_SPEED);
	} else {
		mdelay(100);
	}
}

/*********************************************************************
 * State: SEND_SPEED
 *********************************************************************/

static void host_state_send_speed(struct host_device *dev)
{
	int ret = host_mavlink_send_speed_cmd(&dev->mavlink, POV_TARGET_RPM);
	if (ret < 0) {
		pr_err("Host: failed to send speed command: %d\n", ret);
		mdelay(500);
		return;
	}

	pr_info("Host: sent target RPM=%d to slave\n", POV_TARGET_RPM);
	host_set_state(dev, HOST_STATE_WAIT_STABLE);
}

/*********************************************************************
 * State: WAIT_STABLE
 *********************************************************************/

static void host_state_wait_stable(struct host_device *dev)
{
	/* Process incoming status messages */
	host_mavlink_process(&dev->mavlink, 10);

	s32 rpm = host_mavlink_get_slave_rpm(&dev->mavlink);
	s32 diff = rpm - POV_TARGET_RPM;
	if (diff < 0)
		diff = -diff;

	u64 now = basic_timer_get_ms();

	if (diff <= POV_STABLE_THRESHOLD_RPM) {
		/* RPM is within threshold */
		if (dev->stable_start_ms == 0) {
			dev->stable_start_ms = now;
			pr_info("Host: RPM stable at %d (target %d)\n",
				rpm, POV_TARGET_RPM);
		} else if ((now - dev->stable_start_ms) >= POV_STABLE_DURATION_MS) {
			pr_info("Host: RPM stable for %dms, starting display\n",
				POV_STABLE_DURATION_MS);
			host_set_state(dev, HOST_STATE_DISPLAYING);
			return;
		}
	} else {
		/* RPM not stable, reset timer */
		if (dev->stable_start_ms != 0) {
			pr_info("Host: RPM unstable (%d, diff=%d), resetting timer\n",
				rpm, diff);
		}
		dev->stable_start_ms = 0;
	}

	/* Check slave still online */
	if (!host_mavlink_is_slave_online(&dev->mavlink)) {
		pr_warn("Host: slave went offline while waiting for stable\n");
		host_set_state(dev, HOST_STATE_WAIT_SLAVE);
		return;
	}

	/* Resend speed command periodically */
	u64 elapsed = now - dev->state_enter_ms;
	if (elapsed > 2000) {
		host_mavlink_send_speed_cmd(&dev->mavlink, POV_TARGET_RPM);
		dev->state_enter_ms = now;
	}

	mdelay(10);
}

/*********************************************************************
 * State: DISPLAYING
 *********************************************************************/

static void host_state_displaying(struct host_device *dev)
{
	/* Process MAVLink messages (non-blocking) */
	host_mavlink_process(&dev->mavlink, 1);

	/* Check slave status */
	if (!host_mavlink_is_slave_online(&dev->mavlink)) {
		pr_warn("Host: slave offline during display!\n");
		dev->display_active = 0;
		host_set_state(dev, HOST_STATE_WAIT_SLAVE);
		return;
	}

	u8 status = host_mavlink_get_slave_status(&dev->mavlink);
	if (status == POV_STATUS_FAULT) {
		pr_err("Host: slave fault, stopping display\n");
		dev->display_active = 0;
		host_set_state(dev, HOST_STATE_ERROR);
		return;
	}

	/* Check for new column from angle sensor */
	if (!host_angle_is_new_column(&dev->angle))
		return;

	int column = host_angle_get_column(&dev->angle);
	if (column < 0)
		return;

	/* Read column data from Flash */
	u8 col_data[POV_BYTES_PER_COL];
	int ret = host_flash_fs_read_column(&dev->flash_fs, (u16)column, col_data);
	if (ret < 0) {
		pr_err("Host: failed to read column %d: %d\n", column, ret);
		return;
	}

	/* Send to 4-way SPI DMA */
	ret = host_spi_multi_send(&dev->spi_multi, col_data);
	if (ret < 0) {
		pr_err("Host: SPI send failed: %d\n", ret);
		return;
	}

	/* Wait for DMA completion and latch */
	ret = host_spi_multi_wait_complete(&dev->spi_multi, POV_COL_TIME_US);
	if (ret < 0) {
		pr_warn("Host: DMA timeout on column %d\n", column);
	}

	host_spi_multi_latch(&dev->spi_multi);
}

/*********************************************************************
 * State: ERROR
 *********************************************************************/

static void host_state_error(struct host_device *dev)
{
	pr_err("Host: in ERROR state (code=%d), attempting recovery\n",
		dev->error_code);

	/* Stop display */
	dev->display_active = 0;

	/* Try to stop the slave motor */
	host_mavlink_send_speed_cmd(&dev->mavlink, 0);

	mdelay(1000);

	/* Attempt recovery */
	host_set_state(dev, HOST_STATE_WAIT_SLAVE);
}

/*********************************************************************
 * Host Main Entry
 *********************************************************************/

/**
 * @brief Host main function - runs the host state machine
 * @return 0 on normal exit, negative error code on failure
 */
int host_main(void)
{
	struct host_device *dev = &g_host;
	int ret;

	pr_info("==== POV Host Starting ====\n");

	/* Initialize all subsystems */
	ret = host_mavlink_init(&dev->mavlink);
	if (ret < 0) {
		pr_err("Host: MAVLink init failed: %d\n", ret);
		return ret;
	}

	ret = host_angle_init(&dev->angle);
	if (ret < 0) {
		pr_err("Host: angle init failed: %d\n", ret);
		return ret;
	}

	ret = host_spi_multi_init(&dev->spi_multi);
	if (ret < 0) {
		pr_err("Host: SPI multi init failed: %d\n", ret);
		return ret;
	}

	ret = host_flash_fs_init(&dev->flash_fs);
	if (ret < 0) {
		pr_err("Host: Flash FS init failed: %d\n", ret);
		return ret;
	}

#if KINETIS_FAKE_SIM
	/* In simulation, use fake serial port loopback for MAVLink */
	serial_port_start_thread(dev->mavlink.serial,
		SERIAL_PORT_DF_SELF, NULL, dev->mavlink.serial);
#endif

	/* Load image from Flash */
	ret = host_flash_fs_load_image(&dev->flash_fs, POV_IMAGE_FILENAME);
	if (ret < 0) {
		pr_err("Host: image load failed: %d\n", ret);
		return ret;
	}

	/* Enter main state machine loop */
	dev->state = HOST_STATE_WAIT_SLAVE;
	dev->state_enter_ms = basic_timer_get_ms();
	dev->stable_start_ms = 0;
	dev->display_active = 0;

	pr_info("Host: entering state machine\n");

	while (1) {
		switch (dev->state) {
		case HOST_STATE_WAIT_SLAVE:
			host_state_wait_slave(dev);
			break;
		case HOST_STATE_SEND_SPEED:
			host_state_send_speed(dev);
			break;
		case HOST_STATE_WAIT_STABLE:
			host_state_wait_stable(dev);
			break;
		case HOST_STATE_DISPLAYING:
			host_state_displaying(dev);
			break;
		case HOST_STATE_ERROR:
			host_state_error(dev);
			break;
		default:
			pr_err("Host: invalid state %d\n", dev->state);
			dev->state = HOST_STATE_WAIT_SLAVE;
			break;
		}
	}

	return 0;
}
