// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Linux I2C core
 *
 * Copyright (C) 1995-99 Simon G. Vogl
 *   With some changes from Kyösti Mälkki <kmalkki@cc.hut.fi>
 *   Mux support by Rodolfo Giometti <giometti@enneenne.com> and
 *   Michael Lawnick <michael.lawnick.ext@nsn.com>
 *
 * Copyright (C) 2013-2017 Wolfram Sang <wsa@kernel.org>
 */

#define pr_fmt(fmt) "i2c-core: " fmt

#include <dt-bindings/i2c/i2c.h>
//#include <linux/acpi.h>
#include <linux/completion.h>
#include <linux/delay.h>
#include <linux/jiffies.h>
#include <linux/err.h>
#include <linux/errno.h>
#include <linux/gpio/consumer.h>
#include <linux/i2c.h>
#include <linux/i2c-smbus.h>
//#include <linux/idr.h>
#include <linux/init.h>
//#include <linux/irqflags.h>
#include <linux/kernel.h>
#include <linux/dev_printk.h>
//#include <linux/module.h>
//#include <linux/of_device.h>
//#include <linux/of.h>
//#include <linux/of_irq.h>
//#include <linux/pinctrl/consumer.h>
//#include <linux/pm_domain.h>
//#include <linux/pm_runtime.h>
//#include <linux/pm_wakeirq.h>
#include <linux/slab.h>

#include "i2c-core.h"

#include "kinetis/delay.h"

//#define CREATE_TRACE_POINTS
//#include <trace/events/i2c.h>

#define I2C_ADDR_OFFSET_TEN_BIT	0xa000
#define I2C_ADDR_OFFSET_SLAVE	0x1000

#define I2C_ADDR_7BITS_MAX	0x77
#define I2C_ADDR_7BITS_COUNT	(I2C_ADDR_7BITS_MAX + 1)

#define I2C_ADDR_DEVICE_ID	0x7c

/*
 * core_lock protects i2c_adapter_idr, and guarantees that device detection,
 * deletion of detected devices are serialized
 */
//static DEFINE_IDR(i2c_adapter_idr);

static int i2c_detect(struct i2c_adapter *adapter, struct i2c_driver *driver);

static bool is_registered;

const struct i2c_device_id *i2c_match_id(const struct i2c_device_id *id,
						const struct i2c_client *client)
{
	if (!(id && client))
		return NULL;

	while (id->name[0]) {
		if (strcmp(client->name, id->name) == 0)
			return id;
		id++;
	}
	return NULL;
}
EXPORT_SYMBOL_GPL(i2c_match_id);

//static int i2c_device_match(struct device *dev, struct device_driver *drv)
//{
//	struct i2c_client	*client = i2c_verify_client(dev);
//	struct i2c_driver	*driver;


//	/* Attempt an OF style match */
//	if (i2c_of_match_device(drv->of_match_table, client))
//		return 1;

//	/* Then ACPI style match */
//	if (acpi_driver_match_device(dev, drv))
//		return 1;

//	driver = to_i2c_driver(drv);

//	/* Finally an I2C match */
//	if (i2c_match_id(driver->id_table, client))
//		return 1;

//	return 0;
//}

/* i2c bus recovery routines */
static int get_scl_gpio_value(struct i2c_adapter *adap)
{
	return gpiod_get_value_cansleep(adap->bus_recovery_info->scl_gpiod);
}

static void set_scl_gpio_value(struct i2c_adapter *adap, int val)
{
	gpiod_set_value_cansleep(adap->bus_recovery_info->scl_gpiod, val);
}

static int get_sda_gpio_value(struct i2c_adapter *adap)
{
	return gpiod_get_value_cansleep(adap->bus_recovery_info->sda_gpiod);
}

static void set_sda_gpio_value(struct i2c_adapter *adap, int val)
{
	gpiod_set_value_cansleep(adap->bus_recovery_info->sda_gpiod, val);
}

static int i2c_generic_bus_free(struct i2c_adapter *adap)
{
	struct i2c_bus_recovery_info *bri = adap->bus_recovery_info;
	int ret = -EOPNOTSUPP;

	if (bri->get_bus_free)
		ret = bri->get_bus_free(adap);
	else if (bri->get_sda)
		ret = bri->get_sda(adap);

	if (ret < 0)
		return ret;

	return ret ? 0 : -EBUSY;
}

/*
 * We are generating clock pulses. ndelay() determines durating of clk pulses.
 * We will generate clock with rate 100 KHz and so duration of both clock levels
 * is: delay in us = (10^6 / 100000) / 2
 */
#define RECOVERY_NDELAY		5000
#define RECOVERY_CLK_CNT	9

int i2c_generic_scl_recovery(struct i2c_adapter *adap)
{
	struct i2c_bus_recovery_info *bri = adap->bus_recovery_info;
	int i = 0, scl = 1, ret = 0;

	if (bri->prepare_recovery)
		bri->prepare_recovery(adap);
//	if (bri->pinctrl)
//		pinctrl_select_state(bri->pinctrl, bri->pins_gpio);

	/*
	 * If we can set SDA, we will always create a STOP to ensure additional
	 * pulses will do no harm. This is achieved by letting SDA follow SCL
	 * half a cycle later. Check the 'incomplete_write_byte' fault injector
	 * for details. Note that we must honour tsu:sto, 4us, but lets use 5us
	 * here for simplicity.
	 */
	bri->set_scl(adap, scl);
	ndelay(RECOVERY_NDELAY);
	if (bri->set_sda)
		bri->set_sda(adap, scl);
	ndelay(RECOVERY_NDELAY / 2 + 1);

	/*
	 * By this time SCL is high, as we need to give 9 falling-rising edges
	 */
	while (i++ < RECOVERY_CLK_CNT * 2) {
		if (scl) {
			/* SCL shouldn't be low here */
			if (!bri->get_scl(adap)) {
				dev_err(&adap->dev,
					"SCL is stuck low, exit recovery\n");
				ret = -EBUSY;
				break;
			}
		}

		scl = !scl;
		bri->set_scl(adap, scl);
		/* Creating STOP again, see above */
		if (scl)  {
			/* Honour minimum tsu:sto */
			ndelay(RECOVERY_NDELAY);
		} else {
			/* Honour minimum tf and thd:dat */
			ndelay(RECOVERY_NDELAY / 2);
		}
		if (bri->set_sda)
			bri->set_sda(adap, scl);
		ndelay(RECOVERY_NDELAY / 2);

		if (scl) {
			ret = i2c_generic_bus_free(adap);
			if (ret == 0)
				break;
		}
	}

	/* If we can't check bus status, assume recovery worked */
	if (ret == -EOPNOTSUPP)
		ret = 0;

	if (bri->unprepare_recovery)
		bri->unprepare_recovery(adap);
//	if (bri->pinctrl)
//		pinctrl_select_state(bri->pinctrl, bri->pins_default);

	return ret;
}
EXPORT_SYMBOL_GPL(i2c_generic_scl_recovery);

int i2c_recover_bus(struct i2c_adapter *adap)
{
	if (!adap->bus_recovery_info)
		return -EOPNOTSUPP;

	dev_dbg(&adap->dev, "Trying i2c bus recovery\n");
	return adap->bus_recovery_info->recover_bus(adap);
}
EXPORT_SYMBOL_GPL(i2c_recover_bus);

static void i2c_gpio_init_pinctrl_recovery(struct i2c_adapter *adap)
{
	struct i2c_bus_recovery_info *bri = adap->bus_recovery_info;
	struct device *dev = &adap->dev;
	struct pinctrl *p = bri->pinctrl;

	/*
	 * we can't change states without pinctrl, so remove the states if
	 * populated
	 */
	if (!p) {
		bri->pins_default = NULL;
		bri->pins_gpio = NULL;
		return;
	}

//	if (!bri->pins_default) {
//		bri->pins_default = pinctrl_lookup_state(p,
//							 PINCTRL_STATE_DEFAULT);
//		if (IS_ERR(bri->pins_default)) {
//			dev_dbg(dev, PINCTRL_STATE_DEFAULT " state not found for GPIO recovery\n");
//			bri->pins_default = NULL;
//		}
//	}
//	if (!bri->pins_gpio) {
//		bri->pins_gpio = pinctrl_lookup_state(p, "gpio");
//		if (IS_ERR(bri->pins_gpio))
//			bri->pins_gpio = pinctrl_lookup_state(p, "recovery");

//		if (IS_ERR(bri->pins_gpio)) {
//			dev_dbg(dev, "no gpio or recovery state found for GPIO recovery\n");
//			bri->pins_gpio = NULL;
//		}
//	}

	/* for pinctrl state changes, we need all the information */
	if (bri->pins_default && bri->pins_gpio) {
		dev_info(dev, "using pinctrl states for GPIO recovery");
	} else {
		bri->pinctrl = NULL;
		bri->pins_default = NULL;
		bri->pins_gpio = NULL;
	}
}

