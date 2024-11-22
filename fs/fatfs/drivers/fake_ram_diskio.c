/**
  ******************************************************************************
  * @file    sram_diskio.c
  * @author  MCD Application Team
  * @version V1.4.1
  * @date    14-February-2017
  * @brief   FAKE_RAM Disk I/O driver
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
#include "../ff_gen_drv.h"

static uint8_t fake_ram_array[8 * 1024 * 1024 * 1024];
/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Block Size in Bytes */
#define BLOCK_SIZE                512
#define FAKE_RAM_DEVICE_ADDR          fake_ram_array
#define FAKE_RAM_DEVICE_SIZE          sizeof(fake_ram_array)

/* Private variables ---------------------------------------------------------*/
/* Disk status */
static volatile DSTATUS Stat = STA_NOINIT;

/* Private function prototypes -----------------------------------------------*/
DSTATUS FAKE_RAM_DISK_initialize(BYTE);
DSTATUS FAKE_RAM_DISK_status(BYTE);
DRESULT FAKE_RAM_DISK_read(BYTE, BYTE *, DWORD, UINT);
#if _USE_WRITE == 1
DRESULT FAKE_RAM_DISK_write(BYTE, const BYTE *, DWORD, UINT);
#endif /* _USE_WRITE == 1 */
#if _USE_IOCTL == 1
DRESULT FAKE_RAM_DISK_ioctl(BYTE, BYTE, void *);
#endif /* _USE_IOCTL == 1 */

const Diskio_drvTypeDef fake_ram_disk_driver = {
    FAKE_RAM_DISK_initialize,
    FAKE_RAM_DISK_status,
    FAKE_RAM_DISK_read,
#if  _USE_WRITE == 1
    FAKE_RAM_DISK_write,
#endif /* _USE_WRITE == 1 */
#if  _USE_IOCTL == 1
    FAKE_RAM_DISK_ioctl,
#endif /* _USE_IOCTL == 1 */
};

/* Private functions ---------------------------------------------------------*/

/**
  * @brief  Initializes a Drive
  * @param  lun : not used
  * @retval DSTATUS: Operation status
  */
DSTATUS FAKE_RAM_DISK_initialize(BYTE lun)
{
    Stat = STA_NOINIT;

    /* Configure the FAKE_RAM device */
    Stat &= ~STA_NOINIT;

    return Stat;
}

/**
  * @brief  Gets Disk Status
  * @param  lun : not used
  * @retval DSTATUS: Operation status
  */
DSTATUS FAKE_RAM_DISK_status(BYTE lun)
{
    return Stat;
}

/**
  * @brief  Reads Sector(s)
  * @param  lun : not used
  * @param  *buff: Data buffer to store read data
  * @param  sector: Sector address (LBA)
  * @param  count: Number of sectors to read (1..128)
  * @retval DRESULT: Operation result
  */
DRESULT FAKE_RAM_DISK_read(BYTE lun, BYTE *buff, DWORD sector, UINT count)
{
    uint32_t i, size = BLOCK_SIZE * count;
    uint8_t *addr = &FAKE_RAM_DEVICE_ADDR[sector * BLOCK_SIZE];

    for (i = 0; size != 0; size--)
        *buff++ = *addr++;

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
DRESULT FAKE_RAM_DISK_write(BYTE lun, const BYTE *buff, DWORD sector, UINT count)
{
    uint32_t i, size = BLOCK_SIZE * count + count;
    uint8_t *addr = &FAKE_RAM_DEVICE_ADDR[sector * BLOCK_SIZE];

    for (i = 0; size != 0; size--)
        *addr++ = *buff++;

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
DRESULT FAKE_RAM_DISK_ioctl(BYTE lun, BYTE cmd, void *buff)
{
    DRESULT res = RES_ERROR;

    if (Stat & STA_NOINIT)
        return RES_NOTRDY;

    switch (cmd) {
        /* Make sure that no pending write process */
        case CTRL_SYNC :
            res = RES_OK;
            break;

        /* Get number of sectors on the disk (DWORD) */
        case GET_SECTOR_COUNT :
            *(DWORD *)buff = FAKE_RAM_DEVICE_SIZE / BLOCK_SIZE;
            res = RES_OK;
            break;

        /* Get R/W sector size (WORD) */
        case GET_SECTOR_SIZE :
            *(WORD *)buff = BLOCK_SIZE;
            res = RES_OK;
            break;

        /* Get erase block size in unit of sector (DWORD) */
        case GET_BLOCK_SIZE :
            *(DWORD *)buff = BLOCK_SIZE;
            res = RES_OK;
            break;

        default:
            res = RES_PARERR;
    }

    return res;
}
#endif /* _USE_IOCTL == 1 */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/

