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
#include "spi.h"

#define IO_MAX 64


irqreturn_t irq_handler(int irq, void *_dd, struct pt_regs *regs)
{
	//static u8     tx_scratch[IO_MAX+2];
	//static u8     rx_scratch[IO_MAX+2];
	unsigned long flags;
	struct        daisy_dev *dd = (struct daisy_dev *)_dd;
	u16           intr_status   = daisy_get_register16(dd,
									RFM22B_REG_INTERRUPT_STATUS);
	local_irq_save(flags);

	switch (dd->state) {
	case STATUS_IDLE:
		goto status_idle;
	} // end switch //

#include "status_idle.h"

out:
	local_irq_restore(flags);

	return IRQ_HANDLED;
}

