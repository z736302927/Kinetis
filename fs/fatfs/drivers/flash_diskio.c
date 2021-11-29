/**
  ******************************************************************************
  * @file    flash_diskio.c
  * @author  MCD Application Team
  * @version V1.4.1
  * @date    14-February-2017
  * @brief   Flash Disk I/O driver
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; COPYRIGHT 2017 STMicroelectronics</center></h2>
  *
  * Redistribution and use in source and binary forms, with or without
  * modification, are permitted, provided that the following conditions are met:
  *
  * 1. Redistribution of source code must retain the above copyright notice,
  *    this list of conditions and the following disclaimer.
  * 2. Redistributions in binary form must reproduce the above copyright notice,
  *    this list of conditions and the following disclaimer in the documentation
  *    and/or other materials provided with the distribution.
  * 3. Neither the name of STMicroelectronics nor the names of other
  *    contributors to this software may be used to endorse or promote products
  *    derived from this software without specific written permission.
  * 4. This software, including modifications and/or derivative works of this
  *    software, must execute solely and exclusively on microcontroller or
  *    microprocessor devices manufactured by or for STMicroelectronics.
  * 5. Redistribution and use of this software other than as permitted under
  *    this license is void and will automatically terminate your rights under
  *    this license.
  *
  * THIS SOFTWARE IS PROVIDED BY STMICROELECTRONICS AND CONTRIBUTORS "AS IS"
  * AND ANY EXPRESS, IMPLIED OR STATUTORY WARRANTIES, INCLUDING, BUT NOT
  * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
  * PARTICULAR PURPOSE AND NON-INFRINGEMENT OF THIRD PARTY INTELLECTUAL PROPERTY
  * RIGHTS ARE DISCLAIMED TO THE FULLEST EXTENT PERMITTED BY LAW. IN NO EVENT
  * SHALL STMICROELECTRONICS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
  * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
  * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA,
  * OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
  * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
  * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
  * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/

#include <generated/deconfig.h>
#include <linux/spi/w25qxxx.h>
#include <linux/string.h>
#include <linux/mtd/mtd.h>

#include "../ff_gen_drv.h"
#include "kinetis-core.h"

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
#define STORAGE_SEC_NBR                  1024 * 2 * 32
#define STORAGE_SEC_SIZ                  512
#define STORAGE_SEC_SIZ_POWER            9
#define STORAGE_BLK_SIZ                  8      /* unit is the number of sector in an block. */

/* Private variables ---------------------------------------------------------*/
/* Disk status */
static volatile DSTATUS disk_stat = STA_NOINIT;

/* Private function prototypes -----------------------------------------------*/
DSTATUS flash_disk_initialize(BYTE);
DSTATUS flash_disk_status(BYTE);
DRESULT flash_disk_read(BYTE, BYTE *, DWORD, UINT);
#if _USE_WRITE == 1
DRESULT flash_disk_write(BYTE, const BYTE *, DWORD, UINT);
#endif /* _USE_WRITE == 1 */
#if _USE_IOCTL == 1
DRESULT flash_disk_ioctl(BYTE, BYTE, void *);
#endif /* _USE_IOCTL == 1 */

const Diskio_drvTypeDef flash_disk_driver = {
	flash_disk_initialize,
	flash_disk_status,
	flash_disk_read,
#if  _USE_WRITE == 1
	flash_disk_write,
#endif /* _USE_WRITE == 1 */
#if  _USE_IOCTL == 1
	flash_disk_ioctl,
#endif /* _USE_IOCTL == 1 */
};

/* Private functions ---------------------------------------------------------*/

/**
  * @brief  Initializes a Drive
  * @param  lun : not used
  * @retval DSTATUS: Operation status
  */
DSTATUS flash_disk_initialize(BYTE lun)
{
	disk_stat = STA_NOINIT;

	/* Configure the FLASH device */
	disk_stat = STA_NOINIT;

	switch (lun) {
	case 0:
//            w25qxxx_init(W25Q128);
		disk_stat = disk_status(0);
		break;

	case 1:
//            w25qxxx_init(W25Q256);
		disk_stat = disk_status(1);
		break;

	default:
		disk_stat = STA_NOINIT;
		break;
	}

	return disk_stat;
}

/**
  * @brief  Gets Disk status
  * @param  lun : not used
  * @retval DSTATUS: Operation status
  */
