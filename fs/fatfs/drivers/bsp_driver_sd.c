/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file    bsp_driver_sd.c for H7 (based on stm32h743i_eval_sd.c)
 * @brief   This file includes a generic uSD card driver.
 *          To be completed by the user according to the board used for the project.
 * @note    Some functions generated as weak: they can be overridden by
 *          - code in user files
 *          - or BSP code from the FW pack files
 *          if such files are added to the generated project (by the user).
 ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
 ******************************************************************************
 */
/* USER CODE END Header */

/* USER CODE BEGIN FirstSection */
/* can be used to modify / undefine following code or add new definitions */
/* USER CODE END FirstSection */
/* Includes ------------------------------------------------------------------*/
#include <linux/printk.h>
#include <linux/iopoll.h>

#include "bsp_driver_sd.h"

/* Extern variables ---------------------------------------------------------*/


/* Compatibility macros: STM32F4 (SDIO) vs STM32H7 (SDMMC) ------------------*/
#if defined(SDMMC1)
  /* H7: SDMMC peripheral, no ClockBypass member */
  #define SD_INSTANCE                       SDMMC1
  #define SD_CLKREG                         SDMMC1
  #define SD_CLOCK_EDGE_RISING              SDMMC_CLOCK_EDGE_RISING
  #define SD_CLOCK_POWER_SAVE_DISABLE       SDMMC_CLOCK_POWER_SAVE_DISABLE
  #define SD_BUS_WIDE_1B                    SDMMC_BUS_WIDE_1B
  #define SD_BUS_WIDE_4B                    SDMMC_BUS_WIDE_4B
  #define SD_BUS_WIDE_8B                    SDMMC_BUS_WIDE_8B
  #define SD_HARDWARE_FLOW_CONTROL_DISABLE  SDMMC_HARDWARE_FLOW_CONTROL_DISABLE
  #define SD_HARDWARE_FLOW_CONTROL_ENABLE   SDMMC_HARDWARE_FLOW_CONTROL_ENABLE
  #define SD_CLKCR_CLKDIV                   SDMMC_CLKCR_CLKDIV
  #define SD_HAS_CLOCKBYPASS                0
  #define SD_PERIPH_NAME                    "SDMMC"
  #define SD_HANDLE                         hsd1
  extern SD_HandleTypeDef                   hsd1;
  #define SD_CLEAN_DCACHE(addr, len)        SCB_CleanDCache_by_Addr((uint32_t *)(addr), (len))
  #define SD_INVALIDATE_DCACHE(addr, len)   SCB_InvalidateDCache_by_Addr((uint32_t *)(addr), (len))
#elif defined(SDIO)
  /* F4: SDIO peripheral, has ClockBypass member */
  #define SD_INSTANCE                       SDIO
  #define SD_CLKREG                         SDIO
  #define SD_CLOCK_EDGE_RISING              SDIO_CLOCK_EDGE_RISING
  #define SD_CLOCK_POWER_SAVE_DISABLE       SDIO_CLOCK_POWER_SAVE_DISABLE
  #define SD_BUS_WIDE_1B                    SDIO_BUS_WIDE_1B
  #define SD_BUS_WIDE_4B                    SDIO_BUS_WIDE_4B
  #define SD_BUS_WIDE_8B                    SDIO_BUS_WIDE_8B
  #define SD_HARDWARE_FLOW_CONTROL_DISABLE  SDIO_HARDWARE_FLOW_CONTROL_DISABLE
  #define SD_HARDWARE_FLOW_CONTROL_ENABLE   SDIO_HARDWARE_FLOW_CONTROL_ENABLE
  #define SD_CLKCR_CLKDIV                   SDIO_CLKCR_CLKDIV
  #define SD_HAS_CLOCKBYPASS                1
  #define SD_PERIPH_NAME                    "SDIO"
  #define SD_HANDLE                         hsd
  extern SD_HandleTypeDef                   hsd;
  #define SD_CLEAN_DCACHE(addr, len)        do { } while (0)
  #define SD_INVALIDATE_DCACHE(addr, len)   do { } while (0)
#else
  #error "Neither SDMMC1 nor SDIO peripheral is defined. Check STM32 HAL headers."
#endif

struct transfer_complete_result {
	volatile u8 tx_done;
	volatile u8 rx_done;
	volatile u8 error;
};

extern struct transfer_complete_result sd_dma_result;