static int i2c_gpio_init_generic_recovery(struct i2c_adapter *adap)
{
	struct i2c_bus_recovery_info *bri = adap->bus_recovery_info;
	struct device *dev = &adap->dev;
	struct gpio_desc *gpiod;
	int ret = 0;

	/*
	 * don't touch the recovery information if the driver is not using
	 * generic SCL recovery
	 */
	if (bri->recover_bus && bri->recover_bus != i2c_generic_scl_recovery)
		return 0;

	/*
	 * pins might be taken as GPIO, so we should inform pinctrl about
	 * this and move the state to GPIO
	 */
//	if (bri->pinctrl)
//		pinctrl_select_state(bri->pinctrl, bri->pins_gpio);

	/*
	 * if there is incomplete or no recovery information, see if generic
	 * GPIO recovery is available
	 */
	if (!bri->scl_gpiod) {
		gpiod = devm_gpiod_get(dev, "scl", GPIOD_OUT_HIGH_OPEN_DRAIN);
		if (PTR_ERR(gpiod) == -EPROBE_DEFER) {
			ret  = -EPROBE_DEFER;
			goto cleanup_pinctrl_state;
		}
		if (!IS_ERR(gpiod)) {
			bri->scl_gpiod = gpiod;
			bri->recover_bus = i2c_generic_scl_recovery;
			dev_info(dev, "using generic GPIOs for recovery\n");
		}
	}

	/* SDA GPIOD line is optional, so we care about DEFER only */
	if (!bri->sda_gpiod) {
		/*
		 * We have SCL. Pull SCL low and wait a bit so that SDA glitches
		 * have no effect.
		 */
		gpiod_direction_output(bri->scl_gpiod, 0);
		udelay(10);
		gpiod = devm_gpiod_get(dev, "sda", GPIOD_IN);

		/* Wait a bit in case of a SDA glitch, and then release SCL. */
		udelay(10);
		gpiod_direction_output(bri->scl_gpiod, 1);

		if (PTR_ERR(gpiod) == -EPROBE_DEFER) {
			ret = -EPROBE_DEFER;
			goto cleanup_pinctrl_state;
		}
		if (!IS_ERR(gpiod))
			bri->sda_gpiod = gpiod;
	}

cleanup_pinctrl_state:
	/* change the state of the pins back to their default state */
//	if (bri->pinctrl)
//		pinctrl_select_state(bri->pinctrl, bri->pins_default);

	return ret;
}

static int i2c_gpio_init_recovery(struct i2c_adapter *adap)
{
	i2c_gpio_init_pinctrl_recovery(adap);
	return i2c_gpio_init_generic_recovery(adap);
}

static int i2c_init_recovery(struct i2c_adapter *adap)
{
	struct i2c_bus_recovery_info *bri = adap->bus_recovery_info;
	char *err_str;

	if (!bri)
		return 0;

	if (i2c_gpio_init_recovery(adap) == -EPROBE_DEFER)
		return -EPROBE_DEFER;

	if (!bri->recover_bus) {
		err_str = "no recover_bus() found";
		goto err;
	}

	if (bri->scl_gpiod && bri->recover_bus == i2c_generic_scl_recovery) {
		bri->get_scl = get_scl_gpio_value;
		bri->set_scl = set_scl_gpio_value;
		if (bri->sda_gpiod) {
			bri->get_sda = get_sda_gpio_value;
			/* FIXME: add proper flag instead of '0' once available */
			if (gpiod_get_direction(bri->sda_gpiod) == 0)
				bri->set_sda = set_sda_gpio_value;
		}
	} else if (bri->recover_bus == i2c_generic_scl_recovery) {
		/* Generic SCL recovery */
		if (!bri->set_scl || !bri->get_scl) {
			err_str = "no {get|set}_scl() found";
			goto err;
		}
		if (!bri->set_sda && !bri->get_sda) {
			err_str = "either get_sda() or set_sda() needed";
			goto err;
		}
	}

	return 0;
 err:
	dev_err(&adap->dev, "Not using recovery: %s\n", err_str);
	adap->bus_recovery_info = NULL;

	return -EINVAL;
}

static void i2c_client_dev_release(struct device *dev)
{
	kfree(to_i2c_client(dev));
}

struct device_type i2c_client_type = {
	.groups		= NULL,
	.release	= i2c_client_dev_release,
};
EXPORT_SYMBOL_GPL(i2c_client_type);

/**
 * i2c_verify_client - return parameter as i2c_client, or NULL
 * @dev: device, probably from some driver model iterator
 *
 * When traversing the driver model tree, perhaps using driver model
 * iterators like @device_for_each_child(), you can't assume very much
 * about the nodes you find.  Use this function to avoid oopses caused
 * by wrongly treating some non-I2C device as an i2c_client.
 */
struct i2c_client *i2c_verify_client(struct device *dev)
{
	return (dev->type == &i2c_client_type)
			? to_i2c_client(dev)
			: NULL;
}
EXPORT_SYMBOL(i2c_verify_client);


/* Return a unique address which takes the flags of the client into account */
static unsigned short i2c_encode_flags_to_addr(struct i2c_client *client)
{
	unsigned short addr = client->addr;

	/* For some client flags, add an arbitrary offset to avoid collisions */
	if (client->flags & I2C_CLIENT_TEN)
		addr |= I2C_ADDR_OFFSET_TEN_BIT;

	if (client->flags & I2C_CLIENT_SLAVE)
		addr |= I2C_ADDR_OFFSET_SLAVE;

	return addr;
}

/* This is a permissive address validity check, I2C address map constraints
 * are purposely not enforced, except for the general call address. */
static int i2c_check_addr_validity(unsigned int addr, unsigned short flags)
{
	if (flags & I2C_CLIENT_TEN) {
		/* 10-bit address, all values are valid */
		if (addr > 0x3ff)
			return -EINVAL;
	} else {
		/* 7-bit address, reject the general call address */
		if (addr == 0x00 || addr > 0x7f)
			return -EINVAL;
	}
	return 0;
}

/* And this is a strict address validity check, used when probing. If a
 * device uses a reserved address, then it shouldn't be probed. 7-bit
 * addressing is assumed, 10-bit address devices are rare and should be
 * explicitly enumerated. */
int i2c_check_7bit_addr_validity_strict(unsigned short addr)
{
	/*
	 * Reserved addresses per I2C specification:
	 *  0x00       General call address / START byte
	 *  0x01       CBUS address
	 *  0x02       Reserved for different bus format
	 *  0x03       Reserved for future purposes
	 *  0x04-0x07  Hs-mode master code
	 *  0x78-0x7b  10-bit slave addressing
	 *  0x7c-0x7f  Reserved for future purposes
	 */
	if (addr < 0x08 || addr > 0x77)
		return -EINVAL;
	return 0;
}

static int __i2c_check_addr_busy(struct device *dev, void *addrp)
{
	struct i2c_client	*client = i2c_verify_client(dev);
	int			addr = *(int *)addrp;

	if (client && i2c_encode_flags_to_addr(client) == addr)
		return -EBUSY;
	return 0;
}

/* walk up mux tree */
static int i2c_check_mux_parents(struct i2c_adapter *adapter, int addr)
{
	struct i2c_adapter *parent = i2c_parent_is_i2c_adapter(adapter);
	int result;

	result = device_for_each_child(&adapter->dev, &addr,
					__i2c_check_addr_busy);

	if (!result && parent)
		result = i2c_check_mux_parents(parent, addr);

	return result;
}

