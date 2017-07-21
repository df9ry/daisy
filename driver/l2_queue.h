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

#ifndef _L2_QUEUE_H_
#define _L2_QUEUE_H_

#include <linux/module.h>
#include <linux/list.h>
#include <linux/slab.h>
#include <linux/skbuff.h>
#include <linux/spinlock.h>

struct l2_queue;
struct sk_buff;

/**
 * Entry in l2_queue.
 */
struct l2_entry {
	struct list_head  list;
	struct l2_queue  *queue;
	struct sk_buff   *skb;
};

/**
 * L2 queue. Do never touch directly.
 */
struct l2_queue {
	struct list_head  free;
	struct list_head  fifo;
	spinlock_t        lock;
	struct l2_entry   data[0]; // Hack: dynamically allocation
};

/**
 * Create a new L2 queue. This call is not intended to be used within
 * contexts that can not sleep.
 * @param size Number of entries available in the queue.
 * @return Pointer to new queue.
 * @error  Return NULL, if there was a problem to allocate the queue.
 */
struct l2_queue *l2_queue_new(size_t size);

/**
 * Free L2 queue earlier created with l2_queue_new(). It is not
 * necessary to check q for NULL, this is handled by the call correctly.
 * However it can not be used within contexts that can not sleep.
 * @param q Pointer to L2 queue.
 */
void l2_queue_del(struct l2_queue *q);

/**
 * Check if a subsequent l2_entry_new() would succeed.
 * @param q Pointer to the queue to check.
 * @return Value != 0 if a subsequent l2_entry_new() would succeed.
 */
static inline int l2_entry_can_new(struct l2_queue *q) {
	int empty;
	spin_lock(&q->lock);
	/**/ empty = list_empty(&q->free);
	spin_unlock(&q->lock);
	return !empty;
}

/**
 * Alloc a new entry to later put to the queue. Do never try to
 * delete this entry. Use l2_entry_put or l2_entry_del() to return
 * this entry to the queue.
 * @param q Pointer to the L2 queue.
 * @return Pointer to the new L2 entry.
 * @error  return NULL, if no more L2 entry is available.
 */
static inline struct l2_entry *l2_entry_new(struct l2_queue *q) {
	struct l2_entry  *e = NULL;
	spin_lock(&q->lock);
	/**/ if (!list_empty(&q->free)) {
	/**/ 	struct list_head *_e = q->free.next;
	/**/ 	e = list_entry(_e, struct l2_entry, list);
	/**/ 	list_del_init(_e);
	/**/ }
	spin_unlock(&q->lock);
	return e;
}

/**
 * Return L2 entry to the queue when it is no longer of use, so that it can
 * be reused later.
 * @param e Pointer to the entry to return.
 */
static inline void l2_entry_del(struct l2_entry * e) {
	struct l2_queue *q = e->queue;
	spin_lock(&q->lock);
	/**/ list_add_tail(&q->free, &e->list);
	/**/ if (e->skb) {
	/**/	dev_kfree_skb(e->skb);
	/**/	e->skb = NULL;
	/**/ }
	spin_unlock(&q->lock);
}

/**
 * Put L2 entry to the end of the input FIFO, so that it will be processed
 * after all entries put before.
 * @param e Pointer to the entry to put.
 */
static inline void l2_entry_put(struct l2_entry * e) {
	struct l2_queue *q = e->queue;
	spin_lock(&q->lock);
	/**/ list_add_tail(&q->fifo, &e->list);
	spin_unlock(&q->lock);
}

/**
 * Check if a subsequent l2_entry_get() would succeed.
 * @param q Pointer to the queue to check.
 * @return Value != 0 if a subsequent l2_entry_get() would succeed.
 */
static inline int l2_entry_can_get(struct l2_queue *q) {
	int empty;
	spin_lock(&q->lock);
	/**/ empty = list_empty(&q->fifo);
	spin_unlock(&q->lock);
	return !empty;
}

/**
 * Get a new entry to be processed from the queue. Do never try to
 * delete this entry. Use l2_entry_del() to return this entry to the queue.
 * @param q Pointer to the L2 queue.
 * @return Pointer to the L2 entry got from the queue.
 * @error  return NULL, if no L2 entry is available.
 */
static inline struct l2_entry *l2_entry_get(struct l2_queue *q) {
	struct l2_entry  *e = NULL;
	spin_lock(&q->lock);
	/**/ if (!list_empty(&q->fifo)) {
	/**/ 	struct list_head *_e = q->fifo.next;
	/**/ 	e = list_entry(_e, struct l2_entry, list);
	/**/ 	list_del_init(_e);
	/**/ }
	spin_unlock(&q->lock);
	return e;
}

/**
 * Undoes a recent l2_entry_get. Can be used to undo the get if something
 * happened after the last get.
 * @param q Pointer to the queue.
 * @param e Pointer to the entry to put.
 */
static inline void l2_entry_pushback(struct l2_entry * e) {
	struct l2_queue *q = e->queue;
	spin_lock(&q->lock);
	/**/ list_add(&q->fifo, &e->list);
	spin_unlock(&q->lock);
}

#endif /* _L2_QUEUE_H_ */
