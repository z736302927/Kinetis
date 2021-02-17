#include "kinetis/w25qxxx.h"
#include "kinetis/random-gene.h"
#include "kinetis/idebug.h"
#include "kinetis/delay.h"
#include "kinetis/basic-timer.h"

#include <linux/bitops.h>
#include <linux/iopoll.h>
#include <linux/errno.h>
#include <linux/printk.h>

#include "stdlib.h"
#include "string.h"

#include "spi.h"

/* The following program is modified by the user according to the hardware device, otherwise the driver cannot run. */

/**
  * @step 1:  Modify the corresponding function according to the modified area and the corresponding function name.
  * @step 2:  Modify four areas: GPIO_PORT/GPIO_PIN/Led_TypeDef/LEDn.
  * @step 3:  .
  * @step 4:  .
  * @step 5:
  */


static inline void w25qxxx_port_transmmit(u8 w25qxxx, u8 tmp)
{
    switch (w25qxxx) {
        case W25Q128:
            HAL_SPI_Transmit(&hspi1, &tmp, 1, 1000);
            break;

        case W25Q256:
            HAL_SPI_Transmit(&hspi5, &tmp, 1, 1000);
            break;

        default:
            break;
    }
}

static inline u8 w25qxxx_port_receive(u8 w25qxxx)
{
    u8 tmp = 0;

    switch (w25qxxx) {
        case W25Q128:
            HAL_SPI_Receive(&hspi1, &tmp, 1, 1000);
            break;

        case W25Q256:
            HAL_SPI_Receive(&hspi5, &tmp, 1, 1000);
            break;

        default:
            break;
    }

    return tmp;
}

static inline void w25qxxx_port_multi_transmmit(u8 w25qxxx,
    u8 *pdata, u32 length)
{
    switch (w25qxxx) {
        case W25Q128:
            HAL_SPI_Transmit(&hspi1, pdata, length, 1000);
            break;

        case W25Q256:
            HAL_SPI_Transmit(&hspi5, pdata, length, 1000);
            break;

        default:
            break;
    }
}

static inline void w25qxxx_port_multi_receive(u8 w25qxxx,
    u8 *pdata, u32 length)
{
    switch (w25qxxx) {
        case W25Q128:
            HAL_SPI_Receive(&hspi1, pdata, length, 1000);
            break;

        case W25Q256:
            HAL_SPI_Receive(&hspi5, pdata, length, 1000);
            break;

        default:
            break;
    }
}

static inline void w25qxxx_cs_low(u8 w25qxxx)
{
    switch (w25qxxx) {
        case W25Q128:
            HAL_GPIO_WritePin(GPIOB, GPIO_PIN_14, GPIO_PIN_RESET);
            break;

        case W25Q256:
            HAL_GPIO_WritePin(GPIOF, GPIO_PIN_6, GPIO_PIN_RESET);
            break;

        default:
            break;
    }
}

static inline void w25qxxx_cs_high(u8 w25qxxx)
{
    switch (w25qxxx) {
        case W25Q128:
            HAL_GPIO_WritePin(GPIOB, GPIO_PIN_14, GPIO_PIN_SET);
            break;

        case W25Q256:
            HAL_GPIO_WritePin(GPIOF, GPIO_PIN_6, GPIO_PIN_SET);
            break;

        default:
            break;
    }
}

void w25qxxx_HardReset(u8 w25qxxx)
{

}

/* The above procedure is modified by the user according to the hardware device, otherwise the driver cannot run. */

#define _PAGE_SIZE                       256
#define SECTOR_SIZE                     4096
#define DUMMY_BYTE                      0xFF
#define WRITE_ENABLE                    0x06
#define WRITE_ENABLE_FORVOLATILE        0x50
#define WRITE_DISABLE                   0x04
#define READ_STATUS_REGISTER1           0x05
#define READ_STATUS_REGISTER2           0x35
#define READ_STATUS_REGISTER3           0x15
#define WRITE_STATUS_REGISTER1          0x01
#define WRITE_STATUS_REGISTER2          0x31
#define WRITE_STATUS_REGISTER3          0x11
#define READ_EXTENDED_ADDRESS_REGISTER  0xC8
#define WRITE_EXTENDED_ADDRESS_REGISTER 0xC5
#define ENTER_4BYTE_ADDRESS_MODE        0xB7
#define EXIT_4BYTE_ADDRESS_MODE         0xE9
#define READ_DATA                       0x03
#define READ_DATA_WITH_4Byte_ADDRESS    0x13
#define FAST_READ                       0x0B
#define FAST_READ_WITH_4Byte_ADDRESS    0x0C
#define PAGE_PROGRAM                    0x02
#define SECTOR_ERASE                    0x20
#define BLOCK_ERASE_32KB                0x52
#define BLOCK_ERASE_64KB                0xD8
#define CHIP_ERASE                      0xC7
#define ERASE_PROGRAM_SUSPEND           0x75
#define ERASE_PROGRAM_RESUME            0x7A
#define POWER_DOWN                      0xB9
#define RELEASE_POWERDOWN_DEVICE_ID     0xAB
#define READ_MANUFACTURER_DEVICE_ID     0x90
#define READ_UNIQUE_ID_NUMBER           0x4B
#define READ_JEDEC_ID                   0x9F
#define READ_SFDP_REGISTER              0x5A
#define SECURITY_REGISTERS_1            0x10
#define SECURITY_REGISTERS_2            0x20
#define SECURITY_REGISTERS_3            0x30
#define ERASE_SECURITY_REGISTERS        0x44
#define PROGRAM_SECURITY_REGISTERS      0x42
#define READ_SECURITY_REGISTERS         0x48
#define ENTER_QPI_MODE                  0x38
#define EXIT_QPI_MODE                   0xFF
#define INDIVIDUAL_BLOCK_SECTOR_LOCK    0x36
#define INDIVIDUAL_BLOCK_SECTOR_UNLOCK  0x39
#define READ_BLOCK_SECTOR_LOCK          0x3D
#define GLOBAL_BLOCK_SECTOR_LOCK        0x7E
#define GLOBAL_BLOCK_SECTOR_UNLOCK      0x98
#define ENABLE_RESET                    0x66
#define RESET_DEVICE                    0x99

struct w25qxxx_status {
    unsigned BUSY: 1;
    unsigned WEL: 1;
    unsigned BP0: 1;
    unsigned BP1: 1;
    unsigned BP2: 1;
    unsigned TB: 1;
    unsigned SEC: 1;
    unsigned SRP0: 1;

    unsigned SRP1: 1;
    unsigned QE: 1;
    unsigned LB1: 1;
    unsigned LB2: 1;
    unsigned LB3: 1;
    unsigned CMP: 1;
    unsigned SUS: 1;

    unsigned WPS: 1;
    unsigned DRV0: 1;
    unsigned DRV1: 1;
    unsigned HOLD_RST: 1;
};

