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
#include <linux/spinlock.h>
#include <linux/of.h>
#include <linux/netdevice.h>
#include <linux/skbuff.h>
#include <linux/platform_device.h>

#include <linux/spi/spi.h>

#include "spi-daisy.h"
#include "tx_queue.h"
#include "rx_queue.h"
#include "bcm2835.h"

#define daisy_SPI_MODE_BITS	(SPI_CPOL | SPI_CPHA | SPI_CS_HIGH \
				| SPI_NO_CS | SPI_3WIRE)

struct net_device_stats;

struct daisy_spi {
	struct platform_device  *pdev;
	spinlock_t               transfer_lock;
	struct clk              *clk;
	uint32_t                 spi_hz;
	bool                     speed_lock;
};

struct daisy_dev {
	struct kobject          *kobj;
	struct daisy_spi        *spi;
	struct spi_device       *dev;
	struct spi_master       *master;
	struct rx_queue         *rx_queue;
	struct tx_queue         *tx_queue;
	struct net_device_stats *stats;
	uint16_t                 slot;
};

static struct daisy_dev daisy_slots[N_SLOTS];

static void daisy_spi_handle_err(struct spi_master  *master,
                                 struct spi_message *msg)
{
	printk(KERN_DEBUG DRV_NAME ": Called spi_handle_err()\n");
}

void daisy_register_stats(struct daisy_dev        *dd,
						  struct net_device_stats *stats)
{
	if (dd)
		dd->stats = stats;
}
EXPORT_SYMBOL_GPL(daisy_register_stats);

struct sk_buff *daisy_read(struct daisy_dev *dd) {
	struct rx_entry *e = rx_entry_get(dd->rx_queue);
	struct sk_buff  *skb;

	if (!e)
		return NULL;
	skb = e->skb;
	e->skb = dev_alloc_skb(MAX_PKG_LEN+2);
	if (!e->skb) {
		if (printk_ratelimit())
			printk(KERN_ERR "spi-daisy: Unable to alloc socket buffer\n");
	}
	rx_entry_del(e);
	return skb;
}
EXPORT_SYMBOL_GPL(daisy_read);

int daisy_write(struct daisy_dev *dd, struct sk_buff *skb, bool priority)
{
	struct tx_entry   *e;

	if (!skb)
		return -EINVAL;
	if (skb->len > MAX_PKG_LEN) {
		if (dd->stats) {
			dd->stats->tx_errors ++;
		}
		return -E2BIG;
	}
	e = tx_entry_new(dd->tx_queue);
	if (!e)
		return -EINTR;
	e->skb = skb;
	tx_entry_put(e, priority);
	return skb->len;
}
EXPORT_SYMBOL_GPL(daisy_write);

bool daisy_can_write(struct daisy_dev *dd)
{
	return !list_empty(&dd->tx_queue->free);
}
EXPORT_SYMBOL_GPL(daisy_can_write);

int daisy_try_write(struct daisy_dev *dd, struct sk_buff *skb, bool priority)
{
	struct tx_entry   *e;

	if (!skb)
		return -EINVAL;
	if (skb->len > MAX_PKG_LEN) {
		if (dd->stats) {
			dd->stats->tx_errors ++;
		}
		return -E2BIG;
	}
	e = tx_entry_try_new(dd->tx_queue);
	if (!e)
		return -ERESTARTSYS;
	e->skb = skb;
	tx_entry_put(e, priority);
	return skb->len;
}
EXPORT_SYMBOL_GPL(daisy_try_write);

void daisy_interrupt_read(struct daisy_dev *dd)
{
	if (dd && dd->rx_queue)
		up(&dd->rx_queue->sem);
}
EXPORT_SYMBOL_GPL(daisy_interrupt_read);

struct daisy_spi *daisy_get_controller(struct daisy_dev *dev)
{
	if ((dev == NULL) || (dev->master == NULL))
		return NULL;
	return spi_master_get_devdata(dev->master);
}
EXPORT_SYMBOL_GPL(daisy_get_controller);

void daisy_lock_speed(struct daisy_spi *spi)
{
	if (spi)
		spi->speed_lock = 1;
}
EXPORT_SYMBOL_GPL(daisy_lock_speed);

void daisy_unlock_speed(struct daisy_spi *spi)
{
	if (spi)
		spi->speed_lock = 0;
}
EXPORT_SYMBOL_GPL(daisy_unlock_speed);

struct daisy_dev *daisy_open_device(uint16_t slot)
{
	struct daisy_dev *dd;

	printk(KERN_DEBUG DRV_NAME ": Open handle for slot %d\n", slot);
	if (slot >= N_SLOTS)
		return NULL;
	dd = &daisy_slots[slot];
	if ((dd->dev == NULL) || (dd->kobj != NULL))
		return NULL;

	dd->stats = NULL;
	dd->rx_queue = rx_queue_new(DEFAULT_RX_QUEUE_SIZE);
	if (!dd->rx_queue) {
		return NULL;
	}
	dd->tx_queue = tx_queue_new(DEFAULT_TX_QUEUE_SIZE);
	if (!dd->tx_queue) {
		rx_queue_del(dd->rx_queue); dd->rx_queue = NULL;
		return NULL;
	}
	dd->kobj = kobject_get(&(dd->dev->dev.kobj));
	return dd;
}
EXPORT_SYMBOL_GPL(daisy_open_device);

void daisy_close_device(struct daisy_dev *dd)
{
	if ((dd != NULL) && (dd->kobj != NULL)) {
		printk(KERN_DEBUG DRV_NAME ": Close handle for slot %d\n", dd->slot);
		kobject_put(dd->kobj); dd->kobj = NULL;
		rx_queue_del(dd->rx_queue); dd->rx_queue = NULL;
		tx_queue_del(dd->tx_queue); dd->tx_queue = NULL;
		dd->stats = NULL;
	}
}
EXPORT_SYMBOL_GPL(daisy_close_device);

