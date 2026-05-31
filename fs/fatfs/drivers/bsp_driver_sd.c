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

#include "bsp_driver_sd.h"

/* Extern variables ---------------------------------------------------------*/


/* Compatibility macros: STM32F4 (SDIO) vs STM32H7 (SDMMC) ------------------*/
#if defined(SDMMC1)
  /* H7: SDMMC peripheral, no ClockBypass member */
  #define SD_INSTANCE                 SDMMC1
  #define SD_CLKREG                   SDMMC1
  #define SD_CLOCK_EDGE_RISING        SDMMC_CLOCK_EDGE_RISING
  #define SD_CLOCK_POWER_SAVE_DISABLE SDMMC_CLOCK_POWER_SAVE_DISABLE
  #define SD_BUS_WIDE_1B              SDMMC_BUS_WIDE_1B
  #define SD_BUS_WIDE_4B              SDMMC_BUS_WIDE_4B
  #define SD_BUS_WIDE_8B              SDMMC_BUS_WIDE_8B
  #define SD_HARDWARE_FLOW_CONTROL_DISABLE SDMMC_HARDWARE_FLOW_CONTROL_DISABLE
  #define SD_CLKCR_CLKDIV             SDMMC_CLKCR_CLKDIV
  #define SD_HAS_CLOCKBYPASS          0
  #define SD_PERIPH_NAME              "SDMMC"
#elif defined(SDIO)
  /* F4: SDIO peripheral, has ClockBypass member */
  #define SD_INSTANCE                 SDIO
  #define SD_CLKREG                   SDIO
  #define SD_CLOCK_EDGE_RISING        SDIO_CLOCK_EDGE_RISING
  #define SD_CLOCK_POWER_SAVE_DISABLE SDIO_CLOCK_POWER_SAVE_DISABLE
  #define SD_BUS_WIDE_1B              SDIO_BUS_WIDE_1B
  #define SD_BUS_WIDE_4B              SDIO_BUS_WIDE_4B
  #define SD_BUS_WIDE_8B              SDIO_BUS_WIDE_8B
  #define SD_HARDWARE_FLOW_CONTROL_DISABLE SDIO_HARDWARE_FLOW_CONTROL_DISABLE
  #define SD_CLKCR_CLKDIV             SDIO_CLKCR_CLKDIV
  #define SD_HAS_CLOCKBYPASS          1
  #define SD_PERIPH_NAME              "SDIO"
#else
  #error "Neither SDMMC1 nor SDIO peripheral is defined. Check STM32 HAL headers."
#endif

extern SD_HandleTypeDef hsd1;

/* --------------------------------------------------------------------------*/
/* Fixed clock dividers: read 21 MHz, write 12 MHz                          */
/* --------------------------------------------------------------------------*/
/*
 * SD_CK = ref_hz / (CLKDIV + 2).  Ref=150 MHz (H7 default):
 *   CLKDIV=5  → 150/7  ≈ 21.4 MHz (read — RX FIFO drained by CPU loads)
 *   CLKDIV=10 → 150/12 = 12.5 MHz (write — TX FIFO fills via stores,
 *               IRQ-disabled polling prevents TX_UNDERRUN at this speed)
 *
 * Read/write run at different clocks to maximise throughput on this
 * board/card combo.  Clock is switched on-the-fly in BSP_SD_ReadBlocks
 * and BSP_SD_WriteBlocks.  The card survives a clock change as long as
 * it is in Transfer state (no active CMD/DAT transaction).
 */
#define SD_READ_CLKDIV   5
#define SD_WRITE_CLKDIV  10

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
  if (hsd1.State == HAL_SD_STATE_RESET)
