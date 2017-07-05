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
#include <linux/delay.h>
#include <linux/of.h>

#include <linux/spi/spi.h>

#include "rfm22b.h"
#include "bcm2835.h"
#include "initdata.h"

MODULE_AUTHOR("Tania Hagn - DF9RY");
MODULE_LICENSE("Dual BSD/GPL");
MODULE_DESCRIPTION("SPI driver for RFM2235B chip");
MODULE_VERSION("0.0.1");

/*
 * Useful defaults:
 */
#define SPI_CLOCK_DIVIDER 64   /* SPI communication speed */
#define RFM22B_TYPE_ID     8   /* SPI chip id             */
#define SPI_BUS_NO         0   /* SPI bus number          */

struct rfm22b {
	struct device *dev;
	void          *client;
	struct         rfm22b_client_info *ci;
	int            irq;
};

static int spi_reg_read(struct rfm22b *rfm22b, u8 reg)
{
	struct spi_device *spi = rfm22b->client;
	u8 _reg[2] = { reg, 0x00 };
	return spi_read(spi, _reg, 2);
}

static int spi_reg_write(struct rfm22b *rfm22b, u8 reg, u8 val)
{
	struct spi_device *spi = rfm22b->client;
	u8 _reg[2] = { reg|(1<<8), val };
	return spi_write(spi, _reg, 2);
}

static int spi_block_read(struct rfm22b *rfm22b, u8 reg, u8 length, u8 *values)
{
	///TODO: Implement burst read
	int ret, i;

	for (i = 0; i < length; i++) {
		ret = spi_reg_read(rfm22b, reg + i);
		if (ret < 0)
			return ret;
		*(values + i) = ret;
	}

	return 0;
}

static int spi_block_write(struct rfm22b *rfm22b, u8 reg, u8 length,
		const u8 *values)
{
	///TODO: Implement burst write
	int ret = 0, i;

	for (i = length; i > 0; i--, reg++) {
		ret = spi_reg_write(rfm22b, reg, *(values + i - 1));
		if (ret < 0)
			return ret;
	}

	return ret;
}

static void spi_init(struct rfm22b *rfm22b)
{
	struct spi_device *spi = rfm22b->client;

	spi->bits_per_word = 8;
	///TODO: Initialize the chip

	/* This register is only present for stmpe811 */
	//if (stmpe->variant->id_val == 0x0811)
	//	spi_reg_write(stmpe, STMPE811_REG_SPI_CFG, spi->mode);

	if (spi_setup(spi) < 0)
		dev_dbg(&spi->dev, "spi_setup failed\n");
}

static struct rfm22b_client_info {
	int          (*read_byte)   (struct rfm22b *, u8);
	int          (*write_byte)  (struct rfm22b *, u8, u8);
	int          (*read_block)  (struct rfm22b *, u8, u8, u8 *);
	int          (*write_block) (struct rfm22b *, u8, u8, const u8 *);
	void         (*init)        (struct rfm22b *);
	int            irq;
	void          *client;
	struct device *dev;
} spi_ci = {
	.read_byte   = spi_reg_read,
	.write_byte  = spi_reg_write,
	.read_block  = spi_block_read,
	.write_block = spi_block_write,
	.init        = spi_init,
};

static int rfm22b_spi_probe(struct spi_device *spi)
{
	//const struct spi_device_id *id = spi_get_device_id(spi);

	/* don't exceed max specified rate - 10MHz - Limitation of RFM22B */
	if (spi->max_speed_hz > 10000000) {
		dev_dbg(&spi->dev, "f(sample) %d KHz?\n",
				(spi->max_speed_hz/1000));
		return -EINVAL;
	}

	spi_ci.irq = spi->irq;
	spi_ci.client = spi;
	spi_ci.dev = &spi->dev;

	///TODO: Probing
	//return rfm22b_probe(&spi_ci, id->driver_data);
	return 0;
}

static int rfm22b_spi_remove(struct spi_device *spi)
{
	//struct rfm22b *rfm22b = spi_get_drvdata(spi);

	///TODO: Removing
	//return rfm22b_remove(rfm22b);
	return 0;
}

static void rfm22b_spi_shutdown(struct spi_device *spi)
{
}

static const struct of_device_id rfm22b_spi_of_match[] = {
	{ .compatible = "hoperf,rfm22b" },
	{ .compatible = "rfm22b" },
	{ .compatible = "spidev" },
	{ }
};
MODULE_DEVICE_TABLE(of, rfm22b_spi_of_match);

static const struct spi_device_id rfm22b_spi_id[] = {
	{ "rfm22b", RFM22B_TYPE_ID },
	{ }
};
MODULE_DEVICE_TABLE(spi, rfm22b_spi_id);

static struct spi_driver rfm22b_spi_driver = {
	.driver = {
		.name	= "rfm22b-spi",
		.of_match_table = of_match_ptr(rfm22b_spi_of_match),
	},
	.probe		= rfm22b_spi_probe,
	.remove		= rfm22b_spi_remove,
	.shutdown   = rfm22b_spi_shutdown,
	.id_table	= rfm22b_spi_id,
};

/*
 * Remember, if driver is successfully registered
 */
static bool spi_driver_initialized = false;

/*
 * Finally, the module stuff
 */

static void rfm22b_term(void)
{
	//printk(KERN_DEBUG "rfm22b: Cleanup\n");
	if (spi_driver_initialized)
		spi_unregister_driver(&rfm22b_spi_driver);
#if 0
	bcm2835_release();
#endif
}

static int __init rfm22b_init(void)
{
	int erc;

#if 0
	//printk(KERN_DEBUG "rfm22b: Initializing\n");
	/* Initialize the BCM2835 */
	erc = bcm2835_initialize();
	if (erc) {
		printk(KERN_ERR
				"rfm22b: Unable to initialize BCM2835!\n");
		goto out;
	}

	/* Start the SPI subsystem */
	if (!bcm2835_spi_begin()) {
		printk(KERN_ERR
				"rfm22b: Unable to start SPI!\n");
		goto out;
	}

	/* Increase the SPI bus speed */
	bcm2835_spi_setClockDivider(SPI_CLOCK_DIVIDER);

	/* Check if RFM22B is on board */
	{
		uint8_t tx[3] = { 0x00, 0x00, 0x00 };
		uint8_t rx[3] = { 0x00, 0x00, 0x00 };
		int i;

		bcm2835_spi_transfernb(tx, rx, 3);
		if (rx[1] != 8) {
			printk(KERN_ERR
					"rfm22b: RFM22B chip not present - read: %i!\n",
					(int)rx[1]);
			goto out;
		}
		printk(KERN_INFO
				"rfm22b: Found RFM22B version %d.\n", (int)rx[2]);

		// Issue a chip reset:
		tx[0] = 0x87;
		bcm2835_spi_transfernb(tx, rx, 2);
		mdelay(10); // Let the device time to settle up

		// Setup the device with default data:
		for (i = 0; init_data[i][0] != 0x00; ++i)
			bcm2835_spi_transfernb(init_data[i], rx, 2);
		mdelay(10); // Let the device time to settle up
	}
#endif

	erc = spi_register_driver(&rfm22b_spi_driver);
	if (erc != 0) {
		printk(KERN_ERR
				"rfm22b: Unable to register driver: %i!\n", erc);
		goto out;
	}
	spi_driver_initialized = true;

	erc = 0;

out:
	if (erc)
		rfm22b_term();

	//printk(KERN_DEBUG "rfm22b: Initialize finished with ERC=%i\n", erc);
	return erc;
}

module_init(rfm22b_init);
module_exit(rfm22b_term);

