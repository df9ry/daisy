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
#include <linux/completion.h>
#include <linux/timer.h>

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
};

/*
 * Private data for our daisy device.
 */
struct daisy_priv {
	struct root_descriptor  *root;
	size_t                   slot;
	struct daisy_dev        *daisy_device;
	struct net_device_stats  stats;
	spinlock_t               lock;
	struct workqueue_struct *workqueue;
	struct work_struct       work;
	struct completion       *completion;
	struct timer_list        timer;
};

/*
 * Useful defaults:
 */
#define DEFAULT_TIMEOUT           10   /* In jiffies               */
#define RFM22B_TYPE_ID             8   /* SPI chip id              */
#define SPI_BUS_SPEED        5000000   /* Run with 5 MHz           */

#endif /* _DAISY_H_ */