/*
 * Clock formulas (from reference manual):
 *   H7 (SDMMC):  SD_CK = periph_clk / (2 * CLKDIV)       [CLKDIV ≥ 1]
 *                 SD_CK = periph_clk (bypass)              [CLKDIV = 0]
 *   F4 (SDIO):   SD_CK = periph_clk / (CLKDIV + 2)
 *
 * Clock targets per card type:
 *            Read          Write
 *   SDSC:    25 MHz        25 MHz      (Default Speed max, div=4)
 *   SDHC/SDXC: 50 MHz      50 MHz      (High Speed max, div=2, HWFC enabled)
 *
 * Clock is switched on-the-fly in BSP_SD_ReadBlocks / WriteBlocks.
 * The card survives a clock change as long as it is in Transfer state.
 */

#if defined(SDMMC1)
  /* STM32H7: SDMMC kernel clock (PLL2 typically 200 MHz) */
  #define SD_GET_PERIPH_CLK()      HAL_RCCEx_GetPeriphCLKFreq(RCC_PERIPHCLK_SDMMC)
  /* SD_CK = periph_clk / (2 * CLKDIV), bypass when div=0 */
  #define SD_CLK_HZ(periph, div)    ((div) == 0 ? (periph) : ((periph) / (2 * (div))))
  /* Ceiling division: CLKDIV = ceil(periph / (2 * target)) ensures SD_CK ≤ target
   * Formula: (p + 2*tgt - 1) / (2*tgt), minimum 1 since bypass is unused */
  #define SD_COMPUTE_CLKDIV(p, tgt) ((((p) + 2 * (tgt) - 1) / (2 * (tgt))) == 0 ? 1U : (((p) + 2 * (tgt) - 1) / (2 * (tgt))))
#elif defined(SDIO)
  /* STM32F4: SDIO clock from PCLK2 (APB2) */
  #define SD_GET_PERIPH_CLK()      HAL_RCC_GetPCLK2Freq()
  /* SD_CK = periph_clk / (CLKDIV + 2) */
  #define SD_CLK_HZ(periph, div)    ((periph) / ((div) + 2))
  #define SD_COMPUTE_CLKDIV(p, tgt) (((p) / (tgt) < 2) ? 0U : ((p) / (tgt) - 2))
#else
  #define SD_GET_PERIPH_CLK()      48000000UL
  #define SD_CLK_HZ(periph, div)    ((periph) / ((div) + 2))
  #define SD_COMPUTE_CLKDIV(p, tgt) (((p) / (tgt) < 2) ? 0U : ((p) / (tgt) - 2))
#endif

/* Current clock dividers, set during BSP_SD_Init based on card type */
static u32 g_sd_read_clkdiv  = 10;
static u32 g_sd_write_clkdiv = 10;

#define SD_DMA_POLL_TIMEOUT_US  5000000U

/**
 * @brief  Set SDMMC CLKCR divider on-the-fly, preserving bus width.
 *
 * Called before every read/write block operation to reconfigure the
 * clock without reinitialising the card.  The card must be in Transfer
 * state (no active transfer).
 *
 * @note   CLKCR is write-protected while DPSM is active
 *         (SDMMC_STA_DPSMACT bit).  If the hardware silently ignored
 *         the write, the function retries up to 3 times with a
 *         read-back verify + DSB to flush the store buffer.
 */
static void sd_set_clkdiv(uint32_t div)
{
	uint32_t clkcr;
	int retry;

	clkcr = SD_CLKREG->CLKCR;
	clkcr = (clkcr & ~SD_CLKCR_CLKDIV) | (div & SD_CLKCR_CLKDIV);

	for (retry = 0; retry < 3; retry++) {
		SD_CLKREG->CLKCR = clkcr;
		__DSB();  /* flush store buffer so peripheral sees the write */

		if ((SD_CLKREG->CLKCR & SD_CLKCR_CLKDIV) ==
		    (div & SD_CLKCR_CLKDIV))
			return;  /* write took — success */

		/* DPSM probably still active; short back-off */
		volatile unsigned int wait = 1000;
		while (--wait) __NOP();
	}

	/* Silently accept: next operation's error code will surface the
	 * problem (e.g. RX_OVERRUN / TX_UNDERRUN). */
}

static volatile u8 sd_wait_for_completion = 1;

void sd_set_wait_for_completion(u8 enable)
{
	sd_wait_for_completion = enable ? 1 : 0;
}

