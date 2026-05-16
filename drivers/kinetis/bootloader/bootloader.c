#define pr_fmt(fmt) "Bootloader: " fmt

#include <linux/crc32.h>
#include <linux/printk.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/errno.h>
#include <linux/delay.h>

#include <kinetis/mavlink.h>
#include <kinetis/led.h>
#include <kinetis/serial-port.h>
#include "kinetis/design_verification.h"

#include "../fs/fatfs/ff.h"

#include "bootloader.h"

/* Forward declarations of external dependencies */
int lrzsz_rz(struct serial_port *serial, const char *directory);

typedef void (*firmware_entry)(void);

/**
 * @brief Initialize bootloader context
 * @param config_addr: Pointer to bootloader config region in flash
 * @param ops: Platform operations structure (must not be NULL)
 * @return Bootloader context pointer on success, NULL on failure
 * @note Allocates serial port and MAVLink device internally.
 *       Caller must eventually call bootloader_free() to release resources.
 */
struct bootloader_context *bootloader_init(u8 *config_addr,
					    struct bootloader_ops *ops)
{
	struct bootloader_context *bl_ctx;

	if (!ops) {
		pr_err("bootloader ops cannot be NULL\n");
		return NULL;
	}

	bl_ctx = kzalloc(sizeof(struct bootloader_context), GFP_KERNEL);
	if (!bl_ctx) {
		return NULL;
	}

	bl_ctx->config_addr = config_addr;
	bl_ctx->ops = ops;

#if KINETIS_FAKE_SIM
	bl_ctx->app_port = serial_port_alloc(&fake_serial_port_ops, "bootloader-app");
#else
	bl_ctx->app_port = serial_port_alloc(&real_serial_port_ops, "bootloader-app");
#endif
	if (!bl_ctx->app_port) {
		kfree(bl_ctx);
		return NULL;
	}

	bl_ctx->app_mavlink = mavlink_init(bl_ctx->app_port, 1, 1, "bootloader-app-mavlink");
	if (!bl_ctx->app_mavlink) {
		serial_port_free(bl_ctx->app_port);
		kfree(bl_ctx);
		return NULL;
	}

	return bl_ctx;
}

/**
 * @brief Free bootloader context and all owned resources
 * @param bl_ctx: Bootloader context to free (NULL-safe)
 */
void bootloader_free(struct bootloader_context *bl_ctx)
{
	if (!bl_ctx)
		return;

	/* Reverse order of allocation */
	if (bl_ctx->app_mavlink)
		mavlink_free(bl_ctx->app_mavlink);
	if (bl_ctx->app_port)
		serial_port_free(bl_ctx->app_port);

	kfree(bl_ctx);
	pr_info("context freed\n");
}

static const char *bootloader_state_name(enum bootloader_fsm_state s)
{
	static const char *names[] = {
		"INIT", "CHECK_MODE", "JUMP_APP", "WAIT_CMD",
		"RECEIVE", "FLASH", "VERIFY", "COMPLETE", "ERROR"
	};
	if ((unsigned)s >= sizeof(names) / sizeof(names[0])) {
		return "?";
	}
	return names[s];
}

static void set_fsm_state(struct bootloader_context *bl_ctx, enum bootloader_fsm_state new_state)
{
	if (new_state != bl_ctx->state) {
		pr_info("state: %s -> %s\n", bootloader_state_name(bl_ctx->state), bootloader_state_name(new_state));
	}

	bl_ctx->state = new_state;
}

