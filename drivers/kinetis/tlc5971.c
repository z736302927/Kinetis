#define pr_fmt(fmt) "tlc5971: " fmt

#include <linux/printk.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/errno.h>
#include <linux/string.h>

#include <kinetis/design_verification.h>
#include <kinetis/tlc5971.h>

/*
 * TLC5971 - 12-channel (4 RGB LEDs), 16-bit grayscale, SPI LED driver.
 *
 * 224-bit packet layout, MSB first, byte 0 first on wire:
 *
 *   byte 0 : WRITE[5:0]=0x25 OUTTMG EXTGCK
 *   byte 1 : TMGRST DSPRPT BLANK BCB[6:2]
 *   byte 2 : BCB[1:0] BCG[6:1]
 *   byte 3 : BCG[0] BCR[6:0]
 *   bytes 4-5   : GS3B[15:0]
 *   bytes 6-7   : GS3G[15:0]
 *   bytes 8-9   : GS3R[15:0]
 *   bytes 10-11 : GS2B[15:0]
 *   bytes 12-13 : GS2G[15:0]
 *   bytes 14-15 : GS2R[15:0]
 *   bytes 16-17 : GS1B[15:0]
 *   bytes 18-19 : GS1G[15:0]
 *   bytes 20-21 : GS1R[15:0]
 *   bytes 22-23 : GS0B[15:0]
 *   bytes 24-25 : GS0G[15:0]
 *   bytes 26-27 : GS0R[15:0]
 *
 * Daisy-chain: first packet in the transmitted buffer reaches the last
 * physical device in the chain.
 */

#define TLC5971_WRITE_CMD           0x25
#define TLC5971_MAX_WRITE_BYTES     255
#define TLC5971_DEFAULT_SCK_NS      50      /* 20 MHz */
#define TLC5971_LATCH_GAP_NS        1340    /* Datasheet minimum after latch timing. */

static const u8 tlc5971_zero_buf[TLC5971_MAX_WRITE_BYTES];

static int tlc5971_write_buf(struct tlc5971_device *dev, const u8 *buf, u32 len)
{
    u32 offset = 0;
    int ret;

    if (!buf && len)
        return -EINVAL;

    while (offset < len) {
        u32 remaining = len - offset;
        u32 chunk = remaining < TLC5971_MAX_WRITE_BYTES ? remaining : TLC5971_MAX_WRITE_BYTES;

        ret = dev->spi->write_bytes((u8 *)buf + offset, (u8)chunk);
        if (ret)
            return ret;

        offset += chunk;
    }

    return 0;
}

/*
 * Build the 4-byte FC+BC header common to all serialized paths.
 *   byte0 = WRITE<<2 | OUTTMG<<1 | EXTGCK
 *   byte1 = TMGRST<<7 | DSPRPT<<6 | BLANK<<5 | BCB[6:2]
 *   byte2 = BCB[1:0]<<6 | BCG[6:1]
 *   byte3 = BCG[0]<<7 | BCR[6:0]
 */
static inline void tlc5971_pack_header(u8 outtmg, u8 extgck,
                                       u8 tmgrst, u8 dsprpt, u8 blank,
                                       u8 bcb, u8 bcg, u8 bcr, u8 *out)
{
    out[0] = (u8)((TLC5971_WRITE_CMD << 2) | (!!outtmg << 1) | !!extgck);
    out[1] = (u8)((!!tmgrst << 7) | (!!dsprpt << 6) | (!!blank << 5) | ((bcb & 0x7F) >> 2));
    out[2] = (u8)(((bcb & 0x03) << 6) | ((bcg & 0x7F) >> 1));
    out[3] = (u8)(((bcg & 0x01) << 7) | (bcr & 0x7F));
}

/* Pack 4 * (B,G,R) 16-bit GS values into bytes 4..27, big-endian. */
static inline void tlc5971_pack_gs(const u16 gs_r[TLC5971_LEDS_PER_DEVICE],
                                   const u16 gs_g[TLC5971_LEDS_PER_DEVICE],
                                   const u16 gs_b[TLC5971_LEDS_PER_DEVICE],
                                   u8 *out)
{
    int i;

    for (i = TLC5971_LEDS_PER_DEVICE - 1; i >= 0; i--) {
        int off = 4 + (TLC5971_LEDS_PER_DEVICE - 1 - i) * 6;

        out[off + 0] = (u8)(gs_b[i] >> 8);
        out[off + 1] = (u8)(gs_b[i] & 0xFF);
        out[off + 2] = (u8)(gs_g[i] >> 8);
        out[off + 3] = (u8)(gs_g[i] & 0xFF);
        out[off + 4] = (u8)(gs_r[i] >> 8);
        out[off + 5] = (u8)(gs_r[i] & 0xFF);
    }
}