/* USER CODE BEGIN BeforeInitSection */
/* can be used to modify / undefine following code or add code */
/* USER CODE END BeforeInitSection */
/**
  * @brief  Initializes the SD card device.
  * @retval SD status
  */
__weak uint8_t BSP_SD_Init(void)
{
  HAL_SD_CardInfoTypeDef card_info;
  uint8_t sd_state = MSD_OK;
  const char *type_str;
  const char *bus_str;
  uint64_t total_bytes;
  uint64_t mb;
  u32 periph_clk;
  u32 rd_target, wr_target;
  u32 rd_khz, wr_khz;

  /* Check if the SD card is plugged in the slot */
  if (BSP_SD_IsDetected() != SD_PRESENT)
  {
    return MSD_ERROR_SD_NOT_PRESENT;
  }

  /* --- Step 1: Init with safe settings (1-bit bus, slow clock) --- */
  /* On H7: MX_SDMMC1_SD_Init() already called HAL_SD_Init().
   *         Skip re-init to avoid double HAL_SD_MspInit which would
   *         re-configure PLL2 / GPIO / NVIC and corrupt the active card. */
#if defined(SDMMC1)
  if (SD_HANDLE.State == HAL_SD_STATE_RESET)
#endif
  {
    SD_HANDLE.Instance = SD_INSTANCE;
    SD_HANDLE.Init.ClockEdge   = SD_CLOCK_EDGE_RISING;
#if SD_HAS_CLOCKBYPASS
    SD_HANDLE.Init.ClockBypass = SDIO_CLOCK_BYPASS_DISABLE;
#endif
    SD_HANDLE.Init.ClockPowerSave = SD_CLOCK_POWER_SAVE_DISABLE;
    SD_HANDLE.Init.BusWide     = SD_BUS_WIDE_1B;
    SD_HANDLE.Init.HardwareFlowControl = SD_HARDWARE_FLOW_CONTROL_ENABLE;
    SD_HANDLE.Init.ClockDiv    = 0;

    sd_state = HAL_SD_Init(&SD_HANDLE);
    if (sd_state != MSD_OK)
    {
      return sd_state;
    }
  }

  /* --- Step 2: Read card type from CSD register --- */
  HAL_SD_GetCardInfo(&SD_HANDLE, &card_info);

  switch (card_info.CardType)
  {
    case CARD_SDSC:      type_str = "SDSC";      break;
    case CARD_SDHC_SDXC: type_str = "SDHC/SDXC"; break;
    case CARD_SECURED:   type_str = "SECURED";   break;
    default:             type_str = "?";         break;
  }
  pr_info("SD card: type=%s, version=%lu, class=%lu, RCA=0x%04lX",
          type_str, card_info.CardVersion, card_info.Class, card_info.RelCardAdd);
  total_bytes = (uint64_t)card_info.LogBlockNbr * card_info.LogBlockSize;
  mb = total_bytes / (1024 * 1024);
  if (mb >= 1024) {
    unsigned long gb = (unsigned long)(mb / 1024);
    unsigned long frac = (unsigned long)((mb % 1024) * 100 / 1024);
    pr_info("SD card: %lu blocks x %lu B, capacity %lu.%02lu GB",
            card_info.LogBlockNbr, card_info.LogBlockSize, gb, frac);
  } else
    pr_info("SD card: %lu blocks x %lu B, capacity %lu MB",
            card_info.LogBlockNbr, card_info.LogBlockSize, (unsigned long)mb);

  /* --- Step 3: Configure bus width (at safe clock) --- */
  pr_info("%s: step3 - configuring bus width...", SD_PERIPH_NAME);

  if (card_info.CardType == CARD_SDSC)
  {
    /* SDSC: stay in 1-bit mode, no negotiation needed */
    SD_HANDLE.Init.BusWide = SD_BUS_WIDE_1B;
    pr_info("%s: step3 - SDSC -> 1-bit (fixed)", SD_PERIPH_NAME);
  }
  else
  {
    /* SDHC/SDXC: attempt 4-bit with HAL state sanitized */
    SD_HANDLE.ErrorCode = HAL_SD_ERROR_NONE;
    SD_HANDLE.State     = HAL_SD_STATE_READY;

    if (HAL_SD_ConfigWideBusOperation(&SD_HANDLE, SD_BUS_WIDE_4B) == HAL_OK)
    {
      SD_HANDLE.Init.BusWide = SD_BUS_WIDE_4B;
      pr_info("%s: step3 - 4-bit OK", SD_PERIPH_NAME);
    }
    else
    {
      /* Force 1-bit: both card (ACMD6(0)) and controller (CLKCR) */
      SD_CLKREG->CLKCR &= ~((uint32_t)0x0000C000UL);
      SD_HANDLE.ErrorCode = HAL_SD_ERROR_NONE;
      SD_HANDLE.State     = HAL_SD_STATE_READY;
      HAL_SD_ConfigWideBusOperation(&SD_HANDLE, SD_BUS_WIDE_1B);
      SD_HANDLE.Init.BusWide = SD_BUS_WIDE_1B;
      pr_info("%s: step3 - 4-bit FAIL, forced 1-bit", SD_PERIPH_NAME);
    }
  }

#if defined(SDMMC1)
  if (card_info.CardType != CARD_SDSC) {
    if (HAL_SD_ConfigSpeedBusOperation(&SD_HANDLE, SDMMC_SPEED_MODE_HIGH) == HAL_OK) {
      pr_info("%s: step3.5 - High Speed OK", SD_PERIPH_NAME);
    } else {
      pr_info("%s: step3.5 - High Speed not supported, staying at Default Speed", SD_PERIPH_NAME);
    }
  }
#endif

  /* --- Step 4: Set read/write clocks based on card type ---
   *            Read          Write
   *   SDSC:    25 MHz        25 MHz      (Default Speed max)
   *   SDHC/SDXC: 50 MHz      50 MHz      (High Speed max, HWFC enabled)  */
  periph_clk = SD_GET_PERIPH_CLK();
  pr_info("%s: SDMMC kernel clock = %lu kHz", SD_PERIPH_NAME, (unsigned long)(periph_clk / 1000));

  if (card_info.CardType == CARD_SDSC) {
    rd_target = 25000000UL;    /* 25 MHz - Default Speed max (div=4) */
    wr_target = 25000000UL;    /* 25 MHz - Default Speed max (div=4) */
  } else {
    rd_target = 45000000UL;    /* 45 MHz - High Speed (div=2) */
    wr_target = 45000000UL;    /* 45 MHz - High Speed (div=2) */
  }

  /* Clamp targets to hardware limits */
#if defined(SDMMC1)
  if (rd_target > periph_clk)
    rd_target = periph_clk;
  if (wr_target > periph_clk)
    wr_target = periph_clk;
#else
  if (rd_target > periph_clk / 2)
    rd_target = periph_clk / 2;
  if (wr_target > periph_clk / 2)
    wr_target = periph_clk / 2;
#endif

  g_sd_read_clkdiv  = SD_COMPUTE_CLKDIV(periph_clk, rd_target);
  g_sd_write_clkdiv = SD_COMPUTE_CLKDIV(periph_clk, wr_target);

  /* Apply read clock (write clock is set per-op) */
  sd_set_clkdiv(g_sd_read_clkdiv);
  SD_HANDLE.Init.ClockDiv = g_sd_read_clkdiv;
  SD_HANDLE.ErrorCode = HAL_SD_ERROR_NONE;
  SD_HANDLE.State     = HAL_SD_STATE_READY;

  rd_khz = SD_CLK_HZ(periph_clk, g_sd_read_clkdiv) / 1000;
  wr_khz = SD_CLK_HZ(periph_clk, g_sd_write_clkdiv) / 1000;

  if (SD_HANDLE.Init.BusWide == SD_BUS_WIDE_8B)
    bus_str = "8-bit";
  else if (SD_HANDLE.Init.BusWide == SD_BUS_WIDE_4B)
    bus_str = "4-bit";
  else
    bus_str = "1-bit";
  pr_info("%s: init done - rd=%lu MHz (div=%u), wr=%lu MHz (div=%u), bus=%s",
          SD_PERIPH_NAME,
          (unsigned long)rd_khz / 1000,
          (unsigned int)g_sd_read_clkdiv,
          (unsigned long)wr_khz / 1000,
          (unsigned int)g_sd_write_clkdiv,
          bus_str);

  pr_info("%s: BSP_SD_Init done, returning %u",
          SD_PERIPH_NAME,
          sd_state);
  return sd_state;
}
/* USER CODE BEGIN AfterInitSection */
/* can be used to modify previous code / undefine following code / add code */
/* USER CODE END AfterInitSection */