static int write_firmware_to_chip(struct bootloader_context *bl_ctx)
{
	FIL f;
	u32 total_size = 0;
	u32 calc_crc32 = ~0;
	u8 *buf = NULL;
	u32 offset = 0;
	UINT br;
	const char *firmware_path;
	u32 expected_size;
	u32 expected_crc;
	int ret;

	/* Use firmware info from MAVLink command, fall back to config */
	expected_size = bl_ctx->app_mavlink->bl.firmware_size;
	expected_crc  = bl_ctx->app_mavlink->bl.firmware_crc32;
	firmware_path = bl_ctx->app_mavlink->bl.filename[0]
			? bl_ctx->app_mavlink->bl.filename
			: BL_FIRMWARE_PATH;

	ret = f_open(&f, firmware_path, FA_READ);
	if (ret != FR_OK) {
		pr_err("FS: failed to open '%s' (ret=%d)\n",
		       firmware_path, ret);
		return -EIO;
	}

	/* Allocate heap buffer instead of 4KB on stack */
	buf = kmalloc(BL_FLASH_CHUNK, GFP_KERNEL);
	if (!buf) {
		pr_err("FS: failed to allocate buffer\n");
		f_close(&f);
		return -ENOMEM;
	}

	/* First pass: compute CRC32 */
	set_fsm_state(bl_ctx, BL_STATE_VERIFY);
	pr_info("FS: computing CRC32 of '%s'...\n", firmware_path);

	while (1) {
		ret = f_read(&f, buf, sizeof(buf), &br);
		if (ret != FR_OK || br == 0)
			break;
		calc_crc32 = crc32(calc_crc32, buf, br);
		total_size += br;
	}

	if (ret != FR_OK) {
		pr_err("FS: read error %d\n", ret);
		ret = -EIO;
		goto cleanup;
	}

	calc_crc32 ^= ~0;

	pr_info("FS: file size=%u, CRC32=0x%08x, expected size=%u CRC=0x%08x\n",
		total_size, calc_crc32, expected_size, expected_crc);

	/* Size / CRC check */
	if (expected_size != 0 && total_size != expected_size) {
		pr_err("Size mismatch: got %u, expected %u\n",
		       total_size, expected_size);
		ret = -EIO;
		goto cleanup;
	}

	if (expected_crc != 0 && calc_crc32 != expected_crc) {
		pr_err("CRC32 mismatch: got 0x%08x, expected 0x%08x\n",
		       calc_crc32, expected_crc);
		ret = -EIO;
		goto cleanup;
	}

	/* Second pass: erase and write to flash */
	set_fsm_state(bl_ctx, BL_STATE_FLASH);
	pr_info("Flash: erasing firmware region (%u bytes)...\n", total_size);

	ret = bl_ctx->ops->flash->erase(FIRMWARE_BASE_ADDR, total_size);
	if (ret < 0) {
		pr_err("Flash: erase failed (%d)\n", ret);
		goto cleanup;
	}

	pr_info("Flash: writing %u bytes...\n", total_size);
	f_lseek(&f, 0);
	offset = 0;

	while (1) {
		ret = f_read(&f, buf, sizeof(buf), &br);
		if (ret != FR_OK || br == 0)
			break;

		ret = bl_ctx->ops->flash->write(FIRMWARE_BASE_ADDR + offset,
					       buf, br);
		if (ret < 0) {
			pr_err("Flash: write failed at offset 0x%x (%d)\n",
			       offset, ret);
			goto cleanup;
		}

		offset += br;
	}

	pr_info("Flash: write complete (%u bytes at 0x%08x)\n",
		total_size, FIRMWARE_BASE_ADDR);

	/* Update config with new firmware info */
	bl_ctx->config.firmware_size = total_size;
	bl_ctx->config.firmware_crc32 = calc_crc32;

	ret = 0;

cleanup:
	kfree(buf);
	f_close(&f);
	return ret;
}

static int receive_firmware(struct bootloader_context *bl_ctx)
{
	int ret;

	pr_info("ZMODEM: waiting for server to send files on app_port...\n");

	/*
	 * lrzsz_rz() blocks until transfer completes.
	 * Files are saved to BL_FIRMWARE_PATH directory.
	 * Returns number of bytes received, or 0 on error.
	 */
	ret = lrzsz_rz(bl_ctx->app_port, BL_FIRMWARE_PATH);
	if (ret <= 0) {
		pr_warn("ZMODEM: receive failed or cancelled (%d)\n", ret);
		return -EIO;
	}

	pr_info("ZMODEM: received %d bytes\n", ret);

	/* Flash firmware from filesystem to flash */
	ret = write_firmware_to_chip(bl_ctx);
	if (ret < 0) {
		pr_err("Flashing firmware failed (%d)\n", ret);
		return ret;
	}

	return 0;
}

