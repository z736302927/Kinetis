/**
 * @file main.c
 * @brief Bootloader entry point for STM32F4
 *
 * Initializes HAL, clock, IWDG, then enters Bootloader.
 * This is the standalone Bootloader firmware entry point.
 */

#include <linux/kernel.h>
#include <linux/types.h>
#include <kinetis/bootloader/bootloader.h>

/*********************************************************************
 * IWDG Configuration
 *********************************************************************/

#define IWDG_TIMEOUT_MS    5000    /* 5 second watchdog timeout */

/**
 * @brief Initialize Independent Watchdog (IWDG)
 * @note IWDG is started with a generous timeout to allow Flash erase
 *       operations to complete. The bootloader must periodically feed
 *       the watchdog during long operations.
 */
static void iwdg_init(void)
{
	IWDG_HandleTypeDef hiwdg;

	hiwdg.Instance = IWDG;
	hiwdg.Init.Prescaler = IWDG_PRESCALER_256;
	hiwdg.Init.Reload = (40000u * IWDG_TIMEOUT_MS) / (256u * 1000u);
	hiwdg.Init.Window = 0x0FFF;

	if (HAL_IWDG_Init(&hiwdg) != HAL_OK) {
		pr_err("IWDG init failed\n");
	}
}

/*********************************************************************
 * Clock Configuration
 *********************************************************************/

/**
 * @brief System Clock Configuration
 * @note Configure HSE, PLL to achieve maximum system clock.
 *       STM32F407: 168MHz, STM32F401: 84MHz, etc.
 *       User should adjust PLL parameters for the specific MCU.
 */
static void system_clock_config(void)
{
	RCC_OscInitTypeDef rcc_osc = {0};
	RCC_ClkInitTypeDef rcc_clk = {0};

	/* Configure HSE and PLL */
	rcc_osc.OscillatorType = RCC_OSCILLATORTYPE_HSE;
	rcc_osc.HSEState = RCC_HSE_ON;
	rcc_osc.PLL.PLLState = RCC_PLL_ON;
	rcc_osc.PLL.PLLSource = RCC_PLLSOURCE_HSE;
	rcc_osc.PLL.PLLM = 8;      /* TODO: Adjust for your HSE frequency */
	rcc_osc.PLL.PLLN = 336;
	rcc_osc.PLL.PLLP = RCC_PLLP_DIV2;
	rcc_osc.PLL.PLLQ = 7;

	if (HAL_RCC_OscConfig(&rcc_osc) != HAL_OK) {
		pr_err("Oscillator config failed\n");
		while (1)
			;
	}

	/* Configure clocks */
	rcc_clk.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK |
			    RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
	rcc_clk.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
	rcc_clk.AHBCLKDivider = RCC_SYSCLK_DIV1;
	rcc_clk.APB1CLKDivider = RCC_HCLK_DIV4;
	rcc_clk.APB2CLKDivider = RCC_HCLK_DIV2;

	if (HAL_RCC_ClockConfig(&rcc_clk, FLASH_LATENCY_5) != HAL_OK) {
		pr_err("Clock config failed\n");
		while (1)
			;
	}
}

/*********************************************************************
 * Main Entry Point
 *********************************************************************/

/**
 * @brief Bootloader main entry
 * @note This function does not return. It either jumps to APP
 *       or enters the update state machine loop.
 */
int main(void)
{
	int ret;

	/* HAL initialization */
	HAL_Init();

	/* Configure system clock */
	system_clock_config();

	/* Initialize IWDG */
	iwdg_init();

	pr_info("\n\n=== STM32F4 Bootloader v1.0 ===\n");
	pr_info("Flash: 0x%08x - 0x%08x (%uKB)\n",
		FLASH_BASE_ADDR,
		FLASH_BASE_ADDR + FLASH_TOTAL_SIZE - 1,
		FLASH_TOTAL_SIZE / 1024);
	pr_info("APP:   0x%08x - 0x%08x (%uKB)\n",
		APP_BASE_ADDR,
		APP_BASE_ADDR + APP_MAX_SIZE - 1,
		APP_MAX_SIZE / 1024);
	pr_info("Config: 0x%08x (%uKB)\n",
		CONFIG_BASE_ADDR,
		CONFIG_SIZE / 1024);

	/* Initialize Bootloader */
	ret = bootloader_init();
	if (ret < 0) {
		pr_err("Bootloader init failed: %d\n", ret);
		while (1)
			;
	}

	/* Enter main Bootloader loop (does not return) */
	bootloader_main();

	/* Should never reach here */
	return 0;
}