/* USER CODE BEGIN InterruptMode */
/**
  * @brief  Configures Interrupt mode for SD detection pin.
  * @retval Returns 0
  */
__weak uint8_t BSP_SD_ITConfig(void)
{
  /* Code to be updated by the user or replaced by one from the FW pack (in a stmxxxx_sd.c file) */

  return (uint8_t)0;
}

/* USER CODE END InterruptMode */

/* USER CODE BEGIN BeforeReadBlocksSection */
/* can be used to modify previous code / undefine following code / add code */
/* USER CODE END BeforeReadBlocksSection */
/**
  * @brief  Reads block(s) from a specified address in an SD card, in polling mode.
  * @param  pData: Pointer to the buffer that will contain the data to transmit
  * @param  ReadAddr: Address from where data is to be read
  * @param  NumOfBlocks: Number of SD blocks to read
  * @param  Timeout: Timeout for read operation
  * @retval SD status
  */
__weak uint8_t BSP_SD_ReadBlocks(uint32_t *pData, uint32_t ReadAddr, uint32_t NumOfBlocks, uint32_t Timeout)
{
  uint8_t sd_state = MSD_OK;

  if (HAL_SD_ReadBlocks(&SD_HANDLE, (uint8_t *)pData, ReadAddr, NumOfBlocks, Timeout) != HAL_OK)
  {
    sd_state = MSD_ERROR;
    pr_err("SD read failed: LBA=%lu, cnt=%lu, State=%d, EC=0x%lx",
           (unsigned long)ReadAddr, (unsigned long)NumOfBlocks,
           (int)SD_HANDLE.State, (unsigned long)SD_HANDLE.ErrorCode);
  }

  return sd_state;
}