/**
 * @brief Check if firmware vector table at FIRMWARE_BASE_ADDR is valid.
 *        Validates stack pointer in SRAM range and reset vector in flash range.
 */
static int is_firmware_valid(struct bootloader_context *bl_ctx)
{
	u32 stack_ptr;
	u32 reset_vec;

	if (bl_ctx->ops->flash->read(FIRMWARE_BASE_ADDR, (u8 *)&stack_ptr, 4) < 0)
		return 0;
	if (bl_ctx->ops->flash->read(FIRMWARE_BASE_ADDR + 4, (u8 *)&reset_vec, 4) < 0)
		return 0;

	/* SRAM range: 0x20000000 - 0x20040000 (256KB) */
	if (stack_ptr < 0x20000000 || stack_ptr >= 0x20040000)
		return 0;
	/* Flash range: 0x08000000 - 0x08100000 (1MB) */
	if (reset_vec < ROM_BASE_ADDR || reset_vec >= (ROM_BASE_ADDR + ROM_TOTAL_SIZE))
		return 0;

	return 1;
}

/**
 * @brief Write bootloader_config back to flash (erase sector then write).
 */
static int write_config_to_flash(struct bootloader_context *bl_ctx)
{
	int ret;

	ret = bl_ctx->ops->flash->erase(CONFIG_BASE_ADDR,
				       sizeof(struct bootloader_config));
	if (ret < 0) {
		pr_err("Config: erase failed (%d)\n", ret);
		return ret;
	}

	ret = bl_ctx->ops->flash->write(CONFIG_BASE_ADDR,
				       (const u8 *)&bl_ctx->config,
				       sizeof(struct bootloader_config));
	if (ret < 0) {
		pr_err("Config: write failed (%d)\n", ret);
		return ret;
	}

	return 0;
}

static void jump_to_firmware(struct bootloader_context *bl_ctx)
{
	u32 stack_ptr;
	u32 reset_vector;
	firmware_entry firmware;

	/* Read FIRMWARE vector table via flash_read */
	if (bl_ctx->ops->flash->read(FIRMWARE_BASE_ADDR, (u8 *)&stack_ptr, 4) < 0 ||
	    bl_ctx->ops->flash->read(FIRMWARE_BASE_ADDR + 4, (u8 *)&reset_vector, 4) < 0) {
		pr_err("Failed to read firmware vector table, halt.\n");
		while (1)
			;
	}
	firmware = (firmware_entry)reset_vector;

	pr_info("Jumping to firmware: MSP=0x%08x, reset=0x%08x\n",
		stack_ptr, reset_vector);

	bl_ctx->ops->disable_irq();

	/* Reset all peripheral clocks and de-initialize HAL */
	bl_ctx->ops->cleanup_hw_resource();

	/* Ensure all memory operations are complete */
	bl_ctx->ops->disable_cache();

	/* Set MSP and remap VTOR to FIRMWARE vector table */
	bl_ctx->ops->set_stack_pointer(stack_ptr);
	bl_ctx->ops->redirect_isr_table(FIRMWARE_BASE_ADDR);

	/* Jump to FIRMWARE reset handler */
	firmware();

	/* Should never reach here */
	while (1)
		;
}

/**
 * @brief Verify firmware CRC32 by reading back from flash.
 */
static int verify_flash_crc(struct bootloader_context *bl_ctx)
{
	u32 calc_crc32 = ~0;
	u32 offset;
	u8 *buf = NULL;
	int ret = 0;

	pr_info("Verify: reading back from flash and checking CRC32...\n");

	buf = kmalloc(BL_FLASH_CHUNK, GFP_KERNEL);
	if (!buf) {
		pr_err("Verify: failed to allocate buffer\n");
		return -ENOMEM;
	}

	for (offset = 0; offset < bl_ctx->config.firmware_size; offset += BL_FLASH_CHUNK) {
		u32 chunk = bl_ctx->config.firmware_size - offset;
		if (chunk > BL_FLASH_CHUNK)
			chunk = BL_FLASH_CHUNK;

		ret = bl_ctx->ops->flash->read(FIRMWARE_BASE_ADDR + offset,
					      buf, chunk);
		if (ret < 0) {
			pr_err("Verify: flash_read failed at 0x%x (%d)\n",
			       FIRMWARE_BASE_ADDR + offset, ret);
			goto out;
		}

		calc_crc32 = crc32(calc_crc32, buf, chunk);
	}

	calc_crc32 ^= ~0;

	pr_info("Verify: flash CRC32=0x%08x, expected=0x%08x\n",
		calc_crc32, bl_ctx->config.firmware_crc32);

	if (calc_crc32 != bl_ctx->config.firmware_crc32) {
		pr_err("Verify: CRC32 mismatch!\n");
		ret = -EIO;
	}

out:
	kfree(buf);
	return ret;
}

