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
#include <linux/ioctl.h>
#include <linux/wireless.h>

#include "daisy.h"

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
int daisy_ioctl(struct net_device *dev, struct ifreq *rq, uint32_t cmd)
{
	switch (cmd) {
	case SIOCGIWESSID :
		printk(KERN_DEBUG "daisy: ioctl(SIOCGIWESSID) - ignored\n");
		return -EOPNOTSUPP;
	default:
		printk(KERN_DEBUG "daisy: Undefined ioctl command 0x%08x\n", cmd);
		{
			const char *dir  = "";

			switch (_IOC_DIR(cmd)) {
			case _IOC_WRITE | _IOC_READ :
				dir = "WR";
				break;
			case _IOC_READ :
				dir = "R";
				break;
			case _IOC_WRITE :
				dir = "W";
				break;
			} // end switch //
			if (_IOC_SIZE(cmd) > 0)
				printk(KERN_DEBUG "daisy: _IO%s('%c'(0x%02x),%d,%d);\n", dir,
						_IOC_TYPE(cmd), _IOC_TYPE(cmd), _IOC_NR(cmd),
						_IOC_SIZE(cmd));
			else
				printk(KERN_DEBUG "daisy: _IO%s('%c'(0x%02x),%d);\n", dir,
						_IOC_TYPE(cmd), _IOC_TYPE(cmd), _IOC_NR(cmd));
			return -EOPNOTSUPP;
		}
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
	//if ((new_mtu < 68) || (new_mtu > 1500))
	if ((new_mtu < 68) || (new_mtu > 240))
		return -EINVAL;
	/*
	 * Do anything you need, and the accept the value
	 */
	spin_lock_irqsave(lock, flags);
	dev->mtu = new_mtu;
	spin_unlock_irqrestore(lock, flags);
	return 0; /* success */
}




