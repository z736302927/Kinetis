
#include <linux/printk.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/delay.h>

#include <kinetis/fatfs-intf.h>
#include <kinetis/serial-port.h>
#include <kinetis/mavlink.h>
#include "kinetis/bootloader.h"
#include "kinetis/fatfs.h"

/* config.h provides BL_MEM_LAYOUT (chip-specific layout shortcut). */
#include "config.h"
#include "project.h"
#include "interface.h"

#if KINETIS_FAKE_SIM == 0
/* CubeMX-generated HAL init — declared in main.h */
#include "main.h"
#endif

/* Linker-provided layout symbols (from linker.ld) */
extern u8  _ld_rom_base[];
extern u32 _ld_rom_size;
extern u32 _ld_bootloader_size;
extern u8  _ld_app_flash_start[];
extern u32 _ld_app_flash_size;
extern u8  _ld_sram_start[];
extern u32 _ld_sram_size;

int board_init(void)
{
	pr_info("==== %s v%s ====\n", PROJECT_NAME, PROJECT_VERSION);

#if KINETIS_FAKE_SIM
	return 0;
#else
	arm_dwt_init();

	return stm32_hal_init();
#endif
}

int app_main(void)
{
	struct bootloader_context *bl_ctx;
	struct bootloader_mem_layout layout;
	int ret;

	layout.rom_base = _ld_rom_base;
	layout.rom_size = (u32)&_ld_rom_size;
	layout.bootloader_size = (u32)&_ld_bootloader_size;
	layout.firmware_max_size = (u32)&_ld_app_flash_size;
	layout.sram.start = _ld_sram_start;
	layout.sram.end = _ld_sram_start + (u32)&_ld_sram_size;

	pr_info("Bootloader: starting on real hardware\n");
	pr_info("  ROM base:    0x%08x\n", (u32)(u64)layout.rom_base);
	pr_info("  Bootloader:  0x%08x (%u KB)\n",
		(u32)(u64)layout.rom_base, layout.bootloader_size / 1024);
	pr_info("  Firmware:    0x%08x\n",
		(u32)(u64)(layout.rom_base + layout.bootloader_size));

	ret = fatfs_init();
	if (ret) {
		return ret;
	}

	bl_ctx = bootloader_init(&layout,
			&general_bootloader_ops,
			&general_flash_ops,
			&usart2_serial_port_ops);
	if (!bl_ctx) {
		pr_err("Bootloader init failed\n");
		return -ENOMEM;
	}
	stm32_serial_port[1] = bl_ctx->app_port;

	ret = bootloader_flow(bl_ctx);

	/*
	 * bootloader_flow() normally never returns on hardware
	 * (it either jumps to firmware or resets). If it does
	 * return, clean up and halt.
	 */
	pr_err("Bootloader flow returned unexpectedly: %d\n", ret);
	bootloader_free(bl_ctx);

	return ret;
}
