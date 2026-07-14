#ifndef __TLC5971_H
#define __TLC5971_H

#ifdef __cplusplus
extern "C" {
#endif

#include <linux/types.h>

#include "kinetis/spi_soft.h"

#define TLC5971_LEDS_PER_DEVICE 4
#define TLC5971_PACKET_BYTES    28

/* Per-LED grayscale values (16-bit, 0-65535). */
struct tlc5971_led {
	u16 r;
	u16 g;
	u16 b;
};

/* Per-device slot: 4 RGB LEDs. */
struct tlc5971_slot {
	struct tlc5971_led led[TLC5971_LEDS_PER_DEVICE];
};

/*
 * Supports daisy-chained configurations (num_devices > 1).
 * Data is shifted MSB-first: the last device in the chain receives the first
 * packet serialized into the buffer.
 */
struct tlc5971_device {
	struct spi_master *spi;
	u32 num_devices;

	u8 outtmg;  /* GS ref clock edge (0=falling, 1=rising) */
	u8 extgck;  /* GS ref clock src (0=internal, 1=SCKI) */
	u8 tmgrst;  /* display timing reset (0=off, 1=on) */
	u8 dsprpt;  /* auto display repeat (0=off, 1=on) */
	u8 blank;   /* 0=outputs enabled, 1=all outputs off */

	/* Global brightness control (0-127 each). */
	u8 bcr;
	u8 bcg;
	u8 bcb;

	/* SCKI period in ns, used to calculate the post-transfer latch gap. */
	u32 sck_period_ns;

	/* Per-device grayscale data, kzalloc'd in tlc5971_init(). */
	struct tlc5971_slot *slots;

	/* Pre-allocated SPI frame buffer: num_devices * TLC5971_PACKET_BYTES. */
	u8 *frame_buf;
};

int tlc5971_init(struct tlc5971_device *dev, struct spi_master *spi,
	u32 num_devices);
void tlc5971_exit(struct tlc5971_device *dev);

void tlc5971_set_fc(struct tlc5971_device *dev,
	u8 dsprpt, u8 tmgrst, u8 extgck, u8 outtmg, u8 blank);
void tlc5971_set_brightness_control(struct tlc5971_device *dev,
	u8 bcr, u8 bcg, u8 bcb);
void tlc5971_set_blank(struct tlc5971_device *dev, u8 blank);
int tlc5971_set_sck_period_ns(struct tlc5971_device *dev, u32 period_ns);

int tlc5971_set_led(struct tlc5971_device *dev,
	u32 dev_idx, u8 led_idx,
	u16 r, u16 g, u16 b);
void tlc5971_set_all_leds(struct tlc5971_device *dev,
	u16 r, u16 g, u16 b);

int tlc5971_send(struct tlc5971_device *dev);
int tlc5971_send_internal_repeat(struct tlc5971_device *dev);
int tlc5971_send_external_full_period(struct tlc5971_device *dev);
int tlc5971_send_external_long_chain(struct tlc5971_device *dev);
int tlc5971_send_timing1(struct tlc5971_device *dev);
int tlc5971_send_timing2(struct tlc5971_device *dev);
int tlc5971_send_timing3(struct tlc5971_device *dev);
int tlc5971_sync_with_scki(struct tlc5971_device *dev);

void tlc5971_pack_chip(const u16 gs_r[TLC5971_LEDS_PER_DEVICE],
					  const u16 gs_g[TLC5971_LEDS_PER_DEVICE],
					  const u16 gs_b[TLC5971_LEDS_PER_DEVICE], u8 *out);
int tlc5971_pack_chip_fc(const u16 gs_r[TLC5971_LEDS_PER_DEVICE],
						 const u16 gs_g[TLC5971_LEDS_PER_DEVICE],
						 const u16 gs_b[TLC5971_LEDS_PER_DEVICE],
						 u8 outtmg, u8 extgck, u8 tmgrst, u8 dsprpt,
						 u8 blank, u8 bcr, u8 bcg, u8 bcb, u8 *out);

/* Map 8-bit channel value to 16-bit GS: 0 -> 0x0000, 255 -> 0xFFFF. */
static inline u16 tlc5971_rgb8_to_gs(u8 val)
{
	return (u16)val << 8 | (u16)val;
}

#ifdef __cplusplus
}
#endif

#endif /* __TLC5971_H */