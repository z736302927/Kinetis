
#include <linux/printk.h>
#include <linux/kernel.h>
#include <linux/slab.h>

#include <kinetis/tim-task.h>
#include <kinetis/serial-port.h>
#include <kinetis/basic-timer.h>
#include <kinetis/bootloader.h>
#include <kinetis/nrf24l01_port.h>

#include "project.h"
#include "config.h"
#include "rotor.h"
#include "stator.h"
#include "interface.h"
#include "test.h"

// #include "wifi/tcp_ap.h"
// #include "wifi/http_server.h"
// #include "wifi/wifi.h"

#if KINETIS_FAKE_SIM == 0
#include "main.h"
#endif


// static void wifi_task_cb(struct tim_task *task)
// {
// 	wifi_schedule();
// }

int board_init(void)
{
	pr_info("==== %s v%s ====\n", PROJECT_NAME, PROJECT_VERSION);
	pr_info("Platform: %s\n",
		MCU_PLATFORM_STM32 ? "STM32" : "Simulated");
#if KINETIS_FAKE_SIM
	return 0;
#else
	arm_dwt_init();

	return stm32_hal_init();
#endif
}

int app_main(int argc, char **argv)
{
	int ret = 0;

#if POV_RUN_MODE == POV_RUN_HOST
	pr_info("pov: running as rotor (rotating end)\n");

#if KINETIS_FAKE_SIM
	ret = pov_test_run(argc, argv);
#else
	struct pov_rotor *first;
	struct serial_port_ops *motor_ops;

#if POV_USE_NRF24L01
	motor_ops = &nrf24l01_serial_port_ops;
#else
	motor_ops = &usart2_serial_port_ops;
#endif
	first = pov_rotor_alloc(&general_flash_ops, &general_bootloader_ops,
		motor_ops, &usart6_serial_port_ops,
		&uart4_serial_port_ops, &uart5_serial_port_ops,
		&spi1_master, &spi6_master,
		hall_read_rotated_time);
	if (!first) {
		pr_err("pov: failed to allocate first\n");
		return -ENOMEM;
	}
	stm32_serial_port[1] = first->motor_port;
	stm32_serial_port[2] = first->app_port;
	stm32_serial_port[3] = first->ring_cw_port;
	stm32_serial_port[4] = first->ring_ccw_port;
#if POV_USE_MT6701
#else
	hall_dev = first->angle_sensor;
#endif

// 	first->wifi = wifi_init(WIFI_SSID, WIFI_PASSWORD, 1,
// 		WIFI_STATIC_IP, WIFI_STATIC_GW, WIFI_STATIC_NETMASK);
// 	if (first->wifi) {
// 	} else {
// 		return -ENOMEM;
// 	}
//
// 	ret = tim_task_add(&first->wifi->schedule_task, "wifi_poll", WIFI_POLL_INTERVAL_MS * 1000,
// 				true, false, wifi_task_cb);
// 	if (ret != 0)
// 		return ret;

	ret = pov_rotor_display(first, NULL);

	pov_rotor_free(first);
#endif

#elif POV_RUN_MODE == POV_RUN_SLAVE
	struct pov_stator *stator;
	struct serial_port_ops *motor_ops;

	pr_info("pov: running as stator (fixed end)\n");

#if POV_USE_NRF24L01
	motor_ops = &nrf24l01_serial_port_ops;
#else
	motor_ops = &usart2_serial_port_ops;
#endif
	stator = pov_stator_alloc(motor_ops,
		&general_flash_ops, &general_bootloader_ops,
		hall_read_rotated_time);
	if (!stator) {
		pr_err("pov: failed to allocate stator\n");
		return -ENOMEM;
	}
	stm32_serial_port[1] = stator->motor_port;
	hall_dev = stator->hall;

	while (1) {
		pov_stator_fsm(stator);
		tim_task_loop();
	}

#else
	pr_err("pov: invalid run mode %d\n", POV_RUN_MODE);
	ret = -EINVAL;
#endif

	pr_info("pov: exit with %d\n", ret);

	return ret;
}
