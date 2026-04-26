/**
 * @file bootloader.c
 * @brief Bootloader core logic for STM32F4 OTA firmware update
 *
 * Implements boot mode determination, APP jump, update state machine,
 * protocol event callback, and config region read/write.
 */

#include <linux/kernel.h>
#include <linux/crc32.h>
#include <linux/types.h>
#include <kinetis/bootloader/bootloader.h>

/*********************************************************************
 * Runtime State
 *********************************************************************/

static struct boot_config g_boot_config;
static struct update_stats g_update_stats;
static enum update_state g_update_state = UPDATE_STATE_IDLE;

/*********************************************************************
 * Config Region Operations
 *********************************************************************/

/**
 * @brief Read boot config from Flash config region
 * @param config: Output config structure
 * @return 0 on success, negative on error
 */
int bootloader_read_config(struct boot_config *config)
{
	const struct boot_config *flash_config =
		(const struct boot_config *)CONFIG_BASE_ADDR;

	if (!config)
		return -1;

	memcpy(config, flash_config, sizeof(*config));

	/* Validate magic number */
	if (config->magic != BOOT_CONFIG_MAGIC) {
		pr_warn("Boot config magic mismatch: 0x%08x, expected 0x%08x\n",
			config->magic, BOOT_CONFIG_MAGIC);
		/* Initialize with defaults */
		memset(config, 0, sizeof(*config));
		config->magic = BOOT_CONFIG_MAGIC;
		config->version = 1;
		config->boot_mode = BOOT_MODE_RUN;
	}

	return 0;
}

/**
 * @brief Write boot config to Flash config region
 * @param config: Config structure to write
 * @return 0 on success, negative on error
 */
int bootloader_write_config(const struct boot_config *config)
{
	int ret;

	if (!config)
		return -1;

	ret = flash_ops_erase_config_region();
	if (ret < 0) {
		pr_err("Failed to erase config region: %d\n", ret);
		return ret;
	}

	ret = flash_ops_unlock();
	if (ret < 0) {
		pr_err("Failed to unlock Flash for config write: %d\n", ret);
		return ret;
	}

	ret = flash_ops_write(CONFIG_BASE_ADDR,
			      (const u8 *)config, sizeof(*config));
	flash_ops_lock();

	if (ret < 0) {
		pr_err("Failed to write config region: %d\n", ret);
		return ret;
	}

	pr_info("Boot config written successfully\n");
	return 0;
}

/**
 * @brief Set boot mode in config region
 * @param mode: Boot mode to set
 * @return 0 on success, negative on error
 */
int bootloader_set_boot_mode(u8 mode)
{
	struct boot_config config;
	int ret;

	ret = bootloader_read_config(&config);
	if (ret < 0)
		return ret;

	config.boot_mode = mode;

	return bootloader_write_config(&config);
}

/*********************************************************************
 * APP Jump
 *********************************************************************/

/**
 * @brief Jump to the APP firmware
 * @note This function does not return on success.
 *       Disables interrupts, sets MSP, remaps VTOR, and jumps.
 */
void bootloader_jump_to_app(void)
{
	typedef void (*app_entry_t)(void);

	u32 app_stack;
	u32 app_entry_addr;
	app_entry_t app_entry;

	/* Disable all interrupts */
	__disable_irq();

	/* Reset all peripheral clocks and de-initialize HAL */
	HAL_DeInit();

	/* Disable SysTick */
	SysTick->CTRL = 0;
	SysTick->LOAD = 0;
	SysTick->VAL = 0;

	/* Clear all pending interrupt flags */
	for (u32 i = 0; i < 8; i++) {
		NVIC->ICER[i] = 0xFFFFFFFF;   /* Disable all IRQs */
		NVIC->ICPR[i] = 0xFFFFFFFF;   /* Clear all pending IRQs */
	}

	/* Read APP stack pointer and reset vector */
	app_stack = *(volatile u32 *)APP_BASE_ADDR;
	app_entry_addr = *(volatile u32 *)(APP_BASE_ADDR + 4);

	/* Set MSP from APP vector table */
	__set_MSP(app_stack);

	/* Remap VTOR to APP vector table */
	SCB->VTOR = APP_BASE_ADDR;

	/* Ensure all memory operations are complete */
	__DSB();
	__ISB();

	/* Jump to APP reset handler */
	app_entry = (app_entry_t)app_entry_addr;
	app_entry();

	/* Should never reach here */
	while (1)
		;
}

/*********************************************************************
 * Update State Machine
 *********************************************************************/

