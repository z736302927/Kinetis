#define pr_fmt(fmt) "mt6701: " fmt

#include <linux/bitops.h>
#include <linux/printk.h>
#include <linux/delay.h>
#include <linux/err.h>
#include <linux/slab.h>
#include <linux/random.h>
#include <linux/string.h>
#include <linux/ktime.h>

#include "kinetis/mt6701.h"
#include "kinetis/iic_soft.h"
#include "kinetis/spi_soft.h"
#include "kinetis/regmap-user-bus.h"
#include "kinetis/idebug.h"
#include "kinetis/design_verification.h"
#include "kinetis/random-gene.h"
#include "kinetis/test-kinetis.h"

#ifdef KINETIS_FAKE_SIM
#include <pthread.h>
#endif
#include <math.h>

/*---------------------------------------------------------------------------
 * Section 1: I2C Register Address Definitions
 *---------------------------------------------------------------------------*/

/* Angle data registers (Read-only) */
#define MT6701_REG_ANGLE_H              0x03    /*< Angle[13:6], Read-only */
#define MT6701_REG_ANGLE_L              0x04    /*< Angle[5:0], NA, NA, Read-only */

/* EEPROM programming registers */
#define MT6701_REG_EEPROM_KEY           0x09    /*< EEPROM program key (write 0xB3) */
#define MT6701_REG_EEPROM_CMD           0x0A    /*< EEPROM program command (write 0x05) */

/* Configuration registers */
#define MT6701_REG_UVW_MUX              0x25    /*< UVW output type [7] */
#define MT6701_REG_ABZ_MUX_DIR          0x29    /*< ABZ_MUX[6], DIR[1] */
#define MT6701_REG_UVW_ABZ_RES_H        0x30    /*< UVW_RES[7:4], ABZ_RES[9:8] */
#define MT6701_REG_ABZ_RES_L            0x31    /*< ABZ_RES[7:0] */
#define MT6701_REG_HYST_ZERO_H          0x32    /*< HYST[2][7], Z_PULSE_WIDTH[6:4], ZERO[11:8] */
#define MT6701_REG_ZERO_L               0x33    /*< ZERO[7:0] */
#define MT6701_REG_HYST_L               0x34    /*< HYST[1:0][7:6] */
#define MT6701_REG_PWM_OUT              0x38    /*< PWM_FREQ[7], PWM_POL[6], OUT_MODE[5] */
#define MT6701_REG_A_START_STOP_H       0x3E    /*< A_STOP[11:8][7:4], A_START[11:8][3:0] */
#define MT6701_REG_A_START_L            0x3F    /*< A_START[7:0] */
#define MT6701_REG_A_STOP_L             0x40    /*< A_STOP[7:0] */

#define MT6701_REG_MAX                  0x40

/*
 * Section 2: SSI Frame Bit Definitions
 */

#define SSI_ANGLE_SHIFT                 10      /*< Angle data bit offset in 24-bit frame */
#define SSI_ANGLE_MASK                  0x3FFF  /*< 14-bit angle mask (bits[23:10]) */
#define SSI_MG_SHIFT                    6       /*< Magnetic status bit offset */
#define SSI_MG_MASK                     0x0F    /*< 4-bit magnetic status mask */
#define SSI_CRC_SHIFT                   0       /*< CRC bit offset */
#define SSI_CRC_MASK                    0x3F    /*< 6-bit CRC mask */

#define SSI_FRAME_BITS                  24      /*< SSI frame total bits */
#define SSI_FRAME_BYTES                 3       /*< SSI frame bytes */

/*
 * Section 3: Constants and Bit Masks
 */

/* EEPROM programming constants */
#define MT6701_EEPROM_KEY_VAL           0xB3
#define MT6701_EEPROM_CMD_VAL           0x05
#define MT6701_EEPROM_PROGRAM_MS        650     /*< EEPROM program wait time (with margin) */

/* CRC-6 polynomial: X^6 + X + 1 */
#define MT6701_CRC6_POLY                0x03
#define MT6701_CRC6_BITS                18      /*< CRC covers D[13:0] + Mg[3:0] for 18 bits total */

/* Output mode */
#define MT6701_OUT_MODE_ANALOG          0x00
#define MT6701_OUT_MODE_PWM             0x01

/* Rotation direction */
#define MT6701_DIR_CCW                  0x00    /*< CCW: angle increases */
#define MT6701_DIR_CW                   0x01    /*< CW: angle increases */

/* ABZ/UVW output type */
#define MT6701_ABZ_MUX_ABZ              0x00
#define MT6701_ABZ_MUX_UVW              0x01

/* Reserved bit masks (must be preserved during read-modify-write) */
#define MT6701_REG_25_RSV_MASK          0x7F    /*< 0x25: bits 0-6 reserved */
#define MT6701_REG_29_RSV_MASK          0x5D    /*< 0x29: bits 0,2,3,4,5,7 partially reserved */
#define MT6701_REG_30_RSV_MASK          0x0C    /*< 0x30: bits 2,3 reserved */
#define MT6701_REG_32_RSV_MASK          0x00    /*< 0x32: no reserved bits */
#define MT6701_REG_38_RSV_MASK          0x1F    /*< 0x38: bits 0-4 reserved */
#define MT6701_REG_3E_RSV_MASK          0x00    /*< 0x3E: no reserved bits */

#ifdef KINETIS_FAKE_SIM
#define MT6701_SIM_UPDATE_PERIOD_MS     1       /*< Simulation update period 1ms */
#define MT6701_SIM_ANGLE_STEP           1       /*< Step 1 LSB per iteration (~0.022°) */
#define MT6701_SIM_NOISE_STDEV          2       /*< Angle noise standard deviation (LSB) */
#define MT6701_SIM_ANOMALY_PROB         0x1FF   /*< Anomaly probability ~0.1% (compare with 0x1FF) */
#endif

#define MT6701_RPM_EMA_ALPHA_NUM        1           /*< EMA numerator (alpha = 1/4 = 0.25) */
#define MT6701_RPM_EMA_ALPHA_DEN        4           /*< EMA denominator */

/**
 * crc6_table - CRC-6 lookup table (polynomial X^6+X+1)
 *
 * Used for fast CRC-6 calculation on 18-bit (D[13:0]+Mg[3:0]) data packets.
 * Pre-computed using the standard bit-by-bit method: for each input byte,
 * simulate a 6-bit shift register operation.
 */
