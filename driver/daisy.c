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

#include "daisy.h"

/*
 * I2C adapter dependency.
 */
static struct i2c_board_info __initdata board_info[] = {
	{ IC2_BOARD_INFO("RFM22B", 0x48), }
};

/*
 * Forwards.
 */
int daisy_config(struct net_device *dev, struct ifmap *map);
int daisy_release(struct net_device *dev);
int daisy_open(struct net_device *dev);
int daisy_ioctl(struct net_device *dev, struct ifreq *rq, int cmd);
struct net_device_stats *daisy_stats(struct net_device *dev);
int daisy_change_mtu(struct net_device *dev, int new_mtu);
int daisy_tx(struct sk_buff *skb, struct net_device *dev);
void daisy_tx_timeout (struct net_device *dev);

/*
 * Parameters.
 */
static int timeout   = DEFAULT_TIMEOUT;
static int pool_size = DEFAULT_POOL_SIZE;

/*
 * Private structure.
 */
static const struct net_device_ops daisy_netdev_ops = {
	.ndo_open            = daisy_open,
	.ndo_stop            = daisy_release,
	.ndo_start_xmit      = daisy_tx,
	.ndo_do_ioctl        = daisy_ioctl,
	.ndo_set_config      = daisy_config,
	.ndo_get_stats       = daisy_stats,
	.ndo_change_mtu      = daisy_change_mtu,
	.ndo_tx_timeout      = daisy_tx_timeout,
};

/*
 * Set up a device's packet pool.
 */
void daisy_setup_pool(struct net_device *dev)
{
	struct daisy_priv *priv = netdev_priv(dev);
	int i;
	struct daisy_packet *pkt;

	priv->ppool = NULL;
	for (i = 0; i < pool_size; i++) {
		pkt = kmalloc (sizeof (struct daisy_packet), GFP_KERNEL);
		if (pkt == NULL) {
			printk (KERN_NOTICE "Ran out of memory allocating packet pool\n");
			return;
		}
		pkt->dev = dev;
		pkt->next = priv->ppool;
		priv->ppool = pkt;
	}
}

/*
 * Remove pool.
 */
void daisy_teardown_pool(struct net_device *dev)
{
	struct daisy_priv *priv = netdev_priv(dev);
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
#if 0
    	/*
	 * Make the usual checks: check_region(), probe irq, ...  -ENODEV
	 * should be returned if no device found.  No resource should be
	 * grabbed: this is done on open().
	 */
#endif

    /*
	 * Then, assign other fields in dev, using ether_setup() and some
	 * hand assignments
	 */
	ether_setup(dev); /* assign some of the fields */
	dev->watchdog_timeo = timeout;
	dev->netdev_ops     = &daisy_netdev_ops;

	/*
	 * Then, initialize the priv field. This encloses the statistics
	 * and a few private fields.
	 */
	priv = netdev_priv(dev);
	memset(priv, 0, sizeof(struct daisy_priv));
	spin_lock_init(&priv->lock);
	priv->ic2_c = NULL;
	daisy_setup_pool(dev);
}

/*
 * Finally, the module stuff
 */

struct net_device  *daisy_dev         = NULL;
struct i2c_adapter *daisy_i2c_adapter = NULL;
struct i2c_client  *daisy_i2c_client  = NULL;

static void daisy_cleanup(void)
{
	if (daisy_dev) {
		unregister_netdev(daisy_dev);
		daisy_teardown_pool(daisy_dev);
		free_netdev(daisy_dev);
		daisy_dev = NULL;
	}
	if (daisy_i2c_client) {
		i2c_unregister_device(daisy_i2c_client);
		daisy_i2c_client = NULL;
	}
}

int daisy_init_module(void)
{
	int ret = -ENOMEM;
	int result;

	daisy_dev = alloc_netdev(
			sizeof(struct daisy_priv), "dsy%d",
			NET_NAME_UNKNOWN, daisy_init);
	if (!daisy_dev)
		goto out;

	ret = -ENODEV;
	if (!(daisy_i2c_adapter = i2c_get_adapter(1))) {
		printk("daisy: error registering i2c adapter \"%s\"\n",
				daisy_i2c_adapter->name);
		goto out;
	}
	if (!(daisy_i2c_client = i2c_new_device(daisy_i2c_adapter, board_info))) {
		printk("daisy: error registering i2c client \"%s\"\n",
				daisy_i2c_client->name);
		goto out;
	}
	if ((result = register_netdev(daisy_dev))) {
		printk("daisy: error %i registering network device \"%s\"\n",
				result, daisy_dev->name);
		goto out;
	}

	ret = 0;

out:
	if (ret)
		daisy_cleanup();
	return ret;
}

module_init(daisy_init_module);
module_exit(daisy_cleanup);

MODULE_AUTHOR("Tania Hagn - DF9RY");
MODULE_LICENSE("Dual BSD/GPL");
MODULE_DESCRIPTION("Driver for the Daisy interface");
MODULE_VERSION("0.1");