uint32_t daisy_set_speed(struct daisy_spi *spi, uint32_t spi_hz)
{
	uint32_t clk_hz, cdiv;
	spi->spi_hz = spi_hz;
	/* set clock */
	clk_hz = clk_get_rate(spi->clk);
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
	return cdiv ? (clk_hz / cdiv) : (clk_hz / 65536);
}
EXPORT_SYMBOL_GPL(daisy_set_speed);

void daisy_transfer(struct daisy_dev *dd,
			const volatile uint8_t   *tx,
				  volatile uint8_t   *rx,
				  	  	   size_t     cb)
{
	struct daisy_spi *spi = dd->spi;
	unsigned long flags;

	spin_lock_irqsave(&spi->transfer_lock, flags);
	bcm2835_spi_transfernb(tx, rx, cb);
	spin_unlock_irqrestore(&spi->transfer_lock, flags);
}
EXPORT_SYMBOL_GPL(daisy_transfer);

static int daisy_spi_transfer_one(
					struct spi_master   *master,
				    struct spi_device   *dev,
				    struct spi_transfer *tfr)
{
	struct daisy_spi *spi = spi_master_get_devdata(master);
	if ((spi != NULL) && (!spi->speed_lock)) {
		uint32_t spi_hz = tfr->speed_hz;

		/* Only set speed, if this time is different than last time */
		if (spi_hz != spi->spi_hz) {
			uint32_t spi_used_hz = daisy_set_speed(spi, spi->spi_hz);
			printk(KERN_DEBUG DRV_NAME ": SPI clock req: %d Hz, got %d Hz\n",
					spi_hz, spi_used_hz);
		}
	}

	if (dev->chip_select >= N_SLOTS) {
		printk(KERN_ERR DRV_NAME ": Invalid CS: %d\n", (int)dev->chip_select);
		return -E2BIG;
	}

	daisy_transfer(&daisy_slots[dev->chip_select],
			tfr->tx_buf, tfr->rx_buf, tfr->len);

	spi_finalize_current_transfer(master);
	return 0;
}

static int daisy_spi_setup(struct spi_device *dev)
{
	struct daisy_dev *dd;

	printk(KERN_DEBUG DRV_NAME
			": Called spi_setup(cs=%d)\n", (int)dev->chip_select);
	/*
	 * sanity checking the native-chipselects
	 */
	if (dev->mode & SPI_NO_CS) {
		printk(KERN_ERR DRV_NAME
				": Setup: SPI_NO_CS not supported\n");
		return -EINVAL;
	}
	if (dev->chip_select >= N_SLOTS) {
		printk(KERN_ERR DRV_NAME
				": Setup: Only %i CS are allowed\n", N_SLOTS);
		return -EINVAL;
	}
	if (dev->master == NULL) {
		printk(KERN_ERR DRV_NAME
				": Setup: CS %i is not attached\n", dev->chip_select);
		return -EINVAL;
	}
	dd = &daisy_slots[dev->chip_select];
	if (dd->dev != NULL) {
		printk(KERN_ERR DRV_NAME
				": Setup: CS %i is in use\n", dev->chip_select);
		return -EINVAL;
	}

	dd->dev    = dev;
	dd->kobj   = NULL;
	dd->master = dev->master;
	dd->slot   = dev->chip_select;
	dd->spi    = spi_master_get_devdata(dev->master);

	return 0;
}

static int daisy_spi_probe(struct platform_device *pdev)
{
	struct spi_master  *master;
	struct daisy_spi   *bs;
	int                 err, i;

	printk(KERN_DEBUG DRV_NAME ": Called spi_probe()\n");
	master = spi_alloc_master(&pdev->dev, sizeof(*bs));
	if (!master) {
		printk(KERN_ERR DRV_NAME ": spi_alloc_master() failed\n");
		return -ENOMEM;
	}

	platform_set_drvdata(pdev, master);

	master->mode_bits          = daisy_SPI_MODE_BITS;
	master->bits_per_word_mask = SPI_BPW_MASK(8);
	master->num_chipselect     = N_SLOTS;
	master->setup              = daisy_spi_setup;
	master->transfer_one       = daisy_spi_transfer_one;
	master->handle_err         = daisy_spi_handle_err;
	master->min_speed_hz       = MIN_SPEED_HZ;
	master->max_speed_hz       = MAX_SPEED_HZ;
	master->dev.of_node        = pdev->dev.of_node;

	bs = spi_master_get_devdata(master);
	bs->pdev = pdev;
	spin_lock_init(&bs->transfer_lock);

	bs->clk = devm_clk_get(&pdev->dev, NULL);
	if (IS_ERR(bs->clk)) {
		err = PTR_ERR(bs->clk);
		printk(KERN_ERR DRV_NAME ": could not get clk: %d\n", err);
		goto out_master_put;
	}
	clk_prepare_enable(bs->clk);

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
	for (i = 0; i < N_SLOTS; ++i) {
		daisy_close_device(&daisy_slots[i]);
		daisy_slots[i].dev = NULL;
	}
	return err;
}

static int daisy_spi_remove(struct platform_device *pdev)
{
	struct spi_master *master = platform_get_drvdata(pdev);
	struct daisy_spi  *bs     = spi_master_get_devdata(master);
	int                i;

	printk(KERN_DEBUG DRV_NAME ": Called spi_remove\n");

	clk_disable_unprepare(bs->clk);

	for (i = 0; i < N_SLOTS; ++i) {
		daisy_close_device(&daisy_slots[i]);
		daisy_slots[i].dev = NULL;
	}

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