static const u8 crc6_table[256] = {
	0x00, 0x03, 0x06, 0x05, 0x0C, 0x0F, 0x0A, 0x09,
	0x18, 0x1B, 0x1E, 0x1D, 0x14, 0x17, 0x12, 0x11,
	0x30, 0x33, 0x36, 0x35, 0x3C, 0x3F, 0x3A, 0x39,
	0x28, 0x2B, 0x2E, 0x2D, 0x24, 0x27, 0x22, 0x21,
	0x23, 0x20, 0x25, 0x26, 0x2F, 0x2C, 0x29, 0x2A,
	0x3B, 0x38, 0x3D, 0x3E, 0x37, 0x34, 0x31, 0x32,
	0x13, 0x10, 0x15, 0x16, 0x1F, 0x1C, 0x19, 0x1A,
	0x0B, 0x08, 0x0D, 0x0E, 0x07, 0x04, 0x01, 0x02,
	0x05, 0x06, 0x03, 0x00, 0x09, 0x0A, 0x0F, 0x0C,
	0x1D, 0x1E, 0x1B, 0x18, 0x11, 0x12, 0x17, 0x14,
	0x35, 0x36, 0x33, 0x30, 0x39, 0x3A, 0x3F, 0x3C,
	0x2D, 0x2E, 0x2B, 0x28, 0x21, 0x22, 0x27, 0x24,
	0x26, 0x25, 0x20, 0x23, 0x2A, 0x29, 0x2C, 0x2F,
	0x3E, 0x3D, 0x38, 0x3B, 0x32, 0x31, 0x34, 0x37,
	0x16, 0x15, 0x10, 0x13, 0x1A, 0x19, 0x1C, 0x1F,
	0x0E, 0x0D, 0x08, 0x0B, 0x02, 0x01, 0x04, 0x07,
	0x0A, 0x09, 0x0C, 0x0F, 0x06, 0x05, 0x00, 0x03,
	0x12, 0x11, 0x14, 0x17, 0x1E, 0x1D, 0x18, 0x1B,
	0x3A, 0x39, 0x3C, 0x3F, 0x36, 0x35, 0x30, 0x33,
	0x22, 0x21, 0x24, 0x27, 0x2E, 0x2D, 0x28, 0x2B,
	0x29, 0x2A, 0x2F, 0x2C, 0x25, 0x26, 0x23, 0x20,
	0x31, 0x32, 0x37, 0x34, 0x3D, 0x3E, 0x3B, 0x38,
	0x19, 0x1A, 0x1F, 0x1C, 0x15, 0x16, 0x13, 0x10,
	0x01, 0x02, 0x07, 0x04, 0x0D, 0x0E, 0x0B, 0x08,
	0x0F, 0x0C, 0x09, 0x0A, 0x03, 0x00, 0x05, 0x06,
	0x17, 0x14, 0x11, 0x12, 0x1B, 0x18, 0x1D, 0x1E,
	0x3F, 0x3C, 0x39, 0x3A, 0x33, 0x30, 0x35, 0x36,
	0x27, 0x24, 0x21, 0x22, 0x2B, 0x28, 0x2D, 0x2E,
	0x2C, 0x2F, 0x2A, 0x29, 0x20, 0x23, 0x26, 0x25,
	0x34, 0x37, 0x32, 0x31, 0x38, 0x3B, 0x3E, 0x3D,
	0x1C, 0x1F, 0x1A, 0x19, 0x10, 0x13, 0x16, 0x15,
	0x04, 0x07, 0x02, 0x01, 0x08, 0x0B, 0x0E, 0x0D,
};

/**
 * struct mt6701_ssi_context - SSI regmap bus context
 * @master:   SPI master instance (provides CS/CLK/MISO GPIO callbacks)
 * @frame_buf: Points to device->ssi_frame_buf in sim mode (pre-generated frames), NULL in hardware mode
 */
struct mt6701_ssi_context {
	struct spi_master *master;
	u8 *frame_buf;
};

/**
 * struct mt6701_device - MT6701 device structure
 * @regmap:     Unified regmap instance (I2C standard bus or SSI custom bus)
 * @bus_type:   Current bus type in use
 *
 * Simulation mode extra fields (only valid when KINETIS_FAKE_SIM):
 * @iic_slave:     I2C slave simulation instance
 * @spi_slave:     SPI slave simulation instance (SSI path)
 * @slave_regs:    I2C register map area (0x00~0x40, size max_register+1)
 * @ssi_frame_buf: SSI pre-generated 24-bit frame buffer (updated by sim thread, returned by SSI regmap .read)
 * @thread_running: Simulation thread running flag
 */
struct mt6701_device {
	struct regmap *regmap;
	enum regmap_user_bus_type bus_type;

	s32	rpm_filtered;

#ifdef KINETIS_FAKE_SIM
	struct iic_slave *iic_slave;
	struct spi_slave *spi_slave;
	u8 *slave_regs;
	u8 ssi_frame_buf[SSI_FRAME_BYTES];
	bool thread_running;
#endif
};

/**
 * mt6701_crc6_compute - Compute CRC-6 for 18-bit data
 * @data:     18-bit input data (D[13:0] in high bits + Mg[3:0] in low bits)
 * @num_bits: Number of data bits (should be MT6701_CRC6_BITS = 18)
 *
 * Uses a hybrid lookup-table + bit-by-bit method: first 16 bits via table lookup,
 * remaining 2 bits processed bit-by-bit.
 * CRC-6 polynomial: X^6 + X + 1 (0x03)
 *
 * Return: 6-bit CRC value [0, 63]
 */
u8 mt6701_crc6_compute(u32 data, u8 num_bits)
{
	u8 crc = 0;
	u8 byte_val;
	u8 i;

	if (num_bits == 0)
		return 0;

	/* Process byte-by-byte using lookup table */
	for (i = 0; i < num_bits / 8; i++) {
		byte_val = (data >> (num_bits - 8 * (i + 1))) & 0xFF;
		crc = crc6_table[(crc << 2) ^ byte_val] & SSI_CRC_MASK;
	}

	/* Handle remaining bits (less than 8) */
	{
		u8 remaining = num_bits % 8;
		if (remaining > 0) {
			u8 remaining_byte = (data & ((1 << remaining) - 1)) << (8 - remaining);
			crc = crc6_table[(crc << 2) ^ remaining_byte] & SSI_CRC_MASK;
		}
	}

	return crc;
}

/**
 * mt6701_crc6_verify - Verify CRC-6 of an SSI 24-bit frame
 * @ssi_frame: Complete 24-bit SSI frame data
 *
 * Extracts D[13:0]+Mg[3:0] from the frame, computes local CRC,
 * and compares with CRC[5:0] in the frame.
 *
 * Return: 0 = CRC match, non-zero = CRC mismatch
 */
u8 mt6701_crc6_verify(u32 ssi_frame)
{
	u32 data_bits = (ssi_frame >> SSI_CRC_SHIFT) & ~SSI_CRC_MASK;
	u8 crc_received = ssi_frame & SSI_CRC_MASK;
	u8 crc_calc = mt6701_crc6_compute(data_bits, MT6701_CRC6_BITS);

	return crc_received ^ crc_calc;  /* 0 = match */
}