/**
 * @brief Reset update statistics
 */
static void reset_update_stats(void)
{
	memset(&g_update_stats, 0, sizeof(g_update_stats));
}

/**
 * @brief Calculate progress percentage
 * @return Progress percentage (0-100)
 */
static u8 calculate_progress(void)
{
	u32 percent;

	if (g_update_stats.firmware_size == 0)
		return 0;

	percent = (g_update_stats.bytes_received * 100) /
		  g_update_stats.firmware_size;

	if (percent > 100)
		percent = 100;

	return (u8)percent;
}

/**
 * @brief Handle the first data block (contains firmware header)
 * @param data: Pointer to received data block
 * @param len: Length of data block in bytes
 * @return 0 on success, negative on error
 */
static int handle_first_block(const u8 *data, u32 len)
{
	const struct firmware_header *hdr;
	int ret;

	if (len < FIRMWARE_HEADER_SIZE) {
		pr_err("First block too short: %u bytes, need %u\n",
		       len, (u32)FIRMWARE_HEADER_SIZE);
		return -1;
	}

	hdr = (const struct firmware_header *)data;

	/* Validate firmware size */
	if (hdr->total_size == 0 || hdr->total_size > APP_MAX_SIZE) {
		pr_err("Invalid firmware size: %u (max %u)\n",
		       hdr->total_size, (u32)APP_MAX_SIZE);
		return -1;
	}

	g_update_stats.firmware_size = hdr->total_size;
	g_update_stats.expected_crc32 = hdr->crc32;
	g_update_stats.write_offset = 0;
	g_update_stats.header_parsed = 1;

	pr_info("Firmware header: size=%u, crc32=0x%08x\n",
		hdr->total_size, hdr->crc32);

	/* Erase APP region */
	g_update_state = UPDATE_STATE_ERASING;
	pr_info("Erasing APP region...\n");

	ret = flash_ops_erase_app_region();
	if (ret < 0) {
		pr_err("Failed to erase APP region: %d\n", ret);
		g_update_state = UPDATE_STATE_ERROR;
		return ret;
	}

	pr_info("APP region erased successfully\n");
	g_update_state = UPDATE_STATE_RECEIVING;

	/* Write the payload portion of the first block (after header) */
	if (len > FIRMWARE_HEADER_SIZE) {
		const u8 *payload = data + FIRMWARE_HEADER_SIZE;
		u32 payload_len = len - FIRMWARE_HEADER_SIZE;
		u32 write_len;

		/* Cap payload to firmware size */
		if (payload_len > hdr->total_size)
			payload_len = hdr->total_size;

		/* 4-byte aligned write length */
		write_len = (payload_len + 3u) & ~3u;

		ret = flash_ops_unlock();
		if (ret < 0) {
			pr_err("Failed to unlock Flash: %d\n", ret);
			g_update_state = UPDATE_STATE_ERROR;
			return ret;
		}

		ret = flash_ops_write(APP_BASE_ADDR, payload, write_len);
		flash_ops_lock();

		if (ret < 0) {
			pr_err("Failed to write first block payload: %d\n", ret);
			g_update_state = UPDATE_STATE_ERROR;
			return ret;
		}

		g_update_stats.write_offset = payload_len;
		g_update_stats.bytes_written = payload_len;
		g_update_stats.bytes_received = payload_len;
		g_update_stats.blocks_received = 1;
		g_update_stats.progress = calculate_progress();

		/* Report progress */
		protocol_adapter_report_progress(
			g_update_stats.progress,
			g_update_stats.bytes_received,
			g_update_stats.firmware_size);
	}

	return 0;
}

/**
 * @brief Handle a subsequent data block
 * @param data: Pointer to received data block
 * @param len: Length of data block in bytes
 * @return 0 on success, negative on error
 */