/* recurse down mux tree */
static int i2c_check_mux_children(struct device *dev, void *addrp)
{
	int result;

	if (dev->type == &i2c_adapter_type)
		result = device_for_each_child(dev, addrp,
						i2c_check_mux_children);
	else
		result = __i2c_check_addr_busy(dev, addrp);

	return result;
}

static int i2c_check_addr_busy(struct i2c_adapter *adapter, int addr)
{
	struct i2c_adapter *parent = i2c_parent_is_i2c_adapter(adapter);
	int result = 0;

	if (parent)
		result = i2c_check_mux_parents(parent, addr);

	if (!result)
		result = device_for_each_child(&adapter->dev, &addr,
						i2c_check_mux_children);

	return result;
}

static void i2c_dev_set_name(struct i2c_adapter *adap,
			     struct i2c_client *client,
			     struct i2c_board_info const *info)
{
	if (info && info->dev_name) {
		dev_set_name(&client->dev, "i2c-%s", info->dev_name);
		return;
	}

	dev_set_name(&client->dev, "%d-%04x", i2c_adapter_id(adap),
		     i2c_encode_flags_to_addr(client));
}

/**
 * i2c_new_client_device - instantiate an i2c device
 * @adap: the adapter managing the device
 * @info: describes one I2C device; bus_num is ignored
 * Context: can sleep
 *
 * Create an i2c device. Binding is handled through driver model
 * probe()/remove() methods.  A driver may be bound to this device when we
 * return from this function, or any later moment (e.g. maybe hotplugging will
 * load the driver module).  This call is not appropriate for use by mainboard
 * initialization logic, which usually runs during an arch_initcall() long
 * before any i2c_adapter could exist.
 *
 * This returns the new i2c client, which may be saved for later use with
 * i2c_unregister_device(); or an ERR_PTR to describe the error.
 */
struct i2c_client *
i2c_new_client_device(struct i2c_adapter *adap, struct i2c_board_info const *info)
{
	struct i2c_client	*client;
	int			status;

	client = kzalloc(sizeof *client, GFP_KERNEL);
	if (!client)
		return ERR_PTR(-ENOMEM);

	client->adapter = adap;

	client->dev.platform_data = info->platform_data;
	client->flags = info->flags;
	client->addr = info->addr;

	client->init_irq = info->irq;

	strlcpy(client->name, info->type, sizeof(client->name));

	status = i2c_check_addr_validity(client->addr, client->flags);
	if (status) {
		dev_err(&adap->dev, "Invalid %d-bit I2C address 0x%02hx\n",
			client->flags & I2C_CLIENT_TEN ? 10 : 7, client->addr);
		goto out_err_silent;
	}

	/* Check for address business */
	status = i2c_check_addr_busy(adap, i2c_encode_flags_to_addr(client));
	if (status)
		goto out_err;

	client->dev.parent = &client->adapter->dev;
	client->dev.type = &i2c_client_type;

	i2c_dev_set_name(adap, client, info);

	dev_dbg(&adap->dev, "client [%s] registered with bus id %s\n",
		client->name, dev_name(&client->dev));

	return client;

out_err:
	dev_err(&adap->dev,
		"Failed to register i2c client %s at 0x%02x (%d)\n",
		client->name, client->addr, status);
out_err_silent:
	kfree(client);
	return ERR_PTR(status);
}
EXPORT_SYMBOL_GPL(i2c_new_client_device);

/**
 * i2c_unregister_device - reverse effect of i2c_new_*_device()
 * @client: value returned from i2c_new_*_device()
 * Context: can sleep
 */
void i2c_unregister_device(struct i2c_client *client)
{
	if (IS_ERR_OR_NULL(client))
		return;
}
EXPORT_SYMBOL_GPL(i2c_unregister_device);


static const struct i2c_device_id dummy_id[] = {
	{ "dummy", 0 },
	{ },
};

static int dummy_probe(struct i2c_client *client,
		       const struct i2c_device_id *id)
{
	return 0;
}

static int dummy_remove(struct i2c_client *client)
{
	return 0;
}

static struct i2c_driver dummy_driver = {
	.probe		= dummy_probe,
	.remove		= dummy_remove,
	.id_table	= dummy_id,
};

/**
 * i2c_new_dummy_device - return a new i2c device bound to a dummy driver
 * @adapter: the adapter managing the device
 * @address: seven bit address to be used
 * Context: can sleep
 *
 * This returns an I2C client bound to the "dummy" driver, intended for use
 * with devices that consume multiple addresses.  Examples of such chips
 * include various EEPROMS (like 24c04 and 24c08 models).
 *
 * These dummy devices have two main uses.  First, most I2C and SMBus calls
 * except i2c_transfer() need a client handle; the dummy will be that handle.
 * And second, this prevents the specified address from being bound to a
 * different driver.
 *
 * This returns the new i2c client, which should be saved for later use with
 * i2c_unregister_device(); or an ERR_PTR to describe the error.
 */
struct i2c_client *i2c_new_dummy_device(struct i2c_adapter *adapter, u16 address)
{
	struct i2c_board_info info = {
		I2C_BOARD_INFO("dummy", address),
	};

	return i2c_new_client_device(adapter, &info);
}
EXPORT_SYMBOL_GPL(i2c_new_dummy_device);

struct i2c_dummy_devres {
	struct i2c_client *client;
};

static void devm_i2c_release_dummy(struct device *dev, void *res)
{
	struct i2c_dummy_devres *this = res;

	i2c_unregister_device(this->client);
}

/**
 * devm_i2c_new_dummy_device - return a new i2c device bound to a dummy driver
 * @dev: device the managed resource is bound to
 * @adapter: the adapter managing the device
 * @address: seven bit address to be used
 * Context: can sleep
 *
 * This is the device-managed version of @i2c_new_dummy_device. It returns the
 * new i2c client or an ERR_PTR in case of an error.
 */
struct i2c_client *devm_i2c_new_dummy_device(struct device *dev,
					     struct i2c_adapter *adapter,
					     u16 address)
{
	struct i2c_dummy_devres *dr;
	struct i2c_client *client;

	dr = kmalloc(sizeof(*dr), GFP_KERNEL);
	if (!dr)
		return ERR_PTR(-ENOMEM);

	client = i2c_new_dummy_device(adapter, address);
	if (IS_ERR(client)) {
		kfree(dr);
	} else {
		dr->client = client;
//		devres_add(dev, dr);
	}

	return client;
}
EXPORT_SYMBOL_GPL(devm_i2c_new_dummy_device);

/**
 * i2c_new_ancillary_device - Helper to get the instantiated secondary address
 * and create the associated device
 * @client: Handle to the primary client
 * @name: Handle to specify which secondary address to get
 * @default_addr: Used as a fallback if no secondary address was specified
 * Context: can sleep
 *
 * I2C clients can be composed of multiple I2C slaves bound together in a single
 * component. The I2C client driver then binds to the master I2C slave and needs
 * to create I2C dummy clients to communicate with all the other slaves.
 *
 * This function creates and returns an I2C dummy client whose I2C address is
 * retrieved from the platform firmware based on the given slave name. If no
 * address is specified by the firmware default_addr is used.
 *
 * On DT-based platforms the address is retrieved from the "reg" property entry
 * cell whose "reg-names" value matches the slave name.
 *
 * This returns the new i2c client, which should be saved for later use with
 * i2c_unregister_device(); or an ERR_PTR to describe the error.
 */
struct i2c_client *i2c_new_ancillary_device(struct i2c_client *client,
						const char *name,
						u16 default_addr)
{
	u32 addr = default_addr;
	int i;

	dev_dbg(&client->adapter->dev, "Address for %s : 0x%x\n", name, addr);
	return i2c_new_dummy_device(client->adapter, addr);
}
EXPORT_SYMBOL_GPL(i2c_new_ancillary_device);

/* ------------------------------------------------------------------------- */

/* I2C bus adapters -- one roots each I2C or SMBUS segment */

static void i2c_adapter_dev_release(struct device *dev)
{
	struct i2c_adapter *adap = to_i2c_adapter(dev);
	complete(&adap->dev_released);
}

