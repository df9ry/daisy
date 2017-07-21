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

#ifndef _L2_H_
#define _L2_H_

#include <linux/module.h>

struct net_device;

/**
 * This method must be called before any other call of L2 methods.
 * @param dev The device this L2 instance is attached to.
 */
void l2_init(struct net_device *dev);

/**
 * This method must be called before removing the device.
 * @param dev The device this L2 instance is attached to.
 */
void l2_term(struct net_device *dev);

/**
 * Initialize the net_device on interface UP.
 * @param dev Net device to UP.
 * @result Negative error code in the case of an error, 0 if good.
 */
int l2_up(struct net_device *dev);

/**
 * Stop net_device on interface DOWN.
 * @param dev Net device to DOWN.
 */
void l2_down(struct net_device *dev);

/**
 * Start or restart net device operation. Can and should  be called any time
 * to ensure propper operation of the device.
 */
void l2_pump(struct net_device *dev);

#endif /* _L2_H_ */
