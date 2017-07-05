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
#include <linux/spi/spi.h>

#include "rfm22b-bus.h"

MODULE_AUTHOR("Tania Hagn - DF9RY - <Tania@DF9RY.de>");
MODULE_LICENSE("Dual BSD/GPL");
MODULE_DESCRIPTION("Master for RFM2235B chip");
MODULE_VERSION("0.0.1");


static struct spi_master *rfm22b_master = NULL;
static struct spi_device *rfm22b_device = NULL;

static int rfm22b_match(struct device *dev, struct device_driver *driver) {
	return !strncmp(DRV_NAME, driver->name, strlen(DRV_NAME));
}

static struct bus_type rfm22b_bus_type = {
		.name    = "rfm22b",
		.match   = rfm22b_match,
};
static bool rfm22b_bus_type_initialized = false;

static void rfm22b_bus_release(struct device *dev) {
	printk(KERN_INFO "rfm22b-bus: release\n");
}

static struct device rfm22b_bus = {
		.init_name = "rfm22b",
		.release   = rfm22b_bus_release,
};
static bool rfm22b_bus_initialized = false;

static struct spi_board_info rfm22b_board = {
		.modalias     = DRV_NAME,
		.max_speed_hz = SPI_MAX_SPEED_HZ,
		.bus_num      = SPI_BUS_NUM,
		.chip_select  = 0,
		.mode         = SPI_MODE_0,
};

static const struct of_device_id rfm22b_dt_ids[] = {
	{ .compatible = "hoperf,rfm22b" },
	{ .compatible = "rfm22b" },
	{ .compatible = "spidev" },
	{}
};

static const struct spi_device_id rfm22b_ids[] = {
	{ .name = "hoperf,rfm22b" },
	{ .name = "rfm22b" },
	{ .name = "spidev" },
	{}
};

static int rfm22b_transfer(struct spi_device *spi, struct spi_message *mesg)
{
	return 0;
}

static int rfm22b_probe(struct spi_device *spi) {
	printk(KERN_INFO "rfm22b-bus: rfm22b_probe\n");
	return 0;
}

static int rfm22b_remove(struct spi_device *spi) {
	printk(KERN_INFO "rfm22b-bus: rfm22b_remove\n");
	return 0;
}

static void rfm22b_shutdown(struct spi_device *spi) {
	printk(KERN_INFO "rfm22b-bus: rfm22b_shutdown\n");
}

static struct spi_driver rfm22b_driver = {
	.driver = {
		.name           = "rfm22b",
		.owner          = THIS_MODULE,
		.of_match_table = of_match_ptr(rfm22b_dt_ids),
	},
	.id_table = rfm22b_ids,
	.probe    = rfm22b_probe,
	.remove   = rfm22b_remove,
	.shutdown = rfm22b_shutdown,
};
static bool rfm22b_driver_inititlized = false;

/*
 * Start / stop stuff:
 */
static void rfm22b_term(void)
{
	printk(KERN_INFO "rfm22b-bus: Terminate\n");

	printk(KERN_DEBUG "rfm22b-bus: spi_unregister_driver\n");
	if (rfm22b_driver_inititlized) {
		spi_unregister_driver(&rfm22b_driver);
		rfm22b_driver_inititlized = false;
	}

	printk(KERN_DEBUG "rfm22b-bus: spi_unregister_device\n");
	if (rfm22b_device) {
		spi_unregister_device(rfm22b_device);
		rfm22b_device = NULL;
	}

	printk(KERN_DEBUG "rfm22b-bus: spi_unregister_master\n");
	if (rfm22b_master) {
		spi_unregister_master(rfm22b_master);
		rfm22b_master = NULL;
	}

	printk(KERN_DEBUG "rfm22b-bus: device_unregister\n");
	if (rfm22b_bus_initialized) {
		device_unregister(&rfm22b_bus);
		rfm22b_bus_initialized = false;
	}

	printk(KERN_DEBUG "rfm22b-bus: bus_unregister\n");
	if (rfm22b_bus_type_initialized) {
		bus_unregister(&rfm22b_bus_type);
		rfm22b_bus_type_initialized = false;
	}

	printk(KERN_DEBUG "rfm22b-bus: Terminate complete\n");
}

static int rfm22b_init(void)
{
	int erc = 0;

	printk(KERN_INFO "rfm22b-bus: Initialize\n");
	if (bus_register(&rfm22b_bus_type) != 0) {
		printk(KERN_INFO "rfm22b-bus: bus_register failed\n");
		erc = -ENODEV;
		goto fail;
	}
	rfm22b_bus_type_initialized = true;

	if (device_register(&rfm22b_bus)) {
		printk(KERN_INFO "rfm22b-bus: device_register failed\n");
		erc = -ENODEV;
		goto fail;
	}
	rfm22b_bus_initialized = true;

	rfm22b_master = spi_alloc_master(
			&rfm22b_bus, sizeof(struct rfm22b_master_info));
	if (!rfm22b_master) {
		printk(KERN_INFO "rfm22b-bus: spi_alloc_master failed\n");
		erc = -ENOMEM;
		goto fail;
	}
	rfm22b_master->bits_per_word_mask = SPI_BIT_MASK(8);
	rfm22b_master->bus_num            = SPI_BUS_NUM;
	rfm22b_master->max_speed_hz       = SPI_MAX_SPEED_HZ;
	rfm22b_master->mode_bits          = SPI_MODE_0;
	rfm22b_master->num_chipselect     = 1;
	rfm22b_master->transfer           = rfm22b_transfer;
	if (spi_register_master(rfm22b_master) != 0) {
		printk(KERN_INFO "rfm22b-bus: devm_spi_register_master failed\n");
		erc = -ENODEV;
		goto fail;
	}

	rfm22b_device = spi_new_device(rfm22b_master, &rfm22b_board);
	if (!rfm22b_device) {
		printk(KERN_INFO "rfm22b-bus: spi_new_device failed\n");
		erc = -ENOMEM;
		goto fail;
	}

	if (__spi_register_driver(THIS_MODULE, &rfm22b_driver) != 0) {
		printk(KERN_INFO "rfm22b-bus: __spi_register_driver failed\n");
		erc = -ENODEV;
		goto fail;
	}
	rfm22b_driver_inititlized = true;

	printk(KERN_INFO "rfm22b-bus: Initialize complete\n");

	return 0;

fail:
	rfm22b_term();

	return erc;
}

module_init(rfm22b_init);
module_exit(rfm22b_term);
