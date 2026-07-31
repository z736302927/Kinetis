#define pr_fmt(fmt) "bootloader: " fmt

#include <linux/crc32.h>
#include <linux/printk.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/errno.h>
#include <linux/delay.h>
#include <linux/random.h>

#include <kinetis/mavlink.h>
#include <kinetis/led.h>
#include <kinetis/serial-port.h>
#include "kinetis/design_verification.h"
#include "kinetis/fatfs-intf.h"
#include "kinetis/bootloader.h"
#include "kinetis/basic-timer.h"

#include "fake-mcu/interface.h"

#include "fatfs/ff.h"

#include "lrzsz/zmodem.h"

typedef void (*firmware_entry)(void);

/**
 * @brief Initialize bootloader context
 * @param layout:   Memory layout descriptor
 * @param chip_ops: Platform operations structure (must not be NULL)
 * @return Bootloader context pointer on success, NULL on failure
 * @note Computes firmware_addr from layout fields.
 *       Caller must eventually call bootloader_free() to release resources.
 */
struct bootloader_context *bootloader_init(const struct bootloader_mem_layout *layout,
					   struct bootloader_ops *chip_ops,
					   struct flash_ops *flash_ops,
					   struct serial_port_ops *serial_ops)
{
	struct bootloader_context *bl_ctx;

	if (!layout || !chip_ops || !flash_ops || !serial_ops || !layout->rom_base) {
		pr_err("bootloader init params cannot be NULL\n");
		return NULL;
	}

	if (layout->firmware_max_size + layout->bootloader_size > layout->rom_size) {
		pr_err("layout: firmware (0x%x) + bootloader (0x%x) exceeds ROM (0x%x)\n",
		       layout->firmware_max_size, layout->bootloader_size, layout->rom_size);
		return NULL;
	}

	bl_ctx = kzalloc(sizeof(struct bootloader_context), GFP_KERNEL);
	if (!bl_ctx) {
		return NULL;
	}

	bl_ctx->layout = *layout;
	bl_ctx->rom_base = layout->rom_base;
	bl_ctx->firmware_addr = layout->rom_base + layout->bootloader_size;
	bl_ctx->ops = chip_ops;
	bl_ctx->ops->flash = flash_ops;

	bl_ctx->app_port = serial_port_alloc(serial_ops, "bootloader-app");
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

	bl_ctx->last_send_heartbeat_ms = basic_timer_get_ms();

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
	FIL fp;
	u32 total_size = 0;
	u32 calc_crc32 = ~0;
	u8 *buf = NULL;
	u32 offset = 0;
	UINT br;
	char firmware_path[64];
	const char *fn;
	u32 expected_size;
	u32 expected_crc;
	int ret;

	/* Use firmware info from context (populated by flag file or MAVLink) */
	expected_size = bl_ctx->firmware_size;
	expected_crc  = bl_ctx->firmware_crc32;
	snprintf(firmware_path, sizeof(firmware_path), "%s/%s", BL_BACKUP_PATH,
		 bl_ctx->firmware_filename[0] ? bl_ctx->firmware_filename : BL_FIRMWARE_NAME);

	ret = f_open(&fp, firmware_path, FA_READ);
	if (ret != FR_OK) {
		pr_err("FS: failed to open '%s' (ret=%d)\n",
		       firmware_path, ret);
		return -EIO;
	}

	/* Allocate heap buffer instead of 4KB on stack */
	buf = kmalloc(BL_FLASH_CHUNK, GFP_KERNEL);
	if (!buf) {
		pr_err("FS: failed to allocate buffer\n");
		f_close(&fp);
		return -ENOMEM;
	}

	/* First pass: compute CRC32 */
	set_fsm_state(bl_ctx, BL_STATE_VERIFY);
	pr_info("FS: computing CRC32 of '%s'...\n", firmware_path);

	while (1) {
		ret = f_read(&fp, buf, BL_FLASH_CHUNK, &br);
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
		ret = -EPIPE;
		goto cleanup;
	}

	if (expected_crc != 0 && calc_crc32 != expected_crc) {
		pr_err("CRC32 mismatch: got 0x%08x, expected 0x%08x\n",
		       calc_crc32, expected_crc);
		ret = -EPIPE;
		goto cleanup;
	}

	f_close(&fp);

	fn = bl_ctx->firmware_filename[0] ?  bl_ctx->firmware_filename : BL_FIRMWARE_NAME;
	ret = fatfs_copy_file(BL_BACKUP_PATH, fn, BL_FIRMWARE_PATH, NULL);
	if (ret != FR_OK) {
		ret = -EIO;
		goto cleanup;
	}