static int handle_data_block(const u8 *data, u32 len)
{
	u32 write_addr;
	u32 write_len;
	u32 remaining;
	int ret;

	if (!g_update_stats.header_parsed) {
		pr_err("Data block received before header\n");
		return -1;
	}

	remaining = g_update_stats.firmware_size - g_update_stats.bytes_received;
	if (len > remaining)
		len = remaining;

	write_addr = APP_BASE_ADDR + g_update_stats.write_offset;

	/* 4-byte aligned write length */
	write_len = (len + 3u) & ~3u;

	ret = flash_ops_unlock();
	if (ret < 0) {
		pr_err("Failed to unlock Flash: %d\n", ret);
		g_update_state = UPDATE_STATE_ERROR;
		return ret;
	}

	ret = flash_ops_write(write_addr, data, write_len);
	flash_ops_lock();

	if (ret < 0) {
		pr_err("Failed to write data block at 0x%08x: %d\n",
		       write_addr, ret);
		g_update_state = UPDATE_STATE_ERROR;
		return ret;
	}

	g_update_stats.write_offset += len;
	g_update_stats.bytes_written += len;
	g_update_stats.bytes_received += len;
	g_update_stats.blocks_received++;
	g_update_stats.progress = calculate_progress();

	/* Report progress */
	protocol_adapter_report_progress(
		g_update_stats.progress,
		g_update_stats.bytes_received,
		g_update_stats.firmware_size);

	return 0;
}

/**
 * @brief Perform full CRC32 verification of APP region
 * @return 0 if CRC matches, negative on error
 */
static int verify_firmware_crc32(void)
{
	u32 actual_crc32;
	u32 firmware_size;

	firmware_size = g_update_stats.firmware_size;
	actual_crc32 = crc32(~0u,
			     (const unsigned char *)APP_BASE_ADDR,
			     firmware_size);
	actual_crc32 ^= ~0u;

	pr_info("CRC32 verification: expected=0x%08x, actual=0x%08x\n",
		g_update_stats.expected_crc32, actual_crc32);

	if (actual_crc32 != g_update_stats.expected_crc32) {
		pr_err("CRC32 mismatch! Expected 0x%08x, got 0x%08x\n",
		       g_update_stats.expected_crc32, actual_crc32);
		return -1;
	}

	pr_info("CRC32 verification passed\n");
	return 0;
}

/*********************************************************************
 * Public API
 *********************************************************************/

/**
 * @brief Initialize Bootloader: read config, determine boot mode
 * @return 0 on success, negative on error
 */
int bootloader_init(void)
{
	int ret;

	pr_info("Bootloader initializing...\n");

	/* Read config from Flash */
	ret = bootloader_read_config(&g_boot_config);
	if (ret < 0) {
		pr_err("Failed to read boot config: %d\n", ret);
		return ret;
	}

	pr_info("Boot mode: %d\n", g_boot_config.boot_mode);

	reset_update_stats();
	return 0;
}

/**
 * @brief Main Bootloader entry point (runs the state machine)
 * @note This function does not return in normal operation.
 *       It either jumps to APP or enters update loop.
 */