unsigned int i2c_adapter_depth(struct i2c_adapter *adapter)
{
	unsigned int depth = 0;

	while ((adapter = i2c_parent_is_i2c_adapter(adapter)))
		depth++;

	WARN_ONCE(depth >= MAX_LOCKDEP_SUBCLASSES,
		  "adapter depth exceeds lockdep subclass limit\n");

	return depth;
}
EXPORT_SYMBOL_GPL(i2c_adapter_depth);

///*
// * Let users instantiate I2C devices through sysfs. This can be used when
// * platform initialization code doesn't contain the proper data for
// * whatever reason. Also useful for drivers that do device detection and
// * detection fails, either because the device uses an unexpected address,
// * or this is a compatible device with different ID register values.
// *
// * Parameter checking may look overzealous, but we really don't want
// * the user to provide incorrect parameters.
// */
//static ssize_t
//new_device_store(struct device *dev, struct device_attribute *attr,
//		 const char *buf, size_t count)
//{
//	struct i2c_adapter *adap = to_i2c_adapter(dev);
//	struct i2c_board_info info;
//	struct i2c_client *client;
//	char *blank, end;
//	int res;

//	memset(&info, 0, sizeof(struct i2c_board_info));

//	blank = strchr(buf, ' ');
//	if (!blank) {
//		dev_err(dev, "%s: Missing parameters\n", "new_device");
//		return -EINVAL;
//	}
//	if (blank - buf > I2C_NAME_SIZE - 1) {
//		dev_err(dev, "%s: Invalid device name\n", "new_device");
//		return -EINVAL;
//	}
//	memcpy(info.type, buf, blank - buf);

//	/* Parse remaining parameters, reject extra parameters */
//	res = sscanf(++blank, "%hi%c", &info.addr, &end);
//	if (res < 1) {
//		dev_err(dev, "%s: Can't parse I2C address\n", "new_device");
//		return -EINVAL;
//	}
//	if (res > 1  && end != '\n') {
//		dev_err(dev, "%s: Extra parameters\n", "new_device");
//		return -EINVAL;
//	}

//	if ((info.addr & I2C_ADDR_OFFSET_TEN_BIT) == I2C_ADDR_OFFSET_TEN_BIT) {
//		info.addr &= ~I2C_ADDR_OFFSET_TEN_BIT;
//		info.flags |= I2C_CLIENT_TEN;
//	}

//	if (info.addr & I2C_ADDR_OFFSET_SLAVE) {
//		info.addr &= ~I2C_ADDR_OFFSET_SLAVE;
//		info.flags |= I2C_CLIENT_SLAVE;
//	}

//	client = i2c_new_client_device(adap, &info);
//	if (IS_ERR(client))
//		return PTR_ERR(client);

//	/* Keep track of the added device */
//	list_add_tail(&client->detected, &adap->userspace_clients);
//	dev_info(dev, "%s: Instantiated device %s at 0x%02hx\n", "new_device",
//		 info.type, info.addr);

//	return count;
//}
//static DEVICE_ATTR_WO(new_device);

///*
// * And of course let the users delete the devices they instantiated, if
// * they got it wrong. This interface can only be used to delete devices
// * instantiated by i2c_sysfs_new_device above. This guarantees that we
// * don't delete devices to which some kernel code still has references.
// *
// * Parameter checking may look overzealous, but we really don't want
// * the user to delete the wrong device.
// */
//static ssize_t
//delete_device_store(struct device *dev, struct device_attribute *attr,
//		    const char *buf, size_t count)
//{
//	struct i2c_adapter *adap = to_i2c_adapter(dev);
//	struct i2c_client *client, *next;
//	unsigned short addr;
//	char end;
//	int res;

//	/* Parse parameters, reject extra parameters */
//	res = sscanf(buf, "%hi%c", &addr, &end);
//	if (res < 1) {
//		dev_err(dev, "%s: Can't parse I2C address\n", "delete_device");
//		return -EINVAL;
//	}
//	if (res > 1  && end != '\n') {
//		dev_err(dev, "%s: Extra parameters\n", "delete_device");
//		return -EINVAL;
//	}

//	/* Make sure the device was added through sysfs */
//	res = -ENOENT;
//	list_for_each_entry_safe(client, next, &adap->userspace_clients,
//				 detected) {
//		if (i2c_encode_flags_to_addr(client) == addr) {
//			dev_info(dev, "%s: Deleting device %s at 0x%02hx\n",
//				 "delete_device", client->name, client->addr);

//			list_del(&client->detected);
//			i2c_unregister_device(client);
//			res = count;
//			break;
//		}
//	}

//	if (res < 0)
//		dev_err(dev, "%s: Can't find device in list\n",
//			"delete_device");
//	return res;
//}

//static struct attribute *i2c_adapter_attrs[] = {
//	&dev_attr_name.attr,
//	&dev_attr_new_device.attr,
//	&dev_attr_delete_device.attr,
//	NULL
//};
//ATTRIBUTE_GROUPS(i2c_adapter);

struct device_type i2c_adapter_type = {
//	.groups		= i2c_adapter_groups,
	.release	= i2c_adapter_dev_release,
};
EXPORT_SYMBOL_GPL(i2c_adapter_type);

/**
 * i2c_verify_adapter - return parameter as i2c_adapter or NULL
 * @dev: device, probably from some driver model iterator
 *
 * When traversing the driver model tree, perhaps using driver model
 * iterators like @device_for_each_child(), you can't assume very much
 * about the nodes you find.  Use this function to avoid oopses caused
 * by wrongly treating some non-I2C device as an i2c_adapter.
 */
struct i2c_adapter *i2c_verify_adapter(struct device *dev)
{
	return (dev->type == &i2c_adapter_type)
			? to_i2c_adapter(dev)
			: NULL;
}
EXPORT_SYMBOL(i2c_verify_adapter);

#ifdef CONFIG_I2C_COMPAT
static struct class_compat *i2c_adapter_compat_class;
#endif

static void i2c_scan_static_board_info(struct i2c_adapter *adapter)
{
	struct i2c_devinfo	*devinfo;

	list_for_each_entry(devinfo, &__i2c_board_list, list) {
		if (devinfo->busnum == adapter->nr &&
		    IS_ERR(i2c_new_client_device(adapter, &devinfo->board_info)))
			dev_err(&adapter->dev,
				"Can't create device at 0x%02x\n",
				devinfo->board_info.addr);
	}
}

static int i2c_do_add_adapter(struct i2c_driver *driver,
			      struct i2c_adapter *adap)
{
	/* Detect supported devices on that bus, and instantiate them */
	i2c_detect(adap, driver);

	return 0;
}

//static int __process_new_adapter(struct device_driver *d, void *data)
//{
//	return i2c_do_add_adapter(to_i2c_driver(d), data);
//}

static int i2c_register_adapter(struct i2c_adapter *adap)
{
	int res = -EINVAL;

	/* Can't register until after driver model init */
	if (WARN_ON(!is_registered)) {
		res = -EAGAIN;
		goto out_list;
	}

	/* Sanity checks */
	if (WARN(!adap->name[0], "i2c adapter has no name"))
		goto out_list;

	if (!adap->algo) {
		pr_err("adapter '%s': no algo supplied!\n", adap->name);
		goto out_list;
	}

	adap->locked_flags = 0;
	INIT_LIST_HEAD(&adap->userspace_clients);

	/* Set default timeout to 1 second if not already set */
	if (adap->timeout == 0)
		adap->timeout = HZ;

	dev_set_name(&adap->dev, "i2c-%d", adap->nr);
	adap->dev.type = &i2c_adapter_type;

	res = i2c_init_recovery(adap);
	if (res == -EPROBE_DEFER)
		goto out_reg;

	dev_dbg(&adap->dev, "adapter [%s] registered\n", adap->name);

#ifdef CONFIG_I2C_COMPAT
	res = class_compat_create_link(i2c_adapter_compat_class, &adap->dev,
				       adap->dev.parent);
	if (res)
		dev_warn(&adap->dev,
			 "Failed to create compatibility class link\n");
#endif

	/* create pre-declared device nodes */
	of_i2c_register_devices(adap);
	i2c_acpi_install_space_handler(adap);
	i2c_acpi_register_devices(adap);

	if (adap->nr < __i2c_first_dynamic_bus_num)
		i2c_scan_static_board_info(adap);

	/* Notify drivers */
//	bus_for_each_drv(&i2c_bus_type, NULL, adap, __process_new_adapter);

	return 0;

out_reg:
	init_completion(&adap->dev_released);
	wait_for_completion(&adap->dev_released);
out_list:
//	idr_remove(&i2c_adapter_idr, adap->nr);
	return res;
}