DSTATUS flash_disk_status(BYTE lun)
{
	disk_stat = STA_NOINIT;

	switch (lun) {
	case 0:
//            if (w25qxxx_release_device_id(W25Q128) != 0)
		disk_stat &= ~STA_NOINIT;

		break;

	case 1:
//            if (w25qxxx_release_device_id(W25Q256) != 0)
		disk_stat &= ~STA_NOINIT;

		break;

	default:
		disk_stat = STA_NOINIT;
		break;
	}

	return disk_stat;
}

/**
  * @brief  Reads Sector(s)
  * @param  lun : not used
  * @param  *buff: Data buffer to store read data
  * @param  sector: Sector address (LBA)
  * @param  count: Number of sectors to read (1..128)
  * @retval DRESULT: Operation result
  */
DRESULT flash_disk_read(BYTE lun, BYTE *buff, DWORD sector, UINT count)
{
	struct kineits_system *kineits = lib_get_stm32_val();
	size_t retlen;

	switch (lun) {
	case 0:
		mtd_write(&kineits->nor->mtd,
			sector << STORAGE_SEC_SIZ_POWER,
			count << STORAGE_SEC_SIZ_POWER,
			&retlen,
			buff);
		break;

	case 1:
		mtd_write(&kineits->nor->mtd,
			sector << STORAGE_SEC_SIZ_POWER,
			count << STORAGE_SEC_SIZ_POWER,
			&retlen,
			buff);
		break;

	default:
		break;
	}

	return RES_OK;
}

/**
  * @brief  Writes Sector(s)
  * @param  lun : not used
  * @param  *buff: Data to be written
  * @param  sector: Sector address (LBA)
  * @param  count: Number of sectors to write (1..128)
  * @retval DRESULT: Operation result
  */
#if _USE_WRITE == 1
DRESULT flash_disk_write(BYTE lun, const BYTE *buff, DWORD sector, UINT count)
{
	struct kineits_system *kineits = lib_get_stm32_val();
	size_t retlen;

	switch (lun) {
	case 0:
		mtd_write(&kineits->nor->mtd,
			sector << STORAGE_SEC_SIZ_POWER,
			count << STORAGE_SEC_SIZ_POWER,
			&retlen,
			buff);
		break;

	case 1:
		mtd_write(&kineits->nor->mtd,
			sector << STORAGE_SEC_SIZ_POWER,
			count << STORAGE_SEC_SIZ_POWER,
			&retlen,
			buff);
		break;

	default:
		break;
	}

	return RES_OK;
}
#endif /* _USE_WRITE == 1 */

/**
  * @brief  I/O control operation
  * @param  lun : not used
  * @param  cmd: Control code
  * @param  *buff: Buffer to send/receive control data
  * @retval DRESULT: Operation result
  */
#if _USE_IOCTL == 1
DRESULT flash_disk_ioctl(BYTE lun, BYTE cmd, void *buff)
{
	DRESULT res = RES_ERROR;

	if (disk_stat & STA_NOINIT)
		return RES_NOTRDY;

	switch (lun) {
	case 0:
		switch (cmd) {
		/* Make sure that no pending write process */
		case CTRL_SYNC :
			res = RES_OK;
			break;

		/* Get number of sectors on the disk (DWORD) */
		case GET_SECTOR_COUNT :
			*(DWORD *)buff = STORAGE_SEC_NBR / 2;
			res = RES_OK;
			break;

		/* Get R/W sector size (WORD) */
		case GET_SECTOR_SIZE :
			*(WORD *)buff = STORAGE_SEC_SIZ;
			res = RES_OK;
			break;

		/* Get erase block size in unit of sector (DWORD) */
		case GET_BLOCK_SIZE :
			*(DWORD *)buff = STORAGE_BLK_SIZ;
			res = RES_OK;
			break;

		default:
			res = RES_PARERR;
			break;
		}

		break;

	case 1:
		switch (cmd) {
		/* Make sure that no pending write process */
		case CTRL_SYNC :
			res = RES_OK;
			break;

		/* Get number of sectors on the disk (DWORD) */
		case GET_SECTOR_COUNT :
			*(DWORD *)buff = STORAGE_SEC_NBR;
			res = RES_OK;
			break;

		/* Get R/W sector size (WORD) */
		case GET_SECTOR_SIZE :
			*(WORD *)buff = STORAGE_SEC_SIZ;
			res = RES_OK;
			break;

		/* Get erase block size in unit of sector (DWORD) */
		case GET_BLOCK_SIZE :
			*(DWORD *)buff = STORAGE_BLK_SIZ;
			res = RES_OK;
			break;

		default:
			res = RES_PARERR;
			break;
		}

		break;
	}

	return res;
}
#endif /* _USE_IOCTL == 1 */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/

