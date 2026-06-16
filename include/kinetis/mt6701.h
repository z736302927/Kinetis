#ifndef __MT6701_H
#define __MT6701_H

#ifdef __cplusplus
extern "C" {
#endif

/* The following must be adapted by the user to match the actual hardware,
   otherwise the driver cannot run. */

/* Includes ------------------------------------------------------------------*/
#include <linux/types.h>

#include "kinetis/core_common.h"
#include "kinetis/regmap-user-bus.h"

/* MT6701 virtual register for SSI path (not a real hardware register) */
#define MT6701_SSI_ANGLE_REG        0x00    /*< SSI pseudo-register: read triggers 24-bit SSI frame capture */

/* I2C address */
#define MT6701_I2C_ADDR_DEFAULT     0x06    /*< Default 7-bit I2C slave address b'0000110 */
#define MT6701_I2C_ADDR_ALT         0x46    /*< Alternate 7-bit I2C slave address b'1000110 */

/* Angle resolution */
#define MT6701_ANGLE_RESOLUTION     16384   /*< 14-bit resolution, each LSB = 0.022° */
#define MT6701_ANGLE_LSB_DEG        0.02197265625f  /*< 360/16384 */

/* Magnetic status enum (Mg[3:0]) */
enum mt6701_magnetic_status {
	MT6701_MAG_NORMAL       = 0,  /*< Normal magnetic field */
	MT6701_MAG_STRONG       = 1,  /*< Magnetic field too strong */
	MT6701_MAG_WEAK         = 2,  /*< Magnetic field too weak */
	MT6701_MAG_RESERVED     = 3,  /*< Reserved */
};

enum mt6701_push_status {
	MT6701_PUSH_RELEASED    = 0,  /*< Knob not pressed */
	MT6701_PUSH_PRESSED     = 1,  /*< Knob pressed */
};

enum mt6701_overspeed_status {
	MT6701_SPEED_NORMAL     = 0,  /*< Normal speed */
	MT6701_SPEED_OVERRUN    = 1,  /*< Overspeed detected */
};

/**
 * mt6701_angle_data_t - Raw angle data read via I2C
 * @raw_angle:  14-bit raw angle value [0, 16383]
 * @reserved:   Reserved bits in register 0x04 (upper 2 bits)
 */
typedef struct {
	u16 raw_angle;
	u8  reserved;
} mt6701_angle_data_t;

/**
 * mt6701_ssi_frame_t - SSI 24-bit frame parse result
 * @raw_angle:         14-bit raw angle [0, 16383]
 * @angle_degrees:     Angle in degrees [0.0, 360.0)
 * @mag_status:        Magnetic field strength status (Mg[1:0])
 * @push_status:       Push status (Mg[2])
 * @overspeed_status:  Overspeed status (Mg[3])
 * @crc_received:      Received CRC-6 value
 * @crc_calculated:    Locally computed CRC-6 value
 * @crc_valid:         Whether CRC check passed
 */
typedef struct {
	u16 raw_angle;
	float angle_degrees;
	u8  mag_status;
	u8  push_status;
	u8  overspeed_status;
	u8  crc_received;
	u8  crc_calculated;
	bool crc_valid;
} mt6701_ssi_frame_t;

/* Device lifecycle */
struct mt6701_device *mt6701_init(enum regmap_user_bus_type bus_type, void *bus_master);
void mt6701_exit(struct mt6701_device *dev);

/* Angle read (dual-interface) */
int mt6701_read_angle_raw(struct mt6701_device *dev, mt6701_angle_data_t *data);
int mt6701_read_angle_degrees(struct mt6701_device *dev, float *degrees);

/* SSI high-speed angle read */
int mt6701_read_angle_ssi(struct mt6701_device *dev, mt6701_ssi_frame_t *frame);

/* Configuration register read/write (I2C path) */
int mt6701_read_register(struct mt6701_device *dev, u8 reg, u8 *val);
int mt6701_write_register(struct mt6701_device *dev, u8 reg, u8 val);

/* EEPROM programming (I2C path, VDD > 4.5V) */
int mt6701_eeprom_program(struct mt6701_device *dev);
int mt6701_eeprom_write_config(struct mt6701_device *dev, u8 reg, u8 val);
int mt6701_eeprom_read_config(struct mt6701_device *dev, u8 reg, u8 *val);

/* CRC-6 check utility */
u8 mt6701_crc6_compute(u32 data, u8 num_bits);
u8 mt6701_crc6_verify(u32 ssi_frame);

/* Magnetic status decoder */
void mt6701_decode_magnetic_status(u8 mg, u8 *mag_status, u8 *push_status, u8 *overspeed_status);

/* The above must be adapted by the user to match the actual hardware,
   otherwise the driver cannot run. */

#ifdef __cplusplus
}
#endif

#endif /* __MT6701_H */
