/* Copyright 2017 Tania Hagn
 *
 * This file is part of Daisy.
 *
 *    Daisy is free software: you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation, either version 3 of the License, or
 *    (at your option) any later version.
 *
 *    Daisy is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with Daisy.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <linux/module.h>
#include <linux/device.h>
#include <linux/of.h>
#include <linux/of_platform.h>
#include <linux/spi/spi.h>

#include "spi16.h"

MODULE_AUTHOR("Tania Hagn - DF9RY - <Tania@DF9RY.de>");
MODULE_LICENSE("Dual BSD/GPL");
MODULE_DESCRIPTION("Implementation of SPI bus 16");
MODULE_VERSION("0.0.1");


static struct spi_master *spi16_master = NULL;

static int spi16_match(struct device *dev, struct device_driver *driver) {
	int match = (of_match_device(driver->of_match_table, dev) != NULL);
	printk(KERN_INFO "spi16: spi16_match: %s\n", match?"true":"false");
	return !strncmp(DRV_NAME, driver->name, strlen(DRV_NAME));
}

static struct bus_type spi16_bus_type = {
		.name    = "spi16",
		.match   = spi16_match,
};
static bool spi16_bus_type_initialized = false;

static void spi16_bus_release(struct device *dev) {
	printk(KERN_INFO "spi16: release\n");
}

static struct device spi16_bus = {
		.init_name = "spi16",
		.release   = spi16_bus_release,
};
static bool spi16_bus_initialized = false;

static const struct of_device_id spi16_dt_ids[] = {
	{ .compatible = "hoperf,rfm22b" },
	{ .compatible = "rfm22b" },
	{ .compatible = "spi16dev" },
	{}
};

static const struct spi_device_id spi16_ids[] = {
	{ .name = "hoperf,rfm22b" },
	{ .name = "rfm22b" },
	{ .name = "spi16dev" },
	{}
};

static int spi16_transfer(struct spi_device *spi, struct spi_message *mesg)
{
	printk(KERN_INFO "spi16: spi16_transfer: Not implemented\n");
	return -EINVAL;
}

static int spi16_probe(struct spi_device *spi) {
	printk(KERN_INFO "spi16: spi16_probe\n");
	return 0;
}

static int spi16_remove(struct spi_device *spi) {
	printk(KERN_INFO "spi16: spi16_remove\n");
	return 0;
}

static void spi16_shutdown(struct spi_device *spi) {
	printk(KERN_INFO "spi16: spi16_shutdown\n");
}

static struct spi_driver spi16_driver = {
	.driver = {
		.name           = "spi16",
		.owner          = THIS_MODULE,
		.of_match_table = of_match_ptr(spi16_dt_ids),
	},
	.id_table = spi16_ids,
	.probe    = spi16_probe,
	.remove   = spi16_remove,
	.shutdown = spi16_shutdown,
};
static bool spi16_driver_inititlized = false;

/*
 * Start / stop stuff:
 */
static void spi16_term(void)
{
	printk(KERN_INFO "spi16: Terminate\n");

	printk(KERN_DEBUG "spi16: spi_unregister_driver\n");
	if (spi16_driver_inititlized) {
		spi_unregister_driver(&spi16_driver);
		spi16_driver_inititlized = false;
	}

	printk(KERN_DEBUG "spi16: spi_unregister_master\n");
	if (spi16_master) {
		spi_unregister_master(spi16_master);
		spi16_master = NULL;
	}

	printk(KERN_DEBUG "spi16: device_unregister\n");
	if (spi16_bus_initialized) {
		device_unregister(&spi16_bus);
		spi16_bus_initialized = false;
	}

	printk(KERN_DEBUG "spi16: bus_unregister\n");
	if (spi16_bus_type_initialized) {
		bus_unregister(&spi16_bus_type);
		spi16_bus_type_initialized = false;
	}

	printk(KERN_DEBUG "spi16: Terminate complete\n");
}

static int __init spi16_init(void)
{
	int erc = 0;

	printk(KERN_INFO "spi16: Initialize\n");
	if (bus_register(&spi16_bus_type) != 0) {
		printk(KERN_INFO "spi16: bus_register failed\n");
		erc = -ENODEV;
		goto fail;
	}
	spi16_bus_type_initialized = true;

	if (device_register(&spi16_bus)) {
		printk(KERN_INFO "spi16: device_register failed\n");
		erc = -ENODEV;
		goto fail;
	}
	spi16_bus_initialized = true;

	spi16_master = spi_alloc_master(
			&spi16_bus, sizeof(struct spi16_master_info));
	if (!spi16_master) {
		printk(KERN_INFO "spi16: spi_alloc_master failed\n");
		erc = -ENOMEM;
		goto fail;
	}
	spi16_master->bits_per_word_mask = SPI_BIT_MASK(8);
	spi16_master->bus_num            = SPI_BUS_NUM;
	spi16_master->max_speed_hz       = SPI_MAX_SPEED_HZ;
	spi16_master->mode_bits          = SPI_MODE_0;
	spi16_master->num_chipselect     = 1;
	spi16_master->transfer           = spi16_transfer;
	if (spi_register_master(spi16_master) != 0) {
		printk(KERN_INFO "spi16: devm_spi_register_master failed\n");
		erc = -ENODEV;
		goto fail;
	}

	if (__spi_register_driver(THIS_MODULE, &spi16_driver) != 0) {
		printk(KERN_INFO "spi16: __spi_register_driver failed\n");
		erc = -ENODEV;
		goto fail;
	}
	spi16_driver_inititlized = true;

	printk(KERN_INFO "spi16: Initialize complete\n");

	return 0;

fail:
	spi16_term();

	return erc;
}

module_init(spi16_init);
module_exit(spi16_term);
