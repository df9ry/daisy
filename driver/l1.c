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

#include "l1.h"
#include "l1_queue.h"
#include "daisy.h"

int l1_up(struct net_device *dev) {
	struct daisy_priv *priv = netdev_priv(dev);
	int erc = -ENOMEM;

	priv->rx_l1_queue = l1_queue_new(DEFAULT_L1_RX_QUEUE_SIZE);
	if (!priv->rx_l1_queue)
		goto out;

	priv->tx_l1_queue = l1_queue_new(DEFAULT_L1_TX_QUEUE_SIZE);
	if (!priv->tx_l1_queue)
		goto out_tx_l1;

	erc = 0;
	goto out;

out_tx_l1:
	l1_queue_del(priv->rx_l1_queue);
	priv->rx_l1_queue = NULL;
out:
	return erc;
}

void l1_down(struct net_device *dev) {
	if (dev) {
		struct daisy_priv *priv = netdev_priv(dev);

		l1_queue_del(priv->rx_l1_queue);
		priv->rx_l1_queue = NULL;
		l1_queue_del(priv->tx_l1_queue);
		priv->tx_l1_queue = NULL;
	}
}

void l1_pump(struct net_device *dev) {

}
