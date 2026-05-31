/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    sd_diskio.c
  * @brief   SD Disk I/O driver
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

/* Note: code generation based on sd_diskio_template_bspv1.c v2.1.4.
 *
 * Polling mode for both read and write.  SDMMC IDMA cannot access DTCM,
 * but the CPU can read/write the SDMMC FIFO directly via load/store.
 *
 * Both paths are IRQ-protected (__disable_irq/__enable_irq):
 *   Read:  21 MHz — RX FIFO overflow (RXOVERR 0x20) if CPU stolen.
 *   Write: 12 MHz — TX FIFO underrun (TX_UNDERRUN 0x10) if CPU stolen. */

/* USER CODE BEGIN firstSection */
/* can be used to modify / undefine following code or add new definitions */
/* USER CODE END firstSection*/

/* Includes ------------------------------------------------------------------*/
#include <linux/printk.h>

#include "../ff_gen_drv.h"
#include "sd_diskio.h"

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* use the default SD timout as defined in the platform BSP driver*/
#if defined(SDMMC_DATATIMEOUT)
#define SD_TIMEOUT SDMMC_DATATIMEOUT
#elif defined(SD_DATATIMEOUT)
#define SD_TIMEOUT SD_DATATIMEOUT
#else
#define SD_TIMEOUT 30 * 1000
#endif

#define SD_DEFAULT_BLOCK_SIZE 512

/* Card-ready poll timeout (counter-based, ~4-5 cycles/iter @ 480 MHz).
 * Read:  ~2M iterations  ≈  16 ms
 * Write: ~30M iterations  ≈ 250 ms (card internal programming) */
#define SD_RDY_TMO_RD     0x200000UL
#define SD_RDY_TMO_WR     0x1E00000UL

/*
 * Depending on the use case, the SD card initialization could be done at the
 * application level: if it is the case define the flag below to disable
 * the BSP_SD_Init() call in the SD_Initialize() and add a call to
 * BSP_SD_Init() elsewhere in the application.
 */
/* USER CODE BEGIN disableSDInit */
/* #define DISABLE_SD_INIT */
/* USER CODE END disableSDInit */

/* Private variables ---------------------------------------------------------*/
/* Disk status */
static volatile DSTATUS Stat = STA_NOINIT;

/* Private function prototypes -----------------------------------------------*/
static DSTATUS SD_CheckStatus(BYTE lun);
DSTATUS SD_initialize(BYTE);
DSTATUS SD_status(BYTE);
DRESULT SD_read(BYTE, BYTE*, LBA_t, UINT);
#if _USE_WRITE == 1
DRESULT SD_write(BYTE, const BYTE*, LBA_t, UINT);
#endif /* _USE_WRITE == 1 */
#if _USE_IOCTL == 1
DRESULT SD_ioctl(BYTE, BYTE, void*);
#endif  /* _USE_IOCTL == 1 */

const Diskio_drvTypeDef  SD_Driver = {
	SD_initialize,
	SD_status,
	SD_read,
#if  _USE_WRITE == 1
	SD_write,
#endif /* _USE_WRITE == 1 */

#if  _USE_IOCTL == 1
	SD_ioctl,
#endif /* _USE_IOCTL == 1 */
};

/* USER CODE BEGIN beforeFunctionSection */
/* can be used to modify / undefine following code or add new code */
/* USER CODE END beforeFunctionSection */

/* Private functions ---------------------------------------------------------*/

static DSTATUS SD_CheckStatus(BYTE lun)
{
	Stat = STA_NOINIT;

	if (BSP_SD_GetCardState() == MSD_OK) {
		Stat &= ~STA_NOINIT;
	} else {
		pr_warn("SD: CheckStatus: card not ready (state != MSD_OK)");
	}

	return Stat;
}

/**
  * @brief  Initializes a Drive
  * @param  lun : not used
  * @retval DSTATUS: Operation status
  */
DSTATUS SD_initialize(BYTE lun)
{
	Stat = STA_NOINIT;

#if !defined(DISABLE_SD_INIT)

	if (BSP_SD_Init() == MSD_OK) {
		Stat = SD_CheckStatus(lun);
	} else {
		pr_err("SD: BSP_SD_Init FAILED");
	}

#else
	Stat = SD_CheckStatus(lun);
#endif

	return Stat;
}

/**
  * @brief  Gets Disk Status
  * @param  lun : not used
  * @retval DSTATUS: Operation status
  */
DSTATUS SD_status(BYTE lun)
{
	return SD_CheckStatus(lun);
}

/* USER CODE BEGIN beforeReadSection */
/* can be used to modify previous code / undefine following code / add new code */
/* USER CODE END beforeReadSection */
/**
  * @brief  Reads Sector(s)
  * @param  lun : not used
  * @param  *buff: Data buffer to store read data
  * @param  sector: Sector address (LBA)
  * @param  count: Number of sectors to read (1..128)
  * @retval DRESULT: Operation result
  */

