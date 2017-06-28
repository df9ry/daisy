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

#include "daisy.h"
#include "bcm2835.h"
#include "initdata.h"

MODULE_AUTHOR("Tania Hagn - DF9RY");
MODULE_LICENSE("Dual BSD/GPL");
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

static int daisy_open(struct net_device *dev);
static int daisy_stop(struct net_device *dev);

/*
 * Parameters.
 */
static int timeout   = DEFAULT_TIMEOUT;
static int pool_size = DEFAULT_POOL_SIZE;

/*
 * Definition of root array.
 */
static struct root_descriptor *root    = NULL;
static size_t                  n_roots = 0;

/*
 * Private structure.
 */
static const struct net_device_ops daisy_netdev_ops = {
	.ndo_open            = daisy_open,
	.ndo_stop            = daisy_stop,
	.ndo_start_xmit      = daisy_tx,
	.ndo_do_ioctl        = daisy_ioctl,
	.ndo_set_mac_address = daisy_set_mac_address,
	.ndo_get_stats       = daisy_stats,
	.ndo_change_mtu      = daisy_change_mtu,
	.ndo_tx_timeout      = daisy_tx_timeout,
};

/*
 * Set up a device's packet pool.
 */
static void daisy_setup_pool(struct net_device *dev)
{
	struct daisy_priv   *priv = netdev_priv(dev);
	struct daisy_packet *pkt;
	int i;

	priv->ppool = NULL;
	for (i = 0; i < pool_size; i++) {
		pkt = kmalloc (sizeof (struct daisy_packet), GFP_KERNEL);
		if (pkt == NULL) {
			printk(KERN_NOTICE
					"daisy: Ran out of memory allocating packet pool\n");
			return;
		}
		pkt->dev    = dev;
		pkt->next   = priv->ppool;
		priv->ppool = pkt;
	}
}

/*
 * Remove pool.
 */
static void daisy_teardown_pool(struct net_device *dev)
{
	struct daisy_priv   *priv = netdev_priv(dev);
	struct daisy_packet *pkt;

	while ((pkt = priv->ppool)) {
		priv->ppool = pkt->next;
		kfree (pkt);
	}
	while ((pkt = priv->rx_queue)) {
		priv->rx_queue = pkt->next;
		kfree (pkt);
	}
}

/*
 * The init function (sometimes called probe).
 * It is invoked by register_netdev()
 */
static void daisy_init(struct net_device *dev)
{
	struct daisy_priv *priv;

	printk(KERN_DEBUG "daisy: Setup new net device\n");

	ether_setup(dev);
	dev->watchdog_timeo = timeout;
	dev->netdev_ops     = &daisy_netdev_ops;
	priv = netdev_priv(dev);
	memset(priv, 0, sizeof(struct daisy_priv));
	spin_lock_init(&priv->lock);
	daisy_setup_pool(dev);

	printk(KERN_DEBUG "daisy: Net device has been setup\n");
}

static int daisy_open(struct net_device *dev)
{
	//struct daisy_priv      *priv;
	//struct root_descriptor *rd;

	printk(KERN_DEBUG "daisy: Open net device \"%s\"\n", dev->name);

	netif_start_queue(dev);
	return 0;
}

static int daisy_stop(struct net_device *dev)
{
	//struct daisy_priv      *priv;
	//struct root_descriptor *rd;

	if (dev) {
		printk(KERN_DEBUG "daisy: Stop net device \"%s\"\n", dev->name);
		netif_stop_queue(dev);
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
		if (rd->daisy_dev) {
			printk(KERN_DEBUG "daisy: Close net device \"%s\"\n",
					rd->daisy_dev->name);
			daisy_stop(rd->daisy_dev);
			unregister_netdev(rd->daisy_dev);
			daisy_teardown_pool(rd->daisy_dev);
			printk(KERN_DEBUG "daisy: Free net device \"%s\"\n",
					rd->daisy_dev->name);
			free_netdev(rd->daisy_dev);
			rd->daisy_dev = NULL;
		}
	} // end while //
	printk(KERN_DEBUG "daisy: Free root array\n");
	kfree(root);
	root = NULL;
	n_roots = 0;
	bcm2835_spi_end();
	bcm2835_release();
	printk(KERN_DEBUG "daisy: Cleanup finished\n");
}

static int daisy_init_module(void)
{
	int erc;
	int result, i;
	struct root_descriptor *pd;
	struct daisy_priv      *priv;

	printk(KERN_DEBUG "daisy: Initializing\n");
	if (n_roots) {
		printk(KERN_ERR "daisy: Multiple init_module calls!\n");
		erc = -ENOENT;
		goto out;
	}

	n_roots = 1; // For now, we have only one SPI device.

	/* Initialize the BCM2835 */
	erc = bcm2835_initialize();
	if (erc) {
		printk(KERN_ERR
				"daisy: Unable to initialize BCM2835!\n");
		goto out;
	}

	/* Start the SPI subsystem */
	if (!bcm2835_spi_begin()) {
		printk(KERN_ERR
				"daisy: Unable to start SPI!\n");
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
					"daisy: RFM22B chip not present - read: %i!\n",
					(int)rx[1]);
			goto out;
		}
		printk(KERN_INFO
				"daisy: Found RFM22B version %d.\n", (int)rx[2]);

		// Issue a chip reset:
		tx[0] = 0x87;
		bcm2835_spi_transfernb(tx, rx, 2);
		msleep(10); // Let the device time to settle up

		// Setup the device with default data:
		for (i = 0; init_data[i][0] != 0x00; ++i)
			bcm2835_spi_transfernb(init_data[i], rx, 2);
		msleep(10); // Let the device time to settle up
	}


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
		if (!(pd->daisy_dev = alloc_netdev(
				sizeof(struct daisy_priv), "dsy%d",
				NET_NAME_UNKNOWN, daisy_init)))
		{
			printk(KERN_ERR	"daisy: Unable to allocate net device\n");
			erc = -ENODEV;
			goto out;
		}

		if ((result = register_netdev(pd->daisy_dev))) {
			printk(KERN_ERR
					"daisy: Error %i registering network device \"%s\"\n",
					result, pd->daisy_dev->name);
			erc = -ENODEV;
			goto out;
		}
		printk(KERN_DEBUG "daisy: Registered net device \"%s\"\n",
				pd->daisy_dev->name);

		priv = netdev_priv(pd->daisy_dev);
		if (!priv) {
			printk(KERN_ERR
					"daisy: Unable to get private data for \"%s\"\n",
					pd->daisy_dev->name);
			erc = -ENOMEM;
			goto out;
		}
		priv->root = pd;
	} // end for //

	erc = 0;

out:
	if (erc)
		daisy_cleanup();

	bcm2835_release();

	printk(KERN_DEBUG "daisy: Initialize finished with ERC=%i\n", erc);
	return erc;
}

module_init(daisy_init_module);
module_exit(daisy_cleanup);