static u32 w25q128_max_addr = 0xFFFFFF;
static u32 w25q256_max_addr = 0x1FFFFFF;

void w25qxxx_transmmit_cmd(u8 w25qxxx, u8 cmd)
{
    w25qxxx_cs_low(w25qxxx);
    w25qxxx_port_transmmit(w25qxxx, cmd);
    w25qxxx_cs_high(w25qxxx);
}

u8 w25qxxx_read_busy(u8 w25qxxx)
{
    u8 tmp = 0;

    tmp = w25qxxx_read_status_reg(w25qxxx, READ_STATUS_REGISTER1);

    return test_bit(0, (unsigned long *)&tmp);
}

u8 w25qxxx_read_wel(u8 w25qxxx)
{
    u8 tmp = 0;

    tmp = w25qxxx_read_status_reg(w25qxxx, READ_STATUS_REGISTER1);

    return test_bit(1, (unsigned long *)&tmp);
}

u8 w25qxxx_read_bp0(u8 w25qxxx)
{
    u8 tmp = 0;

    tmp = w25qxxx_read_status_reg(w25qxxx, READ_STATUS_REGISTER1);

    return test_bit(2, (unsigned long *)&tmp);
}

u8 w25qxxx_read_bp1(u8 w25qxxx)
{
    u8 tmp = 0;

    tmp = w25qxxx_read_status_reg(w25qxxx, READ_STATUS_REGISTER1);

    return test_bit(4, (unsigned long *)&tmp);
}

u8 w25qxxx_read_bp2(u8 w25qxxx)
{
    u8 tmp = 0;

    tmp = w25qxxx_read_status_reg(w25qxxx, READ_STATUS_REGISTER1);

    return test_bit(5, (unsigned long *)&tmp);
}

u8 w25qxxx_read_sec(u8 w25qxxx)
{
    u8 tmp = 0;

    tmp = w25qxxx_read_status_reg(w25qxxx, READ_STATUS_REGISTER1);

    return test_bit(6, (unsigned long *)&tmp);
}

u8 w25qxxx_read_bp3(u8 w25qxxx)
{
    u8 tmp = 0;

    tmp = w25qxxx_read_status_reg(w25qxxx, READ_STATUS_REGISTER1);

    return test_bit(6, (unsigned long *)&tmp);
}

u8 w25qxxx_read_tb(u8 w25qxxx)
{
    u8 tmp = 0;

    tmp = w25qxxx_read_status_reg(w25qxxx, READ_STATUS_REGISTER1);

    return test_bit(7, (unsigned long *)&tmp);
}

u8 w25qxxx_read_srp0(u8 w25qxxx)
{
    u8 tmp = 0;

    tmp = w25qxxx_read_status_reg(w25qxxx, READ_STATUS_REGISTER1);

    return test_bit(7, (unsigned long *)&tmp);
}

u8 w25qxxx_read_srp1(u8 w25qxxx)
{
    u8 tmp = 0;

    tmp = w25qxxx_read_status_reg(w25qxxx, READ_STATUS_REGISTER2);

    return test_bit(0, (unsigned long *)&tmp);
}

u8 w25qxxx_read_qe(u8 w25qxxx)
{
    u8 tmp = 0;

    tmp = w25qxxx_read_status_reg(w25qxxx, READ_STATUS_REGISTER2);

    return test_bit(1, (unsigned long *)&tmp);
}

u8 w25qxxx_read_lb1(u8 w25qxxx)
{
    u8 tmp = 0;

    tmp = w25qxxx_read_status_reg(w25qxxx, READ_STATUS_REGISTER2);

    return test_bit(3, (unsigned long *)&tmp);
}

u8 w25qxxx_read_lb2(u8 w25qxxx)
{
    u8 tmp = 0;

    tmp = w25qxxx_read_status_reg(w25qxxx, READ_STATUS_REGISTER2);

    return test_bit(4, (unsigned long *)&tmp);
}

u8 w25qxxx_read_lb3(u8 w25qxxx)
{
    u8 tmp = 0;

    tmp = w25qxxx_read_status_reg(w25qxxx, READ_STATUS_REGISTER2);

    return test_bit(5, (unsigned long *)&tmp);
}

u8 w25qxxx_read_cmp(u8 w25qxxx)
{
    u8 tmp = 0;

    tmp = w25qxxx_read_status_reg(w25qxxx, READ_STATUS_REGISTER2);

    return test_bit(6, (unsigned long *)&tmp);
}

u8 w25qxxx_read_sus(u8 w25qxxx)
{
    u8 tmp = 0;

    tmp = w25qxxx_read_status_reg(w25qxxx, READ_STATUS_REGISTER2);

    return test_bit(7, (unsigned long *)&tmp);
}

u8 w25qxxx_read_ads(u8 w25qxxx)
{
    u8 tmp = 0;

    tmp = w25qxxx_read_status_reg(w25qxxx, READ_STATUS_REGISTER3);

    return test_bit(0, (unsigned long *)&tmp);
}

u8 w25qxxx_read_adp(u8 w25qxxx)
{
    u8 tmp = 0;

    tmp = w25qxxx_read_status_reg(w25qxxx, READ_STATUS_REGISTER3);

    return test_bit(1, (unsigned long *)&tmp);
}

u8 w25qxxx_read_wps(u8 w25qxxx)
{
    u8 tmp = 0;

    tmp = w25qxxx_read_status_reg(w25qxxx, READ_STATUS_REGISTER3);

    return test_bit(2, (unsigned long *)&tmp);
}

u8 w25qxxx_read_drv0(u8 w25qxxx)
{
    u8 tmp = 0;

    tmp = w25qxxx_read_status_reg(w25qxxx, READ_STATUS_REGISTER3);

    return test_bit(5, (unsigned long *)&tmp);
}

u8 w25qxxx_read_drv1(u8 w25qxxx)
{
    u8 tmp = 0;

    tmp = w25qxxx_read_status_reg(w25qxxx, READ_STATUS_REGISTER3);

    return test_bit(6, (unsigned long *)&tmp);
}

u8 w25qxxx_read_hold_rst(u8 w25qxxx)
{
    u8 tmp = 0;

    tmp = w25qxxx_read_status_reg(w25qxxx, READ_STATUS_REGISTER3);

    return test_bit(7, (unsigned long *)&tmp);
}

void w25qxxx_write_srp0(u8 w25qxxx, u8 tmp)
{
    u8 reg = 0;

    reg = w25qxxx_read_status_reg(w25qxxx, READ_STATUS_REGISTER1);

    __assign_bit(7, (unsigned long *)&reg, tmp);

    w25qxxx_transmmit_cmd(w25qxxx, WRITE_ENABLE);
    w25qxxx_write_status_reg(w25qxxx, WRITE_STATUS_REGISTER1, reg);
}

