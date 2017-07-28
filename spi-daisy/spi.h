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

#ifndef _SPI_H_
#define _SPI_H_

#include <linux/interrupt.h>

#define GPIO_PIN     17 ///TODO: Investigate correct number
#define GPIO_DESC    "DAISY Interrupt line"

#define daisy_SPI_MODE_BITS	(SPI_CPOL | SPI_CPHA | SPI_CS_HIGH \
				| SPI_NO_CS | SPI_3WIRE)

struct net_device_stats;

struct daisy_spi {
	struct platform_device  *pdev;
	spinlock_t               transfer_lock;
	struct clk              *clk;
	uint32_t                 spi_hz;
	bool                     speed_lock;
};

struct daisy_dev {
	struct kobject          *kobj;
	struct daisy_spi        *spi;
	struct spi_device       *dev;
	struct spi_master       *master;
	struct rx_queue         *rx_queue;
	struct tx_queue         *tx_queue;
	struct net_device_stats *stats;
	uint16_t                 slot;
	short int                irq;
};

extern irqreturn_t irq_handler(int irq, void *_dev, struct pt_regs *regs);

#endif //_SPI_H_//