/**
 * __i2c_add_numbered_adapter - i2c_add_numbered_adapter where nr is never -1
 * @adap: the adapter to register (with adap->nr initialized)
 * Context: can sleep
 *
 * See i2c_add_numbered_adapter() for details.
 */
static int __i2c_add_numbered_adapter(struct i2c_adapter *adap)
{
	int id;

	id = 1;//idr_alloc(&i2c_adapter_idr, adap, adap->nr, adap->nr + 1, GFP_KERNEL);
	if (WARN(id < 0, "couldn't get idr"))
		return id == -ENOSPC ? -EBUSY : id;

	return i2c_register_adapter(adap);
}

/**
 * i2c_add_adapter - declare i2c adapter, use dynamic bus number
 * @adapter: the adapter to add
 * Context: can sleep
 *
 * This routine is used to declare an I2C adapter when its bus number
 * doesn't matter or when its bus number is specified by an dt alias.
 * Examples of bases when the bus number doesn't matter: I2C adapters
 * dynamically added by USB links or PCI plugin cards.
 *
 * When this returns zero, a new bus number was allocated and stored
 * in adap->nr, and the specified adapter became available for clients.
 * Otherwise, a negative errno value is returned.
 */
int i2c_add_adapter(struct i2c_adapter *adapter)
{
	struct device *dev = &adapter->dev;
	int id = 0;

//	if (dev->of_node) {
//		id = of_alias_get_id(dev->of_node, "i2c");
//		if (id >= 0) {
//			adapter->nr = id;
//			return __i2c_add_numbered_adapter(adapter);
//		}
//	}
//
//	id = idr_alloc(&i2c_adapter_idr, adapter,
//		       __i2c_first_dynamic_bus_num, 0, GFP_KERNEL);
//	if (WARN(id < 0, "couldn't get idr"))
//		return id;

	adapter->nr = id;

	return i2c_register_adapter(adapter);
}
EXPORT_SYMBOL(i2c_add_adapter);

/**
 * i2c_add_numbered_adapter - declare i2c adapter, use static bus number
 * @adap: the adapter to register (with adap->nr initialized)
 * Context: can sleep
 *
 * This routine is used to declare an I2C adapter when its bus number
 * matters.  For example, use it for I2C adapters from system-on-chip CPUs,
 * or otherwise built in to the system's mainboard, and where i2c_board_info
 * is used to properly configure I2C devices.
 *
 * If the requested bus number is set to -1, then this function will behave
 * identically to i2c_add_adapter, and will dynamically assign a bus number.
 *
 * If no devices have pre-been declared for this bus, then be sure to
 * register the adapter before any dynamically allocated ones.  Otherwise
 * the required bus ID may not be available.
 *
 * When this returns zero, the specified adapter became available for
 * clients using the bus number provided in adap->nr.  Also, the table
 * of I2C devices pre-declared using i2c_register_board_info() is scanned,
 * and the appropriate driver model device nodes are created.  Otherwise, a
 * negative errno value is returned.
 */
int i2c_add_numbered_adapter(struct i2c_adapter *adap)
{
	if (adap->nr == -1) /* -1 means dynamically assign bus id */
		return i2c_add_adapter(adap);

	return __i2c_add_numbered_adapter(adap);
}
EXPORT_SYMBOL_GPL(i2c_add_numbered_adapter);

static void i2c_do_del_adapter(struct i2c_driver *driver,
			      struct i2c_adapter *adapter)
{
	struct i2c_client *client, *_n;

	/* Remove the devices we created ourselves as the result of hardware
	 * probing (using a driver's detect method) */
	list_for_each_entry_safe(client, _n, &driver->clients, detected) {
		if (client->adapter == adapter) {
			dev_dbg(&adapter->dev, "Removing %s at 0x%x\n",
				client->name, client->addr);
			list_del(&client->detected);
			i2c_unregister_device(client);
		}
	}
}

static int __unregister_client(struct device *dev, void *dummy)
{
	struct i2c_client *client = i2c_verify_client(dev);
	if (client && strcmp(client->name, "dummy"))
		i2c_unregister_device(client);
	return 0;
}

static int __unregister_dummy(struct device *dev, void *dummy)
{
	struct i2c_client *client = i2c_verify_client(dev);
	i2c_unregister_device(client);
	return 0;
}

//static int __process_removed_adapter(struct device_driver *d, void *data)
//{
//	i2c_do_del_adapter(to_i2c_driver(d), data);
//	return 0;
//}

/**
 * i2c_del_adapter - unregister I2C adapter
 * @adap: the adapter being unregistered
 * Context: can sleep
 *
 * This unregisters an I2C adapter which was previously registered
 * by @i2c_add_adapter or @i2c_add_numbered_adapter.
 */
void i2c_del_adapter(struct i2c_adapter *adap)
{
	struct i2c_adapter *found;
	struct i2c_client *client, *next;

	/* First make sure that this adapter was ever added */
//	found = idr_find(&i2c_adapter_idr, adap->nr);
//	if (found != adap) {
//		pr_debug("attempting to delete unregistered adapter [%s]\n", adap->name);
//		return;
//	}

//	i2c_acpi_remove_space_handler(adap);
//	/* Tell drivers about this removal */
//	bus_for_each_drv(&i2c_bus_type, NULL, adap,
//			       __process_removed_adapter);

	/* Remove devices instantiated from sysfs */
	list_for_each_entry_safe(client, next, &adap->userspace_clients,
				 detected) {
		dev_dbg(&adap->dev, "Removing %s at 0x%x\n", client->name,
			client->addr);
		list_del(&client->detected);
		i2c_unregister_device(client);
	}

#ifdef CONFIG_I2C_COMPAT
	class_compat_remove_link(i2c_adapter_compat_class, &adap->dev,
				 adap->dev.parent);
#endif

	/* device name is gone after device_unregister */
	dev_dbg(&adap->dev, "adapter [%s] unregistered\n", adap->name);

	/* wait until all references to the device are gone
	 *
	 * FIXME: This is old code and should ideally be replaced by an
	 * alternative which results in decoupling the lifetime of the struct
	 * device from the i2c_adapter, like spi or netdev do. Any solution
	 * should be thoroughly tested with DEBUG_KOBJECT_RELEASE enabled!
	 */
	init_completion(&adap->dev_released);
	wait_for_completion(&adap->dev_released);

	/* free bus id */
//	idr_remove(&i2c_adapter_idr, adap->nr);

	/* Clear the device structure in case this adapter is ever going to be
	   added again */
	memset(&adap->dev, 0, sizeof(adap->dev));
}
EXPORT_SYMBOL(i2c_del_adapter);

static void i2c_parse_timing(struct device *dev, char *prop_name, u32 *cur_val_p,
			    u32 def_val, bool use_def)
{
//	int ret;

//	ret = device_property_read_u32(dev, prop_name, cur_val_p);
//	if (ret && use_def)
//		*cur_val_p = def_val;

//	dev_dbg(dev, "%s: %u\n", prop_name, *cur_val_p);
}

/**
 * i2c_parse_fw_timings - get I2C related timing parameters from firmware
 * @dev: The device to scan for I2C timing properties
 * @t: the i2c_timings struct to be filled with values
 * @use_defaults: bool to use sane defaults derived from the I2C specification
 *		  when properties are not found, otherwise don't update
 *
 * Scan the device for the generic I2C properties describing timing parameters
 * for the signal and fill the given struct with the results. If a property was
 * not found and use_defaults was true, then maximum timings are assumed which
 * are derived from the I2C specification. If use_defaults is not used, the
 * results will be as before, so drivers can apply their own defaults before
 * calling this helper. The latter is mainly intended for avoiding regressions
 * of existing drivers which want to switch to this function. New drivers
 * almost always should use the defaults.
 */