	snprintf(firmware_path, sizeof(firmware_path),
		 "%s/%s", BL_FIRMWARE_PATH, fn);

	ret = f_open(&fp, firmware_path, FA_READ);
	if (ret != FR_OK) {
		pr_err("FS: failed to open '%s' (ret=%d)\n",
		       firmware_path, ret);
		goto cleanup;
	}

	/* Second pass: erase and write to flash */
	set_fsm_state(bl_ctx, BL_STATE_FLASH);
	pr_info("Flash: erasing firmware region (%u bytes)...\n", total_size);

	ret = bl_ctx->ops->flash->erase((u64)bl_ctx->firmware_addr, total_size);
	if (ret < 0) {
		pr_err("Flash: erase failed (%d)\n", ret);
		goto cleanup;
	}

	pr_info("Flash: writing %u bytes...\n", total_size);
	offset = 0;

	while (1) {
		ret = f_read(&fp, buf, BL_FLASH_CHUNK, &br);
		if (ret != FR_OK || br == 0)
			break;

		ret = bl_ctx->ops->flash->write((u64)bl_ctx->firmware_addr + offset,
					       buf, br);
		if (ret < 0) {
			pr_err("Flash: write failed at offset 0x%x (%d)\n",
			       offset, ret);
			goto cleanup;
		}

		offset += br;
	}

	pr_info("Flash: write complete (%u bytes at 0x%08x)\n",
		total_size, (u64)bl_ctx->firmware_addr);

	/* Store firmware metadata for verification */
	bl_ctx->firmware_size = total_size;
	bl_ctx->firmware_crc32 = calc_crc32;

	ret = 0;

cleanup:
	kfree(buf);
	f_close(&fp);
	return ret;
}

static int receive_firmware(struct bootloader_context *bl_ctx)
{
	int ret;

	if (serial_port_data_available(bl_ctx->app_port) == 0) {
		return -ENODATA;
	}

	pr_info("ZMODEM: waiting for server to send files on app_port...\n");

	/*
	 * lrzsz_rz() blocks until transfer completes.
	 * Files are saved to BL_FIRMWARE_PATH directory.
	 * Returns number of bytes received, or 0 on error.
	 */
	ret = lrzsz_rz(bl_ctx->app_port, BL_BACKUP_PATH);
	if (ret <= 0) {
		pr_warn("ZMODEM: receive failed or cancelled (%d)\n", ret);
		return ret;
	}

	/* Flash firmware from filesystem to flash */
	ret = write_firmware_to_chip(bl_ctx);
	if (ret < 0) {
		pr_err("Flashing firmware failed (%d)\n", ret);
		return ret;
	}

	return 0;
}

/**
 * @brief Check if firmware vector table is valid.
 *        Validates stack pointer in SRAM range and reset vector in flash range.
 */
static int is_firmware_valid(struct bootloader_context *bl_ctx)
{
	u32 stack_ptr;
	u32 reset_vec;
	u8 *sram_start = bl_ctx->layout.sram.start;
	u8 *sram_end   = bl_ctx->layout.sram.end;

	if (bl_ctx->ops->flash->read((u64)bl_ctx->firmware_addr, (u8 *)&stack_ptr, 4) < 0) {
		pr_err("firmware invalid: failed to read stack pointer at 0x%08x\n",
		       (u32)(u64)bl_ctx->firmware_addr);
		return 0;
	}
	if (bl_ctx->ops->flash->read((u64)bl_ctx->firmware_addr + 4, (u8 *)&reset_vec, 4) < 0) {
		pr_err("firmware invalid: failed to read reset vector at 0x%08x\n",
		       (u32)(u64)bl_ctx->firmware_addr + 4);
		return 0;
	}

	/* SRAM range from layout */
	if (stack_ptr < (u32)(u64)sram_start || stack_ptr > (u32)(u64)sram_end) {
		pr_err("firmware invalid: stack pointer 0x%08x out of SRAM [0x%08x, 0x%08x)\n",
		       stack_ptr, (u32)(u64)sram_start, (u32)(u64)sram_end);
		return 0;
	}
	/* Flash range from layout */
	if (reset_vec < (u32)(u64)bl_ctx->layout.rom_base ||
	    reset_vec >= (u32)(u64)bl_ctx->layout.rom_base + bl_ctx->layout.rom_size) {
		pr_err("firmware invalid: reset vector 0x%08x out of flash [0x%08x, 0x%08x)\n",
		       reset_vec, (u32)(u64)bl_ctx->layout.rom_base,
		       (u32)(u64)bl_ctx->layout.rom_base + bl_ctx->layout.rom_size);
		return 0;
	}

	return 1;
}