void w25qxxx_write_sec(u8 w25qxxx, u8 tmp)
{
    u8 reg = 0;

    reg = w25qxxx_read_status_reg(w25qxxx, READ_STATUS_REGISTER1);

    __assign_bit(6, (unsigned long *)&reg, tmp);

    w25qxxx_transmmit_cmd(w25qxxx, WRITE_ENABLE);
    w25qxxx_write_status_reg(w25qxxx, WRITE_STATUS_REGISTER1, reg);
}

void w25qxxx_write_tb(u8 w25qxxx, u8 tmp)
{
    u8 reg = 0;

    reg = w25qxxx_read_status_reg(w25qxxx, READ_STATUS_REGISTER1);

    __assign_bit(5, (unsigned long *)&reg, tmp);

    w25qxxx_transmmit_cmd(w25qxxx, WRITE_ENABLE);
    w25qxxx_write_status_reg(w25qxxx, WRITE_STATUS_REGISTER1, reg);
}

void w25qxxx_write_bp(u8 w25qxxx, u8 tmp)
{
  u8 reg = 0;

  reg = w25qxxx_read_status_reg(w25qxxx, READ_STATUS_REGISTER1);

  __assign_bit(7, (unsigned long *)&reg, tmp);

  w25qxxx_transmmit_cmd(w25qxxx, WRITE_ENABLE);
  w25qxxx_write_status_reg(w25qxxx, WRITE_STATUS_REGISTER1, reg);
}

void w25qxxx_write_cmp(u8 w25qxxx, u8 tmp)
{
    u8 reg = 0;

    reg = w25qxxx_read_status_reg(w25qxxx, READ_STATUS_REGISTER2);

    __assign_bit(6, (unsigned long *)&reg, tmp);

    w25qxxx_transmmit_cmd(w25qxxx, WRITE_ENABLE);
    w25qxxx_write_status_reg(w25qxxx, WRITE_STATUS_REGISTER2, reg);
}

void w25qxxx_write_lb(u8 w25qxxx, u8 tmp)
{
  u8 reg = 0;

  reg = w25qxxx_read_status_reg(w25qxxx, READ_STATUS_REGISTER2);
  
  __assign_bit(7, (unsigned long *)&reg, tmp);

  w25qxxx_transmmit_cmd(w25qxxx, WRITE_ENABLE);
  w25qxxx_write_status_reg(w25qxxx, WRITE_STATUS_REGISTER2, reg);
}

void w25qxxx_write_qe(u8 w25qxxx, u8 tmp)
{
    u8 reg = 0;

    reg = w25qxxx_read_status_reg(w25qxxx, READ_STATUS_REGISTER2);

    __assign_bit(1, (unsigned long *)&reg, tmp);

    w25qxxx_transmmit_cmd(w25qxxx, WRITE_ENABLE);
    w25qxxx_write_status_reg(w25qxxx, WRITE_STATUS_REGISTER2, reg);
}

void w25qxxx_write_srp1(u8 w25qxxx, u8 tmp)
{
    u8 reg = 0;

    reg = w25qxxx_read_status_reg(w25qxxx, READ_STATUS_REGISTER2);

    __assign_bit(0, (unsigned long *)&reg, tmp);

    w25qxxx_transmmit_cmd(w25qxxx, WRITE_ENABLE);
    w25qxxx_write_status_reg(w25qxxx, WRITE_STATUS_REGISTER2, reg);
}

void w25qxxx_write_hold_rst(u8 w25qxxx, u8 tmp)
{
    u8 reg = 0;

    reg = w25qxxx_read_status_reg(w25qxxx, READ_STATUS_REGISTER3);

    __assign_bit(7, (unsigned long *)&reg, tmp);

    w25qxxx_transmmit_cmd(w25qxxx, WRITE_ENABLE);
    w25qxxx_write_status_reg(w25qxxx, WRITE_STATUS_REGISTER3, reg);
}

void w25qxxx_write_drv1(u8 w25qxxx, u8 tmp)
{
    u8 reg = 0;

    reg = w25qxxx_read_status_reg(w25qxxx, READ_STATUS_REGISTER3);

    __assign_bit(6, (unsigned long *)&reg, tmp);

    w25qxxx_transmmit_cmd(w25qxxx, WRITE_ENABLE);
    w25qxxx_write_status_reg(w25qxxx, WRITE_STATUS_REGISTER3, reg);
}

void w25qxxx_write_drv0(u8 w25qxxx, u8 tmp)
{
    u8 reg = 0;

    reg = w25qxxx_read_status_reg(w25qxxx, READ_STATUS_REGISTER3);

    __assign_bit(5, (unsigned long *)&reg, tmp);

    w25qxxx_transmmit_cmd(w25qxxx, WRITE_ENABLE);
    w25qxxx_write_status_reg(w25qxxx, WRITE_STATUS_REGISTER3, reg);
}

void w25qxxx_write_wps(u8 w25qxxx, u8 tmp)
{
    u8 reg = 0;

    reg = w25qxxx_read_status_reg(w25qxxx, READ_STATUS_REGISTER3);

    __assign_bit(2, (unsigned long *)&reg, tmp);

    w25qxxx_transmmit_cmd(w25qxxx, WRITE_ENABLE);
    w25qxxx_write_status_reg(w25qxxx, WRITE_STATUS_REGISTER3, reg);
}

void w25qxxx_write_adp(u8 w25qxxx, u8 tmp)
{
    u8 reg = 0;

    reg = w25qxxx_read_status_reg(w25qxxx, READ_STATUS_REGISTER3);

    __assign_bit(1, (unsigned long *)&reg, tmp);

    w25qxxx_transmmit_cmd(w25qxxx, WRITE_ENABLE);
    w25qxxx_write_status_reg(w25qxxx, WRITE_STATUS_REGISTER3, reg);
}

void w25qxxx_write_enable(u8 w25qxxx)
{
    w25qxxx_transmmit_cmd(w25qxxx, WRITE_ENABLE);
}

void w25qxxx_write_enable_forvolatile(u8 w25qxxx)
{
    w25qxxx_transmmit_cmd(w25qxxx, WRITE_ENABLE_FORVOLATILE);
}

void w25qxxx_write_disable(u8 w25qxxx)
{
    w25qxxx_transmmit_cmd(w25qxxx, WRITE_DISABLE);
}

u8 w25qxxx_read_status_reg(u8 w25qxxx, u8 num)
{
    u8 tmp = 0;

    w25qxxx_cs_low(w25qxxx);

    switch (num) {
        case READ_STATUS_REGISTER1:
            w25qxxx_port_transmmit(w25qxxx, READ_STATUS_REGISTER1);
            tmp = w25qxxx_port_receive(w25qxxx);
            break;

        case READ_STATUS_REGISTER2:
            w25qxxx_port_transmmit(w25qxxx, READ_STATUS_REGISTER2);
            tmp = w25qxxx_port_receive(w25qxxx);
            break;

        case READ_STATUS_REGISTER3:
            w25qxxx_port_transmmit(w25qxxx, READ_STATUS_REGISTER3);
            tmp = w25qxxx_port_receive(w25qxxx);
            break;

        default:
            break;
    }

    w25qxxx_cs_high(w25qxxx);

    return tmp;
}