void i2c_parse_fw_timings(struct device *dev, struct i2c_timings *t, bool use_defaults)
{
	bool u = use_defaults;
	u32 d;

	i2c_parse_timing(dev, "clock-frequency", &t->bus_freq_hz,
			 I2C_MAX_STANDARD_MODE_FREQ, u);

	d = t->bus_freq_hz <= I2C_MAX_STANDARD_MODE_FREQ ? 1000 :
	    t->bus_freq_hz <= I2C_MAX_FAST_MODE_FREQ ? 300 : 120;
	i2c_parse_timing(dev, "i2c-scl-rising-time-ns", &t->scl_rise_ns, d, u);

	d = t->bus_freq_hz <= I2C_MAX_FAST_MODE_FREQ ? 300 : 120;
	i2c_parse_timing(dev, "i2c-scl-falling-time-ns", &t->scl_fall_ns, d, u);

	i2c_parse_timing(dev, "i2c-scl-internal-delay-ns",
			 &t->scl_int_delay_ns, 0, u);
	i2c_parse_timing(dev, "i2c-sda-falling-time-ns", &t->sda_fall_ns,
			 t->scl_fall_ns, u);
	i2c_parse_timing(dev, "i2c-sda-hold-time-ns", &t->sda_hold_ns, 0, u);
	i2c_parse_timing(dev, "i2c-digital-filter-width-ns",
			 &t->digital_filter_width_ns, 0, u);
	i2c_parse_timing(dev, "i2c-analog-filter-cutoff-frequency",
			 &t->analog_filter_cutoff_freq_hz, 0, u);
}
EXPORT_SYMBOL_GPL(i2c_parse_fw_timings);

/* ------------------------------------------------------------------------- */

int i2c_for_each_dev(void *data, int (*fn)(struct device *dev, void *data))
{
	int res;

//	res = bus_for_each_dev(&i2c_bus_type, NULL, data, fn);

	return res;
}
EXPORT_SYMBOL_GPL(i2c_for_each_dev);

static int __process_new_driver(struct device *dev, void *data)
{
	if (dev->type != &i2c_adapter_type)
		return 0;
	return i2c_do_add_adapter(data, to_i2c_adapter(dev));
}

/*
 * An i2c_driver is used with one or more i2c_client (device) nodes to access
 * i2c slave chips, on a bus instance associated with some i2c_adapter.
 */

int i2c_register_driver(struct module *owner, struct i2c_driver *driver)
{
	int res;

	/* Can't register until after driver model init */
	if (WARN_ON(!is_registered))
		return -EAGAIN;

	/* add the driver to the list of i2c drivers in the driver core */
//	driver->driver.owner = owner;
//	driver->driver.bus = &i2c_bus_type;
	INIT_LIST_HEAD(&driver->clients);

	/* When registration returns, the driver core
	 * will have called probe() for all matching-but-unbound devices.
	 */
//	res = driver_register(&driver->driver);
//	if (res)
//		return res;

//	pr_debug("driver [%s] registered\n", driver->driver.name);

	/* Walk the adapters that are already present */
	i2c_for_each_dev(driver, __process_new_driver);

	return 0;
}
EXPORT_SYMBOL(i2c_register_driver);

static int __process_removed_driver(struct device *dev, void *data)
{
	if (dev->type == &i2c_adapter_type)
		i2c_do_del_adapter(data, to_i2c_adapter(dev));
	return 0;
}

/**
 * i2c_del_driver - unregister I2C driver
 * @driver: the driver being unregistered
 * Context: can sleep
 */
void i2c_del_driver(struct i2c_driver *driver)
{
	i2c_for_each_dev(driver, __process_removed_driver);

//	driver_unregister(&driver->driver);
//	pr_debug("driver [%s] unregistered\n", driver->driver.name);
}
EXPORT_SYMBOL(i2c_del_driver);

/* ------------------------------------------------------------------------- */

struct i2c_cmd_arg {
	unsigned	cmd;
	void		*arg;
};

static int i2c_cmd(struct device *dev, void *_arg)
{
	struct i2c_client	*client = i2c_verify_client(dev);
	struct i2c_cmd_arg	*arg = _arg;
	struct i2c_driver	*driver;

//	if (!client || !client->dev.driver)
//		return 0;

//	driver = to_i2c_driver(client->dev.driver);
	if (driver->command)
		driver->command(client, arg->cmd, arg->arg);
	return 0;
}

void i2c_clients_command(struct i2c_adapter *adap, unsigned int cmd, void *arg)
{
	struct i2c_cmd_arg	cmd_arg;

	cmd_arg.cmd = cmd;
	cmd_arg.arg = arg;
	device_for_each_child(&adap->dev, &cmd_arg, i2c_cmd);
}
EXPORT_SYMBOL(i2c_clients_command);

static int __init i2c_init(void)
{
	int retval;

//	retval = of_alias_get_highest_id("i2c");

//	if (retval >= __i2c_first_dynamic_bus_num)
//		__i2c_first_dynamic_bus_num = retval + 1;

	is_registered = true;

#ifdef CONFIG_I2C_COMPAT
	i2c_adapter_compat_class = class_compat_register("i2c-adapter");
	if (!i2c_adapter_compat_class) {
		retval = -ENOMEM;
		goto bus_err;
	}
#endif
	retval = i2c_add_driver(&dummy_driver);
	if (retval)
		goto class_err;

	return 0;

class_err:
#ifdef CONFIG_I2C_COMPAT
	class_compat_unregister(i2c_adapter_compat_class);
bus_err:
#endif
	is_registered = false;
	return retval;
}

static void __exit i2c_exit(void)
{
	i2c_del_driver(&dummy_driver);
#ifdef CONFIG_I2C_COMPAT
	class_compat_unregister(i2c_adapter_compat_class);
#endif
}

/* ----------------------------------------------------
 * the functional interface to the i2c busses.
 * ----------------------------------------------------
 */

/* Check if val is exceeding the quirk IFF quirk is non 0 */
#define i2c_quirk_exceeded(val, quirk) ((quirk) && ((val) > (quirk)))

static int i2c_quirk_error(struct i2c_adapter *adap, struct i2c_msg *msg, char *err_msg)
{
	dev_err(&adap->dev, "adapter quirk: %s (addr 0x%04x, size %u, %s)\n",
			    err_msg, msg->addr, msg->len,
			    msg->flags & I2C_M_RD ? "read" : "write");
	return -EOPNOTSUPP;
}

static int i2c_check_for_quirks(struct i2c_adapter *adap, struct i2c_msg *msgs, int num)
{
	const struct i2c_adapter_quirks *q = adap->quirks;
	int max_num = q->max_num_msgs, i;
	bool do_len_check = true;

	if (q->flags & I2C_AQ_COMB) {
		max_num = 2;

		/* special checks for combined messages */
		if (num == 2) {
			if (q->flags & I2C_AQ_COMB_WRITE_FIRST && msgs[0].flags & I2C_M_RD)
				return i2c_quirk_error(adap, &msgs[0], "1st comb msg must be write");

			if (q->flags & I2C_AQ_COMB_READ_SECOND && !(msgs[1].flags & I2C_M_RD))
				return i2c_quirk_error(adap, &msgs[1], "2nd comb msg must be read");

			if (q->flags & I2C_AQ_COMB_SAME_ADDR && msgs[0].addr != msgs[1].addr)
				return i2c_quirk_error(adap, &msgs[0], "comb msg only to same addr");

			if (i2c_quirk_exceeded(msgs[0].len, q->max_comb_1st_msg_len))
				return i2c_quirk_error(adap, &msgs[0], "msg too long");

			if (i2c_quirk_exceeded(msgs[1].len, q->max_comb_2nd_msg_len))
				return i2c_quirk_error(adap, &msgs[1], "msg too long");

			do_len_check = false;
		}
	}

	if (i2c_quirk_exceeded(num, max_num))
		return i2c_quirk_error(adap, &msgs[0], "too many messages");

	for (i = 0; i < num; i++) {
		u16 len = msgs[i].len;

		if (msgs[i].flags & I2C_M_RD) {
			if (do_len_check && i2c_quirk_exceeded(len, q->max_read_len))
				return i2c_quirk_error(adap, &msgs[i], "msg too long");

			if (q->flags & I2C_AQ_NO_ZERO_LEN_READ && len == 0)
				return i2c_quirk_error(adap, &msgs[i], "no zero length");
		} else {
			if (do_len_check && i2c_quirk_exceeded(len, q->max_write_len))
				return i2c_quirk_error(adap, &msgs[i], "msg too long");

			if (q->flags & I2C_AQ_NO_ZERO_LEN_WRITE && len == 0)
				return i2c_quirk_error(adap, &msgs[i], "no zero length");
		}
	}

	return 0;
}