/**
 * mt6701_decode_magnetic_status - Decode magnetic status
 * @mg:             Raw Mg[3:0] value
 * @mag_status:     Output: magnetic field strength status
 * @push_status:    Output: push status
 * @overspeed_status: Output: overspeed status
 */
void mt6701_decode_magnetic_status(u8 mg, u8 *mag_status, u8 *push_status, u8 *overspeed_status)
{
	if (mag_status)
		*mag_status = mg & 0x03;
	if (push_status)
		*push_status = (mg >> 2) & 0x01;
	if (overspeed_status)
		*overspeed_status = (mg >> 3) & 0x01;
}

/**
 * mt6701_ssi_regmap_read - SSI regmap .read callback
 * @context:   mt6701_ssi_context pointer
 * @reg:       Register address (ignored by SSI)
 * @reg_size:  Register address size (ignored by SSI)
 * @val:       Output buffer
 * @val_size:  Number of bytes to read
 *
 * Performs SSI 24-bit frame read:
 *   Simulation mode: copy pre-generated frame from frame_buf
 *   Hardware mode: CS↓ → 24×(SCK pulse + read MISO) → CS↑
 *
 * Return: 0 on success, negative on error
 */
static int mt6701_ssi_regmap_read(void *context, const void *reg,
				   size_t reg_size, void *val, size_t val_size)
{
	struct mt6701_ssi_context *ctx = context;
	u32 frame = 0;
	u8 i;
	u8 bit_val;

	if (!ctx || !val || val_size == 0)
		return -EINVAL;

#ifdef KINETIS_FAKE_SIM
	/* Simulation mode: return pre-generated frame */
	if (ctx->frame_buf) {
		frame = ((u32)ctx->frame_buf[0] << 16) |
			((u32)ctx->frame_buf[1] << 8) |
			 (u32)ctx->frame_buf[2];
		goto out;
	}
#endif

	/* Hardware mode: execute SSI protocol */
	if (!ctx->master) {
		pr_err("SSI read: no spi_master configured\n");
		return -EIO;
	}

	/* CS low, start transmission */
	if (ctx->master->cs_low) {
		ctx->master->cs_low();
	}

	/* 24 clock cycles, MSB first, sample MISO after each CLK rising edge */
	for (i = 0; i < SSI_FRAME_BITS; i++) {
		if (ctx->master->sck_low) {
			ctx->master->sck_low();
		}
		/* Short delay (meet SSI minimum 30ns low time) */
		udelay(1);

		if (ctx->master->sck_high) {
			ctx->master->sck_high();
		}
		/* Delay then sample (DO settles after CLK rising edge, max 15ns Tdv) */
		udelay(1);

		/* Read MISO/DO */
		if (ctx->master->miso_read) {
			bit_val = ctx->master->miso_read() ? 1 : 0;
		} else {
			bit_val = 0;
		}

		frame = (frame << 1) | bit_val;
	}

	/* CS high, end transmission */
	if (ctx->master->cs_high) {
		ctx->master->cs_high();
	}

out:
	/* Fill output buffer (big-endian: high byte first) */
	if (val_size >= 4) {
		*(u32 *)val = frame;
	} else if (val_size == 3) {
		((u8 *)val)[0] = (frame >> 16) & 0xFF;
		((u8 *)val)[1] = (frame >> 8) & 0xFF;
		((u8 *)val)[2] = frame & 0xFF;
	} else if (val_size == 2) {
		((u8 *)val)[0] = (frame >> 16) & 0xFF;
		((u8 *)val)[1] = (frame >> 8) & 0xFF;
	} else {
		((u8 *)val)[0] = (frame >> 16) & 0xFF;
	}

	return 0;
}

/**
 * mt6701_ssi_regmap_write - SSI regmap .write callback
 *
 * SSI is a read-only protocol, write is not supported.
 *
 * Return: Always returns -ENOTSUPP
 */
static int mt6701_ssi_regmap_write(void *context, const void *data, size_t count)
{
	pr_err("SSI bus does not support write operations\n");
	return -ENOTSUPP;
}

/**
 * mt6701_ssi_free_context - SSI regmap .free_context callback
 */
static void mt6701_ssi_free_context(void *context)
{
	kfree(context);
}

/** regmap_bus_mt6701_ssi - SSI custom regmap bus */
static struct regmap_bus regmap_bus_mt6701_ssi = {
	.write = mt6701_ssi_regmap_write,
	.read = mt6701_ssi_regmap_read,
	.free_context = mt6701_ssi_free_context,
	.reg_format_endian_default = REGMAP_ENDIAN_BIG,
	.val_format_endian_default = REGMAP_ENDIAN_BIG,
};

/**
 * mt6701_init_ssi_regmap - Initialize SSI regmap instance
 * @master: SPI master instance
 * @frame_buf: Simulation frame buffer pointer (NULL in hardware mode)
 * @config: regmap configuration
 *
 * Return: regmap pointer on success, ERR_PTR on failure
 */
static struct regmap *mt6701_init_ssi_regmap(struct spi_master *master,
					      u8 *frame_buf,
					      const struct regmap_config *config)
{
	struct mt6701_ssi_context *ctx;
	struct regmap *map;

	if (!master || !config) {
		pr_err("invalid parameters for SSI regmap init\n");
		return ERR_PTR(-EINVAL);
	}

	ctx = kzalloc(sizeof(*ctx), GFP_KERNEL);
	if (!ctx)
		return ERR_PTR(-ENOMEM);

	ctx->master = master;
	ctx->frame_buf = frame_buf;

	map = regmap_init(NULL, &regmap_bus_mt6701_ssi, ctx, config);
	if (IS_ERR(map)) {
		pr_err("failed to initialize SSI regmap: %ld\n", PTR_ERR(map));
		kfree(ctx);
		return map;
	}

	pr_debug("SSI regmap initialized\n");
	return map;
}

/**
 * mt6701_read_angle_raw - Read raw angle data via I2C
 * @dev:  MT6701 device pointer
 * @data: Output: raw angle data structure
 *
 * Reads 0x03(ANGLE_H) and 0x04(ANGLE_L), forming a 14-bit raw angle value.
 * Note: must read 0x03 before 0x04 (chip requirement).
 *
 * Return: 0 on success, negative on error
 */
int mt6701_read_angle_raw(struct mt6701_device *dev, mt6701_angle_data_t *data)
{
	u8 buf[2];
	int ret;

	if (!dev || !data)
		return -EINVAL;

	if (dev->bus_type != REGMAP_BUS_IIC_SOFT) {
		pr_err("read_angle_raw requires I2C bus\n");
		return -ENOTSUPP;
	}

	ret = regmap_bulk_read(dev->regmap, MT6701_REG_ANGLE_H, buf, 2);
	if (ret) {
		pr_err("failed to read angle registers: %d\n", ret);
		return ret;
	}

	data->raw_angle = ((u16)buf[0] << 6) | (buf[1] & 0x3F);
	data->reserved = (buf[1] >> 6) & 0x03;

	return 0;
}