void w25qxxx_write_status_reg(u8 w25qxxx, u8 num, u8 tmp)
{
    w25qxxx_write_enable(w25qxxx);

    w25qxxx_cs_low(w25qxxx);

    switch (num) {
        case WRITE_STATUS_REGISTER1:
            w25qxxx_port_transmmit(w25qxxx, WRITE_STATUS_REGISTER1);
            w25qxxx_port_transmmit(w25qxxx, tmp);
            break;

        case WRITE_STATUS_REGISTER2:
            w25qxxx_port_transmmit(w25qxxx, WRITE_STATUS_REGISTER2);
            w25qxxx_port_transmmit(w25qxxx, tmp);
            break;

        case WRITE_STATUS_REGISTER3:
            w25qxxx_port_transmmit(w25qxxx, WRITE_STATUS_REGISTER3);
            w25qxxx_port_transmmit(w25qxxx, tmp);
            break;

        default:
            break;
    }

    w25qxxx_cs_high(w25qxxx);
}

u8 w25q256_read_ext_addr_reg(u8 w25qxxx)
{
    u8 tmp = 0;

    w25qxxx_cs_low(w25qxxx);
    w25qxxx_port_transmmit(w25qxxx, READ_EXTENDED_ADDRESS_REGISTER);
    tmp = w25qxxx_port_receive(w25qxxx);
    w25qxxx_cs_high(w25qxxx);

    return tmp;
}

void w25q256_write_ext_addr_reg(u8 w25qxxx, u8 tmp)
{
    w25qxxx_write_enable(w25qxxx);
    w25qxxx_cs_low(w25qxxx);
    w25qxxx_port_transmmit(w25qxxx, WRITE_EXTENDED_ADDRESS_REGISTER);
    w25qxxx_port_transmmit(w25qxxx, tmp);
    w25qxxx_cs_high(w25qxxx);
}

void w25q256_enter_4byte_addr_mode(u8 w25qxxx)
{
    w25qxxx_transmmit_cmd(w25qxxx, ENTER_4BYTE_ADDRESS_MODE);
}

void w25q256_exit_4byte_addr_mode(u8 w25qxxx)
{
    w25qxxx_transmmit_cmd(w25qxxx, EXIT_4BYTE_ADDRESS_MODE);
}

void w25qxxx_read_data(u8 w25qxxx, u32 addr, u8 *pdata, u32 length)
{
    u8 sub_addr[4];

    if (w25qxxx == W25Q256)
        sub_addr[0] = (addr & 0xFF000000) >> 24;

    sub_addr[1] = (addr & 0x00FF0000) >> 16;
    sub_addr[2] = (addr & 0x0000FF00) >> 8;
    sub_addr[3] = (addr & 0x000000FF);

    w25qxxx_cs_low(w25qxxx);
    w25qxxx_port_transmmit(w25qxxx, READ_DATA);

    if (w25qxxx == W25Q256)
        w25qxxx_port_transmmit(w25qxxx, sub_addr[0]);

    w25qxxx_port_multi_transmmit(w25qxxx, &sub_addr[1], 3);

    w25qxxx_port_multi_receive(w25qxxx, pdata, length);
    w25qxxx_cs_high(w25qxxx);
}

void w25q256_read_data_with_4byte_addr(u8 w25qxxx, u32 addr, u8 *pdata, u32 length)
{
    u8 sub_addr[4];

    sub_addr[0] = (addr & 0xFF000000) >> 24;
    sub_addr[1] = (addr & 0x00FF0000) >> 16;
    sub_addr[2] = (addr & 0x0000FF00) >> 8;
    sub_addr[3] = (addr & 0x000000FF);

    w25qxxx_cs_low(w25qxxx);
    w25qxxx_port_transmmit(w25qxxx, READ_DATA_WITH_4Byte_ADDRESS);
    w25qxxx_port_multi_transmmit(w25qxxx, sub_addr, 4);

    w25qxxx_port_multi_receive(w25qxxx, pdata, length);
    w25qxxx_cs_high(w25qxxx);
}

void w25qxxx_fast_read(u8 w25qxxx, u32 addr, u8 *pdata, u32 length)
{
    u8 sub_addr[4];

    if (w25qxxx == W25Q256)
        sub_addr[0] = (addr & 0xFF000000) >> 24;

    sub_addr[1] = (addr & 0x00FF0000) >> 16;
    sub_addr[2] = (addr & 0x0000FF00) >> 8;
    sub_addr[3] = (addr & 0x000000FF);

    w25qxxx_cs_low(w25qxxx);
    w25qxxx_port_transmmit(w25qxxx, FAST_READ);

    if (w25qxxx == W25Q256)
        w25qxxx_port_transmmit(w25qxxx, sub_addr[0]);

    w25qxxx_port_multi_transmmit(w25qxxx, &sub_addr[1], 3);
    w25qxxx_port_transmmit(w25qxxx, DUMMY_BYTE);

    w25qxxx_port_multi_receive(w25qxxx, pdata, length);
    w25qxxx_cs_high(w25qxxx);
}

void w25q256_fast_read_with_4byte_addr(u8 w25qxxx, u32 addr, u8 *pdata, u32 length)
{
    u8 sub_addr[4];

    sub_addr[0] = (addr & 0xFF000000) >> 24;
    sub_addr[1] = (addr & 0x00FF0000) >> 16;
    sub_addr[2] = (addr & 0x0000FF00) >> 8;
    sub_addr[3] = (addr & 0x000000FF);

    w25qxxx_cs_low(w25qxxx);
    w25qxxx_port_transmmit(w25qxxx, FAST_READ_WITH_4Byte_ADDRESS);
    w25qxxx_port_multi_transmmit(w25qxxx, sub_addr, 4);
    w25qxxx_port_transmmit(w25qxxx, DUMMY_BYTE);

    w25qxxx_port_multi_receive(w25qxxx, pdata, length);
    w25qxxx_cs_high(w25qxxx);
}

static int w25qxxx_page_program(u8 w25qxxx, u32 addr, u8 *pdata, u16 length)
{
    u8 sub_addr[4];
    u8 busy;
    int ret;

    if (length == 0)
        printk(KERN_ERR "Programing page failed.");

    if (w25qxxx == W25Q256)
        sub_addr[0] = (addr & 0xFF000000) >> 24;

    sub_addr[1] = (addr & 0x00FF0000) >> 16;
    sub_addr[2] = (addr & 0x0000FF00) >> 8;
    sub_addr[3] = (addr & 0x000000FF);

    w25qxxx_write_enable(w25qxxx);
    w25qxxx_cs_low(w25qxxx);
    w25qxxx_port_transmmit(w25qxxx, PAGE_PROGRAM);

    if (w25qxxx == W25Q256)
        w25qxxx_port_transmmit(w25qxxx, sub_addr[0]);

    w25qxxx_port_multi_transmmit(w25qxxx, &sub_addr[1], 3);
    w25qxxx_port_multi_transmmit(w25qxxx, pdata, length);
    w25qxxx_cs_high(w25qxxx);

    ret = readx_poll_timeout_atomic(w25qxxx_read_busy, w25qxxx, busy,
            busy == 0, 0, 30000000);

    if (ret)
        printk(KERN_ERR "Programing page is timeout.");

    return 0;
}