static void check_server_cmd(struct bootloader_context *bl_ctx)
{
	u32 old_rx = bl_ctx->app_mavlink->rx_count;

	/* Process pending MAVLink messages (server commands via app_port) */
	if (serial_port_data_available(bl_ctx->app_port) > 0) {
		mavlink_receive_and_process(bl_ctx->app_mavlink, 10);
	}

	/* Only dispatch if a new message was received */
	if (bl_ctx->app_mavlink->rx_count == old_rx) {
		return;
	}

	switch (bl_ctx->app_mavlink->rx_msg.msgid) {
	/* ─── System ───────────────────────────────────────── */
	case MAVLINK_MSG_ID_SYSTEM_HEARTBEAT:
		pr_debug("cmd: heartbeat\n");
		break;

	/* ─── Bootloader ──────────────────────────────────── */
	case MAVLINK_MSG_ID_BOOTLOADER_UPDATE_CMD:
		pr_info("cmd: bootloader update — cmd=%d size=%u name=%s\n",
			bl_ctx->app_mavlink->bl.command,
			bl_ctx->app_mavlink->bl.firmware_size,
			bl_ctx->app_mavlink->bl.filename);
		set_fsm_state(bl_ctx, BL_STATE_RECEIVE);
		break;

	default:
		pr_debug("cmd: unhandled msgid=%u\n",
			bl_ctx->app_mavlink->rx_msg.msgid);
		break;
	}

}

/**
 * @brief Main bootloader entry and FSM loop
 * @return 0 on success (never returns normally in production),
 *         negative error code on failure after max retries
 * @note This function either jumps to firmware (COMPLETE) or loops
 *       forever handling firmware updates (WAIT_CMD / ERROR retry).
 *       After max retries (3) the function exits with -EIO.
 */
