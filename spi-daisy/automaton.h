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

	if (!dd->pkg_ptr) {
		ev_queue_put(&dd->evq, EVQ_PKG_NULL);
		tx_entry_del(dd->tx_entry);
		dd->tx_entry = NULL;
		if (dd->stats) {
			dd->stats->tx_errors ++;
		}
		rx_start(dd);
		return;
	}
	// Calculate how many octets to write now:
	cb_to_write = (dd->pkg_len > IO_MAX) ? IO_MAX : dd->pkg_len;
	if (cb_to_write == 0) {
		ev_queue_put(&dd->evq, EVQ_EMPTY_PKG);
		tx_entry_del(dd->tx_entry);
		dd->tx_entry = NULL;
		rx_start(dd);
		return;
	}
	if ((cb_to_write < 0) || (cb_to_write > IO_MAX)) {
		ev_queue_put(&dd->evq, EVQ_INVAL_WRCOUNT);
		tx_entry_del(dd->tx_entry);
		dd->tx_entry = NULL;
		if (dd->stats) {
			dd->stats->tx_errors ++;
			dd->stats->tx_dropped ++;
		}
		rx_start(dd);
		return;
	}
	ev_queue_put_op(&dd->evq, EVQ_TXSTART, cb_to_write);
	// Clear the TX FIFO:
	daisy_clear_tx_fifo(dd);
	// Push the PTT:
	daisy_set_register8(dd, RFM22B_REG_OP_MODE_1, RFM22B_TXON);
	// Fill the TX FIFO:
	daisy_set_register8(dd, RFM22B_TXPKLEN, dd->pkg_len);
	tx_buffer[0] = RFM22B_REG_FIFO | RFM22B_WRITE_FLAG;
	memcpy(&tx_buffer[1], dd->pkg_ptr, cb_to_write);
	memset(&rx_buffer[0], 0x55, cb_to_write + 1);
	daisy_transfer(dd, tx_buffer, rx_buffer, cb_to_write);
	dd->state = STATUS_SEND;
	ev_queue_put_op(&dd->evq, EVQ_STATUS_SEND, cb_to_write );
	dd->timeout = jiffies + DEFAULT_TX_TIMEOUT;
}

static inline void on_idle_poll(struct daisy_dev *dd) {
	if (!tx_entry_can_get(dd->tx_queue))
		return;
	if (squelch_open(dd))
		return;
	dd->tx_entry = tx_entry_get(dd->tx_queue);
	if (!dd->tx_entry)
		return;
	if (!dd->tx_entry->skb) {
		ev_queue_put(&dd->evq, EVQ_SKB_NULL);
		tx_entry_del(dd->tx_entry);
		dd->tx_entry = NULL;
		if (dd->stats) {
			dd->stats->tx_dropped ++;
			dd->stats->tx_errors ++;
		}
		return;
	}
	dd->pkg_len = dd->tx_entry->skb->len;
	dd->pkg_idx = 0;
	dd->pkg_ptr = dd->tx_entry->skb->data;
	tx_start(dd);
}

static inline void on_send_timeout(struct daisy_dev *dd) {
	ev_queue_put(&dd->evq, EVQ_TXTIMEOUT);
	if (dd->tx_entry) {
		tx_entry_del(dd->tx_entry);
		dd->tx_entry = NULL;
		if (dd->stats) {
			dd->stats->tx_aborted_errors ++;
			dd->stats->tx_dropped ++;
			dd->stats->tx_errors ++;
		}
	}
	rx_start(dd);
}

#endif //_AUTOMATON_H_//
