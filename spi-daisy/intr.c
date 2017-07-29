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

#define IO_MAX 64

void tasklet(unsigned long _dd) {
	u32 dropped;
	struct daisy_dev *dd = (struct daisy_dev *)_dd;
	u8 event = ev_queue_get(&dd->evq);
	while (event != EVQ_EOF) {
		switch (event) {
		case EVQ_EOF:
			trace("EOF");
			break;
		case EVQ_POR:
			trace("POR");
			break;
		case EVQ_CHIPRDY:
			trace("CHIPRDY");
			break;
		case EVQ_LBD:
			trace("LBD");
			break;
		case EVQ_WUT:
			trace("WUT");
			break;
		case EVQ_RSSI:
			trace("RSSI");
			break;
		case EVQ_PREAINVAL:
			trace("PREAINVAL");
			break;
		case EVQ_PREAVAL:
			trace("PREAVAL");
			break;
		case EVQ_SWDET:
			trace("SWDET");
			break;
		case EVQ_CRCERROR:
			trace("CRCERROR");
			break;
		case EVQ_PKVALID:
			trace("PKVALID");
			break;
		case EVQ_PKSENT:
			trace("PKSENT");
			break;
		case EVQ_EXT:
			trace("EXT");
			break;
		case EVQ_RXFFAFUL:
			trace("RXFFAFUL");
			break;
		case EVQ_TXFFAEM:
			trace("TXFFAEM");
			break;
		case EVQ_TXFFAFULL:
			trace("TXFFAFULL");
			break;
		case EVQ_FFERR:
			trace("FFERR");
			break;
		case EVQ_WATCHDOG:
			//trace("WATCHDOG");
			break;
		} // end switch //
		event = ev_queue_get(&dd->evq);
	} // end while //
	dropped = ev_queue_overruns(&dd->evq);
	if (dropped)
		printk(KERN_INFO "spi-dev: dropped %d events\n", dropped);
}

void watchdog(unsigned long _dd) {
	struct daisy_dev *dd = (struct daisy_dev *)_dd;
	unsigned long flags;

	spin_lock_irqsave(&dd->evq.lock, flags);
	/**/ if ((!time_before(jiffies, dd->timeout)) ||
	/**/	 (!timer_pending(&dd->watchdog)))
	/**/ {
	/**/ 	ev_queue_put(&dd->evq, EVQ_WATCHDOG);
	/**/	dd->timeout = jiffies + DEFAULT_WATCHDOG;
	/**/	if (timer_pending(&dd->watchdog)) {
	/**/ 		mod_timer(&dd->watchdog, dd->timeout);
	/**/	} else {
	/**/		dd->watchdog.expires = dd->timeout;
	/**/ 		add_timer(&dd->watchdog);
	/**/	}
	/**/ }
	spin_unlock_irqrestore(&dd->evq.lock, flags);
	tasklet_hi_schedule(&dd->tasklet);
}

irqreturn_t irq_handler(int irq, void *_dd, struct pt_regs *regs)
{
	struct daisy_dev *dd  = (struct daisy_dev *)_dd;
	struct ev_queue  *evq = &dd->evq;
	u16               is;
	unsigned long     flags1, flags2;

	local_irq_save(flags1);
	/**/ is = daisy_get_register16(dd, RFM22B_REG_INTERRUPT_STATUS);
	/**/ spin_lock_irqsave(&evq->lock, flags2);
	/****/ if (is & RFM22B_POR      ) _ev_queue_put(evq, EVQ_POR      );
	/****/ if (is & RFM22B_CHIPRDY  ) _ev_queue_put(evq, EVQ_CHIPRDY  );
	/****/ if (is & RFM22B_LBD      ) _ev_queue_put(evq, EVQ_LBD      );
	/****/ if (is & RFM22B_WUT      ) _ev_queue_put(evq, EVQ_WUT      );
	/****/ if (is & RFM22B_RSSI     ) _ev_queue_put(evq, EVQ_RSSI     );
	/****/ if (is & RFM22B_PREAINVAL) _ev_queue_put(evq, EVQ_PREAINVAL);
	/****/ if (is & RFM22B_PREAVAL  ) _ev_queue_put(evq, EVQ_PREAVAL  );
	/****/ if (is & RFM22B_SWDET    ) _ev_queue_put(evq, EVQ_SWDET    );
	/****/ if (is & RFM22B_CRCERROR ) _ev_queue_put(evq, EVQ_CRCERROR );
	/****/ if (is & RFM22B_PKVALID  ) _ev_queue_put(evq, EVQ_PKVALID  );
	/****/ if (is & RFM22B_PKSENT   ) _ev_queue_put(evq, EVQ_PKSENT   );
	/****/ if (is & RFM22B_EXT      ) _ev_queue_put(evq, EVQ_EXT      );
	/****/ if (is & RFM22B_RXFFAFUL ) _ev_queue_put(evq, EVQ_RXFFAFUL );
	/****/ if (is & RFM22B_TXFFAEM  ) _ev_queue_put(evq, EVQ_TXFFAEM  );
	/****/ if (is & RFM22B_TXFFAFULL) _ev_queue_put(evq, EVQ_TXFFAFULL);
	/****/ if (is & RFM22B_FFERR    ) _ev_queue_put(evq, EVQ_FFERR    );
	/****/ dd->timeout = jiffies + DEFAULT_WATCHDOG;
	/****/ if (timer_pending(&dd->watchdog)) {
	/****/ 		mod_timer(&dd->watchdog, dd->timeout);
	/****/ } else {
	/****/ 		dd->watchdog.expires = dd->timeout;
	/****/ 		add_timer(&dd->watchdog);
	/****/ }
	/**/ spin_unlock_irqrestore(&evq->lock, flags2);
	local_irq_restore(flags1);
	tasklet_hi_schedule(&dd->tasklet);

	return IRQ_HANDLED;
}