/**
 * mt6701_read_angle_degrees - Read angle in degrees via I2C
 * @dev:     MT6701 device pointer
 * @degrees: Output: angle value (0.0 ~ 360.0)
 *
 * Angle formula: angle = raw / 16384 * 360
 *
 * Return: 0 on success, negative on error
 */
int mt6701_read_angle_degrees(struct mt6701_device *dev, float *degrees)
{
	mt6701_angle_data_t data;
	int ret;

	if (!dev || !degrees)
		return -EINVAL;

	ret = mt6701_read_angle_raw(dev, &data);
	if (ret)
		return ret;

	*degrees = (float)data.raw_angle / (float)MT6701_ANGLE_RESOLUTION * 360.0f;

	return 0;
}

/**
 * mt6701_read_angle_ssi - Read angle via SSI and parse the full frame
 * @dev:   MT6701 device pointer
 * @frame: Output: SSI frame parse result
 *
 * Triggers an SSI 24-bit frame read via regmap_read, parses angle,
 * magnetic status, and CRC check.
 * Suitable for high-speed angle reading (up to 55,000 RPM).
 *
 * Return: 0 on success, negative on error
 */
int mt6701_read_angle_ssi(struct mt6701_device *dev, mt6701_ssi_frame_t *frame)
{
	u32 ssi_raw;
	u8 mg;
	int ret;

	if (!dev || !frame)
		return -EINVAL;

	ret = regmap_read(dev->regmap, MT6701_SSI_ANGLE_REG, &ssi_raw);
	if (ret) {
		pr_err("SSI read failed: %d\n", ret);
		return ret;
	}

	/* Extract D[13:0] 14-bit angle (bits 23:10) */
	frame->raw_angle = (ssi_raw >> SSI_ANGLE_SHIFT) & SSI_ANGLE_MASK;
	frame->angle_degrees = (float)frame->raw_angle / (float)MT6701_ANGLE_RESOLUTION * 360.0f;

	/* Extract Mg[3:0] magnetic status (bits 9:6) */
	mg = (ssi_raw >> SSI_MG_SHIFT) & SSI_MG_MASK;
	mt6701_decode_magnetic_status(mg, &frame->mag_status,
				       &frame->push_status,
				       &frame->overspeed_status);

	/* CRC-6 verification */
	frame->crc_received = ssi_raw & SSI_CRC_MASK;
	frame->crc_calculated = mt6701_crc6_compute(ssi_raw >> SSI_CRC_SHIFT, MT6701_CRC6_BITS);
	frame->crc_valid = (frame->crc_received == frame->crc_calculated);

	return 0;
}

/*
 * RPM formula: rpm = delta_angle / 16384 * 60 / (dt_ns / 1e9) = delta_angle * 60000000000 / (16384 * dt_ns)
 *
 * EMA (when enable_filter=true): rpm = rpm*(1-alpha) + instant*alpha
 *   where alpha = 0.25 (MT6701_RPM_EMA_ALPHA_NUM / MT6701_RPM_EMA_ALPHA_DEN)
 */
s32 mt6701_read_rpm_block(struct mt6701_device *dev, bool enable_filter, unsigned long usecs)
{
	mt6701_ssi_frame_t frame, last_frame;
	ktime_t start;
	s64 target_ns;
	s32 delta_angle;
	s32 rpm_instant;

	mt6701_read_angle_ssi(dev, &last_frame);

	target_ns = (u64)usecs * NSEC_PER_USEC;
	start = ktime_get();
	do {
	} while (ktime_before(ktime_get(), ktime_add_ns(start, target_ns)));

	mt6701_read_angle_ssi(dev, &frame);

	/* Handle angle wraparound [0, 16383] */
	delta_angle = (s32)frame.raw_angle - (s32)last_frame.raw_angle;
	if (delta_angle < -8192)
		delta_angle += MT6701_ANGLE_RESOLUTION;
	if (delta_angle > 8192)
		delta_angle -= MT6701_ANGLE_RESOLUTION;

	/* Force s64 to prevent overflow when usecs > 262144 (~0.26s),  60 * 1e6 for RPM from us deltas */
	rpm_instant = (s32)((s64)delta_angle * 60000000LL / ((s64)MT6701_ANGLE_RESOLUTION * usecs));

	if (enable_filter) {
		/* EMA: rpm = rpm*(1-alpha) + instant*alpha, alpha=0.25 */
		dev->rpm_filtered = (dev->rpm_filtered * (MT6701_RPM_EMA_ALPHA_DEN - MT6701_RPM_EMA_ALPHA_NUM) +
			rpm_instant * MT6701_RPM_EMA_ALPHA_NUM) / MT6701_RPM_EMA_ALPHA_DEN;
		return dev->rpm_filtered;
	}

	return rpm_instant;
}

/**
 * mt6701_read_register - Read configuration register (I2C path)
 * @dev: MT6701 device pointer
 * @reg: Register address
 * @val: Output: register value
 *
 * Return: 0 on success, negative on error
 */
int mt6701_read_register(struct mt6701_device *dev, u8 reg, u8 *val)
{
	u32 tmp;
	int ret;

	if (!dev || !val)
		return -EINVAL;

	if (dev->bus_type != REGMAP_BUS_IIC_SOFT) {
		pr_err("register read requires I2C bus\n");
		return -ENOTSUPP;
	}

	ret = regmap_read(dev->regmap, reg, &tmp);
	if (ret) {
		pr_err("failed to read register 0x%02x: %d\n", reg, ret);
		return ret;
	}

	*val = tmp & 0xFF;
	return 0;
}

/**
 * mt6701_write_register - Write configuration register (I2C path)
 * @dev: MT6701 device pointer
 * @reg: Register address
 * @val: Value to write
 *
 * Note: This does NOT trigger EEPROM programming.
 * Call mt6701_eeprom_program() to persist.
 *
 * Return: 0 on success, negative on error
 */
int mt6701_write_register(struct mt6701_device *dev, u8 reg, u8 val)
{
	int ret;

	if (!dev)
		return -EINVAL;

	if (dev->bus_type != REGMAP_BUS_IIC_SOFT) {
		pr_err("register write requires I2C bus\n");
		return -ENOTSUPP;
	}

	ret = regmap_write(dev->regmap, reg, val);
	if (ret) {
		pr_err("failed to write register 0x%02x = 0x%02x: %d\n", reg, val, ret);
		return ret;
	}

	return 0;
}