static void tlc5971_serialize(const struct tlc5971_device *dev, u32 dev_idx, u8 *out)
{
    const struct tlc5971_slot *s = &dev->slots[dev_idx];
    u16 gs_r[TLC5971_LEDS_PER_DEVICE];
    u16 gs_g[TLC5971_LEDS_PER_DEVICE];
    u16 gs_b[TLC5971_LEDS_PER_DEVICE];
    int i;

    tlc5971_pack_header(dev->outtmg, dev->extgck,
                        dev->tmgrst, dev->dsprpt, dev->blank,
                        dev->bcb, dev->bcg, dev->bcr, out);

    for (i = 0; i < TLC5971_LEDS_PER_DEVICE; i++) {
        gs_r[i] = s->led[i].r;
        gs_g[i] = s->led[i].g;
        gs_b[i] = s->led[i].b;
    }
    tlc5971_pack_gs(gs_r, gs_g, gs_b, out);
}

int tlc5971_pack_chip_fc(const u16 gs_r[TLC5971_LEDS_PER_DEVICE],
                         const u16 gs_g[TLC5971_LEDS_PER_DEVICE],
                         const u16 gs_b[TLC5971_LEDS_PER_DEVICE],
                         u8 outtmg, u8 extgck, u8 tmgrst, u8 dsprpt,
                         u8 blank, u8 bcr, u8 bcg, u8 bcb, u8 *out)
{
    tlc5971_pack_header(outtmg, extgck, tmgrst, dsprpt, blank,
                        bcb, bcg, bcr, out);
    tlc5971_pack_gs(gs_r, gs_g, gs_b, out);

    return 0;
}

void tlc5971_pack_chip(const u16 gs_r[TLC5971_LEDS_PER_DEVICE],
                       const u16 gs_g[TLC5971_LEDS_PER_DEVICE],
                       const u16 gs_b[TLC5971_LEDS_PER_DEVICE], u8 *out)
{
    tlc5971_pack_chip_fc(gs_r, gs_g, gs_b,
			1, 0, 0, 1, 0, 0x7F, 0x7F, 0x7F, out);
}

static void tlc5971_latch_pulse(u32 sck_period_ns)
{
    u64 wait_ns = (u64)sck_period_ns * 8 + TLC5971_LATCH_GAP_NS;
    u32 wait_us = (u32)((wait_ns + 999) / 1000);

    if (!wait_us)
        wait_us = 1;
    ndelay(wait_ns);
}

static int tlc5971_send_idle_bits(struct tlc5971_device *dev, u32 n_bits)
{
    u32 remaining;
    int ret;

    if (n_bits & 0x7)
        return -EINVAL;

    remaining = n_bits / 8;
    while (remaining) {
        u32 chunk = remaining < sizeof(tlc5971_zero_buf) ?
                    remaining : sizeof(tlc5971_zero_buf);

        ret = tlc5971_write_buf(dev, tlc5971_zero_buf, chunk);
        if (ret)
            return ret;
        remaining -= chunk;
    }

    return 0;
}

int tlc5971_init(struct tlc5971_device *dev, struct spi_master *spi, u32 num_devices)
{
    u32 slots_size;
    u32 frame_size;

    if (!dev || !spi || !num_devices)
        return -EINVAL;
    if ((num_devices > (u32)-1 / sizeof(*dev->slots)) ||
        (num_devices > (u32)-1 / TLC5971_PACKET_BYTES))
        return -EOVERFLOW;

    memset(dev, 0, sizeof(*dev));

    slots_size = num_devices * sizeof(*dev->slots);
    frame_size = num_devices * TLC5971_PACKET_BYTES;

    dev->slots = kzalloc(slots_size, GFP_KERNEL);
    if (!dev->slots)
        return -ENOMEM;

    dev->frame_buf = kzalloc(frame_size, GFP_KERNEL);
    if (!dev->frame_buf) {
        kfree(dev->slots);
        dev->slots = NULL;
        return -ENOMEM;
    }

    dev->spi = spi;
    dev->num_devices = num_devices;
    dev->outtmg = 1;
    dev->extgck = 0;
    dev->tmgrst = 0;
    dev->dsprpt = 0;
    dev->blank = 0;
    dev->bcr = 0x7F;
    dev->bcg = 0x7F;
    dev->bcb = 0x7F;
    dev->sck_period_ns = TLC5971_DEFAULT_SCK_NS;

    pr_info("init: %u device(s)\n", num_devices);

    return 0;
}