static int w25qxxx_multi_page_program(u8 w25qxxx, u32 addr, u8 *pdata, u16 length)
{
    u8 num_of_page = 0, num_of_single = 0, sub_addr = 0, cnt = 0, remain_of_single = 0;

    /* Mod operation, if addr is an integer multiple of _PAGE_SIZE, sub_addr value is 0 */
    sub_addr = addr % _PAGE_SIZE;

    /* The difference count is just enough to line up to the page addr */
    cnt = _PAGE_SIZE - sub_addr;
    /* Figure out how many integer pages to write */
    num_of_page =  length / _PAGE_SIZE;
    /* mod operation is used to calculate the num of bytes less than one page */
    num_of_single = length % _PAGE_SIZE;

    /* sub_addr=0, then addr is just aligned by page */
    if (sub_addr == 0) {
        /* length < _PAGE_SIZE */
        if (num_of_page == 0)
            w25qxxx_page_program(w25qxxx, addr, pdata, length);
        else { /* length > _PAGE_SIZE */
            /* Let me write down all the integer pages */
            while (num_of_page--) {
                w25qxxx_page_program(w25qxxx, addr, pdata, _PAGE_SIZE);

                addr +=  _PAGE_SIZE;
                pdata += _PAGE_SIZE;
            }

            /* If you have more than one page of data, write it down*/
            w25qxxx_page_program(w25qxxx, addr, pdata, num_of_single);
        }
    }
    /* If the addr is not aligned with _PAGE_SIZE */
    else {
        /* length < _PAGE_SIZE */
        if (num_of_page == 0) {
            /* The remaining count positions on the current page are smaller than num_of_single */
            if (num_of_single > cnt) {
                remain_of_single = num_of_single - cnt;

                /* Fill in the front page first */
                w25qxxx_page_program(w25qxxx, addr, pdata, cnt);

                addr +=  cnt;
                pdata += cnt;

                /* Let me write the rest of the data */
                w25qxxx_page_program(w25qxxx, addr, pdata, remain_of_single);
            } else  /* The remaining count position of the current page can write num_of_single data */
                w25qxxx_page_program(w25qxxx, addr, pdata, length);
        } else { /* length > _PAGE_SIZE */
            /* The addr is not aligned and the extra count is treated separately, not added to the operation */
            length -= cnt;
            num_of_page =  length / _PAGE_SIZE;
            num_of_single = length % _PAGE_SIZE;

            w25qxxx_page_program(w25qxxx, addr, pdata, cnt);

            addr +=  cnt;
            pdata += cnt;

            /* Write all the integer pages */
            while (num_of_page--) {
                w25qxxx_page_program(w25qxxx, addr, pdata, _PAGE_SIZE);

                addr +=  _PAGE_SIZE;
                pdata += _PAGE_SIZE;
            }

            /* If you have more than one page of data, write it down */
            if (num_of_single != 0)
                w25qxxx_page_program(w25qxxx, addr, pdata, num_of_single);
        }
    }

    return 0;
}

static u8 w25qxxx_single_sector[SECTOR_SIZE];

static int w25qxxx_process_partial_sector(u8 w25qxxx, u32 start_addr, u8 *pdata, u16 length)
{
    u32 offset = 0;
    u32 addr = 0;
    u32 i = 0;

    offset = start_addr % SECTOR_SIZE;
    addr = start_addr - offset;

    w25qxxx_read_data(w25qxxx, start_addr, &w25qxxx_single_sector[offset], length);

    for (i = offset; i < offset + length; i++) {
        if (w25qxxx_single_sector[i] != 0xFF) {
            w25qxxx_read_data(w25qxxx, addr, &w25qxxx_single_sector[0], offset);
            w25qxxx_read_data(w25qxxx, start_addr + length, &w25qxxx_single_sector[offset + length], SECTOR_SIZE - offset - length);
            memcpy(&w25qxxx_single_sector[offset], pdata, length);
            w25qxxx_sector_erase(w25qxxx, addr);
            w25qxxx_multi_page_program(w25qxxx, addr, w25qxxx_single_sector, SECTOR_SIZE);
            return -EPIPE;
        }
    }

    w25qxxx_multi_page_program(w25qxxx, start_addr, pdata, length);

    return 0;
}

void w25qxxx_multi_sector_program(u8 w25qxxx, u32 addr, u8 *pdata, u32 length)
{
    u32 num_of_sector = 0, num_of_single = 0, offset = 0;
    u32 cnt = 0, remain_of_single = 0;

    /* Mod operation, if addr is an integer multiple of SECTOR_SIZE, offset value is 0 */
    offset = addr % SECTOR_SIZE;

    /* The difference count is just enough to line up to the page addr */
    cnt = SECTOR_SIZE - offset;
    /* Figure out how many integer pages to write */
    num_of_sector =  length / SECTOR_SIZE;
    /* mod operation is used to calculate the num of bytes less than one page */
    num_of_single = length % SECTOR_SIZE;

    /* offset=0, then addr is just aligned by page */
    if (offset == 0) {
        /* length < SECTOR_SIZE */
        if (num_of_sector == 0)
            w25qxxx_process_partial_sector(w25qxxx, addr, pdata, length);
        else { /* length > SECTOR_SIZE */
            /* Let me write down all the integer pages */
            while (num_of_sector--) {
                w25qxxx_sector_erase(w25qxxx, addr);
                w25qxxx_multi_page_program(w25qxxx, addr, pdata, SECTOR_SIZE);
                addr +=  SECTOR_SIZE;
                pdata += SECTOR_SIZE;
            }

            /* If you have more than one page of data, write it down*/
            w25qxxx_process_partial_sector(w25qxxx, addr, pdata, num_of_single);
        }
    }
    /* If the addr is not aligned with SECTOR_SIZE */
    else {
        /* length < SECTOR_SIZE */
        if (num_of_sector == 0) {
            /* The remaining count positions on the current page are smaller than num_of_single */
            if (num_of_single > cnt) {
                remain_of_single = num_of_single - cnt;

                /* Fill in the front page first */
                w25qxxx_process_partial_sector(w25qxxx, addr, pdata, cnt);
                addr +=  cnt;
                pdata += cnt;

                /* Let me write the rest of the data */
                w25qxxx_process_partial_sector(w25qxxx, addr, pdata, remain_of_single);
            } else /* The remaining count position of the current page can write num_of_single data */
                w25qxxx_process_partial_sector(w25qxxx, addr, pdata, length);
        } else { /* length > SECTOR_SIZE */
            /* The addr is not aligned and the extra count is treated separately, not added to the operation */
            length -= cnt;
            num_of_sector =  length / SECTOR_SIZE;
            num_of_single = length % SECTOR_SIZE;

            w25qxxx_process_partial_sector(w25qxxx, addr, pdata, cnt);
            addr +=  cnt;
            pdata += cnt;

            /* Write all the integer pages */
            while (num_of_sector--) {
                w25qxxx_sector_erase(w25qxxx, addr);
                w25qxxx_multi_page_program(w25qxxx, addr, pdata, SECTOR_SIZE);
                addr +=  SECTOR_SIZE;
                pdata += SECTOR_SIZE;
            }

            /* If you have more than one page of data, write it down */
            if (num_of_single != 0)
                w25qxxx_process_partial_sector(w25qxxx, addr, pdata, num_of_single);
        }
    }
}