static void jump_to_firmware(struct bootloader_context *bl_ctx)
{
	u32 stack_ptr;
	u32 reset_vector;
	firmware_entry firmware;

	/* Read FIRMWARE vector table via flash_read */
	if (bl_ctx->ops->flash->read((u64)bl_ctx->firmware_addr, (u8 *)&stack_ptr, 4) < 0 ||
	    bl_ctx->ops->flash->read((u64)bl_ctx->firmware_addr + 4, (u8 *)&reset_vector, 4) < 0) {
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
	bl_ctx->ops->redirect_isr_table((u64)bl_ctx->firmware_addr);

	/* Clear all pending NVIC interrupts to prevent stray IRQ from
	 * firing during the transition using the newly assigned VTOR,
	 * then re-enable global interrupts (PRIMASK = 0) so the app
	 * starts in the same state as after a real hardware reset. */
	bl_ctx->ops->clear_pending_irqs();
	bl_ctx->ops->enable_irq();

	/* Jump to FIRMWARE reset handler */
#ifdef KINETIS_FAKE_SIM
	pr_info("simulation: firmware jump skipped (reset=0x%08x)\n", reset_vector);
	return;
#else
	firmware();
	/* Should never reach here */
	while (1)
		;
#endif
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

	for (offset = 0; offset < bl_ctx->firmware_size; offset += BL_FLASH_CHUNK) {
		u32 chunk = bl_ctx->firmware_size - offset;
		if (chunk > BL_FLASH_CHUNK)
			chunk = BL_FLASH_CHUNK;

		ret = bl_ctx->ops->flash->read((u64)bl_ctx->firmware_addr + offset,
					      buf, chunk);
		if (ret < 0) {
			pr_err("Verify: flash_read failed at 0x%x (%d)\n",
			       (u64)bl_ctx->firmware_addr + offset, ret);
			goto err;
		}

		calc_crc32 = crc32(calc_crc32, buf, chunk);
	}

	calc_crc32 ^= ~0;

	pr_info("Verify: flash CRC32=0x%08x, expected=0x%08x\n",
		calc_crc32, bl_ctx->firmware_crc32);

	if (calc_crc32 != bl_ctx->firmware_crc32) {
		pr_err("Verify: CRC32 mismatch!\n");
		ret = -EIO;
	}

err:
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
		pr_info("cmd: bootloader update - cmd=%d size=%u name=%s\n",
			bl_ctx->app_mavlink->bl.command,
			bl_ctx->app_mavlink->bl.firmware_size,
			bl_ctx->app_mavlink->bl.filename);

		bl_ctx->firmware_size = bl_ctx->app_mavlink->bl.firmware_size;
		bl_ctx->firmware_crc32 = bl_ctx->app_mavlink->bl.firmware_crc32;
		strncpy(bl_ctx->firmware_filename, bl_ctx->app_mavlink->bl.filename,
				sizeof(bl_ctx->firmware_filename) - 1);

		set_fsm_state(bl_ctx, BL_STATE_RECEIVE);
		break;

	default:
		pr_debug("cmd: unhandled msgid=%u\n",
			bl_ctx->app_mavlink->rx_msg.msgid);
		break;
	}

}

static int read_update_flag(struct bootloader_context *bl_ctx)
{
	struct bootloader_update_flag flag;
	FIL fp;
	UINT br;
	int ret;

	ret = f_open(&fp, BL_UPDATE_FLAG_PATH, FA_READ);
	if (ret != FR_OK)
		return -ENOENT;

	memset(&flag, 0, sizeof(flag));
	ret = f_read(&fp, &flag, sizeof(flag), &br);
	f_close(&fp);

	if (ret != FR_OK || br < sizeof(flag.magic) + sizeof(flag.firmware_size)) {
		pr_err("update_flag: read failed, ret=%d br=%u\n", ret, br);
		return -EIO;
	}

	if (flag.magic != BL_UPDATE_FLAG_MAGIC) {
		pr_err("update_flag: bad magic 0x%08x\n", flag.magic);
		return -EINVAL;
	}

	bl_ctx->firmware_size  = flag.firmware_size;
	bl_ctx->firmware_crc32 = flag.firmware_crc32;
	strncpy(bl_ctx->firmware_filename, flag.firmware_filename,
		sizeof(bl_ctx->firmware_filename) - 1);

	pr_info("update_flag: size=%u crc=0x%08x file=%s\n",
		bl_ctx->firmware_size, bl_ctx->firmware_crc32,
		bl_ctx->firmware_filename);

	return 0;
}

