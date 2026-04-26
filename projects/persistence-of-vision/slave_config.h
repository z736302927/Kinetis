/**
 * @file slave_config.h
 * @brief Slave (fixed end) configuration parameters
 * @note Defines PID coefficients, timing parameters, motor
 *       configuration, and stability thresholds
 */

#ifndef POV_SLAVE_CONFIG_H
#define POV_SLAVE_CONFIG_H

/*********************************************************************
 * PID Controller Parameters
 *********************************************************************/

#define SLAVE_PID_KP            2.0f    /* Proportional gain */
#define SLAVE_PID_KI            0.5f    /* Integral gain */
#define SLAVE_PID_KD            0.1f    /* Derivative gain */
#define SLAVE_PID_OUTPUT_MAX    100.0f  /* Max output (0-100% duty cycle) */
#define SLAVE_PID_OUTPUT_MIN    0.0f    /* Min output */
#define SLAVE_PID_INTEGRAL_MAX  50.0f   /* Anti-windup: max integral term */

/*********************************************************************
 * Motor Parameters
 *********************************************************************/

#define SLAVE_TARGET_RPM        900     /* Default target RPM */
#define SLAVE_PWM_FREQ_HZ       1000    /* PWM frequency for motor drive */
#define SLAVE_PWM_RESOLUTION    1000    /* PWM counter period (0.1% steps) */
#define SLAVE_MOTOR_MAX_RPM     3000    /* Motor max RPM at 100% duty */
#define SLAVE_MOTOR_DIR_FORWARD 1
#define SLAVE_MOTOR_DIR_REVERSE 0

/*********************************************************************
 * Hall Sensor Parameters
 *********************************************************************/

#define SLAVE_HALL_PULSES_PER_REV  1    /* Hall pulses per revolution */
#define SLAVE_HALL_TIMEOUT_MS      500  /* No-pulse timeout (motor stalled) */

/*********************************************************************
 * Timing Parameters (ms)
 *********************************************************************/

#define SLAVE_PID_PERIOD_MS     10      /* PID calculation interval */
#define SLAVE_HEARTBEAT_MS      1000    /* Heartbeat send interval */
#define SLAVE_STATUS_MS         100     /* Status report interval */

/*********************************************************************
 * Stability Thresholds
 *********************************************************************/

#define SLAVE_STABLE_RPM_THRESH 2       /* RPM tolerance for "stable" */
#define SLAVE_STABLE_COUNT_REQ  100     /* Consecutive stable PID cycles (1s) */

/*********************************************************************
 * MAVLink IDs (must match host)
 *********************************************************************/

#define SLAVE_SYSID             2
#define SLAVE_COMPID            0

#endif /* POV_SLAVE_CONFIG_H */