/* USER CODE BEGIN BeforeWriteBlocksSection */
/* can be used to modify previous code / undefine following code / add code */
/* USER CODE END BeforeWriteBlocksSection */
/**
  * @brief  Writes block(s) to a specified address in an SD card, in polling mode.
  * @param  pData: Pointer to the buffer that will contain the data to transmit
  * @param  WriteAddr: Address from where data is to be written
  * @param  NumOfBlocks: Number of SD blocks to write
  * @param  Timeout: Timeout for write operation
  * @retval SD status
  */
__weak uint8_t BSP_SD_WriteBlocks(uint32_t *pData, uint32_t WriteAddr, uint32_t NumOfBlocks, uint32_t Timeout)
{
  uint8_t sd_state = MSD_OK;

  if (HAL_SD_WriteBlocks(&SD_HANDLE, (uint8_t *)pData, WriteAddr, NumOfBlocks, Timeout) != HAL_OK)
  {
    sd_state = MSD_ERROR;
    pr_err("SD write failed: LBA=%lu, cnt=%lu, State=%d, EC=0x%lx",
           (unsigned long)WriteAddr, (unsigned long)NumOfBlocks,
           (int)SD_HANDLE.State, (unsigned long)SD_HANDLE.ErrorCode);
  }

  return sd_state;
}

/* USER CODE BEGIN BeforeReadDMABlocksSection */
/* can be used to modify previous code / undefine following code / add code */
/* USER CODE END BeforeReadDMABlocksSection */
/**
  * @brief  Reads block(s) from a specified address in an SD card, in DMA mode.
  * @param  pData: Pointer to the buffer that will contain the data to transmit
  * @param  ReadAddr: Address from where data is to be read
  * @param  NumOfBlocks: Number of SD blocks to read
  * @retval SD status
  */