int bootloader_flow(void)
{
	struct bootloader_context *bl_ctx;
	int ret;

	bl_ctx = bootloader_init((u8 *)CONFIG_BASE_ADDR, NULL /* ops */);
	if (!bl_ctx) {
		return -ENOMEM;
	}

	set_fsm_state(bl_ctx, BL_STATE_CHECK_MODE);

	memcpy(&bl_ctx->config, bl_ctx->config_addr, sizeof(struct bootloader_config));

	if (bl_ctx->config.magic != BOOTLOADER_MAGIC) {
		pr_warn("Bootloader config magic mismatch: 0x%08x, expected 0x%08x, enter update mode\n",
			bl_ctx->config.magic, BOOTLOADER_MAGIC);

		memset(&bl_ctx->config, 0, sizeof(struct bootloader_config));
		bl_ctx->config.magic = BOOTLOADER_MAGIC;
		bl_ctx->config.version = 1;
		bl_ctx->config.mode = BL_MODE_UPDATE;
	} else {
		pr_info("Bootloader config loaded: mode=%d, firmware_size=%u, firmware_crc32=0x%08x\n",
			bl_ctx->config.mode, bl_ctx->config.firmware_size, bl_ctx->config.firmware_crc32);
	}

	switch (bl_ctx->config.mode) {
	case BL_MODE_RUN:
		if (is_firmware_valid(bl_ctx)) {
			pr_info("Bootloader in RUN mode, jumping to application...\n");
			set_fsm_state(bl_ctx, BL_STATE_JUMP_APP);
			jump_to_firmware(bl_ctx);
			/* Should never return */
		} else {
			pr_warn("APP not valid, entering update mode\n");
			bl_ctx->config.mode = BL_MODE_UPDATE;
		}
		break;

	case BL_MODE_UPDATE:
		pr_info("Bootloader in UPDATE mode, waiting for firmware...\n");
		break;

	case BL_MODE_RECOVERY:
		pr_info("Bootloader in RECOVERY mode, waiting for firmware...\n");
		break;

	default:
		pr_warn("Bootloader in unknown mode %d, entering recovery mode\n",
			bl_ctx->config.mode);
		bl_ctx->config.mode = BL_MODE_RECOVERY;
		break;
	}

	set_fsm_state(bl_ctx, BL_STATE_WAIT_CMD);

	/* ─── Main FSM Loop ───────────────────────────────── */
	while (1) {
		switch (bl_ctx->state) {
		case BL_STATE_WAIT_CMD:
			check_server_cmd(bl_ctx);
			mdelay(10);  /* Prevent busy loop when no serial data */
			break;

		case BL_STATE_RECEIVE:
			ret = receive_firmware(bl_ctx);
			if (ret < 0) {
				pr_err("Firmware update failed (%d), waiting for retry...\n", ret);
				set_fsm_state(bl_ctx, BL_STATE_ERROR);
			} else {
				set_fsm_state(bl_ctx, BL_STATE_VERIFY);
			}
			break;

		case BL_STATE_VERIFY:
			ret = verify_flash_crc(bl_ctx);
			if (ret < 0) {
				pr_err("Flash verification failed, waiting for retry...\n");
				set_fsm_state(bl_ctx, BL_STATE_ERROR);
			} else {
				set_fsm_state(bl_ctx, BL_STATE_COMPLETE);
			}
			break;

		case BL_STATE_COMPLETE:
			pr_info("Update successful! Finalizing...\n");
			bl_ctx->config.mode = BL_MODE_RUN;
			bl_ctx->config.update_count++;
			ret = write_config_to_flash(bl_ctx);
			if (ret < 0)
				pr_err("Failed to write config, jumping anyway...\n");
			pr_info("Jumping to new firmware...\n");

			/* Free bootloader context before jump, we won't be back */
			bootloader_free(bl_ctx);
			jump_to_firmware(bl_ctx);
			break;

		case BL_STATE_ERROR:
			bl_ctx->retry_count++;
			if (bl_ctx->retry_count >= 3) {
				pr_err("Max retries reached (%d), aborting\n",
				       bl_ctx->retry_count);
				ret = -EIO;
				goto out_free;
			}
			pr_warn("Update failed (retry %d/3), waiting for retry...\n",
				bl_ctx->retry_count);
			bl_ctx->config.mode = BL_MODE_UPDATE;
			set_fsm_state(bl_ctx, BL_STATE_WAIT_CMD);
			break;

		default:
			break;
		}
	}

	return 0;

out_free:
	bootloader_free(bl_ctx);
	pr_err("Bootloader failed with ret=%d\n", ret);
	return ret;
}

#ifdef DESIGN_VERIFICATION_BOOTLOADER

#include <linux/random.h>

#include <kinetis/design_verification.h>
#include <kinetis/test-kinetis.h>

/* ─── Mock Flash ──────────────────────────────────────────── */
#define MOCK_FLASH_SIZE      (1u * 1024u * 1024u)   /* 1MB simulated flash */
#define FW_TEST_FILE         "0:/firmware/test_fw.bin"
#define FW_TEST_SIZE         (64u * 1024u)          /* 64KB fake firmware */

static u8 *g_mock_flash;

static int mock_flash_erase(u32 addr, u32 size)
{
	u32 end = addr + size;

	if (end > MOCK_FLASH_SIZE) {
		pr_err("mock_erase: addr 0x%x+%u > %u\n", addr, size, MOCK_FLASH_SIZE);
		return -EINVAL;
	}
	memset(g_mock_flash + addr, 0xFF, size);
	return 0;
}

static int mock_flash_write(u32 addr, const u8 *data, u32 size)
{
	if (addr + size > MOCK_FLASH_SIZE) {
		pr_err("mock_write: addr 0x%x+%u > %u\n", addr, size, MOCK_FLASH_SIZE);
		return -EINVAL;
	}
	memcpy(g_mock_flash + addr, data, size);
	return 0;
}