/**
 * __i2c_transfer - unlocked flavor of i2c_transfer
 * @adap: Handle to I2C bus
 * @msgs: One or more messages to execute before STOP is issued to
 *	terminate the operation; each message begins with a START.
 * @num: Number of messages to be executed.
 *
 * Returns negative errno, else the number of messages executed.
 *
 * Adapter lock must be held when calling this function. No debug logging
 * takes place. adap->algo->master_xfer existence isn't checked.
 */
int __i2c_transfer(struct i2c_adapter *adap, struct i2c_msg *msgs, int num)
{
	unsigned long orig_jiffies;
	int ret, try;

	if (WARN_ON(!msgs || num < 1))
		return -EINVAL;

	if (adap->quirks && i2c_check_for_quirks(adap, msgs, num))
		return -EOPNOTSUPP;

	/* Retry automatically on arbitration loss */
	orig_jiffies = jiffies;
	for (ret = 0, try = 0; try <= adap->retries; try++) {
		if (adap->algo->master_xfer_atomic)
			ret = adap->algo->master_xfer_atomic(adap, msgs, num);
		else
			ret = adap->algo->master_xfer(adap, msgs, num);

		if (ret != -EAGAIN)
			break;
		if (time_after(jiffies, orig_jiffies + adap->timeout))
			break;
	}
//	orig_jiffies = jiffies;
//	for (ret = 0, try = 0; try <= adap->retries; try++) {
//		if (i2c_in_atomic_xfer_mode() && adap->algo->master_xfer_atomic)
//			ret = adap->algo->master_xfer_atomic(adap, msgs, num);
//		else
//			ret = adap->algo->master_xfer(adap, msgs, num);

//		if (ret != -EAGAIN)
//			break;
//		if (time_after(jiffies, orig_jiffies + adap->timeout))
//			break;
//	}

	return ret;
}
EXPORT_SYMBOL(__i2c_transfer);

/**
 * i2c_transfer - execute a single or combined I2C message
 * @adap: Handle to I2C bus
 * @msgs: One or more messages to execute before STOP is issued to
 *	terminate the operation; each message begins with a START.
 * @num: Number of messages to be executed.
 *
 * Returns negative errno, else the number of messages executed.
 *
 * Note that there is no requirement that each message be sent to
 * the same slave address, although that is the most common model.
 */
int i2c_transfer(struct i2c_adapter *adap, struct i2c_msg *msgs, int num)
{
	int ret;

	if (!adap->algo->master_xfer) {
		dev_dbg(&adap->dev, "I2C level transfers not supported\n");
		return -EOPNOTSUPP;
	}

	/* REVISIT the fault reporting model here is weak:
	 *
	 *  - When we get an error after receiving N bytes from a slave,
	 *    there is no way to report "N".
	 *
	 *  - When we get a NAK after transmitting N bytes to a slave,
	 *    there is no way to report "N" ... or to let the master
	 *    continue executing the rest of this combined message, if
	 *    that's the appropriate response.
	 *
	 *  - When for example "num" is two and we successfully complete
	 *    the first message but get an error part way through the
	 *    second, it's unclear whether that should be reported as
	 *    one (discarding status on the second message) or errno
	 *    (discarding status on the first one).
	 */

	ret = __i2c_transfer(adap, msgs, num);
	i2c_unlock_bus(adap, I2C_LOCK_SEGMENT);

	return ret;
}
EXPORT_SYMBOL(i2c_transfer);

/**
 * i2c_transfer_buffer_flags - issue a single I2C message transferring data
 *			       to/from a buffer
 * @client: Handle to slave device
 * @buf: Where the data is stored
 * @count: How many bytes to transfer, must be less than 64k since msg.len is u16
 * @flags: The flags to be used for the message, e.g. I2C_M_RD for reads
 *
 * Returns negative errno, or else the number of bytes transferred.
 */
int i2c_transfer_buffer_flags(const struct i2c_client *client, void *buf,
			      int count, u16 flags)
{
	int ret;
	struct i2c_msg msg = {
		.addr = client->addr,
		.flags = flags | (client->flags & I2C_M_TEN),
		.len = count,
		.buf = buf,
	};

	ret = i2c_transfer(client->adapter, &msg, 1);

	/*
	 * If everything went ok (i.e. 1 msg transferred), return #bytes
	 * transferred, else error code.
	 */
	return (ret == 1) ? count : ret;
}
EXPORT_SYMBOL(i2c_transfer_buffer_flags);

/**
 * i2c_get_device_id - get manufacturer, part id and die revision of a device
 * @client: The device to query
 * @id: The queried information
 *
 * Returns negative errno on error, zero on success.
 */
int i2c_get_device_id(const struct i2c_client *client,
		      struct i2c_device_identity *id)
{
	struct i2c_adapter *adap = client->adapter;
	union i2c_smbus_data raw_id;
	int ret;

	if (!i2c_check_functionality(adap, I2C_FUNC_SMBUS_READ_I2C_BLOCK))
		return -EOPNOTSUPP;

	raw_id.block[0] = 3;
	ret = i2c_smbus_xfer(adap, I2C_ADDR_DEVICE_ID, 0,
			     I2C_SMBUS_READ, client->addr << 1,
			     I2C_SMBUS_I2C_BLOCK_DATA, &raw_id);
	if (ret)
		return ret;

	id->manufacturer_id = (raw_id.block[1] << 4) | (raw_id.block[2] >> 4);
	id->part_id = ((raw_id.block[2] & 0xf) << 5) | (raw_id.block[3] >> 3);
	id->die_revision = raw_id.block[3] & 0x7;
	return 0;
}
EXPORT_SYMBOL_GPL(i2c_get_device_id);

/* ----------------------------------------------------
 * the i2c address scanning function
 * Will not work for 10-bit addresses!
 * ----------------------------------------------------
 */

/*
 * Legacy default probe function, mostly relevant for SMBus. The default
 * probe method is a quick write, but it is known to corrupt the 24RF08
 * EEPROMs due to a state machine bug, and could also irreversibly
 * write-protect some EEPROMs, so for address ranges 0x30-0x37 and 0x50-0x5f,
 * we use a short byte read instead. Also, some bus drivers don't implement
 * quick write, so we fallback to a byte read in that case too.
 * On x86, there is another special case for FSC hardware monitoring chips,
 * which want regular byte reads (address 0x73.) Fortunately, these are the
 * only known chips using this I2C address on PC hardware.
 * Returns 1 if probe succeeded, 0 if not.
 */
static int i2c_default_probe(struct i2c_adapter *adap, unsigned short addr)
{
	int err;
	union i2c_smbus_data dummy;

#ifdef CONFIG_X86
	if (addr == 0x73 && (adap->class & I2C_CLASS_HWMON)
	 && i2c_check_functionality(adap, I2C_FUNC_SMBUS_READ_BYTE_DATA))
		err = i2c_smbus_xfer(adap, addr, 0, I2C_SMBUS_READ, 0,
				     I2C_SMBUS_BYTE_DATA, &dummy);
	else
#endif
	if (!((addr & ~0x07) == 0x30 || (addr & ~0x0f) == 0x50)
	 && i2c_check_functionality(adap, I2C_FUNC_SMBUS_QUICK))
		err = i2c_smbus_xfer(adap, addr, 0, I2C_SMBUS_WRITE, 0,
				     I2C_SMBUS_QUICK, NULL);
	else if (i2c_check_functionality(adap, I2C_FUNC_SMBUS_READ_BYTE))
		err = i2c_smbus_xfer(adap, addr, 0, I2C_SMBUS_READ, 0,
				     I2C_SMBUS_BYTE, &dummy);
	else {
		dev_warn(&adap->dev, "No suitable probing method supported for address 0x%02X\n",
			 addr);
		err = -EOPNOTSUPP;
	}

	return err >= 0;
}

