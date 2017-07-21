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
#include <linux/etherdevice.h>

#include "l1.h"
#include "l1_queue.h"
#include "l2.h"
#include "l2_queue.h"
#include "daisy.h"

static void worker(struct work_struct *w) {
	struct daisy_priv *priv =
			container_of(w, struct daisy_priv, l2_workstruct);
	struct net_device *dev = priv->root->net_device;
	l2_pump(dev);
}

void l2_init(struct net_device *dev) {
	struct daisy_priv *priv = netdev_priv(dev);
	memset(priv, 0x00, sizeof(struct daisy_priv));
	spin_lock_init(&priv->lock);
}

void l2_term(struct net_device *dev) {
	if (dev) {
		struct daisy_priv *priv = netdev_priv(dev);

		if (priv)
			l2_down(dev);
	}
}

int l2_up(struct net_device *dev) {
	struct daisy_priv *priv = netdev_priv(dev);
	int erc;

	spin_lock(&priv->lock);
	/**/ erc = l1_up(dev);
	/**/ if (erc)
	/**/ 	goto out;
	/**/
	/**/ erc = -ENOMEM;
	/**/ priv->rx_l2_queue = l2_queue_new(DEFAULT_L2_RX_QUEUE_SIZE);
	/**/ if (!priv->rx_l2_queue)
	/**/ 	goto out;
	/**/
	/**/ priv->tx_l2_queue = l2_queue_new(DEFAULT_L2_TX_QUEUE_SIZE);
	/**/ if (!priv->tx_l2_queue)
	/**/ 	goto out_rx_l2;
	/**/
	/**/ priv->l2_workqueue = create_singlethread_workqueue(dev->name);
	/**/ if (!priv->l2_workqueue)
	/**/ 	goto out_tx_l2;
	/**/ INIT_WORK(&priv->l2_workstruct, worker);
	/**/ erc = 0;
	/**/ goto out;
	/**/
out_tx_l2:
	/**/ l2_queue_del(priv->tx_l2_queue);
	/**/ priv->tx_l2_queue = NULL;
out_rx_l2:
	/**/ l2_queue_del(priv->rx_l2_queue);
	/**/ priv->rx_l2_queue = NULL;
out:
	spin_unlock(&priv->lock);
	if (erc == 0)
		queue_work(priv->l2_workqueue, &priv->l2_workstruct);
	return erc;
}

void l2_down(struct net_device *dev) {
	if (dev) {
		struct daisy_priv *priv = netdev_priv(dev);

		spin_lock(&priv->lock);
		/**/ destroy_workqueue(priv->l2_workqueue);
		/**/ priv->l2_workqueue = NULL;
		/**/ l2_queue_del(priv->rx_l2_queue);
		/**/ priv->rx_l2_queue = NULL;
		/**/ l2_queue_del(priv->tx_l2_queue);
		/**/ priv->tx_l2_queue = NULL;
		/**/ l1_down(dev);
		spin_unlock(&priv->lock);
	}
}

void l2_pump(struct net_device *dev) {
	struct daisy_priv *priv = netdev_priv(dev);
	bool               fdo;

	spin_lock(&priv->lock);
	do {
		fdo = 0;

		// Transmit:
		netif_trans_update(dev); // Advice kernel timer watchdog.

		while (l1_entry_can_new(priv->tx_l1_queue)) {
			struct l1_entry *e1;
			struct l2_entry *e2 = l2_entry_get(priv->tx_l2_queue);

			if (!e2)
				break;

			if (!e2->skb) {
				if (printk_ratelimit())
					printk(KERN_INFO
							"daisy: Dropping package without socket buffer\n");
				l2_entry_del(e2);
				continue;
			}

			if (e2->skb->len > PKG_SIZE) {
				if (printk_ratelimit())
					printk(KERN_INFO
							"daisy: Dropping long package (len=%d)\n",
							e2->skb->len);
				dev_kfree_skb(e2->skb);
				e2->skb = NULL;
				priv->stats.tx_dropped++;
				l2_entry_del(e2);
				continue;
			}

			e1 = l1_entry_new(priv->tx_l1_queue);
			if (!e1) {
				if (printk_ratelimit())
					printk(KERN_INFO
							"daisy: Unable to get L1 TX buffer\n");
				l2_entry_pushback(e2);
				break;
			}

			e1->len = e2->skb->len;
			memcpy(e1->pkg, e2->skb->data, e1->len);
			dev_kfree_skb(e2->skb);
			e2->skb = NULL;
			priv->stats.tx_packets ++;
			l2_entry_del(e2);
		} // end while //
		if (l2_entry_can_new(priv->tx_l2_queue))
			netif_wake_queue(dev);

		// Perform L1 IO
		l1_pump(dev);

		// Receive:
		while (l2_entry_can_new(priv->rx_l2_queue)) {
			struct l1_entry *e1 = l1_entry_get(priv->rx_l1_queue);
			struct l2_entry *e2;

			if (!e1)
				break;

			e2 = l2_entry_new(priv->rx_l2_queue);
			if (!e2) {
				if (printk_ratelimit())
					printk(KERN_INFO
							"daisy: Unable to get L2 RX buffer\n");
				l1_entry_pushback(e1);
				break;
			}

			e2->skb = dev_alloc_skb(e1->len+2);
			if (!e2->skb) {
				if (printk_ratelimit())
					printk(KERN_INFO
							"daisy: Unable to alloc socket buffer\n");
				l1_entry_pushback(e1);
				break;
			}

			memcpy(skb_put(e2->skb, e1->len), e1->pkg, e1->len);
			e2->skb->dev = dev;
			e2->skb->protocol = eth_type_trans(e2->skb, dev);
			e2->skb->ip_summed = CHECKSUM_NONE;
			l1_entry_del(e1);
			l2_entry_put(e2);
		} // end while //

		while (1) {
			struct l2_entry *e2 = l2_entry_get(priv->rx_l2_queue);
			if (!e2)
				break;
			priv->stats.rx_packets++;
			priv->stats.rx_bytes += e2->skb->len;
			netif_rx(e2->skb);
			e2->skb = NULL;
			l2_entry_del(e2);
		} // end while //
	} while (fdo);
	spin_unlock(&priv->lock);
}
