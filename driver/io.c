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
#include <linux/etherdevice.h>

#include "daisy.h"
#include "spi-daisy.h"

static void on_timer(unsigned long _dev) {
	struct net_device *dev = (struct net_device *)_dev;
	struct daisy_priv *priv = netdev_priv(dev);

	if ((priv == NULL) || (priv->completion != NULL))
		return;
	if (daisy_can_write(priv->daisy_device)) {
		netif_wake_queue(dev);
	} else {
		priv->timer.expires = jiffies + DEFAULT_TIMEOUT;
		add_timer(&priv->timer);
	}
}

/*
 * Transmit a packet (called by the kernel)
 */
int daisy_tx(struct sk_buff *skb, struct net_device *dev)
{
	int erc;
	struct daisy_priv *priv = netdev_priv(dev);

	if (priv->completion)
		return -ERESTARTSYS;
	erc = daisy_try_write(priv->daisy_device, skb->data, skb->len, 0);
	if (erc < 0) {
		printk(KERN_ERR "daisy: TX %d octets failed with erc %d\n",
				skb->len, erc);
		 netif_stop_queue(dev);
		 priv->timer.expires = jiffies + DEFAULT_TIMEOUT;
		 priv->timer.function = on_timer;
		 priv->timer.data = (unsigned long)dev;
		 add_timer(&priv->timer);
		 return erc;
	}
	if (printk_ratelimit())
		printk(KERN_DEBUG "daisy: TX %d octets\n", skb->len);
	dev_trans_start(dev);
	priv->stats.tx_packets ++;
	priv->stats.tx_bytes += skb->len;
	dev_kfree_skb(skb);
	return 0;
}

/*
 * Deal with a transmit timeout.
 */
void daisy_tx_timeout (struct net_device *dev)
{
	if (printk_ratelimit())
		printk(KERN_DEBUG "daisy: TX timeout at %ld\n", jiffies);
}

/**
 * Receive packet worker
 */
void daisy_rx(struct work_struct *ws)
{
	u8 buf[MAX_PKG_LEN+2];
	struct daisy_priv *priv = container_of(ws, struct daisy_priv, work);
	int res = daisy_read(priv->daisy_device, buf, sizeof(buf));
	if (priv->completion) {
		printk(KERN_DEBUG "daisy: RX exit\n");
		complete(priv->completion);
		return;
	}
	if (res >= 0) {
		struct sk_buff    *skb;
		struct net_device *dev = priv->root->net_device;

		if (printk_ratelimit())
			printk(KERN_DEBUG "daisy: RX %d octets\n", res);
		skb = dev_alloc_skb(res + 2);
		if (skb) {
			memcpy(skb_put(skb, res), &buf[1], res);
			skb->dev = dev;
			skb->protocol = eth_type_trans(skb, dev);
			skb->ip_summed = 0;
			priv->stats.rx_packets++;
			priv->stats.rx_bytes += res;
			netif_rx(skb);
		} else {
			printk(KERN_ERR "daisy: RX: Unable to allocate socket buffer\n");
			priv->stats.rx_dropped++;
		}
	} else {
		printk(KERN_ERR "daisy: RX failed with erc=%d\n", res);
		priv->stats.rx_errors++;
	}
	queue_work(priv->workqueue, &priv->work);
}
