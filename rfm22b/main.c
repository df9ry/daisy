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

MODULE_AUTHOR("Tania Hagn - DF9RY - <Tania@DF9RY.de>");
MODULE_LICENSE("Dual BSD/GPL");
MODULE_DESCRIPTION("Master for RFM2235B chip");
MODULE_VERSION("0.0.1");

#define SPI_BUS_NO 16

static struct spi_device *rfm22b_device = NULL;

static struct spi_board_info __initdata rfm22b_board = {
		.modalias     = "rfm22b",
		.max_speed_hz = 10000000,
		.bus_num      = SPI_BUS_NO,
		.chip_select  = 0,
		.mode         = SPI_MODE_0,
};

/*
 * Start / stop stuff:
 */
static void __exit rfm22b_term(void)
{
	printk(KERN_INFO "rfm22b: Terminate\n");

	if (rfm22b_device) {
		printk(KERN_DEBUG "rfm22b: spi_unregister_device\n");
		spi_unregister_device(rfm22b_device);
		rfm22b_device = NULL;
	}

	printk(KERN_DEBUG "rfm22b: Terminate complete\n");
}

static int __init rfm22b_init(void)
{
	struct spi_master *master;

	printk(KERN_INFO "rfm22b: Initialize\n");

	master = spi_busnum_to_master(SPI_BUS_NO);
	/**/	if (!master) {
	/**/		printk(KERN_DEBUG "rfm22b: Unable to get SPI16\n");
	/**/		return -ENODEV;
	/**/	}
	/**/	rfm22b_device = spi_new_device(master, &rfm22b_board);
	spi_master_put(master);

	if (!rfm22b_device) {
		printk(KERN_INFO "rfm22b: Unable to create device\n");
		return -EINVAL;
	}

	return 0;
}

module_init(rfm22b_init);
module_exit(rfm22b_term);
