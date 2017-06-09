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

#ifndef DAISY_H_
#define DAISY_H_

#undef PDEBUG             /* undef it, just in case */
#ifdef SNULL_DEBUG
#  ifdef __KERNEL__
     /* This one if debugging is on, and kernel space */
#    define PDEBUG(fmt, args...) printk( KERN_DEBUG "daisy: " fmt, ## args)
#  else
     /* This one for user space */
#    define PDEBUG(fmt, args...) fprintf(stderr, fmt, ## args)
#  endif
#else
#  define PDEBUG(fmt, args...) /* not debugging: nothing */
#endif

#undef PDEBUGG
#define PDEBUGG(fmt, args...) /* nothing: it's a placeholder */

#include <linux/netdevice.h>
#include <linux/i2c.h>

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
 * Private data for our daisy device.
 */
struct daisy_priv {
	struct net_device_stats stats;
	struct daisy_packet    *ppool;
	struct daisy_packet    *rx_queue;  /* List of incoming packets */
	int                     tx_packetlen;
	u8                     *tx_packetdata;
	struct sk_buff         *skb;
	spinlock_t              lock;
	struct ic2_client      *ic2_c;
};

/*
 *  Default timeout period.
 */
#define DEFAULT_TIMEOUT 5   /* In jiffies */

/*
 * Default pool size.
 */
#define DEFAULT_POOL_SIZE 8

/*
 * Pointer to our I2C client struct.
 */
extern struct i2c_client *daisy_i2c_client;

/*
 * Pointer to our net_device struct.
 */
extern struct net_device *daisy_dev;

#endif /* DAISY_H_ */
