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

#include <linux/module.h>
#include <linux/netdevice.h>
#include <linux/workqueue.h>

/*
 * Forward declaration of the daisy device handle.
 */
struct daisy_dev;
struct l2_queue;
struct l1_queue;

/*
 * Top level data structure in this driver.
 */
struct root_descriptor {
	struct net_device *net_device;
	struct daisy_dev  *daisy_device;
};

/*
 * Private data for our daisy device.
 */
struct daisy_priv {
	struct l2_queue         *rx_l2_queue;
	struct l2_queue         *tx_l2_queue;
	struct workqueue_struct *l2_workqueue;
	struct work_struct       l2_workstruct;
	struct l1_queue         *rx_l1_queue;
	struct l1_queue         *tx_l1_queue;
	struct root_descriptor  *root;
	spinlock_t               lock;
	struct net_device_stats  stats;
};

/*
 * Useful defaults:
 */
#define DEFAULT_TIMEOUT            5   /* In jiffies               */
#define RFM22B_TYPE_ID             8   /* SPI chip id              */
#define DEFAULT_L1_TX_QUEUE_SIZE 128   /* Default L1 TX queue size */
#define DEFAULT_L1_RX_QUEUE_SIZE 128   /* Default L1 RX queue size */
#define DEFAULT_L2_TX_QUEUE_SIZE  16   /* Default L2 TX queue size */
#define DEFAULT_L2_RX_QUEUE_SIZE  16   /* Default L2 RX queue size */
#define SPI_BUS_SPEED        5000000   /* Run with 5 MHz           */

#endif /* _DAISY_H_ */
