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

#ifndef _EV_QUEUE_H_
#define _EV_QUEUE_H_

#include <linux/module.h>
#include <linux/spinlock.h>

enum evq_event {
	EVQ_EOF,
	EVQ_INTERRUPT,
	EVQ_POR,
	EVQ_CHIPRDY,
	EVQ_LBD,
	EVQ_WUT,
	EVQ_RSSI,
	EVQ_PREAINVAL,
	EVQ_PREAVAL,
	EVQ_SWDET,
	EVQ_CRCERROR,
	EVQ_PKVALID,
	EVQ_PKSENT,
	EVQ_EXT,
	EVQ_RXFFAFUL,
	EVQ_TXFFAEM,
	EVQ_TXFFAFULL,
	EVQ_FFERR,
	EVQ_TIMEOUT,
	EVQ_TXTIMEOUT,
	EVQ_TXSTART,
	EVQ_RXSTART,
	EVQ_ENTRY_NULL,
	EVQ_SKB_NULL,
	EVQ_PKG_NULL,
	EVQ_EMPTY_PKG,
	EVQ_INVAL_WRCOUNT,
	EVQ_INVALID_STATE,
	EVQ_STATUS_IDLE,
	EVQ_STATUS_SEND,
};

struct ev_entry {
	u16             timestamp;
	u16             event;
	u16             operand;
};

struct ev_queue {
	u8              head;
	u8              tail;
	u32             ovrr;
	spinlock_t      lock;
	struct ev_entry data[256];
};

static inline void ev_queue_init(struct ev_queue *q)
{
	memset(q, 0x00, sizeof(struct ev_queue));
	q->head = 0;
	q->tail = 1;
	q->ovrr = 0;
	spin_lock_init(&q->lock);
}

static inline void ev_queue_put_op(struct ev_queue *q, enum evq_event v, u16 op)
{
	struct ev_entry e;
	unsigned long flags;

	e.event     = v;
	e.operand   = op;
	e.timestamp = jiffies;
	spin_lock_irqsave(&q->lock, flags);
	/**/ if (q->tail != q->head)
	/**/	memcpy(&q->data[q->tail++], &e, sizeof(struct ev_entry));
	/**/ else
	/**/	++q->ovrr;
	spin_unlock_irqrestore(&q->lock, flags);
}

#define ev_queue_put(Q,V) ev_queue_put_op(Q,V,0);

static inline bool ev_queue_get(struct ev_queue *q, struct ev_entry *e) {
	unsigned long flags;
	bool          result = 0;

	spin_lock_irqsave(&q->lock, flags);
	/**/ if (((q->head + 1) & 0xff) != q->tail) {
	/**/ 	memcpy(e, &q->data[++q->head], sizeof(struct ev_entry));
	/**/	result = 1;
	/**/ }
	spin_unlock_irqrestore(&q->lock, flags);
	return result;
}

static inline u32 ev_queue_overruns(struct ev_queue *q) {
	unsigned long flags;
	u32           result;

	spin_lock_irqsave(&q->lock, flags);
	/**/ result = q->ovrr;
	/**/ q->ovrr = 0;
	spin_unlock_irqrestore(&q->lock, flags);
	return result;
}

#endif //_EV_QUEUE_H_//
