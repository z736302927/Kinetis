#ifndef __W25QXXX_H
#define __W25QXXX_H

#ifdef __cplusplus
extern "C" {
#endif

/* The following program is modified by the user according to the hardware device, otherwise the driver cannot run. */

/* Includes ------------------------------------------------------------------*/

#include <linux/types.h>

#include <kinetis/spi_soft.h>

#define W25Q64                          0x16
#define W25Q128                         0x17
#define W25Q256                         0x18
#define W25Q512                         0x19

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

struct w25qxxx_device {
	struct spi_master *spi;

	u8 id;
	u32 max_addr;
	struct list_head list;

	struct w25qxxx_status status;
	enum qspi_read_mode read_mode;
#ifdef KINETIS_FAKE_SIM
	struct spi_slave *spi_slave;
	u8 *slave_regs;
	bool thread_running;
#endif
};

/* The above procedure is modified by the user according to the hardware device, otherwise the driver cannot run. */

struct w25qxxx_device *w25qxxx_alloc_device(struct spi_master *spi);
void w25qxxx_free_device(struct w25qxxx_device *dev);
struct w25qxxx_device *w25qxxx_find_device(u8 id);
int w25qxxx_init(struct w25qxxx_device *dev);
void w25qxxx_read_info(struct w25qxxx_device *dev);
void w25qxxx_write_enable(struct w25qxxx_device *dev);
void w25qxxx_write_enable_forvolatile(struct w25qxxx_device *dev);
void w25qxxx_write_disable(struct w25qxxx_device *dev);
u8 w25qxxx_read_status_reg(struct w25qxxx_device *dev, u8 num);
void w25qxxx_write_status_reg(struct w25qxxx_device *dev, u8 num, u8 tmp);
u8 w25qxxx_read_busy(struct w25qxxx_device *dev);
void w25qxxx_read_jedec_id(struct w25qxxx_device *dev, u8 *JEDECID);
void w25qxxx_read_manufacturer_device_id(struct w25qxxx_device *dev,
					 u8 *ManufacturerID, u8 *DeviceID);
void w25qxxx_read_unique_id(struct w25qxxx_device *dev, u8 *unique_id);
void w25qxxx_read_sfdp_reg(struct w25qxxx_device *dev, u32 addr,
			   u8 *pdata, u16 length);
void w25qxxx_read_data(struct w25qxxx_device *dev, u32 addr,
		       u8 *pdata, u32 length);
void w25qxxx_read_data_with_4byte_addr(struct w25qxxx_device *dev, u32 addr,
				       u8 *pdata, u32 length);
void w25qxxx_fast_read(struct w25qxxx_device *dev, u32 addr,
		       u8 *pdata, u32 length);
void w25qxxx_fast_read_with_4byte_addr(struct w25qxxx_device *dev, u32 addr,
				       u8 *pdata, u32 length);
void w25qxxx_fast_read_dual_output(struct w25qxxx_device *dev, u32 addr,
				   u8 *pdata, u32 length);
void w25qxxx_fast_read_dual_io(struct w25qxxx_device *dev, u32 addr,
			       u8 *pdata, u32 length);
void w25qxxx_fast_read_quad_output(struct w25qxxx_device *dev, u32 addr,
				   u8 *pdata, u32 length);
void w25qxxx_fast_read_quad_io(struct w25qxxx_device *dev, u32 addr,
			      u8 *pdata, u32 length);
void w25qxxx_word_read_quad_io(struct w25qxxx_device *dev, u32 addr,
			       u8 *pdata, u32 length);
void w25qxxx_octal_word_read_quad_io(struct w25qxxx_device *dev, u32 addr,
				     u8 *pdata, u32 length);
void w25qxxx_page_read(struct w25qxxx_device *dev, u32 addr,
		       u8 *pdata, u32 length);
int w25qxxx_write_data(struct w25qxxx_device *dev, u32 addr,
		       u8 *pdata, u16 length);
int w25qxxx_sector_erase(struct w25qxxx_device *dev, u32 addr);
int w25qxxx_block_erase_with_32kb(struct w25qxxx_device *dev, u32 addr);
int w25qxxx_block_erase_with_64kb(struct w25qxxx_device *dev, u32 addr);
int w25qxxx_chip_erase(struct w25qxxx_device *dev);
void w25qxxx_erase_program_suspend(struct w25qxxx_device *dev);
void w25qxxx_erase_program_resume(struct w25qxxx_device *dev);
void w25qxxx_power_down(struct w25qxxx_device *dev);
void w25qxxx_release_power_down(struct w25qxxx_device *dev);
u8 w25qxxx_release_device_id(struct w25qxxx_device *dev);
void w25qxxx_enable_reset(struct w25qxxx_device *dev);
void w25qxxx_hard_reset(struct w25qxxx_device *dev);
int w25qxxx_soft_reset(struct w25qxxx_device *dev);
u8 w25qxxx_read_ext_addr_reg(struct w25qxxx_device *dev);
void w25qxxx_write_ext_addr_reg(struct w25qxxx_device *dev, u8 tmp);
void w25qxxx_enter_4byte_addr_mode(struct w25qxxx_device *dev);
void w25qxxx_exit_4byte_addr_mode(struct w25qxxx_device *dev);
void w25qxxx_erase_security_regs(struct w25qxxx_device *dev, u8 regNum);
void w25qxxx_program_security_regs(struct w25qxxx_device *dev, u8 regNum,
				   u16 Byteaddr, u8 *pdata, u16 length);
void w25qxxx_read_security_regs(struct w25qxxx_device *dev, u8 regNum,
				u8 Byteaddr, u8 *pdata, u16 length);
void w25qxxx_enter_qspi_mode(struct w25qxxx_device *dev);
void w25qxxx_exit_qspi_mode(struct w25qxxx_device *dev);

void w25qxxx_set_burst_wrap(struct w25qxxx_device *dev, u8 wrap_size);
void w25qxxx_individual_block_sector_lock(struct w25qxxx_device *dev, u32 addr);
void w25qxxx_individual_block_sector_unlock(struct w25qxxx_device *dev, u32 addr);
u8 w25qxxx_read_block_sector_lock(struct w25qxxx_device *dev, u32 addr);
void w25qxxx_global_block_sector_lock(struct w25qxxx_device *dev);
void w25qxxx_global_block_sector_unlock(struct w25qxxx_device *dev);
void w25qxxx_set_read_mode(struct w25qxxx_device *dev,
			   enum qspi_read_mode mode);

#ifdef __cplusplus
}
#endif

#endif /* __W25QXXX_H */