/**
 * mt6701_eeprom_read_config - Read EEPROM configuration register
 * @dev: MT6701 device pointer
 * @reg: Register address
 * @val: Output: current register value
 *
 * Return: 0 on success, negative on error
 */
int mt6701_eeprom_read_config(struct mt6701_device *dev, u8 reg, u8 *val)
{
	return mt6701_read_register(dev, reg, val);
}

/**
 * mt6701_eeprom_write_config - Write config register with read-modify-write
 * @dev: MT6701 device pointer
 * @reg: Target register address
 * @val: Value to write
 *
 * Performs read-modify-write to protect reserved bits from being altered.
 * Writing does NOT trigger EEPROM programming automatically.
 * Call mt6701_eeprom_program() to persist.
 *
 * Datasheet warning: Reserved register bits must NOT be changed!
 *
 * Return: 0 on success, negative on error
 */
int mt6701_eeprom_write_config(struct mt6701_device *dev, u8 reg, u8 val)
{
	u8 current_val;
	u8 rsv_mask;
	int ret;

	if (!dev)
		return -EINVAL;

	if (dev->bus_type != REGMAP_BUS_IIC_SOFT) {
		pr_err("EEPROM config write requires I2C bus\n");
		return -ENOTSUPP;
	}

	/* Read current register value */
	ret = mt6701_read_register(dev, reg, &current_val);
	if (ret)
		return ret;

	/* Determine reserved bit mask */
	switch (reg) {
	case 0x25:
		rsv_mask = MT6701_REG_25_RSV_MASK;
		break;
	case 0x29:
		rsv_mask = MT6701_REG_29_RSV_MASK;
		break;
	case 0x30:
		rsv_mask = MT6701_REG_30_RSV_MASK;
		break;
	case 0x38:
		rsv_mask = MT6701_REG_38_RSV_MASK;
		break;
	default:
		rsv_mask = 0x00;  /* no reserved bits */
		break;
	}

	/* Read-modify-write: preserve reserved bits, modify only target bits */
	val = (current_val & rsv_mask) | (val & ~rsv_mask);

	pr_debug("eeprom_write_config: reg=0x%02x current_val=0x%02x write=0x%02x\n",
		 reg, current_val, val);

	return mt6701_write_register(dev, reg, val);
}

/**
 * mt6701_eeprom_program - Execute EEPROM programming
 * @dev: MT6701 device pointer
 *
 * EEPROM program sequence:
 *   1. Write 0xB3 to register 0x09 (program key)
 *   2. Write 0x05 to register 0x0A (program command)
 *   3. Wait >600ms (chip must not be accessed during programming)
 *   4. Power-cycle and verify recommended
 *
 * Note: VDD must be > 4.5V during EEPROM programming!
 *
 * Return: 0 on success, negative on error
 */
int mt6701_eeprom_program(struct mt6701_device *dev)
{
	int ret;

	if (!dev)
		return -EINVAL;

	if (dev->bus_type != REGMAP_BUS_IIC_SOFT) {
		pr_err("EEPROM program requires I2C bus\n");
		return -ENOTSUPP;
	}

	pr_info("starting EEPROM programming...\n");
	pr_info("ensure VDD > 4.5V during programming!\n");

	/* Step 1: Write program key */
	ret = regmap_write(dev->regmap, MT6701_REG_EEPROM_KEY, MT6701_EEPROM_KEY_VAL);
	if (ret) {
		pr_err("failed to write EEPROM key: %d\n", ret);
		return ret;
	}

	/* Step 2: Write program command */
	ret = regmap_write(dev->regmap, MT6701_REG_EEPROM_CMD, MT6701_EEPROM_CMD_VAL);
	if (ret) {
		pr_err("failed to write EEPROM command: %d\n", ret);
		return ret;
	}

	/* Step 3: Wait for programming to complete (600ms + margin), chip must not be accessed */
	pr_info("waiting %dms for EEPROM programming...\n", MT6701_EEPROM_PROGRAM_MS);
	mdelay(MT6701_EEPROM_PROGRAM_MS);

	pr_info("EEPROM programming complete. Power-cycle and verify.\n");

	return 0;
}

#ifdef KINETIS_FAKE_SIM

/**
 * mt6701_reg_random_thread - Simulation thread main loop
 * @arg: mt6701_device pointer
 *
 * Function:
 * - Initialize I2C slave register defaults (slave_regs)
 * - Generate simulated angle data, updating both I2C registers (0x03/0x04)
 *   and SSI frame buffer
 * - Angle cycles from 0° to 360°, stepping 1 LSB, period 1ms
 * - Adds ±2 LSB Gaussian noise
 * - Sporadic anomaly simulation (magnetic status errors, CRC errors)
 *
 * I2C register initialization:
 * - 0x03 ANGLE_H: real-time angle high byte
 * - 0x04 ANGLE_L: real-time angle low byte + reserved bits = 0
 * - 0x09 EEPROM_KEY: 0x00 (unlocked)
 * - 0x0A EEPROM_CMD: 0x00 (idle)
 * - 0x25,0x29-0x34,0x38,0x3E-0x40: config registers = initial defaults
 *
 * Return: NULL
 */
