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
#include <linux/slab.h>
#include <linux/ip.h>

#include "tx_queue.h"

struct tx_queue *tx_queue_new(size_t size) {
	int    i;
	struct tx_queue *q = kmalloc(
			sizeof(struct tx_queue) + sizeof(struct tx_entry) * size,
			GFP_KERNEL
	);

	if (!q) {
		printk(KERN_ERR "spi-daisy: Unable to alloc tx_queue()\n");
		return NULL;
	}

	INIT_LIST_HEAD(&q->free);
	INIT_LIST_HEAD(&q->prio);
	INIT_LIST_HEAD(&q->fifo);
	sema_init(&q->sem, size);
	spin_lock_init(&q->lock);
	for (i = 0; i < size; i++) {
		struct tx_entry *e = &q->data[i];
		INIT_LIST_HEAD(&e->list);
		e->queue = q;
	} //end for //

	return q;
}

void tx_queue_del(struct tx_queue *q) {
	if (q) {
		struct tx_entry *e = tx_entry_get(q);
		while (e) {
			if (e->skb) {
				dev_kfree_skb(e->skb);
				e->skb = NULL;
			}
			e = tx_entry_get(q);
		} // end while //
		e = tx_entry_new(q);
		while (e) {
			if (e->skb) {
				dev_kfree_skb(e->skb);
				e->skb = NULL;
			}
			e = tx_entry_new(q);
		} // end while //
		kfree(q);
	}
}