void tlc5971_exit(struct tlc5971_device *dev)
{
    if (!dev)
        return;
    kfree(dev->slots);
    kfree(dev->frame_buf);
    memset(dev, 0, sizeof(*dev));
}

void tlc5971_set_fc(struct tlc5971_device *dev,
                    u8 dsprpt, u8 tmgrst, u8 extgck, u8 outtmg, u8 blank)
{
    dev->dsprpt = !!dsprpt;
    dev->tmgrst = !!tmgrst;
    dev->extgck = !!extgck;
    dev->outtmg = !!outtmg;
    dev->blank = !!blank;
}

void tlc5971_set_brightness_control(struct tlc5971_device *dev,
                                    u8 bcr, u8 bcg, u8 bcb)
{
    dev->bcr = bcr & 0x7F;
    dev->bcg = bcg & 0x7F;
    dev->bcb = bcb & 0x7F;
}

void tlc5971_set_blank(struct tlc5971_device *dev, u8 blank)
{
    dev->blank = !!blank;
}

int tlc5971_set_sck_period_ns(struct tlc5971_device *dev, u32 period_ns)
{
    if (!period_ns)
        return -EINVAL;

    dev->sck_period_ns = period_ns;

    return 0;
}

int tlc5971_set_led(struct tlc5971_device *dev,
                    u32 dev_idx, u8 led_idx,
                    u16 r, u16 g, u16 b)
{
    if (dev_idx >= dev->num_devices || led_idx >= TLC5971_LEDS_PER_DEVICE)
        return -EINVAL;

    dev->slots[dev_idx].led[led_idx].r = r;
    dev->slots[dev_idx].led[led_idx].g = g;
    dev->slots[dev_idx].led[led_idx].b = b;

    return 0;
}

void tlc5971_set_all_leds(struct tlc5971_device *dev, u16 r, u16 g, u16 b)
{
    u32 d;
    u8 l;

    for (d = 0; d < dev->num_devices; d++) {
        for (l = 0; l < TLC5971_LEDS_PER_DEVICE; l++) {
            dev->slots[d].led[l].r = r;
            dev->slots[d].led[l].g = g;
            dev->slots[d].led[l].b = b;
        }
    }
}

int tlc5971_send(struct tlc5971_device *dev)
{
    u32 i;
    u32 len;

    if (dev->num_devices > (u32)-1 / TLC5971_PACKET_BYTES)
        return -EOVERFLOW;

    len = dev->num_devices * TLC5971_PACKET_BYTES;
    for (i = 0; i < dev->num_devices; i++)
        tlc5971_serialize(dev, i, dev->frame_buf + i * TLC5971_PACKET_BYTES);

    return tlc5971_write_buf(dev, dev->frame_buf, len);
}

static int tlc5971_send_latched(struct tlc5971_device *dev)
{
    int ret;

	ret = tlc5971_send(dev);
    if (ret)
        return ret;

    tlc5971_latch_pulse(dev->sck_period_ns);

    return 0;
}

int tlc5971_send_internal_repeat(struct tlc5971_device *dev)
{
    /* Internal GS clock, automatic display repeat. */
    tlc5971_set_fc(dev, 1, 0, 0, 1, 0);

    return tlc5971_send_latched(dev);
}

int tlc5971_send_external_full_period(struct tlc5971_device *dev)
{
	int ret;

    /* External SCKI GS clock: restart the GS counter on latch. */
    tlc5971_set_fc(dev, 0, 1, 1, 1, 0);

    ret = tlc5971_send_latched(dev);
    if (ret)
        return ret;

    return tlc5971_send_idle_bits(dev, 65536);
}