static void *mt6701_reg_random_thread(void *arg)
{
	struct mt6701_device *dev = (struct mt6701_device *)arg;
	u16 angle = 0;
	s32 noise;
	u32 ssi_data_bits;
	u8 crc_val;
	u8 mg_status;
	u32 random_val;

	/* ---- Initialize I2C slave register defaults ---- */

	/* Angle registers initialized to 0 */
	dev->slave_regs[0x03] = 0x00;
	dev->slave_regs[0x04] = 0x00;

	/* EEPROM key/command registers */
	dev->slave_regs[0x09] = 0x00;
	dev->slave_regs[0x0A] = 0x00;

	/* Configuration register defaults */
	dev->slave_regs[0x25] = 0x00;  /* UVW_MUX = 0 (UVW output) */
	dev->slave_regs[0x29] = 0x00;  /* ABZ_MUX=ABZ, DIR=CCW */
	dev->slave_regs[0x30] = 0x00;  /* UVW_RES=1, ABZ_RES=0 */
	dev->slave_regs[0x31] = 0x01;  /* ABZ_RES=1 pulse/rev */
	dev->slave_regs[0x32] = 0x00;  /* HYST[2]=0, Z_PULSE_WIDTH=1LSB, ZERO=0 */
	dev->slave_regs[0x33] = 0x00;  /* ZERO=0 */
	dev->slave_regs[0x34] = 0x00;  /* HYST[1:0]=0 (1 LSB hysteresis) */
	dev->slave_regs[0x38] = 0x00;  /* PWM_FREQ=994.4Hz, PWM_POL=active high, OUT_MODE=analog */
	dev->slave_regs[0x3E] = 0x00;  /* A_STOP/A_START high 4 bits = 0 */
	dev->slave_regs[0x3F] = 0x00;  /* A_START=0deg */
	dev->slave_regs[0x40] = 0x00;  /* A_STOP=0deg */

	while (dev->thread_running) {
		/* Generate random noise (approximately Gaussian distribution) */
		noise = ((s32)(get_random_u32() & 0xF) - 8) * MT6701_SIM_NOISE_STDEV / 4;
		angle = (u16)((u32)(angle + MT6701_SIM_ANGLE_STEP + noise) % MT6701_ANGLE_RESOLUTION);

		/* Update I2C registers */
		dev->slave_regs[MT6701_REG_ANGLE_H] = (angle >> 6) & 0xFF;
		dev->slave_regs[MT6701_REG_ANGLE_L] = angle & 0x3F;  /* bits 5-0, bits 7-6 reserved = 00 */

		/* ---- Update SSI frame buffer ---- */

		/* Default magnetic status: normal */
		mg_status = 0x00;

		/* Sporadic anomaly simulation: ~0.1% probability */
		random_val = get_random_u32();
		if ((random_val & MT6701_SIM_ANOMALY_PROB) == 0) {
			u8 anomaly_type = random_val & 0x07;
			switch (anomaly_type) {
			case 0:
				mg_status = 0x01;  /* Magnetic field too strong */
				break;
			case 1:
				mg_status = 0x02;  /* Magnetic field too weak */
				break;
			case 2:
				mg_status = 0x04;  /* Knob pressed */
				break;
			case 3:
				mg_status = 0x08;  /* Overspeed */
				break;
			default:
				break;
			}
			pr_debug("sim anomaly: mg=0x%x, angle=%u\n", mg_status, angle);
		}

		/* Pack 24-bit SSI frame: D[13:0] + Mg[3:0] */
		ssi_data_bits = ((u32)angle << 4) | mg_status;

		/* Compute CRC-6 */
		crc_val = mt6701_crc6_compute(ssi_data_bits, MT6701_CRC6_BITS);

		/* Assemble complete 24-bit frame: D[13:0](14bits) + Mg[3:0](4bits) + CRC[5:0](6bits) */
		{
			u32 full_frame = (ssi_data_bits << SSI_CRC_SHIFT) | crc_val;
			dev->ssi_frame_buf[0] = (full_frame >> 16) & 0xFF;
			dev->ssi_frame_buf[1] = (full_frame >> 8) & 0xFF;
			dev->ssi_frame_buf[2] = full_frame & 0xFF;
		}

		msleep(MT6701_SIM_UPDATE_PERIOD_MS);
	}

	return NULL;
}

static pthread_t reg_thread;

/**
 * mt6701_start_reg_random - Start simulation angle generation thread
 * @dev: MT6701 device pointer
 *
 * Return: 0 on success, negative on error
 */
static int mt6701_start_reg_random(struct mt6701_device *dev)
{
	int ret;

	dev->thread_running = true;

	ret = pthread_create(&reg_thread, NULL,
			     mt6701_reg_random_thread, dev);
	if (ret) {
		pr_err("failed to create simulation thread: %d\n", ret);
		dev->thread_running = false;
		return -ret;
	}

	/* Wait for thread initialization to complete */
	msleep(100);

	return 0;
}

/**
 * mt6701_stop_reg_random - Stop simulation angle generation thread
 * @dev: MT6701 device pointer
 */
static void mt6701_stop_reg_random(struct mt6701_device *dev)
{
	dev->thread_running = false;
	pthread_join(reg_thread, NULL);
}

#endif /* KINETIS_FAKE_SIM */

/**
 * I2C path volatile ranges: ANGLE_H(0x03) ~ ANGLE_L(0x04)
 *
 * Angle data registers change at any time. They must be marked as volatile
 * to ensure each regmap_read fetches the latest value, bypassing cache.
 */
static const struct regmap_range mt6701_volatile_ranges[] = {
	regmap_reg_range(MT6701_REG_ANGLE_H, MT6701_REG_ANGLE_L),
};

static const struct regmap_access_table mt6701_volatile_table = {
	.yes_ranges = mt6701_volatile_ranges,
	.n_yes_ranges = ARRAY_SIZE(mt6701_volatile_ranges),
};

/** I2C path regmap configuration */
static const struct regmap_config mt6701_i2c_regmap_config = {
	.reg_bits = 8,
	.val_bits = 8,
	.max_register = MT6701_REG_MAX,
	.volatile_table = &mt6701_volatile_table,
	.cache_type = REGCACHE_NONE,
};

/** SSI path regmap configuration (virtual register 0x00 only) */
static const struct regmap_config mt6701_ssi_regmap_config = {
	.reg_bits = 8,
	.val_bits = 24,
	.max_register = MT6701_SSI_ANGLE_REG,
	.cache_type = REGCACHE_NONE,
};

/**
 * mt6701_init - Initialize MT6701 device
 * @bus_type:   Bus type (REGMAP_BUS_IIC_SOFT or REGMAP_BUS_SPI_SOFT i.e. SSI)
 * @bus_master: I2C: &iic1_master, SPI: &spi1_master
 *
 * I2C mode: register regmap + I2C slave simulation + start simulation thread
 * SSI mode: register custom SSI regmap + SPI slave simulation + start simulation thread
 *
 * Return: device pointer on success, ERR_PTR/NULL on failure
 */
struct mt6701_device *mt6701_init(enum regmap_user_bus_type bus_type, void *bus_master)
{
	struct mt6701_device *dev;
	int ret;

	dev = kzalloc(sizeof(*dev), GFP_KERNEL);
	if (!dev) {
		return ERR_PTR(-ENOMEM);
	}

	dev->bus_type = bus_type;

