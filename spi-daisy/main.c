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
#include <linux/platform_device.h>
#include <linux/workqueue.h>
#include <linux/timer.h>

#include <linux/spi/spi.h>

#include "spi-daisy.h"
#include "bcm2835.h"
#include "rx_queue.h"
#include "rd_queue.h"
#include "tx_queue.h"

#define daisy_SPI_MODE_BITS	(SPI_CPOL | SPI_CPHA | SPI_CS_HIGH \
				| SPI_NO_CS | SPI_3WIRE)

struct daisy_spi {
	struct platform_device  *pdev;
	spinlock_t               transfer_lock;
	struct clk              *clk;
	uint32_t                 spi_hz;
	bool                     speed_lock;
	struct workqueue_struct *workqueue;
};

struct daisy_dev {
	struct kobject          *kobj;
	struct daisy_spi        *spi;
	struct spi_device       *dev;
	struct spi_master       *master;
	uint16_t                 slot;
	struct rx_queue         *rx_queue;
	struct rd_queue         *rd_queue;
	struct tx_queue         *tx_queue;
	struct work_struct       pump;
	struct timer_list        timer;
	volatile bool            quit;
};

static struct daisy_dev daisy_slots[N_SLOTS];

static void daisy_spi_handle_err(struct spi_master  *master,
                                 struct spi_message *msg)
{
	printk(KERN_DEBUG DRV_NAME ": Called spi_handle_err()\n");
}

static void rd_async_work(struct work_struct *ws)
{
	struct rd_entry *e = container_of(ws, struct rd_entry, rd_work);
	e->andthen(e->cb, e->user_data);
	rd_entry_del(e);
}

int daisy_read_async(struct daisy_dev *dd, uint8_t *pb, size_t cb,
					 void *user_data,
					 void (*andthen)(int cb_read, void *user_data))
{
	struct rd_entry *e = rd_entry_new(dd->rd_queue);
	if (!e)
		return -ENOMEM;
	e->pb = pb;
	e->cb = cb;
	e->andthen = andthen;
	e->user_data = user_data;
	INIT_WORK(&e->rd_work, rd_async_work);
	rd_entry_put(e);
	if (!dd->quit)
		queue_work(dd->spi->workqueue, &dd->pump);
	return 0;
}
EXPORT_SYMBOL_GPL(daisy_read_async);

static void rd_sync_work(struct work_struct *ws)
{
	struct rd_entry *e = container_of(ws, struct rd_entry, rd_work);
	complete((struct completion *)e->user_data);
}

int daisy_read(struct daisy_dev *dd, uint8_t *pb, size_t cb) {
	struct rd_entry   *e = rd_entry_new(dd->rd_queue);
	struct completion  completion;

	if (!e)
		return -ENOMEM;
	init_completion(&completion);
	e->pb = pb;
	e->cb = cb;
	e->user_data = &completion;
	e->andthen = NULL;
	INIT_WORK(&e->rd_work, rd_sync_work);
	rd_entry_put(e);
	if (!dd->quit)
		queue_work(dd->spi->workqueue, &dd->pump);
	wait_for_completion(&completion);
	return e->cb;
}
EXPORT_SYMBOL_GPL(daisy_read);

static void tx_async_work(struct work_struct *ws)
{
	struct tx_entry *e = container_of(ws, struct tx_entry, tx_work);
	e->andthen(e->erc, e->user_data);
	tx_entry_del(e);
}

int daisy_write_async(struct daisy_dev *dd, uint8_t *pb, size_t cb,
					  void *user_data, bool priority,
					  void (*andthen)(int cb_wrote, void *user_data))
{
	struct tx_entry *e = tx_entry_new(dd->tx_queue);
	if (!e)
		return -ENOMEM;
	e->user_data = user_data;
	e->andthen = andthen;
	e->erc = 0;
	INIT_WORK(&e->tx_work, tx_async_work);
	tx_entry_put(e, priority);
	if (!dd->quit)
		queue_work(dd->spi->workqueue, &dd->pump);
	return 0;
}
EXPORT_SYMBOL_GPL(daisy_write_async);

static void tx_sync_work(struct work_struct *ws)
{
	struct tx_entry *e = container_of(ws, struct tx_entry, tx_work);
	complete((struct completion *)e->user_data);
}

