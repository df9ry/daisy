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

#ifndef _BCM2835_H_
#define _BCM2835_H_

#include <linux/module.h>

/*
 * Initialize the BCM2835.
 */
extern int bcm2835_initialize(struct resource *res);

/*
 * Release the BCM2835.
 */
extern void bcm2835_release(void);

/*
 * Begin SPI communication:
 */
extern int bcm2835_spi_begin(void);

/*
 * Shutdown SPI communication:
 */
extern void bcm2835_spi_end(void);

/*
 * Set the SPI communication speed:
 */
extern void bcm2835_spi_setClockDivider(uint16_t divider);

/*
 * SPI transfer:
 */
extern void bcm2835_spi_transfernb(const volatile uint8_t *tx,
										 volatile uint8_t *rx, size_t cb);

#endif /* _BCM2835_H_ */
