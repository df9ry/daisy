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

/*
 * Release a tx buffer.
 */
static void daisy_release_buffer(struct daisy_packet *pkt)
{
	unsigned long flags;
	struct daisy_priv *priv = netdev_priv(pkt->dev);

	spin_lock_irqsave(&priv->lock, flags);
	pkt->next = priv->ppool;
	priv->ppool = pkt;
	spin_unlock_irqrestore(&priv->lock, flags);
	if (netif_queue_stopped(pkt->dev) && pkt->next == NULL)
		netif_wake_queue(pkt->dev);
}

/*
 * Transmit a packet (low level interface)
 */
static void daisy_hw_tx(char *buf, int len, struct net_device *dev)
{
	struct daisy_priv   *priv;
	struct daisy_packet *pkt = NULL;

	/*
	 * This function deals with hw details. This interface loops
	 * back the packet to the other snull interface (if any).
	 * In other words, this function implements the snull behaviour,
	 * while all other procedures are rather device-independent
	 */

	/* I am paranoid. Ain't I? */
	if (len < sizeof(struct ethhdr) + sizeof(struct iphdr)) {
		printk("daisy: Hmm... packet too short (%i octets)\n",
				len);
		return;
	}

	if (0) { /* enable this conditional to look at the data */
		int i;
		PDEBUG("len is %i\n" KERN_DEBUG "data:",len);
		for (i=14 ; i<len; i++)
			printk(" %02x",buf[i]&0xff);
		printk("\n");
	}

	priv = netdev_priv(dev);

	/* Lock the device */
	spin_lock(&priv->lock);

	pkt = priv->rx_queue;

	/** TODO: Transmit data */

	/* a transmission is over: free the skb */
	priv->stats.tx_packets++;
	priv->stats.tx_bytes += priv->tx_packetlen;
	dev_kfree_skb(priv->skb);

	/* Unlock the device and we are done */
	spin_unlock(&priv->lock);

	if (pkt)
		daisy_release_buffer(pkt); /* Do this outside the lock! */
}

/*
 * Transmit a packet (called by the kernel)
 */
int daisy_tx(struct sk_buff *skb, struct net_device *dev)
{
	int len;
	char *data, shortpkt[ETH_ZLEN];
	struct daisy_priv *priv = netdev_priv(dev);
	struct netdev_queue *txq = netdev_get_tx_queue(dev, 0);

	data = skb->data;
	len = skb->len;
	if (len < ETH_ZLEN) {
		memset(shortpkt, 0, ETH_ZLEN);
		memcpy(shortpkt, skb->data, skb->len);
		len = ETH_ZLEN;
		data = shortpkt;
	}
	txq->trans_start = jiffies; /* save the timestamp */

	/* Remember the skb, so we can free it at interrupt time */
	priv->skb = skb;

	/* actual deliver of data is device-specific, and not shown here */
	daisy_hw_tx(data, len, dev);

	return 0; /* Our simple device can not fail */
}

/*
 * Deal with a transmit timeout.
 */
void daisy_tx_timeout (struct net_device *dev)
{
	struct daisy_priv *priv = netdev_priv(dev);
	struct daisy_packet *pkt = NULL;

	PDEBUG("Transmit timeout at %ld, latency %ld\n", jiffies,
			jiffies - dev->trans_start);

	/* Lock the device */
	spin_lock(&priv->lock);

	pkt = priv->rx_queue;
	/* a transmission is over: free the skb */
	priv->stats.tx_errors++;
	dev_kfree_skb(priv->skb);

	/* Unlock the device and we are done */
	spin_unlock(&priv->lock);

	if (pkt)
		daisy_release_buffer(pkt); /* Do this outside the lock! */

	netif_wake_queue(dev);
	return;
}



