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
#include <linux/clk.h>
#include <linux/platform_device.h>
#include <linux/spinlock.h>
#include <linux/of.h>

#include <linux/spi/spi.h>

#include "spi-daisy.h"
#include "bcm2835.h"

#define daisy_SPI_MODE_BITS	(SPI_CPOL | SPI_CPHA | SPI_CS_HIGH \
				| SPI_NO_CS | SPI_3WIRE)

#undef PREPARE_MESSAGE

struct daisy_spi {
	spinlock_t    transfer_lock;
	struct clk   *clk;
	uint32_t      spi_hz;
};

#ifdef PREPARE_MESSAGE
static int daisy_spi_prepare_message(struct spi_master *master,
				       struct spi_message *msg)
{
	//struct spi_device *spi = msg->spi;
	//struct bcm2835_spi *bs = spi_master_get_devdata(master);

	printk(KERN_DEBUG DRV_NAME ": Called spi_prepare_message()\n");

	return 0;
}
#endif

static void daisy_spi_handle_err(struct spi_master *master,
                                 struct spi_message *msg)
{
	printk(KERN_DEBUG DRV_NAME ": Called spi_handle_err()\n");
}

static int daisy_spi_transfer_one(
					struct spi_master   *master,
				    struct spi_device   *spi,
				    struct spi_transfer *tfr)
{
	struct daisy_spi *bs = spi_master_get_devdata(master);
	uint32_t spi_hz = tfr->speed_hz;
	/* Only set speed, if this time is different than last time */
	if (spi_hz != bs->spi_hz) {
		uint32_t clk_hz, cdiv, spi_used_hz;
		bs->spi_hz = spi_hz;
		/* set clock */
		clk_hz = clk_get_rate(bs->clk);
		if (spi_hz >= clk_hz / 2) {
			cdiv = 2; /* clk_hz/2 is the fastest we can go */
		} else if (spi_hz) {
			/* CDIV must be a multiple of two */
			cdiv = DIV_ROUND_UP(clk_hz, spi_hz);
			cdiv += (cdiv % 2);
			if (cdiv >= 65536)
				cdiv = 0; /* 0 is the slowest we can go */
		} else {
			cdiv = 0; /* 0 is the slowest we can go */
		}
		bcm2835_spi_setClockDivider(cdiv);
		spi_used_hz = cdiv ? (clk_hz / cdiv) : (clk_hz / 65536);
		printk(KERN_DEBUG DRV_NAME ": SPI clock req: %d Hz, got %d Hz\n",
				spi_hz, spi_used_hz);
	}

	spin_lock(&bs->transfer_lock);
	bcm2835_spi_transfernb(tfr->tx_buf, tfr->rx_buf, tfr->len);
	spin_unlock(&bs->transfer_lock);

	spi_finalize_current_transfer(master);
	return 0;
}

static void daisy_spi_set_cs(struct spi_device *spi, bool gpio_level)
{
	printk(KERN_DEBUG DRV_NAME ": Called spi_set_cs()\n");
}

static int daisy_spi_setup(struct spi_device *spi)
{
	printk(KERN_DEBUG DRV_NAME ": Called spi_setup()\n");
	/*
	 * sanity checking the native-chipselects
	 */
	if (spi->mode & SPI_NO_CS)
		return 0;
	if (spi->chip_select > 1) {
		/* error in the case of native CS requested with CS > 1
		 * officially there is a CS2, but it is not documented
		 * which GPIO is connected with that...
		 */
		printk(KERN_INFO DRV_NAME
				": Setup: only two native chip-selects are supported\n");
		return -EINVAL;
	}

	return 0;
}

static int daisy_spi_probe(struct platform_device *pdev)
{
	struct spi_master  *master;
	struct daisy_spi   *bs;
	int err;

	printk(KERN_DEBUG DRV_NAME ": Called spi_probe()\n");
	master = spi_alloc_master(&pdev->dev, sizeof(*bs));
	if (!master) {
		printk(KERN_ERR DRV_NAME ": spi_alloc_master() failed\n");
		return -ENOMEM;
	}

	platform_set_drvdata(pdev, master);

	master->mode_bits          = daisy_SPI_MODE_BITS;
	master->bits_per_word_mask = SPI_BPW_MASK(8);
	master->num_chipselect     = 3;
	master->setup              = daisy_spi_setup;
	master->set_cs             = daisy_spi_set_cs;
	master->transfer_one       = daisy_spi_transfer_one;
	master->handle_err         = daisy_spi_handle_err;
	master->min_speed_hz       = MIN_SPEED_HZ;
	master->max_speed_hz       = MAX_SPEED_HZ;
#ifdef PREPARE_MESSAGE
	master->prepare_message    = daisy_spi_prepare_message;
#endif
	master->dev.of_node        = pdev->dev.of_node;

	bs = spi_master_get_devdata(master);
	spin_lock_init(&bs->transfer_lock);

	bs->clk = devm_clk_get(&pdev->dev, NULL);
	if (IS_ERR(bs->clk)) {
		err = PTR_ERR(bs->clk);
		printk(KERN_ERR DRV_NAME ": could not get clk: %d\n", err);
		goto out_master_put;
	}
	clk_prepare_enable(bs->clk);

	//bs->irq = platform_get_irq(pdev, 0);
	//if (bs->irq <= 0) {
	//	dev_err(&pdev->dev, "could not get IRQ: %d\n", bs->irq);
	//	err = bs->irq ? bs->irq : -ENODEV;
	//	goto out_master_put;
	//}

	err = devm_spi_register_master(&pdev->dev, master);
	if (err) {
		printk(KERN_ERR DRV_NAME ": Could not register SPI master: %d\n", err);
		goto out_clk_disable;
	}

	err = bcm2835_initialize(platform_get_resource(pdev, IORESOURCE_MEM, 0));
	if (err) {
		printk(KERN_ERR DRV_NAME
				": Could not initialize the BCM2835 chip: %d\n", err);
		goto out_clk_disable;
	}

	err = bcm2835_spi_begin();
	if (err) {
		printk(KERN_ERR DRV_NAME
				": Could not start SPI communication: %d\n", err);
		goto out_clk_disable;
	}

	return 0;

out_clk_disable:
	clk_disable_unprepare(bs->clk);
out_master_put:
	spi_master_put(master);
	return err;
}


static int daisy_spi_remove(struct platform_device *pdev)
{
	struct spi_master *master = platform_get_drvdata(pdev);
	struct daisy_spi  *bs     = spi_master_get_devdata(master);

	printk(KERN_DEBUG DRV_NAME ": Called spi_remove\n");

	clk_disable_unprepare(bs->clk);

	bcm2835_spi_end();
	bcm2835_release();

	return 0;
}

static const struct of_device_id daisy_spi_match[] = {
	{ .compatible = "brcm,bcm2835-spi", }, // We need this for spidev
	{ .compatible = "daisy,daisy-spi", },
	{}
};
MODULE_DEVICE_TABLE(of, daisy_spi_match);

static struct platform_driver daisy_spi_driver = {
	.driver		= {
		.name			= DRV_NAME,
		.of_match_table	= daisy_spi_match,
	},
	.probe		= daisy_spi_probe,
	.remove		= daisy_spi_remove,
};
module_platform_driver(daisy_spi_driver);

MODULE_AUTHOR("Tania Hagn - DF9RY - Tania@DF9RY.de");
MODULE_LICENSE("Dual BSD/GPL");
MODULE_DESCRIPTION("SPI driver for daisy project");
MODULE_VERSION("0.0.1");


