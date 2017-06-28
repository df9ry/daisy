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

#ifndef _DAISY_H_
#define _DAISY_H_

#include <linux/netdevice.h>

/*
 * A structure representing an in-flight packet.
 */
struct daisy_packet {
	struct daisy_packet *next;
	struct net_device   *dev;
	int	                 datalen;
	u8                   data[ETH_DATA_LEN];
};

/*
 * Top level data structure in this driver.
 */
struct root_descriptor {
	struct net_device *daisy_dev;
};

/*
 * Private data for our daisy device.
 */
struct daisy_priv {
	struct net_device_stats stats;
	struct daisy_packet    *ppool;
	struct daisy_packet    *rx_queue;  /* List of incoming packets */
	int                     tx_packetlen;
	u8                     *tx_packetdata;
	struct sk_buff         *skb;
	struct root_descriptor *root;
	spinlock_t              lock;
};

/*
 * Useful defaults:
 */
#define DEFAULT_TIMEOUT    5   /* In jiffies              */
#define SPI_CLOCK_DIVIDER 64   /* SPI communication speed */
#define RFM22B_TYPE_ID     8   /* SPI chip id             */

/*
 * Default pool size.
 */
#define DEFAULT_POOL_SIZE 8

#endif /* _DAISY_H_ */