void w25qxxx_write_data(u8 w25qxxx, u32 addr, u8 *pdata, u16 length)
{
    u32 remain = 0;

    if (w25qxxx == W25Q128)
        remain = w25q128_max_addr - addr;
    else if (w25qxxx == W25Q256)
        remain = w25q256_max_addr - addr;

    if (remain < length) {
        printk(KERN_DEBUG
            "There is not enough space left to write the specified length.");
        return ;
    }

    w25qxxx_multi_sector_program(w25qxxx, addr, pdata, length);
}

void w25qxxx_sector_erase(u8 w25qxxx, u32 addr)
{
    u8 sub_addr[4];
    u8 busy;

    if (w25qxxx == W25Q256)
        sub_addr[0] = (addr & 0xFF000000) >> 24;

    sub_addr[1] = (addr & 0x00FF0000) >> 16;
    sub_addr[2] = (addr & 0x0000FF00) >> 8;
    sub_addr[3] = (addr & 0x000000FF);

    w25qxxx_write_enable(w25qxxx);
    w25qxxx_cs_low(w25qxxx);
    w25qxxx_port_transmmit(w25qxxx, SECTOR_ERASE);

    if (w25qxxx == W25Q256)
        w25qxxx_port_transmmit(w25qxxx, sub_addr[0]);

    w25qxxx_port_multi_transmmit(w25qxxx, &sub_addr[1], 3);
    w25qxxx_cs_high(w25qxxx);

    readx_poll_timeout_atomic(w25qxxx_read_busy, w25qxxx, busy,
        busy == 0, 0, 30000000);
}

void w25qxxx_block_erase_with_32kb(u8 w25qxxx, u32 addr)
{
    u8 sub_addr[4];
    u8 busy;

    if (w25qxxx == W25Q256)
        sub_addr[0] = (addr & 0xFF000000) >> 24;

    sub_addr[1] = (addr & 0x00FF0000) >> 16;
    sub_addr[2] = (addr & 0x0000FF00) >> 8;
    sub_addr[3] = (addr & 0x000000FF);

    w25qxxx_write_enable(w25qxxx);
    w25qxxx_cs_low(w25qxxx);
    w25qxxx_port_transmmit(w25qxxx, BLOCK_ERASE_32KB);

    if (w25qxxx == W25Q256)
        w25qxxx_port_transmmit(w25qxxx, sub_addr[0]);

    w25qxxx_port_multi_transmmit(w25qxxx, &sub_addr[1], 3);
    w25qxxx_cs_high(w25qxxx);

    readx_poll_timeout_atomic(w25qxxx_read_busy, w25qxxx, busy,
        busy == 0, 0, 30000000);
}

void w25qxxx_block_erase_with_64kb(u8 w25qxxx, u32 addr)
{
    u8 sub_addr[4];
    u8 busy;

    if (w25qxxx == W25Q256)
        sub_addr[0] = (addr & 0xFF000000) >> 24;

    sub_addr[1] = (addr & 0x00FF0000) >> 16;
    sub_addr[2] = (addr & 0x0000FF00) >> 8;
    sub_addr[3] = (addr & 0x000000FF);

    w25qxxx_write_enable(w25qxxx);
    w25qxxx_cs_low(w25qxxx);
    w25qxxx_port_transmmit(w25qxxx, BLOCK_ERASE_64KB);

    if (w25qxxx == W25Q256)
        w25qxxx_port_transmmit(w25qxxx, sub_addr[0]);

    w25qxxx_port_multi_transmmit(w25qxxx, &sub_addr[1], 3);
    w25qxxx_cs_high(w25qxxx);

    readx_poll_timeout_atomic(w25qxxx_read_busy, w25qxxx, busy,
        busy == 0, 0, 30000000);
}

void sw25qxxx_chip_erase(u8 w25qxxx)
{
    u8 busy;

    w25qxxx_write_enable(w25qxxx);
    w25qxxx_transmmit_cmd(w25qxxx, CHIP_ERASE);

    readx_poll_timeout_atomic(w25qxxx_read_busy, w25qxxx, busy,
        busy == 0, 0, 30000000);
}

void w25qxxx_erase_program_suspend(u8 w25qxxx)
{
    if (w25qxxx_read_busy(w25qxxx) == 0 && w25qxxx_read_sus(w25qxxx) == 1)
        return ;

    w25qxxx_transmmit_cmd(w25qxxx, ERASE_PROGRAM_SUSPEND);
    udelay(20);
}

void w25qxxx_erase_program_resume(u8 w25qxxx)
{
    if (w25qxxx_read_busy(w25qxxx) == 1 && w25qxxx_read_sus(w25qxxx) == 0)
        return ;

    w25qxxx_transmmit_cmd(w25qxxx, ERASE_PROGRAM_RESUME);
}

void w25qxxx_power_down(u8 w25qxxx)
{
    w25qxxx_transmmit_cmd(w25qxxx, POWER_DOWN);
    udelay(3);
}

void w25qxxx_release_power_down(u8 w25qxxx)
{
    w25qxxx_transmmit_cmd(w25qxxx, RELEASE_POWERDOWN_DEVICE_ID);
    udelay(3);
}

u8 w25qxxx_release_device_id(u8 w25qxxx)
{
    u8 tmp = 0;

    w25qxxx_cs_low(w25qxxx);
    w25qxxx_port_transmmit(w25qxxx, RELEASE_POWERDOWN_DEVICE_ID);
    w25qxxx_port_transmmit(w25qxxx, DUMMY_BYTE);
    w25qxxx_port_transmmit(w25qxxx, DUMMY_BYTE);
    w25qxxx_port_transmmit(w25qxxx, DUMMY_BYTE);
    tmp = w25qxxx_port_receive(w25qxxx);
    w25qxxx_cs_high(w25qxxx);

    return tmp;
}

