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
#include <linux/interrupt.h>
#include <linux/gpio.h>

#include "spi-daisy.h"

#define IO_MAX 64

static u8 tx[IO_MAX+2];
static u8 rx[IO_MAX+2];

irqreturn_t irq_handler(int irq, void *_dev, struct pt_regs *regs)
{

	unsigned long flags;

	local_irq_save(flags);

	local_irq_restore(flags);
	return IRQ_HANDLED;
}