	if (bus_type == REGMAP_BUS_IIC_SOFT) {
		struct iic_master *i2c_master = (struct iic_master *)bus_master;

		dev->regmap = regmap_init_iic_soft(i2c_master, MT6701_I2C_ADDR_DEFAULT,
						    &mt6701_i2c_regmap_config);
		if (IS_ERR(dev->regmap)) {
			pr_err("failed to init I2C regmap\n");
			kfree(dev);
			return NULL;
		}

#ifdef KINETIS_FAKE_SIM
		dev->slave_regs = kmalloc(mt6701_i2c_regmap_config.max_register + 1, GFP_KERNEL);
		if (!dev->slave_regs) {
			regmap_exit(dev->regmap);
			kfree(dev);
			return ERR_PTR(-ENOMEM);
		}
		memset(dev->slave_regs, 0, mt6701_i2c_regmap_config.max_register + 1);

		ret = mt6701_start_reg_random(dev);
		if (ret) {
			pr_err("failed to start simulation thread\n");
			kfree(dev->slave_regs);
			regmap_exit(dev->regmap);
			kfree(dev);
			return NULL;
		}

		dev->iic_slave = iic_slave_soft_init("mt6701", MT6701_I2C_ADDR_DEFAULT,
						      dev->slave_regs,
						      mt6701_i2c_regmap_config.max_register + 1);
		if (IS_ERR(dev->iic_slave)) {
			pr_err("failed to init I2C slave for mt6701\n");
			mt6701_stop_reg_random(dev);
			kfree(dev->slave_regs);
			regmap_exit(dev->regmap);
			kfree(dev);
			return NULL;
		}
#endif
		pr_info("mt6701 initialized on I2C (addr=0x%02x)\n", MT6701_I2C_ADDR_DEFAULT);

	} else if (bus_type == REGMAP_BUS_SPI_SOFT) {
		struct spi_master *spi_master = (struct spi_master *)bus_master;
		u8 *frame_buf = NULL;

#ifdef KINETIS_FAKE_SIM
		frame_buf = dev->ssi_frame_buf;
#endif

		dev->regmap = mt6701_init_ssi_regmap(spi_master, frame_buf,
						      &mt6701_ssi_regmap_config);
		if (IS_ERR(dev->regmap)) {
			pr_err("failed to init SSI regmap\n");
			kfree(dev);
			return NULL;
		}

#ifdef KINETIS_FAKE_SIM
		/* Simulation mode: no slave_regs needed (SSI is read-only), but need thread for frame gen */
		ret = mt6701_start_reg_random(dev);
		if (ret) {
			pr_err("failed to start SSI simulation thread\n");
			regmap_exit(dev->regmap);
			kfree(dev);
			return NULL;
		}

		/* Register SPI slave (for simulation framework's SPI path, though SSI bypasses it) */
		dev->spi_slave = spi_slave_soft_init("mt6701_ssi", 0, 0, SPI_BIT_ORDER_MSB,
						      dev->ssi_frame_buf, SSI_FRAME_BYTES);
		if (IS_ERR(dev->spi_slave)) {
			pr_err("failed to init SPI slave for mt6701 SSI\n");
			mt6701_stop_reg_random(dev);
			regmap_exit(dev->regmap);
			kfree(dev);
			return NULL;
		}
#endif
		pr_info("mt6701 initialized on SSI (SPI mode)\n");

	} else {
		pr_err("unsupported bus type: %d\n", bus_type);
		kfree(dev);
		return ERR_PTR(-EINVAL);
	}

	return dev;
}

/**
 * mt6701_exit - Release MT6701 device resources
 * @dev: MT6701 device pointer
 */
void mt6701_exit(struct mt6701_device *dev)
{
	if (!dev)
		return;

#ifdef KINETIS_FAKE_SIM
	mt6701_stop_reg_random(dev);

	if (dev->iic_slave) {
		iic_slave_soft_exit(dev->iic_slave);
		dev->iic_slave = NULL;
	}

	if (dev->spi_slave) {
		spi_slave_soft_exit(dev->spi_slave);
		dev->spi_slave = NULL;
	}

	kfree(dev->slave_regs);
	dev->slave_regs = NULL;
#endif

	regmap_exit(dev->regmap);
	dev->regmap = NULL;

	kfree(dev);
	pr_info("mt6701 device released\n");
}

#ifdef DESIGN_VERIFICATION_MT6701

static struct mt6701_device *mt6701_dev;

/**
 * t_mt6701_initialize - Initialize MT6701 device
 * @argc: Argument count
 * @argv: Argument array
 *   argv[1]: "on" | "off"  start/stop
 *   argv[2]: "i2c" | "ssi" bus type (default i2c)
 *
 * Return: 0 on success, negative on error
 */
int t_mt6701_initialize(int argc, char **argv)
{
	bool on_off = true;
	enum regmap_user_bus_type bus_type = REGMAP_BUS_IIC_SOFT;

	if (argc > 1) {
		if (!strcmp(argv[1], "on")) {
			on_off = true;
		} else if (!strcmp(argv[1], "off")) {
			on_off = false;
		} else {
			pr_err("usage: mt6701_init on|off [i2c|ssi]\n");
			return -EINVAL;
		}
	}

	if (argc > 2) {
		if (!strcmp(argv[2], "ssi")) {
			bus_type = REGMAP_BUS_SPI_SOFT;
		} else if (!strcmp(argv[2], "i2c")) {
			bus_type = REGMAP_BUS_IIC_SOFT;
		} else {
			pr_err("invalid bus type: %s (use 'i2c' or 'ssi')\n", argv[2]);
			return -EINVAL;
		}
	}

	if (on_off) {
		pr_info("starting mt6701 with %s mode\n", bus_type == REGMAP_BUS_IIC_SOFT ? "i2c" : "ssi");
		mt6701_dev = mt6701_init(bus_type,
					  bus_type == REGMAP_BUS_IIC_SOFT ?
					  (void *)&iic1_master :
					  (void *)&spi1_master);
		if (IS_ERR_OR_NULL(mt6701_dev)) {
			pr_err("mt6701 init failed\n");
			return -EINVAL;
		}
		return 0;
	}

	mt6701_exit(mt6701_dev);
	mt6701_dev = NULL;
	return 0;
}

/**
 * t_mt6701_read_angle_i2c - Read angle via I2C
 * @argc: Argument count
 * @argv: argv[1]: read count (default 10)
 *
 * Return: 0 on success, negative on error
 */
int t_mt6701_read_angle_i2c(int argc, char **argv)
{
	struct mt6701_device *dev = mt6701_dev;
	mt6701_angle_data_t data;
	float degrees;
	u16 times = 10;
	u16 i;
	int ret;

	if (!dev) {
		pr_err("mt6701 not initialized\n");
		return -EINVAL;
	}

	if (dev->bus_type != REGMAP_BUS_IIC_SOFT) {
		pr_err("device is not in I2C mode\n");
		return -ENOTSUPP;
	}

	if (argc > 1) {
		times = (u16)simple_strtoul(argv[1], &argv[1], 10);
	}

	for (i = 0; i < times; i++) {
		ret = mt6701_read_angle_raw(dev, &data);
		if (ret) {
			pr_err("read angle raw failed at %d: %d\n", i, ret);
			return ret;
		}

		degrees = (float)data.raw_angle / (float)MT6701_ANGLE_RESOLUTION * 360.0f;

		pr_info("[%3d/%3d] raw=0x%04X (%5u) angle=%.2f° reserved=%u\n",
			i + 1, times, data.raw_angle, data.raw_angle,
			degrees, data.reserved);

		if (times > 1)
			mdelay(100);
	}

	return 0;
}