void w25qxxx_read_manufacturer_device_id(u8 w25qxxx, u8 *ManufacturerID, u8 *DeviceID)
{
    w25qxxx_cs_low(w25qxxx);
    w25qxxx_port_transmmit(w25qxxx, READ_MANUFACTURER_DEVICE_ID);
    w25qxxx_port_transmmit(w25qxxx, 0x00);
    w25qxxx_port_transmmit(w25qxxx, 0x00);
    w25qxxx_port_transmmit(w25qxxx, 0x00);
    *ManufacturerID = w25qxxx_port_receive(w25qxxx);
    *DeviceID = w25qxxx_port_receive(w25qxxx);
    w25qxxx_cs_high(w25qxxx);
}

void w25qxxx_read_unique_id(u8 w25qxxx, u8 *unique_id)
{
    w25qxxx_cs_low(w25qxxx);
    w25qxxx_port_transmmit(w25qxxx, READ_UNIQUE_ID_NUMBER);

    if (w25qxxx == W25Q256) {
        if (w25qxxx_read_ads(w25qxxx) == 1)
            w25qxxx_port_transmmit(w25qxxx, DUMMY_BYTE);
    }

    w25qxxx_port_transmmit(w25qxxx, DUMMY_BYTE);
    w25qxxx_port_transmmit(w25qxxx, DUMMY_BYTE);
    w25qxxx_port_transmmit(w25qxxx, DUMMY_BYTE);
    w25qxxx_port_transmmit(w25qxxx, DUMMY_BYTE);

    w25qxxx_port_multi_receive(w25qxxx, unique_id, 8);
    w25qxxx_cs_high(w25qxxx);
}

void w25qxxx_read_jedec_id(u8 w25qxxx, u8 *JEDECID)
{
    w25qxxx_cs_low(w25qxxx);
    w25qxxx_port_transmmit(w25qxxx, READ_JEDEC_ID);

    w25qxxx_port_multi_receive(w25qxxx, JEDECID, 3);
    w25qxxx_cs_high(w25qxxx);
}

void w25qxxx_read_sfdp_reg(u8 w25qxxx, u32 addr, u8 *pdata, u16 length)
{
    u8 sub_addr = 0;

    sub_addr = (addr & 0x000000FF);

    w25qxxx_cs_low(w25qxxx);
    w25qxxx_port_transmmit(w25qxxx, READ_SFDP_REGISTER);
    w25qxxx_port_transmmit(w25qxxx, 0x00);
    w25qxxx_port_transmmit(w25qxxx, 0x00);
    w25qxxx_port_transmmit(w25qxxx, sub_addr);
    w25qxxx_port_transmmit(w25qxxx, DUMMY_BYTE);

    w25qxxx_port_multi_receive(w25qxxx, pdata, length);
    w25qxxx_cs_high(w25qxxx);
}

void w25qxxx_erase_security_regs(u8 w25qxxx, u8 addr)
{
    u8 busy;

    w25qxxx_write_enable(w25qxxx);
    w25qxxx_cs_low(w25qxxx);
    w25qxxx_port_transmmit(w25qxxx, ERASE_SECURITY_REGISTERS);

    if (w25qxxx == W25Q256)
        w25qxxx_port_transmmit(w25qxxx, 0x00);

    w25qxxx_port_transmmit(w25qxxx, 0x00);
    w25qxxx_port_transmmit(w25qxxx, addr);
    w25qxxx_port_transmmit(w25qxxx, 0x00);
    w25qxxx_cs_high(w25qxxx);

    readx_poll_timeout_atomic(w25qxxx_read_busy, w25qxxx, busy,
        busy == 0, 0, 30000000);
}

//void w25qxxx_program_security_regs(u8 w25qxxx, u32 regNum, u32 Byteaddr, u8 *pdata, u16 length)
//{
//  w25qxxx_write_enable(w25qxxx);
//  w25qxxx_cs_low(w25qxxx);
//  w25qxxx_port_transmmit(w25qxxx, PROGRAM_SECURITY_REGISTERS);
//  if(w25qxxx == W25Q256)
//  {
//    w25qxxx_port_transmmit(w25qxxx, 0x00);
//  }
//  w25qxxx_port_transmmit(w25qxxx, 0x00);
//  w25qxxx_port_transmmit(w25qxxx, regNum);
//  w25qxxx_port_transmmit(w25qxxx, Byteaddr);
//  w25qxxx_cs_high(w25qxxx);
//}
//
//void w25qxxx_read_security_regs(u8 w25qxxx, u8 regNum, u8 Byteaddr, u8 *pdata, u16 length)
//{
//  w25qxxx_cs_low(w25qxxx);
//  w25qxxx_port_transmmit(w25qxxx, READ_SECURITY_REGISTERS);
//  w25qxxx_port_transmmit(w25qxxx, 0x00);
//  w25qxxx_port_transmmit(w25qxxx, regNum);
//  w25qxxx_port_transmmit(w25qxxx, Byteaddr);
//  w25qxxx_port_transmmit(w25qxxx, DUMMY_BYTE);
//
//  for(u32 i = 0;i < length;i++)
//  {
//    pdata[i] = w25qxxx_port_receive(w25qxxx);
//  }
//
//  w25qxxx_cs_high(w25qxxx);
//}

void w25qxxx_enter_qpi_mode(u8 w25qxxx)
{
    w25qxxx_write_qe(w25qxxx, 1);
    w25qxxx_transmmit_cmd(w25qxxx, ENTER_QPI_MODE);
}

void w25qxxx_exit_qpi_mode(u8 w25qxxx)
{
    w25qxxx_transmmit_cmd(w25qxxx, EXIT_QPI_MODE);
}

void w25qxxx_individual_block_sector_lock(u8 w25qxxx, u32 addr)
{
    u8 sub_addr[4];

    if (w25qxxx == W25Q256)
        sub_addr[0] = (addr & 0xFF000000) >> 24;

    sub_addr[1] = (addr & 0x00FF0000) >> 16;
    sub_addr[2] = (addr & 0x0000FF00) >> 8;
    sub_addr[3] = (addr & 0x000000FF);

    w25qxxx_write_wps(w25qxxx, 1);
    w25qxxx_cs_low(w25qxxx);
    w25qxxx_port_transmmit(w25qxxx, INDIVIDUAL_BLOCK_SECTOR_LOCK);

    if (w25qxxx == W25Q256)
        w25qxxx_port_transmmit(w25qxxx, sub_addr[0]);

    w25qxxx_port_multi_transmmit(w25qxxx, &sub_addr[1], 3);
    w25qxxx_cs_high(w25qxxx);
}

