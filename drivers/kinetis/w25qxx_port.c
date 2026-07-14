
#include <linux/printk.h>
#include <linux/delay.h>
#include <linux/errno.h>

#include "driver_w25qxx.h"
#include "driver_w25qxx_interface.h"

int w25qxx_port_init(w25qxx_handle_t *handle)
{
	int ret;

	DRIVER_W25QXX_LINK_INIT(handle, w25qxx_handle_t);
	DRIVER_W25QXX_LINK_SPI_QSPI_INIT(handle, w25qxx_interface_spi_qspi_init);
	DRIVER_W25QXX_LINK_SPI_QSPI_DEINIT(handle, w25qxx_interface_spi_qspi_deinit);
	DRIVER_W25QXX_LINK_SPI_QSPI_WRITE_READ(handle, w25qxx_interface_spi_qspi_write_read);
	DRIVER_W25QXX_LINK_DELAY_MS(handle, w25qxx_interface_delay_ms);
	DRIVER_W25QXX_LINK_DELAY_US(handle, w25qxx_interface_delay_us);
	DRIVER_W25QXX_LINK_DEBUG_PRINT(handle, w25qxx_interface_debug_print);

	ret = w25qxx_set_type(handle, W25Q256);
	if (ret != 0) {
		pr_err("w25qxx: set type failed.\n");
		return -EIO;
	}

	ret = w25qxx_set_interface(handle, W25QXX_INTERFACE_QSPI);
	if (ret != 0) {
		pr_err("w25qxx: set interface failed.\n");
		return -EIO;
	}

	ret = w25qxx_init(handle);
	if (ret != 0) {
		pr_err("w25qxx: init failed.\n");
		return -EIO;
	}

	return 0;
}