int daisy_write(struct daisy_dev *dd, uint8_t *pb, size_t cb, bool priority)
{
	struct tx_entry   *e = tx_entry_new(dd->tx_queue);
	struct completion  completion;
	int                erc;

	if (!e)
		return -ENOMEM;
	init_completion(&completion);
	e->user_data = &completion;
	e->andthen = NULL;
	e->erc = 0;
	INIT_WORK(&e->tx_work, tx_sync_work);
	tx_entry_put(e, priority);
	wait_for_completion(&completion);
	erc = e->erc;
	tx_entry_del(e);
	if (!dd->quit)
		queue_work(dd->spi->workqueue, &dd->pump);
	return (erc < 0) ? erc : cb;
}
EXPORT_SYMBOL_GPL(daisy_write);

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

static void pump(struct work_struct *ws)
{
	struct daisy_dev *dd = container_of(ws, struct daisy_dev, pump);

	if (dd->quit)
		return;

	while (rd_entry_can_new(dd->rd_queue)) {
		struct rx_entry *rx = rx_entry_get(dd->rx_queue);
		struct rd_entry *rd;

		if (!rx)
			break;
		rd = rd_entry_new(dd->rd_queue);
		if (!rd) {
			rx_entry_push_back(rx);
			break;
		}
		if (rd->cb >= rx->len) {
			memcpy(rd->pb, &rx->pkg[1], rx->len);
			rd->cb = rx->len;
		} else {
			memcpy(rd->pb, &rx->pkg[1], rd->cb);
			rd->cb = -E2BIG;
		}
		rx_entry_del(rx);
	} // end while //
}

static void timer(unsigned long _dd)
{
	struct daisy_dev *dd =(struct daisy_dev *)_dd;

	if (dd->quit)
		return;

	queue_work(dd->spi->workqueue, &dd->pump);
	dd->timer.expires += DEFAULT_PUMP_INTERVAL;
	add_timer(&dd->timer);
}

struct daisy_dev *daisy_open_device(uint16_t slot)
{
	struct daisy_dev *dd;

	printk(KERN_DEBUG DRV_NAME ": Open handle for slot %d\n", slot);
	if (slot >= N_SLOTS)
		return NULL;
	dd = &daisy_slots[slot];
	if ((dd->dev == NULL) || (dd->kobj != NULL))
		return NULL;

	dd->quit = 0;
	dd->rx_queue = rx_queue_new(DEFAULT_RX_QUEUE_SIZE);
	if (!dd->rx_queue) {
		return NULL;
	}
	dd->rd_queue = rd_queue_new(DEFAULT_RD_QUEUE_SIZE);
	if (!dd->rd_queue) {
		rx_queue_del(dd->rx_queue); dd->rx_queue = NULL;
		return NULL;
	}
	dd->tx_queue = tx_queue_new(DEFAULT_TX_QUEUE_SIZE);
	if (!dd->tx_queue) {
		rx_queue_del(dd->rx_queue); dd->rx_queue = NULL;
		rd_queue_del(dd->rd_queue); dd->rd_queue = NULL;
		return NULL;
	}
	INIT_WORK(&dd->pump, pump);
	init_timer(&dd->timer);
	dd->timer.expires = jiffies + DEFAULT_PUMP_INTERVAL;
	dd->timer.data = (unsigned long)dd;
	dd->timer.function = timer;
	dd->kobj = kobject_get(&(dd->dev->dev.kobj));
	queue_work(dd->spi->workqueue, &dd->pump);
	add_timer(&dd->timer);
	return dd;
}
EXPORT_SYMBOL_GPL(daisy_open_device);

void daisy_close_device(struct daisy_dev *dd)
{
	if ((dd != NULL) && (dd->kobj != NULL)) {
		printk(KERN_DEBUG DRV_NAME ": Close handle for slot %d\n", dd->slot);
		dd->quit = 1;
		cancel_work(&dd->pump);
		del_timer(&dd->timer);
		kobject_put(dd->kobj); dd->kobj = NULL;
		rx_queue_del(dd->rx_queue); dd->rx_queue = NULL;
		rd_queue_del(dd->rd_queue); dd->rd_queue = NULL;
		tx_queue_del(dd->tx_queue); dd->tx_queue = NULL;
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

	bs->workqueue = create_workqueue(pdev->name);
	if (!bs->workqueue) {
		printk(KERN_ERR DRV_NAME
				": Could not allocate workqueue\n");
		err = -ENOMEM;
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

	if (bs->workqueue) {
		flush_workqueue(bs->workqueue);
		destroy_workqueue(bs->workqueue);
		bs->workqueue = NULL;
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