void bootloader_main(void)
{
	int ret;

	switch (g_boot_config.boot_mode) {
	case BOOT_MODE_RUN:
		/* Check APP validity */
		if (flash_ops_is_app_valid()) {
			pr_info("APP valid, jumping to 0x%08x\n", APP_BASE_ADDR);
			bootloader_jump_to_app();
		} else {
			pr_warn("APP invalid, entering recovery mode\n");
			g_boot_config.boot_mode = BOOT_MODE_RECOVERY;
			/* Fall through to RECOVERY handling */
		}
		fallthrough;

	case BOOT_MODE_UPDATE:
	case BOOT_MODE_RECOVERY:
		g_update_state = UPDATE_STATE_WAIT_PROTOCOL;
		break;

	default:
		pr_warn("Unknown boot mode: %d, entering recovery\n",
			g_boot_config.boot_mode);
		g_boot_config.boot_mode = BOOT_MODE_RECOVERY;
		g_update_state = UPDATE_STATE_WAIT_PROTOCOL;
		break;
	}

	/* Enter update/recovery loop */
	pr_info("Entering update loop (mode=%d)\n", g_boot_config.boot_mode);

	ret = protocol_adapter_init();
	if (ret < 0) {
		pr_err("Protocol adapter init failed: %d\n", ret);
		g_update_state = UPDATE_STATE_ERROR;
		goto error_loop;
	}

	while (1) {
		u8 recv_buf[1024];
		u32 recv_len = 0;
		int recv_result;

		switch (g_update_state) {
		case UPDATE_STATE_WAIT_PROTOCOL:
			pr_info("Waiting for protocol connection...\n");
			ret = protocol_adapter_wait_connection(-1);
			if (ret < 0) {
				pr_err("Protocol connection failed: %d\n", ret);
				continue;
			}
			g_update_state = UPDATE_STATE_RECEIVING;
			pr_info("Protocol connected, ready to receive\n");
			break;

		case UPDATE_STATE_RECEIVING:
			recv_result = protocol_adapter_recv_block(
				recv_buf, sizeof(recv_buf), &recv_len, -1);

			if (recv_result == PROTO_RECV_OK) {
				ret = bootloader_on_data_received(
					recv_buf, recv_len);
				if (ret < 0) {
					pr_err("Data handling failed: %d\n", ret);
					protocol_adapter_send_nack();
					g_update_state = UPDATE_STATE_ERROR;
				} else {
					protocol_adapter_send_ack();
				}
			} else if (recv_result == PROTO_RECV_COMPLETE) {
				/* Transfer complete, verify */
				pr_info("Transfer complete, starting verification\n");
				g_update_state = UPDATE_STATE_VERIFYING;
			} else if (recv_result == PROTO_RECV_CRC_ERR) {
				pr_warn("Block CRC error, requesting retransmit\n");
				protocol_adapter_send_nack();
			} else if (recv_result == PROTO_RECV_TIMEOUT) {
				/* Timeout, continue waiting */
				continue;
			} else {
				pr_err("Protocol receive error: %d\n",
				       recv_result);
				g_update_state = UPDATE_STATE_ERROR;
			}
			break;

		case UPDATE_STATE_VERIFYING: {
			int verify_ret;

			verify_ret = verify_firmware_crc32();
			if (verify_ret == 0) {
				g_update_state = UPDATE_STATE_COMPLETE;

				/* Update config region */
				g_boot_config.firmware_size =
					g_update_stats.firmware_size;
				g_boot_config.firmware_crc32 =
					g_update_stats.expected_crc32;
				g_boot_config.boot_mode = BOOT_MODE_RUN;
				g_boot_config.update_count++;

				ret = bootloader_write_config(&g_boot_config);
				if (ret < 0) {
					pr_err("Failed to write config: %d\n",
					       ret);
					g_update_state = UPDATE_STATE_ERROR;
					break;
				}

				protocol_adapter_report_result(1, 0, 0);
				pr_info("Update successful, resetting...\n");

				/* Small delay for protocol to send result */
				mdelay(100);
				bootloader_reset();
			} else {
				/* Verification failed */
				u32 actual_crc;

				actual_crc = crc32(~0u,
					(const unsigned char *)APP_BASE_ADDR,
					g_update_stats.firmware_size);
				actual_crc ^= ~0u;

				protocol_adapter_report_result(
					0,
					g_update_stats.expected_crc32,
					actual_crc);

				pr_err("Verification failed, staying in recovery\n");
				g_boot_config.boot_mode = BOOT_MODE_RECOVERY;
				bootloader_write_config(&g_boot_config);

				g_update_state = UPDATE_STATE_ERROR;
			}
			break;
		}

		case UPDATE_STATE_COMPLETE:
			/* Should not reach here, reset was called */
			mdelay(100);
			bootloader_reset();
			break;

		case UPDATE_STATE_ERROR:
			pr_err("Update error, waiting for retry\n");
			protocol_adapter_deinit();
			mdelay(500);
			protocol_adapter_init();
			reset_update_stats();
			g_update_state = UPDATE_STATE_WAIT_PROTOCOL;
			break;

		default:
			g_update_state = UPDATE_STATE_WAIT_PROTOCOL;
			break;
		}
	}

error_loop:
	/* Error loop with periodic retry */
	while (1) {
		mdelay(5000);
		bootloader_reset();
	}
}

/**
 * @brief Handle a protocol data event during update
 * @param data: Pointer to received data block
 * @param len: Length of data block in bytes
 * @return 0 on success, negative on error
 * @note First block is expected to contain firmware_header (8 bytes) + payload.
 *       Subsequent blocks are written directly to Flash.
 */
int bootloader_on_data_received(const u8 *data, u32 len)
{
	if (!data || len == 0)
		return -1;

	/* First block: parse firmware header */
	if (!g_update_stats.header_parsed)
		return handle_first_block(data, len);

	/* Subsequent blocks: write to Flash */
	return handle_data_block(data, len);
}

/**
 * @brief Get current update state
 * @return Current update_state enum value
 */
enum update_state bootloader_get_state(void)
{
	return g_update_state;
}

/**
 * @brief Get current update statistics
 * @return Pointer to update_stats structure
 */
const struct update_stats *bootloader_get_stats(void)
{
	return &g_update_stats;
}

/**
 * @brief Software reset (NVIC_SystemReset)
 * @note This function does not return
 */
void bootloader_reset(void)
{
	pr_info("Performing software reset...\n");
	NVIC_SystemReset();
	while (1)
		;
}
