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
#include "l2.h"
#include "l2_queue.h"

/*
 * Transmit a packet (called by the kernel)
 */
int daisy_tx(struct sk_buff *skb, struct net_device *dev)
{
	struct daisy_priv *priv = netdev_priv(dev);
	struct l2_entry   *e    = l2_entry_new(priv->tx_l2_queue);

	if (!e) {
		printk(KERN_DEBUG "daisy: Unable to get TX entry\n");
		netif_stop_queue(dev);
		return -EAGAIN;
	}
	printk(KERN_DEBUG "daisy: L2 enqueue TX %d octets\n", skb->len);
	e->skb = skb;
	l2_entry_put(e);
	l2_pump(dev);
	if (!l2_entry_can_new(priv->tx_l2_queue))
		 netif_stop_queue(dev);
	return 0;
}

/*
 * Deal with a transmit timeout.
 */
void daisy_tx_timeout (struct net_device *dev)
{
	printk(KERN_DEBUG "daisy: TX timeout at %ld\n", jiffies);
	l2_pump(dev);
	return;
}
