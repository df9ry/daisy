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

#include "daisy.h"

/*
 * Open and close
 */

int daisy_open(struct net_device *dev)
{
	/* request_region(), request_irq(), ....  (like fops->open) */

	/*
	 * Assign the hardware address of the board: use "\0SNULx", where
	 * x is 0 or 1. The first byte is '\0' to avoid being a multicast
	 * address (the first byte of multicast addrs is odd).
	 */
	struct daisy_priv *priv = netdev_priv(dev);
	if (!priv)
		return -EFAULT;
	priv->i2c_c = daisy_i2c_client;

	/** TODO set correct adapter address */
	memcpy(dev->dev_addr, "\0DF9RY", ETH_ALEN);
	if (dev == daisy_dev)
		dev->dev_addr[ETH_ALEN-1]++;
	netif_start_queue(dev);
	return 0;
}

int daisy_release(struct net_device *dev)
{
    /* release ports, irq and such -- like fops->close */

	netif_stop_queue(dev); /* can't transmit any more */
	return 0;
}