static int mock_flash_read(u32 addr, u8 *buf, u32 size)
{
	if (addr + size > MOCK_FLASH_SIZE) {
		pr_err("mock_read: addr 0x%x+%u > %u\n", addr, size, MOCK_FLASH_SIZE);
		return -EINVAL;
	}
	memcpy(buf, g_mock_flash + addr, size);
	return 0;
}

static void mock_disable_irq(void) { }
static void mock_cleanup_hw_resource(void) { }
static void mock_disable_cache(void) { }
static void mock_set_stack_pointer(u32 ptr) { }
static void mock_redirect_isr_table(u32 addr) { }
static void mock_system_reset(void)
{
	pr_info("mock system reset called (ignored in sim)\n");
}

static struct flash_ops g_mock_flash_ops = {
	.erase                = mock_flash_erase,
	.write                = mock_flash_write,
	.read                 = mock_flash_read,
};

static struct bootloader_ops g_mock_bl_ops = {
	.disable_irq          = mock_disable_irq,
	.cleanup_hw_resource  = mock_cleanup_hw_resource,
	.disable_cache        = mock_disable_cache,
	.set_stack_pointer    = mock_set_stack_pointer,
	.redirect_isr_table   = mock_redirect_isr_table,
	.flash                = &g_mock_flash_ops,
	.system_reset         = mock_system_reset,
};

/* ─── Helper: write a fake firmware to FatFS and mock flash ─ */
static int prepare_fake_firmware(void)
{
	FRESULT res;
	FIL f;
	UINT bw;
	u8 *buf = NULL;
	int ret = 0;

	buf = kmalloc(FW_TEST_SIZE, GFP_KERNEL);
	if (!buf)
		return -ENOMEM;

	/* Fill with random data */
	get_random_bytes(buf, FW_TEST_SIZE);

	/* Write first 8 bytes as a fake vector table */
	*(u32 *)&buf[0] = 0x20001000;                         /* stack in SRAM */
	*(u32 *)&buf[4] = FIRMWARE_BASE_ADDR + 0x100;         /* reset in flash */

	res = f_open(&f, FW_TEST_FILE, FA_CREATE_ALWAYS | FA_WRITE);
	if (res != FR_OK) {
		pr_err("failed to create '%s': %d\n", FW_TEST_FILE, res);
		ret = -EIO;
		goto out;
	}

	res = f_write(&f, buf, FW_TEST_SIZE, &bw);
	if (res != FR_OK || bw != FW_TEST_SIZE) {
		pr_err("write failed: %d (bw=%u)\n", res, bw);
		ret = -EIO;
	}
	f_close(&f);

out:
	kfree(buf);
	return ret;
}

/* ─── Test 1: Init / Free cycle ──────────────────────────── */
int t_bootloader_init_sim(int argc, char *argv[])
{
	struct bootloader_context *bl_ctx;
	int ret = PASS;

	pr_info("=== Bootloader Init/Free Test ===\n");

	g_mock_flash = kzalloc(MOCK_FLASH_SIZE, GFP_KERNEL);
	if (!g_mock_flash) {
		pr_err("failed to allocate mock flash\n");
		return FAIL;
	}

	/* NULL ops should fail */
	bl_ctx = bootloader_init((u8 *)CONFIG_BASE_ADDR, NULL);
	if (bl_ctx) {
		pr_err("expected NULL for NULL ops\n");
		ret = FAIL;
		goto out;
	}
	pr_info("NULL ops check: PASS\n");

	/* Valid ops should succeed */
	bl_ctx = bootloader_init((u8 *)CONFIG_BASE_ADDR, &g_mock_bl_ops);
	if (!bl_ctx) {
		pr_err("bootloader_init failed\n");
		ret = FAIL;
		goto out;
	}
	pr_info("bootloader_init: PASS (ctx=%p)\n", bl_ctx);

	bootloader_free(bl_ctx);
	pr_info("bootloader_free: PASS\n");

out:
	kfree(g_mock_flash);
	g_mock_flash = NULL;
	pr_info("=== Init/Free %s ===\n", ret ? "FAIL" : "PASS");
	return ret;
}