DRESULT SD_read(BYTE lun, BYTE *buff, LBA_t sector, UINT count)
{
	DRESULT res = RES_ERROR;
	uint32_t tmo;

	/* Polling mode: CPU reads SDMMC FIFO directly into DTCM.
	 * SDMMC IDMA cannot access DTCM on STM32H7, but CPU load/store
	 * instructions (via AHB bus) can reach both SDMMC registers (APB2)
	 * and DTCM (Cortex-M7 TCM interface) simultaneously.
	 * BSP_SD_ReadBlocks switches to 21 MHz read clock internally.
	 *
	 * IRQ-disabled: any DMA/UART/Timer IRQ can steal CPU cycles and
	 * cause RX FIFO overflow (EC=0x20, SDMMC_RXOVERR).  The same
	 * approach as SD_write: bracket the transfer with IRQ off. */
	__disable_irq();

	if (BSP_SD_ReadBlocks((uint32_t *)buff,
		(uint32_t)(sector),
		count, SD_TIMEOUT) == MSD_OK) {
		/* wait until the read operation is finished */
		tmo = SD_RDY_TMO_RD;

		while (BSP_SD_GetCardState() != MSD_OK && --tmo) {
		}
		if (tmo == 0) {
			pr_err("SD read: card-ready timeout after block transfer, "
			       "sector=%lu\r\n", sector);
			__enable_irq();
			return RES_ERROR;
		}
		res = RES_OK;
	}

	__enable_irq();
	return res;
}

/* USER CODE BEGIN beforeWriteSection */
/* can be used to modify previous code / undefine following code / add new code */
/* USER CODE END beforeWriteSection */
/**
  * @brief  Writes Sector(s)
  * @param  lun : not used
  * @param  *buff: Data to be written
  * @param  sector: Sector address (LBA)
  * @param  count: Number of sectors to write (1..128)
  * @retval DRESULT: Operation result
  */
#if _USE_WRITE == 1

DRESULT SD_write(BYTE lun, const BYTE *buff, LBA_t sector, UINT count)
{
	DRESULT res = RES_ERROR;
	uint32_t tmo;

	/* IRQ-disabled polling write.  BSP_SD_WriteBlocks switches to
	 * 12 MHz write clock internally to prevent TX_UNDERRUN.  Any IRQ
	 * (UART DMA, timers) can still delay TXFIFOHE handling, so we
	 * disable IRQ for the duration of the transfer. */
	__disable_irq();

	if (BSP_SD_WriteBlocks((uint32_t *)buff,
		(uint32_t)(sector),
		count, SD_TIMEOUT) != MSD_OK) {
		__enable_irq();
		return RES_ERROR;
	}

	/* Wait in IRQ-disabled region — card state poll sends CMD over
	 * CMD line via CPU, no SDMMC interrupt needed. */
	tmo = SD_RDY_TMO_WR;
	while (BSP_SD_GetCardState() != MSD_OK && --tmo) {
	}
	if (tmo == 0) {
		pr_err("SD write: card-ready timeout after block transfer, "
		       "sector=%lu, count=%u\r\n", sector, (unsigned int)count);
		__enable_irq();
		return RES_ERROR;
	}

	res = RES_OK;
	__enable_irq();

	return res;
}
#endif /* _USE_WRITE == 1 */

/* USER CODE BEGIN beforeIoctlSection */
/* can be used to modify previous code / undefine following code / add new code */
/* USER CODE END beforeIoctlSection */
/**
  * @brief  I/O control operation
  * @param  lun : not used
  * @param  cmd: Control code
  * @param  *buff: Buffer to send/receive control data
  * @retval DRESULT: Operation result
  */
#if _USE_IOCTL == 1
DRESULT SD_ioctl(BYTE lun, BYTE cmd, void *buff)
{
	DRESULT res = RES_ERROR;
	BSP_SD_CardInfo CardInfo;

	if (Stat & STA_NOINIT) {
		return RES_NOTRDY;
	}

	switch (cmd) {
	/* Make sure that no pending write process */
	case CTRL_SYNC :
		res = RES_OK;
		break;

	/* Get number of sectors on the disk (DWORD) */
	case GET_SECTOR_COUNT :
		BSP_SD_GetCardInfo(&CardInfo);
		*(DWORD*)buff = CardInfo.LogBlockNbr;
		res = RES_OK;
		break;

	/* Get R/W sector size (WORD) */
	case GET_SECTOR_SIZE :
		BSP_SD_GetCardInfo(&CardInfo);
		*(WORD*)buff = CardInfo.LogBlockSize;
		res = RES_OK;
		break;

	/* Get erase block size in unit of sector (DWORD) */
	case GET_BLOCK_SIZE :
		BSP_SD_GetCardInfo(&CardInfo);
		*(DWORD*)buff = CardInfo.LogBlockSize / SD_DEFAULT_BLOCK_SIZE;
		res = RES_OK;
		break;

	default:
		res = RES_PARERR;
	}

	return res;
}
#endif /* _USE_IOCTL == 1 */

/* USER CODE BEGIN afterIoctlSection */
/* can be used to modify previous code / undefine following code / add new code */
/* USER CODE END afterIoctlSection */

/* USER CODE BEGIN lastSection */
/* can be used to modify / undefine previous code or add new code */
/* USER CODE END lastSection */
