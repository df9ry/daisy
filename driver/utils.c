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
#include <linux/sockios.h>

#include "daisy.h"

/* IOCTL COMMANDS */
#define IOCTL_TRANSFER_B (SIOCDEVPRIVATE + 0)

/*
 * Configuration changes (passed on by ifconfig)
 */

struct mac_addr { /* Looks like that this kernel send it this way */
	short family;
	u8    a[ETH_ALEN];
};

int daisy_set_mac_address(struct net_device *dev, void *addr)
{
	struct mac_addr *_addr = addr;
	const char hex[17] = "0123456789abcdef";
	char       buf[3*ETH_ALEN];
	int        i, j, a;

	if (dev->flags & IFF_UP)
		return -EBUSY;

	for (i = 0; i < ETH_ALEN; ++i, j+=3) {
		a = _addr->a[i];
		buf[j  ] = hex[a / 16];
		buf[j+1] = hex[a % 16];
		buf[j+2] = ':';
	}
	buf[3*ETH_ALEN-1] = '\0';
	printk(KERN_DEBUG "daisy: Set HW addr on \"%s\" to %s\n",
			dev->name, buf);
	memcpy(dev->dev_addr, _addr->a, ETH_ALEN);
	return 0;
}

/*
 * Ioctl commands
 */
int daisy_ioctl(struct net_device *dev, struct ifreq *rq, int cmd)
{
	switch (cmd) {
	case IOCTL_TRANSFER_B :
		printk(KERN_DEBUG "IOCTL transfer byte\n");
		rq->ifr_ifru.ifru_ivalue = -rq->ifr_ifru.ifru_ivalue;
		break;
	default:
		printk(KERN_DEBUG "Undefined ioctl command 0x%x\n", cmd);
		return -EOPNOTSUPP;
	} // end switch //
	return 0;
}

/*
 * Return statistics to the caller
 */
struct net_device_stats *daisy_stats(struct net_device *dev)
{
	struct daisy_priv *priv = netdev_priv(dev);
	return &priv->stats;
}

/*
 * The "change_mtu" method is usually not needed.
 * If you need it, it must be like this.
 */
int daisy_change_mtu(struct net_device *dev, int new_mtu)
{
	unsigned long flags;
	struct daisy_priv *priv = netdev_priv(dev);
	spinlock_t *lock = &priv->lock;

	/* check ranges */
	if ((new_mtu < 68) || (new_mtu > 1500))
		return -EINVAL;
	/*
	 * Do anything you need, and the accept the value
	 */
	spin_lock_irqsave(lock, flags);
	dev->mtu = new_mtu;
	spin_unlock_irqrestore(lock, flags);
	return 0; /* success */
}