__weak uint8_t BSP_SD_ReadBlocks_DMA(uint32_t *pData, uint32_t ReadAddr, uint32_t NumOfBlocks)
{
  uint8_t sd_state = MSD_OK;
  u32 val;
  int ret;

  if (!sd_wait_for_completion) {
    ret = readl_poll_timeout(&sd_dma_result.rx_done, val, val || sd_dma_result.error,
                              0, SD_DMA_POLL_TIMEOUT_US);
    if (ret < 0) {
      sd_state = MSD_ERROR;
      pr_err("Last SD DMA read timeout: LBA=%lu, cnt=%lu, State=%d, EC=0x%lx",
              (unsigned long)ReadAddr, (unsigned long)NumOfBlocks,
              (int)SD_HANDLE.State, (unsigned long)SD_HANDLE.ErrorCode);
      HAL_SD_Abort(&SD_HANDLE);
    } else if (sd_dma_result.error) {
      sd_state = MSD_ERROR;
    }
  }

  sd_dma_result.rx_done = 0;
  sd_dma_result.error   = 0;

  SD_CLEAN_DCACHE(pData, NumOfBlocks * 512);

  if (HAL_SD_ReadBlocks_DMA(&SD_HANDLE, (uint8_t *)pData, ReadAddr, NumOfBlocks) != HAL_OK)
  {
    sd_state = MSD_ERROR;
    pr_err("SD DMA read failed to start: LBA=%lu, cnt=%lu, State=%d, EC=0x%lx",
           (unsigned long)ReadAddr, (unsigned long)NumOfBlocks,
           (int)SD_HANDLE.State, (unsigned long)SD_HANDLE.ErrorCode);
    return sd_state;
  }

  if (sd_wait_for_completion) {
    ret = readl_poll_timeout(&sd_dma_result.rx_done, val, val || sd_dma_result.error,
                             0, SD_DMA_POLL_TIMEOUT_US);
    if (ret < 0) {
      sd_state = MSD_ERROR;
      pr_err("SD DMA read timeout: LBA=%lu, cnt=%lu, State=%d, EC=0x%lx",
             (unsigned long)ReadAddr, (unsigned long)NumOfBlocks,
             (int)SD_HANDLE.State, (unsigned long)SD_HANDLE.ErrorCode);
      HAL_SD_Abort(&SD_HANDLE);
    } else if (sd_dma_result.error) {
      sd_state = MSD_ERROR;
    }
  }

  if (sd_state == MSD_OK)
    SD_INVALIDATE_DCACHE(pData, NumOfBlocks * 512);

  return sd_state;
}

/* USER CODE BEGIN BeforeWriteDMABlocksSection */
/* can be used to modify previous code / undefine following code / add code */
/* USER CODE END BeforeWriteDMABlocksSection */
/**
  * @brief  Writes block(s) to a specified address in an SD card, in DMA mode.
  * @param  pData: Pointer to the buffer that will contain the data to transmit
  * @param  WriteAddr: Address from where data is to be written
  * @param  NumOfBlocks: Number of SD blocks to write
  * @retval SD status
  */
__weak uint8_t BSP_SD_WriteBlocks_DMA(uint32_t *pData, uint32_t WriteAddr, uint32_t NumOfBlocks)
{
  uint8_t sd_state = MSD_OK;
  u32 val;
  int ret;

  if (!sd_wait_for_completion) {
    ret = readl_poll_timeout(&sd_dma_result.tx_done, val,
                              val || sd_dma_result.error,
                              0, SD_DMA_POLL_TIMEOUT_US);
    if (ret < 0) {
      sd_state = MSD_ERROR;
      pr_err("Last SD DMA write timeout: LBA=%lu, cnt=%lu, State=%d, EC=0x%lx",
              (unsigned long)WriteAddr, (unsigned long)NumOfBlocks,
              (int)SD_HANDLE.State, (unsigned long)SD_HANDLE.ErrorCode);
      HAL_SD_Abort(&SD_HANDLE);
    } else if (sd_dma_result.error) {
      sd_state = MSD_ERROR;
    }
  }

  sd_dma_result.tx_done = 0;
  sd_dma_result.error   = 0;

  SD_CLEAN_DCACHE(pData, NumOfBlocks * 512);

  /* Write block(s) in DMA transfer mode */
  if (HAL_SD_WriteBlocks_DMA(&SD_HANDLE, (uint8_t *)pData, WriteAddr, NumOfBlocks) != HAL_OK)
  {
    sd_state = MSD_ERROR;
    pr_err("SD DMA write failed to start: LBA=%lu, cnt=%lu, State=%d, EC=0x%lx",
           (unsigned long)WriteAddr, (unsigned long)NumOfBlocks,
           (int)SD_HANDLE.State, (unsigned long)SD_HANDLE.ErrorCode);
    return sd_state;
  }

  if (sd_wait_for_completion) {
    ret = readl_poll_timeout(&sd_dma_result.tx_done, val,
                             val || sd_dma_result.error,
                             0, SD_DMA_POLL_TIMEOUT_US);
    if (ret < 0) {
      sd_state = MSD_ERROR;
      pr_err("SD DMA write timeout: LBA=%lu, cnt=%lu, State=%d, EC=0x%lx",
             (unsigned long)WriteAddr, (unsigned long)NumOfBlocks,
             (int)SD_HANDLE.State, (unsigned long)SD_HANDLE.ErrorCode);
      HAL_SD_Abort(&SD_HANDLE);
    } else if (sd_dma_result.error) {
      sd_state = MSD_ERROR;
    }
  }

  return sd_state;
}