void w25qxxx_individual_block_sector_unlock(u8 w25qxxx, u32 addr)
{
    u8 sub_addr[4];

    if (w25qxxx == W25Q256)
        sub_addr[0] = (addr & 0xFF000000) >> 24;

    sub_addr[1] = (addr & 0x00FF0000) >> 16;
    sub_addr[2] = (addr & 0x0000FF00) >> 8;
    sub_addr[3] = (addr & 0x000000FF);

    w25qxxx_write_wps(w25qxxx, 1);
    w25qxxx_cs_low(w25qxxx);
    w25qxxx_port_transmmit(w25qxxx, INDIVIDUAL_BLOCK_SECTOR_UNLOCK);

    if (w25qxxx == W25Q256)
        w25qxxx_port_transmmit(w25qxxx, sub_addr[0]);

    w25qxxx_port_multi_transmmit(w25qxxx, &sub_addr[1], 3);
    w25qxxx_cs_high(w25qxxx);
}

u8 w25qxxx_read_block_sector_lock(u8 w25qxxx, u32 addr)
{
    u8 tmp = 0;
    u8 sub_addr[4];

    if (w25qxxx == W25Q256)
        sub_addr[0] = (addr & 0xFF000000) >> 24;

    sub_addr[1] = (addr & 0x00FF0000) >> 16;
    sub_addr[2] = (addr & 0x0000FF00) >> 8;
    sub_addr[3] = (addr & 0x000000FF);

    w25qxxx_write_wps(w25qxxx, 1);
    w25qxxx_cs_low(w25qxxx);
    w25qxxx_port_transmmit(w25qxxx, READ_BLOCK_SECTOR_LOCK);

    if (w25qxxx == W25Q256)
        w25qxxx_port_transmmit(w25qxxx, sub_addr[0]);

    w25qxxx_port_multi_transmmit(w25qxxx, &sub_addr[1], 3);
    tmp = w25qxxx_port_receive(w25qxxx);
    w25qxxx_cs_high(w25qxxx);

    return (tmp & 0x01);
}

void w25qxxx_global_block_sector_lock(u8 w25qxxx)
{
    w25qxxx_write_enable(w25qxxx);
    w25qxxx_transmmit_cmd(w25qxxx, GLOBAL_BLOCK_SECTOR_LOCK);
}

void w25qxxx_global_block_sector_unlock(u8 w25qxxx)
{
    w25qxxx_write_enable(w25qxxx);
    w25qxxx_transmmit_cmd(w25qxxx, GLOBAL_BLOCK_SECTOR_UNLOCK);
}

void w25qxxx_enable_reset(u8 w25qxxx)
{
    w25qxxx_transmmit_cmd(w25qxxx, ENABLE_RESET);
}

u8 w25qxxx_soft_reset(u8 w25qxxx)
{
    if (w25qxxx_read_busy(w25qxxx) == 1 || w25qxxx_read_sus(w25qxxx) == 1)
        return false;

    w25qxxx_enable_reset(w25qxxx);
    w25qxxx_transmmit_cmd(w25qxxx, RESET_DEVICE);
    udelay(30);

    return true;
}

void w25qxxx_init(u8 w25qxxx)
{
    w25qxxx_release_power_down(w25qxxx);
    w25qxxx_soft_reset(w25qxxx);
    w25qxxx_release_device_id(w25qxxx);

    switch (w25qxxx) {
        case W25Q128:
            break;

        case W25Q256:
            if (w25qxxx_read_ads(w25qxxx) == 0)
                w25q256_enter_4byte_addr_mode(w25qxxx);

            break;

        default:
            break;
    }
}

void w25qxxx_read_info(u8 w25qxxx)
{
    u8 manufacturer_id = 0, device_id = 0;
    u8 jedec_id[3];
    u8 unique_id[8];

    printk(KERN_DEBUG "w25qxxx is 0x%02X.", w25qxxx);
    w25qxxx_read_jedec_id(w25qxxx, jedec_id);
    printk(KERN_DEBUG "JEDEC ID is 0x%02X%02X%02X",
        jedec_id[0], jedec_id[1], jedec_id[2]);
    w25qxxx_read_manufacturer_device_id(w25qxxx, &manufacturer_id, &device_id);
    printk(KERN_DEBUG "Manufacturer ID is 0x%02X, Device ID is 0x%02X.",
        manufacturer_id, device_id);
    w25qxxx_read_unique_id(w25qxxx, unique_id);
    printk(KERN_DEBUG "Unique ID is %02X%02X%02X%02X%02X%02X%02X%02X",
        unique_id[0], unique_id[1], unique_id[2], unique_id[3],
        unique_id[4], unique_id[5], unique_id[6], unique_id[7]);
}

#ifdef DESIGN_VERIFICATION_W25QXXX
#include "kinetis/test-kinetis.h"

static u8 tx_buffer[32767];
static u8 rx_buffer[32767];

int t_w25qxxx_loopback(int argc, char **argv)
{   
    u8 w25qxxx = W25Q128;
    u32 tmp_rng = 0;
    u16 i, length = 0;
    u32 test_addr = 0;

    if (argc > 1) {
        if (!strcmp(argv[1], "w25q128"))
            w25qxxx = W25Q128;
        else if (!strcmp(argv[1], "w25q256"))
            w25qxxx = W25Q256;
    }
    
    tmp_rng = random_get32bit();
    length = tmp_rng & 0x7FFF;
    printk(KERN_DEBUG "length = %d.", length);

    memset(tx_buffer, 0, length);
    memset(rx_buffer, 0, length);

    tmp_rng = random_get32bit();

    switch (w25qxxx) {
        case W25Q128:
            test_addr = tmp_rng & 0xFFFFFF;
            break;

        case W25Q256:
            test_addr = tmp_rng & 0x1FFFFFF;
            break;

        default:
            test_addr = 0;
            break;
    }

    printk(KERN_DEBUG "test addr: 0x%08x.", test_addr);

    for (i = 0; i < length; i += 4) {
        tmp_rng = random_get32bit();
        tx_buffer[i + 3] = (tmp_rng & 0xFF000000) >> 24;;
        tx_buffer[i + 2] = (tmp_rng & 0x00FF0000) >> 16;
        tx_buffer[i + 1] = (tmp_rng & 0x0000FF00) >> 8;
        tx_buffer[i + 0] = (tmp_rng & 0x000000FF);
        memcpy(&tx_buffer[i], &tmp_rng, sizeof(tmp_rng));
    }

    w25qxxx_write_data(w25qxxx, test_addr, tx_buffer, length);
    w25qxxx_read_data(w25qxxx, test_addr, rx_buffer, length);

    for (i = 0; i < length; i++) {
        if (tx_buffer[i] != rx_buffer[i]) {
            printk(KERN_DEBUG "tx[%d]: 0x%02x, rx[%d]: 0x%02x",
                i, tx_buffer[i],
                i, rx_buffer[i]);
            printk(KERN_DEBUG
                "Data writes and reads do not match, TEST FAILED !");
            return -EPIPE;
        }
    }

    printk(KERN_DEBUG "w25qxxx read and write TEST PASSED !");
    
    return 0;
}

#endif
