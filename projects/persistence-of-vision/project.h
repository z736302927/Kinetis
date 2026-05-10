
#ifndef PROJECT_POV_H
#define PROJECT_POV_H

#define PROJECT_NAME          "Persistence-of-Vision"
#define PROJECT_VERSION       "0.2.0"

#define KINETIS_FAKE_SIM      1
#define MCU_PLATFORM_STM32    0

/* Select which MCU to simulate/run:
 * POV_RUN_HOST  (0) = Host (rotating end, STM32H743) - LED display + MAVLink master
 * POV_RUN_SLAVE (1) = Slave (fixed end,  STM32F103) - PID motor control
 * (Reserved: 2 = Both interleaved for testing)
 *
 * Change POV_RUN_MODE to select the active role.
 */
#define POV_RUN_HOST          0
#define POV_RUN_SLAVE         1

#define POV_RUN_MODE          POV_RUN_HOST

int board_init(void);
int app_main(void);

#endif /* PROJECT_POV_H */