/**
 * @brief Main bootloader entry and FSM loop
 * @return 0 on success (never returns normally in production),
 *         negative error code on failure after max retries
 * @note Boot decision: check update_flag on FatFS -> if exists, enter update;
 *       otherwise check firmware vector table -> if valid, jump; else enter update.
 *       After max retries (3) the function exits with -EIO.
 */
int bootloader_flow(struct bootloader_context *bl_ctx)
{
	u64 now;
	int ret;
	bool enter_update = false;

	set_fsm_state(bl_ctx, BL_STATE_CHECK_MODE);

	ret = read_update_flag(bl_ctx);
	if (ret == 0) {
		enter_update = true;
		pr_info("Update flag detected, entering update mode\n");
	} else if (is_firmware_valid(bl_ctx)) {
		pr_info("Firmware valid, jumping to application...\n");
		set_fsm_state(bl_ctx, BL_STATE_JUMP_APP);
		jump_to_firmware(bl_ctx);
		/* Should never return */
	} else {
		pr_warn("No valid firmware and no update flag, entering update mode\n");
		enter_update = true;
	}

	if (enter_update) {
		ret = fatfs_create_dirs(BL_FIRMWARE_PATH);
		if (ret)
			goto err;
		ret = fatfs_create_dirs(BL_BACKUP_PATH);
		if (ret)
			goto err;

		set_fsm_state(bl_ctx, BL_STATE_WAIT_CMD);
	}

	/* ─── Main FSM Loop ───────────────────────────────── */
	while (1) {
		switch (bl_ctx->state) {
		case BL_STATE_WAIT_CMD:
			check_server_cmd(bl_ctx);

			now = basic_timer_get_ms();

			if (now - bl_ctx->last_send_heartbeat_ms >= 10000) {
				bl_ctx->last_send_heartbeat_ms = now;
				mavlink_send_heartbeat(bl_ctx->app_mavlink, MAV_SYS_ACTIVE, 0, 0);
			}
			break;

		case BL_STATE_RECEIVE:
			ret = receive_firmware(bl_ctx);
			if (ret == 0) {
				set_fsm_state(bl_ctx, BL_STATE_VERIFY);
			} else if (ret == -ENODATA) {
				break;
			} else {
				pr_err("Firmware update failed (%d), waiting for retry...\n", ret);
				set_fsm_state(bl_ctx, BL_STATE_ERROR);
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
			fatfs_delete_file(NULL, BL_UPDATE_FLAG_PATH);
#ifdef KINETIS_FAKE_SIM
			pr_info("simulation: update complete, returning\n");
			return 0;
#else
			pr_info("Jumping to new firmware...\n");
			jump_to_firmware(bl_ctx);
#endif
			break;

		case BL_STATE_ERROR:
			bl_ctx->retry_count++;
			if (bl_ctx->retry_count >= 3) {
				pr_err("Max retries reached (%d), aborting\n",
				       bl_ctx->retry_count);
				fatfs_delete_file(NULL, BL_UPDATE_FLAG_PATH);
				ret = -EIO;
				goto err;
			}
			pr_warn("Update failed (retry %d/3), waiting for retry...\n",
				bl_ctx->retry_count);
			set_fsm_state(bl_ctx, BL_STATE_WAIT_CMD);
			break;

		default:
			break;
		}
	}

	return 0;

err:
	pr_err("Bootloader failed with ret=%d\n", ret);
	return ret;
}

int bootloader_write_update_flag(u32 firmware_size, u32 firmware_crc32,
				 const char *filename)
{
	struct bootloader_update_flag flag;
	FIL fp;
	UINT bw;
	FRESULT res;

	memset(&flag, 0, sizeof(flag));
	flag.magic = BL_UPDATE_FLAG_MAGIC;
	flag.firmware_size = firmware_size;
	flag.firmware_crc32 = firmware_crc32;
	if (filename && filename[0])
		strncpy(flag.firmware_filename, filename,
			sizeof(flag.firmware_filename) - 1);

	if (f_open(&fp, BL_UPDATE_FLAG_PATH, FA_CREATE_ALWAYS | FA_WRITE) != FR_OK) {
		pr_err("Failed to create update flag file\n");
		return -EIO;
	}
	res = f_write(&fp, &flag, sizeof(flag), &bw);
	if (res != FR_OK || bw != sizeof(flag)) {
		pr_err("Failed to write update flag file\n");
		f_close(&fp);
		return -EIO;
	}
	f_close(&fp);

	return 0;
}

#ifdef DESIGN_VERIFICATION_BOOTLOADER

/* ─── Mock Flash ──────────────────────────────────────────── */
#define FIRMWARE_SIZE        (1u * 1024u)   /* default firmware size (256KB) */

/* ─── PC simulation thread (MAVLink cmd + ZMODEM) ───── */
struct bl_thread_arg {
	struct serial_port *serial;
	u32 fw_size;
	u32 fw_crc32;
	char fw_path[128];
};

static void *bl_computer_thread(void *data)
{
	struct bl_thread_arg *arg = (struct bl_thread_arg *)data;
	struct mavlink_device *pc_mav;
	const char *fw_path;
	int ret;

	pc_mav = mavlink_init(arg->serial, 2, 0, "pc-sim");
	if (!pc_mav) {
		pr_err("pc-sim: failed to init mavlink\n");
		return NULL;
	}

	/* Step 1: Send MAVLink prepare update cmd */
	pr_info("pc-sim: sending MAVLink BOOTLOADER_UPDATE_CMD...\n");
	ret = mavlink_send_bootloader_update(pc_mav,
		MAV_BL_CMD_PREPARE_UPDATE,
		arg->fw_size, arg->fw_crc32,
		BL_FIRMWARE_NAME, 1000);
	if (ret < 0)
		pr_warn("pc-sim: mavlink returned %d (may still be OK)\n", ret);

	/* Step 2: Wait for bootloader to enter RECEIVE + lrzsz_rz */
	mdelay(200);

	/* Step 3: Send firmware via ZMODEM */
	pr_info("pc-sim: sending firmware via ZMODEM...\n");
	fw_path = arg->fw_path;
	ret = lrzsz_sz(arg->serial, 1, &fw_path);
	pr_info("pc-sim: firmware send complete\n");

	mavlink_free(pc_mav);
	return NULL;
}

int t_bootloader_flow_test(int argc, char *argv[])
{
	struct bootloader_context *bl_ctx = NULL;
	struct bootloader_update_flag flag;
	struct bootloader_mem_layout layout;
	struct serial_port *serial_computer = NULL;
	struct bl_thread_arg *thread_arg = NULL;
	pthread_t thread = 0;
	u8 *file_buf = NULL;
	u32 fw_size = FIRMWARE_SIZE;
	u32 fw_crc32;
	u32 fw_offset;
	char computer_fw_path[64];
	char firmware_fw_path[64];
	FIL fp;
	UINT bw;
	int ret = 0;

	layout.rom_base = (u8 *)ROM_BASE_ADDR;
	layout.rom_size = ROM_TOTAL_SIZE;
	layout.bootloader_size = BOOTLOADER_SIZE;
	layout.firmware_max_size = FIRMWARE_MAX_SIZE;
	layout.sram.start = (u8 *)0x20000000u;
	layout.sram.end = (u8 *)0x20040000u;

	/* ── Parse args ─────────────────────────────────── */
	if (argc > 1) {
		fw_size = simple_strtol(argv[1], NULL, 10);
		if (fw_size < 1024 || fw_size > layout.rom_size / 2)
			return -EINVAL;
	}

	snprintf(computer_fw_path, sizeof(computer_fw_path),
		 "%s/%s", BL_COMPUTER_PATH, BL_FIRMWARE_NAME);
	snprintf(firmware_fw_path, sizeof(firmware_fw_path),
		 "%s/%s", BL_FIRMWARE_PATH, BL_FIRMWARE_NAME);

	pr_info("=== Bootloader Full Update Test (FW size: %u) ===\n", fw_size);

	/* ── 1. Mock flash ─────────────────────────────── */
	general_flash_addr = kzalloc(layout.rom_size, GFP_KERNEL);
	if (!general_flash_addr)
		return -ENOMEM;
	layout.rom_base = general_flash_addr;

	fw_offset = layout.bootloader_size;

	/* ── 2. Firmware file on computer side ──────────── */
	ret = fatfs_create_dirs(BL_COMPUTER_PATH);
	if (ret)
		goto cleanup;

	file_buf = kzalloc(fw_size, GFP_KERNEL);
	if (!file_buf) {
		ret = -ENOMEM;
		goto cleanup;
	}

	get_random_bytes(file_buf, fw_size);
	*(u32 *)&file_buf[0] = (u32)(u64)layout.sram.start + 0x1000;     /* stack */
	*(u32 *)&file_buf[4] = (u32)(u64)(general_flash_addr + fw_offset + 0x100); /* reset */
	fw_crc32 = crc32(~0, file_buf, fw_size) ^ ~0;

	ret = fatfs_create_file(BL_COMPUTER_PATH, BL_FIRMWARE_NAME);
	if (ret) {
		goto cleanup;
	}

	ret = fatfs_write_store_info(BL_COMPUTER_PATH, BL_FIRMWARE_NAME,
				     0, file_buf, fw_size);
	kfree(file_buf);
	file_buf = NULL;
	if (ret)
		goto cleanup;

	pr_info("firmware: %s (%u bytes, CRC32=0x%08x)\n",
		computer_fw_path, fw_size, fw_crc32);

	/* ── 3. Write update flag to trigger bootloader update mode ── */
	memset(&flag, 0, sizeof(flag));
	flag.magic = BL_UPDATE_FLAG_MAGIC;
	flag.firmware_size = fw_size;
	flag.firmware_crc32 = fw_crc32;
	strncpy(flag.firmware_filename, BL_FIRMWARE_NAME, sizeof(flag.firmware_filename) - 1);

	if (f_open(&fp, BL_UPDATE_FLAG_PATH, FA_CREATE_ALWAYS | FA_WRITE) != FR_OK) {
		ret = -EIO;
		goto cleanup;
	}
	ret = f_write(&fp, &flag, sizeof(flag), &bw);
	f_close(&fp);
	if (ret != FR_OK || bw != sizeof(flag)) {
		ret = -EACCES;
		goto cleanup;
	}

	/* ── 4. Bootloader init ────────────────────────── */
	bl_ctx = bootloader_init(&layout, &general_bootloader_ops,
				 &general_flash_ops, &usart1_serial_port_ops);
	if (!bl_ctx) {
		ret = -ENOMEM;
		goto cleanup;
	}

	pr_info("is_firmware_valid (before) = %d\n",
		is_firmware_valid(bl_ctx));

	/* ── 5. Serial port pair ───────────────────────── */
	serial_computer = serial_port_alloc(&usart1_serial_port_ops,
					    "bl-computer");
	if (IS_ERR(serial_computer)) {
		ret = PTR_ERR(serial_computer);
		serial_computer = NULL;
		goto cleanup;
	}
	serial_port_start_thread(serial_computer, SERIAL_PORT_DF_OTHERS,
				 NULL, bl_ctx->app_port);
	serial_port_start_thread(bl_ctx->app_port, SERIAL_PORT_DF_OTHERS,
				 NULL, serial_computer);

	/* ── 6. Start PC simulation thread ─────────────── */
	thread_arg = kzalloc(sizeof(*thread_arg), GFP_KERNEL);
	if (!thread_arg) {
		ret = -ENOMEM;
		goto cleanup;
	}
	thread_arg->serial = serial_computer;
	thread_arg->fw_size = fw_size;
	thread_arg->fw_crc32 = fw_crc32;
	strncpy(thread_arg->fw_path, computer_fw_path, sizeof(thread_arg->fw_path) - 1);

	ret = pthread_create(&thread, NULL, bl_computer_thread, thread_arg);
	if (ret != 0) {
		pr_err("failed to create thread: %d\n", ret);
		kfree(thread_arg);
		thread_arg = NULL;
		goto cleanup;
	}

	/* ── 7. Run bootloader FSM ─────────────────────── */
	ret = bootloader_flow(bl_ctx);

	pthread_join(thread, NULL);
	thread = 0;

	pr_info("=== Full Update %s ===\n", ret ? "FAIL" : "PASS");

cleanup:
	kfree(thread_arg);
	serial_port_free(serial_computer);
	bootloader_free(bl_ctx);
	kfree(file_buf);
	fatfs_delete_file(NULL, BL_UPDATE_FLAG_PATH);
	fatfs_delete_file(NULL, computer_fw_path);
	fatfs_delete_dir(BL_COMPUTER_PATH);
	fatfs_delete_dir(BL_FIRMWARE_PATH);
	fatfs_delete_dir(BL_BACKUP_PATH);
	kfree(general_flash_addr);
	general_flash_addr = NULL;
	return ret;
}

#endif /* DESIGN_VERIFICATION_BOOTLOADER */