/* USER CODE BEGIN BeforeEraseSection */
/* can be used to modify previous code / undefine following code / add code */
/* USER CODE END BeforeEraseSection */
/**
  * @brief  Erases the specified memory area of the given SD card.
  * @param  StartAddr: Start byte address
  * @param  EndAddr: End byte address
  * @retval SD status
  */
__weak uint8_t BSP_SD_Erase(uint32_t StartAddr, uint32_t EndAddr)
{
  uint8_t sd_state = MSD_OK;

  if (HAL_SD_Erase(&SD_HANDLE, StartAddr, EndAddr) != HAL_OK)
  {
    sd_state = MSD_ERROR;
  }

  return sd_state;
}

/* USER CODE BEGIN BeforeGetCardStateSection */
/* can be used to modify previous code / undefine following code / add code */
/* USER CODE END BeforeGetCardStateSection */

/**
  * @brief  Gets the current SD card data status.
  * @param  None
  * @retval Data transfer state.
  *          This value can be one of the following values:
  *            @arg  SD_TRANSFER_OK: No data transfer is acting
  *            @arg  SD_TRANSFER_BUSY: Data transfer is acting
  */
__weak uint8_t BSP_SD_GetCardState(void)
{
  return ((HAL_SD_GetCardState(&SD_HANDLE) == HAL_SD_CARD_TRANSFER ) ? SD_TRANSFER_OK : SD_TRANSFER_BUSY);
}

/**
  * @brief  Get SD information about specific SD card.
  * @param  CardInfo: Pointer to HAL_SD_CardInfoTypedef structure
  * @retval None
  */
__weak void BSP_SD_GetCardInfo(HAL_SD_CardInfoTypeDef *CardInfo)
{
  /* Get SD card Information */
  HAL_SD_GetCardInfo(&SD_HANDLE, CardInfo);
}

/* USER CODE BEGIN BeforeCallBacksSection */
/* can be used to modify previous code / undefine following code / add code */
/* USER CODE END BeforeCallBacksSection */
/**
  * @brief SD Abort callbacks
  * @param hsd: SD handle
  * @retval None
  */
void HAL_SD_AbortCallback(SD_HandleTypeDef *hsd)
{
  BSP_SD_AbortCallback();
}

/**
  * @brief Tx Transfer completed callback
  * @param hsd: SD handle
  * @retval None
  */
__weak void HAL_SD_TxCpltCallback(SD_HandleTypeDef *hsd)
{
  BSP_SD_WriteCpltCallback();
}

/**
  * @brief Rx Transfer completed callback
  * @param hsd: SD handle
  * @retval None
  */
__weak void HAL_SD_RxCpltCallback(SD_HandleTypeDef *hsd)
{
  BSP_SD_ReadCpltCallback();
}

/* USER CODE BEGIN CallBacksSection_C */
/**
  * @brief BSP SD Abort callback
  * @retval None
  * @note empty (up to the user to fill it in or to remove it if useless)
  */
__weak void BSP_SD_AbortCallback(void)
{

}

/**
  * @brief BSP Tx Transfer completed callback
  * @retval None
  * @note empty (up to the user to fill it in or to remove it if useless)
  */
__weak void BSP_SD_WriteCpltCallback(void)
{

}

/**
  * @brief BSP Rx Transfer completed callback
  * @retval None
  * @note empty (up to the user to fill it in or to remove it if useless)
  */
__weak void BSP_SD_ReadCpltCallback(void)
{

}
/* USER CODE END CallBacksSection_C */

/**
 * @brief  Detects if SD card is correctly plugged in the memory slot or not.
 * @param  None
 * @retval Returns if SD is detected or not
 */
__weak uint8_t BSP_SD_IsDetected(void)
{
  __IO uint8_t status = SD_PRESENT;

  /* USER CODE BEGIN IsDetectedSection */
  /* user code can be inserted here */
  /* USER CODE END IsDetectedSection */

  return status;
}

/* USER CODE BEGIN AdditionalCode */
/* user code can be inserted here */
/* USER CODE END AdditionalCode */