static int i2c_detect_address(struct i2c_client *temp_client,
			      struct i2c_driver *driver)
{
	struct i2c_board_info info;
	struct i2c_adapter *adapter = temp_client->adapter;
	int addr = temp_client->addr;
	int err;

	/* Make sure the address is valid */
	err = i2c_check_7bit_addr_validity_strict(addr);
	if (err) {
		dev_warn(&adapter->dev, "Invalid probe address 0x%02x\n",
			 addr);
		return err;
	}

	/* Skip if already in use (7 bit, no need to encode flags) */
	if (i2c_check_addr_busy(adapter, addr))
		return 0;

	/* Make sure there is something at this address */
	if (!i2c_default_probe(adapter, addr))
		return 0;

	/* Finally call the custom detection function */
	memset(&info, 0, sizeof(struct i2c_board_info));
	info.addr = addr;
	err = driver->detect(temp_client, &info);
	if (err) {
		/* -ENODEV is returned if the detection fails. We catch it
		   here as this isn't an error. */
		return err == -ENODEV ? 0 : err;
	}

	/* Consistency check */
	if (info.type[0] == '\0') {
//		dev_err(&adapter->dev,
//			"%s detection function provided no name for 0x%x\n",
//			driver->driver.name, addr);
	} else {
		struct i2c_client *client;

		/* Detection succeeded, instantiate the device */
		if (adapter->class & I2C_CLASS_DEPRECATED)
			dev_warn(&adapter->dev,
				"This adapter will soon drop class based instantiation of devices. "
				"Please make sure client 0x%02x gets instantiated by other means. "
				"Check 'Documentation/i2c/instantiating-devices.rst' for details.\n",
				info.addr);

		dev_dbg(&adapter->dev, "Creating %s at 0x%02x\n",
			info.type, info.addr);
		client = i2c_new_client_device(adapter, &info);
		if (!IS_ERR(client))
			list_add_tail(&client->detected, &driver->clients);
		else
			dev_err(&adapter->dev, "Failed creating %s at 0x%02x\n",
				info.type, info.addr);
	}
	return 0;
}

static int i2c_detect(struct i2c_adapter *adapter, struct i2c_driver *driver)
{
	const unsigned short *address_list;
	struct i2c_client *temp_client;
	int i, err = 0;

	address_list = driver->address_list;
	if (!driver->detect || !address_list)
		return 0;

	/* Warn that the adapter lost class based instantiation */
	if (adapter->class == I2C_CLASS_DEPRECATED) {
//		dev_dbg(&adapter->dev,
//			"This adapter dropped support for I2C classes and won't auto-detect %s devices anymore. "
//			"If you need it, check 'Documentation/i2c/instantiating-devices.rst' for alternatives.\n",
//			driver->driver.name);
		return 0;
	}

	/* Stop here if the classes do not match */
	if (!(adapter->class & driver->class))
		return 0;

	/* Set up a temporary client to help detect callback */
	temp_client = kzalloc(sizeof(struct i2c_client), GFP_KERNEL);
	if (!temp_client)
		return -ENOMEM;
	temp_client->adapter = adapter;

	for (i = 0; address_list[i] != I2C_CLIENT_END; i += 1) {
		dev_dbg(&adapter->dev,
			"found normal entry for adapter %d, addr 0x%02x\n",
			i2c_adapter_id(adapter), address_list[i]);
		temp_client->addr = address_list[i];
		err = i2c_detect_address(temp_client, driver);
		if (unlikely(err))
			break;
	}

	kfree(temp_client);
	return err;
}

int i2c_probe_func_quick_read(struct i2c_adapter *adap, unsigned short addr)
{
	return i2c_smbus_xfer(adap, addr, 0, I2C_SMBUS_READ, 0,
			      I2C_SMBUS_QUICK, NULL) >= 0;
}
EXPORT_SYMBOL_GPL(i2c_probe_func_quick_read);

struct i2c_client *
i2c_new_scanned_device(struct i2c_adapter *adap,
		       struct i2c_board_info *info,
		       unsigned short const *addr_list,
		       int (*probe)(struct i2c_adapter *adap, unsigned short addr))
{
	int i;

	if (!probe)
		probe = i2c_default_probe;

	for (i = 0; addr_list[i] != I2C_CLIENT_END; i++) {
		/* Check address validity */
		if (i2c_check_7bit_addr_validity_strict(addr_list[i]) < 0) {
			dev_warn(&adap->dev, "Invalid 7-bit address 0x%02x\n",
				 addr_list[i]);
			continue;
		}

		/* Check address availability (7 bit, no need to encode flags) */
		if (i2c_check_addr_busy(adap, addr_list[i])) {
			dev_dbg(&adap->dev,
				"Address 0x%02x already in use, not probing\n",
				addr_list[i]);
			continue;
		}

		/* Test address responsiveness */
		if (probe(adap, addr_list[i]))
			break;
	}

	if (addr_list[i] == I2C_CLIENT_END) {
		dev_dbg(&adap->dev, "Probing failed, no device found\n");
		return ERR_PTR(-ENODEV);
	}

	info->addr = addr_list[i];
	return i2c_new_client_device(adap, info);
}
EXPORT_SYMBOL_GPL(i2c_new_scanned_device);

struct i2c_adapter *i2c_get_adapter(int nr)
{
	struct i2c_adapter *adapter;

//	adapter = idr_find(&i2c_adapter_idr, nr);
	if (!adapter)
		goto exit;

 exit:
	return adapter;
}
EXPORT_SYMBOL(i2c_get_adapter);

void i2c_put_adapter(struct i2c_adapter *adap)
{
	if (!adap)
		return;
}
EXPORT_SYMBOL(i2c_put_adapter);

/**
 * i2c_get_dma_safe_msg_buf() - get a DMA safe buffer for the given i2c_msg
 * @msg: the message to be checked
 * @threshold: the minimum number of bytes for which using DMA makes sense.
 *	       Should at least be 1.
 *
 * Return: NULL if a DMA safe buffer was not obtained. Use msg->buf with PIO.
 *	   Or a valid pointer to be used with DMA. After use, release it by
 *	   calling i2c_put_dma_safe_msg_buf().
 *
 * This function must only be called from process context!
 */
u8 *i2c_get_dma_safe_msg_buf(struct i2c_msg *msg, unsigned int threshold)
{
    void *buffer;
    
	/* also skip 0-length msgs for bogus thresholds of 0 */
	if (!threshold)
		pr_debug("DMA buffer for addr=0x%02x with length 0 is bogus\n",
			 msg->addr);
	if (msg->len < threshold || msg->len == 0)
		return NULL;

	if (msg->flags & I2C_M_DMA_SAFE)
		return msg->buf;

	pr_debug("using bounce buffer for addr=0x%02x, len=%d\n",
		 msg->addr, msg->len);

	if (msg->flags & I2C_M_RD)
		return kzalloc(msg->len, GFP_KERNEL);
	else {
        buffer = kzalloc(msg->len, GFP_KERNEL);
		return memcpy(buffer, msg->buf, msg->len);
    }
}
EXPORT_SYMBOL_GPL(i2c_get_dma_safe_msg_buf);

/**
 * i2c_put_dma_safe_msg_buf - release DMA safe buffer and sync with i2c_msg
 * @buf: the buffer obtained from i2c_get_dma_safe_msg_buf(). May be NULL.
 * @msg: the message which the buffer corresponds to
 * @xferred: bool saying if the message was transferred
 */
void i2c_put_dma_safe_msg_buf(u8 *buf, struct i2c_msg *msg, bool xferred)
{
	if (!buf || buf == msg->buf)
		return;

	if (xferred && msg->flags & I2C_M_RD)
		memcpy(msg->buf, buf, msg->len);

	kfree(buf);
}
EXPORT_SYMBOL_GPL(i2c_put_dma_safe_msg_buf);
