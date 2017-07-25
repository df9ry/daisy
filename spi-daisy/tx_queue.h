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

#ifndef _TX_QUEUE_H_
#define _TX_QUEUE_H_

#include <linux/module.h>
#include <linux/list.h>
#include <linux/spinlock.h>
#include <linux/semaphore.h>

#include "spi-daisy.h"

struct tx_queue;

/**
 * tx_entry in tx_queue.
 */
struct tx_entry {
	struct list_head   list;
	struct tx_queue   *queue;
	u8                 pkg[MAX_PKG_LEN + 2];
	int                len;
};

/**
 * tx_queue. Do never touch directly.
 */
struct tx_queue {
	struct list_head   free;
	struct list_head   fifo;
	struct list_head   prio;
	struct semaphore   sem;
	spinlock_t         lock;
	struct tx_entry    data[0]; // Hack: dynamically allocation
};

/**
 * Create a new tx_queue. This call is not intended to be used within
 * contexts that can not sleep.
 * @param size Number of entries available in the tx_queue.
 * @return Pointer to new tx_queue.
 * @error  Return NULL, if there was a problem to allocate the tx_queue.
 */
struct tx_queue *tx_queue_new(size_t size);

/**
 * Free tx_queue earlier created with tx_queue_new(). It is not
 * necessary to check q for NULL, this is handled by the call correctly.
 * However it can not be used within contexts that can not sleep.
 * @param q Pointer to tx_queue.
 */
void tx_queue_del(struct tx_queue *q);

/**
 * Alloc a new tx_entry to later put to the tx_queue. Do never try to
 * delete this tx_entry. Use tx_entry_put or tx_entry_del() to return
 * this tx_entry to the tx_queue.
 * @param q Pointer to the tx_queue.
 * @return Pointer to the new tx_entry.
 * @error  return NULL, if no more tx_entry is available.
 */
static inline struct tx_entry *tx_entry_new(struct tx_queue *q) {
	struct tx_entry  *e = NULL;
	int               d = down_interruptible(&q->sem);
	unsigned long     flags;

	if (d)
		return NULL;
	spin_lock_irqsave(&q->lock, flags);
	/**/ if (!list_empty(&q->free)) {
	/**/ 	struct list_head *_e = q->free.next;
	/**/ 	e = list_entry(_e, struct tx_entry, list);
	/**/ 	list_del_init(_e);
	/**/ }
	spin_unlock_irqrestore(&q->lock, flags);
	return e;
}

/**
 * Return tx_entry to the tx_queue when it is no longer of use, so that it can
 * be reused later.
 * @param e Pointer to the tx_entry to return.
 */
static inline void tx_entry_del(struct tx_entry *e) {
	struct tx_queue *q = e->queue;
	unsigned long    flags;

	spin_lock_irqsave(&q->lock, flags);
	/**/ list_add_tail(&q->free, &e->list);
	spin_unlock_irqrestore(&q->lock, flags);
	up(&q->sem);
}

/**
 * Put tx_entry to the end of the input FIFO, so that it will be processed
 * after all entries put before.
 * @param e Pointer to the tx_entry to put.
 */
static inline void tx_entry_put(struct tx_entry *e, bool prio) {
	struct tx_queue *q = e->queue;
	unsigned long    flags;

	spin_lock_irqsave(&q->lock, flags);
	/**/ if (prio)
	/**/ 	list_add_tail(&q->prio, &e->list);
	/**/ else
	/**/ 	list_add_tail(&q->fifo, &e->list);
	spin_unlock_irqrestore(&q->lock, flags);
}

/**
 * Get a new tx_entry to be processed from the tx_queue. Do never try to
 * delete this tx_entry. Use tx_entry_del() to return this tx_entry to the
 * tx_queue.
 * @param q Pointer to the tx_queue.
 * @return Pointer to the tx_entry got from the tx_queue.
 * @error  return NULL, if no tx_entry is available.
 */
static inline struct tx_entry *tx_entry_get(struct tx_queue *q) {
	struct tx_entry  *e = NULL;
	unsigned long     flags;

	spin_lock_irqsave(&q->lock, flags);
	/**/ if (!list_empty(&q->prio)) {
	/**/ 	struct list_head *_e = q->prio.next;
	/**/ 	e = list_entry(_e, struct tx_entry, list);
	/**/ 	list_del_init(_e);
	/**/ } else if (!list_empty(&q->fifo)) {
	/**/ 	struct list_head *_e = q->fifo.next;
	/**/ 	e = list_entry(_e, struct tx_entry, list);
	/**/ 	list_del_init(_e);
	/**/ }
	spin_unlock_irqrestore(&q->lock, flags);
	return e;
}

#endif /* _TX_QUEUE_H_ */
