
#ifndef _PROJECT_POV_H
#define _PROJECT_POV_H

#define PROJECT_NAME        "Persistence-of-Vision"
#define PROJECT_VERSION     "0.2.0"

#define KINETIS_FAKE_SIM    1
#define MCU_PLATFORM_STM32  0

/*********************************************************************
 * POV Run Mode Selection
 *********************************************************************/

/* Select which MCU to simulate/run:
 * 0 = Host (rotating end, STM32H743)
 * 1 = Slave (fixed end, STM32F103)
 * 2 = Both (interleaved simulation for testing)
 */
#define POV_RUN_HOST        0
#define POV_RUN_SLAVE       1
#define POV_RUN_BOTH        2

#define POV_RUN_MODE        POV_RUN_HOST

/*********************************************************************
 * Public Functions
 *********************************************************************/

int board_init(void);
int app_main(void);

/* Host and Slave entry points */
int host_main(void);
int slave_main(void);

#endif /* _PROJECT_POV_H */
