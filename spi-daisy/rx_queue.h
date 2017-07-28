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

#ifndef _RX_QUEUE_H_
#define _RX_QUEUE_H_

#include <linux/module.h>
#include <linux/list.h>
#include <linux/spinlock.h>
#include <linux/semaphore.h>

#include "spi-daisy.h"

struct rx_queue;
struct sk_buff;

/**
 * rx_entry in rx_queue.
 */
struct rx_entry {
	struct list_head   list;
	struct rx_queue   *queue;
	struct sk_buff    *skb;
};

/**
 * rx_queue. Do never touch directly.
 */
struct rx_queue {
	struct list_head   free;
	struct list_head   fifo;
	struct semaphore   sem;
	spinlock_t         lock;
	struct rx_entry    data[0]; // Hack: dynamically allocation
};

/**
 * Create a new rx_queue. This call is not intended to be used within
 * contexts that can not sleep.
 * @param size Number of entries available in the rx_queue.
 * @return Pointer to new rx_queue.
 * @error  Return NULL, if there was a problem to allocate the rx_queue.
 */
struct rx_queue *rx_queue_new(size_t size);

/**
 * Free rx_queue earlier created with rx_queue_new(). It is not
 * necessary to check q for NULL, this is handled by the call correctly.
 * However it can not be used within contexts that can not sleep.
 * @param q Pointer to rx_queue.
 */
void rx_queue_del(struct rx_queue *q);

/**
 * Alloc a new rx_entry to later put to the rx_queue. Do never try to
 * delete this rx_entry. Use rx_entry_put or rx_entry_del() to return
 * this rx_entry to the rx_queue.
 * @param q Pointer to the rx_queue.
 * @return Pointer to the new rx_entry.
 * @error  return NULL, if no more rx_entry is available.
 */
static inline struct rx_entry *rx_entry_new(struct rx_queue *q) {
	struct rx_entry  *e = NULL;
	unsigned long     flags;

	spin_lock_irqsave(&q->lock, flags);
	/**/ if (!list_empty(&q->free)) {
	/**/ 	struct list_head *_e = q->free.next;
	/**/ 	e = list_entry(_e, struct rx_entry, list);
	/**/ 	list_del_init(_e);
	/**/ }
	spin_unlock_irqrestore(&q->lock, flags);
	return e;
}

/**
 * Return rx_entry to the rx_queue when it is no longer of use, so that it can
 * be reused later.
 * @param e Pointer to the rx_entry to return.
 */
static inline void rx_entry_del(struct rx_entry *e) {
	struct rx_queue *q = e->queue;
	unsigned long    flags;

	spin_lock_irqsave(&q->lock, flags);
	/**/ list_add_tail(&q->free, &e->list);
	spin_unlock_irqrestore(&q->lock, flags);
}

/**
 * Put rx_entry to the end of the input FIFO, so that it will be processed
 * after all entries put before.
 * @param e Pointer to the rx_entry to put.
 */
static inline void rx_entry_put(struct rx_entry *e) {
	struct rx_queue *q = e->queue;
	unsigned long    flags;

	spin_lock_irqsave(&q->lock, flags);
	/**/ list_add_tail(&q->fifo, &e->list);
	spin_unlock_irqrestore(&q->lock, flags);
	up(&q->sem);
}

/**
 * Get a new rx_entry to be processed from the rx_queue. Do never try to
 * delete this rx_entry. Use rx_entry_del() to return this rx_entry to the
 * rx_queue.
 * @param q Pointer to the rx_queue.
 * @return Pointer to the rx_entry got from the rx_queue.
 * @error  return NULL, if no rx_entry is available.
 */
static inline struct rx_entry *rx_entry_get(struct rx_queue *q) {
	struct rx_entry  *e = NULL;
	int               d = down_interruptible(&q->sem);
	unsigned long     flags;

	if (d)
		return NULL;
	spin_lock_irqsave(&q->lock, flags);
	/**/ if (!list_empty(&q->fifo)) {
	/**/ 	struct list_head *_e = q->fifo.next;
	/**/ 	e = list_entry(_e, struct rx_entry, list);
	/**/ 	list_del_init(_e);
	/**/ }
	spin_unlock_irqrestore(&q->lock, flags);
	return e;
}

#endif /* _RX_QUEUE_H_ */
