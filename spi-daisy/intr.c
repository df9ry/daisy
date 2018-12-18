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

#include "spi-daisy.h"
#include "spi.h"
#include "trace.h"
#include "automaton.h"

u8 tx_buffer[IO_MAX+2];
u8 rx_buffer[IO_MAX+2];

void watchdog(unsigned long _dd) {
	struct daisy_dev *dd = (struct daisy_dev *)_dd;
	unsigned long flags, expires;

	if (!dd)
		return;
	spin_lock_irqsave(&dd->evq.lock, flags);
	/**/ if (time_after(jiffies, dd->timeout)) {
	/**/	ev_queue_put(&dd->evq, EVQ_TIMEOUT);
	/**/    dd->timeout = jiffies;
	/**/ }
	/**/ expires = jiffies + DEFAULT_TIMER_TICK;
	/**/ if (timer_pending(&dd->watchdog)) {
	/**/ 	mod_timer(&dd->watchdog, expires);
	/**/ } else {
	/**/	dd->watchdog.expires = expires;
	/**/	add_timer(&dd->watchdog);
	/**/ }
	spin_unlock_irqrestore(&dd->evq.lock, flags);
	tasklet_hi_schedule(&dd->tasklet);
}

void tasklet(unsigned long _dd) {
	u32 dropped;
	struct daisy_dev *dd = (struct daisy_dev *)_dd;
	struct ev_entry   ee;
	while (ev_queue_get(&dd->evq, &ee)) {
		switch (ee.event) {
		case EVQ_EOF:
			trace1("EOF", ee.timestamp);
			break;
		case EVQ_INTERRUPT:
			trace2("INTERRUPT", ee.timestamp, ee.operand);
			break;
		case EVQ_POR:
			trace1("POR", ee.timestamp);
			break;
		case EVQ_CHIPRDY:
			trace1("CHIPRDY", ee.timestamp);
			break;
		case EVQ_LBD:
			trace1("LBD", ee.timestamp);
			break;
		case EVQ_WUT:
			trace1("WUT", ee.timestamp);
			break;
		case EVQ_RSSI:
			trace1("RSSI", ee.timestamp);
			break;
		case EVQ_PREAINVAL:
			trace1("PREAINVAL", ee.timestamp);
			break;
		case EVQ_PREAVAL:
			trace1("PREAVAL", ee.timestamp);
			break;
		case EVQ_SWDET:
			trace1("SWDET", ee.timestamp);
			break;
		case EVQ_CRCERROR:
			trace1("CRCERROR", ee.timestamp);
			break;
		case EVQ_PKVALID:
			trace1("PKVALID", ee.timestamp);
			break;
		case EVQ_PKSENT:
			trace1("PKSENT", ee.timestamp);
			break;
		case EVQ_EXT:
			trace1("EXT", ee.timestamp);
			break;
		case EVQ_RXFFAFUL:
			trace1("RXFFAFUL", ee.timestamp);
			break;
		case EVQ_TXFFAEM:
			trace1("TXFFAEM", ee.timestamp);
			break;
		case EVQ_TXFFAFULL:
			trace1("TXFFAFULL", ee.timestamp);
			break;
		case EVQ_FFERR:
			trace1("FFERR", ee.timestamp);
			break;
		case EVQ_TIMEOUT:
			//trace1("TIMEOUT", ee.timestamp);
			switch (dd->state) {
			case STATUS_IDLE:
				on_idle_poll(dd);
				break;
			case STATUS_SEND:
				on_send_timeout(dd);
				break;
			default:
				break;
			} // end switch //
			break;
		case EVQ_TXTIMEOUT:
			trace1("TXTIMEOUT", ee.timestamp);
			break;
		case EVQ_TXSTART:
			trace2("TXSTART", ee.timestamp, ee.operand);
			break;
		case EVQ_RXSTART:
			trace1("RXSTART", ee.timestamp);
			break;
		case EVQ_ENTRY_NULL:
			trace1("ENTRY_NULL", ee.timestamp);
			break;
		case EVQ_EMPTY_PKG:
			trace1("EMPTY_PKG", ee.timestamp);
			break;
		case EVQ_INVAL_WRCOUNT:
			trace1("INVAL_WRCOUNT", ee.timestamp);
			break;
		case EVQ_INVALID_STATE:
			trace1("INVALID_STATE", ee.timestamp);
			break;
		case EVQ_STATUS_IDLE:
			trace1("STATUS_IDLE", ee.timestamp);
			break;
		case EVQ_STATUS_SEND:
			trace1("STATUS_SEND", ee.timestamp);
			break;
		default:
			trace2("UNKNOWN", ee.timestamp, ee.event);
			break;
		} // end switch //
	} // end while //
	dropped = ev_queue_overruns(&dd->evq);
	if (dropped)
		printk(KERN_INFO "spi-dev: dropped %d events\n", dropped);
}

irqreturn_t irq_handler(int irq, void *_dd, struct pt_regs *regs)
{
	struct daisy_dev *dd  = (struct daisy_dev *)_dd;
	struct ev_queue  *evq = &dd->evq;
	u16               is;
	unsigned long     flags;

	local_irq_save(flags);
	is = daisy_get_register16(dd, RFM22B_REG_INTERRUPT_STATUS);
	if ((is & RFM22B_ENINTR) == 0)
		goto end;

	//ev_queue_put_op(evq, EVQ_INTERRUPT, is);
	switch (dd->state) {
	case STATUS_IDLE:
		if (is & RFM22B_ISWDET) {
			ev_queue_put(evq, EVQ_SWDET);
			///TODO: SWDET
		}
		if (is & RFM22B_ICRCERROR) {
			ev_queue_put(evq, EVQ_CRCERROR);
			///TODO: CRCERROR
		}
		if (is & RFM22B_IPKVALID) {
			ev_queue_put(evq, EVQ_PKVALID);
			///TODO: PKVALID
		}
		if (is & RFM22B_IRXFFAFUL) {
			ev_queue_put(evq, EVQ_RXFFAFUL);
			///TODO:RXFFAFUL
		}
		if (is & RFM22B_IFFERR) {
			ev_queue_put(evq, EVQ_FFERR);
			///TODO:FFERR
		}
		break;
	case STATUS_SEND:
		if (is & RFM22B_IPKSENT) {
			ev_queue_put(evq, EVQ_PKSENT);
			tx_sent(dd);
		}
		if (is & RFM22B_ITXFFAEM) {
			ev_queue_put(evq, EVQ_TXFFAEM);
			tx_fifo(dd);
		}
		if (is & RFM22B_IFFERR) {
			ev_queue_put(evq, EVQ_FFERR);
			///TODO:FFERR
		}
		break;
	default:
		ev_queue_put(evq, EVQ_INVALID_STATE);
		break;
	} // end switch //

end:
	local_irq_restore(flags);
	tasklet_hi_schedule(&dd->tasklet);

	return IRQ_HANDLED;
}