int tlc5971_send_external_long_chain(struct tlc5971_device *dev)
{
    u32 shifted_bits;
    u32 idle_bits;
    int ret;

    if (dev->num_devices > (u32)-1 / 224)
        return -EOVERFLOW;

    shifted_bits = dev->num_devices * 224;
    if (shifted_bits > 65536)
        return -ERANGE;

    /* External SCKI GS clock: data clocks count as part of the GS period. */
    tlc5971_set_fc(dev, 0, 1, 1, 1, 0);

    ret = tlc5971_send_latched(dev);
    if (ret)
        return ret;

    idle_bits = 65536 - shifted_bits;
    if (!idle_bits)
        return 0;

    return tlc5971_send_idle_bits(dev, idle_bits);
}

int tlc5971_send_timing1(struct tlc5971_device *dev)
{
    return tlc5971_send_internal_repeat(dev);
}

int tlc5971_send_timing2(struct tlc5971_device *dev)
{
    return tlc5971_send_external_full_period(dev);
}

int tlc5971_send_timing3(struct tlc5971_device *dev)
{
    return tlc5971_send_external_long_chain(dev);
}

int tlc5971_sync_with_scki(struct tlc5971_device *dev)
{
    return tlc5971_send_idle_bits(dev, 65536);
}

#ifdef DESIGN_VERIFICATION_TLC5971
#include "kinetis/test-kinetis.h"

static struct tlc5971_device g_tlc5971_dev;

static int tlc5971_test_init(void)
{
    tlc5971_exit(&g_tlc5971_dev);

    return tlc5971_init(&g_tlc5971_dev, &spi1_master, 1);
}

int t_tlc5971_send_packet(int argc, char **argv)
{
    int ret;

    ret = tlc5971_test_init();
    if (ret)
        return ret;

    ret = tlc5971_send_internal_repeat(&g_tlc5971_dev);
    if (!ret)
        ret = tlc5971_send_external_full_period(&g_tlc5971_dev);
    if (!ret)
        ret = tlc5971_send_external_long_chain(&g_tlc5971_dev);

    tlc5971_exit(&g_tlc5971_dev);

    return ret;
}

/* tlc5971.color <r> <g> <b> - 16-bit GS values. */
int t_tlc5971_set_color(int argc, char **argv)
{
    u16 r = 0, g = 0, b = 0;
    int ret;

    if (argc >= 4) {
        r = (u16)simple_strtoul(argv[1], NULL, 0);
        g = (u16)simple_strtoul(argv[2], NULL, 0);
        b = (u16)simple_strtoul(argv[3], NULL, 0);
    }

    ret = tlc5971_test_init();
    if (ret)
        return ret;

    tlc5971_set_all_leds(&g_tlc5971_dev, r, g, b);

    ret = tlc5971_send_internal_repeat(&g_tlc5971_dev);

    pr_info("color r=%u g=%u b=%u ret=%d\n", r, g, b, ret);

    tlc5971_exit(&g_tlc5971_dev);

    return ret;
}

/* tlc5971.bc <bcr> <bcg> <bcb> - 0-127 brightness control. */
int t_tlc5971_brightness(int argc, char **argv)
{
    u8 bcr = 0x7F, bcg = 0x7F, bcb = 0x7F;
    int ret;

    if (argc >= 4) {
        bcr = (u8)(simple_strtoul(argv[1], NULL, 0) & 0x7F);
        bcg = (u8)(simple_strtoul(argv[2], NULL, 0) & 0x7F);
        bcb = (u8)(simple_strtoul(argv[3], NULL, 0) & 0x7F);
    }

    ret = tlc5971_test_init();
    if (ret)
        return ret;

    tlc5971_set_brightness_control(&g_tlc5971_dev, bcr, bcg, bcb);

    ret = tlc5971_send_internal_repeat(&g_tlc5971_dev);

    pr_info("bc r=%u g=%u b=%u ret=%d\n", bcr, bcg, bcb, ret);

    tlc5971_exit(&g_tlc5971_dev);

    return ret;
}

/* tlc5971.blank <0|1> - enable/disable all outputs. */
int t_tlc5971_blank(int argc, char **argv)
{
    u8 blank = 0;
    int ret;

    if (argc >= 2)
        blank = (u8)simple_strtoul(argv[1], NULL, 0);

    ret = tlc5971_test_init();
    if (ret)
        return ret;

    tlc5971_set_blank(&g_tlc5971_dev, blank);

    ret = tlc5971_send_latched(&g_tlc5971_dev);

    pr_info("blank=%u ret=%d\n", blank, ret);

    tlc5971_exit(&g_tlc5971_dev);

    return ret;
}

#endif /* DESIGN_VERIFICATION_TLC5971 */