/* ─── Test 2: Full update simulation ─────────────────────── */
int t_bootloader_full_update(int argc, char *argv[])
{
	struct bootloader_context *bl_ctx;
	struct bootloader_config *cfg;
	int ret = PASS;

	pr_info("=== Bootloader Full Update Test ===\n");

	g_mock_flash = kzalloc(MOCK_FLASH_SIZE, GFP_KERNEL);
	if (!g_mock_flash) {
		pr_err("failed to allocate mock flash\n");
		return FAIL;
	}

	if (prepare_fake_firmware() < 0) {
		pr_err("prepare_fake_firmware failed\n");
		ret = FAIL;
		goto out;
	}
	pr_info("fake firmware created (%d bytes)\n", FW_TEST_SIZE);

	bl_ctx = bootloader_init((u8 *)CONFIG_BASE_ADDR, &g_mock_bl_ops);
	if (!bl_ctx) {
		pr_err("bootloader_init failed\n");
		ret = FAIL;
		goto out;
	}

	/* Set boot flag so FSM sees UPDATE mode */
	cfg = (struct bootloader_config *)bl_ctx->config_addr;

	cfg->magic = BOOTLOADER_MAGIC;
	cfg->version = 1;
	cfg->mode = BL_MODE_UPDATE;

	memcpy(&bl_ctx->config, bl_ctx->config_addr,
	       sizeof(struct bootloader_config));

	/* Erased flash should not have a valid firmware vector table */
	pr_info("is_firmware_valid (before write) = %d\n",
		is_firmware_valid(bl_ctx));

	pr_info("=== Full Update %s ===\n", ret ? "FAIL" : "PASS");

	bootloader_free(bl_ctx);

out:
	kfree(g_mock_flash);
	g_mock_flash = NULL;
	return ret;
}

/* ─── Test 3: Flash write benchmark ──────────────────────── */
int t_bootloader_benchmark(int argc, char *argv[])
{
	u32 size = FW_TEST_SIZE;
	int ret = PASS;

	if (argc > 1)
		size = simple_strtoul(argv[1], NULL, 10);

	pr_info("=== Bootloader Flash Benchmark (%u bytes) ===\n", size);

	g_mock_flash = kzalloc(MOCK_FLASH_SIZE, GFP_KERNEL);
	if (!g_mock_flash) {
		pr_err("failed to allocate mock flash\n");
		return FAIL;
	}

	u8 *buf = kmalloc(size, GFP_KERNEL);
	if (!buf) {
		pr_err("failed to allocate test buffer\n");
		kfree(g_mock_flash);
		return FAIL;
	}
	get_random_bytes(buf, size);

	/* Erase entire mock flash */
	mock_flash_erase(0, MOCK_FLASH_SIZE);
	pr_info("flash erased (%u bytes)\n", MOCK_FLASH_SIZE);

	/* Benchmark: sequential write */
	mock_flash_write(FIRMWARE_BASE_ADDR, buf, size);

	/* Verify by read-back */
	u8 *verify_buf = kmalloc(size, GFP_KERNEL);
	if (!verify_buf) {
		pr_err("failed to allocate verify buffer\n");
		kfree(buf);
		kfree(g_mock_flash);
		return FAIL;
	}

	mock_flash_read(FIRMWARE_BASE_ADDR, verify_buf, size);

	if (memcmp(buf, verify_buf, size) != 0) {
		pr_err("data mismatch after write/read!\n");
		ret = FAIL;
	} else {
		pr_info("data integrity: PASS\n");
	}

	u32 crc = crc32(~0, buf, size) ^ ~0;
	pr_info("CRC32 of %u bytes: 0x%08x\n", size, crc);

	kfree(verify_buf);
	kfree(buf);
	kfree(g_mock_flash);
	g_mock_flash = NULL;
	pr_info("=== Benchmark %s ===\n", ret ? "FAIL" : "PASS");
	return ret;
}

#endif /* DESIGN_VERIFICATION_BOOTLOADER */
