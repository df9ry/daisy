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
#include <linux/string.h>
#include <linux/errno.h>
#include <linux/ip.h>

#include "daisy.h"
#include "spi-daisy.h"

/*
 * Transmit a packet (called by the kernel)
 */
int daisy_tx(struct sk_buff *skb, struct net_device *dev)
{
	struct daisy_priv *priv = netdev_priv(dev);
	int erc = daisy_try_write(priv->root->daisy_device, skb->data, skb->len, 0);
	if (erc < 0) {
		printk(KERN_DEBUG "daisy: TX %d octets failed with erc %d\n",
				skb->len, erc);
		 netif_stop_queue(dev);
		 return erc;
	}
	printk(KERN_DEBUG "daisy: TX %d octets\n", skb->len);
	priv->stats.tx_packets ++;
	priv->stats.tx_bytes += skb->len;
	return 0;
}

/*
 * Deal with a transmit timeout.
 */
void daisy_tx_timeout (struct net_device *dev)
{
	printk(KERN_DEBUG "daisy: TX timeout at %ld\n", jiffies);
}