/**
 * t_mt6701_read_angle_ssi - Read angle via SSI (high-speed mode)
 * @argc: Argument count
 * @argv: argv[1]: read count (default 10)
 *
 * Return: 0 on success, negative on error
 */
int t_mt6701_read_angle_ssi(int argc, char **argv)
{
	struct mt6701_device *dev = mt6701_dev;
	mt6701_ssi_frame_t frame;
	u16 times = 10;
	u16 i;
	int ret;

	if (!dev) {
		pr_err("mt6701 not initialized\n");
		return -EINVAL;
	}

	if (argc > 1) {
		times = (u16)simple_strtoul(argv[1], &argv[1], 10);
	}

	for (i = 0; i < times; i++) {
		ret = mt6701_read_angle_ssi(dev, &frame);
		if (ret) {
			pr_err("SSI read failed at %d: %d\n", i, ret);
			return ret;
		}

		pr_info("[%3d/%3d] raw=0x%04X angle=%.2f° "
			"mg=%u push=%u overspeed=%u "
			"crc_rx=0x%02x crc_calc=0x%02x %s\n",
			i + 1, times,
			frame.raw_angle, frame.angle_degrees,
			frame.mag_status, frame.push_status, frame.overspeed_status,
			frame.crc_received, frame.crc_calculated,
			frame.crc_valid ? "OK" : "FAIL");

		if (times > 1)
			mdelay(50);
	}

	return 0;
}

/**
 * t_mt6701_config - Read/write configuration registers
 * @argc: Argument count
 * @argv:
 *   argv[1]: "read" | "write"
 *   argv[2]: Register address (hex, e.g. 0x25)
 *   argv[3]: Value to write (write only)
 *
 * Return: 0 on success, negative on error
 */
int t_mt6701_config(int argc, char **argv)
{
	struct mt6701_device *dev = mt6701_dev;
	u8 reg, val;
	int ret;

	if (!dev) {
		pr_err("mt6701 not initialized\n");
		return -EINVAL;
	}

	if (dev->bus_type != REGMAP_BUS_IIC_SOFT) {
		pr_err("config only available on I2C bus\n");
		return -ENOTSUPP;
	}

	if (argc < 3) {
		pr_info("usage: mt6701_config read|write <reg> [val]\n");
		return -EINVAL;
	}

	reg = (u8)simple_strtoul(argv[2], NULL, 16);

	if (!strcmp(argv[1], "read")) {
		ret = mt6701_read_register(dev, reg, &val);
		if (ret) {
			pr_err("read register 0x%02x failed: %d\n", reg, ret);
			return ret;
		}
		pr_info("register 0x%02x = 0x%02x (%u)\n", reg, val, val);

	} else if (!strcmp(argv[1], "write")) {
		if (argc < 4) {
			pr_err("write requires val parameter\n");
			return -EINVAL;
		}
		val = (u8)simple_strtoul(argv[3], NULL, 16);

		ret = mt6701_eeprom_write_config(dev, reg, val);
		if (ret) {
			pr_err("write register 0x%02x = 0x%02x failed: %d\n", reg, val, ret);
			return ret;
		}
		pr_info("wrote register 0x%02x = 0x%02x\n", reg, val);

	} else {
		pr_err("unknown config command: %s\n", argv[1]);
		return -EINVAL;
	}

	return 0;
}

/**
 * t_mt6701_eeprom - EEPROM program and verify
 * @argc: Argument count
 * @argv: argv[1]: "program" execute programming, "verify" verify config
 *
 * Return: 0 on success, negative on error
 */
int t_mt6701_eeprom(int argc, char **argv)
{
	struct mt6701_device *dev = mt6701_dev;
	int ret;

	if (!dev) {
		pr_err("mt6701 not initialized\n");
		return -EINVAL;
	}

	if (dev->bus_type != REGMAP_BUS_IIC_SOFT) {
		pr_err("EEPROM only available on I2C bus\n");
		return -ENOTSUPP;
	}

	if (argc < 2) {
		pr_info("usage: mt6701_eeprom program|verify\n");
		return -EINVAL;
	}

	if (!strcmp(argv[1], "program")) {
		pr_info("=== EEPROM PROGRAM START ===\n");
		pr_info("ensure VDD > 4.5V during programming!\n");

		ret = mt6701_eeprom_program(dev);
		if (ret) {
			pr_err("EEPROM programming failed: %d\n", ret);
			return ret;
		}
		pr_info("=== EEPROM PROGRAM COMPLETE ===\n");
		pr_info("please power-cycle and verify\n");

	} else if (!strcmp(argv[1], "verify")) {
		u8 regs[] = {0x25, 0x29, 0x30, 0x31, 0x32, 0x33, 0x34, 0x38, 0x3E, 0x3F, 0x40};
		u8 i;
		u8 val;

		pr_info("=== EEPROM CONFIG VERIFICATION ===\n");
		for (i = 0; i < ARRAY_SIZE(regs); i++) {
			ret = mt6701_eeprom_read_config(dev, regs[i], &val);
			if (ret) {
				pr_err("read config reg 0x%02x failed: %d\n", regs[i], ret);
			} else {
				pr_info("  reg 0x%02x = 0x%02x\n", regs[i], val);
			}
		}

	} else {
		pr_err("unknown eeprom command: %s\n", argv[1]);
		return -EINVAL;
	}

	return 0;
}

/**
 * t_mt6701_crc_test - CRC-6 computation test
 *
 * Uses known test vectors to verify CRC-6 computation correctness.
 *
 * Return: 0 on success, negative on error
 */
int t_mt6701_crc_test(int argc, char **argv)
{
	struct {
		u32 data;
		u8 bits;
		u8 expected;
	} test_vectors[] = {
		{0x00000, 18, 0x00},  /* All zero data */
		{0x3FFFF, 18, 0x3F},  /* All one data (max 18-bit) */
		{(0x0800 << 4) | 0x00, 18, 0x00},  /* angle=2048, mg=0 */
	};

	u8 i;
	u8 crc;
	int errors = 0;

	pr_info("=== CRC-6 Test ===\n");
	for (i = 0; i < ARRAY_SIZE(test_vectors); i++) {
		crc = mt6701_crc6_compute(test_vectors[i].data, test_vectors[i].bits);
		pr_info("[%d] data=0x%05X bits=%d crc=0x%02X expected=0x%02X %s\n",
			i, test_vectors[i].data, test_vectors[i].bits,
			crc, test_vectors[i].expected,
			(crc == test_vectors[i].expected) ? "OK" : "FAIL");
		if (crc != test_vectors[i].expected)
			errors++;
	}

	if (errors == 0)
		pr_info("all CRC tests passed\n");
	else
		pr_err("%d CRC tests failed\n", errors);

	return (errors == 0) ? 0 : -EINVAL;
}

#endif /* DESIGN_VERIFICATION_MT6701 */