#endif
  {
    hsd1.Instance = SD_INSTANCE;
    hsd1.Init.ClockEdge   = SD_CLOCK_EDGE_RISING;
#if SD_HAS_CLOCKBYPASS
    hsd1.Init.ClockBypass = SDIO_CLOCK_BYPASS_DISABLE;
#endif
    hsd1.Init.ClockPowerSave = SD_CLOCK_POWER_SAVE_DISABLE;
    hsd1.Init.BusWide     = SD_BUS_WIDE_1B;
    hsd1.Init.HardwareFlowControl = SD_HARDWARE_FLOW_CONTROL_DISABLE;
    hsd1.Init.ClockDiv    = 0xFF;   /* ~400 kHz safe for all cards */

    sd_state = HAL_SD_Init(&hsd1);
    if (sd_state != MSD_OK)
    {
      return sd_state;
    }
  }

  /* --- Step 2: Read card type from CSD register --- */
  HAL_SD_GetCardInfo(&hsd1, &card_info);

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
    hsd1.Init.BusWide = SD_BUS_WIDE_1B;
    pr_info("%s: step3 - SDSC -> 1-bit (fixed)", SD_PERIPH_NAME);
  }
  else
  {
    /* SDHC/SDXC: attempt 4-bit with HAL state sanitized */
    hsd1.ErrorCode = HAL_SD_ERROR_NONE;
    hsd1.State     = HAL_SD_STATE_READY;

    if (HAL_SD_ConfigWideBusOperation(&hsd1, SD_BUS_WIDE_4B) == HAL_OK)
    {
      hsd1.Init.BusWide = SD_BUS_WIDE_4B;
      pr_info("%s: step3 - 4-bit OK", SD_PERIPH_NAME);
    }
    else
    {
      /* Force 1-bit: both card (ACMD6(0)) and controller (CLKCR) */
      SD_CLKREG->CLKCR &= ~((uint32_t)0x0000C000UL);
      hsd1.ErrorCode = HAL_SD_ERROR_NONE;
      hsd1.State     = HAL_SD_STATE_READY;
      HAL_SD_ConfigWideBusOperation(&hsd1, SD_BUS_WIDE_1B);
      hsd1.Init.BusWide = SD_BUS_WIDE_1B;
      pr_info("%s: step3 - 4-bit FAIL, forced 1-bit", SD_PERIPH_NAME);
    }
  }

  /* --- Step 4: Set fixed read clock (21 MHz via CLKDIV=5) ---
   * Write-side switches to CLKDIV=10 (12 MHz) per-op in
   * BSP_SD_WriteBlocks.  No auto-probe — fixed dividers only. */
  sd_set_clkdiv(SD_READ_CLKDIV);
  hsd1.Init.ClockDiv = SD_READ_CLKDIV;
  hsd1.ErrorCode = HAL_SD_ERROR_NONE;
  hsd1.State     = HAL_SD_STATE_READY;

  if      (hsd1.Init.BusWide == SD_BUS_WIDE_8B) bus_str = "8-bit";
  else if (hsd1.Init.BusWide == SD_BUS_WIDE_4B) bus_str = "4-bit";
  else                                              bus_str = "1-bit";
  pr_info("%s: init done - rd=%lu MHz (div=%u), wr=%lu MHz (div=%u), bus=%s",
          SD_PERIPH_NAME,
          (unsigned long)(150UL / (SD_READ_CLKDIV + 2)),
          (unsigned int)SD_READ_CLKDIV,
          (unsigned long)(150UL / (SD_WRITE_CLKDIV + 2)),
          (unsigned int)SD_WRITE_CLKDIV,
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

  /* Switch to read clock (21 MHz) before every read.
   * Card is in Transfer state at this point — safe to change CLKCR. */
  sd_set_clkdiv(SD_READ_CLKDIV);
  hsd1.ErrorCode = HAL_SD_ERROR_NONE;
  hsd1.State     = HAL_SD_STATE_READY;

  if (HAL_SD_ReadBlocks(&hsd1, (uint8_t *)pData, ReadAddr, NumOfBlocks, Timeout) != HAL_OK)
  {
    sd_state = MSD_ERROR;
    pr_err("SD read failed: LBA=%lu, cnt=%lu, State=%d, EC=0x%lx",
           (unsigned long)ReadAddr, (unsigned long)NumOfBlocks,
           (int)hsd1.State, (unsigned long)hsd1.ErrorCode);
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

  /* Switch to write clock (12 MHz) before every write.
   * Card is in Transfer state at this point — safe to change CLKCR.
   * Lower clock prevents TX_UNDERRUN in IRQ-disabled polling mode
   * (SDMMC TX FIFO fills via CPU stores, slower than RX FIFO drain). */
  sd_set_clkdiv(SD_WRITE_CLKDIV);
  hsd1.ErrorCode = HAL_SD_ERROR_NONE;
  hsd1.State     = HAL_SD_STATE_READY;

  if (HAL_SD_WriteBlocks(&hsd1, (uint8_t *)pData, WriteAddr, NumOfBlocks, Timeout) != HAL_OK)
  {
    sd_state = MSD_ERROR;
    pr_err("SD write failed: LBA=%lu, cnt=%lu, State=%d, EC=0x%lx",
           (unsigned long)WriteAddr, (unsigned long)NumOfBlocks,
           (int)hsd1.State, (unsigned long)hsd1.ErrorCode);
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

  /* Read block(s) in DMA transfer mode */
  if (HAL_SD_ReadBlocks_DMA(&hsd1, (uint8_t *)pData, ReadAddr, NumOfBlocks) != HAL_OK)
  {
    sd_state = MSD_ERROR;
  }

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

  /* Write block(s) in DMA transfer mode */
  if (HAL_SD_WriteBlocks_DMA(&hsd1, (uint8_t *)pData, WriteAddr, NumOfBlocks) != HAL_OK)
  {
    sd_state = MSD_ERROR;
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

  if (HAL_SD_Erase(&hsd1, StartAddr, EndAddr) != HAL_OK)
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
  return ((HAL_SD_GetCardState(&hsd1) == HAL_SD_CARD_TRANSFER ) ? SD_TRANSFER_OK : SD_TRANSFER_BUSY);
}

/**
  * @brief  Get SD information about specific SD card.
  * @param  CardInfo: Pointer to HAL_SD_CardInfoTypedef structure
  * @retval None
  */
__weak void BSP_SD_GetCardInfo(HAL_SD_CardInfoTypeDef *CardInfo)
{
  /* Get SD card Information */
  HAL_SD_GetCardInfo(&hsd1, CardInfo);
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
void HAL_SD_TxCpltCallback(SD_HandleTypeDef *hsd)
{
  BSP_SD_WriteCpltCallback();
}

/**
  * @brief Rx Transfer completed callback
  * @param hsd: SD handle
  * @retval None
  */
void HAL_SD_RxCpltCallback(SD_HandleTypeDef *hsd)
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
