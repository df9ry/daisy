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

#ifndef _AUTOMATON_H_
#define _AUTOMATON_H_

#include <linux/module.h>
#include <linux/etherdevice.h>

#include "spi.h"
#include "spi-daisy.h"
#include "tx_queue.h"
#include "ev_queue.h"

static inline void on_idle_poll(struct daisy_dev *dd);

static inline bool squelch_open(struct daisy_dev *dd) {
#if 0
	u16 _rssi = daisy_get_register16(dd, RFM22B_REG_RSSI);
	u8  rssi  = (_rssi > 8) & 0x00ff;
	u8  th    = _rssi & 0x00ff;
	return (rssi > th);
#endif
	return 0;
}

static inline void rx_start(struct daisy_dev *dd) {
	ev_queue_put(&dd->evq, EVQ_RXSTART);
	// Clear the RX FIFO:
	daisy_clear_rx_fifo(dd);
	// Drop the PTT and listen:
	daisy_set_register8(dd, RFM22B_REG_OP_MODE_1, RFM22B_RXON);
	dd->state = STATUS_IDLE;
	ev_queue_put(&dd->evq, EVQ_STATUS_IDLE);
	dd->timeout = jiffies + DEFAULT_TIMER_TICK;
}

static inline void tx_start(struct daisy_dev *dd) {
	int cb_to_write;
	u8 *pb_tx;

	// Support spurious interrupts:
	if (!dd->tx_entry) {
		ev_queue_put(&dd->evq, EVQ_ENTRY_NULL);
		return;
	}

	// Calculate how many octets to write now:
	cb_to_write = dd->tx_entry->pkg_len;
	if (cb_to_write > IO_MAX)
		cb_to_write = IO_MAX;

	// Fill the TX FIFO:
	pb_tx = dd->tx_entry->pkg;
	(*pb_tx) = RFM22B_REG_FIFO | RFM22B_WRITE_FLAG;
	daisy_transfer(dd, pb_tx, rx_buffer, cb_to_write);
	dd->pkg_idx = cb_to_write + 1;
	dd->state = STATUS_SEND;
	ev_queue_put_op(&dd->evq, EVQ_STATUS_SEND, cb_to_write );

	dd->timeout = jiffies + DEFAULT_TX_TIMEOUT;
}

static inline void tx_fifo(struct daisy_dev *dd) {
	int cb_to_write;
	u8 *pb_tx;

	// Support spurious interrupts:
	if (!dd->tx_entry) {
		ev_queue_put(&dd->evq, EVQ_ENTRY_NULL);
		return;
	}

	// Calculate how many octets to write now:
	cb_to_write = dd->tx_entry->pkg_len - dd->pkg_idx;
	if (cb_to_write > IO_MAX)
		cb_to_write = IO_MAX;

	// Fill the TX FIFO:
	pb_tx = &dd->tx_entry->pkg[dd->pkg_idx - 1];
	(*pb_tx) = RFM22B_REG_FIFO | RFM22B_WRITE_FLAG;
	daisy_transfer(dd, pb_tx, rx_buffer, cb_to_write);
	dd->pkg_idx = cb_to_write + 1;

	dd->timeout = jiffies + DEFAULT_TX_TIMEOUT;
}

static inline void tx_sent(struct daisy_dev *dd) {
	// Support spurious interrupts:
	if (!dd->tx_entry) {
		ev_queue_put(&dd->evq, EVQ_ENTRY_NULL);
		return;
	}

	tx_entry_del(dd->tx_entry);
	dd->tx_entry = NULL;
	on_idle_poll(dd);
}

static inline void on_idle_poll(struct daisy_dev *dd) {
	if (!tx_entry_can_get(dd->tx_queue))
		return;
	if (squelch_open(dd))
		return;
	dd->tx_entry = tx_entry_get(dd->tx_queue);
	if (!dd->tx_entry)
		return;
	dd->pkg_idx = 0;
	tx_start(dd);
}

static inline void on_send_timeout(struct daisy_dev *dd) {
	ev_queue_put(&dd->evq, EVQ_TXTIMEOUT);
	if (dd->tx_entry) {
		tx_entry_del(dd->tx_entry);
		dd->tx_entry = NULL;
	}
	on_idle_poll(dd);
}

#endif //_AUTOMATON_H_//
