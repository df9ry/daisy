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

#ifndef _L1_QUEUE_H_
#define _L1_QUEUE_H_

#include <linux/module.h>
#include <linux/list.h>
#include <linux/slab.h>
#include <linux/spinlock.h>

#include "l1.h"

struct l1_queue;

/**
 * Entry in l1_queue.
 */
struct l1_entry {
	struct list_head  list;
	struct l1_queue  *queue;
	u8                pkg[PKG_SIZE+1];
	u16               len;
};

/**
 * L1 queue. Do never touch directly.
 */
struct l1_queue {
	struct list_head  free;
	struct list_head  fifo;
	struct list_head  prio;
	spinlock_t        lock;
	struct l1_entry   data[0]; // Hack: dynamically allocation
};

/**
 * Create a new L1 queue. This call is not intended to be used within
 * contexts that can not sleep.
 * @param size Number of entries available in the queue.
 * @return Pointer to new queue.
 * @error  Return NULL, if there was a problem to allocate the queue.
 */
struct l1_queue *l1_queue_new(size_t size);

/**
 * Free L1 queue earlier created with l1_queue_new(). It is not
 * necessary to check q for NULL, this is handled by the call correctly.
 * However it can not be used within contexts that can not sleep.
 * @param q Pointer to L1 queue.
 */
void l1_queue_del(struct l1_queue *q);

/**
 * Check if a subsequent l1_entry_new() would succeed.
 * @param q Pointer to the queue to check.
 * @return Value != 0 if a subsequent l1_entry_new() would succeed.
 */
static inline int l1_entry_can_new(struct l1_queue *q) {
	int empty;
	spin_lock(&q->lock);
	/**/ empty = list_empty(&q->free);
	spin_unlock(&q->lock);
	return !empty;
}

/**
 * Alloc a new entry to later put to the queue. Do never try to
 * delete this entry. Use l1_entry_put or l1_entry_del() to return
 * this entry to the queue.
 * @param q Pointer to the L1 queue.
 * @return Pointer to the new L1 entry.
 * @error  return NULL, if no more L1 entry is available.
 */
static inline struct l1_entry *l1_entry_new(struct l1_queue *q) {
	struct l1_entry  *e = NULL;
	unsigned long     flags;
	spin_lock_irqsave(&q->lock, flags);
	/**/ if (!list_empty(&q->free)) {
	/**/ 	struct list_head *_e = q->free.next;
	/**/ 	e = list_entry(_e, struct l1_entry, list);
	/**/ 	list_del_init(_e);
	/**/ }
	spin_unlock_irqrestore(&q->lock, flags);
	return e;
}

/**
 * Return L1 entry to the queue when it is no longer of use, so that it can
 * be reused later.
 * @param e Pointer to the entry to return.
 */
static inline void l1_entry_del(struct l1_entry * e) {
	struct l1_queue *q = e->queue;
	unsigned long flags;
	spin_lock_irqsave(&q->lock, flags);
	/**/ list_add_tail(&q->free, &e->list);
	spin_unlock_irqrestore(&q->lock, flags);
}

/**
 * Put L1 entry to the end of the input FIFO, so that it will be processed
 * after all entries put before.
 * @param e Pointer to the entry to put.
 */
static inline void l1_entry_put(struct l1_entry * e) {
	struct l1_queue *q = e->queue;
	unsigned long flags;
	spin_lock_irqsave(&q->lock, flags);
	/**/ list_add_tail(&q->fifo, &e->list);
	spin_unlock_irqrestore(&q->lock, flags);
}

/**
 * Check if a subsequent l1_entry_get() would succeed.
 * @param q Pointer to the queue to check.
 * @return Value != 0 if a subsequent l1_entry_get() would succeed.
 */
static inline int l1_entry_can_get(struct l1_queue *q) {
	int empty;
	spin_lock(&q->lock);
	/**/ empty = (list_empty(&q->fifo) && list_empty(&q->prio));
	spin_unlock(&q->lock);
	return !empty;
}

/**
 * Put L1 entry to the end of the input FIFO, so that it will be processed
 * after all entries put before.
 * @param e Pointer to the entry to put.
 */
static inline void l1_entry_put_prio(struct l1_entry * e) {
	struct l1_queue *q = e->queue;
	unsigned long flags;
	spin_lock_irqsave(&q->lock, flags);
	/**/ list_add_tail(&q->prio, &e->list);
	spin_unlock_irqrestore(&q->lock, flags);
}

/**
 * Get a new entry to be processed from the queue. Do never try to
 * delete this entry. Use l2_entry_del() to return this entry to the queue.
 * @param q Pointer to the L2 queue.
 * @return Pointer to the L2 entry got from the queue.
 * @error  return NULL, if no L2 entry is available.
 */
static inline struct l1_entry *l1_entry_get(struct l1_queue *q) {
	struct l1_entry  *e = NULL;
	unsigned long flags;
	spin_lock_irqsave(&q->lock, flags);
	/**/ if (!list_empty(&q->prio)) {
	/**/ 	struct list_head *_e = q->prio.next;
	/**/ 	e = list_entry(_e, struct l1_entry, list);
	/**/ 	list_del_init(_e);
	/**/ } else if (!list_empty(&q->fifo)) {
	/**/ 	struct list_head *_e = q->fifo.next;
	/**/ 	e = list_entry(_e, struct l1_entry, list);
	/**/ 	list_del_init(_e);
	/**/ }
	spin_unlock_irqrestore(&q->lock, flags);
	return e;
}

/**
 * Undoes a recent l1_entry_get. Can be used to undo the get if something
 * happened after the last get.
 * @param e Pointer to the entry to pushback.
 */
static inline void l1_entry_pushback(struct l1_entry * e) {
	struct l1_queue *q = e->queue;
	unsigned long flags;
	spin_lock_irqsave(&q->lock, flags);
	/**/ list_add(&q->prio, &e->list);
	spin_unlock_irqrestore(&q->lock, flags);
}

#endif /* _L1_QUEUE_H_ */
