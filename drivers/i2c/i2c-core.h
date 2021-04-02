/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * i2c-core.h - interfaces internal to the I2C framework
 */

#include <linux/list.h>

struct i2c_devinfo {
	struct list_head	list;
	int			busnum;
	struct i2c_board_info	board_info;
};

/* board_lock protects board_list and first_dynamic_bus_num.
 * only i2c core components are allowed to use these symbols.
 */
extern struct list_head	__i2c_board_list;
extern int		__i2c_first_dynamic_bus_num;

int i2c_check_7bit_addr_validity_strict(unsigned short addr);

#ifdef CONFIG_ACPI
void i2c_acpi_register_devices(struct i2c_adapter *adap);

int i2c_acpi_get_irq(struct i2c_client *client);
#else /* CONFIG_ACPI */
static inline void i2c_acpi_register_devices(struct i2c_adapter *adap) { }

static inline int i2c_acpi_get_irq(struct i2c_client *client)
{
	return 0;
}
#endif /* CONFIG_ACPI */

#ifdef CONFIG_ACPI_I2C_OPREGION
int i2c_acpi_install_space_handler(struct i2c_adapter *adapter);
void i2c_acpi_remove_space_handler(struct i2c_adapter *adapter);
#else /* CONFIG_ACPI_I2C_OPREGION */
static inline int i2c_acpi_install_space_handler(struct i2c_adapter *adapter) { return 0; }
static inline void i2c_acpi_remove_space_handler(struct i2c_adapter *adapter) { }
#endif /* CONFIG_ACPI_I2C_OPREGION */

#ifdef CONFIG_OF
void of_i2c_register_devices(struct i2c_adapter *adap);
#else
static inline void of_i2c_register_devices(struct i2c_adapter *adap) { }
#endif
extern struct notifier_block i2c_of_notifier;
