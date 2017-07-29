
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
#include <linux/init.h>
#include <linux/string.h>
#include <linux/errno.h>
#include <linux/stat.h>
#include <linux/slab.h>
#include <linux/semaphore.h>

#include "spi-daisy.h"
#include "daisy.h"

MODULE_AUTHOR("Tania Hagn - DF9RY");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Driver for the Daisy interface");
MODULE_VERSION("0.1.1");

/*
 * Forwards.
 */
int daisy_set_mac_address(struct net_device *dev, void *addr);
int daisy_ioctl(struct net_device *dev, struct ifreq *rq, int cmd);
struct net_device_stats *daisy_stats(struct net_device *dev);
int daisy_change_mtu(struct net_device *dev, int new_mtu);
int daisy_tx(struct sk_buff *skb, struct net_device *dev);
void daisy_tx_timeout (struct net_device *dev);
void daisy_rx(struct work_struct *ws);

static int daisy_up(struct net_device *dev);
static int daisy_down(struct net_device *dev);

/*
 * Parameters.
 */
static int timeout   = DEFAULT_TIMEOUT;

/*
 * Definition of root array.
 */
static struct root_descriptor *root    = NULL;
static size_t                  n_roots = 0;

/*
 * Private structure.
 */
static const struct net_device_ops daisy_netdev_ops = {
	.ndo_open            = daisy_up,
	.ndo_stop            = daisy_down,
	.ndo_start_xmit      = daisy_tx,
	.ndo_do_ioctl        = daisy_ioctl,
	.ndo_set_mac_address = daisy_set_mac_address,
	.ndo_get_stats       = daisy_stats,
	.ndo_change_mtu      = daisy_change_mtu,
	.ndo_tx_timeout      = daisy_tx_timeout,
};

/*
 * The init function (sometimes called probe).
 * It is invoked by register_netdev()
 */
static void daisy_init(struct net_device *dev)
{
	printk(KERN_DEBUG "daisy: Setup new net device\n");
	ether_setup(dev);
	dev->watchdog_timeo = timeout;
	dev->netdev_ops     = &daisy_netdev_ops;
	printk(KERN_DEBUG "daisy: Net device has been setup\n");
}

static int daisy_up(struct net_device *dev)
{
	int erc = 0;
	struct daisy_priv *priv = netdev_priv(dev);
	struct daisy_spi  *daisy_spi;
	u32 speed;
	int id;

	printk(KERN_DEBUG "daisy: Net device up \"%s\"\n", dev->name);
	priv->completion = NULL;
	init_timer(&priv->timer);

	// Open daisy device:
	priv->daisy_device = daisy_open_device(priv->slot);
	if (!priv->daisy_device) {
		printk(KERN_ERR	"daisy: Unable to open daisy device %d\n", priv->slot);
		erc = -ENODEV;
		goto out_exit;
	}
	daisy_register_stats(priv->daisy_device, &priv->stats);

	// Lock and set SPI bus speed:
	daisy_spi = daisy_get_controller(priv->daisy_device);
	if (!daisy_spi) {
		printk(KERN_ERR
				"daisy: Unable to get controller for daisy device %d\n",
				priv->slot);
		erc = -ENODEV;
		goto out_close_device;
	}
	daisy_lock_speed(daisy_spi);
	speed = daisy_set_speed(daisy_spi, SPI_BUS_SPEED);
	printk(KERN_DEBUG "daisy: SPI bus speed set to %d kHz\n",
			speed / 1000);

	// Test if there really is a RFM22B chip on the line:
	id = daisy_get_register8(priv->daisy_device, 0);
	if (id != RFM22B_TYPE_ID) {
		printk(KERN_ERR
				"daisy: RFM22B chip not detected (id=0x%02x)\n", id);
		erc = -ENODEV;
		goto out_unlock_speed;
	}
	printk(KERN_DEBUG "daisy: Found RFM22B version %d\n",
			(int)daisy_get_register8(priv->daisy_device, 1));

	// Allocate workqueue:
	priv->workqueue = create_singlethread_workqueue(dev->name);
	if (!priv->workqueue) {
		printk(KERN_ERR
				"daisy: Unable to create workqueue %s\n", dev->name);
		erc = -ENOMEM;
		goto out_close_device;
	}

	// Init the worker:
	INIT_WORK(&priv->work, daisy_rx);

	// Start Transmit:
	netif_start_queue(dev);

	// Start Receive:
	queue_work(priv->workqueue, &priv->work);

	// Start hardware:
	daisy_device_up(priv->daisy_device);

	erc = 0;
	goto out_exit;

out_unlock_speed:
	daisy_unlock_speed(daisy_spi);
out_close_device:
	daisy_close_device(priv->daisy_device);
	priv->daisy_device = NULL;
out_exit:
	return erc;
}

