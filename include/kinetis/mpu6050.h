#ifndef __MPU6050_H
#define __MPU6050_H

#ifdef __cplusplus
extern "C" {
#endif

#include <linux/types.h>

#include "kinetis/core_common.h"

/* Type definitions */
typedef struct {
    int16_t x;
    int16_t y;
    int16_t z;
} mpu6050_raw_data_t;

typedef struct {
    float x;
    float y;
    float z;
} mpu6050_data_t;

typedef struct {
    uint8_t device_present;
    uint8_t gyro_sensitivity;
    uint8_t accel_sensitivity;
    float gyro_scale;
    float accel_scale;
    uint8_t initialized;
} mpu6050_config_t;

#ifdef __cplusplus
}
#endif

#endif /* __MPU6050_H */
