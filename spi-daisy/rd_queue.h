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

#ifndef _RD_QUEUE_H_
#define _RD_QUEUE_H_

#include <linux/module.h>
#include <linux/list.h>
#include <linux/spinlock.h>
#include <linux/completion.h>
#include <linux/workqueue.h>

struct rd_queue;

/**
 * rd_entry in rd_queue.
 */
struct rd_entry {
	struct list_head   list;
	struct rd_queue   *queue;
	struct work_struct rd_work;
	void             (*andthen)(int cb_read, void *user_data);
	void              *user_data;
	u8                *pb;
	int                cb;
};

/**
 * rd_queue. Do never touch directly.
 */
struct rd_queue {
	struct list_head   free;
	struct list_head   fifo;
	spinlock_t         lock;
	struct rd_entry    data[0]; // Hack: dynamically allocation
};

/**
 * Create a new rd_queue. This call is not intended to be used within
 * contexts that can not sleep.
 * @param size Number of entries available in the rd_queue.
 * @return Pointer to new rd_queue.
 * @error  Return NULL, if there was a problem to allocate the rd_queue.
 */
struct rd_queue *rd_queue_new(size_t size);

/**
 * Free rd_queue earlier created with rd_queue_new(). It is not
 * necessary to check q for NULL, this is handled by the call correctly.
 * However it can not be used within contexts that can not sleep.
 * @param q Pointer to rd_queue.
 */
void rd_queue_del(struct rd_queue *q);

/**
 * Check if a subsequent rd_entry_new() would succeed.
 * @param q Pointer to the rd_queue to check.
 * @return Value != 0 if a subsequent rd_entry_new() would succeed.
 */
static inline int rd_entry_can_new(struct rd_queue *q) {
	int empty;
	spin_lock(&q->lock);
	/**/ empty = list_empty(&q->free);
	spin_unlock(&q->lock);
	return !empty;
}

/**
 * Alloc a new rd_entry to later put to the rd_queue. Do never try to
 * delete this rd_entry. Use rd_entry_put or rd_entry_del() to return
 * this rd_entry to the rd_queue.
 * @param q Pointer to the rd_queue.
 * @return Pointer to the new rd_entry.
 * @error  return NULL, if no more rd_entry is available.
 */
static inline struct rd_entry *rd_entry_new(struct rd_queue *q) {
	struct rd_entry  *e = NULL;
	spin_lock(&q->lock);
	/**/ if (!list_empty(&q->free)) {
	/**/ 	struct list_head *_e = q->free.next;
	/**/ 	e = list_entry(_e, struct rd_entry, list);
	/**/ 	list_del_init(_e);
	/**/ }
	spin_unlock(&q->lock);
	return e;
}

/**
 * Return rd_entry to the rd_queue when it is no longer of use, so that it can
 * be reused later.
 * @param e Pointer to the rd_entry to return.
 */
static inline void rd_entry_del(struct rd_entry * e) {
	struct rd_queue *q = e->queue;
	spin_lock(&q->lock);
	/**/ list_add_tail(&q->free, &e->list);
	spin_unlock(&q->lock);
}

/**
 * Put rd_entry to the end of the input FIFO, so that it will be processed
 * after all entries put before.
 * @param e Pointer to the rd_entry to put.
 */
static inline void rd_entry_put(struct rd_entry *e) {
	struct rd_queue *q = e->queue;
	spin_lock(&q->lock);
	/**/ list_add_tail(&q->fifo, &e->list);
	spin_unlock(&q->lock);
}

/**
 * Check if a subsequent rd_entry_get() would succeed.
 * @param q Pointer to the rd_queue to check.
 * @return Value != 0 if a subsequent rd_entry_get() would succeed.
 */
static inline int rd_entry_can_get(struct rd_queue *q) {
	int empty;
	spin_lock(&q->lock);
	/**/ empty = list_empty(&q->fifo);
	spin_unlock(&q->lock);
	return !empty;
}

/**
 * Get a new rd_entry to be processed from the rd_queue. Do never try to
 * delete this rd_entry. Use rd_entry_del() to return this rd_entry to the
 * rd_queue.
 * @param q Pointer to the rd_queue.
 * @return Pointer to the rd_entry got from the rd_queue.
 * @error  return NULL, if no rd_entry is available.
 */
static inline struct rd_entry *rd_entry_get(struct rd_queue *q) {
	struct rd_entry  *e = NULL;
	spin_lock(&q->lock);
	/**/ if (!list_empty(&q->fifo)) {
	/**/ 	struct list_head *_e = q->fifo.next;
	/**/ 	e = list_entry(_e, struct rd_entry, list);
	/**/ 	list_del_init(_e);
	/**/ }
	spin_unlock(&q->lock);
	return e;
}

#endif /* _RD_QUEUE_H_ */