static int daisy_down(struct net_device *dev)
{
	if (dev) {
		struct daisy_priv *priv = netdev_priv(dev);

		printk(KERN_DEBUG "daisy: Net device down \"%s\"\n", dev->name);
		daisy_device_down(priv->daisy_device);
		del_timer(&priv->timer);
		netif_stop_queue(dev);

		// Stop the receive loop:
		if (priv->workqueue) {
			struct completion completion;

			// Stop the receive loop:
			init_completion(&completion);
			priv->completion = &completion;
			daisy_interrupt_read(priv->daisy_device);
			wait_for_completion(&completion);
			flush_workqueue(priv->workqueue);
			destroy_workqueue(priv->workqueue);
			priv->workqueue = NULL;
		}

		// Close daisy device:
		if (priv->daisy_device) {
			struct daisy_spi *daisy_spi =
					daisy_get_controller(priv->daisy_device);

			// Unlock bus speed:
			if (daisy_spi)
				daisy_unlock_speed(daisy_spi);
			daisy_close_device(priv->daisy_device);
			priv->daisy_device = NULL;
		}
	}
	return 0;
}

/*
 * Finally, the module stuff
 */

static void daisy_cleanup(void)
{
	struct root_descriptor *rd;
	int i;

	printk(KERN_DEBUG "daisy: Cleanup start\n");
	for (i = 0, rd = root; i < n_roots; ++i, ++rd) {
		if (rd->net_device) {
			printk(KERN_DEBUG "daisy: Close net device \"%s\"\n",
					rd->net_device->name);
			daisy_down(rd->net_device);
			unregister_netdev(rd->net_device);
			printk(KERN_DEBUG "daisy: Free net device \"%s\"\n",
					rd->net_device->name);
			free_netdev(rd->net_device);
			rd->net_device = NULL;
		}
	} // end while //
	printk(KERN_DEBUG "daisy: Free root array\n");
	kfree(root);
	root = NULL;
	n_roots = 0;
	printk(KERN_DEBUG "daisy: Cleanup finished\n");
}

static int __init daisy_init_module(void)
{
	int                     erc, result, i;
	struct root_descriptor *pd;
	struct daisy_priv      *priv;

	printk(KERN_DEBUG "daisy: Initializing\n");
	if (n_roots) {
		printk(KERN_ERR "daisy: Multiple init_module calls!\n");
		erc = -ENOENT;
		goto out;
	}

	n_roots = 1; // For now, we have only one SPI device.

	/* Allocate memory for the root array. */
	if (!(root = kcalloc(sizeof(struct root_descriptor), n_roots, GFP_KERNEL)))
	{
		printk(KERN_ERR
				"daisy: Unable to allocate memory for root array!\n");
		erc = -ENOMEM;
		goto out;
	}

	/* Open network devices */
	for (i = 0, pd = root; i < n_roots; ++i, ++pd) {
		if (!(pd->net_device = alloc_netdev(
				sizeof(struct daisy_priv), "dsy%d",
				NET_NAME_UNKNOWN, daisy_init)))
		{
			printk(KERN_ERR	"daisy: Unable to allocate net device\n");
			erc = -ENODEV;
			goto out;
		}

		if ((result = register_netdev(pd->net_device))) {
			printk(KERN_ERR
					"daisy: Error %i registering network device \"%s\"\n",
					result, pd->net_device->name);
			erc = -ENODEV;
			goto out;
		}
		printk(KERN_DEBUG "daisy: Registered net device \"%s\"\n",
				pd->net_device->name);

		priv = netdev_priv(pd->net_device);
		if (!priv) {
			printk(KERN_ERR
					"daisy: Unable to get private data for \"%s\"\n",
					pd->net_device->name);
			erc = -ENOMEM;
			goto out;
		}
		priv->root = pd;
		priv->slot = i;
	} // end for //

	erc = 0;

out:
	if (erc)
		daisy_cleanup();
	printk(KERN_DEBUG "daisy: Initialize finished with ERC=%i\n", erc);
	return erc;
}

module_init(daisy_init_module);
module_exit(daisy_cleanup);

