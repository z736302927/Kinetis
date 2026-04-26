/**
 * @file host_pov_config.h
 * @brief Host (rotating end) POV display parameters
 * @note Defines display resolution, LED count, SPI configuration,
 *       timing constraints, and stability thresholds
 */

#ifndef POV_HOST_CONFIG_H
#define POV_HOST_CONFIG_H

/*********************************************************************
 * Display Resolution
 *********************************************************************/

#define POV_DISPLAY_COLS        720     /* Columns per revolution */
#define POV_LED_COUNT           256     /* Total LEDs per column */
#define POV_SPI_GROUPS          4       /* Number of parallel SPI groups */
#define POV_LEDS_PER_GROUP      (POV_LED_COUNT / POV_SPI_GROUPS)  /* 64 */
#define POV_BYTES_PER_GROUP     (POV_LEDS_PER_GROUP * 3)          /* 192 (RGB) */
#define POV_BYTES_PER_COL       (POV_BYTES_PER_GROUP * POV_SPI_GROUPS) /* 768 */

/*********************************************************************
 * Motor and Rotation
 *********************************************************************/

#define POV_TARGET_RPM          900     /* Target rotation speed */
#define POV_RPS                 (POV_TARGET_RPM / 60)   /* 15 rev/s */
#define POV_COL_TIME_US         92      /* Microseconds per column (~92.6us) */

/*********************************************************************
 * SPI Configuration
 *********************************************************************/

#define POV_SPI_CLOCK_HZ        20000000    /* 20 MHz SPI clock */
#define POV_SPI_SEND_TIME_US    76          /* ~76.8us for 192 bytes @ 20MHz */

/*********************************************************************
 * Stability Thresholds
 *********************************************************************/

#define POV_STABLE_THRESHOLD_RPM    2       /* RPM tolerance for stability */
#define POV_STABLE_DURATION_MS      1000    /* Must be stable for 1 second */

/*********************************************************************
 * Flash and Image
 *********************************************************************/

#define POV_IMAGE_FILENAME       "pov.img"  /* Image file on QSPI Flash */
#define POV_IMAGE_HEADER_SIZE    16         /* Image file header bytes */

/*********************************************************************
 * Angle Sensor
 *********************************************************************/

#define POV_HALL_PULSES_PER_REV  1          /* Hall sensor pulses per revolution */
#define POV_TIM_CLOCK_HZ         200000000  /* TIM2 clock (200MHz on H743) */
#define POV_TIM_PRESCALER        199        /* Divide to 1MHz tick (1us) */

/*********************************************************************
 * QSPI Flash Configuration (STM32H743)
 *********************************************************************/

#define POV_QSPI_BASE_ADDR       0x90000000  /* QSPI memory-mapped base */

#endif /* POV_HOST_CONFIG_H */
